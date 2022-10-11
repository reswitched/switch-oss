/*
 * Copyright (C) 2008, 2010 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Google Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#include "config.h"

#include "Worker.h"

#include "DOMWindow.h"
#include "CachedResourceLoader.h"
#include "Document.h"
#include "EventException.h"
#include "EventListener.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "InspectorInstrumentation.h"
#include "MessageEvent.h"
#include "NetworkStateNotifier.h"
#include "TextEncoding.h"
#include "WorkerGlobalScopeProxy.h"
#include "WorkerScriptLoader.h"
#include "WorkerThread.h"
#include <wtf/HashSet.h>
#include <wtf/MainThread.h>

namespace WebCore {

#if !PLATFORM(WKC)
static HashSet<Worker*>* allWorkers;
#else
WKC_DEFINE_GLOBAL_PTR(HashSet<Worker*>*, allWorkers, 0);
#endif

void networkStateChanged(bool isOnLine)
{
    for (auto& worker : *allWorkers)
        worker->notifyNetworkStateChange(isOnLine);
}

inline Worker::Worker(ScriptExecutionContext& context)
    : ActiveDOMObject(&context)
    , m_contextProxy(WorkerGlobalScopeProxy::create(this))
{
    if (!allWorkers) {
        allWorkers = new HashSet<Worker*>;
        networkStateNotifier().addNetworkStateChangeListener(networkStateChanged);
    }

    HashSet<Worker*>::AddResult addResult = allWorkers->add(this);
    ASSERT_UNUSED(addResult, addResult.isNewEntry);
}

RefPtr<Worker> Worker::create(ScriptExecutionContext& context, const String& url, ExceptionCode& ec)
{
    ASSERT(isMainThread());

    // We don't currently support nested workers, so workers can only be created from documents.
    ASSERT_WITH_SECURITY_IMPLICATION(context.isDocument());

    Ref<Worker> worker = adoptRef(*new Worker(context));

    worker->suspendIfNeeded();

    URL scriptURL = worker->resolveURL(url, ec);
    if (scriptURL.isEmpty())
        return nullptr;

    // The worker context does not exist while loading, so we must ensure that the worker object is not collected, nor are its event listeners.
    worker->setPendingActivity(worker.ptr());

    worker->m_scriptLoader = WorkerScriptLoader::create();
    worker->m_scriptLoader->loadAsynchronously(&context, scriptURL, DenyCrossOriginRequests, worker.ptr());

    return WTF::move(worker);
}

Worker::~Worker()
{
    ASSERT(isMainThread());
    ASSERT(scriptExecutionContext()); // The context is protected by worker context proxy, so it cannot be destroyed while a Worker exists.
    allWorkers->remove(this);
    m_contextProxy->workerObjectDestroyed();
}

void Worker::postMessage(PassRefPtr<SerializedScriptValue> message, MessagePort* port, ExceptionCode& ec)
{
    MessagePortArray ports;
    if (port)
        ports.append(port);
    postMessage(message, &ports, ec);
}

void Worker::postMessage(PassRefPtr<SerializedScriptValue> message, const MessagePortArray* ports, ExceptionCode& ec)
{
    // Disentangle the port in preparation for sending it to the remote context.
    std::unique_ptr<MessagePortChannelArray> channels = MessagePort::disentanglePorts(ports, ec);
    if (ec)
        return;
    m_contextProxy->postMessageToWorkerGlobalScope(message, WTF::move(channels));
}

void Worker::terminate()
{
    m_contextProxy->terminateWorkerGlobalScope();
}

bool Worker::canSuspendForPageCache() const
{
    // FIXME: It is not currently possible to suspend a worker, so pages with workers can not go into page cache.
    return false;
}

const char* Worker::activeDOMObjectName() const
{
    return "Worker";
}

void Worker::stop()
{
    terminate();
}

bool Worker::hasPendingActivity() const
{
    return m_contextProxy->hasPendingActivity() || ActiveDOMObject::hasPendingActivity();
}

void Worker::notifyNetworkStateChange(bool isOnLine)
{
    m_contextProxy->notifyNetworkStateChange(isOnLine);
}

void Worker::didReceiveResponse(unsigned long identifier, const ResourceResponse&)
{
    InspectorInstrumentation::didReceiveScriptResponse(scriptExecutionContext(), identifier);
}

void Worker::notifyFinished()
{
    if (m_scriptLoader->failed())
        dispatchEvent(Event::create(eventNames().errorEvent, false, true));
    else {
        WorkerThreadStartMode startMode = DontPauseWorkerGlobalScopeOnStart;
        if (InspectorInstrumentation::shouldPauseDedicatedWorkerOnStart(scriptExecutionContext()))
            startMode = PauseWorkerGlobalScopeOnStart;
        m_contextProxy->startWorkerGlobalScope(m_scriptLoader->url(), scriptExecutionContext()->userAgent(m_scriptLoader->url()), m_scriptLoader->script(), startMode);
        InspectorInstrumentation::scriptImported(scriptExecutionContext(), m_scriptLoader->identifier(), m_scriptLoader->script());
    }
    m_scriptLoader = nullptr;

    unsetPendingActivity(this);
}

} // namespace WebCore

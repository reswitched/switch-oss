/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "WorkerMessagingProxy.h"

#include "ContentSecurityPolicy.h"
#include "DOMWindow.h"
#include "DedicatedWorkerGlobalScope.h"
#include "DedicatedWorkerThread.h"
#include "Document.h"
#include "ErrorEvent.h"
#include "Event.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "InspectorInstrumentation.h"
#include "MessageEvent.h"
#include "PageGroup.h"
#include "ScriptExecutionContext.h"
#include "Worker.h"
#include "WorkerDebuggerAgent.h"
#include "WorkerInspectorController.h"
#include <inspector/InspectorAgentBase.h>
#include <inspector/ScriptCallStack.h>
#include <runtime/ConsoleTypes.h>
#include <wtf/MainThread.h>

namespace WebCore {

WorkerGlobalScopeProxy* WorkerGlobalScopeProxy::create(Worker* worker)
{
    return new WorkerMessagingProxy(worker);
}

WorkerMessagingProxy::WorkerMessagingProxy(Worker* workerObject)
    : m_scriptExecutionContext(workerObject->scriptExecutionContext())
    , m_workerObject(workerObject)
    , m_mayBeDestroyed(false)
    , m_unconfirmedMessageCount(0)
    , m_workerThreadHadPendingActivity(false)
    , m_askedToTerminate(false)
    , m_pageInspector(nullptr)
{
    ASSERT(m_workerObject);
    ASSERT((is<Document>(*m_scriptExecutionContext) && isMainThread())
        || (is<WorkerGlobalScope>(*m_scriptExecutionContext) && currentThread() == downcast<WorkerGlobalScope>(*m_scriptExecutionContext).thread().threadID()));

    // Nobody outside this class ref counts this object. The original ref
    // is balanced by the deref in workerGlobalScopeDestroyedInternal.
}

WorkerMessagingProxy::~WorkerMessagingProxy()
{
    ASSERT(!m_workerObject);
    ASSERT((is<Document>(*m_scriptExecutionContext) && isMainThread())
        || (is<WorkerGlobalScope>(*m_scriptExecutionContext) && currentThread() == downcast<WorkerGlobalScope>(*m_scriptExecutionContext).thread().threadID()));
}

void WorkerMessagingProxy::startWorkerGlobalScope(const URL& scriptURL, const String& userAgent, const String& sourceCode, WorkerThreadStartMode startMode)
{
    // FIXME: This need to be revisited when we support nested worker one day
    ASSERT(m_scriptExecutionContext);
    Document& document = downcast<Document>(*m_scriptExecutionContext);
    RefPtr<DedicatedWorkerThread> thread = DedicatedWorkerThread::create(scriptURL, userAgent, sourceCode, *this, *this, startMode, document.contentSecurityPolicy()->deprecatedHeader(), document.contentSecurityPolicy()->deprecatedHeaderType(), document.topOrigin());
    workerThreadCreated(thread);
    thread->start();
    InspectorInstrumentation::didStartWorkerGlobalScope(m_scriptExecutionContext.get(), this, scriptURL);
}

void WorkerMessagingProxy::postMessageToWorkerObject(PassRefPtr<SerializedScriptValue> message, std::unique_ptr<MessagePortChannelArray> channels)
{
    MessagePortChannelArray* channelsPtr = channels.release();
#if !PLATFORM(WKC)
    m_scriptExecutionContext->postTask([this, channelsPtr, message] (ScriptExecutionContext& context) {
        Worker* workerObject = this->workerObject();
        if (!workerObject || askedToTerminate())
            return;

        std::unique_ptr<MessagePortArray> ports = MessagePort::entanglePorts(context, std::unique_ptr<MessagePortChannelArray>(channelsPtr));
        workerObject->dispatchEvent(MessageEvent::create(WTF::move(ports), message));
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [this, channelsPtr, message](ScriptExecutionContext& context) {
        Worker* workerObject = this->workerObject();
        if (!workerObject || askedToTerminate())
            return;

        std::unique_ptr<MessagePortArray> ports = MessagePort::entanglePorts(context, std::unique_ptr<MessagePortChannelArray>(channelsPtr));
        workerObject->dispatchEvent(MessageEvent::create(WTF::move(ports), message));
    });
    m_scriptExecutionContext->postTask(p);
#endif
}

void WorkerMessagingProxy::postMessageToWorkerGlobalScope(PassRefPtr<SerializedScriptValue> message, std::unique_ptr<MessagePortChannelArray> channels)
{
    if (m_askedToTerminate)
        return;

    MessagePortChannelArray* channelsPtr = channels.release();
#if !PLATFORM(WKC)
    ScriptExecutionContext::Task task([channelsPtr, message] (ScriptExecutionContext& scriptContext) {
        ASSERT_WITH_SECURITY_IMPLICATION(scriptContext.isWorkerGlobalScope());
        DedicatedWorkerGlobalScope& context = static_cast<DedicatedWorkerGlobalScope&>(scriptContext);
        std::unique_ptr<MessagePortArray> ports = MessagePort::entanglePorts(scriptContext, std::unique_ptr<MessagePortChannelArray>(channelsPtr));
        context.dispatchEvent(MessageEvent::create(WTF::move(ports), message));
        context.thread().workerObjectProxy().confirmMessageFromWorkerObject(context.hasPendingActivity());
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [channelsPtr, message](ScriptExecutionContext& scriptContext) {
        ASSERT_WITH_SECURITY_IMPLICATION(scriptContext.isWorkerGlobalScope());
        DedicatedWorkerGlobalScope& context = static_cast<DedicatedWorkerGlobalScope&>(scriptContext);
        std::unique_ptr<MessagePortArray> ports = MessagePort::entanglePorts(scriptContext, std::unique_ptr<MessagePortChannelArray>(channelsPtr));
        context.dispatchEvent(MessageEvent::create(WTF::move(ports), message));
        context.thread().workerObjectProxy().confirmMessageFromWorkerObject(context.hasPendingActivity());
    });
    ScriptExecutionContext::Task task(p);
#endif

    if (m_workerThread) {
        ++m_unconfirmedMessageCount;
        m_workerThread->runLoop().postTask(WTF::move(task));
    } else
        m_queuedEarlyTasks.append(std::make_unique<ScriptExecutionContext::Task>(WTF::move(task)));
}

void WorkerMessagingProxy::postTaskToLoader(ScriptExecutionContext::Task task)
{
    // FIXME: In case of nested workers, this should go directly to the root Document context.
    ASSERT(m_scriptExecutionContext->isDocument());
    m_scriptExecutionContext->postTask(WTF::move(task));
}

bool WorkerMessagingProxy::postTaskForModeToWorkerGlobalScope(ScriptExecutionContext::Task task, const String& mode)
{
    if (m_askedToTerminate)
        return false;

    ASSERT(m_workerThread);
    m_workerThread->runLoop().postTaskForMode(WTF::move(task), mode);
    return true;
}

void WorkerMessagingProxy::postExceptionToWorkerObject(const String& errorMessage, int lineNumber, int columnNumber, const String& sourceURL)
{
    StringCapture capturedErrorMessage(errorMessage);
    StringCapture capturedSourceURL(sourceURL);
#if !PLATFORM(WKC)
    m_scriptExecutionContext->postTask([this, capturedErrorMessage, capturedSourceURL, lineNumber, columnNumber] (ScriptExecutionContext& context) {
        Worker* workerObject = this->workerObject();
        if (!workerObject)
            return;

        // We don't bother checking the askedToTerminate() flag here, because exceptions should *always* be reported even if the thread is terminated.
        // This is intentionally different than the behavior in MessageWorkerTask, because terminated workers no longer deliver messages (section 4.6 of the WebWorker spec), but they do report exceptions.

        bool errorHandled = !workerObject->dispatchEvent(ErrorEvent::create(capturedErrorMessage.string(), capturedSourceURL.string(), lineNumber, columnNumber));
        if (!errorHandled)
            context.reportException(capturedErrorMessage.string(), lineNumber, columnNumber, capturedSourceURL.string(), 0);
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [this, capturedErrorMessage, capturedSourceURL, lineNumber, columnNumber](ScriptExecutionContext& context) {
        Worker* workerObject = this->workerObject();
        if (!workerObject)
            return;

        // We don't bother checking the askedToTerminate() flag here, because exceptions should *always* be reported even if the thread is terminated.
        // This is intentionally different than the behavior in MessageWorkerTask, because terminated workers no longer deliver messages (section 4.6 of the WebWorker spec), but they do report exceptions.

        bool errorHandled = !workerObject->dispatchEvent(ErrorEvent::create(capturedErrorMessage.string(), capturedSourceURL.string(), lineNumber, columnNumber));
        if (!errorHandled)
            context.reportException(capturedErrorMessage.string(), lineNumber, columnNumber, capturedSourceURL.string(), 0);
    });
    m_scriptExecutionContext->postTask(p);
#endif
}

void WorkerMessagingProxy::postConsoleMessageToWorkerObject(MessageSource source, MessageLevel level, const String& message, int lineNumber, int columnNumber, const String& sourceURL)
{
    StringCapture capturedMessage(message);
    StringCapture capturedSourceURL(sourceURL);
#if !PLATFORM(WKC)
    m_scriptExecutionContext->postTask([this, source, level, capturedMessage, capturedSourceURL, lineNumber, columnNumber] (ScriptExecutionContext& context) {
        if (askedToTerminate())
            return;
        context.addConsoleMessage(source, level, capturedMessage.string(), capturedSourceURL.string(), lineNumber, columnNumber);
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [this, source, level, capturedMessage, capturedSourceURL, lineNumber, columnNumber](ScriptExecutionContext& context) {
        if (askedToTerminate())
            return;
        context.addConsoleMessage(source, level, capturedMessage.string(), capturedSourceURL.string(), lineNumber, columnNumber);
    });
    m_scriptExecutionContext->postTask(p);
#endif
}

void WorkerMessagingProxy::workerThreadCreated(PassRefPtr<DedicatedWorkerThread> workerThread)
{
    m_workerThread = workerThread;

    if (m_askedToTerminate) {
        // Worker.terminate() could be called from JS before the thread was created.
        m_workerThread->stop();
    } else {
        ASSERT(!m_unconfirmedMessageCount);
        m_unconfirmedMessageCount = m_queuedEarlyTasks.size();
        m_workerThreadHadPendingActivity = true; // Worker initialization means a pending activity.

        auto queuedEarlyTasks = WTF::move(m_queuedEarlyTasks);
        for (auto& task : queuedEarlyTasks)
            m_workerThread->runLoop().postTask(WTF::move(*task));
    }
}

void WorkerMessagingProxy::workerObjectDestroyed()
{
    m_workerObject = 0;
#if !PLATFORM(WKC)
    m_scriptExecutionContext->postTask([this] (ScriptExecutionContext&) {
        m_mayBeDestroyed = true;
        if (m_workerThread)
            terminateWorkerGlobalScope();
        else
            workerGlobalScopeDestroyedInternal();
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [this](ScriptExecutionContext&) {
        m_mayBeDestroyed = true;
        if (m_workerThread)
            terminateWorkerGlobalScope();
        else
            workerGlobalScopeDestroyedInternal();
    });
    m_scriptExecutionContext->postTask(p);
#endif
}

void WorkerMessagingProxy::notifyNetworkStateChange(bool isOnline)
{
    if (m_askedToTerminate)
        return;

    if (!m_workerThread)
        return;

#if !PLATFORM(WKC)
    m_workerThread->runLoop().postTask([isOnline] (ScriptExecutionContext& context) {
        downcast<WorkerGlobalScope>(context).dispatchEvent(Event::create(isOnline ? eventNames().onlineEvent : eventNames().offlineEvent, false, false));
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [isOnline](ScriptExecutionContext& context) {
        downcast<WorkerGlobalScope>(context).dispatchEvent(Event::create(isOnline ? eventNames().onlineEvent : eventNames().offlineEvent, false, false));
    });
    m_workerThread->runLoop().postTask(p);
#endif
}

void WorkerMessagingProxy::connectToInspector(WorkerGlobalScopeProxy::PageInspector* pageInspector)
{
    if (m_askedToTerminate)
        return;
    ASSERT(!m_pageInspector);
    m_pageInspector = pageInspector;
#if !PLATFORM(WKC)
    m_workerThread->runLoop().postTaskForMode([] (ScriptExecutionContext& context) {
        downcast<WorkerGlobalScope>(context).workerInspectorController().connectFrontend();
    }, WorkerDebuggerAgent::debuggerTaskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [](ScriptExecutionContext& context) {
        downcast<WorkerGlobalScope>(context).workerInspectorController().connectFrontend();
    });
    m_workerThread->runLoop().postTaskForMode(p, WorkerDebuggerAgent::debuggerTaskMode);
#endif
}

void WorkerMessagingProxy::disconnectFromInspector()
{
    m_pageInspector = nullptr;
    if (m_askedToTerminate)
        return;
#if !PLATFORM(WKC)
    m_workerThread->runLoop().postTaskForMode([] (ScriptExecutionContext& context) {
        downcast<WorkerGlobalScope>(context).workerInspectorController().disconnectFrontend(Inspector::DisconnectReason::InspectorDestroyed);
    }, WorkerDebuggerAgent::debuggerTaskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [](ScriptExecutionContext& context) {
        downcast<WorkerGlobalScope>(context).workerInspectorController().disconnectFrontend(Inspector::DisconnectReason::InspectorDestroyed);
    });
    m_workerThread->runLoop().postTaskForMode(p, WorkerDebuggerAgent::debuggerTaskMode);
#endif
}

void WorkerMessagingProxy::sendMessageToInspector(const String& message)
{
    if (m_askedToTerminate)
        return;
    StringCapture capturedMessage(message);
#if !PLATFORM(WKC)
    m_workerThread->runLoop().postTaskForMode([capturedMessage] (ScriptExecutionContext& context) {
        downcast<WorkerGlobalScope>(context).workerInspectorController().dispatchMessageFromFrontend(capturedMessage.string());
    }, WorkerDebuggerAgent::debuggerTaskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [capturedMessage](ScriptExecutionContext& context) {
        downcast<WorkerGlobalScope>(context).workerInspectorController().dispatchMessageFromFrontend(capturedMessage.string());
    });
    m_workerThread->runLoop().postTaskForMode(p, WorkerDebuggerAgent::debuggerTaskMode);
#endif
    WorkerDebuggerAgent::interruptAndDispatchInspectorCommands(m_workerThread.get());
}

void WorkerMessagingProxy::workerGlobalScopeDestroyed()
{
#if !PLATFORM(WKC)
    m_scriptExecutionContext->postTask([this] (ScriptExecutionContext&) {
        workerGlobalScopeDestroyedInternal();
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [this](ScriptExecutionContext&) {
        workerGlobalScopeDestroyedInternal();
    });
    m_scriptExecutionContext->postTask(p);
#endif
    // Will execute workerGlobalScopeDestroyedInternal() on context's thread.
}

void WorkerMessagingProxy::workerGlobalScopeClosed()
{
    // Executes terminateWorkerGlobalScope() on parent context's thread.
#if !PLATFORM(WKC)
    m_scriptExecutionContext->postTask([this] (ScriptExecutionContext&) {
        terminateWorkerGlobalScope();
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [this](ScriptExecutionContext&) {
        terminateWorkerGlobalScope();
    });
    m_scriptExecutionContext->postTask(p);
#endif
}

void WorkerMessagingProxy::workerGlobalScopeDestroyedInternal()
{
    // WorkerGlobalScopeDestroyedTask is always the last to be performed, so the proxy is not needed for communication
    // in either side any more. However, the Worker object may still exist, and it assumes that the proxy exists, too.
    m_askedToTerminate = true;
    m_workerThread = nullptr;

    InspectorInstrumentation::workerGlobalScopeTerminated(m_scriptExecutionContext.get(), this);

    // This balances the original ref in construction.
    if (m_mayBeDestroyed)
        deref();
}

void WorkerMessagingProxy::terminateWorkerGlobalScope()
{
    if (m_askedToTerminate)
        return;
    m_askedToTerminate = true;

    if (m_workerThread)
        m_workerThread->stop();

    InspectorInstrumentation::workerGlobalScopeTerminated(m_scriptExecutionContext.get(), this);
}

void WorkerMessagingProxy::postMessageToPageInspector(const String& message)
{
    StringCapture capturedMessage(message);
#if !PLATFORM(WKC)
    m_scriptExecutionContext->postTask([this, capturedMessage] (ScriptExecutionContext&) {
        if (m_pageInspector)
            m_pageInspector->dispatchMessageFromWorker(capturedMessage.string());
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [this, capturedMessage](ScriptExecutionContext&) {
        if (m_pageInspector && !m_mayBeDestroyed)
            m_pageInspector->dispatchMessageFromWorker(capturedMessage.string());
    });
    m_scriptExecutionContext->postTask(p);
#endif
}

void WorkerMessagingProxy::confirmMessageFromWorkerObject(bool hasPendingActivity)
{
#if !PLATFORM(WKC)
    m_scriptExecutionContext->postTask([this, hasPendingActivity] (ScriptExecutionContext&) {
        reportPendingActivityInternal(true, hasPendingActivity);
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [this, hasPendingActivity](ScriptExecutionContext&) {
        reportPendingActivityInternal(true, hasPendingActivity);
    });
    m_scriptExecutionContext->postTask(p);
#endif
    // Will execute reportPendingActivityInternal() on context's thread.
}

void WorkerMessagingProxy::reportPendingActivity(bool hasPendingActivity)
{
#if !PLATFORM(WKC)
    m_scriptExecutionContext->postTask([this, hasPendingActivity] (ScriptExecutionContext&) {
        reportPendingActivityInternal(false, hasPendingActivity);
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [this, hasPendingActivity](ScriptExecutionContext&) {
        reportPendingActivityInternal(false, hasPendingActivity);
    });
    m_scriptExecutionContext->postTask(p);
#endif
    // Will execute reportPendingActivityInternal() on context's thread.
}

void WorkerMessagingProxy::reportPendingActivityInternal(bool confirmingMessage, bool hasPendingActivity)
{
    if (confirmingMessage && !m_askedToTerminate) {
        ASSERT(m_unconfirmedMessageCount);
        --m_unconfirmedMessageCount;
    }

    m_workerThreadHadPendingActivity = hasPendingActivity;
}

bool WorkerMessagingProxy::hasPendingActivity() const
{
    return (m_unconfirmedMessageCount || m_workerThreadHadPendingActivity) && !m_askedToTerminate;
}

} // namespace WebCore

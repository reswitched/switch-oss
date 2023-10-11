/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InspectorWorkerAgent.h"

#include "InspectorForwarding.h"
#include "InstrumentingAgents.h"
#include "URL.h"
#include "WorkerGlobalScopeProxy.h"
#if !PLATFORM(WKC)
#include <inspector/InspectorFrontendDispatchers.h>
#include <inspector/InspectorValues.h>
#else
#include <InspectorFrontendDispatchers.h>
#include <InspectorValues.h>
#endif
#include <wtf/RefPtr.h>

using namespace Inspector;

namespace WebCore {

class InspectorWorkerAgent::WorkerFrontendChannel : public WorkerGlobalScopeProxy::PageInspector {
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit WorkerFrontendChannel(Inspector::WorkerFrontendDispatcher* frontendDispatcher, WorkerGlobalScopeProxy* proxy)
        : m_frontendDispatcher(frontendDispatcher)
        , m_proxy(proxy)
        , m_id(s_nextId++)
        , m_connected(false)
    {
    }
    virtual ~WorkerFrontendChannel()
    {
        disconnectFromWorkerGlobalScope();
    }

    int id() const { return m_id; }
    WorkerGlobalScopeProxy* proxy() const { return m_proxy; }

    void connectToWorkerGlobalScope()
    {
        if (m_connected)
            return;
        m_connected = true;
        m_proxy->connectToInspector(this);
    }

    void disconnectFromWorkerGlobalScope()
    {
        if (!m_connected)
            return;
        m_connected = false;
        m_proxy->disconnectFromInspector();
    }

private:
    // WorkerGlobalScopeProxy::PageInspector implementation
    virtual void dispatchMessageFromWorker(const String& messageString) override
    {
        RefPtr<InspectorValue> parsedMessage;
        if (!InspectorValue::parseJSON(messageString, parsedMessage))
            return;

        RefPtr<InspectorObject> messageObject;
        if (!parsedMessage->asObject(messageObject))
            return;

        m_frontendDispatcher->dispatchMessageFromWorker(m_id, messageObject);
    }

    Inspector::WorkerFrontendDispatcher* m_frontendDispatcher;
    WorkerGlobalScopeProxy* m_proxy;
    int m_id;
    bool m_connected;
    static int s_nextId;
};

int InspectorWorkerAgent::WorkerFrontendChannel::s_nextId = 1;

InspectorWorkerAgent::InspectorWorkerAgent(InstrumentingAgents* instrumentingAgents)
    : InspectorAgentBase(ASCIILiteral("Worker"), instrumentingAgents)
    , m_enabled(false)
    , m_shouldPauseDedicatedWorkerOnStart(false)
{
    m_instrumentingAgents->setInspectorWorkerAgent(this);
}

InspectorWorkerAgent::~InspectorWorkerAgent()
{
    m_instrumentingAgents->setInspectorWorkerAgent(nullptr);
}

void InspectorWorkerAgent::didCreateFrontendAndBackend(Inspector::FrontendChannel* frontendChannel, Inspector::BackendDispatcher* backendDispatcher)
{
    m_frontendDispatcher = std::make_unique<Inspector::WorkerFrontendDispatcher>(frontendChannel);
    m_backendDispatcher = Inspector::WorkerBackendDispatcher::create(backendDispatcher, this);
}

void InspectorWorkerAgent::willDestroyFrontendAndBackend(Inspector::DisconnectReason)
{
    m_shouldPauseDedicatedWorkerOnStart = false;
    ErrorString unused;
    disable(unused);

    m_frontendDispatcher = nullptr;
    m_backendDispatcher = nullptr;
}

void InspectorWorkerAgent::enable(ErrorString&)
{
    m_enabled = true;
    if (!m_frontendDispatcher)
        return;

    createWorkerFrontendChannelsForExistingWorkers();
}

void InspectorWorkerAgent::disable(ErrorString&)
{
    m_enabled = false;
    if (!m_frontendDispatcher)
        return;

    destroyWorkerFrontendChannels();
}

void InspectorWorkerAgent::canInspectWorkers(ErrorString&, bool* result)
{
    *result = true;
}

void InspectorWorkerAgent::connectToWorker(ErrorString& error, int workerId)
{
    WorkerFrontendChannel* channel = m_idToChannel.get(workerId);
    if (channel)
        channel->connectToWorkerGlobalScope();
    else
        error = ASCIILiteral("Worker is gone");
}

void InspectorWorkerAgent::disconnectFromWorker(ErrorString& error, int workerId)
{
    WorkerFrontendChannel* channel = m_idToChannel.get(workerId);
    if (channel)
        channel->disconnectFromWorkerGlobalScope();
    else
        error = ASCIILiteral("Worker is gone");
}

void InspectorWorkerAgent::sendMessageToWorker(ErrorString& error, int workerId, const InspectorObject& message)
{
    WorkerFrontendChannel* channel = m_idToChannel.get(workerId);
    if (channel)
        channel->proxy()->sendMessageToInspector(message.toJSONString());
    else
        error = ASCIILiteral("Worker is gone");
}

void InspectorWorkerAgent::setAutoconnectToWorkers(ErrorString&, bool value)
{
    m_shouldPauseDedicatedWorkerOnStart = value;
}

bool InspectorWorkerAgent::shouldPauseDedicatedWorkerOnStart() const
{
    return m_shouldPauseDedicatedWorkerOnStart;
}

void InspectorWorkerAgent::didStartWorkerGlobalScope(WorkerGlobalScopeProxy* workerGlobalScopeProxy, const URL& url)
{
    m_dedicatedWorkers.set(workerGlobalScopeProxy, url.string());
    if (m_frontendDispatcher && m_enabled)
        createWorkerFrontendChannel(workerGlobalScopeProxy, url.string());
}

void InspectorWorkerAgent::workerGlobalScopeTerminated(WorkerGlobalScopeProxy* proxy)
{
    m_dedicatedWorkers.remove(proxy);
    for (WorkerChannels::iterator it = m_idToChannel.begin(); it != m_idToChannel.end(); ++it) {
        if (proxy == it->value->proxy()) {
            m_frontendDispatcher->workerTerminated(it->key);
            delete it->value;
            m_idToChannel.remove(it);
            return;
        }
    }
}

void InspectorWorkerAgent::createWorkerFrontendChannelsForExistingWorkers()
{
    for (DedicatedWorkers::iterator it = m_dedicatedWorkers.begin(); it != m_dedicatedWorkers.end(); ++it)
        createWorkerFrontendChannel(it->key, it->value);
}

void InspectorWorkerAgent::destroyWorkerFrontendChannels()
{
    for (WorkerChannels::iterator it = m_idToChannel.begin(); it != m_idToChannel.end(); ++it) {
        it->value->disconnectFromWorkerGlobalScope();
        delete it->value;
    }
    m_idToChannel.clear();
}

void InspectorWorkerAgent::createWorkerFrontendChannel(WorkerGlobalScopeProxy* workerGlobalScopeProxy, const String& url)
{
    WorkerFrontendChannel* channel = new WorkerFrontendChannel(m_frontendDispatcher.get(), workerGlobalScopeProxy);
    m_idToChannel.set(channel->id(), channel);

    ASSERT(m_frontendDispatcher);
    if (m_shouldPauseDedicatedWorkerOnStart)
        channel->connectToWorkerGlobalScope();
    m_frontendDispatcher->workerCreated(channel->id(), url, m_shouldPauseDedicatedWorkerOnStart);
}

} // namespace WebCore

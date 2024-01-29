/*
 * Copyright (C) 2009, 2012 Ericsson AB. All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#ifndef EventSource_h
#define EventSource_h

#include "ActiveDOMObject.h"
#include "EventTarget.h"
#include "URL.h"
#include "ThreadableLoaderClient.h"
#include "Timer.h"
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class Dictionary;
class MessageEvent;
class ResourceResponse;
class TextResourceDecoder;
class ThreadableLoader;

class EventSource final : public RefCounted<EventSource>, public EventTargetWithInlineData, private ThreadableLoaderClient, public ActiveDOMObject {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static RefPtr<EventSource> create(ScriptExecutionContext&, const String& url, const Dictionary&, ExceptionCode&);
    virtual ~EventSource();

    static const unsigned long long defaultReconnectDelay;

    String url() const;
    bool withCredentials() const;

    typedef short State;
    static const State CONNECTING = 0;
    static const State OPEN = 1;
    static const State CLOSED = 2;

    State readyState() const;

    void close();

    using RefCounted<EventSource>::ref;
    using RefCounted<EventSource>::deref;

    virtual EventTargetInterface eventTargetInterface() const override { return EventSourceEventTargetInterfaceType; }
    virtual ScriptExecutionContext* scriptExecutionContext() const override { return ActiveDOMObject::scriptExecutionContext(); }

private:
    EventSource(ScriptExecutionContext&, const URL&, const Dictionary&);

    virtual void refEventTarget() override { ref(); }
    virtual void derefEventTarget() override { deref(); }

    virtual void didReceiveResponse(unsigned long, const ResourceResponse&) override;
    virtual void didReceiveData(const char*, int) override;
    virtual void didFinishLoading(unsigned long, double) override;
    virtual void didFail(const ResourceError&) override;
    virtual void didFailAccessControlCheck(const ResourceError&) override;
    virtual void didFailRedirectCheck() override;

    // ActiveDOMObject API.
    void stop() override;
    const char* activeDOMObjectName() const override;
    bool canSuspendForPageCache() const override;

    void connect();
    void networkRequestEnded();
    void scheduleInitialConnect();
    void scheduleReconnect();
    void connectTimerFired();
    void abortConnectionAttempt();
    void parseEventStream();
    void parseEventStreamLine(unsigned pos, int fieldLength, int lineLength);
    PassRefPtr<MessageEvent> createMessageEvent();

    URL m_url;
    bool m_withCredentials;
    State m_state;

    RefPtr<TextResourceDecoder> m_decoder;
    RefPtr<ThreadableLoader> m_loader;
    Timer m_connectTimer;
    Vector<UChar> m_receiveBuf;
    bool m_discardTrailingNewline;
    bool m_requestInFlight;

    String m_eventName;
    Vector<UChar> m_data;
    String m_currentlyParsedEventId;
    String m_lastEventId;
    unsigned long long m_reconnectDelay;
    String m_eventStreamOrigin;
};

} // namespace WebCore

#endif // EventSource_h

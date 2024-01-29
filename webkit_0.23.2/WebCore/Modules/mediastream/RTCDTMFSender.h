/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RTCDTMFSender_h
#define RTCDTMFSender_h

#if ENABLE(MEDIA_STREAM)

#include "ActiveDOMObject.h"
#include "EventTarget.h"
#include "RTCDTMFSenderHandlerClient.h"
#include "ScriptWrappable.h"
#include "Timer.h"
#include <wtf/RefCounted.h>

namespace WebCore {

class MediaStreamTrack;
class RTCPeerConnectionHandler;
class RTCDTMFSenderHandler;

class RTCDTMFSender final : public RefCounted<RTCDTMFSender>, public ScriptWrappable, public EventTargetWithInlineData, public RTCDTMFSenderHandlerClient, public ActiveDOMObject {
public:
    static RefPtr<RTCDTMFSender> create(ScriptExecutionContext*, RTCPeerConnectionHandler*, PassRefPtr<MediaStreamTrack>, ExceptionCode&);
    ~RTCDTMFSender();

    bool canInsertDTMF() const;
    MediaStreamTrack* track() const;
    String toneBuffer() const;
    long duration() const { return m_duration; }
    long interToneGap() const { return m_interToneGap; }

    void insertDTMF(const String& tones, ExceptionCode&);
    void insertDTMF(const String& tones, long duration, ExceptionCode&);
    void insertDTMF(const String& tones, long duration, long interToneGap, ExceptionCode&);

    // EventTarget
    virtual EventTargetInterface eventTargetInterface() const override { return RTCDTMFSenderEventTargetInterfaceType; }
    virtual ScriptExecutionContext* scriptExecutionContext() const override { return ActiveDOMObject::scriptExecutionContext(); }

    using RefCounted<RTCDTMFSender>::ref;
    using RefCounted<RTCDTMFSender>::deref;

private:
    RTCDTMFSender(ScriptExecutionContext*, PassRefPtr<MediaStreamTrack>, std::unique_ptr<RTCDTMFSenderHandler>);

    // ActiveDOMObject
    void stop() override;
    const char* activeDOMObjectName() const override;
    bool canSuspendForPageCache() const override;

    void scheduleDispatchEvent(PassRefPtr<Event>);
    void scheduledEventTimerFired();

    // EventTarget
    virtual void refEventTarget() override { ref(); }
    virtual void derefEventTarget() override { deref(); }

    // RTCDTMFSenderHandlerClient
    virtual void didPlayTone(const String&) override;

    RefPtr<MediaStreamTrack> m_track;
    long m_duration;
    long m_interToneGap;

    std::unique_ptr<RTCDTMFSenderHandler> m_handler;

    bool m_stopped;

    Timer m_scheduledEventTimer;
    Vector<RefPtr<Event>> m_scheduledEvents;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // RTCDTMFSender_h

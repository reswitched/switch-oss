/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2011, 2015 Ericsson AB. All rights reserved.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MediaStream_h
#define MediaStream_h

#if ENABLE(MEDIA_STREAM)

#include "ContextDestructionObserver.h"
#include "EventTarget.h"
#include "ExceptionBase.h"
#include "MediaStreamPrivate.h"
#include "MediaStreamTrack.h"
#include "ScriptWrappable.h"
#include "Timer.h"
#include "URLRegistry.h"
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class MediaStream final : public URLRegistrable, public ScriptWrappable, public MediaStreamPrivateClient, public EventTargetWithInlineData, public ContextDestructionObserver, public MediaStreamTrack::Observer {
public:
    class Observer {
    public:
        virtual ~Observer() { }
        virtual void didAddOrRemoveTrack() = 0;
    };

    static Ref<MediaStream> create(ScriptExecutionContext&);
    static Ref<MediaStream> create(ScriptExecutionContext&, MediaStream*);
    static Ref<MediaStream> create(ScriptExecutionContext&, const Vector<RefPtr<MediaStreamTrack>>&);
    static Ref<MediaStream> create(ScriptExecutionContext&, RefPtr<MediaStreamPrivate>&&);
    virtual ~MediaStream();

    String id() const { return m_private->id(); }

    void addTrack(RefPtr<MediaStreamTrack>&&);
    void removeTrack(MediaStreamTrack*);
    MediaStreamTrack* getTrackById(String);

    Vector<RefPtr<MediaStreamTrack>> getAudioTracks() override final;
    Vector<RefPtr<MediaStreamTrack>> getVideoTracks() override final;
    Vector<RefPtr<MediaStreamTrack>> getTracks() const;

    RefPtr<MediaStream> clone();

    bool active() const { return m_isActive; }

    MediaStreamPrivate* privateStream() const { return m_private.get(); }

    // EventTarget
    virtual EventTargetInterface eventTargetInterface() const final { return MediaStreamEventTargetInterfaceType; }
    virtual ScriptExecutionContext* scriptExecutionContext() const final { return ContextDestructionObserver::scriptExecutionContext(); }

    using RefCounted<MediaStreamPrivateClient>::ref;
    using RefCounted<MediaStreamPrivateClient>::deref;

    // URLRegistrable
    virtual URLRegistry& registry() const override;

    void addObserver(Observer*);
    void removeObserver(Observer*);

protected:
    MediaStream(ScriptExecutionContext&, const Vector<RefPtr<MediaStreamTrack>>&);
    MediaStream(ScriptExecutionContext&, RefPtr<MediaStreamPrivate>&&);

    // ContextDestructionObserver
    virtual void contextDestroyed() override final;

private:
    enum class StreamModifier { DomAPI, Platform };

    // EventTarget
    virtual void refEventTarget() override final { ref(); }
    virtual void derefEventTarget() override final { deref(); }

    // MediaStreamTrack::Observer
    virtual void trackDidEnd() override final;

    // MediaStreamPrivateClient
    virtual void activeStatusChanged() override final;
    virtual void didAddTrackToPrivate(MediaStreamTrackPrivate&) override final;
    virtual void didRemoveTrackFromPrivate(MediaStreamTrackPrivate&) override final;

    bool internalAddTrack(RefPtr<MediaStreamTrack>&&, StreamModifier);
    bool internalRemoveTrack(RefPtr<MediaStreamTrack>&&, StreamModifier);

    void scheduleActiveStateChange();
    void activityEventTimerFired();

    Vector<RefPtr<MediaStreamTrack>> trackVectorForType(RealtimeMediaSource::Type) const;

    RefPtr<MediaStreamPrivate> m_private;

    bool m_isActive;
    HashMap<String, RefPtr<MediaStreamTrack>> m_trackSet;

    Timer m_activityEventTimer;
    Vector<RefPtr<Event>> m_scheduledActivityEvents;

    Vector<Observer*> m_observers;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // MediaStream_h

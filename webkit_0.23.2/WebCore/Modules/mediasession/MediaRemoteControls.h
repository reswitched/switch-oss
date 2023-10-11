/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
 */

#ifndef MediaRemoteControls_h
#define MediaRemoteControls_h

#if ENABLE(MEDIA_SESSION)

#include "EventTarget.h"
#include <wtf/RefCounted.h>

namespace WebCore {

class MediaRemoteControls : public RefCounted<MediaRemoteControls>, public EventTargetWithInlineData {
public:
    static Ref<MediaRemoteControls> create(ScriptExecutionContext& context)
    {
        return adoptRef(*new MediaRemoteControls(context));
    }

    bool previousTrackEnabled() const { return m_previousTrackEnabled; }
    void setPreviousTrackEnabled(bool enabled) { m_previousTrackEnabled = enabled; }

    bool nextTrackEnabled() const { return m_nextTrackEnabled; }
    void setNextTrackEnabled(bool enabled) { m_nextTrackEnabled = enabled; }

    using RefCounted<MediaRemoteControls>::ref;
    using RefCounted<MediaRemoteControls>::deref;

    virtual ~MediaRemoteControls();

    MediaRemoteControls(ScriptExecutionContext&);

    virtual EventTargetInterface eventTargetInterface() const override { return MediaRemoteControlsEventTargetInterfaceType; }
    virtual ScriptExecutionContext* scriptExecutionContext() const override { return &m_scriptExecutionContext; }

private:
    ScriptExecutionContext& m_scriptExecutionContext;

    bool m_previousTrackEnabled { false };
    bool m_nextTrackEnabled { false };

    virtual void refEventTarget() override final { ref(); }
    virtual void derefEventTarget() override final { deref(); }
};

} // namespace WebCore

#endif /* ENABLE(MEDIA_SESSION) */

#endif /* MediaRemoteControls_h */

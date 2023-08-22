/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#include "config.h"
#include "AudioTrackPrivateMediaSourceAVFObjC.h"

#if ENABLE(MEDIA_SOURCE) && ENABLE(VIDEO_TRACK)

#include "AVTrackPrivateAVFObjCImpl.h"
#include "SourceBufferPrivateAVFObjC.h"

namespace WebCore {

AudioTrackPrivateMediaSourceAVFObjC::AudioTrackPrivateMediaSourceAVFObjC(AVAssetTrack* track, SourceBufferPrivateAVFObjC* parent)
    : m_impl(std::make_unique<AVTrackPrivateAVFObjCImpl>(track))
    , m_parent(parent)
    , m_trackID(-1)
    , m_enabled(false)
{
    resetPropertiesFromTrack();
}

void AudioTrackPrivateMediaSourceAVFObjC::resetPropertiesFromTrack()
{
    m_trackID = m_impl->trackID();

    setKind(m_impl->audioKind());
    setId(m_impl->id());
    setLabel(m_impl->label());
    setLanguage(m_impl->language());
}

void AudioTrackPrivateMediaSourceAVFObjC::setAssetTrack(AVAssetTrack *track)
{
    m_impl = std::make_unique<AVTrackPrivateAVFObjCImpl>(track);
    resetPropertiesFromTrack();
}

AVAssetTrack* AudioTrackPrivateMediaSourceAVFObjC::assetTrack()
{
    return m_impl->assetTrack();
}

bool AudioTrackPrivateMediaSourceAVFObjC::enabled() const
{
    return m_enabled;
}

void AudioTrackPrivateMediaSourceAVFObjC::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    m_parent->trackDidChangeEnabled(this);
}

}

#endif

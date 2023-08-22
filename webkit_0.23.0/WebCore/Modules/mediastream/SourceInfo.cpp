/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "config.h"
#include "SourceInfo.h"

#if ENABLE(MEDIA_STREAM)

#include <wtf/NeverDestroyed.h>

namespace WebCore {

Ref<SourceInfo> SourceInfo::create(PassRefPtr<TrackSourceInfo> trackSourceInfo)
{
    return adoptRef(*new SourceInfo(trackSourceInfo));
}

SourceInfo::SourceInfo(PassRefPtr<TrackSourceInfo> trackSourceInfo)
    : m_trackSourceInfo(trackSourceInfo)
{
}

const AtomicString& SourceInfo::kind() const
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> audioKind("audio", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<AtomicString> videoKind("video", AtomicString::ConstructFromLiteral);

    switch (m_trackSourceInfo->kind()) {
    case TrackSourceInfo::Audio:
        return audioKind;
    case TrackSourceInfo::Video:
        return videoKind;
    }
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, audioKind, 0);
    WKC_DEFINE_STATIC_PTR(AtomicString*, videoKind, 0);
    if (!audioKind)
        audioKind = new AtomicString("audio", AtomicString::ConstructFromLiteral);
    if (!videoKind)
        videoKind = new AtomicString("video", AtomicString::ConstructFromLiteral);

    switch (m_trackSourceInfo->kind()) {
    case TrackSourceInfo::Audio:
        return *audioKind;
    case TrackSourceInfo::Video:
        return *videoKind;
    }
#endif


    ASSERT_NOT_REACHED();
    return emptyAtom;
}

} // namespace WebCore

#endif

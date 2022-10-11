/*
 * Copyright (C) 2011 Apple Inc.  All rights reserved.
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
#include "TrackBase.h"

#include "HTMLMediaElement.h"

#if ENABLE(VIDEO_TRACK)

namespace WebCore {

#if !PLATFORM(WKC)
static int s_uniqueId = 0;
#else
WKC_DEFINE_GLOBAL_INT(s_uniqueId, 0);
#endif

TrackBase::TrackBase(Type type, const AtomicString& id, const AtomicString& label, const AtomicString& language)
    : m_mediaElement(0)
#if ENABLE(MEDIA_SOURCE)
    , m_sourceBuffer(0)
#endif
    , m_uniqueId(++s_uniqueId)
    , m_id(id)
    , m_label(label)
    , m_language(language)
{
    ASSERT(type != BaseTrack);
    m_type = type;
}

TrackBase::~TrackBase()
{
}

Element* TrackBase::element()
{
    return m_mediaElement;
}

void TrackBase::setKind(const AtomicString& kind)
{
    setKindInternal(kind);
}

void TrackBase::setKindInternal(const AtomicString& kind)
{
    String oldKind = m_kind;

    if (isValidKind(kind))
        m_kind = kind;
    else
        m_kind = defaultKindKeyword();
}

} // namespace WebCore

#endif

/*
 * Copyright (C) 2013 University of Washington. All rights reserved.
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ReplaySession.h"

#if ENABLE(WEB_REPLAY)

#include "ReplaySessionSegment.h"
#include <wtf/CurrentTime.h>

namespace WebCore {

static unsigned s_nextIdentifier = 1;

Ref<ReplaySession> ReplaySession::create()
{
    return adoptRef(*new ReplaySession());
}

ReplaySession::ReplaySession()
    : m_identifier(s_nextIdentifier++)
    , m_timestamp(currentTimeMS())
{
}

ReplaySession::~ReplaySession()
{
}

RefPtr<ReplaySessionSegment> ReplaySession::at(size_t position) const
{
    ASSERT_ARG(position, position >= 0 && position < m_segments.size());

    return m_segments.at(position).copyRef();
}

void ReplaySession::appendSegment(RefPtr<ReplaySessionSegment>&& segment)
{
    ASSERT_ARG(segment, segment);
    // For now, only support one segment.
    ASSERT(!m_segments.size());

    // Since replay locations are specified with segment IDs, we can only
    // have one instance of a segment in the session.
    size_t offset = m_segments.find(segment.copyRef());
    ASSERT_UNUSED(offset, offset == notFound);

    m_segments.append(WTF::move(segment));
}

void ReplaySession::insertSegment(size_t position, RefPtr<ReplaySessionSegment>&& segment)
{
    ASSERT_ARG(segment, segment);
    ASSERT_ARG(position, position >= 0 && position < m_segments.size());

    m_segments.insert(position, WTF::move(segment));
}

void ReplaySession::removeSegment(size_t position)
{
    ASSERT_ARG(position, position >= 0 && position < m_segments.size());

    m_segments.remove(position);
}

}; // namespace WebCore

#endif // ENABLE(WEB_REPLAY)

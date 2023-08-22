/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SimpleLineLayoutFlowContents_h
#define SimpleLineLayoutFlowContents_h

#include "RenderText.h"

namespace WebCore {
class RenderBlockFlow;

namespace SimpleLineLayout {

class FlowContents {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    FlowContents(const RenderBlockFlow&);

    struct Segment {
#if PLATFORM(WKC)
        WTF_MAKE_FAST_ALLOCATED;
    public:
#endif
        unsigned start;
        unsigned end;
        String text;
        const RenderObject& renderer;
    };
    const Segment& segmentForRun(unsigned start, unsigned end) const;

    typedef Vector<Segment, 8>::const_iterator Iterator;
    Iterator begin() const { return m_segments.begin(); }
    Iterator end() const { return m_segments.end(); }

private:
    unsigned segmentIndexForRunSlow(unsigned start, unsigned end) const;
    const Vector<Segment, 8> m_segments;
    mutable unsigned m_lastSegmentIndex;
};

inline const FlowContents::Segment& FlowContents::segmentForRun(unsigned start, unsigned end) const
{
    ASSERT(start < end);
    auto& lastSegment = m_segments[m_lastSegmentIndex];
    if (lastSegment.start <= start && end <= lastSegment.end)
        return m_segments[m_lastSegmentIndex];
    return m_segments[segmentIndexForRunSlow(start, end)];
}

}
}

#endif

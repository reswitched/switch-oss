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

#ifndef SimpleLineLayoutResolver_h
#define SimpleLineLayoutResolver_h

#include "LayoutRect.h"
#include "RenderBlockFlow.h"
#include "SimpleLineLayoutFlowContents.h"
#include "SimpleLineLayoutFunctions.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
namespace SimpleLineLayout {

template <class IteratorType>
class Range {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    Range(IteratorType begin, IteratorType end) : m_begin(begin), m_end(end) { }

    IteratorType begin() const { return m_begin; }
    IteratorType end() const { return m_end; }

private:
    IteratorType m_begin;
    IteratorType m_end;
};

class RunResolver {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    class Iterator;

    class Run {
#if PLATFORM(WKC)
        WTF_MAKE_FAST_ALLOCATED;
#endif
    public:
        explicit Run(const Iterator&);

        unsigned start() const;
        unsigned end() const;

        LayoutRect rect() const;
        FloatPoint baseline() const;
        StringView text() const;
        bool isEndOfLine() const;

        unsigned lineIndex() const;

    private:
        const Iterator& m_iterator;
    };

    class Iterator {
#if PLATFORM(WKC)
        WTF_MAKE_FAST_ALLOCATED;
#endif
    friend class Run;
    friend class RunResolver;
    friend class LineResolver;
    public:
        Iterator(const RunResolver&, unsigned runIndex, unsigned lineIndex);

        Iterator& operator++();
        Iterator& operator--();

        bool operator==(const Iterator&) const;
        bool operator!=(const Iterator&) const;

        Run operator*() const;

    private:
        const SimpleLineLayout::Run& simpleRun() const;
        unsigned lineIndex() const { return m_lineIndex; }
        Iterator& advance();
        Iterator& advanceLines(unsigned);
        const RunResolver& resolver() const { return m_resolver; }
        bool inQuirksMode() const { return m_resolver.m_inQuirksMode; }

        const RunResolver& m_resolver;
        unsigned m_runIndex;
        unsigned m_lineIndex;
    };

    RunResolver(const RenderBlockFlow&, const Layout&);

    const RenderBlockFlow& flow() const { return m_flowRenderer; }
    Iterator begin() const;
    Iterator end() const;

    Range<Iterator> rangeForRect(const LayoutRect&) const;
    Range<Iterator> rangeForRenderer(const RenderObject&) const;

private:
    enum class IndexType { First, Last };
    unsigned lineIndexForHeight(LayoutUnit, IndexType) const;

    const RenderBlockFlow& m_flowRenderer;
    const Layout& m_layout;
    const FlowContents m_flowContents;
    const LayoutUnit m_lineHeight;
    const LayoutUnit m_baseline;
    const LayoutUnit m_borderAndPaddingBefore;
    const float m_ascent;
    const float m_descent;
    const bool m_inQuirksMode;
};

class LineResolver {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    class Iterator;

    class Iterator {
#if PLATFORM(WKC)
        WTF_MAKE_FAST_ALLOCATED;
#endif
    public:
        explicit Iterator(RunResolver::Iterator);

        Iterator& operator++();
        bool operator==(const Iterator&) const;
        bool operator!=(const Iterator&) const;

        const LayoutRect operator*() const;

    private:
        RunResolver::Iterator m_runIterator;
        LayoutRect m_rect;
    };

    LineResolver(const RenderBlockFlow&, const Layout&);

    Iterator begin() const;
    Iterator end() const;

    Range<Iterator> rangeForRect(const LayoutRect&) const;

private:
    RunResolver m_runResolver;
};

RunResolver runResolver(const RenderBlockFlow&, const Layout&);
LineResolver lineResolver(const RenderBlockFlow&, const Layout&);

inline unsigned RunResolver::Run::start() const
{
    return m_iterator.simpleRun().start;
}

inline unsigned RunResolver::Run::end() const
{
    return m_iterator.simpleRun().end;
}

inline bool RunResolver::Run::isEndOfLine() const
{
    return m_iterator.simpleRun().isEndOfLine;
}

inline unsigned RunResolver::Run::lineIndex() const
{
    return m_iterator.lineIndex();
}

inline RunResolver::Iterator& RunResolver::Iterator::operator++()
{
    return advance();
}

inline RunResolver::Iterator& RunResolver::Iterator::operator--()
{
    --m_runIndex;
    if (simpleRun().isEndOfLine)
        --m_lineIndex;
    return *this;
}

inline bool RunResolver::Iterator::operator==(const Iterator& other) const
{
    ASSERT(&m_resolver == &other.m_resolver);
    return m_runIndex == other.m_runIndex;
}

inline bool RunResolver::Iterator::operator!=(const Iterator& other) const
{
    return !(*this == other);
}

inline RunResolver::Run RunResolver::Iterator::operator*() const
{
    return Run(*this);
}

inline const SimpleLineLayout::Run& RunResolver::Iterator::simpleRun() const
{
    return m_resolver.m_layout.runAt(m_runIndex);
}

inline RunResolver::Iterator RunResolver::begin() const
{
    return Iterator(*this, 0, 0);
}

inline RunResolver::Iterator RunResolver::end() const
{
    return Iterator(*this, m_layout.runCount(), m_layout.lineCount());
}

inline LineResolver::Iterator& LineResolver::Iterator::operator++()
{
    m_runIterator.advanceLines(1);
    return *this;
}

inline bool LineResolver::Iterator::operator==(const Iterator& other) const
{
    return m_runIterator == other.m_runIterator;
}

inline bool LineResolver::Iterator::operator!=(const Iterator& other) const
{
    return m_runIterator != other.m_runIterator;
}

inline LineResolver::Iterator LineResolver::begin() const
{
    return Iterator(m_runResolver.begin());
}

inline LineResolver::Iterator LineResolver::end() const
{
    return Iterator(m_runResolver.end());
}

inline Range<LineResolver::Iterator> LineResolver::rangeForRect(const LayoutRect& rect) const
{
    auto runRange = m_runResolver.rangeForRect(rect);
    return Range<Iterator>(Iterator(runRange.begin()), Iterator(runRange.end()));
}

inline RunResolver runResolver(const RenderBlockFlow& flow, const Layout& layout)
{
    return RunResolver(flow, layout);
}

inline LineResolver lineResolver(const RenderBlockFlow& flow, const Layout& layout)
{
    return LineResolver(flow, layout);
}

}
}

#endif

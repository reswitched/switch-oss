/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All right reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2013 ChangSeok Oh <shivamidow@gmail.com>
 * Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LineLayoutState_h
#define LineLayoutState_h

#include "LayoutRect.h"
#include "RenderBox.h"
#include <wtf/RefCounted.h>

namespace WebCore {

class FloatWithRect : public RefCounted<FloatWithRect> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static Ref<FloatWithRect> create(RenderBox& renderer)
    {
        return adoptRef(*new FloatWithRect(renderer));
    }

    RenderBox& renderer() const { return m_renderer; }
    LayoutRect rect() const { return m_rect; }
    bool everHadLayout() const { return m_everHadLayout; }

    void adjustRect(const LayoutRect& rect) { m_rect = rect; }

private:
    FloatWithRect() = default;

    FloatWithRect(RenderBox& renderer)
        : m_renderer(renderer)
        , m_rect(LayoutRect(renderer.x() - renderer.marginLeft(), renderer.y() - renderer.marginTop(), renderer.width() + renderer.horizontalMarginExtent(), renderer.height() + renderer.verticalMarginExtent()))
        , m_everHadLayout(renderer.everHadLayout())
    {
    }

    RenderBox& m_renderer;
    LayoutRect m_rect;
    bool m_everHadLayout{ false };
};

// Like LayoutState for layout(), LineLayoutState keeps track of global information
// during an entire linebox tree layout pass (aka layoutInlineChildren).
class LineLayoutState {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    class FloatList {
    public:
        void append(Ref<FloatWithRect>&& floatWithRect)
        {
            m_floats.add(floatWithRect.copyRef());
            m_floatWithRectMap.add(&floatWithRect->renderer(), WTF::move(floatWithRect));
        }
        void setLastFloat(FloatingObject* lastFloat) { m_lastFloat = lastFloat; }
        FloatingObject* lastFloat() const { return m_lastFloat; }

        void setLastCleanFloat(RenderBox& floatBox) { m_lastCleanFloat = &floatBox; }
        RenderBox* lastCleanFloat() const { return m_lastCleanFloat; }

        FloatWithRect& floatWithRect(RenderBox& floatBox) const { return *m_floatWithRectMap.get(&floatBox); }

        using Iterator = ListHashSet<Ref<FloatWithRect>>::iterator;
        Iterator begin() { return m_floats.begin(); }
        Iterator end() { return m_floats.end(); }
        Iterator find(FloatWithRect& floatBoxWithRect) { return m_floats.find(floatBoxWithRect); }
        bool isEmpty() const { return m_floats.isEmpty(); }

    private:
        ListHashSet<Ref<FloatWithRect>> m_floats;
        HashMap<RenderBox*, Ref<FloatWithRect>> m_floatWithRectMap;
        FloatingObject* m_lastFloat{ nullptr };
        RenderBox* m_lastCleanFloat{ nullptr };
    };

    LineLayoutState(bool fullLayout, LayoutUnit& repaintLogicalTop, LayoutUnit& repaintLogicalBottom, RenderFlowThread* flowThread)
        : m_flowThread(flowThread)
        , m_repaintLogicalTop(repaintLogicalTop)
        , m_repaintLogicalBottom(repaintLogicalBottom)
        , m_endLineMatched(false)
        , m_checkForFloatsFromLastLine(false)
        , m_isFullLayout(fullLayout)
        , m_usesRepaintBounds(false)
    {
    }

    LineInfo& lineInfo() { return m_lineInfo; }
    const LineInfo& lineInfo() const { return m_lineInfo; }

    LayoutUnit endLineLogicalTop() const { return m_endLineLogicalTop; }
    void setEndLineLogicalTop(LayoutUnit logicalTop) { m_endLineLogicalTop = logicalTop; }

    RootInlineBox* endLine() const { return m_endLine; }
    void setEndLine(RootInlineBox* line) { m_endLine = line; }

    LayoutUnit adjustedLogicalLineTop() const { return m_adjustedLogicalLineTop; }
    void setAdjustedLogicalLineTop(LayoutUnit value) { m_adjustedLogicalLineTop = value; }

    RenderFlowThread* flowThread() const { return m_flowThread; }
    void setFlowThread(RenderFlowThread* thread) { m_flowThread = thread; }

    bool endLineMatched() const { return m_endLineMatched; }
    void setEndLineMatched(bool endLineMatched) { m_endLineMatched = endLineMatched; }

    bool checkForFloatsFromLastLine() const { return m_checkForFloatsFromLastLine; }
    void setCheckForFloatsFromLastLine(bool check) { m_checkForFloatsFromLastLine = check; }

    void markForFullLayout() { m_isFullLayout = true; }
    bool isFullLayout() const { return m_isFullLayout; }

    bool usesRepaintBounds() const { return m_usesRepaintBounds; }

    void setRepaintRange(LayoutUnit logicalHeight)
    {
        m_usesRepaintBounds = true;
        m_repaintLogicalTop = m_repaintLogicalBottom = logicalHeight;
    }

    void updateRepaintRangeFromBox(RootInlineBox* box, LayoutUnit paginationDelta = 0)
    {
        m_usesRepaintBounds = true;
        m_repaintLogicalTop = std::min(m_repaintLogicalTop, box->logicalTopVisualOverflow() + std::min<LayoutUnit>(paginationDelta, 0));
        m_repaintLogicalBottom = std::max(m_repaintLogicalBottom, box->logicalBottomVisualOverflow() + std::max<LayoutUnit>(paginationDelta, 0));
    }

    FloatList& floatList() { return m_floatList; }

private:
    LineInfo m_lineInfo;
    LayoutUnit m_endLineLogicalTop;
    RootInlineBox* m_endLine { nullptr };

    LayoutUnit m_adjustedLogicalLineTop;

    RenderFlowThread* m_flowThread { nullptr };

    FloatList m_floatList;
    // FIXME: Should this be a range object instead of two ints?
    LayoutUnit& m_repaintLogicalTop;
    LayoutUnit& m_repaintLogicalBottom;

    bool m_endLineMatched : 1;
    bool m_checkForFloatsFromLastLine : 1;
    bool m_isFullLayout : 1;
    bool m_usesRepaintBounds : 1;
};

}

#endif // LineLayoutState_h

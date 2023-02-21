/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RenderLineBreak_h
#define RenderLineBreak_h

#include "RenderBoxModelObject.h"

namespace WebCore {

class InlineElementBox;
class HTMLElement;
class Position;

class RenderLineBreak final : public RenderBoxModelObject {
public:
    RenderLineBreak(HTMLElement&, Ref<RenderStyle>&&);
    virtual ~RenderLineBreak();

    // FIXME: The lies here keep render tree dump based test results unchanged.
    virtual const char* renderName() const override { return m_isWBR ? "RenderWordBreak" : "RenderBR"; }

    virtual bool isWBR() const override { return m_isWBR; }

    std::unique_ptr<InlineElementBox> createInlineBox();
    InlineElementBox* inlineBoxWrapper() const { return m_inlineBoxWrapper; }
    void setInlineBoxWrapper(InlineElementBox*);
    void deleteInlineBoxWrapper();
    void replaceInlineBoxWrapper(InlineElementBox&);
    void dirtyLineBoxes(bool fullLayout);
    void deleteLineBoxesBeforeSimpleLineLayout();

    IntRect linesBoundingBox() const;

    virtual void absoluteRects(Vector<IntRect>&, const LayoutPoint& accumulatedOffset) const override;
    virtual void absoluteQuads(Vector<FloatQuad>&, bool* wasFixed) const override;
#if PLATFORM(IOS)
virtual void collectSelectionRects(Vector<SelectionRect>&, unsigned startOffset = 0, unsigned endOffset = std::numeric_limits<unsigned>::max()) override;
#endif

private:
    void node() const  = delete;

    virtual bool canHaveChildren() const override { return false; }
    virtual void paint(PaintInfo&, const LayoutPoint&) override { }

    virtual VisiblePosition positionForPoint(const LayoutPoint&, const RenderRegion*) override;
    virtual int caretMinOffset() const override;
    virtual int caretMaxOffset() const override;
    virtual bool canBeSelectionLeaf() const override;
    virtual LayoutRect localCaretRect(InlineBox*, int caretOffset, LayoutUnit* extraWidthToEndOfLine) override;
    virtual void setSelectionState(SelectionState) override;

    virtual LayoutUnit lineHeight(bool firstLine, LineDirectionMode, LinePositionMode) const override;
    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode) const override;

    virtual LayoutUnit marginTop() const override { return 0; }
    virtual LayoutUnit marginBottom() const override { return 0; }
    virtual LayoutUnit marginLeft() const override { return 0; }
    virtual LayoutUnit marginRight() const override { return 0; }
    virtual LayoutUnit marginBefore(const RenderStyle*) const override { return 0; }
    virtual LayoutUnit marginAfter(const RenderStyle*) const override { return 0; }
    virtual LayoutUnit marginStart(const RenderStyle*) const override { return 0; }
    virtual LayoutUnit marginEnd(const RenderStyle*) const override { return 0; }
    virtual LayoutUnit offsetWidth() const override { return linesBoundingBox().width(); }
    virtual LayoutUnit offsetHeight() const override { return linesBoundingBox().height(); }
    virtual IntRect borderBoundingBox() const override;
    virtual LayoutRect frameRectForStickyPositioning() const override { ASSERT_NOT_REACHED(); return LayoutRect(); }
    virtual LayoutRect clippedOverflowRectForRepaint(const RenderLayerModelObject*) const override { return LayoutRect(); }

    virtual void updateFromStyle() override;
    virtual bool requiresLayer() const override { return false; }

    InlineElementBox* m_inlineBoxWrapper;
    mutable int m_cachedLineHeight;
    bool m_isWBR;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderLineBreak, isLineBreak())

#endif // RenderLineBreak_h

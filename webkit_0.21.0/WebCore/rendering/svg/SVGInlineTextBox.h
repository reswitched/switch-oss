/*
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
 */

#ifndef SVGInlineTextBox_h
#define SVGInlineTextBox_h

#include "InlineTextBox.h"
#include "SVGTextLayoutEngine.h"
#include "RenderSVGInlineText.h"

namespace WebCore {

class RenderSVGResource;
class SVGRootInlineBox;

class SVGInlineTextBox final : public InlineTextBox {
public:
    explicit SVGInlineTextBox(RenderSVGInlineText&);

    RenderSVGInlineText& renderer() const { return downcast<RenderSVGInlineText>(InlineTextBox::renderer()); }

    virtual float virtualLogicalHeight() const override { return m_logicalHeight; }
    void setLogicalHeight(float height) { m_logicalHeight = height; }

    int selectionTop() { return top(); }
    int selectionHeight() { return static_cast<int>(ceilf(m_logicalHeight)); }
    virtual int offsetForPosition(float x, bool includePartialGlyphs = true) const override;
    virtual float positionForOffset(int offset) const override;

    void paintSelectionBackground(PaintInfo&);
    virtual void paint(PaintInfo&, const LayoutPoint&, LayoutUnit lineTop, LayoutUnit lineBottom) override;
    virtual LayoutRect localSelectionRect(int startPosition, int endPosition) const override;

    bool mapStartEndPositionsIntoFragmentCoordinates(const SVGTextFragment&, int& startPosition, int& endPosition) const;

    virtual FloatRect calculateBoundaries() const override;

    void clearTextFragments() { m_textFragments.clear(); }
    Vector<SVGTextFragment>& textFragments() { return m_textFragments; }
    const Vector<SVGTextFragment>& textFragments() const { return m_textFragments; }

    virtual void dirtyOwnLineBoxes() override;
    virtual void dirtyLineBoxes() override;

    bool startsNewTextChunk() const { return m_startsNewTextChunk; }
    void setStartsNewTextChunk(bool newTextChunk) { m_startsNewTextChunk = newTextChunk; }

    int offsetForPositionInFragment(const SVGTextFragment&, float position, bool includePartialGlyphs) const;
    FloatRect selectionRectForTextFragment(const SVGTextFragment&, int fragmentStartPosition, int fragmentEndPosition, RenderStyle*) const;

private:
    virtual bool isSVGInlineTextBox() const override { return true; }

    TextRun constructTextRun(RenderStyle*, const SVGTextFragment&) const;

    bool acquirePaintingResource(GraphicsContext*&, float scalingFactor, RenderBoxModelObject&, RenderStyle*);
    void releasePaintingResource(GraphicsContext*&, const Path*);

    bool prepareGraphicsContextForTextPainting(GraphicsContext*&, float scalingFactor, TextRun&, RenderStyle*);
    void restoreGraphicsContextAfterTextPainting(GraphicsContext*&, TextRun&);

    void paintDecoration(GraphicsContext*, TextDecoration, const SVGTextFragment&);
    void paintDecorationWithStyle(GraphicsContext*, TextDecoration, const SVGTextFragment&, RenderBoxModelObject& decorationRenderer);
    void paintTextWithShadows(GraphicsContext*, RenderStyle*, TextRun&, const SVGTextFragment&, int startPosition, int endPosition);
    void paintText(GraphicsContext*, RenderStyle*, RenderStyle* selectionStyle, const SVGTextFragment&, bool hasSelection, bool paintSelectedTextOnly);

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, LayoutUnit lineTop, LayoutUnit lineBottom, HitTestAction) override;

private:
    float m_logicalHeight;
    unsigned m_paintingResourceMode : 4;
    unsigned m_startsNewTextChunk : 1;
    RenderSVGResource* m_paintingResource;
    Vector<SVGTextFragment> m_textFragments;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_INLINE_BOX(SVGInlineTextBox, isSVGInlineTextBox())

#endif // SVGInlineTextBox_h

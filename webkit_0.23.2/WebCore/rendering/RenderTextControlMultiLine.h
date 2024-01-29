/*
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/) 
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef RenderTextControlMultiLine_h
#define RenderTextControlMultiLine_h

#include "RenderTextControl.h"

namespace WebCore {

class HTMLTextAreaElement;

class RenderTextControlMultiLine final : public RenderTextControl {
public:
    RenderTextControlMultiLine(HTMLTextAreaElement&, Ref<RenderStyle>&&);
    virtual ~RenderTextControlMultiLine();

    HTMLTextAreaElement& textAreaElement() const;

private:
    void element() const  = delete;

    virtual bool isTextArea() const override { return true; }

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const HitTestLocation& locationInContainer, const LayoutPoint& accumulatedOffset, HitTestAction) override;

    virtual float getAverageCharWidth() override;
    virtual LayoutUnit preferredContentLogicalWidth(float charWidth) const override;
    virtual LayoutUnit computeControlLogicalHeight(LayoutUnit lineHeight, LayoutUnit nonContentHeight) const override;
    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const override;

    virtual Ref<RenderStyle> createInnerTextStyle(const RenderStyle* startStyle) const override;
    virtual RenderObject* layoutSpecialExcludedChild(bool relayoutChildren) override;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderTextControlMultiLine, isTextArea())

#endif // RenderTextControlMultiLine_h

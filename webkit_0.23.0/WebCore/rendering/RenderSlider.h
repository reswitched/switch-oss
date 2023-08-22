/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef RenderSlider_h
#define RenderSlider_h

#include "RenderFlexibleBox.h"

namespace WebCore {

class HTMLInputElement;
class MouseEvent;
class SliderThumbElement;

class RenderSlider final : public RenderFlexibleBox {
public:
    static const int defaultTrackLength;

    RenderSlider(HTMLInputElement&, Ref<RenderStyle>&&);
    virtual ~RenderSlider();

    HTMLInputElement& element() const;

    bool inDragMode() const;

private:
    virtual const char* renderName() const override { return "RenderSlider"; }
    virtual bool isSlider() const override { return true; }

    virtual int baselinePosition(FontBaseline, bool firstLine, LineDirectionMode, LinePositionMode = PositionOnContainingLine) const override;
    virtual void computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const override;
    virtual void computePreferredLogicalWidths() override;
    virtual bool requiresForcedStyleRecalcPropagation() const override { return true; }
    virtual void layout() override;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderSlider, isSlider())

#endif // RenderSlider_h

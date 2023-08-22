/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "RenderFrameBase.h"

#include "Frame.h"
#include "FrameView.h"
#include "HTMLFrameElementBase.h"
#include "RenderView.h"

namespace WebCore {

RenderFrameBase::RenderFrameBase(HTMLFrameElementBase& element, Ref<RenderStyle>&& style)
    : RenderWidget(element, WTF::move(style))
{
}

inline bool shouldExpandFrame(LayoutUnit width, LayoutUnit height, bool hasFixedWidth, bool hasFixedHeight)
{
    // If the size computed to zero never expand.
    if (!width || !height)
        return false;
    // Really small fixed size frames can't be meant to be scrolled and are there probably by mistake. Avoid expanding.
    static const unsigned smallestUsefullyScrollableDimension = 8;
    if (hasFixedWidth && width < LayoutUnit(smallestUsefullyScrollableDimension))
        return false;
    if (hasFixedHeight && height < LayoutUnit(smallestUsefullyScrollableDimension))
        return false;
    return true;
}

void RenderFrameBase::layoutWithFlattening(bool hasFixedWidth, bool hasFixedHeight)
{
    view().protectRenderWidgetUntilLayoutIsDone(*this);

    peformLayoutWithFlattening(hasFixedWidth, hasFixedHeight);

    clearNeedsLayout();
}

RenderView* RenderFrameBase::childRenderView() const
{
    if (!childView())
        return nullptr;
    return childView()->renderView();
}

void RenderFrameBase::peformLayoutWithFlattening(bool hasFixedWidth, bool hasFixedHeight)
{
    if (!childRenderView() || !shouldExpandFrame(width(), height(), hasFixedWidth, hasFixedHeight)) {
        if (updateWidgetPosition() == ChildWidgetState::ChildWidgetIsDestroyed)
            return;
        childView()->layout();
        return;
    }

    // need to update to calculate min/max correctly
    if (updateWidgetPosition() == ChildWidgetState::ChildWidgetIsDestroyed)
        return;
    
    // if scrollbars are off, and the width or height are fixed
    // we obey them and do not expand. With frame flattening
    // no subframe much ever become scrollable.
    bool isScrollable = frameOwnerElement().scrollingMode() != ScrollbarAlwaysOff;

    // consider iframe inset border
    int hBorder = borderLeft() + borderRight();
    int vBorder = borderTop() + borderBottom();

    // make sure minimum preferred width is enforced
    if (isScrollable || !hasFixedWidth) {
        ASSERT(childRenderView());
        setWidth(std::max(width(), childRenderView()->minPreferredLogicalWidth() + hBorder));
        // update again to pass the new width to the child frame
        if (updateWidgetPosition() == ChildWidgetState::ChildWidgetIsDestroyed)
            return;
        childView()->layout();
    }

    ASSERT(childView());
    // expand the frame by setting frame height = content height
    if (isScrollable || !hasFixedHeight || childRenderView()->isFrameSet())
        setHeight(std::max<LayoutUnit>(height(), childView()->contentsHeight() + vBorder));
    if (isScrollable || !hasFixedWidth || childRenderView()->isFrameSet())
        setWidth(std::max<LayoutUnit>(width(), childView()->contentsWidth() + hBorder));

    if (updateWidgetPosition() == ChildWidgetState::ChildWidgetIsDestroyed)
        return;

    ASSERT(!childView()->layoutPending());
    ASSERT(!childRenderView()->needsLayout());
    ASSERT(!childRenderView()->firstChild() || !childRenderView()->firstChild()->firstChildSlow() || !childRenderView()->firstChild()->firstChildSlow()->needsLayout());
}

}

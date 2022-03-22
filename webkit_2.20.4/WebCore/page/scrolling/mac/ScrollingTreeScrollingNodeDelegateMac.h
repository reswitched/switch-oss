/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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

#pragma once

#include "ScrollingTreeScrollingNodeDelegate.h"

#if ENABLE(ASYNC_SCROLLING) && PLATFORM(MAC)

#include "ScrollController.h"

OBJC_CLASS NSScrollerImp;

namespace WebCore {

class FloatPoint;
class FloatSize;
class IntPoint;
class ScrollingStateScrollingNode;
class ScrollingTreeScrollingNode;
class ScrollingTree;

class ScrollingTreeScrollingNodeDelegateMac : public ScrollingTreeScrollingNodeDelegate, private ScrollControllerClient {
public:
    explicit ScrollingTreeScrollingNodeDelegateMac(ScrollingTreeScrollingNode&);
    virtual ~ScrollingTreeScrollingNodeDelegateMac();

    bool handleWheelEvent(const PlatformWheelEvent&);

#if ENABLE(CSS_SCROLL_SNAP)
    void updateScrollSnapPoints(ScrollEventAxis, const Vector<LayoutUnit>&, const Vector<ScrollOffsetRange<LayoutUnit>>&);
    void setActiveScrollSnapIndexForAxis(ScrollEventAxis, unsigned);
    bool activeScrollSnapIndexDidChange() const;
    unsigned activeScrollSnapIndexForAxis(ScrollEventAxis) const;
    bool isScrollSnapInProgress() const;
#endif

    void updateFromStateNode(const ScrollingStateScrollingNode&);
    void updateScrollbarPainters();

    void deferWheelEventTestCompletionForReason(WheelEventTestMonitor::ScrollableAreaIdentifier, WheelEventTestMonitor::DeferReason) const override;
    void removeWheelEventTestCompletionDeferralForReason(WheelEventTestMonitor::ScrollableAreaIdentifier, WheelEventTestMonitor::DeferReason) const override;

private:
    bool isAlreadyPinnedInDirectionOfGesture(const PlatformWheelEvent&, ScrollEventAxis) const;

    // ScrollControllerClient.
    bool allowsHorizontalStretching(const PlatformWheelEvent&) const override;
    bool allowsVerticalStretching(const PlatformWheelEvent&) const override;
    IntSize stretchAmount() const override;
    bool pinnedInDirection(const FloatSize&) const override;
    bool canScrollHorizontally() const override;
    bool canScrollVertically() const override;
    bool shouldRubberBandInDirection(ScrollDirection) const override;
    void immediateScrollBy(const FloatSize&) override;
    void immediateScrollByWithoutContentEdgeConstraints(const FloatSize&) override;
    void stopSnapRubberbandTimer() override;
    void adjustScrollPositionToBoundsIfNecessary() override;

#if ENABLE(CSS_SCROLL_SNAP)
    FloatPoint scrollOffset() const override;
    void immediateScrollOnAxis(ScrollEventAxis, float delta) override;
    float pageScaleFactor() const override;
    void startScrollSnapTimer() override;
    void stopScrollSnapTimer() override;
    LayoutSize scrollExtent() const override;
    FloatSize viewportSize() const override;
#endif

    void releaseReferencesToScrollerImpsOnTheMainThread();

    ScrollController m_scrollController;

    RetainPtr<NSScrollerImp> m_verticalScrollerImp;
    RetainPtr<NSScrollerImp> m_horizontalScrollerImp;
};

} // namespace WebCore

#endif // PLATFORM(MAC) && ENABLE(ASYNC_SCROLLING)

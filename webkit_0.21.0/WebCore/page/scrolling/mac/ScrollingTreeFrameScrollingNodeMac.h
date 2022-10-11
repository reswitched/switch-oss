/*
 * Copyright (C) 2012, 2014-2015 Apple Inc. All rights reserved.
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

#ifndef ScrollingTreeFrameScrollingNodeMac_h
#define ScrollingTreeFrameScrollingNodeMac_h

#if ENABLE(ASYNC_SCROLLING) && PLATFORM(MAC)

#include "ScrollController.h"
#include "ScrollbarThemeMac.h"
#include "ScrollingTreeFrameScrollingNode.h"
#include <wtf/RetainPtr.h>

OBJC_CLASS CALayer;

namespace WebCore {

class ScrollingTreeFrameScrollingNodeMac : public ScrollingTreeFrameScrollingNode, private ScrollControllerClient {
public:
    WEBCORE_EXPORT static Ref<ScrollingTreeFrameScrollingNode> create(ScrollingTree&, ScrollingNodeID);
    virtual ~ScrollingTreeFrameScrollingNodeMac();

private:
    ScrollingTreeFrameScrollingNodeMac(ScrollingTree&, ScrollingNodeID);

    void releaseReferencesToScrollbarPaintersOnTheMainThread();

    // ScrollingTreeNode member functions.
    virtual void updateBeforeChildren(const ScrollingStateNode&) override;
    virtual void updateAfterChildren(const ScrollingStateNode&) override;
    virtual void handleWheelEvent(const PlatformWheelEvent&) override;

    // ScrollController member functions.
    bool allowsHorizontalStretching(const PlatformWheelEvent&) override;
    bool allowsVerticalStretching(const PlatformWheelEvent&) override;
    IntSize stretchAmount() override;
    bool pinnedInDirection(const FloatSize&) override;
    bool canScrollHorizontally() override;
    bool canScrollVertically() override;
    bool shouldRubberBandInDirection(ScrollDirection) override;
    IntPoint absoluteScrollPosition() override;
    void immediateScrollBy(const FloatSize&) override;
    void immediateScrollByWithoutContentEdgeConstraints(const FloatSize&) override;
    void stopSnapRubberbandTimer() override;
    void adjustScrollPositionToBoundsIfNecessary() override;

    FloatPoint scrollPosition() const override;
    void setScrollPosition(const FloatPoint&) override;
    void setScrollPositionWithoutContentEdgeConstraints(const FloatPoint&) override;

    void updateLayersAfterViewportChange(const FloatRect& fixedPositionRect, double scale) override;

    void setScrollLayerPosition(const FloatPoint&) override;

    FloatPoint minimumScrollPosition() const override;
    FloatPoint maximumScrollPosition() const override;

    void updateMainFramePinState(const FloatPoint& scrollPosition);

    bool isAlreadyPinnedInDirectionOfGesture(const PlatformWheelEvent&, ScrollEventAxis);

    void deferTestsForReason(WheelEventTestTrigger::ScrollableAreaIdentifier, WheelEventTestTrigger::DeferTestTriggerReason) const override;
    void removeTestDeferralForReason(WheelEventTestTrigger::ScrollableAreaIdentifier, WheelEventTestTrigger::DeferTestTriggerReason) const override;

#if ENABLE(CSS_SCROLL_SNAP) && PLATFORM(MAC)
    LayoutUnit scrollOffsetOnAxis(ScrollEventAxis) const override;
    void immediateScrollOnAxis(ScrollEventAxis, float delta) override;
    float pageScaleFactor() const override;
    void startScrollSnapTimer() override;
    void stopScrollSnapTimer() override;
    LayoutSize scrollExtent() const override;
#endif

    void logExposedUnfilledArea();

    ScrollController m_scrollController;

    RetainPtr<CALayer> m_scrollLayer;
    RetainPtr<CALayer> m_scrolledContentsLayer;
    RetainPtr<CALayer> m_counterScrollingLayer;
    RetainPtr<CALayer> m_insetClipLayer;
    RetainPtr<CALayer> m_contentShadowLayer;
    RetainPtr<CALayer> m_headerLayer;
    RetainPtr<CALayer> m_footerLayer;
    RetainPtr<ScrollbarPainter> m_verticalScrollbarPainter;
    RetainPtr<ScrollbarPainter> m_horizontalScrollbarPainter;
    FloatPoint m_probableMainThreadScrollPosition;
    bool m_lastScrollHadUnfilledPixels { false };
    bool m_hadFirstUpdate { false };
    bool m_expectsWheelEventTestTrigger { false };
};

} // namespace WebCore

#endif // ENABLE(ASYNC_SCROLLING) && PLATFORM(MAC)

#endif // ScrollingTreeFrameScrollingNodeMac_h

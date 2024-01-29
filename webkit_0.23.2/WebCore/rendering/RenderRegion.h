/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER �gAS IS�h AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef RenderRegion_h
#define RenderRegion_h

#include "LayerFragment.h"
#include "RenderBlockFlow.h"
#include "StyleInheritedData.h"
#include "VisiblePosition.h"
#include <memory>

namespace WebCore {

class Element;
class RenderBox;
class RenderBoxRegionInfo;
class RenderFlowThread;
class RenderNamedFlowThread;

class RenderRegion : public RenderBlockFlow {
public:
    virtual void styleDidChange(StyleDifference, const RenderStyle* oldStyle) override;

    void setFlowThreadPortionRect(const LayoutRect& rect) { m_flowThreadPortionRect = rect; }
    LayoutRect flowThreadPortionRect() const { return m_flowThreadPortionRect; }
    LayoutRect flowThreadPortionOverflowRect();

    LayoutPoint flowThreadPortionLocation() const;

    virtual void attachRegion();
    virtual void detachRegion();

    RenderNamedFlowThread* parentNamedFlowThread() const { return m_parentNamedFlowThread; }
    RenderFlowThread* flowThread() const { return m_flowThread; }

    // Valid regions do not create circular dependencies with other flows.
    bool isValid() const { return m_isValid; }
    void setIsValid(bool valid) { m_isValid = valid; }

    RenderBoxRegionInfo* renderBoxRegionInfo(const RenderBox*) const;
    RenderBoxRegionInfo* setRenderBoxRegionInfo(const RenderBox*, LayoutUnit logicalLeftInset, LayoutUnit logicalRightInset,
        bool containingBlockChainIsInset);
    std::unique_ptr<RenderBoxRegionInfo> takeRenderBoxRegionInfo(const RenderBox*);
    void removeRenderBoxRegionInfo(const RenderBox*);

    void deleteAllRenderBoxRegionInfo();

    bool isFirstRegion() const;
    bool isLastRegion() const;
    virtual bool shouldClipFlowThreadContent() const;

    // These methods represent the width and height of a "page" and for a RenderRegion they are just the
    // content width and content height of a region. For RenderRegionSets, however, they will be the width and
    // height of a single column or page in the set.
    virtual LayoutUnit pageLogicalWidth() const;
    virtual LayoutUnit pageLogicalHeight() const;

    LayoutUnit logicalTopOfFlowThreadContentRect(const LayoutRect&) const;
    LayoutUnit logicalBottomOfFlowThreadContentRect(const LayoutRect&) const;
    LayoutUnit logicalTopForFlowThreadContent() const { return logicalTopOfFlowThreadContentRect(flowThreadPortionRect()); };
    LayoutUnit logicalBottomForFlowThreadContent() const { return logicalBottomOfFlowThreadContentRect(flowThreadPortionRect()); };

    // This method represents the logical height of the entire flow thread portion used by the region or set.
    // For RenderRegions it matches logicalPaginationHeight(), but for sets it is the height of all the pages
    // or columns added together.
    virtual LayoutUnit logicalHeightOfAllFlowThreadContent() const;

    // The top of the nearest page inside the region. For RenderRegions, this is just the logical top of the
    // flow thread portion we contain. For sets, we have to figure out the top of the nearest column or
    // page.
    virtual LayoutUnit pageLogicalTopForOffset(LayoutUnit offset) const;

    // Whether or not this region is a set.
    virtual bool isRenderRegionSet() const { return false; }
    
    virtual void repaintFlowThreadContent(const LayoutRect& repaintRect);

    virtual void collectLayerFragments(LayerFragments&, const LayoutRect&, const LayoutRect&) { }

    virtual void adjustRegionBoundsFromFlowThreadPortionRect(LayoutRect& regionBounds) const;

    void addLayoutOverflowForBox(const RenderBox*, const LayoutRect&);
    void addVisualOverflowForBox(const RenderBox*, const LayoutRect&);
    LayoutRect layoutOverflowRectForBox(const RenderBox*);
    LayoutRect visualOverflowRectForBox(const RenderBoxModelObject&);
    LayoutRect layoutOverflowRectForBoxForPropagation(const RenderBox*);
    LayoutRect visualOverflowRectForBoxForPropagation(const RenderBoxModelObject&);

    LayoutRect rectFlowPortionForBox(const RenderBox*, const LayoutRect&) const;
    
    void setRegionObjectsRegionStyle();
    void restoreRegionObjectsOriginalStyle();

    virtual bool canHaveChildren() const override { return false; }
    virtual bool canHaveGeneratedChildren() const override { return true; }
    virtual VisiblePosition positionForPoint(const LayoutPoint&, const RenderRegion*) override;

    virtual bool hasAutoLogicalHeight() const { return false; }

    virtual void absoluteQuadsForBoxInRegion(Vector<FloatQuad>&, bool*, const RenderBox*, float, float) { }
#if PLATFORM(WKC)
    WeakPtr<RenderRegion> createWeakPtr() { return m_weakFactory.createWeakPtr(); }
#endif

protected:
    RenderRegion(Element&, Ref<RenderStyle>&&, RenderFlowThread*);
    RenderRegion(Document&, Ref<RenderStyle>&&, RenderFlowThread*);

    void ensureOverflowForBox(const RenderBox*, RefPtr<RenderOverflow>&, bool);

    virtual void computePreferredLogicalWidths() override;
    virtual void computeIntrinsicLogicalWidths(LayoutUnit& minLogicalWidth, LayoutUnit& maxLogicalWidth) const override;

    enum OverflowType {
        LayoutOverflow = 0,
        VisualOverflow
    };

    LayoutRect overflowRectForFlowThreadPortion(const LayoutRect& flowThreadPortionRect, bool isFirstPortion, bool isLastPortion, OverflowType);
    void repaintFlowThreadContentRectangle(const LayoutRect& repaintRect, const LayoutRect& flowThreadPortionRect, const LayoutPoint& regionLocation, const LayoutRect* flowThreadPortionClipRect = 0);

    void computeOverflowFromFlowThread();

private:
    virtual bool isRenderRegion() const override final { return true; }
    virtual const char* renderName() const override { return "RenderRegion"; }

    virtual void insertedIntoTree() override;
    virtual void willBeRemovedFromTree() override;

    virtual void installFlowThread();

    LayoutPoint mapRegionPointIntoFlowThreadCoordinates(const LayoutPoint&);

protected:
    RenderFlowThread* m_flowThread;
#if PLATFORM(WKC)
    WeakPtrFactory<RenderRegion> m_weakFactory;
#endif

private:
    // If this RenderRegion is displayed as part of another named flow,
    // we need to create a dependency tree, so that layout of the
    // regions is always done before the regions themselves.
    RenderNamedFlowThread* m_parentNamedFlowThread;
    LayoutRect m_flowThreadPortionRect;

    // This map holds unique information about a block that is split across regions.
    // A RenderBoxRegionInfo* tells us about any layout information for a RenderBox that
    // is unique to the region. For now it just holds logical width information for RenderBlocks, but eventually
    // it will also hold a custom style for any box (for region styling).
    typedef HashMap<const RenderBox*, std::unique_ptr<RenderBoxRegionInfo>> RenderBoxRegionInfoMap;
    RenderBoxRegionInfoMap m_renderBoxRegionInfo;

    bool m_isValid : 1;
};

class CurrentRenderRegionMaintainer {
    WTF_MAKE_NONCOPYABLE(CurrentRenderRegionMaintainer);
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    CurrentRenderRegionMaintainer(RenderRegion&);
    ~CurrentRenderRegionMaintainer();

    RenderRegion& region() const { return m_region; }
private:
    RenderRegion& m_region;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_RENDER_OBJECT(RenderRegion, isRenderRegion())

#endif // RenderRegion_h

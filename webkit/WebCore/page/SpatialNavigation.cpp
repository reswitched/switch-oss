/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Antonio Gomes <tonikitoo@webkit.org>
 * Copyright (C) 2012-2017 ACCESS CO., LTD. All rights reserved.
 *
 * All rights reserved.
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
#include "SpatialNavigation.h"

#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLAreaElement.h"
#include "HTMLImageElement.h"
#include "HTMLMapElement.h"
#include "HTMLNames.h"
#include "IntRect.h"
#include "MainFrame.h"
#include "Node.h"
#include "Page.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "Settings.h"
#if PLATFORM(WKC)
#include "RenderTextControl.h"
#include "HTMLTextFormControlElement.h"
#include "TextControlInnerElements.h"
#endif

namespace WebCore {

static bool areRectsFullyAligned(FocusDirection, const LayoutRect&, const LayoutRect&);
static bool areRectsPartiallyAligned(FocusDirection, const LayoutRect&, const LayoutRect&);
static bool areRectsMoreThanFullScreenApart(FocusDirection, const LayoutRect& curRect, const LayoutRect& targetRect, const LayoutSize& viewSize);
#if !PLATFORM(WKC)
static bool isRectInDirection(FocusDirection, const LayoutRect&, const LayoutRect&);
#else
static bool isRectInsideEffectiveAngleRange(FocusDirection direction, const LayoutRect& curRect, const LayoutRect& targetRect);
#endif
static void deflateIfOverlapped(LayoutRect&, LayoutRect&);
#if !PLATFORM(WKC)
static LayoutRect rectToAbsoluteCoordinates(Frame* initialFrame, const LayoutRect&);
#endif
static void entryAndExitPointsForDirection(FocusDirection, const LayoutRect& startingRect, const LayoutRect& potentialRect, LayoutPoint& exitPoint, LayoutPoint& entryPoint);
static bool isScrollableNode(const Node*);

#if PLATFORM(WKC)
WKC_DEFINE_GLOBAL_UINT(gEffectiveAngleForHorizontalNavigation, 0);
WKC_DEFINE_GLOBAL_UINT(gEffectiveAngleForVerticalNavigation, 0);
WKC_DEFINE_GLOBAL_UINT(gMinEffectiveAngleForDiagonalNavigation, 0);
WKC_DEFINE_GLOBAL_UINT(gMaxEffectiveAngleForDiagonalNavigation, 0);

void SpatialNavigation_SetEffectiveAngleForHorizontalNavigation(unsigned int angle)
{
    gEffectiveAngleForHorizontalNavigation = angle;
}

void SpatialNavigation_SetEffectiveAngleForVerticalNavigation(unsigned int angle)
{
    gEffectiveAngleForVerticalNavigation = angle;
}

void SpatialNavigation_SetEffectiveAngleForDiagonalNavigation(unsigned int minAngle, unsigned int maxAngle)
{
    gMinEffectiveAngleForDiagonalNavigation = minAngle;
    gMaxEffectiveAngleForDiagonalNavigation = maxAngle;
}

static inline bool isDiagonalDirection(FocusDirection direction)
{
    switch (direction) {
    case FocusDirectionUpLeft:
    case FocusDirectionUpRight:
    case FocusDirectionDownLeft:
    case FocusDirectionDownRight:
        return true;
    default:
        return false;
    }
}

bool isInsideFocusableFrame(Element* element)
{
    FrameView* view = element->document().frame()->view();

    bool isFocusable = true;
    for (; view; view = downcast<FrameView>(view->parent())) {
        Element* owner = view->frame().ownerElement();
        if (owner && !owner->isFocusable()) {
            isFocusable = false;
            break;
        }
    }

    return isFocusable;
}
#endif

FocusCandidate::FocusCandidate(Node* node, FocusDirection direction)
    : visibleNode(nullptr)
    , focusableNode(nullptr)
    , enclosingScrollableBox(nullptr)
    , distance(maxDistance())
    , alignment(None)
    , isOffscreen(true)
    , isOffscreenAfterScrolling(true)
#if PLATFORM(WKC)
    , isOrgRectEmpty(false)
    , exitRect(0,0,0,0)
    , entryRect(0,0,0,0)
    , direction(FocusDirectionNone)
#endif
{
    ASSERT(is<Element>(node));

    if (is<HTMLAreaElement>(*node)) {
        HTMLAreaElement& area = downcast<HTMLAreaElement>(*node);
        HTMLImageElement* image = area.imageElement();
        if (!image || !image->renderer())
            return;

        visibleNode = image;
        rect = virtualRectForAreaElementAndDirection(&area, direction);
    } else {
        if (!node->renderer())
            return;

        visibleNode = node;
        rect = nodeRectInAbsoluteCoordinates(node, true /* ignore border */);
    }

    focusableNode = node;
#if PLATFORM(WKC)
    FrameView* frameView = node->document().view();
    if (frameView) {
        if (frameView->parent()) {
            frameView = frameView->frame().tree().top().view();
        }
        IntRect r(rect.x(), rect.y(), rect.width(), rect.height());
        frameView->contentsToWindow(r);
        isOffscreen = !frameView->visibleContentRect().intersects(r);
    }
#else
    isOffscreen = hasOffscreenRect(visibleNode);
#endif
    isOffscreenAfterScrolling = hasOffscreenRect(visibleNode, direction);
#if PLATFORM(WKC)
    if (visibleNode->hasTagName(HTMLNames::aTag) && visibleNode->firstChild()) {
        ASSERT(node->renderer());
        LayoutRect org_rect = rectToAbsoluteCoordinates(visibleNode->document().frame(), node->renderer()->absoluteBoundingBoxRect(true));
        isOrgRectEmpty = org_rect.isEmpty();
    }
#endif
}

bool isSpatialNavigationEnabled(const Frame* frame)
{
    return (frame && frame->settings().spatialNavigationEnabled());
}

#if !PLATFORM(WKC)
static RectsAlignment alignmentForRects(FocusDirection direction, const LayoutRect& curRect, const LayoutRect& targetRect, const LayoutSize& viewSize)
#else
RectsAlignment alignmentForRects(FocusDirection direction, const LayoutRect& curRect, const LayoutRect& targetRect, const LayoutSize& viewSize)
#endif
{
    // If we found a node in full alignment, but it is too far away, ignore it.
    if (areRectsMoreThanFullScreenApart(direction, curRect, targetRect, viewSize))
        return None;

#if PLATFORM(WKC)
    bool intersected = curRect.intersects(targetRect);

    if (isDiagonalDirection(direction)) {
        if (isRectInsideEffectiveAngleRange(direction, curRect, targetRect))
            return intersected ? OverlapFull : Full;
        return None;
    }

    if (areRectsFullyAligned(direction, curRect, targetRect))
        return intersected ? OverlapFull : Full;

    if (areRectsPartiallyAligned(direction, curRect, targetRect))
        return intersected ? OverlapPartial : Partial;

#else
    if (areRectsFullyAligned(direction, curRect, targetRect))
        return Full;

    if (areRectsPartiallyAligned(direction, curRect, targetRect))
        return Partial;
#endif

    return None;
}

static inline bool isHorizontalMove(FocusDirection direction)
{
    return direction == FocusDirectionLeft || direction == FocusDirectionRight;
}

static inline LayoutUnit start(FocusDirection direction, const LayoutRect& rect)
{
    return isHorizontalMove(direction) ? rect.y() : rect.x();
}

static inline LayoutUnit middle(FocusDirection direction, const LayoutRect& rect)
{
    LayoutPoint center(rect.center());
    return isHorizontalMove(direction) ? center.y(): center.x();
}

static inline LayoutUnit end(FocusDirection direction, const LayoutRect& rect)
{
    return isHorizontalMove(direction) ? rect.maxY() : rect.maxX();
}

// This method checks if rects |a| and |b| are fully aligned either vertically or
// horizontally. In general, rects whose central point falls between the top or
// bottom of each other are considered fully aligned.
// Rects that match this criteria are preferable target nodes in move focus changing
// operations.
// * a = Current focused node's rect.
// * b = Focus candidate node's rect.
static bool areRectsFullyAligned(FocusDirection direction, const LayoutRect& a, const LayoutRect& b)
{
    LayoutUnit aStart, bStart, aEnd, bEnd;

#if !PLATFORM(WKC)
    switch (direction) {
    case FocusDirectionLeft:
        aStart = a.x();
        bEnd = b.maxX();
        break;
    case FocusDirectionRight:
        aStart = b.x();
        bEnd = a.maxX();
        break;
    case FocusDirectionUp:
        aStart = a.y();
        bEnd = b.y();
        break;
    case FocusDirectionDown:
        aStart = b.y();
        bEnd = a.y();
        break;
    default:
        ASSERT_NOT_REACHED();
        return false;
    }

    if (aStart < bEnd)
        return false;
#endif

    aStart = start(direction, a);
    bStart = start(direction, b);

    LayoutUnit aMiddle = middle(direction, a);
    LayoutUnit bMiddle = middle(direction, b);

    aEnd = end(direction, a);
    bEnd = end(direction, b);

    // Picture of the totally aligned logic:
    //
    //     Horizontal    Vertical        Horizontal     Vertical
    //  ****************************  *****************************
    //  *  _          *   _ _ _ _  *  *         _   *      _ _    *
    //  * |_|     _   *  |_|_|_|_| *  *  _     |_|  *     |_|_|   *
    //  * |_|....|_|  *      .     *  * |_|....|_|  *       .     *
    //  * |_|    |_| (1)     .     *  * |_|    |_| (2)      .     *
    //  * |_|         *     _._    *  *        |_|  *    _ _._ _  *
    //  *             *    |_|_|   *  *             *   |_|_|_|_| *
    //  *             *            *  *             *             *
    //  ****************************  *****************************

    //     Horizontal    Vertical        Horizontal     Vertical
    //  ****************************  *****************************
    //  *  _......_   *   _ _ _ _  *  *  _          *    _ _ _ _  *
    //  * |_|    |_|  *  |_|_|_|_| *  * |_|     _   *   |_|_|_|_| *
    //  * |_|    |_|  *  .         *  * |_|    |_|  *           . *
    //  * |_|        (3) .         *  * |_|....|_| (4)          . *
    //  *             *  ._ _      *  *             *        _ _. *
    //  *             *  |_|_|     *  *             *       |_|_| *
    //  *             *            *  *             *             *
    //  ****************************  *****************************

    return ((bMiddle >= aStart && bMiddle <= aEnd) // (1)
            || (aMiddle >= bStart && aMiddle <= bEnd) // (2)
            || (bStart == aStart) // (3)
            || (bEnd == aEnd)); // (4)
}

// This method checks if |start| and |dest| have a partial intersection, either
// horizontally or vertically.
// * a = Current focused node's rect.
// * b = Focus candidate node's rect.
static bool areRectsPartiallyAligned(FocusDirection direction, const LayoutRect& a, const LayoutRect& b)
{
    LayoutUnit aStart  = start(direction, a);
    LayoutUnit bStart  = start(direction, b);
    LayoutUnit bMiddle = middle(direction, b);
    LayoutUnit aEnd = end(direction, a);
    LayoutUnit bEnd = end(direction, b);

    // Picture of the partially aligned logic:
    //
    //    Horizontal       Vertical
    // ********************************
    // *  _            *   _ _ _      *
    // * |_|           *  |_|_|_|     *
    // * |_|.... _     *      . .     *
    // * |_|    |_|    *      . .     *
    // * |_|....|_|    *      ._._ _  *
    // *        |_|    *      |_|_|_| *
    // *        |_|    *              *
    // *               *              *
    // ********************************
    //
    // ... and variants of the above cases.
    return ((bStart >= aStart && bStart <= aEnd)
            || (bMiddle >= aStart && bMiddle <= aEnd)
            || (bEnd >= aStart && bEnd <= aEnd));
}

static bool areRectsMoreThanFullScreenApart(FocusDirection direction, const LayoutRect& curRect, const LayoutRect& targetRect, const LayoutSize& viewSize)
{
#if !PLATFORM(WKC)
    ASSERT(isRectInDirection(direction, curRect, targetRect));
#endif

    switch (direction) {
    case FocusDirectionLeft:
        return curRect.x() - targetRect.maxX() > viewSize.width();
    case FocusDirectionRight:
        return targetRect.x() - curRect.maxX() > viewSize.width();
    case FocusDirectionUp:
        return curRect.y() - targetRect.maxY() > viewSize.height();
    case FocusDirectionDown:
        return targetRect.y() - curRect.maxY() > viewSize.height();
#if PLATFORM(WKC)
    case FocusDirectionUpLeft:
        return (curRect.x() - targetRect.maxX() > viewSize.width()) || (curRect.y() - targetRect.maxY() > viewSize.height());
    case FocusDirectionUpRight:
        return (targetRect.x() - curRect.maxX() > viewSize.width()) || (curRect.y() - targetRect.maxY() > viewSize.height());
    case FocusDirectionDownLeft:
        return (curRect.x() - targetRect.maxX() > viewSize.width()) || (targetRect.y() - curRect.maxY() > viewSize.height());
    case FocusDirectionDownRight:
        return (targetRect.x() - curRect.maxX() > viewSize.width()) || (targetRect.y() - curRect.maxY() > viewSize.height());
#endif
    default:
        ASSERT_NOT_REACHED();
        return true;
    }
}

// Return true if rect |a| is below |b|. False otherwise.
static inline bool below(const LayoutRect& a, const LayoutRect& b)
{
    return a.y() > b.maxY();
}

// Return true if rect |a| is on the right of |b|. False otherwise.
static inline bool rightOf(const LayoutRect& a, const LayoutRect& b)
{
    return a.x() > b.maxX();
}

#if PLATFORM(WKC)
struct LayoutLine {
    LayoutPoint start;
    LayoutPoint end;
};

// Return true if point |p| is below line |a|. False otherwise.
static inline bool below(const LayoutPoint& p, const LayoutLine& line)
{
    if (line.end.x() - line.start.x() > 0)
        return (line.end.x() - line.start.x()) * (p.y() - line.start.y()) + (line.end.y() - line.start.y()) * (line.start.x() - p.x()) > 0;
    else
        return (line.end.x() - line.start.x()) * (p.y() - line.start.y()) + (line.end.y() - line.start.y()) * (line.start.x() - p.x()) < 0;
}

static double cross(const LayoutPoint& a1, const LayoutPoint& a2, const LayoutPoint& p)
{
    LayoutPoint va(a2.x() - a1.x(), a2.y() - a1.y());
    LayoutPoint vp(p.x() - a1.x(), p.y() - a1.y());
    return va.x() * vp.y() - va.y() * vp.x();
}

// Note: |b| may be a line segment, whereas |a| must be a line.
static bool areLinesCrossed(const LayoutLine& a, const LayoutLine& b)
{
    return cross(a.start, a.end, b.start) * cross(a.start, a.end, b.end) < 0;
}

// Note: |b1b2| may be a line segment, whereas |a1a2| must be a line.
static bool intersectionOfLines(const LayoutLine& a, const LayoutLine& b, LayoutPoint& intersectionPoint)
{
    double c1 = cross(a.start, a.end, b.start);
    if (c1 == 0) {
        intersectionPoint.setX(b.start.x());
        intersectionPoint.setY(b.start.y());
        return true;
    }

    double c2 = cross(a.start, a.end, b.end);
    if (c2 == 0) {
        intersectionPoint.setX(b.end.x());
        intersectionPoint.setY(b.end.y());
        return true;
    }

    if (c1 * c2 > 0)
        return false;

    double proportional = fabs(c1) / (fabs(c1) + fabs(c2));

    intersectionPoint.setX(b.start.x() + (b.end.x() - b.start.x()) * proportional);
    intersectionPoint.setY(b.start.y() + (b.end.y() - b.start.y()) * proportional);

    return true;
}

static bool getEffectiveAngleLine(FocusDirection direction, const LayoutRect& curRect, LayoutLine& leftOrUpperLine, LayoutLine& rightOrLowerLine)
{
    if (gMinEffectiveAngleForDiagonalNavigation <= 0 || 90 <= gMinEffectiveAngleForDiagonalNavigation || gMaxEffectiveAngleForDiagonalNavigation <= 0 || 90 <= gMaxEffectiveAngleForDiagonalNavigation) {
        gMinEffectiveAngleForDiagonalNavigation = 20;
        gMaxEffectiveAngleForDiagonalNavigation = 40;
    }
    if (gEffectiveAngleForHorizontalNavigation <= 0 || 90 <= gEffectiveAngleForHorizontalNavigation) {
        gEffectiveAngleForHorizontalNavigation = 89;
    }
    if (gEffectiveAngleForVerticalNavigation <= 0 || 90 <= gEffectiveAngleForVerticalNavigation) {
        gEffectiveAngleForVerticalNavigation = 89;
    }

    ASSERT(gMinEffectiveAngleForDiagonalNavigation < gMaxEffectiveAngleForDiagonalNavigation);

    float leftOrUpperLineTan;
    float rightOrLowerLineTan;

    LayoutUnit visibleContentWidth = 10000; // FIXME: Set this to the width of visible content.

    switch (direction) {
    case FocusDirectionLeft:
    case FocusDirectionRight:
        leftOrUpperLineTan = tan(deg2rad(static_cast<float>(gEffectiveAngleForHorizontalNavigation)));
        rightOrLowerLineTan = leftOrUpperLineTan;
        break;
    case FocusDirectionUp:
    case FocusDirectionDown:
        leftOrUpperLineTan = tan(deg2rad(static_cast<float>(90 - gEffectiveAngleForVerticalNavigation)));
        rightOrLowerLineTan = leftOrUpperLineTan;
        break;
    case FocusDirectionUpLeft:
    case FocusDirectionUpRight:
    case FocusDirectionDownLeft:
    case FocusDirectionDownRight:
        leftOrUpperLineTan = tan(deg2rad(static_cast<float>(gMaxEffectiveAngleForDiagonalNavigation)));
        rightOrLowerLineTan = tan(deg2rad(static_cast<float>(gMinEffectiveAngleForDiagonalNavigation)));
        break;
    default:
        ASSERT_NOT_REACHED();
        return false;
    }

    switch (direction) {
    case FocusDirectionLeft:
        {
            LayoutPoint origin(curRect.x(), curRect.y());
            leftOrUpperLine.start.setX(origin.x());
            leftOrUpperLine.start.setY(origin.y());
            leftOrUpperLine.end.setX(origin.x() - visibleContentWidth);
            leftOrUpperLine.end.setY(origin.y() - leftOrUpperLineTan * visibleContentWidth);

            origin = LayoutPoint(curRect.x(), curRect.maxY());
            rightOrLowerLine.start.setX(origin.x());
            rightOrLowerLine.start.setY(origin.y());
            rightOrLowerLine.end.setX(origin.x() - visibleContentWidth);
            rightOrLowerLine.end.setY(origin.y() + rightOrLowerLineTan * visibleContentWidth);
        }
        break;
    case FocusDirectionRight:
        {
            LayoutPoint origin(curRect.maxX(), curRect.y());
            leftOrUpperLine.start.setX(origin.x());
            leftOrUpperLine.start.setY(origin.y());
            leftOrUpperLine.end.setX(origin.x() + visibleContentWidth);
            leftOrUpperLine.end.setY(origin.y() - leftOrUpperLineTan * visibleContentWidth);

            origin = LayoutPoint(curRect.maxX(), curRect.maxY());
            rightOrLowerLine.start.setX(origin.x());
            rightOrLowerLine.start.setY(origin.y());
            rightOrLowerLine.end.setX(origin.x() + visibleContentWidth);
            rightOrLowerLine.end.setY(origin.y() + rightOrLowerLineTan * visibleContentWidth);
        }
        break;
    case FocusDirectionUp:
        {
            LayoutPoint origin(curRect.x(), curRect.y());
            leftOrUpperLine.start.setX(origin.x());
            leftOrUpperLine.start.setY(origin.y());
            leftOrUpperLine.end.setX(origin.x() - visibleContentWidth);
            leftOrUpperLine.end.setY(origin.y() - leftOrUpperLineTan * visibleContentWidth);

            origin = LayoutPoint(curRect.maxX(), curRect.y());
            rightOrLowerLine.start.setX(origin.x());
            rightOrLowerLine.start.setY(origin.y());
            rightOrLowerLine.end.setX(origin.x() + visibleContentWidth);
            rightOrLowerLine.end.setY(origin.y() - rightOrLowerLineTan * visibleContentWidth);
        }
        break;
    case FocusDirectionDown:
        {
            LayoutPoint origin(curRect.x(), curRect.maxY());
            leftOrUpperLine.start.setX(origin.x());
            leftOrUpperLine.start.setY(origin.y());
            leftOrUpperLine.end.setX(origin.x() - visibleContentWidth);
            leftOrUpperLine.end.setY(origin.y() + leftOrUpperLineTan * visibleContentWidth);

            origin = LayoutPoint(curRect.maxX(), curRect.maxY());
            rightOrLowerLine.start.setX(origin.x());
            rightOrLowerLine.start.setY(origin.y());
            rightOrLowerLine.end.setX(origin.x() + visibleContentWidth);
            rightOrLowerLine.end.setY(origin.y() + rightOrLowerLineTan * visibleContentWidth);
        }
        break;
    case FocusDirectionUpLeft:
        {
            LayoutPoint origin(curRect.x(), curRect.y());
            leftOrUpperLine.start.setX(origin.x());
            leftOrUpperLine.start.setY(origin.y());
            leftOrUpperLine.end.setX(origin.x() - visibleContentWidth);
            leftOrUpperLine.end.setY(origin.y() - leftOrUpperLineTan * visibleContentWidth);

            rightOrLowerLine.start.setX(origin.x());
            rightOrLowerLine.start.setY(origin.y());
            rightOrLowerLine.end.setX(origin.x() - visibleContentWidth);
            rightOrLowerLine.end.setY(origin.y() - rightOrLowerLineTan * visibleContentWidth);
        }
        break;
    case FocusDirectionUpRight:
        {
            LayoutPoint origin(curRect.maxX(), curRect.y());
            leftOrUpperLine.start.setX(origin.x());
            leftOrUpperLine.start.setY(origin.y());
            leftOrUpperLine.end.setX(origin.x() + visibleContentWidth);
            leftOrUpperLine.end.setY(origin.y() - leftOrUpperLineTan * visibleContentWidth);

            rightOrLowerLine.start.setX(origin.x());
            rightOrLowerLine.start.setY(origin.y());
            rightOrLowerLine.end.setX(origin.x() + visibleContentWidth);
            rightOrLowerLine.end.setY(origin.y() - rightOrLowerLineTan * visibleContentWidth);
        }
        break;
    case FocusDirectionDownLeft:
        {
            LayoutPoint origin(curRect.x(), curRect.maxY());
            leftOrUpperLine.start.setX(origin.x());
            leftOrUpperLine.start.setY(origin.y());
            leftOrUpperLine.end.setX(origin.x() - visibleContentWidth);
            leftOrUpperLine.end.setY(origin.y() + rightOrLowerLineTan * visibleContentWidth);

            rightOrLowerLine.start.setX(origin.x());
            rightOrLowerLine.start.setY(origin.y());
            rightOrLowerLine.end.setX(origin.x() - visibleContentWidth);
            rightOrLowerLine.end.setY(origin.y() + leftOrUpperLineTan * visibleContentWidth);
        }
        break;
    case FocusDirectionDownRight:
        {
            LayoutPoint origin(curRect.maxX(), curRect.maxY());
            leftOrUpperLine.start.setX(origin.x());
            leftOrUpperLine.start.setY(origin.y());
            leftOrUpperLine.end.setX(origin.x() + visibleContentWidth);
            leftOrUpperLine.end.setY(origin.y() + rightOrLowerLineTan * visibleContentWidth);

            rightOrLowerLine.start.setX(origin.x());
            rightOrLowerLine.start.setY(origin.y());
            rightOrLowerLine.end.setX(origin.x() + visibleContentWidth);
            rightOrLowerLine.end.setY(origin.y() + leftOrUpperLineTan * visibleContentWidth);
        }
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    return true;
}

static bool isPointInsideEffectiveAngleRange(FocusDirection direction, const LayoutRect& curRect, const LayoutPoint& targetPoint)
{
    LayoutLine leftOrUpperEffectiveAngleLine;
    LayoutLine rightOrLowerEffectiveAngleLine;

    getEffectiveAngleLine(direction, curRect, leftOrUpperEffectiveAngleLine, rightOrLowerEffectiveAngleLine);

    switch (direction) {
    case FocusDirectionUp:
        return !below(targetPoint, leftOrUpperEffectiveAngleLine) && !below(targetPoint, rightOrLowerEffectiveAngleLine);
    case FocusDirectionDown:
        return below(targetPoint, leftOrUpperEffectiveAngleLine) && below(targetPoint, rightOrLowerEffectiveAngleLine);
    case FocusDirectionLeft:
    case FocusDirectionRight:
    case FocusDirectionUpLeft:
    case FocusDirectionUpRight:
    case FocusDirectionDownLeft:
    case FocusDirectionDownRight:
        return below(targetPoint, leftOrUpperEffectiveAngleLine) && !below(targetPoint, rightOrLowerEffectiveAngleLine);
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
}

static bool isRectInsideEffectiveAngleRange(FocusDirection direction, const LayoutRect& curRect, const LayoutRect& targetRect)
{
    if (targetRect.contains(curRect) || curRect.contains(targetRect))
        return true;

    bool intersected = curRect.intersects(targetRect);

    LayoutLine targetDiagonalLine;

    LayoutLine leftOrUpperBoundaryLine;
    LayoutLine rightOrLowerBoundaryLine;

    switch (direction) {
    case FocusDirectionLeft:
        if (intersected)
            return targetRect.maxX() < curRect.maxX();
        if (targetRect.maxX() > curRect.x())
            return false;
        if (isPointInsideEffectiveAngleRange(direction, curRect, targetRect.center()))
            return true;
        getEffectiveAngleLine(direction, curRect, leftOrUpperBoundaryLine, rightOrLowerBoundaryLine);
        targetDiagonalLine.start.setX(targetRect.x());
        targetDiagonalLine.start.setY(targetRect.maxY());
        targetDiagonalLine.end.setX(targetRect.maxX());
        targetDiagonalLine.end.setY(targetRect.y());
        if (areLinesCrossed(leftOrUpperBoundaryLine, targetDiagonalLine))
            return true;
        targetDiagonalLine.start.setX(targetRect.x());
        targetDiagonalLine.start.setY(targetRect.y());
        targetDiagonalLine.end.setX(targetRect.maxX());
        targetDiagonalLine.end.setY(targetRect.maxY());
        if (areLinesCrossed(rightOrLowerBoundaryLine, targetDiagonalLine))
            return true;
        break;
    case FocusDirectionRight:
        if (intersected)
            return targetRect.x() > curRect.x();
        if (targetRect.x() < curRect.maxX())
            return false;
        if (isPointInsideEffectiveAngleRange(direction, curRect, targetRect.center()))
            return true;
        getEffectiveAngleLine(direction, curRect, leftOrUpperBoundaryLine, rightOrLowerBoundaryLine);
        targetDiagonalLine.start.setX(targetRect.x());
        targetDiagonalLine.start.setY(targetRect.y());
        targetDiagonalLine.end.setX(targetRect.maxX());
        targetDiagonalLine.end.setY(targetRect.maxY());
        if (areLinesCrossed(leftOrUpperBoundaryLine, targetDiagonalLine))
            return true;
        targetDiagonalLine.start.setX(targetRect.x());
        targetDiagonalLine.start.setY(targetRect.maxY());
        targetDiagonalLine.end.setX(targetRect.maxX());
        targetDiagonalLine.end.setY(targetRect.y());
        if (areLinesCrossed(rightOrLowerBoundaryLine, targetDiagonalLine))
            return true;
        break;
    case FocusDirectionUp:
        if (intersected)
            return targetRect.maxY() < curRect.maxY();
        if (targetRect.maxY() > curRect.y())
            return false;
        if (isPointInsideEffectiveAngleRange(direction, curRect, targetRect.center()))
            return true;
        getEffectiveAngleLine(direction, curRect, leftOrUpperBoundaryLine, rightOrLowerBoundaryLine);
        targetDiagonalLine.start.setX(targetRect.x());
        targetDiagonalLine.start.setY(targetRect.maxY());
        targetDiagonalLine.end.setX(targetRect.maxX());
        targetDiagonalLine.end.setY(targetRect.y());
        if (areLinesCrossed(leftOrUpperBoundaryLine, targetDiagonalLine))
            return true;
        targetDiagonalLine.start.setX(targetRect.x());
        targetDiagonalLine.start.setY(targetRect.y());
        targetDiagonalLine.end.setX(targetRect.maxX());
        targetDiagonalLine.end.setY(targetRect.maxY());
        if (areLinesCrossed(rightOrLowerBoundaryLine, targetDiagonalLine))
            return true;
        break;
    case FocusDirectionDown:
        if (intersected)
            return targetRect.y() > curRect.y();
        if (targetRect.y() < curRect.maxY())
            return false;
        if (isPointInsideEffectiveAngleRange(direction, curRect, targetRect.center()))
            return true;
        getEffectiveAngleLine(direction, curRect, leftOrUpperBoundaryLine, rightOrLowerBoundaryLine);
        targetDiagonalLine.start.setX(targetRect.x());
        targetDiagonalLine.start.setY(targetRect.y());
        targetDiagonalLine.end.setX(targetRect.maxX());
        targetDiagonalLine.end.setY(targetRect.maxY());
        if (areLinesCrossed(leftOrUpperBoundaryLine, targetDiagonalLine))
            return true;
        targetDiagonalLine.start.setX(targetRect.x());
        targetDiagonalLine.start.setY(targetRect.maxY());
        targetDiagonalLine.end.setX(targetRect.maxX());
        targetDiagonalLine.end.setY(targetRect.y());
        if (areLinesCrossed(rightOrLowerBoundaryLine, targetDiagonalLine))
            return true;
        break;
    case FocusDirectionUpLeft:
        if (intersected)
            return targetRect.maxX() < curRect.maxX() && targetRect.maxY() < curRect.maxY();
        if (targetRect.x() >= curRect.x() || targetRect.y() >= curRect.y())
            return false;
        if (isPointInsideEffectiveAngleRange(direction, curRect, targetRect.center()))
            return true;
        targetDiagonalLine.start.setX(targetRect.x());
        targetDiagonalLine.start.setY(targetRect.maxY());
        targetDiagonalLine.end.setX(targetRect.maxX());
        targetDiagonalLine.end.setY(targetRect.y());
        getEffectiveAngleLine(direction, curRect, leftOrUpperBoundaryLine, rightOrLowerBoundaryLine);
        if (areLinesCrossed(leftOrUpperBoundaryLine, targetDiagonalLine))
            return true;
        if (areLinesCrossed(rightOrLowerBoundaryLine, targetDiagonalLine))
            return true;
        break;
    case FocusDirectionUpRight:
        if (intersected)
            return targetRect.x() > curRect.x() && targetRect.maxY() < curRect.maxY();
        if (targetRect.maxX() <= curRect.maxX() || targetRect.y() >= curRect.y())
            return false;
        if (isPointInsideEffectiveAngleRange(direction, curRect, targetRect.center()))
            return true;
        targetDiagonalLine.start.setX(targetRect.x());
        targetDiagonalLine.start.setY(targetRect.y());
        targetDiagonalLine.end.setX(targetRect.maxX());
        targetDiagonalLine.end.setY(targetRect.maxY());
        getEffectiveAngleLine(direction, curRect, leftOrUpperBoundaryLine, rightOrLowerBoundaryLine);
        if (areLinesCrossed(leftOrUpperBoundaryLine, targetDiagonalLine))
            return true;
        if (areLinesCrossed(rightOrLowerBoundaryLine, targetDiagonalLine))
            return true;
        break;
    case FocusDirectionDownLeft:
        if (intersected)
            return targetRect.maxX() < curRect.maxX() && targetRect.y() > curRect.y();
        if (targetRect.x() >= curRect.x() || targetRect.maxY() <= curRect.maxY())
            return false;
        if (isPointInsideEffectiveAngleRange(direction, curRect, targetRect.center()))
            return true;
        targetDiagonalLine.start.setX(targetRect.x());
        targetDiagonalLine.start.setY(targetRect.y());
        targetDiagonalLine.end.setX(targetRect.maxX());
        targetDiagonalLine.end.setY(targetRect.maxY());
        getEffectiveAngleLine(direction, curRect, leftOrUpperBoundaryLine, rightOrLowerBoundaryLine);
        if (areLinesCrossed(leftOrUpperBoundaryLine, targetDiagonalLine))
            return true;
        if (areLinesCrossed(rightOrLowerBoundaryLine, targetDiagonalLine))
            return true;
        break;
    case FocusDirectionDownRight:
        if (intersected)
            return targetRect.x() > curRect.x() && targetRect.y() > curRect.y();
        if (targetRect.maxX() <= curRect.maxX() || targetRect.maxY() <= curRect.maxY())
            return false;
        if (isPointInsideEffectiveAngleRange(direction, curRect, targetRect.center()))
            return true;
        targetDiagonalLine.start.setX(targetRect.x());
        targetDiagonalLine.start.setY(targetRect.maxY());
        targetDiagonalLine.end.setX(targetRect.maxX());
        targetDiagonalLine.end.setY(targetRect.y());
        getEffectiveAngleLine(direction, curRect, leftOrUpperBoundaryLine, rightOrLowerBoundaryLine);
        if (areLinesCrossed(leftOrUpperBoundaryLine, targetDiagonalLine))
            return true;
        if (areLinesCrossed(rightOrLowerBoundaryLine, targetDiagonalLine))
            return true;
        break;
    default:
        break;
    }
    return false;
}
#endif

#if !PLATFORM(WKC)
static bool isRectInDirection(FocusDirection direction, const LayoutRect& curRect, const LayoutRect& targetRect)
#else
bool isRectInDirection(FocusDirection direction, const LayoutRect& curRect, const LayoutRect& targetRect)
#endif
{
    switch (direction) {
    case FocusDirectionLeft:
        return targetRect.maxX() <= curRect.x();
    case FocusDirectionRight:
        return targetRect.x() >= curRect.maxX();
    case FocusDirectionUp:
        return targetRect.maxY() <= curRect.y();
    case FocusDirectionDown:
        return targetRect.y() >= curRect.maxY();
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
}

// Checks if |node| is offscreen the visible area (viewport) of its container
// document. In case it is, one can scroll in direction or take any different
// desired action later on.
bool hasOffscreenRect(Node* node, FocusDirection direction)
{
    // Get the FrameView in which |node| is (which means the current viewport if |node|
    // is not in an inner document), so we can check if its content rect is visible
    // before we actually move the focus to it.
    FrameView* frameView = node->document().view();
    if (!frameView)
        return true;

    ASSERT(!frameView->needsLayout());

    LayoutRect containerViewportRect = frameView->visibleContentRect();
    // We want to select a node if it is currently off screen, but will be
    // exposed after we scroll. Adjust the viewport to post-scrolling position.
    // If the container has overflow:hidden, we cannot scroll, so we do not pass direction
    // and we do not adjust for scrolling.
    switch (direction) {
    case FocusDirectionLeft:
        containerViewportRect.setX(containerViewportRect.x() - Scrollbar::pixelsPerLineStep());
        containerViewportRect.setWidth(containerViewportRect.width() + Scrollbar::pixelsPerLineStep());
        break;
    case FocusDirectionRight:
        containerViewportRect.setWidth(containerViewportRect.width() + Scrollbar::pixelsPerLineStep());
        break;
    case FocusDirectionUp:
        containerViewportRect.setY(containerViewportRect.y() - Scrollbar::pixelsPerLineStep());
        containerViewportRect.setHeight(containerViewportRect.height() + Scrollbar::pixelsPerLineStep());
        break;
    case FocusDirectionDown:
        containerViewportRect.setHeight(containerViewportRect.height() + Scrollbar::pixelsPerLineStep());
        break;
#if PLATFORM(WKC)
    case FocusDirectionUpLeft:
        containerViewportRect.setX(containerViewportRect.x() - Scrollbar::pixelsPerLineStep());
        containerViewportRect.setWidth(containerViewportRect.width() + Scrollbar::pixelsPerLineStep());
        containerViewportRect.setY(containerViewportRect.y() - Scrollbar::pixelsPerLineStep());
        containerViewportRect.setHeight(containerViewportRect.height() + Scrollbar::pixelsPerLineStep());
        break;
    case FocusDirectionUpRight:
        containerViewportRect.setWidth(containerViewportRect.width() + Scrollbar::pixelsPerLineStep());
        containerViewportRect.setY(containerViewportRect.y() - Scrollbar::pixelsPerLineStep());
        containerViewportRect.setHeight(containerViewportRect.height() + Scrollbar::pixelsPerLineStep());
        break;
    case FocusDirectionDownLeft:
        containerViewportRect.setX(containerViewportRect.x() - Scrollbar::pixelsPerLineStep());
        containerViewportRect.setWidth(containerViewportRect.width() + Scrollbar::pixelsPerLineStep());
        containerViewportRect.setHeight(containerViewportRect.height() + Scrollbar::pixelsPerLineStep());
        break;
    case FocusDirectionDownRight:
        containerViewportRect.setWidth(containerViewportRect.width() + Scrollbar::pixelsPerLineStep());
        containerViewportRect.setHeight(containerViewportRect.height() + Scrollbar::pixelsPerLineStep());
        break;
#endif
    default:
        break;
    }

    RenderObject* render = node->renderer();
    if (!render)
        return true;

#if PLATFORM(WKC)
    LayoutRect rect = render->absoluteClippedOverflowRect();
    if (node->hasTagName(HTMLNames::aTag) && node->firstChild()) {
        node->getNodeCompositeRect(&rect);
    }
#else
    LayoutRect rect(render->absoluteClippedOverflowRect());
#endif
    if (rect.isEmpty())
        return true;

    return !containerViewportRect.intersects(rect);
}

bool scrollInDirection(Frame* frame, FocusDirection direction)
{
    ASSERT(frame);

    if (frame && canScrollInDirection(frame->document(), direction)) {
        LayoutUnit dx = 0;
        LayoutUnit dy = 0;
        switch (direction) {
        case FocusDirectionLeft:
            dx = - Scrollbar::pixelsPerLineStep();
            break;
        case FocusDirectionRight:
            dx = Scrollbar::pixelsPerLineStep();
            break;
        case FocusDirectionUp:
            dy = - Scrollbar::pixelsPerLineStep();
            break;
        case FocusDirectionDown:
            dy = Scrollbar::pixelsPerLineStep();
            break;
#if PLATFORM(WKC)
        case FocusDirectionUpLeft:
            dx = - Scrollbar::pixelsPerLineStep();
            dy = - Scrollbar::pixelsPerLineStep();
            break;
        case FocusDirectionUpRight:
            dx = Scrollbar::pixelsPerLineStep();
            dy = - Scrollbar::pixelsPerLineStep();
            break;
        case FocusDirectionDownLeft:
            dx = - Scrollbar::pixelsPerLineStep();
            dy = Scrollbar::pixelsPerLineStep();
            break;
        case FocusDirectionDownRight:
            dx = Scrollbar::pixelsPerLineStep();
            dy = Scrollbar::pixelsPerLineStep();
            break;
#endif
        default:
            ASSERT_NOT_REACHED();
            return false;
        }

        frame->view()->scrollBy(IntSize(dx, dy));
        return true;
    }
    return false;
}

bool scrollInDirection(Node* container, FocusDirection direction)
{
    ASSERT(container);
    if (is<Document>(*container))
        return scrollInDirection(downcast<Document>(*container).frame(), direction);

    if (!container->renderBox())
        return false;

    if (canScrollInDirection(container, direction)) {
        LayoutUnit dx = 0;
        LayoutUnit dy = 0;
        switch (direction) {
        case FocusDirectionLeft:
            dx = - std::min<LayoutUnit>(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollLeft());
            break;
        case FocusDirectionRight:
            ASSERT(container->renderBox()->scrollWidth() > (container->renderBox()->scrollLeft() + container->renderBox()->clientWidth()));
            dx = std::min<LayoutUnit>(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollWidth() - (container->renderBox()->scrollLeft() + container->renderBox()->clientWidth()));
            break;
        case FocusDirectionUp:
            dy = - std::min<LayoutUnit>(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollTop());
            break;
        case FocusDirectionDown:
            ASSERT(container->renderBox()->scrollHeight() - (container->renderBox()->scrollTop() + container->renderBox()->clientHeight()));
            dy = std::min<LayoutUnit>(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollHeight() - (container->renderBox()->scrollTop() + container->renderBox()->clientHeight()));
            break;
#if PLATFORM(WKC)
        case FocusDirectionUpLeft:
            dx = - std::min<LayoutUnit>(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollLeft());
            dy = - std::min<LayoutUnit>(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollTop());
            break;
        case FocusDirectionUpRight:
            ASSERT(container->renderBox()->scrollWidth() > (container->renderBox()->scrollLeft() + container->renderBox()->clientWidth()));
            dx = std::min<LayoutUnit>(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollWidth() - (container->renderBox()->scrollLeft() + container->renderBox()->clientWidth()));
            dy = - std::min<LayoutUnit>(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollTop());
            break;
        case FocusDirectionDownLeft:
            dx = - std::min<LayoutUnit>(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollLeft());
            ASSERT(container->renderBox()->scrollHeight() - (container->renderBox()->scrollTop() + container->renderBox()->clientHeight()));
            dy = std::min<LayoutUnit>(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollHeight() - (container->renderBox()->scrollTop() + container->renderBox()->clientHeight()));
            break;
        case FocusDirectionDownRight:
            ASSERT(container->renderBox()->scrollWidth() > (container->renderBox()->scrollLeft() + container->renderBox()->clientWidth()));
            dx = std::min<LayoutUnit>(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollWidth() - (container->renderBox()->scrollLeft() + container->renderBox()->clientWidth()));
            ASSERT(container->renderBox()->scrollHeight() - (container->renderBox()->scrollTop() + container->renderBox()->clientHeight()));
            dy = std::min<LayoutUnit>(Scrollbar::pixelsPerLineStep(), container->renderBox()->scrollHeight() - (container->renderBox()->scrollTop() + container->renderBox()->clientHeight()));
            break;
#endif
        default:
            ASSERT_NOT_REACHED();
            return false;
        }

        container->renderBox()->enclosingLayer()->scrollByRecursively(IntSize(dx, dy));
        return true;
    }

    return false;
}

static void deflateIfOverlapped(LayoutRect& a, LayoutRect& b)
{
    if (!a.intersects(b) || a.contains(b) || b.contains(a))
        return;

    LayoutUnit deflateFactor = -fudgeFactor();

    // Avoid negative width or height values.
    if ((a.width() + 2 * deflateFactor > 0) && (a.height() + 2 * deflateFactor > 0))
        a.inflate(deflateFactor);

    if ((b.width() + 2 * deflateFactor > 0) && (b.height() + 2 * deflateFactor > 0))
        b.inflate(deflateFactor);
}

bool isScrollableNode(const Node* node)
{
    ASSERT(!node->isDocumentNode());

    if (!node)
        return false;

#if PLATFORM(WKC)
    if (RenderObject* renderer = node->renderer()) {
        if (!is<RenderBox>(*renderer) || !downcast<RenderBox>(*renderer).canBeScrolledAndHasScrollableArea())
            return false;
        if (renderer->isTextArea()) {
            // Check shadow tree for the text control.
            HTMLTextFormControlElement& form = downcast<RenderTextControl>(*renderer).textFormControlElement();
            TextControlInnerTextElement* innerTextElement = form.innerTextElement();
            return innerTextElement ? innerTextElement->hasChildNodes() : false;
        } else {
            return node->hasChildNodes();
        }
    }
#else
    if (RenderObject* renderer = node->renderer())
        return is<RenderBox>(*renderer) && downcast<RenderBox>(*renderer).canBeScrolledAndHasScrollableArea() && node->hasChildNodes();
#endif

    return false;
}

Node* scrollableEnclosingBoxOrParentFrameForNodeInDirection(FocusDirection direction, Node* node)
{
    ASSERT(node);
    Node* parent = node;
    do {
        if (is<Document>(*parent))
            parent = downcast<Document>(*parent).document().frame()->ownerElement();
        else
            parent = parent->parentNode();
    } while (parent && !canScrollInDirection(parent, direction) && !is<Document>(*parent));

    return parent;
}

bool canScrollInDirection(const Node* container, FocusDirection direction)
{
    ASSERT(container);

    if (is<HTMLSelectElement>(*container))
        return false;

    if (is<Document>(*container))
        return canScrollInDirection(downcast<Document>(*container).frame(), direction);

    if (!isScrollableNode(container))
        return false;

    switch (direction) {
    case FocusDirectionLeft:
        return (container->renderer()->style().overflowX() != OHIDDEN && container->renderBox()->scrollLeft() > 0);
    case FocusDirectionUp:
        return (container->renderer()->style().overflowY() != OHIDDEN && container->renderBox()->scrollTop() > 0);
    case FocusDirectionRight:
        return (container->renderer()->style().overflowX() != OHIDDEN && container->renderBox()->scrollLeft() + container->renderBox()->clientWidth() < container->renderBox()->scrollWidth());
    case FocusDirectionDown:
        return (container->renderer()->style().overflowY() != OHIDDEN && container->renderBox()->scrollTop() + container->renderBox()->clientHeight() < container->renderBox()->scrollHeight());
#if PLATFORM(WKC)
    case FocusDirectionUpLeft:
        return (container->renderer()->style().overflowX() != OHIDDEN && container->renderBox()->scrollLeft() > 0) &&
               (container->renderer()->style().overflowY() != OHIDDEN && container->renderBox()->scrollTop() > 0);
    case FocusDirectionUpRight:
        return (container->renderer()->style().overflowX() != OHIDDEN && container->renderBox()->scrollLeft() + container->renderBox()->clientWidth() < container->renderBox()->scrollWidth()) &&
               (container->renderer()->style().overflowY() != OHIDDEN && container->renderBox()->scrollTop() > 0);
    case FocusDirectionDownLeft:
        return (container->renderer()->style().overflowX() != OHIDDEN && container->renderBox()->scrollLeft() > 0) &&
               (container->renderer()->style().overflowY() != OHIDDEN && container->renderBox()->scrollTop() + container->renderBox()->clientHeight() < container->renderBox()->scrollHeight());
    case FocusDirectionDownRight:
        return (container->renderer()->style().overflowX() != OHIDDEN && container->renderBox()->scrollLeft() + container->renderBox()->clientWidth() < container->renderBox()->scrollWidth()) &&
               (container->renderer()->style().overflowY() != OHIDDEN && container->renderBox()->scrollTop() + container->renderBox()->clientHeight() < container->renderBox()->scrollHeight());
#endif
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
}

bool canScrollInDirection(const Frame* frame, FocusDirection direction)
{
    if (!frame->view())
        return false;
    ScrollbarMode verticalMode;
    ScrollbarMode horizontalMode;
    frame->view()->calculateScrollbarModesForLayout(horizontalMode, verticalMode);
    if ((direction == FocusDirectionLeft || direction == FocusDirectionRight) && ScrollbarAlwaysOff == horizontalMode)
        return false;
    if ((direction == FocusDirectionUp || direction == FocusDirectionDown) &&  ScrollbarAlwaysOff == verticalMode)
        return false;
#if PLATFORM(WKC)
    if (isDiagonalDirection(direction) && (ScrollbarAlwaysOff == horizontalMode || ScrollbarAlwaysOff == verticalMode))
        return false;
#endif
    LayoutSize size = frame->view()->totalContentsSize();
    LayoutSize offset = frame->view()->scrollOffset();
    LayoutRect rect = frame->view()->unobscuredContentRectIncludingScrollbars();

    switch (direction) {
    case FocusDirectionLeft:
        return offset.width() > 0;
    case FocusDirectionUp:
        return offset.height() > 0;
    case FocusDirectionRight:
        return rect.width() + offset.width() < size.width();
    case FocusDirectionDown:
        return rect.height() + offset.height() < size.height();
#if PLATFORM(WKC)
    case FocusDirectionUpLeft:
        return (offset.width() > 0) && (offset.height() > 0);
    case FocusDirectionUpRight:
        return (rect.width() + offset.width() < size.width()) && (offset.height() > 0);
    case FocusDirectionDownLeft:
        return (offset.width() > 0) && (rect.height() + offset.height() < size.height());
    case FocusDirectionDownRight:
        return (rect.width() + offset.width() < size.width()) && (rect.height() + offset.height() < size.height());
#endif
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
}

#if PLATFORM(WKC)
static int offsetLeft(Element* in_element)
{
    in_element->document().updateLayoutIgnorePendingStylesheets();
    if (RenderBoxModelObject* rend = in_element->renderBoxModelObject())
        return rend->offsetLeft();
    return 0;
}

static int offsetTop(Element* in_element)
{
    in_element->document().updateLayoutIgnorePendingStylesheets();
    if (RenderBoxModelObject* rend = in_element->renderBoxModelObject())
        return rend->offsetTop();
    return 0;
}
#endif

// FIXME: This is completely broken. This should be deleted and callers should be calling ScrollView::contentsToWindow() instead.
#if !PLATFORM(WKC)
static LayoutRect rectToAbsoluteCoordinates(Frame* initialFrame, const LayoutRect& initialRect)
#else
LayoutRect rectToAbsoluteCoordinates(Frame* initialFrame, const LayoutRect& initialRect)
#endif
{
    LayoutRect rect = initialRect;
    for (Frame* frame = initialFrame; frame; frame = frame->tree().parent()) {
        if (Element* element = frame->ownerElement()) {
            do {
#if PLATFORM(WKC)
                rect.move(offsetLeft(element), offsetTop(element));
                if (element->renderer() && element->renderer()->style().position() == FixedPosition) {
                    rect.move(frame->tree().parent()->view()->scrollOffset());
                }
#else
                rect.move(element->offsetLeft(), element->offsetTop());
#endif
            } while ((element = element->offsetParent()));
            rect.move((-frame->view()->scrollOffset()));
        }
    }
    return rect;
}

LayoutRect nodeRectInAbsoluteCoordinates(Node* node, bool ignoreBorder)
{
#if !PLATFORM(WKC)
    ASSERT(node && node->renderer() && !node->document().view()->needsLayout());
#else
    if (!node)
        return LayoutRect();
    if (!node->renderer())
        return LayoutRect();
#endif

    if (is<Document>(*node))
        return frameRectInAbsoluteCoordinates(downcast<Document>(*node).frame());

#if PLATFORM(WKC)
    LayoutRect rect;
    if (RenderObject* renderer = node->renderer()) {
        rect = renderer->absoluteBoundingBoxRect(true);
        if (node->hasTagName(HTMLNames::aTag) && node->firstChild()) {
            node->getNodeCompositeRect(&rect);
        }
        rect = rectToAbsoluteCoordinates(node->document().frame(), rect);
    }
#else
    LayoutRect rect;
    if (RenderObject* renderer = node->renderer())
        rect = rectToAbsoluteCoordinates(node->document().frame(), renderer->absoluteBoundingBoxRect());
#endif

    // For authors that use border instead of outline in their CSS, we compensate by ignoring the border when calculating
    // the rect of the focused element.
    if (ignoreBorder) {
        rect.move(node->renderer()->style().borderLeftWidth(), node->renderer()->style().borderTopWidth());
        rect.setWidth(rect.width() - node->renderer()->style().borderLeftWidth() - node->renderer()->style().borderRightWidth());
        rect.setHeight(rect.height() - node->renderer()->style().borderTopWidth() - node->renderer()->style().borderBottomWidth());
    }
    return rect;
}

LayoutRect frameRectInAbsoluteCoordinates(Frame* frame)
{
    return rectToAbsoluteCoordinates(frame, frame->view()->visibleContentRect());
}

#if PLATFORM(WKC)
static inline float euclidianDistance(const LayoutPoint& a, const LayoutPoint& b)
{
    float dx = b.x() - a.x();
    float dy = b.y() - a.y();
    return sqrt((dx * dx) + (dy * dy));
}

static void entryAndExitPointsForDiagonalDirection(FocusDirection direction, const LayoutRect& startingRect, const LayoutRect& potentialRect, LayoutPoint& exitPoint, LayoutPoint& entryPoint)
{
    LayoutLine intersectionLineWithLeftOrUpperBoundary;
    LayoutLine intersectionLineWithRightOrLowerBoundary;

    bool isStartingRectInsidePotentialRect = potentialRect.contains(startingRect);

    switch (direction) {
    case FocusDirectionUpLeft:
        // Exit point is the up-left corner of startingRect.
        exitPoint.setX(startingRect.x());
        exitPoint.setY(startingRect.y());
        if (isStartingRectInsidePotentialRect) {
            // Entry point is the up-left corner of potentialRect.
            entryPoint.setX(potentialRect.x());
            entryPoint.setY(potentialRect.y());
            return;
        } else if (isPointInsideEffectiveAngleRange(direction, startingRect, LayoutPoint(potentialRect.maxX(), potentialRect.maxY()))) {
            // Entry point is the down-right corner of potentialRect.
            entryPoint.setX(potentialRect.maxX());
            entryPoint.setY(potentialRect.maxY());
            return;
        }
        // bottom line
        intersectionLineWithLeftOrUpperBoundary.start.setX(potentialRect.x());
        intersectionLineWithLeftOrUpperBoundary.start.setY(potentialRect.maxY());
        intersectionLineWithLeftOrUpperBoundary.end.setX(potentialRect.maxX());
        intersectionLineWithLeftOrUpperBoundary.end.setY(potentialRect.maxY());
        // right line
        intersectionLineWithRightOrLowerBoundary.start.setX(potentialRect.maxX());
        intersectionLineWithRightOrLowerBoundary.start.setY(potentialRect.y());
        intersectionLineWithRightOrLowerBoundary.end.setX(potentialRect.maxX());
        intersectionLineWithRightOrLowerBoundary.end.setY(potentialRect.maxY());
        break;
    case FocusDirectionUpRight:
        // Exit point is the up-right corner of startingRect.
        exitPoint.setX(startingRect.maxX());
        exitPoint.setY(startingRect.y());
        if (isStartingRectInsidePotentialRect) {
            // Entry point is the up-left corner of potentialRect.
            entryPoint.setX(potentialRect.maxX());
            entryPoint.setY(potentialRect.y());
            return;
        } else if (isPointInsideEffectiveAngleRange(direction, startingRect, LayoutPoint(potentialRect.x(), potentialRect.maxY()))) {
            // Entry point is the down-left corner of potentialRect.
            entryPoint.setX(potentialRect.x());
            entryPoint.setY(potentialRect.maxY());
            return;
        }
        // bottom line
        intersectionLineWithLeftOrUpperBoundary.start.setX(potentialRect.x());
        intersectionLineWithLeftOrUpperBoundary.start.setY(potentialRect.maxY());
        intersectionLineWithLeftOrUpperBoundary.end.setX(potentialRect.maxX());
        intersectionLineWithLeftOrUpperBoundary.end.setY(potentialRect.maxY());
        // left line
        intersectionLineWithRightOrLowerBoundary.start.setX(potentialRect.x());
        intersectionLineWithRightOrLowerBoundary.start.setY(potentialRect.y());
        intersectionLineWithRightOrLowerBoundary.end.setX(potentialRect.x());
        intersectionLineWithRightOrLowerBoundary.end.setY(potentialRect.maxY());
        break;
    case FocusDirectionDownLeft:
        // Exit point is the down-left corner of startingRect.
        exitPoint.setX(startingRect.x());
        exitPoint.setY(startingRect.maxY());
        if (isStartingRectInsidePotentialRect) {
            // Entry point is the down-left corner of potentialRect.
            entryPoint.setX(potentialRect.x());
            entryPoint.setY(potentialRect.maxY());
            return;
        } else if (isPointInsideEffectiveAngleRange(direction, startingRect, LayoutPoint(potentialRect.maxX(), potentialRect.y()))) {
            // Entry point is the up-right corner of potentialRect.
            entryPoint.setX(potentialRect.maxX());
            entryPoint.setY(potentialRect.y());
            return;
        }
        // right line
        intersectionLineWithLeftOrUpperBoundary.start.setX(potentialRect.maxX());
        intersectionLineWithLeftOrUpperBoundary.start.setY(potentialRect.y());
        intersectionLineWithLeftOrUpperBoundary.end.setX(potentialRect.maxX());
        intersectionLineWithLeftOrUpperBoundary.end.setY(potentialRect.maxY());
        // top line
        intersectionLineWithRightOrLowerBoundary.start.setX(potentialRect.x());
        intersectionLineWithRightOrLowerBoundary.start.setY(potentialRect.y());
        intersectionLineWithRightOrLowerBoundary.end.setX(potentialRect.maxX());
        intersectionLineWithRightOrLowerBoundary.end.setY(potentialRect.y());
        break;
    case FocusDirectionDownRight:
        // Exit point is the down-right corner of startingRect.
        exitPoint.setX(startingRect.maxX());
        exitPoint.setY(startingRect.maxY());
        if (isStartingRectInsidePotentialRect) {
            // Entry point is the down-right corner of potentialRect.
            entryPoint.setX(potentialRect.maxX());
            entryPoint.setY(potentialRect.maxY());
            return;
        } else if (isPointInsideEffectiveAngleRange(direction, startingRect, LayoutPoint(potentialRect.x(), potentialRect.y()))) {
            // Entry point is the up-left corner of potentialRect.
            entryPoint.setX(potentialRect.x());
            entryPoint.setY(potentialRect.y());
            return;
        }
        // left line
        intersectionLineWithLeftOrUpperBoundary.start.setX(potentialRect.x());
        intersectionLineWithLeftOrUpperBoundary.start.setY(potentialRect.y());
        intersectionLineWithLeftOrUpperBoundary.end.setX(potentialRect.x());
        intersectionLineWithLeftOrUpperBoundary.end.setY(potentialRect.maxY());
        // top line
        intersectionLineWithRightOrLowerBoundary.start.setX(potentialRect.x());
        intersectionLineWithRightOrLowerBoundary.start.setY(potentialRect.y());
        intersectionLineWithRightOrLowerBoundary.end.setX(potentialRect.maxX());
        intersectionLineWithRightOrLowerBoundary.end.setY(potentialRect.y());
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    LayoutLine leftOrUpperBoundaryLine;
    LayoutLine rightOrLowerBoundaryLine;

    getEffectiveAngleLine(direction, startingRect, leftOrUpperBoundaryLine, rightOrLowerBoundaryLine);

    LayoutPoint candidateEntryPoint1;
    LayoutPoint candidateEntryPoint2;

    bool existsCandidateEntryPoint1 = intersectionOfLines(leftOrUpperBoundaryLine, intersectionLineWithLeftOrUpperBoundary, candidateEntryPoint1);
    bool existsCandidateEntryPoint2 = intersectionOfLines(rightOrLowerBoundaryLine, intersectionLineWithRightOrLowerBoundary, candidateEntryPoint2);

    if (existsCandidateEntryPoint1 && !existsCandidateEntryPoint2) {
        entryPoint.setX(candidateEntryPoint1.x());
        entryPoint.setY(candidateEntryPoint1.y());
    } else if (!existsCandidateEntryPoint1 && existsCandidateEntryPoint2) {
        entryPoint.setX(candidateEntryPoint2.x());
        entryPoint.setY(candidateEntryPoint2.y());
    } else if (existsCandidateEntryPoint1 && existsCandidateEntryPoint2) {
        if (euclidianDistance(exitPoint, candidateEntryPoint1) < euclidianDistance(exitPoint, candidateEntryPoint2)) {
            entryPoint.setX(candidateEntryPoint1.x());
            entryPoint.setY(candidateEntryPoint1.y());
        } else {
            entryPoint.setX(candidateEntryPoint2.x());
            entryPoint.setY(candidateEntryPoint2.y());
        }
    } else
        ASSERT_NOT_REACHED();
}
#endif

// This method calculates the exitPoint from the startingRect and the entryPoint into the candidate rect.
// The line between those 2 points is the closest distance between the 2 rects.
void entryAndExitPointsForDirection(FocusDirection direction, const LayoutRect& startingRect, const LayoutRect& potentialRect, LayoutPoint& exitPoint, LayoutPoint& entryPoint)
{
#if PLATFORM(WKC)
    if (isDiagonalDirection(direction)) {
        entryAndExitPointsForDiagonalDirection(direction, startingRect, potentialRect, exitPoint, entryPoint);
        return;
    }

    bool isStartingRectInsidePotentialRect = potentialRect.contains(startingRect);
    bool isPotentialRectInsideStartingRect = startingRect.contains(potentialRect);
    bool intersected = startingRect.intersects(potentialRect);

    switch (direction) {
    case FocusDirectionLeft:
        if (isStartingRectInsidePotentialRect) {
            exitPoint.setX(startingRect.x());
            entryPoint.setX(potentialRect.x());
        } else if (isPotentialRectInsideStartingRect || intersected) {
            exitPoint.setX(startingRect.maxX());
            entryPoint.setX(potentialRect.maxX());
        } else {
            exitPoint.setX(startingRect.x());
            entryPoint.setX(potentialRect.maxX());
        }
        break;
    case FocusDirectionUp:
        if (isStartingRectInsidePotentialRect) {
            exitPoint.setY(startingRect.y());
            entryPoint.setY(potentialRect.y());
        } else if (isPotentialRectInsideStartingRect || intersected) {
            exitPoint.setY(startingRect.maxY());
            entryPoint.setY(potentialRect.maxY());
        } else {
            exitPoint.setY(startingRect.y());
            entryPoint.setY(potentialRect.maxY());
        }
        break;
    case FocusDirectionRight:
        if (isStartingRectInsidePotentialRect) {
            exitPoint.setX(startingRect.maxX());
            entryPoint.setX(potentialRect.maxX());
        } else if (isPotentialRectInsideStartingRect || intersected) {
            exitPoint.setX(startingRect.x());
            entryPoint.setX(potentialRect.x());
        } else {
            exitPoint.setX(startingRect.maxX());
            entryPoint.setX(potentialRect.x());
        }
        break;
    case FocusDirectionDown:
        if (isStartingRectInsidePotentialRect) {
            exitPoint.setY(startingRect.maxY());
            entryPoint.setY(potentialRect.maxY());
        } else if (isPotentialRectInsideStartingRect || intersected) {
            exitPoint.setY(startingRect.y());
            entryPoint.setY(potentialRect.y());
        } else {
            exitPoint.setY(startingRect.maxY());
            entryPoint.setY(potentialRect.y());
        }
        break;
    default:
        ASSERT_NOT_REACHED();
    }
#else
    switch (direction) {
    case FocusDirectionLeft:
        exitPoint.setX(startingRect.x());
        entryPoint.setX(potentialRect.maxX());
        break;
    case FocusDirectionUp:
        exitPoint.setY(startingRect.y());
        entryPoint.setY(potentialRect.maxY());
        break;
    case FocusDirectionRight:
        exitPoint.setX(startingRect.maxX());
        entryPoint.setX(potentialRect.x());
        break;
    case FocusDirectionDown:
        exitPoint.setY(startingRect.maxY());
        entryPoint.setY(potentialRect.y());
        break;
    default:
        ASSERT_NOT_REACHED();
    }
#endif

    switch (direction) {
    case FocusDirectionLeft:
    case FocusDirectionRight:
        if (below(startingRect, potentialRect)) {
            exitPoint.setY(startingRect.y());
            entryPoint.setY(potentialRect.maxY());
        } else if (below(potentialRect, startingRect)) {
            exitPoint.setY(startingRect.maxY());
            entryPoint.setY(potentialRect.y());
        } else {
#if PLATFORM(WKC)
            exitPoint.setY((std::max(startingRect.y(), potentialRect.y()) + std::min(startingRect.maxY(), potentialRect.maxY())) / 2);
#else
            exitPoint.setY(std::max(startingRect.y(), potentialRect.y()));
#endif
            entryPoint.setY(exitPoint.y());
        }
        break;
    case FocusDirectionUp:
    case FocusDirectionDown:
        if (rightOf(startingRect, potentialRect)) {
            exitPoint.setX(startingRect.x());
            entryPoint.setX(potentialRect.maxX());
        } else if (rightOf(potentialRect, startingRect)) {
            exitPoint.setX(startingRect.maxX());
            entryPoint.setX(potentialRect.x());
        } else {
#if PLATFORM(WKC)
            exitPoint.setX((std::max(startingRect.x(), potentialRect.x()) + std::min(startingRect.maxX(), potentialRect.maxX())) / 2);
#else
            exitPoint.setX(std::max(startingRect.x(), potentialRect.x()));
#endif
            entryPoint.setX(exitPoint.x());
        }
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

bool areElementsOnSameLine(const FocusCandidate& firstCandidate, const FocusCandidate& secondCandidate)
{
    if (firstCandidate.isNull() || secondCandidate.isNull())
        return false;

    if (!firstCandidate.visibleNode->renderer() || !secondCandidate.visibleNode->renderer())
        return false;

    if (!firstCandidate.rect.intersects(secondCandidate.rect))
        return false;

    if (is<HTMLAreaElement>(*firstCandidate.focusableNode) || is<HTMLAreaElement>(*secondCandidate.focusableNode))
        return false;

    if (!firstCandidate.visibleNode->renderer()->isRenderInline() || !secondCandidate.visibleNode->renderer()->isRenderInline())
        return false;

    if (firstCandidate.visibleNode->renderer()->containingBlock() != secondCandidate.visibleNode->renderer()->containingBlock())
        return false;

    return true;
}

// Consider only those nodes as candidate which are exactly in the focus-direction.
// e.g. If we are moving down then the nodes that are above current focused node should be considered as invalid.
bool isValidCandidate(FocusDirection direction, const FocusCandidate& current, FocusCandidate& candidate)
{
    LayoutRect currentRect = current.rect;
    LayoutRect candidateRect = candidate.rect;

    switch (direction) {
    case FocusDirectionLeft:
        return candidateRect.x() < currentRect.maxX();
    case FocusDirectionUp:
        return candidateRect.y() < currentRect.maxY();
    case FocusDirectionRight:
        return candidateRect.maxX() > currentRect.x();
    case FocusDirectionDown:
        return candidateRect.maxY() > currentRect.y();
    default:
        ASSERT_NOT_REACHED();
    }
    return false;
}

#if PLATFORM(WKC)
static float distanceDataBetweenPoints(FocusDirection direction, const LayoutPoint& exitPoint, const LayoutPoint& entryPoint)
{
    LayoutUnit sameAxisDistance = 0;
    LayoutUnit otherAxisDistance = 0;

    switch (direction) {
    case FocusDirectionLeft:
        sameAxisDistance = exitPoint.x() - entryPoint.x();
        otherAxisDistance = absoluteValue(exitPoint.y() - entryPoint.y());
        break;
    case FocusDirectionUp:
        sameAxisDistance = exitPoint.y() - entryPoint.y();
        otherAxisDistance = absoluteValue(exitPoint.x() - entryPoint.x());
        break;
    case FocusDirectionRight:
        sameAxisDistance = entryPoint.x() - exitPoint.x();
        otherAxisDistance = absoluteValue(entryPoint.y() - exitPoint.y());
        break;
    case FocusDirectionDown:
        sameAxisDistance = entryPoint.y() - exitPoint.y();
        otherAxisDistance = absoluteValue(entryPoint.x() - exitPoint.x());
        break;
    case FocusDirectionUpLeft:
    case FocusDirectionUpRight:
    case FocusDirectionDownLeft:
    case FocusDirectionDownRight:
        // Just use the euclidian distance.
        break;
    default:
        ASSERT_NOT_REACHED();
        return std::numeric_limits<float>::max();
    }

    float x = (entryPoint.x() - exitPoint.x()) * (entryPoint.x() - exitPoint.x());
    float y = (entryPoint.y() - exitPoint.y()) * (entryPoint.y() - exitPoint.y());

    float euclidianDistance = sqrt(x + y);

    // Loosely based on http://www.w3.org/TR/WICD/#focus-handling
    // df = dotDist + dx + dy + 2 * (xdisplacement + ydisplacement) - sqrt(Overlap)

    return euclidianDistance + sameAxisDistance + 2 * otherAxisDistance;
}

void distanceDataForNode(FocusDirection direction, const FocusCandidate& current, FocusCandidate& candidate)
{
    LayoutRect candidateRect = candidate.rect;
    LayoutRect currentRect = current.rect;
    LayoutRect currentTargetRect = current.entryRect;
    LayoutRect lastExitRect = current.exitRect;
    FocusDirection lastDirection = current.direction;

    candidate.alignment = None;
    candidate.distance = maxDistance();

    if (!candidate.visibleNode->renderer())
        return;

    if (!isRectInsideEffectiveAngleRange(direction, currentRect, candidateRect))
        return;

    RectsAlignment alignment;
    long long distance;
    LayoutPoint exitPoint;
    LayoutPoint entryPoint;

    LayoutSize viewSize = candidate.visibleNode->document().page()->mainFrame().view()->visibleContentRect().size();
    LayoutRect rect = candidateRect;

    Vector<FloatQuad> quads;
    candidate.visibleNode->renderer()->absoluteQuads(quads);
    size_t n = quads.size();

    for (size_t i = 0; i < n; ++i) {
        if (n != 1)
            rect = rectToAbsoluteCoordinates(candidate.visibleNode->document().frame(), quads[i].enclosingBoundingBox());
        if (rect.isEmpty())
            continue;
        if (rect.contains(currentTargetRect)) {
            candidate.alignment = None;
            candidate.distance = maxDistance();
            return;
        }
        if (!isRectInsideEffectiveAngleRange(direction, currentTargetRect, rect))
            continue;
        alignment = alignmentForRects(direction, currentTargetRect, rect, viewSize);
        if (alignment < candidate.alignment)
            continue;
        if (isDiagonalDirection(direction)) {
            // Calculate the shortest distance between the current rect and the candidate rect.
            entryAndExitPointsForDirection(direction, currentTargetRect, rect, exitPoint, entryPoint);
        } else {
            // Calculate the distance between the entry point of the current rect and the entry point of the candidate rect, and add it to the distance calculated above.
            LayoutRect entryPointRect(current.entryPoint.x(), current.entryPoint.y(), 1, 1);
            entryAndExitPointsForDirection(direction, entryPointRect, rect, exitPoint, entryPoint);
        }
        distance = llroundf(distanceDataBetweenPoints(direction, exitPoint, entryPoint));
        if (alignment > candidate.alignment || distance < candidate.distance) {
            candidate.alignment = alignment;
            candidate.distance = distance;
            candidate.exitRect = currentTargetRect;
            candidate.entryRect = rect;
            candidate.entryPoint = entryPoint;
        }
    }
}
#else
void distanceDataForNode(FocusDirection direction, const FocusCandidate& current, FocusCandidate& candidate)
{
    if (areElementsOnSameLine(current, candidate)) {
        if ((direction == FocusDirectionUp && current.rect.y() > candidate.rect.y()) || (direction == FocusDirectionDown && candidate.rect.y() > current.rect.y())) {
            candidate.distance = 0;
            candidate.alignment = Full;
            return;
        }
    }

    LayoutRect nodeRect = candidate.rect;
    LayoutRect currentRect = current.rect;
    deflateIfOverlapped(currentRect, nodeRect);

    if (!isRectInDirection(direction, currentRect, nodeRect))
        return;

    LayoutPoint exitPoint;
    LayoutPoint entryPoint;
    LayoutUnit sameAxisDistance = 0;
    LayoutUnit otherAxisDistance = 0;
    entryAndExitPointsForDirection(direction, currentRect, nodeRect, exitPoint, entryPoint);

    switch (direction) {
    case FocusDirectionLeft:
        sameAxisDistance = exitPoint.x() - entryPoint.x();
        otherAxisDistance = absoluteValue(exitPoint.y() - entryPoint.y());
        break;
    case FocusDirectionUp:
        sameAxisDistance = exitPoint.y() - entryPoint.y();
        otherAxisDistance = absoluteValue(exitPoint.x() - entryPoint.x());
        break;
    case FocusDirectionRight:
        sameAxisDistance = entryPoint.x() - exitPoint.x();
        otherAxisDistance = absoluteValue(entryPoint.y() - exitPoint.y());
        break;
    case FocusDirectionDown:
        sameAxisDistance = entryPoint.y() - exitPoint.y();
        otherAxisDistance = absoluteValue(entryPoint.x() - exitPoint.x());
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    float x = (entryPoint.x() - exitPoint.x()) * (entryPoint.x() - exitPoint.x());
    float y = (entryPoint.y() - exitPoint.y()) * (entryPoint.y() - exitPoint.y());

    float euclidianDistance = sqrt(x + y);

    // Loosely based on http://www.w3.org/TR/WICD/#focus-handling
    // df = dotDist + dx + dy + 2 * (xdisplacement + ydisplacement) - sqrt(Overlap)

    float distance = euclidianDistance + sameAxisDistance + 2 * otherAxisDistance;
    candidate.distance = roundf(distance);
    LayoutSize viewSize = candidate.visibleNode->document().page()->mainFrame().view()->visibleContentRect().size();
    candidate.alignment = alignmentForRects(direction, currentRect, nodeRect, viewSize);
}
#endif

bool canBeScrolledIntoView(FocusDirection direction, const FocusCandidate& candidate)
{
    ASSERT(candidate.visibleNode && candidate.isOffscreen);
    LayoutRect candidateRect = candidate.rect;
    for (Node* parentNode = candidate.visibleNode->parentNode(); parentNode; parentNode = parentNode->parentNode()) {
        LayoutRect parentRect = nodeRectInAbsoluteCoordinates(parentNode);
        if (!candidateRect.intersects(parentRect)) {
#if PLATFORM(WKC)
            if (isDiagonalDirection(direction) &&
                (parentNode->renderer()->style().overflowX() == OHIDDEN || parentNode->renderer()->style().overflowY() == OHIDDEN))
                return false;
#endif
            if (((direction == FocusDirectionLeft || direction == FocusDirectionRight) && parentNode->renderer()->style().overflowX() == OHIDDEN)
                || ((direction == FocusDirectionUp || direction == FocusDirectionDown) && parentNode->renderer()->style().overflowY() == OHIDDEN))
                return false;
        }
        if (parentNode == candidate.enclosingScrollableBox)
            return canScrollInDirection(parentNode, direction);
    }
    return true;
}

// The starting rect is the rect of the focused node, in document coordinates.
// Compose a virtual starting rect if there is no focused node or if it is off screen.
// The virtual rect is the edge of the container or frame. We select which
// edge depending on the direction of the navigation.
LayoutRect virtualRectForDirection(FocusDirection direction, const LayoutRect& startingRect, LayoutUnit width)
{
    LayoutRect virtualStartingRect = startingRect;
    switch (direction) {
    case FocusDirectionLeft:
        virtualStartingRect.setX(virtualStartingRect.maxX() - width);
        virtualStartingRect.setWidth(width);
        break;
    case FocusDirectionUp:
        virtualStartingRect.setY(virtualStartingRect.maxY() - width);
        virtualStartingRect.setHeight(width);
        break;
    case FocusDirectionRight:
        virtualStartingRect.setWidth(width);
        break;
    case FocusDirectionDown:
        virtualStartingRect.setHeight(width);
        break;
#if PLATFORM(WKC)
    case FocusDirectionUpLeft:
        virtualStartingRect.setX(virtualStartingRect.maxX() - width);
        virtualStartingRect.setWidth(width);
        virtualStartingRect.setY(virtualStartingRect.maxY() - width);
        virtualStartingRect.setHeight(width);
        break;
    case FocusDirectionUpRight:
        virtualStartingRect.setWidth(width);
        virtualStartingRect.setY(virtualStartingRect.maxY() - width);
        virtualStartingRect.setHeight(width);
        break;
    case FocusDirectionDownLeft:
        virtualStartingRect.setX(virtualStartingRect.maxX() - width);
        virtualStartingRect.setWidth(width);
        virtualStartingRect.setHeight(width);
        break;
    case FocusDirectionDownRight:
        virtualStartingRect.setWidth(width);
        virtualStartingRect.setHeight(width);
        break;
#endif
    default:
        ASSERT_NOT_REACHED();
    }

    return virtualStartingRect;
}

LayoutRect virtualRectForAreaElementAndDirection(HTMLAreaElement* area, FocusDirection direction)
{
    ASSERT(area);
    ASSERT(area->imageElement());
    // Area elements tend to overlap more than other focusable elements. We flatten the rect of the area elements
    // to minimize the effect of overlapping areas.
    LayoutRect rect = virtualRectForDirection(direction, rectToAbsoluteCoordinates(area->document().frame(), area->computeRect(area->imageElement()->renderer())), 1);
    return rect;
}

HTMLFrameOwnerElement* frameOwnerElement(FocusCandidate& candidate)
{
    return candidate.isFrameOwnerElement() ? downcast<HTMLFrameOwnerElement>(candidate.visibleNode) : nullptr;
}

} // namespace WebCore

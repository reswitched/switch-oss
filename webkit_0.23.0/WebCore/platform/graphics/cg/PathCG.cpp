/*
 * Copyright (C) 2003, 2006 Apple Inc.  All rights reserved.
 *                     2006, 2008 Rob Buis <buis@kde.org>
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
#include "Path.h"

#if USE(CG)

#include "AffineTransform.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "StrokeStyleApplier.h"
#include <CoreGraphics/CoreGraphics.h>
#include <wtf/MathExtras.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(COCOA)
#include "WebCoreSystemInterface.h"
#endif

#if PLATFORM(WIN)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

namespace WebCore {

static size_t putBytesNowhere(void*, const void*, size_t count)
{
    return count;
}

static CGContextRef createScratchContext()
{
    CGDataConsumerCallbacks callbacks = { putBytesNowhere, 0 };
    RetainPtr<CGDataConsumerRef> consumer = adoptCF(CGDataConsumerCreate(0, &callbacks));
    CGContextRef context = CGPDFContextCreate(consumer.get(), 0, 0);

    CGFloat black[4] = { 0, 0, 0, 1 };
    CGContextSetFillColor(context, black);
    CGContextSetStrokeColor(context, black);

    return context;
}

static inline CGContextRef scratchContext()
{
    static CGContextRef context = createScratchContext();
    return context;
}

Path::Path()
    : m_path(nullptr)
{
}

Path::Path(RetainPtr<CGMutablePathRef> p)
    : m_path(p.leakRef())
{
}

Path::~Path()
{
    if (m_path)
        CGPathRelease(m_path);
}

PlatformPathPtr Path::ensurePlatformPath()
{
    if (!m_path)
        m_path = CGPathCreateMutable();
    return m_path;
}

Path::Path(const Path& other)
{
    m_path = other.m_path ? CGPathCreateMutableCopy(other.m_path) : 0;
}

Path& Path::operator=(const Path& other)
{
    CGMutablePathRef path = other.m_path ? CGPathCreateMutableCopy(other.m_path) : 0;
    if (m_path)
        CGPathRelease(m_path);
    m_path = path;
    return *this;
}

static void copyClosingSubpathsApplierFunction(void* info, const CGPathElement* element)
{
    CGMutablePathRef path = static_cast<CGMutablePathRef>(info);
    CGPoint* points = element->points;
    
    switch (element->type) {
    case kCGPathElementMoveToPoint:
        if (!CGPathIsEmpty(path)) // to silence a warning when trying to close an empty path
            CGPathCloseSubpath(path); // This is the only change from CGPathCreateMutableCopy
        CGPathMoveToPoint(path, 0, points[0].x, points[0].y);
        break;
    case kCGPathElementAddLineToPoint:
        CGPathAddLineToPoint(path, 0, points[0].x, points[0].y);
        break;
    case kCGPathElementAddQuadCurveToPoint:
        CGPathAddQuadCurveToPoint(path, 0, points[0].x, points[0].y, points[1].x, points[1].y);
        break;
    case kCGPathElementAddCurveToPoint:
        CGPathAddCurveToPoint(path, 0, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
        break;
    case kCGPathElementCloseSubpath:
        CGPathCloseSubpath(path);
        break;
    }
}

static CGMutablePathRef copyCGPathClosingSubpaths(CGPathRef originalPath)
{
    CGMutablePathRef path = CGPathCreateMutable();
    CGPathApply(originalPath, path, copyClosingSubpathsApplierFunction);
    CGPathCloseSubpath(path);
    return path;
}

bool Path::contains(const FloatPoint &point, WindRule rule) const
{
    if (isNull())
        return false;

    if (!fastBoundingRect().contains(point))
        return false;

    // CGPathContainsPoint returns false for non-closed paths, as a work-around, we copy and close the path first.  Radar 4758998 asks for a better CG API to use
    RetainPtr<CGMutablePathRef> path = adoptCF(copyCGPathClosingSubpaths(m_path));
    bool ret = CGPathContainsPoint(path.get(), 0, point, rule == RULE_EVENODD ? true : false);
    return ret;
}

bool Path::strokeContains(StrokeStyleApplier* applier, const FloatPoint& point) const
{
    if (isNull())
        return false;

    ASSERT(applier);

    CGContextRef context = scratchContext();

    CGContextSaveGState(context);
    CGContextBeginPath(context);
    CGContextAddPath(context, platformPath());

    GraphicsContext gc(context);
    applier->strokeStyle(&gc);

    bool hitSuccess = CGContextPathContainsPoint(context, point, kCGPathStroke);
    CGContextRestoreGState(context);
    
    return hitSuccess;
}

void Path::translate(const FloatSize& size)
{
    CGAffineTransform translation = CGAffineTransformMake(1, 0, 0, 1, size.width(), size.height());
    CGMutablePathRef newPath = CGPathCreateMutable();
    // FIXME: This is potentially wasteful to allocate an empty path only to create a transformed copy.
    CGPathAddPath(newPath, &translation, ensurePlatformPath());
    CGPathRelease(m_path);
    m_path = newPath;
}

FloatRect Path::boundingRect() const
{
    if (isNull())
        return CGRectZero;

    // CGPathGetBoundingBox includes the path's control points, CGPathGetPathBoundingBox
    // does not, but only exists on 10.6 and above.

    CGRect bound = CGPathGetPathBoundingBox(m_path);
    return CGRectIsNull(bound) ? CGRectZero : bound;
}

FloatRect Path::fastBoundingRect() const
{
    if (isNull())
        return CGRectZero;
    CGRect bound = CGPathGetBoundingBox(m_path);
    return CGRectIsNull(bound) ? CGRectZero : bound;
}

FloatRect Path::strokeBoundingRect(StrokeStyleApplier* applier) const
{
    if (isNull())
        return CGRectZero;

    CGContextRef context = scratchContext();

    CGContextSaveGState(context);
    CGContextBeginPath(context);
    CGContextAddPath(context, platformPath());

    if (applier) {
        GraphicsContext graphicsContext(context);
        applier->strokeStyle(&graphicsContext);
    }

    CGContextReplacePathWithStrokedPath(context);
    CGRect box = CGContextIsPathEmpty(context) ? CGRectZero : CGContextGetPathBoundingBox(context);
    CGContextRestoreGState(context);

    return CGRectIsNull(box) ? CGRectZero : box;
}

void Path::moveTo(const FloatPoint& point)
{
    CGPathMoveToPoint(ensurePlatformPath(), 0, point.x(), point.y());
}

void Path::addLineTo(const FloatPoint& p)
{
    CGPathAddLineToPoint(ensurePlatformPath(), 0, p.x(), p.y());
}

void Path::addQuadCurveTo(const FloatPoint& cp, const FloatPoint& p)
{
    CGPathAddQuadCurveToPoint(ensurePlatformPath(), 0, cp.x(), cp.y(), p.x(), p.y());
}

void Path::addBezierCurveTo(const FloatPoint& cp1, const FloatPoint& cp2, const FloatPoint& p)
{
    CGPathAddCurveToPoint(ensurePlatformPath(), 0, cp1.x(), cp1.y(), cp2.x(), cp2.y(), p.x(), p.y());
}

void Path::addArcTo(const FloatPoint& p1, const FloatPoint& p2, float radius)
{
    CGPathAddArcToPoint(ensurePlatformPath(), 0, p1.x(), p1.y(), p2.x(), p2.y(), radius);
}

void Path::platformAddPathForRoundedRect(const FloatRect& rect, const FloatSize& topLeftRadius, const FloatSize& topRightRadius, const FloatSize& bottomLeftRadius, const FloatSize& bottomRightRadius)
{
#if PLATFORM(COCOA)
    bool equalWidths = (topLeftRadius.width() == topRightRadius.width() && topRightRadius.width() == bottomLeftRadius.width() && bottomLeftRadius.width() == bottomRightRadius.width());
    bool equalHeights = (topLeftRadius.height() == bottomLeftRadius.height() && bottomLeftRadius.height() == topRightRadius.height() && topRightRadius.height() == bottomRightRadius.height());

    if (equalWidths && equalHeights) {
        // Ensure that CG can render the rounded rect.
        CGFloat radiusWidth = topLeftRadius.width();
        CGFloat radiusHeight = topLeftRadius.height();
        CGRect rectToDraw = rect;
        CGFloat rectWidth = CGRectGetWidth(rectToDraw);
        CGFloat rectHeight = CGRectGetHeight(rectToDraw);
        if (rectWidth < 2 * radiusWidth)
            radiusWidth = rectWidth / 2 - std::numeric_limits<CGFloat>::epsilon();
        if (rectHeight < 2 * radiusHeight)
            radiusHeight = rectHeight / 2 - std::numeric_limits<CGFloat>::epsilon();
        CGPathAddRoundedRect(ensurePlatformPath(), nullptr, rectToDraw, radiusWidth, radiusHeight);
        return;
    }
#endif

    addBeziersForRoundedRect(rect, topLeftRadius, topRightRadius, bottomLeftRadius, bottomRightRadius);
}

void Path::closeSubpath()
{
    // FIXME: Unclear if close commands should have meaning for a null path.
    if (isNull())
        return;

    CGPathCloseSubpath(m_path);
}

void Path::addArc(const FloatPoint& p, float radius, float startAngle, float endAngle, bool clockwise)
{
    // Workaround for <rdar://problem/5189233> CGPathAddArc hangs or crashes when passed inf as start or end angle
    if (std::isfinite(startAngle) && std::isfinite(endAngle))
        CGPathAddArc(ensurePlatformPath(), nullptr, p.x(), p.y(), radius, startAngle, endAngle, clockwise);
}

void Path::addRect(const FloatRect& r)
{
    CGPathAddRect(ensurePlatformPath(), 0, r);
}

void Path::addEllipse(FloatPoint p, float radiusX, float radiusY, float rotation, float startAngle, float endAngle, bool anticlockwise)
{
    AffineTransform transform;
    transform.translate(p.x(), p.y()).rotate(rad2deg(rotation)).scale(radiusX, radiusY);

    CGAffineTransform cgTransform = transform;
    CGPathAddArc(ensurePlatformPath(), &cgTransform, 0, 0, 1, startAngle, endAngle, anticlockwise);
}

void Path::addEllipse(const FloatRect& r)
{
    CGPathAddEllipseInRect(ensurePlatformPath(), 0, r);
}

void Path::addPath(const Path& path, const AffineTransform& transform)
{
    if (!path.platformPath())
        return;

    if (!transform.isInvertible())
        return;

    CGAffineTransform transformCG = transform;
    // CG doesn't allow adding a path to itself. Optimize for the common case
    // and copy the path for the self referencing case.
    if (ensurePlatformPath() != path.platformPath()) {
        CGPathAddPath(ensurePlatformPath(), &transformCG, path.platformPath());
        return;
    }
    CGPathRef pathCopy = CGPathCreateCopy(path.platformPath());
    CGPathAddPath(ensurePlatformPath(), &transformCG, pathCopy);
    CGPathRelease(pathCopy);
}


void Path::clear()
{
    if (isNull())
        return;

    CGPathRelease(m_path);
    m_path = CGPathCreateMutable();
}

bool Path::isEmpty() const
{
    return isNull() || CGPathIsEmpty(m_path);
}

bool Path::hasCurrentPoint() const
{
    return !isEmpty();
}
    
FloatPoint Path::currentPoint() const 
{
    if (isNull())
        return FloatPoint();
    return CGPathGetCurrentPoint(m_path);
}

struct PathApplierInfo {
    void* info;
    PathApplierFunction function;
};

static void CGPathApplierToPathApplier(void *info, const CGPathElement *element)
{
    PathApplierInfo* pinfo = (PathApplierInfo*)info;
    FloatPoint points[3];
    PathElement pelement;
    pelement.type = (PathElementType)element->type;
    pelement.points = points;
    CGPoint* cgPoints = element->points;
    switch (element->type) {
    case kCGPathElementMoveToPoint:
    case kCGPathElementAddLineToPoint:
        points[0] = cgPoints[0];
        break;
    case kCGPathElementAddQuadCurveToPoint:
        points[0] = cgPoints[0];
        points[1] = cgPoints[1];
        break;
    case kCGPathElementAddCurveToPoint:
        points[0] = cgPoints[0];
        points[1] = cgPoints[1];
        points[2] = cgPoints[2];
        break;
    case kCGPathElementCloseSubpath:
        break;
    }
    pinfo->function(pinfo->info, &pelement);
}

void Path::apply(void* info, PathApplierFunction function) const
{
    if (isNull())
        return;

    PathApplierInfo pinfo;
    pinfo.info = info;
    pinfo.function = function;
    CGPathApply(m_path, &pinfo, CGPathApplierToPathApplier);
}

void Path::transform(const AffineTransform& transform)
{
    if (transform.isIdentity() || isEmpty())
        return;

    CGMutablePathRef path = CGPathCreateMutable();
    CGAffineTransform transformCG = transform;
    CGPathAddPath(path, &transformCG, m_path);
    CGPathRelease(m_path);
    m_path = path;
}

}

#endif // USE(CG)

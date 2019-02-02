/*
 * Copyright (C) 2006 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008, 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Brent Fulgham <bfulgham@webkit.org>
 * Copyright (C) 2010, 2011 Igalia S.L.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2012, Intel Corporation
 * Copyright (c) 2011-2018 ACCESS CO., LTD. All rights reserved.
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
#include "GraphicsContext.h"

#if USE(WKC_CAIRO)

#include "AffineTransform.h"
#include "CairoUtilities.h"
#include "DrawErrorUnderline.h"
#include "FloatConversion.h"
#include "FloatRect.h"
#include "FloatRoundedRect.h"
#include "Font.h"
#include "IntRect.h"
#include "NotImplemented.h"
#include "Path.h"
#include "Pattern.h"
#include "PlatformContextCairo.h"
#include "PlatformPathCairo.h"
#include "RefPtrCairo.h"
#include "ShadowBlur.h"
#include <cairo.h>
#include <math.h>
#include <stdio.h>
#include <wtf/MathExtras.h>
#if USE(TEXTURE_MAPPER)
#include "TransformationMatrix.h"
#endif

#include "ImageWKC.h"
#include <wkc/wkcpeer.h>
#include <wkc/wkcgpeer.h>

#if COMPILER(MSVC) && _MSC_VER < 1700
namespace std {
inline double round(double d) { return ::round(d); }
}
#endif

using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define platformColor(a,r,g,b) ((((a)&0x000000ff) << 24) | (((r)&0x000000ff) << 16) | (((g)&0x000000ff) << 8) | ((b)&0x000000ff))

// For graphics peer
static cairo_t* getCairoFromPlatformContext(void* ctx)
{
    PlatformGraphicsContext* gc = static_cast<PlatformGraphicsContext*>(ctx);
    return gc->cr();
}

static void destroyPlatformContext(void* ctx)
{
    PlatformGraphicsContext* gc = static_cast<PlatformGraphicsContext*>(ctx);
    delete gc;
}

static void* createPlatformContext(cairo_t* cr)
{
    void* ctx = new PlatformGraphicsContext(cr);
    return ctx;
}

// followings are originally from WebCore/platform/graphics/cairo/* .

namespace WebCore {

class GraphicsContextPlatformPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    GraphicsContextPlatformPrivate(PlatformContextCairo* newPlatformContext)
        : platformContext(newPlatformContext)
    {
    }
    virtual ~GraphicsContextPlatformPrivate()
    {
    };

    void save()
    {
        if (platformContext)
            wkcDrawContextSaveStatePeer(platformContext);
    }
    void restore()
    {
        if (platformContext)
            wkcDrawContextRestoreStatePeer(platformContext);
    }
    void flush() {}
    void clip(const FloatRect&) {}
    void clip(const Path&) {}
    void scale(const FloatSize&) {}
    void rotate(float) {}
    void translate(float, float) {}
    void concatCTM(const AffineTransform&) {}
    void setCTM(const AffineTransform&) {}
    void beginTransparencyLayer() {}
    void endTransparencyLayer() {}
    void syncContext() {}
    bool isDrawingOnOffscreen()
    {
        if (!platformContext)
            return false;
        return wkcDrawContextIsForOffscreenPeer(platformContext);
    }

public:
    PlatformContextCairo* platformContext;
    Vector<float> layers;
    ShadowBlur shadow;
    Vector<ShadowBlur> shadowStack;
};

class GraphicsContextPlatformPrivateToplevel : public GraphicsContextPlatformPrivate {
public:
    GraphicsContextPlatformPrivateToplevel(PlatformContextCairo* platformContext)
        : GraphicsContextPlatformPrivate(platformContext)
    {
    }

    virtual ~GraphicsContextPlatformPrivateToplevel()
    {
        delete platformContext;
    }
};

class PatternPrivateData {
    WTF_MAKE_FAST_ALLOCATED;
public:
    PatternPrivateData()
        : m_repeatX(false)
        , m_repeatY(false)
        , m_width(0)
        , m_height(0)
        {}

    bool m_repeatX;
    bool m_repeatY;
    int m_width;
    int m_height;
};

static void _PatternPrivateDataDelete(void* self)
{
    delete (PatternPrivateData*)self;
}

static const cairo_user_data_key_t cPatternPrivateDataKey = {0};
static const bool cGraphicsPeerInitialize = wkcDrawContextSetCallbacksPeer((wkcCreateEngineContextProc)createPlatformContext, (wkcDestroyEngineContextProc)destroyPlatformContext ,(wkcGetPeerContextProc)getCairoFromPlatformContext);

// A helper which quickly fills a rectangle with a simple color fill.
static inline void fillRectWithColor(cairo_t* cr, const FloatRect& rect, const Color& color)
{
    if (!color.alpha() && cairo_get_operator(cr) == CAIRO_OPERATOR_OVER)
        return;
    setSourceRGBAFromColor(cr, color);
    cairo_rectangle(cr, rect.x(), rect.y(), rect.width(), rect.height());
    cairo_fill(cr);
}

static void addConvexPolygonToContext(cairo_t* context, size_t numPoints, const FloatPoint* points)
{
    cairo_move_to(context, points[0].x(), points[0].y());
    for (size_t i = 1; i < numPoints; i++)
        cairo_line_to(context, points[i].x(), points[i].y());
    cairo_close_path(context);
}

enum PathDrawingStyle { 
    Fill = 1,
    Stroke = 2,
    FillAndStroke = Fill + Stroke
};

static inline void drawPathShadow(GraphicsContext* context, PathDrawingStyle drawingStyle)
{
    ShadowBlur& shadow = context->platformContext()->shadowBlur();
    if (shadow.type() == ShadowBlur::NoShadow)
        return;

    // Calculate the extents of the rendered solid paths.
    cairo_t* cairoContext = context->platformContext()->cr();
    std::unique_ptr<cairo_path_t, void(*)(cairo_path_t*)> path(cairo_copy_path(cairoContext), [](cairo_path_t* path) {
        cairo_path_destroy(path);
    });

    FloatRect solidFigureExtents;
    double x0 = 0;
    double x1 = 0;
    double y0 = 0;
    double y1 = 0;
    if (drawingStyle & Stroke) {
        cairo_stroke_extents(cairoContext, &x0, &y0, &x1, &y1);
        solidFigureExtents = FloatRect(x0, y0, x1 - x0, y1 - y0);
    }
    if (drawingStyle & Fill) {
        cairo_fill_extents(cairoContext, &x0, &y0, &x1, &y1);
        FloatRect fillExtents(x0, y0, x1 - x0, y1 - y0);
        solidFigureExtents.unite(fillExtents);
    }

    GraphicsContext* shadowContext = shadow.beginShadowLayer(context, solidFigureExtents);
    if (!shadowContext)
        return;

    cairo_t* cairoShadowContext = shadowContext->platformContext()->cr();
    if (!cairoShadowContext)
        return;

    // It's important to copy the context properties to the new shadow
    // context to preserve things such as the fill rule and stroke width.
    copyContextProperties(cairoContext, cairoShadowContext);

    if (drawingStyle & Fill) {
        cairo_save(cairoShadowContext);
        cairo_append_path(cairoShadowContext, path.get());
        shadowContext->platformContext()->prepareForFilling(context->state(), PlatformContextCairo::NoAdjustment);
        cairo_fill(cairoShadowContext);
        cairo_restore(cairoShadowContext);
    }

    if (drawingStyle & Stroke) {
        cairo_append_path(cairoShadowContext, path.get());
        shadowContext->platformContext()->prepareForStroking(context->state(), PlatformContextCairo::DoNotPreserveAlpha);
        cairo_stroke(cairoShadowContext);
    }

    // The original path may still be hanging around on the context and endShadowLayer
    // will take care of properly creating a path to draw the result shadow. We remove the path
    // temporarily and then restore it.
    // See: https://bugs.webkit.org/show_bug.cgi?id=108897
    cairo_new_path(cairoContext);
    shadow.endShadowLayer(context);
    cairo_append_path(cairoContext, path.get());
}

static inline void shadowAndFillCurrentCairoPath(GraphicsContext* context)
{
    cairo_t* cr = context->platformContext()->cr();
    cairo_save(cr);

    drawPathShadow(context, Fill);

    context->platformContext()->prepareForFilling(context->state(), PlatformContextCairo::AdjustPatternForGlobalAlpha);
    cairo_fill(cr);

    cairo_restore(cr);
}

static inline void shadowAndStrokeCurrentCairoPath(GraphicsContext* context)
{
    drawPathShadow(context, Stroke);
    context->platformContext()->prepareForStroking(context->state());
    cairo_stroke(context->platformContext()->cr());
}

GraphicsContext::GraphicsContext(cairo_t* cr)
    : m_updatingControlTints(false),
      m_transparencyCount(0)
{
    m_data = new GraphicsContextPlatformPrivateToplevel(new PlatformContextCairo(cr));

    setPlatformFillColor(fillColor(), ColorSpaceDeviceRGB);
    setPlatformStrokeColor(strokeColor(), ColorSpaceDeviceRGB);
    setPlatformStrokeStyle(SolidStroke);
    setPlatformTextDrawingMode(TextModeFill);
    setAlpha(1.0);
}

void GraphicsContext::platformInit(PlatformContextCairo* platformContext)
{
    m_data = new GraphicsContextPlatformPrivate(platformContext);
    if (platformContext) {
        m_data->syncContext();
        // Make sure the context starts in sync with our state.
        setPlatformFillColor(fillColor(), ColorSpaceDeviceRGB);
        setPlatformStrokeColor(strokeColor(), ColorSpaceDeviceRGB);
        setPlatformStrokeStyle(SolidStroke);
        setPlatformTextDrawingMode(TextModeFill);
        setAlpha(1.0);
    } else
        setPaintingDisabled(true);
}

void GraphicsContext::platformDestroy()
{
    delete m_data;
}

AffineTransform GraphicsContext::getCTM(IncludeDeviceScale) const
{
    if (paintingDisabled())
        return AffineTransform();

    cairo_t* cr = platformContext()->cr();
    cairo_matrix_t m;
    cairo_get_matrix(cr, &m);
    return AffineTransform(m.xx, m.yx, m.xy, m.yy, m.x0, m.y0);
}

PlatformContextCairo* GraphicsContext::platformContext() const
{
    return m_data->platformContext;
}

bool GraphicsContext::isAcceleratedContext() const
{
    return false;
}

void GraphicsContext::savePlatformState()
{
    platformContext()->save();
    m_data->save();
}

void GraphicsContext::restorePlatformState()
{
    platformContext()->restore();
    m_data->restore();

    platformContext()->shadowBlur().setShadowValues(FloatSize(m_state.shadowBlur, m_state.shadowBlur),
                                                    m_state.shadowOffset,
                                                    m_state.shadowColor,
                                                    m_state.shadowColorSpace,
                                                    m_state.shadowsIgnoreTransforms);
}

// Draws a filled rectangle with a stroked border.
void GraphicsContext::drawRect(const FloatRect& rect, float /*borderThickness*/)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    cairo_save(cr);

    FloatRect newRect;
    if (m_data->isDrawingOnOffscreen())
        newRect = roundToDevicePixels(rect);
    else
        newRect = rect;

    fillRectWithColor(cr, newRect, fillColor());

    if (strokeStyle() != NoStroke) {
        setSourceRGBAFromColor(cr, strokeColor());
        FloatRect r(newRect);
        r.inflate(-.5f);
        cairo_rectangle(cr, r.x(), r.y(), r.width(), r.height());
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    }

    cairo_restore(cr);
}

static double calculateStrokePatternOffset(float distance, float patternWidth)
{
    // Example: 80 pixels with a width of 30 pixels. Remainder is 20.
    // The maximum pixels of line we could paint will be 50 pixels.
    float remainder = fmod(distance, patternWidth);
    float numSegments = (distance - remainder) / patternWidth;

    // Special case 1px dotted borders for speed.
//    if (patternWidth == 1)
//        return 1;

    bool evenNumberOfSegments = !(fmod(numSegments,2));
    if (remainder)
        evenNumberOfSegments = !evenNumberOfSegments;

    if (evenNumberOfSegments) {
        if (remainder)
            return (patternWidth - remainder) + (remainder / 2);
        return patternWidth / 2;
    }

    // Odd number of segments.
    if (remainder)
        return (patternWidth - remainder) / 2.;
    return 0;
}

static void adjustToCenterPointX(FloatPoint& point1, FloatPoint& point2)
{
    ASSERT(point1.x() > point2.x());
    float diff = (point1.x() - point2.x()) / 2;
    point1.setX(point1.x() - diff);
    point2.setX(point2.x() + diff);
}

static void adjustToCenterPointY(FloatPoint& point1, FloatPoint& point2)
{
    ASSERT(point1.y() > point2.y());
    float diff = (point1.y() - point2.y()) / 2;
    point1.setY(point1.y() - diff);
    point2.setY(point2.y() + diff);
}

static void adjustPointsForBoxSide(FloatPoint& point1, FloatPoint& point2, bool isVerticalLine)
{
    if (isVerticalLine) {
        if (point1.x() > point2.x()) {
            adjustToCenterPointX(point1, point2);
        } else {
            adjustToCenterPointX(point2, point1);
        }
    } else {
        if (point1.y() > point2.y()) {
            adjustToCenterPointY(point1, point2);
        } else {
            adjustToCenterPointY(point2, point1);
        }
    }
}

static void drawLineOnCairoContext(GraphicsContext* graphicsContext, cairo_t* context, const FloatPoint& point1, const FloatPoint& point2)
{
    StrokeStyle style = graphicsContext->strokeStyle();
    if (style == NoStroke)
        return;

    const Color& strokeColor = graphicsContext->strokeColor();
    float strokeThickness = graphicsContext->strokeThickness();
    if (strokeThickness < 1)
        strokeThickness = 1;

    float patternWidth = 0;
    if (style == DottedStroke)
        patternWidth = strokeThickness;
    else if (style == DashedStroke)
        patternWidth = 3 * strokeThickness;

    bool isVerticalLine = (point1.x() + strokeThickness == point2.x()) && (point1.y() != point2.y());
    FloatPoint point1OnPixelBoundaries = point1;
    FloatPoint point2OnPixelBoundaries = point2;
    if (strokeThickness==1) {
        if (isVerticalLine) {
            if (point1.x()!=point2.x())
                point2OnPixelBoundaries.setX(point1.x());
        } else {
            if (point1.y()!=point2.y())
                point2OnPixelBoundaries.setY(point1.y());
        }
    } else {
        // Adjust the line from point1 and point2 to become pararell with box's frame
        adjustPointsForBoxSide(point1OnPixelBoundaries, point2OnPixelBoundaries, isVerticalLine);
    }
    GraphicsContext::adjustLineToPixelBoundaries(point1OnPixelBoundaries, point2OnPixelBoundaries, strokeThickness, style);

    cairo_antialias_t prev = cairo_get_antialias(context);
    cairo_set_antialias(context, CAIRO_ANTIALIAS_NONE);
    if (patternWidth>0) {
        // Do a rect fill of our endpoints.  This ensures we always have the
        // appearance of being a border.  We then draw the actual dotted/dashed line.
        FloatRect firstRect(point1OnPixelBoundaries, FloatSize(strokeThickness, strokeThickness));
        FloatRect secondRect(point2OnPixelBoundaries, FloatSize(strokeThickness, strokeThickness));
        if (isVerticalLine) {
            firstRect.move(-strokeThickness / 2, -strokeThickness);
            secondRect.move(-strokeThickness / 2, 0);
        } else {
            firstRect.move(-strokeThickness, -strokeThickness / 2);
            secondRect.move(0, -strokeThickness / 2);
        }
        fillRectWithColor(context, firstRect, strokeColor);
        fillRectWithColor(context, secondRect, strokeColor);

        float distance = (isVerticalLine ? (point2.y() - point1.y()) : (point2.x() - point1.x())) - 2 * strokeThickness;
        double patternOffset = calculateStrokePatternOffset(distance, patternWidth);
        double patternWidthAsDouble = patternWidth;
        cairo_set_dash(context, &patternWidthAsDouble, 1, patternOffset);
    }

    // Center line and cut off corners for pattern patining.
    if (isVerticalLine) {
        float centerOffset = (point2OnPixelBoundaries.x() - point1OnPixelBoundaries.x()) / 2;
        point1OnPixelBoundaries.move(centerOffset, patternWidth);
        point2OnPixelBoundaries.move(-centerOffset, -patternWidth);
    } else {
        float centerOffset = (point2OnPixelBoundaries.y() - point1OnPixelBoundaries.y()) / 2;
        point1OnPixelBoundaries.move(patternWidth, centerOffset);
        point2OnPixelBoundaries.move(-patternWidth, -centerOffset);
    }
    setSourceRGBAFromColor(context, strokeColor);
    cairo_set_line_width(context, strokeThickness);
    cairo_move_to(context, point1OnPixelBoundaries.x(), point1OnPixelBoundaries.y());
    cairo_line_to(context, point2OnPixelBoundaries.x(), point2OnPixelBoundaries.y());
    cairo_stroke(context);
    cairo_set_antialias(context, prev);
}

// This is only used to draw borders, so we should not draw shadows.
void GraphicsContext::drawLine(const FloatPoint& point1, const FloatPoint& point2)
{
    if (paintingDisabled())
        return;

    cairo_t* cairoContext = platformContext()->cr();
    cairo_save(cairoContext);
    drawLineOnCairoContext(this, cairoContext, point1, point2);
    cairo_restore(cairoContext);
}

// This method is only used to draw the little circles used in lists.
void GraphicsContext::drawEllipse(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    cairo_save(cr);
    float yRadius = .5 * rect.height();
    float xRadius = .5 * rect.width();
    cairo_translate(cr, rect.x() + xRadius, rect.y() + yRadius);
    cairo_scale(cr, xRadius, yRadius);
    cairo_arc(cr, 0., 0., 1., 0., 2 * M_PI);
    cairo_restore(cr);

    if (fillColor().alpha()) {
        setSourceRGBAFromColor(cr, fillColor());
        cairo_fill_preserve(cr);
    }

    if (strokeStyle() != NoStroke) {
        setSourceRGBAFromColor(cr, strokeColor());
        cairo_set_line_width(cr, strokeThickness());
        cairo_stroke(cr);
    } else
        cairo_new_path(cr);
}

void GraphicsContext::drawConvexPolygon(size_t npoints, const FloatPoint* points, bool shouldAntialias)
{
    if (paintingDisabled())
        return;

    if (npoints <= 1)
        return;

    cairo_t* cr = platformContext()->cr();

    cairo_save(cr);
    cairo_set_antialias(cr, shouldAntialias ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);
    addConvexPolygonToContext(cr, npoints, points);

    if (fillColor().alpha()) {
        setSourceRGBAFromColor(cr, fillColor());
        cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
        cairo_fill_preserve(cr);
    }

    if (strokeStyle() != NoStroke) {
        setSourceRGBAFromColor(cr, strokeColor());
        cairo_set_line_width(cr, strokeThickness());
        cairo_stroke(cr);
    } else
        cairo_new_path(cr);

    cairo_restore(cr);
}

void GraphicsContext::clipConvexPolygon(size_t numPoints, const FloatPoint* points, bool antialiased)
{
    if (paintingDisabled())
        return;

    if (numPoints <= 1)
        return;

    cairo_t* cr = platformContext()->cr();

    cairo_new_path(cr);
    cairo_fill_rule_t savedFillRule = cairo_get_fill_rule(cr);
    cairo_antialias_t savedAntialiasRule = cairo_get_antialias(cr);

    cairo_set_antialias(cr, antialiased ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
    addConvexPolygonToContext(cr, numPoints, points);
    cairo_clip(cr);

    cairo_set_antialias(cr, savedAntialiasRule);
    cairo_set_fill_rule(cr, savedFillRule);
}

void GraphicsContext::fillPath(const Path& path)
{
    if (paintingDisabled() || path.isEmpty())
        return;

    cairo_t* cr = platformContext()->cr();
    setPathOnCairoContext(cr, path.platformPath()->context());
    shadowAndFillCurrentCairoPath(this);
}

void GraphicsContext::strokePath(const Path& path)
{
    if (paintingDisabled() || path.isEmpty())
        return;

    cairo_t* cr = platformContext()->cr();
    setPathOnCairoContext(cr, path.platformPath()->context());
    shadowAndStrokeCurrentCairoPath(this);
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    cairo_rectangle(cr, rect.x(), rect.y(), rect.width(), rect.height());
    shadowAndFillCurrentCairoPath(this);
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color, ColorSpace)
{
    if (paintingDisabled())
        return;

    FloatRoundedRect newRect(0,0,0,0);
    if (m_data->isDrawingOnOffscreen())
        newRect.setRect(roundToDevicePixels(rect));
    else
        newRect.setRect(rect);

    if (hasShadow())
        platformContext()->shadowBlur().drawRectShadow(this, newRect);

    fillRectWithColor(platformContext()->cr(), newRect.rect(), color);
}

void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    FloatRect newRect;
    if (m_data->isDrawingOnOffscreen())
        newRect = roundToDevicePixels(rect);
    else
        newRect = rect;

    cairo_t* cr = platformContext()->cr();
    cairo_rectangle(cr, newRect.x(), newRect.y(), newRect.width(), newRect.height());
    cairo_fill_rule_t savedFillRule = cairo_get_fill_rule(cr);
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
    // The rectangular clip function is traditionally not expected to
    // antialias. If we don't force antialiased clipping here,
    // edge fringe artifacts may occur at the layer edges
    // when a transformation is applied to the GraphicsContext
    // while drawing the transformed layer.
    cairo_antialias_t savedAntialiasRule = cairo_get_antialias(cr);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
    cairo_clip(cr);
    cairo_set_fill_rule(cr, savedFillRule);
    cairo_set_antialias(cr, savedAntialiasRule);
    m_data->clip(rect);
}

void GraphicsContext::clipPath(const Path& path, WindRule clipRule)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    if (!path.isNull())
        setPathOnCairoContext(cr, path.platformPath()->context());
    cairo_set_fill_rule(cr, clipRule == RULE_EVENODD ? CAIRO_FILL_RULE_EVEN_ODD : CAIRO_FILL_RULE_WINDING);
    cairo_clip(cr);
}

IntRect GraphicsContext::clipBounds() const
{
    double x1, x2, y1, y2;
    cairo_clip_extents(platformContext()->cr(), &x1, &y1, &x2, &y2);
    return enclosingIntRect(FloatRect(x1, y1, x2 - x1, y2 - y1));
}

static inline void adjustFocusRingColor(Color& color)
{
    // Force the alpha to 50%.  This matches what the Mac does with outline rings.
    color.setRGB(makeRGBA(color.red(), color.green(), color.blue(), 127));
}

static inline void adjustFocusRingLineWidth(int& width)
{
}

static inline StrokeStyle focusRingStrokeStyle()
{
    return SolidStroke;
}

void GraphicsContext::drawFocusRing(const Path& path, int width, int /* offset */, const Color& color)
{
    // FIXME: We should draw paths that describe a rectangle with rounded corners
    // so as to be consistent with how we draw rectangular focus rings.
    Color ringColor = color;
    adjustFocusRingColor(ringColor);
    adjustFocusRingLineWidth(width);

    cairo_t* cr = platformContext()->cr();
    cairo_save(cr);
    appendWebCorePathToCairoContext(cr, path);
    setSourceRGBAFromColor(cr, ringColor);
    cairo_set_line_width(cr, width);
    setPlatformStrokeStyle(focusRingStrokeStyle());
    cairo_stroke(cr);
    cairo_restore(cr);
}

void GraphicsContext::drawFocusRing(const Vector<IntRect>& rects, int width, int /* offset */, const Color& color)
{
    if (paintingDisabled())
        return;

    unsigned rectCount = rects.size();

    cairo_t* cr = platformContext()->cr();
    cairo_save(cr);
    cairo_push_group(cr);
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        cairo_restore(cr);
        return;
    }
    cairo_new_path(cr);

    int radius = (width - 1) / 2;
    Path path;
    for (unsigned i = 0; i < rectCount; ++i) {
        if (i > 0)
            path.clear();
        path.addRoundedRect(rects[i], FloatSize(radius, radius));
        appendWebCorePathToCairoContext(cr, path);
    }

    Color ringColor = color;
    adjustFocusRingColor(ringColor);
    adjustFocusRingLineWidth(width);
    setSourceRGBAFromColor(cr, ringColor);
    cairo_set_line_width(cr, width);
    setPlatformStrokeStyle(focusRingStrokeStyle());

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_stroke_preserve(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
    cairo_fill(cr);

    cairo_pop_group_to_source(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_paint(cr);
    cairo_restore(cr);
}

FloatRect GraphicsContext::computeLineBoundsForText(const FloatPoint& origin, float width, bool printing)
{
    bool dummyBool;
    Color dummyColor;
    return computeLineBoundsAndAntialiasingModeForText(origin, width, printing, dummyBool, dummyColor);
}

void GraphicsContext::drawLineForText(const FloatPoint& origin, float width, bool printing, bool doubleLines, StrokeStyle strokeStyle)
{
    if (paintingDisabled())
        return;

    cairo_t* cairoContext = platformContext()->cr();
    cairo_save(cairoContext);

    // This bumping of <1 stroke thicknesses matches the one in drawLineOnCairoContext.
    FloatPoint endPoint(origin + IntSize(width, 0));
    FloatRect lineExtents(origin, FloatSize(width, strokeThickness()));

    ShadowBlur& shadow = platformContext()->shadowBlur();
    if (shadow.type() != ShadowBlur::NoShadow) {
        if (GraphicsContext* shadowContext = shadow.beginShadowLayer(this, lineExtents)) {
            drawLineOnCairoContext(this, shadowContext->platformContext()->cr(), origin, endPoint);
            shadow.endShadowLayer(this);
        }
    }

    drawLineOnCairoContext(this, cairoContext, origin, endPoint);
    if (doubleLines) {
        FloatPoint start(origin + FloatSize(0, strokeThickness()*2));
        endPoint += FloatSize(0, strokeThickness()*2);
        drawLineOnCairoContext(this, cairoContext, start, endPoint);
    }
    cairo_restore(cairoContext);
}

void GraphicsContext::drawLinesForText(const FloatPoint& point, const DashArray& widths, bool printing, bool doubleUnderlines, StrokeStyle strokeStyle)
{
    if (paintingDisabled())
        return;

    if (widths.size() <= 0)
        return;

    Color localStrokeColor(strokeColor());

    bool shouldAntialiasLine;
    FloatRect bounds = computeLineBoundsAndAntialiasingModeForText(point, widths.last(), printing, shouldAntialiasLine, localStrokeColor);

    Vector<FloatRect, 4> dashBounds;
    ASSERT(!(widths.size() % 2));
    dashBounds.reserveInitialCapacity(dashBounds.size() / 2);

    float dashWidth = 0;
    switch (strokeStyle) {
    case DottedStroke:
        dashWidth = bounds.height();
        break;
    case DashedStroke:
        dashWidth = 2 * bounds.height();
        break;
    case SolidStroke:
    default:
        break;
    }

    for (size_t i = 0; i < widths.size(); i += 2) {
        auto left = widths[i];
        auto width = widths[i + 1] - widths[i];
        if (!dashWidth)
            dashBounds.append(FloatRect(FloatPoint(bounds.x() + left, bounds.y()), FloatSize(width, bounds.height())));
        else {
            auto startParticle = static_cast<unsigned>(std::ceil(left / (2 * dashWidth)));
            auto endParticle = static_cast<unsigned>((left + width) / (2 * dashWidth));
            for (unsigned j = startParticle; j < endParticle; ++j)
                dashBounds.append(FloatRect(FloatPoint(bounds.x() + j * 2 * dashWidth, bounds.y()), FloatSize(dashWidth, bounds.height())));
        }
    }

    if (doubleUnderlines) {
        // The space between double underlines is equal to the height of the underline
        for (size_t i = 0; i < widths.size(); i += 2)
            dashBounds.append(FloatRect(FloatPoint(bounds.x() + widths[i], bounds.y() + 2 * bounds.height()), FloatSize(widths[i + 1] - widths[i], bounds.height())));
    }

    cairo_t* cr = platformContext()->cr();
    cairo_save(cr);

    for (auto& dash : dashBounds)
        fillRectWithColor(cr, dash, localStrokeColor);

    cairo_restore(cr);
}

void GraphicsContext::drawLineForDocumentMarker(const FloatPoint& origin, float width, DocumentMarkerLineStyle style)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    cairo_save(cr);

    switch (style) {
    case DocumentMarkerSpellingLineStyle:
        cairo_set_source_rgb(cr, 1, 0, 0);
        break;
    case DocumentMarkerGrammarLineStyle:
        cairo_set_source_rgb(cr, 0, 1, 0);
        break;
    default:
        cairo_restore(cr);
        return;
    }

    drawErrorUnderline(cr, origin.x(), origin.y(), width, cMisspellingLineThickness);

    cairo_restore(cr);
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& frect, RoundingMode)
{
    FloatRect result;
    double x = frect.x();
    double y = frect.y();
    cairo_t* cr = platformContext()->cr();
    cairo_user_to_device(cr, &x, &y);
    // round half up
    x = floor(x + 0.5);
    y = floor(y + 0.5);
    cairo_device_to_user(cr, &x, &y);
    result.setX(narrowPrecisionToFloat(x));
    result.setY(narrowPrecisionToFloat(y));

    // We must ensure width and height are at least 1 (or -1) when
    // we're given float values in the range between 0 and 1 (or -1 and 0).
    double width = frect.width();
    double height = frect.height();
    cairo_user_to_device_distance(cr, &width, &height);
    if (width > -1 && width < 0)
        width = -1;
    else if (width > 0 && width < 1)
        width = 1;
    else
        width = std::round(width);
    if (height > -1 && height < 0)
        height = -1;
    else if (height > 0 && height < 1)
        height = 1;
    else
        height = std::round(height);
    cairo_device_to_user_distance(cr, &width, &height);
    result.setWidth(narrowPrecisionToFloat(width));
    result.setHeight(narrowPrecisionToFloat(height));

    return result;
}

void GraphicsContext::translate(float x, float y)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    cairo_translate(cr, x, y);
    m_data->translate(x, y);
}

void GraphicsContext::setPlatformFillColor(const Color& col, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;
    if (!platformContext())
        return;

    unsigned int color = platformColor(col.alpha(), col.red(), col.green(), col.blue());
    wkcDrawContextSetFillColorPeer(platformContext(), color);
}

void GraphicsContext::setPlatformStrokeColor(const Color& col, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;
    if (!platformContext())
        return;

    unsigned int color = platformColor(col.alpha(), col.red(), col.green(), col.blue());
    wkcDrawContextSetStrokeColorPeer(platformContext(), color);
}

void GraphicsContext::setPlatformStrokeThickness(float strokeThickness)
{
    if (paintingDisabled())
        return;
    if (!platformContext())
        return;

    cairo_set_line_width(platformContext()->cr(), strokeThickness);
}

void GraphicsContext::setPlatformStrokeStyle(StrokeStyle strokeStyle)
{
    static double dashPattern[] = {5.0, 5.0};
    static double dotPattern[] = {1.0, 1.0};

    if (paintingDisabled())
        return;

    int style = WKC_STROKESTYLE_SOLID;

    switch (strokeStyle) {
    case NoStroke:
        // FIXME: is it the right way to emulate NoStroke?
        cairo_set_line_width(platformContext()->cr(), 0);
        style = WKC_STROKESTYLE_NO;
        break;
    case SolidStroke:
        cairo_set_dash(platformContext()->cr(), 0, 0, 0);
        style = WKC_STROKESTYLE_SOLID;
        break;
    case DottedStroke:
        cairo_set_dash(platformContext()->cr(), dotPattern, 2, 0);
        style = WKC_STROKESTYLE_DOTTED;
        break;
    case DashedStroke:
        cairo_set_dash(platformContext()->cr(), dashPattern, 2, 0);
        style = WKC_STROKESTYLE_DASHED;
        break;
    case DoubleStroke:
    case WavyStroke:
        // Ugh!: not supported
        // 150413 ACCESS Co.,Ltd.
        cairo_set_dash(platformContext()->cr(), 0, 0, 0);
        style = WKC_STROKESTYLE_SOLID;
        break;
    default:
        return;
    }

    if (!platformContext())
        return;
    wkcDrawContextSetStrokeStylePeer(platformContext(), style);
}

void GraphicsContext::setPlatformTextDrawingMode(TextDrawingModeFlags mode)
{
    if (paintingDisabled())
        return;

    int ret = WKC_FONT_DRAWINGMODE_INVISIBLE;
    if (mode&TextModeFill)
        ret |= WKC_FONT_DRAWINGMODE_FILL;
    if (mode&TextModeStroke)
        ret |= WKC_FONT_DRAWINGMODE_STROKE;

    if (!platformContext())
        return;
    wkcDrawContextSetTextDrawingModePeer(platformContext(), ret);
}

void GraphicsContext::setURLForRect(const URL& link, const IntRect& destRect)
{
    notImplemented();
}

void GraphicsContext::concatCTM(const AffineTransform& transform)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    const cairo_matrix_t matrix = cairo_matrix_t(transform);
    cairo_transform(cr, &matrix);
    m_data->concatCTM(transform);
}

void GraphicsContext::setCTM(const AffineTransform& transform)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    const cairo_matrix_t matrix = cairo_matrix_t(transform);
    cairo_set_matrix(cr, &matrix);
    m_data->setCTM(transform);
}

void GraphicsContext::setPlatformShadow(FloatSize const& size, float blur, Color const& color, ColorSpace)
{
    if (paintingDisabled())
        return;

    if (m_state.shadowsIgnoreTransforms) {
        // Meaning that this graphics context is associated with a CanvasRenderingContext
        // We flip the height since CG and HTML5 Canvas have opposite Y axis
        m_state.shadowOffset = FloatSize(size.width(), -size.height());
    }

    // Cairo doesn't support shadows natively, they are drawn manually in the draw* functions using ShadowBlur.
    platformContext()->shadowBlur().setShadowValues(FloatSize(m_state.shadowBlur, m_state.shadowBlur),
                                                    m_state.shadowOffset,
                                                    m_state.shadowColor,
                                                    m_state.shadowColorSpace,
                                                    m_state.shadowsIgnoreTransforms);
}

void GraphicsContext::clearPlatformShadow()
{
    if (paintingDisabled())
        return;

    platformContext()->shadowBlur().clear();
}

void GraphicsContext::beginPlatformTransparencyLayer(float opacity)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    cairo_push_group(cr);
    m_data->layers.append(opacity);
}

void GraphicsContext::endPlatformTransparencyLayer()
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();

    cairo_pop_group_to_source(cr);
    cairo_paint_with_alpha(cr, m_data->layers.last());
    m_data->layers.removeLast();
}

bool GraphicsContext::supportsTransparencyLayers()
{
    return true;
}

void GraphicsContext::clearRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();

    cairo_save(cr);
    cairo_rectangle(cr, rect.x(), rect.y(), rect.width(), rect.height());
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_fill(cr);
    cairo_restore(cr);
}

void GraphicsContext::strokeRect(const FloatRect& rect, float width)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    cairo_save(cr);
    cairo_rectangle(cr, rect.x(), rect.y(), rect.width(), rect.height());
    cairo_set_line_width(cr, width);
    shadowAndStrokeCurrentCairoPath(this);
    cairo_restore(cr);
}

void GraphicsContext::setLineCap(LineCap lineCap)
{
    if (paintingDisabled())
        return;

    cairo_line_cap_t cairoCap = CAIRO_LINE_CAP_BUTT;
    switch (lineCap) {
    case ButtCap:
        // no-op
        break;
    case RoundCap:
        cairoCap = CAIRO_LINE_CAP_ROUND;
        break;
    case SquareCap:
        cairoCap = CAIRO_LINE_CAP_SQUARE;
        break;
    }
    cairo_set_line_cap(platformContext()->cr(), cairoCap);
}

void GraphicsContext::setLineDash(const DashArray& dashes, float dashOffset)
{
    DashArray::const_iterator end = dashes.end();
    double sum = 0;

    for (DashArray::const_iterator it = dashes.begin(); it != end; ++it)
        sum += *it;
    if (sum == 0)
        cairo_set_dash(platformContext()->cr(), 0, 0, 0);
    else
        cairo_set_dash(platformContext()->cr(), dashes.data(), dashes.size(), dashOffset);
}

void GraphicsContext::setLineJoin(LineJoin lineJoin)
{
    if (paintingDisabled())
        return;

    cairo_line_join_t cairoJoin = CAIRO_LINE_JOIN_MITER;
    switch (lineJoin) {
    case MiterJoin:
        // no-op
        break;
    case RoundJoin:
        cairoJoin = CAIRO_LINE_JOIN_ROUND;
        break;
    case BevelJoin:
        cairoJoin = CAIRO_LINE_JOIN_BEVEL;
        break;
    }
    cairo_set_line_join(platformContext()->cr(), cairoJoin);
}

void GraphicsContext::setMiterLimit(float miter)
{
    if (paintingDisabled())
        return;

    cairo_set_miter_limit(platformContext()->cr(), miter);
}

void GraphicsContext::setAlpha(float alpha)
{
    platformContext()->setGlobalAlpha(alpha);
    if (alpha<0)
        alpha = 0;
    else if (alpha>1.f)
        alpha = 1.f;
    wkcDrawContextSetAlphaPeer(platformContext(), 255 * alpha);
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator op, BlendMode blendOp)
{
    if (paintingDisabled())
        return;

    cairo_operator_t cairo_op;
    if (blendOp == BlendModeNormal)
        cairo_op = toCairoOperator(op);
    else
        cairo_op = toCairoOperator(blendOp);

    cairo_set_operator(platformContext()->cr(), cairo_op);
}

void GraphicsContext::clip(const Path& path, WindRule windRule)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    if (!path.isNull()) {
        cairo_path_t* pathCopy = cairo_copy_path(path.platformPath()->context());
        if (!pathCopy)
            return;
        WKC_CAIRO_ADD_OBJECT(pathCopy, cairo_path);
        cairo_append_path(cr, pathCopy);
        cairo_path_destroy(pathCopy);
    }
    cairo_fill_rule_t savedFillRule = cairo_get_fill_rule(cr);
    if (windRule == RULE_NONZERO)
        cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);
    else
        cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_clip(cr);
    cairo_set_fill_rule(cr, savedFillRule);
    m_data->clip(path);
}

void GraphicsContext::canvasClip(const Path& path, WindRule windRule)
{
    clip(path, windRule);
}

void GraphicsContext::clipOut(const Path& path)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    double x1, y1, x2, y2;
    cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
    cairo_rectangle(cr, x1, y1, x2 - x1, y2 - y1);
    appendWebCorePathToCairoContext(cr, path);

    cairo_fill_rule_t savedFillRule = cairo_get_fill_rule(cr);
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_clip(cr);
    cairo_set_fill_rule(cr, savedFillRule);
}

void GraphicsContext::rotate(float radians)
{
    if (paintingDisabled())
        return;

    cairo_rotate(platformContext()->cr(), radians);
    m_data->rotate(radians);
}

void GraphicsContext::scale(const FloatSize& size)
{
    if (paintingDisabled())
        return;

    cairo_scale(platformContext()->cr(), size.width(), size.height());
    m_data->scale(size);
}

void GraphicsContext::clipOut(const FloatRect& r)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    double x1, y1, x2, y2;
    cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
    cairo_rectangle(cr, x1, y1, x2 - x1, y2 - y1);
    cairo_rectangle(cr, r.x(), r.y(), r.width(), r.height());
    cairo_fill_rule_t savedFillRule = cairo_get_fill_rule(cr);
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_clip(cr);
    cairo_set_fill_rule(cr, savedFillRule);
}

static inline FloatPoint getPhase(const FloatRect& dest, const FloatRect& tile)
{
    FloatPoint phase = dest.location();
    phase.move(-tile.x(), -tile.y());

    return phase;
}

void GraphicsContext::fillRoundedRect(const FloatRoundedRect& r, const Color& color, ColorSpace colorSpace, BlendMode)
{
    if (paintingDisabled())
        return;

    if (hasShadow())
        platformContext()->shadowBlur().drawRectShadow(this, r);

    cairo_t* cr = platformContext()->cr();
    cairo_save(cr);
    Path path;
    path.addRoundedRect(r);
    appendWebCorePathToCairoContext(cr, path);
    setSourceRGBAFromColor(cr, color);
    cairo_fill(cr);
    cairo_restore(cr);
}

void GraphicsContext::setPlatformShouldAntialias(bool enable)
{
    if (paintingDisabled())
        return;

    // When true, use the default Cairo backend antialias mode (usually this
    // enables standard 'grayscale' antialiasing); false to explicitly disable
    // antialiasing. This is the same strategy as used in drawConvexPolygon().
    cairo_set_antialias(platformContext()->cr(), enable ? CAIRO_ANTIALIAS_DEFAULT : CAIRO_ANTIALIAS_NONE);
}

void GraphicsContext::setImageInterpolationQuality(InterpolationQuality quality)
{
    platformContext()->setImageInterpolationQuality(quality);
}

InterpolationQuality GraphicsContext::imageInterpolationQuality() const
{
    return platformContext()->imageInterpolationQuality();
}

#if ENABLE(3D_TRANSFORMS) && USE(TEXTURE_MAPPER)
TransformationMatrix GraphicsContext::get3DTransform() const
{
    notImplemented();
    return TransformationMatrix();
}

void GraphicsContext::concat3DTransform(const TransformationMatrix& transform)
{
    notImplemented();
}

void GraphicsContext::set3DTransform(const TransformationMatrix& transform)
{
    notImplemented();
}
#endif

PlatformPatternPtr
Pattern::createPlatformPattern(const WebCore::AffineTransform& userSpaceTransformation) const
{
    Image* tile = tileImage();
    if (!tile) return 0;
    ImageWKC* img = (ImageWKC *)tile->nativeImageForCurrentFrame();
    if (!img) return 0;
    if (!img->isBitmapAvail()) return 0;

    cairo_surface_t* surface = 0;

    switch (img->type()) {
    case ImageWKC::EColorARGB8888:
        surface = cairo_image_surface_create_for_data((unsigned char*)img->bitmap(), CAIRO_FORMAT_ARGB32, img->size().width(), img->size().height(), img->rowbytes());
        WKC_CAIRO_ADD_OBJECT(surface, cairo_surface);
        break;
    case ImageWKC::EColorRGB565:
        surface = cairo_image_surface_create_for_data((unsigned char*)img->bitmap(), CAIRO_FORMAT_RGB16_565, img->size().width(), img->size().height(), img->rowbytes());
        WKC_CAIRO_ADD_OBJECT(surface, cairo_surface);
        break;
    default:
        return 0;
    }

    cairo_pattern_t* pattern = cairo_pattern_create_for_surface(surface);
    WKC_CAIRO_ADD_OBJECT(pattern, cairo_pattern);

    // cairo merges patter space and user space itself
    cairo_matrix_t matrix = m_patternSpaceTransformation;
    cairo_matrix_invert(&matrix);
    cairo_pattern_set_matrix(pattern, &matrix);

    if (wkcDrawContexCairoIsUseFilterNearestPeer())
        cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);

    if (m_repeatX || m_repeatY) {
        cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

        PatternPrivateData* repeat = new PatternPrivateData;
        repeat->m_repeatX = m_repeatX;
        repeat->m_repeatY = m_repeatY;
        repeat->m_width = img->size().width();
        repeat->m_height = img->size().height();
        cairo_pattern_set_user_data(pattern, &cPatternPrivateDataKey, repeat, _PatternPrivateDataDelete);
    }
    WKC_CAIRO_REMOVE_OBJECT(surface);
    cairo_surface_destroy(surface);

    return pattern;
}

void GraphicsContext::updateDocumentMarkerResources()
{
    // Unnecessary, since our document markers don't use resources.
}

} // namespace WebCore

#endif // USE(WKC_CAIRO)

/*
 * Copyright (C) 2006 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008, 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Brent Fulgham <bfulgham@webkit.org>
 * Copyright (C) 2010, 2011 Igalia S.L.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2012, Intel Corporation
 * Copyright (c) 2011-2020 ACCESS CO., LTD. All rights reserved.
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
#include "CairoOperations.h"
#include "CairoUtilities.h"
#include "DrawErrorUnderline.h"
#include "FloatConversion.h"
#include "FloatRect.h"
#include "FloatRoundedRect.h"
#include "Font.h"
#include "GraphicsContextImpl.h"
#include "GraphicsContextPlatformPrivateCairo.h"
#include "Image.h"
#include "ImageBuffer.h"
#include "ImageObserver.h"
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

AffineTransform::operator cairo_matrix_t() const
{
    cairo_matrix_t m;

    cairo_matrix_init (&m,
                       a(),
                       b(),
                       c(),
                       d(),
                       e(),
                       f());
    return m;
}

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

enum PathDrawingStyle { 
    Fill = 1,
    Stroke = 2,
    FillAndStroke = Fill + Stroke
};

void GraphicsContext::platformInit(PlatformContextCairo* platformContext)
{
    if (!platformContext)
        return;

    m_data = new GraphicsContextPlatformPrivate(*platformContext);
    m_data->platformContext.setGraphicsContextPrivate(m_data);
    m_data->syncContext(platformContext->cr());

    // Make sure the context starts in sync with our state.
    setPlatformFillColor(fillColor());
    setPlatformStrokeColor(strokeColor());
    setPlatformStrokeStyle(SolidStroke);
    setPlatformTextDrawingMode(TextModeFill);
    setAlpha(1.0);
}

void GraphicsContext::platformDestroy()
{
    if (m_data) {
        m_data->platformContext.setGraphicsContextPrivate(nullptr);
        delete m_data;
        m_data = nullptr;
    }
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
    return &m_data->platformContext;
}

bool GraphicsContext::isAcceleratedContext() const
{
    return false;
}

void GraphicsContext::savePlatformState()
{
    platformContext()->save();
    wkcDrawContextSaveStatePeer(platformContext());
}

void GraphicsContext::restorePlatformState()
{
    platformContext()->restore();
    wkcDrawContextRestoreStatePeer(platformContext());
}

// Draws a filled rectangle with a stroked border.
void GraphicsContext::drawRect(const FloatRect& rect, float /*borderThickness*/)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    cairo_save(cr);

    FloatRect newRect;
    if (wkcDrawContextIsForOffscreenPeer(platformContext()))
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

void GraphicsContext::fillPath(const Path& path)
{
    if (paintingDisabled() || path.isEmpty())
        return;

    auto& state = this->state();
    Cairo::fillPath(*platformContext(), path, Cairo::FillSource(state), Cairo::ShadowState(state));
}

void GraphicsContext::strokePath(const Path& path)
{
    if (paintingDisabled() || path.isEmpty())
        return;

    auto& state = this->state();
    Cairo::strokePath(*platformContext(), path, Cairo::StrokeSource(state), Cairo::ShadowState(state));
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    auto& state = this->state();
    Cairo::fillRect(*platformContext(), rect, Cairo::FillSource(state), Cairo::ShadowState(state));
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color)
{
    if (paintingDisabled())
        return;

    FloatRoundedRect newRect(0,0,0,0);
    if (wkcDrawContextIsForOffscreenPeer(platformContext()))
        newRect.setRect(roundToDevicePixels(rect));
    else
        newRect.setRect(rect);

#if 0 // to avoid a performance issue
    Cairo::fillRect(*platformContext(), rect, color, Cairo::ShadowState(state()));
#else
    const Cairo::ShadowState& shadowState = Cairo::ShadowState(state());
    if (shadowState.isVisible()) {
        ShadowBlur shadow({ shadowState.blur, shadowState.blur }, shadowState.offset, shadowState.color, shadowState.ignoreTransforms);
        shadow.drawRectShadow(*this, newRect);
    }

    fillRectWithColor(platformContext()->cr(), newRect.rect(), color);
#endif
}

void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    FloatRect newRect;
    if (wkcDrawContextIsForOffscreenPeer(platformContext()))
        newRect = roundToDevicePixels(rect);
    else
        newRect = rect;

    Cairo::clip(*platformContext(), newRect);

    m_data->clip(rect);
}

void GraphicsContext::clipPath(const Path& path, WindRule clipRule)
{
    if (paintingDisabled())
        return;

    Cairo::clipPath(*platformContext(), path, clipRule);

    m_data->clip(path);
}

void GraphicsContext::clipToImageBuffer(ImageBuffer& buffer, const FloatRect& rect)
{
    if (paintingDisabled())
        return;
    buffer.clip(*this, rect);
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
    color = Color(makeRGBA(color.red(), color.green(), color.blue(), 127));
}

static inline void adjustFocusRingLineWidth(float& width)
{
}

static inline StrokeStyle focusRingStrokeStyle()
{
    return SolidStroke;
}

void GraphicsContext::drawFocusRing(const Path& path, float width, float /* offset */, const Color& color)
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

void GraphicsContext::drawFocusRing(const Vector<FloatRect>& rects, float width, float /* offset */, const Color& color)
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

void GraphicsContext::drawLineForText(const FloatPoint& origin, float width, bool printing, bool doubleUnderlines, StrokeStyle strokeStyle)
{
    drawLinesForText(origin, DashArray { 0, width }, printing, doubleUnderlines, strokeStyle);
}

void GraphicsContext::drawLinesForText(const FloatPoint& point, const DashArray& widths, bool printing, bool doubleUnderlines, StrokeStyle strokeStyle)
{
    if (paintingDisabled())
        return;

    if (widths.isEmpty())
        return;

    Color localStrokeColor(strokeColor());

    FloatRect bounds = computeLineBoundsAndAntialiasingModeForText(point, widths.last(), printing, localStrokeColor);

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

void GraphicsContext::drawDotsForDocumentMarker(const FloatRect& rect, DocumentMarkerLineStyle style)
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    cairo_save(cr);

    switch (style.mode) {
    case DocumentMarkerLineStyle::Mode::Spelling:
        cairo_set_source_rgb(cr, 1, 0, 0);
        break;
    case DocumentMarkerLineStyle::Mode::Grammar:
        cairo_set_source_rgb(cr, 0, 1, 0);
        break;
    default:
        cairo_restore(cr);
        return;
    }

    drawErrorUnderline(cr, rect.x(), rect.y(), rect.width(), rect.height());

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

void GraphicsContext::setPlatformFillColor(const Color& col)
{
    if (paintingDisabled())
        return;
    if (!platformContext())
        return;

    unsigned int color = platformColor(col.alpha(), col.red(), col.green(), col.blue());
    wkcDrawContextSetFillColorPeer(platformContext(), color);
}

void GraphicsContext::setPlatformStrokeColor(const Color& col)
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
    static const double dashPattern[] = {5.0, 5.0};
    static const double dotPattern[] = {1.0, 1.0};

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

void GraphicsContext::setURLForRect(const URL& link, const FloatRect& destRect)
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

void GraphicsContext::setPlatformShadow(FloatSize const& size, float blur, Color const& color)
{
    if (paintingDisabled())
        return;

    if (m_state.shadowsIgnoreTransforms) {
        // Meaning that this graphics context is associated with a CanvasRenderingContext
        // We flip the height since CG and HTML5 Canvas have opposite Y axis
        m_state.shadowOffset = FloatSize(size.width(), -size.height());
    }
}

void GraphicsContext::clearPlatformShadow()
{
    if (paintingDisabled())
        return;
}

void GraphicsContext::beginPlatformTransparencyLayer(float opacity)
{
    if (paintingDisabled())
        return;

    cairo_push_group(platformContext()->cr());
    platformContext()->layers().append(opacity);
}

void GraphicsContext::endPlatformTransparencyLayer()
{
    if (paintingDisabled())
        return;

    cairo_t* cr = platformContext()->cr();
    cairo_pop_group_to_source(cr);
    cairo_paint_with_alpha(cr, platformContext()->layers().takeLast());
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

void GraphicsContext::strokeRect(const FloatRect& rect, float lineWidth)
{
    if (paintingDisabled())
        return;

    auto& state = this->state();
    Cairo::strokeRect(*platformContext(), rect, lineWidth, Cairo::StrokeSource(state), Cairo::ShadowState(state));
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
    if (alpha<0)
        alpha = 0;
    else if (alpha>1.f)
        alpha = 1.f;
    wkcDrawContextSetAlphaPeer(platformContext(), 255 * alpha);
    m_state.alpha = alpha;
}

void GraphicsContext::setPlatformAlpha(float alpha)
{
    setAlpha(alpha);
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator op, BlendMode blendOp)
{
    if (paintingDisabled())
        return;

    cairo_set_operator(platformContext()->cr(), toCairoOperator(op, blendOp));
}

void GraphicsContext::canvasClip(const Path& path, WindRule windRule)
{
    clipPath(path,windRule);
}

void GraphicsContext::clipOut(const Path& path)
{
    if (paintingDisabled())
        return;

    Cairo::clipOut(*platformContext(), path);
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

void GraphicsContext::clipOut(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    Cairo::clipOut(*platformContext(), rect);
}

static inline FloatPoint getPhase(const FloatRect& dest, const FloatRect& tile)
{
    FloatPoint phase = dest.location();
    phase.move(-tile.x(), -tile.y());

    return phase;
}

void GraphicsContext::fillRoundedRect(const FloatRoundedRect& rect, const Color& color, BlendMode)
{
    if (paintingDisabled())
        return;

#if 0 // to avoid a performance issue
    Cairo::fillRoundedRect(*platformContext(), rect, color, Cairo::ShadowState(state()));
#else
    const Cairo::ShadowState& shadowState = Cairo::ShadowState(state());
    if (shadowState.isVisible()) {
        ShadowBlur shadow({ shadowState.blur, shadowState.blur }, shadowState.offset, shadowState.color, shadowState.ignoreTransforms);
        shadow.drawRectShadow(*this, rect);
    }

    cairo_t* cr = platformContext()->cr();
    cairo_save(cr);
    Path path;
    path.addRoundedRect(rect);
    appendWebCorePathToCairoContext(cr, path);
    setSourceRGBAFromColor(cr, color);
    cairo_fill(cr);
    cairo_restore(cr);
#endif
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

void GraphicsContext::setPlatformImageInterpolationQuality(InterpolationQuality quality)
{
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
    Image& tile = tileImage();
    NativeImagePtr img = tile.nativeImageForCurrentFrame();
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

void GraphicsContext::drawPattern(Image& image, const FloatRect& destRect, const FloatRect& srcRect, const AffineTransform& patternTransform,
    const FloatPoint& phase, const FloatSize& spacing, CompositeOperator op, BlendMode blendMode)
{
    void* dc = (void *)platformContext();
    NativeImagePtr bitmap = image.nativeImageForCurrentFrame();

    // Avoid NaN
    if (!isfinite(phase.x()) || !isfinite(phase.y()))
        return;
    if (!patternTransform.a() || !patternTransform.d())
        return;
    if (!bitmap) // If it's too early we won't have an image yet.
        return;

    if (!bitmap->isBitmapAvail())
        return;

    cairo_surface_t* surface = 0;
    cairo_t* cr = ((PlatformContextCairo*)dc)->cr();

    cairo_operator_t cop;
    cop = toCairoOperator(op, blendMode);

    IntSize isize(image.size());
    if (bitmap->type() == ImageWKC::EColorARGB8888) {
        surface = cairo_image_surface_create_for_data((unsigned char*)bitmap->bitmap(), CAIRO_FORMAT_ARGB32, bitmap->size().width(), bitmap->size().height(), bitmap->rowbytes());
        WKC_CAIRO_ADD_OBJECT(surface, cairo_surface);
        drawPatternToCairoContext(cr, surface, isize, srcRect, patternTransform, phase, cop, destRect);
        WKC_CAIRO_REMOVE_OBJECT(surface);
        cairo_surface_destroy(surface);
    } else if (bitmap->type() == ImageWKC::EColorRGB565) {
        surface = cairo_image_surface_create_for_data((unsigned char*)bitmap->bitmap(), CAIRO_FORMAT_RGB16_565, bitmap->size().width(), bitmap->size().height(), bitmap->rowbytes());
        WKC_CAIRO_ADD_OBJECT(surface, cairo_surface);
        drawPatternToCairoContext(cr, surface, isize, srcRect, patternTransform, phase, cop, destRect);
        WKC_CAIRO_REMOVE_OBJECT(surface);
        cairo_surface_destroy(surface);
    }
    if (ImageObserver* observer = image.imageObserver())
        observer->didDraw(image);
}

} // namespace WebCore

#endif // USE(WKC_CAIRO)

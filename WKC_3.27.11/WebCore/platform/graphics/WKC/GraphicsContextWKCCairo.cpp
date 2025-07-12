/*
 * Copyright (C) 2006 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008, 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Brent Fulgham <bfulgham@webkit.org>
 * Copyright (C) 2010, 2011 Igalia S.L.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2012, Intel Corporation
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
#include "GraphicsContextCairo.h"

#if USE(WKC_CAIRO)

#include "CairoUtilities.h"
#include <wkc/wkcgpeer.h>

namespace WebCore {

static uint32_t
platformColor(const Color& color)
{
    if (auto srgb = color.tryGetAsSRGBABytes())
        return PackedColor::ARGB { *srgb }.value;

    return 0;
}

static int
platformTextDrawingMode(const TextDrawingModeFlags& mode)
{
    int ret = WKC_FONT_DRAWINGMODE_INVISIBLE;

    if (mode & TextDrawingMode::Fill)
        ret |= WKC_FONT_DRAWINGMODE_FILL;

    if (mode & TextDrawingMode::Stroke)
        ret |= WKC_FONT_DRAWINGMODE_STROKE;

    return ret;
}

static int
platformStrokeStyle(const StrokeStyle& strokeStyle)
{
    int style = WKC_STROKESTYLE_SOLID;

    switch (strokeStyle) {
    case NoStroke:
        style = WKC_STROKESTYLE_NO;
        break;
    case SolidStroke:
        style = WKC_STROKESTYLE_SOLID;
        break;
    case DottedStroke:
        style = WKC_STROKESTYLE_DOTTED;
        break;
    case DashedStroke:
        style = WKC_STROKESTYLE_DASHED;
        break;
    case DoubleStroke:
    case WavyStroke:
        // Ugh!: not supported
        // 150413 ACCESS Co.,Ltd.
        style = WKC_STROKESTYLE_SOLID;
        break;
    default:
        break;;
    }

    return style;
}

static int
platformAlpha(float alpha)
{
    if (alpha < 0)
        alpha = 0.f;
    else if (alpha > 1.f)
        alpha = 1.f;

    return alpha * 255.f;
}

static void
setPlatformState(GraphicsContextCairo* ctx)
{
    wkcDrawContextSetFillColorPeer(ctx, platformColor(ctx->fillColor()));
    wkcDrawContextSetStrokeColorPeer(ctx, platformColor(ctx->strokeColor()));
    wkcDrawContextSetTextDrawingModePeer(ctx, platformTextDrawingMode(ctx->textDrawingMode()));
    wkcDrawContextSetStrokeStylePeer(ctx, platformStrokeStyle(ctx->strokeStyle()));
    wkcDrawContextSetAlphaPeer(ctx, platformAlpha(ctx->alpha()));
}

void
GraphicsContextCairo::platformInit()
{
    setPlatformState(this);
}

void
GraphicsContextCairo::drawGlyphs(const Font& font, const GlyphBufferGlyph* glyphs, const GlyphBufferAdvance* advances, unsigned numGlyphs, const FloatPoint& point, FontSmoothingMode fontSmoothing)
{
    setPlatformState(this);
    GraphicsContext::drawGlyphs(font, glyphs, advances, numGlyphs, point, fontSmoothing);
}

//This function is copied from CairoOperations.cpp.
static inline void fillRectWithColor(cairo_t* cr, const FloatRect& rect, const Color& color)
{
    if (!color.isVisible() && cairo_get_operator(cr) == CAIRO_OPERATOR_OVER)
        return;

    setSourceRGBAFromColor(cr, color);
    cairo_rectangle(cr, rect.x(), rect.y(), rect.width(), rect.height());
    cairo_fill(cr);
}

void GraphicsContextCairo::drawLinesForText(const FloatPoint& point, float thickness, const DashArray& widths, bool printing, bool doubleUnderlines, StrokeStyle strokeStyle)
{
    if (widths.isEmpty())
        return;

    Color localStrokeColor(strokeColor());

    FloatRect bounds = computeLineBoundsAndAntialiasingModeForText(FloatRect{ point , FloatSize{ static_cast<float>(widths.last()) , thickness} }, printing, localStrokeColor);

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

} // namespace WebCore

#endif // USE(WKC_CAIRO)

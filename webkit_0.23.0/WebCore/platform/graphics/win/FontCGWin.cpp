/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2013 Apple Inc.  All rights reserved.
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
#include "FontCascade.h"

#include "AffineTransform.h"
#include "FloatConversion.h"
#include "Font.h"
#include "GlyphBuffer.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "UniscribeController.h"
#include "WebCoreTextRenderer.h"
#include <ApplicationServices/ApplicationServices.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#include <wtf/MathExtras.h>

namespace WebCore {

const int syntheticObliqueAngle = 14;

static inline CGFloat toCGFloat(FIXED f)
{
    return f.value + f.fract / CGFloat(65536.0);
}

static CGPathRef createPathForGlyph(HDC hdc, Glyph glyph)
{
    CGMutablePathRef path = CGPathCreateMutable();

    static const MAT2 identity = { 0, 1,  0, 0,  0, 0,  0, 1 };
    GLYPHMETRICS glyphMetrics;
    // GGO_NATIVE matches the outline perfectly when Windows font smoothing is off.
    // GGO_NATIVE | GGO_UNHINTED does not match perfectly either when Windows font smoothing is on or off.
    DWORD outlineLength = GetGlyphOutline(hdc, glyph, GGO_GLYPH_INDEX | GGO_NATIVE, &glyphMetrics, 0, 0, &identity);
    ASSERT(outlineLength >= 0);
    if (outlineLength < 0)
        return path;

    Vector<UInt8> outline(outlineLength);
    GetGlyphOutline(hdc, glyph, GGO_GLYPH_INDEX | GGO_NATIVE, &glyphMetrics, outlineLength, outline.data(), &identity);

    unsigned offset = 0;
    while (offset < outlineLength) {
        LPTTPOLYGONHEADER subpath = reinterpret_cast<LPTTPOLYGONHEADER>(outline.data() + offset);
        ASSERT(subpath->dwType == TT_POLYGON_TYPE);
        if (subpath->dwType != TT_POLYGON_TYPE)
            return path;

        CGPathMoveToPoint(path, 0, toCGFloat(subpath->pfxStart.x), toCGFloat(subpath->pfxStart.y));

        unsigned subpathOffset = sizeof(*subpath);
        while (subpathOffset < subpath->cb) {
            LPTTPOLYCURVE segment = reinterpret_cast<LPTTPOLYCURVE>(reinterpret_cast<UInt8*>(subpath) + subpathOffset);
            switch (segment->wType) {
                case TT_PRIM_LINE:
                    for (unsigned i = 0; i < segment->cpfx; i++)
                        CGPathAddLineToPoint(path, 0, toCGFloat(segment->apfx[i].x), toCGFloat(segment->apfx[i].y));
                    break;

                case TT_PRIM_QSPLINE:
                    for (unsigned i = 0; i < segment->cpfx; i++) {
                        CGFloat x = toCGFloat(segment->apfx[i].x);
                        CGFloat y = toCGFloat(segment->apfx[i].y);
                        CGFloat cpx;
                        CGFloat cpy;

                        if (i == segment->cpfx - 2) {
                            cpx = toCGFloat(segment->apfx[i + 1].x);
                            cpy = toCGFloat(segment->apfx[i + 1].y);
                            i++;
                        } else {
                            cpx = (toCGFloat(segment->apfx[i].x) + toCGFloat(segment->apfx[i + 1].x)) / 2;
                            cpy = (toCGFloat(segment->apfx[i].y) + toCGFloat(segment->apfx[i + 1].y)) / 2;
                        }

                        CGPathAddQuadCurveToPoint(path, 0, x, y, cpx, cpy);
                    }
                    break;

                case TT_PRIM_CSPLINE:
                    for (unsigned i = 0; i < segment->cpfx; i += 3) {
                        CGFloat cp1x = toCGFloat(segment->apfx[i].x);
                        CGFloat cp1y = toCGFloat(segment->apfx[i].y);
                        CGFloat cp2x = toCGFloat(segment->apfx[i + 1].x);
                        CGFloat cp2y = toCGFloat(segment->apfx[i + 1].y);
                        CGFloat x = toCGFloat(segment->apfx[i + 2].x);
                        CGFloat y = toCGFloat(segment->apfx[i + 2].y);

                        CGPathAddCurveToPoint(path, 0, cp1x, cp1y, cp2x, cp2y, x, y);
                    }
                    break;

                default:
                    ASSERT_NOT_REACHED();
                    return path;
            }

            subpathOffset += sizeof(*segment) + (segment->cpfx - 1) * sizeof(segment->apfx[0]);
        }
        CGPathCloseSubpath(path);
        offset += subpath->cb;
    }
    return path;
}

void FontCascade::drawGlyphs(GraphicsContext* graphicsContext, const Font* font, const GlyphBuffer& glyphBuffer,
    int from, int numGlyphs, const FloatPoint& point) const
{
    CGContextRef cgContext = graphicsContext->platformContext();
    bool shouldUseFontSmoothing = WebCoreShouldUseFontSmoothing();

    switch(fontDescription().fontSmoothing()) {
    case Antialiased: {
        graphicsContext->setShouldAntialias(true);
        shouldUseFontSmoothing = false;
        break;
    }
    case SubpixelAntialiased: {
        graphicsContext->setShouldAntialias(true);
        shouldUseFontSmoothing = true;
        break;
    }
    case NoSmoothing: {
        graphicsContext->setShouldAntialias(false);
        shouldUseFontSmoothing = false;
        break;
    }
    case AutoSmoothing: {
        // For the AutoSmooth case, don't do anything! Keep the default settings.
        break; 
    }
    default: 
        ASSERT_NOT_REACHED();
    }

    uint32_t oldFontSmoothingStyle = wkSetFontSmoothingStyle(cgContext, shouldUseFontSmoothing);

    const FontPlatformData& platformData = font->platformData();

    CGContextSetFont(cgContext, platformData.cgFont());

    CGAffineTransform matrix = CGAffineTransformIdentity;
    matrix.b = -matrix.b;
    matrix.d = -matrix.d;

    if (platformData.syntheticOblique()) {
        static float skew = -tanf(syntheticObliqueAngle * piFloat / 180.0f);
        matrix = CGAffineTransformConcat(matrix, CGAffineTransformMake(1, 0, skew, 1, 0, 0));
    }

    CGContextSetTextMatrix(cgContext, matrix);

    CGContextSetFontSize(cgContext, platformData.size());
    wkSetCGContextFontRenderingStyle(cgContext, font->isSystemFont(), false, font->platformData().useGDI());

    FloatSize shadowOffset;
    float shadowBlur;
    Color shadowColor;
    ColorSpace shadowColorSpace;
    graphicsContext->getShadow(shadowOffset, shadowBlur, shadowColor, shadowColorSpace);

    bool hasSimpleShadow = graphicsContext->textDrawingMode() == TextModeFill && shadowColor.isValid() && !shadowBlur && (!graphicsContext->shadowsIgnoreTransforms() || graphicsContext->getCTM().isIdentityOrTranslationOrFlipped());
    if (hasSimpleShadow) {
        // Paint simple shadows ourselves instead of relying on CG shadows, to avoid losing subpixel antialiasing.
        graphicsContext->clearShadow();
        Color fillColor = graphicsContext->fillColor();
        ColorSpace fillColorSpace = graphicsContext->fillColorSpace();
        Color shadowFillColor(shadowColor.red(), shadowColor.green(), shadowColor.blue(), shadowColor.alpha() * fillColor.alpha() / 255);
        graphicsContext->setFillColor(shadowFillColor, shadowColorSpace);
        float shadowTextX = point.x() + shadowOffset.width();
        // If shadows are ignoring transforms, then we haven't applied the Y coordinate flip yet, so down is negative.
        float shadowTextY = point.y() + shadowOffset.height() * (graphicsContext.shadowsIgnoreTransforms() ? -1 : 1);
        CGContextSetTextPosition(cgContext, shadowTextX, shadowTextY);
        CGContextShowGlyphsWithAdvances(cgContext, glyphBuffer.glyphs(from), static_cast<const CGSize*>(glyphBuffer.advances(from)), numGlyphs);
        if (font->syntheticBoldOffset()) {
            CGContextSetTextPosition(cgContext, point.x() + shadowOffset.width() + font.syntheticBoldOffset(), point.y() + shadowOffset.height());
            CGContextShowGlyphsWithAdvances(cgContext, glyphBuffer.glyphs(from), static_cast<const CGSize*>(glyphBuffer.advances(from)), numGlyphs);
        }
        graphicsContext->setFillColor(fillColor, fillColorSpace);
    }

    CGContextSetTextPosition(cgContext, point.x(), point.y());
    CGContextShowGlyphsWithAdvances(cgContext, glyphBuffer.glyphs(from), static_cast<const CGSize*>(glyphBuffer.advances(from)), numGlyphs);
    if (font->syntheticBoldOffset()) {
        CGContextSetTextPosition(cgContext, point.x() + font.syntheticBoldOffset(), point.y());
        CGContextShowGlyphsWithAdvances(cgContext, glyphBuffer.glyphs(from), static_cast<const CGSize*>(glyphBuffer.advances(from)), numGlyphs);
    }

    if (hasSimpleShadow)
        graphicsContext->setShadow(shadowOffset, shadowBlur, shadowColor, shadowColorSpace);

    wkRestoreFontSmoothingStyle(cgContext, oldFontSmoothingStyle);
}

}

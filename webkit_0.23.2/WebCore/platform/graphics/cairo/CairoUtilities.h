/*
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2011 ProFUSION embedded systems
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

#ifndef CairoUtilities_h
#define CairoUtilities_h

#if USE(CAIRO) || USE(WKC_CAIRO)

#include "GraphicsTypes.h"
#include "IntSize.h"
#include <cairo.h>

// This function was added pretty much simultaneous to when 1.13 was branched.
#define HAVE_CAIRO_SURFACE_SET_DEVICE_SCALE CAIRO_VERSION_MAJOR > 1 || (CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR >= 13)

namespace WebCore {
class AffineTransform;
class Color;
class FloatRect;
class FloatPoint;
class IntSize;
class IntRect;
class Path;

void copyContextProperties(cairo_t* srcCr, cairo_t* dstCr);
void setSourceRGBAFromColor(cairo_t*, const Color&);
void appendPathToCairoContext(cairo_t* to, cairo_t* from);
void setPathOnCairoContext(cairo_t* to, cairo_t* from);
void appendWebCorePathToCairoContext(cairo_t* context, const Path& path);
void appendRegionToCairoContext(cairo_t*, const cairo_region_t*);
cairo_operator_t toCairoOperator(CompositeOperator op);
cairo_operator_t toCairoOperator(BlendMode blendOp);
void drawPatternToCairoContext(cairo_t* cr, cairo_surface_t* image, const IntSize& imageSize, const FloatRect& tileRect,
                               const AffineTransform& patternTransform, const FloatPoint& phase, cairo_operator_t op, const FloatRect& destRect);
PassRefPtr<cairo_surface_t> copyCairoImageSurface(cairo_surface_t*);

void copyRectFromCairoSurfaceToContext(cairo_surface_t* from, cairo_t* to, const IntSize& offset, const IntRect&);
void copyRectFromOneSurfaceToAnother(cairo_surface_t* from, cairo_surface_t* to, const IntSize& offset, const IntRect&, const IntSize& = IntSize(), cairo_operator_t = CAIRO_OPERATOR_OVER);

IntSize cairoSurfaceSize(cairo_surface_t*);
void flipImageSurfaceVertically(cairo_surface_t*);
void cairoSurfaceSetDeviceScale(cairo_surface_t*, double xScale, double yScale);
void cairoSurfaceGetDeviceScale(cairo_surface_t*, double& xScale, double& yScale);

} // namespace WebCore

#endif // USE(CAIRO)

#endif // CairoUtilities_h

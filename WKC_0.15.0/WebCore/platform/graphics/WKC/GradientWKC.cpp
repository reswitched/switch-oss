/*
 * Copyright (C) 2006, 2007, 2008 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (c) 2010-2014 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if !USE(WKC_CAIRO)

#include "Gradient.h"
#include "FloatQuad.h"

#include "CSSParser.h"
#include "GraphicsContext.h"

#include <wkc/wkcgpeer.h>

namespace WebCore {

static void
_getGradientColor(void* self, float val, float* red, float* green, float* blue, float* alpha)
{
    Gradient* iself = (Gradient* )self;
    if (iself) {
        iself->getColor(val, red, green, blue, alpha);
    } else {
        *red = *green = *blue = *alpha = 0.f;
    }
}

static inline bool compareStops(const Gradient::ColorStop& a, const Gradient::ColorStop& b)
{
    return a.stop < b.stop;
}

void Gradient::getColor(float value, float* r, float* g, float* b, float* a) const
{
    ASSERT(value >= 0);
    ASSERT(value <= 1);

    if (m_stops.isEmpty()) {
        *r = 0;
        *g = 0;
        *b = 0;
        *a = 0;
        return;
    }
    if (!m_stopsSorted) {
        if (m_stops.size())
            std::stable_sort(m_stops.begin(), m_stops.end(), compareStops);
        m_stopsSorted = true;
    }
    if (value <= 0 || value <= m_stops.first().stop) {
        *r = m_stops.first().red;
        *g = m_stops.first().green;
        *b = m_stops.first().blue;
        *a = m_stops.first().alpha;
        return;
    }
    if (value >= 1 || value >= m_stops.last().stop) {
        *r = m_stops.last().red;
        *g = m_stops.last().green;
        *b = m_stops.last().blue;
        *a = m_stops.last().alpha;
        return;
    }

    // Find stop before and stop after and interpolate.
    int stop = findStop(value);
    const ColorStop& lastStop = m_stops[stop];    
    const ColorStop& nextStop = m_stops[stop + 1];
    float stopFraction = (value - lastStop.stop) / (nextStop.stop - lastStop.stop);
    *r = lastStop.red + (nextStop.red - lastStop.red) * stopFraction;
    *g = lastStop.green + (nextStop.green - lastStop.green) * stopFraction;
    *b = lastStop.blue + (nextStop.blue - lastStop.blue) * stopFraction;
    *a = lastStop.alpha + (nextStop.alpha - lastStop.alpha) * stopFraction;
}

int Gradient::findStop(float value) const
{
    static int m_lastStop = 0;

    ASSERT(value >= 0);
    ASSERT(value <= 1);
    ASSERT(m_stopsSorted);

    int numStops = m_stops.size();
    ASSERT(numStops >= 2);
    ASSERT(m_lastStop < numStops - 1);

    int i = m_lastStop;
    if (value < m_stops[i].stop)
        i = 1;
    else
        i = m_lastStop + 1;

    for (; i < numStops - 1; ++i)
        if (value < m_stops[i].stop)
            break;

    m_lastStop = i - 1;
    return m_lastStop;
}

PlatformGradient
Gradient::platformGradient()
{
    WKC_DEFINE_STATIC_PTR(WKCPeerPattern*, pattern, new WKCPeerPattern);

    const FloatPoint p0 = gradientSpaceTransform().mapPoint(m_p0);
    const FloatPoint p1 = gradientSpaceTransform().mapPoint(m_p1);
    const float p0x = p0.x();
    const float p0y = p0.y();
    const float p1x = p1.x();
    const float p1y = p1.y();

    pattern->fType = WKC_PATTERN_GRADIENT;
    pattern->u.fGradient.fSelf = this;
    pattern->u.fGradient.fRadial = m_radial;
    WKCFloatPoint_Set(&pattern->u.fGradient.fPoint0, p0x, p0y);
    WKCFloatPoint_Set(&pattern->u.fGradient.fPoint1, p1x, p1y);
    const int stops = WKC_MIN(m_stops.size(), WKCPEERGRADIENT_MAXSTOPS);
    pattern->u.fGradient.fStops = stops;
    int i=0;
    for (i=0; i<stops; i++)
        pattern->u.fGradient.fStop[i] = m_stops[i].stop;
    for (;i<WKCPEERGRADIENT_MAXSTOPS; i++)
        pattern->u.fGradient.fStop[i] = 0;
    pattern->u.fGradient.fGetColorProc = _getGradientColor;

    if (m_radial) {
        const float xd = p1x-p0x;
        const float yd = p1y-p0y;
        const float r0 = m_r0 * gradientSpaceTransform().xScale();
        const float r1 = m_r1 * gradientSpaceTransform().xScale();
        float rd = r1-r0;
        const float m0 = (xd*xd+yd*yd-rd*rd);
        if (m0==0.f) {
            rd += 0.01f;
        }
        pattern->u.fGradient.fMatrix[0] = (xd*xd+yd*yd-rd*rd);
        pattern->u.fGradient.fMatrix[1] = xd*p0x+yd*p0y-rd*r0;
        pattern->u.fGradient.fMatrix[2] = p0x*p0x+p0y*p0y-r0*r0;
        pattern->u.fGradient.fMatrix[3] = 1.f / pattern->u.fGradient.fMatrix[0];
        pattern->u.fGradient.fR0 = m_r0;
        pattern->u.fGradient.fR1 = m_r1;
    } else {
        if (p0x!=p1x && p0y!=p1y) {
            const float k = 1.f/(p1x-p0x);
            const float div = -k * (p0y-p1y);
            const float im = 1.f/(div*div+1);
            const float nn = div*div*p0x - p0y*div;
            pattern->u.fGradient.fMatrix[0] = k*div*im;
            pattern->u.fGradient.fMatrix[1] = k*im;
            pattern->u.fGradient.fMatrix[2] = k*(nn*im-p0x);
        } else if (p0y!=p1y) {
            pattern->u.fGradient.fMatrix[0] = 1/(p1y-p0y);
            pattern->u.fGradient.fMatrix[1] = 0;
            pattern->u.fGradient.fMatrix[2] = -p0y;
        } else if (p0x!=p1x) {
            pattern->u.fGradient.fMatrix[0] = 0;
            pattern->u.fGradient.fMatrix[1] = 1/(p1x-p0x);
            pattern->u.fGradient.fMatrix[2] = -p0x;
        } else {
            return 0;
        }
    }

    return pattern;
}

void Gradient::platformDestroy()
{
    // nothing to do
}

void Gradient::fill(GraphicsContext* context, const FloatRect& rect)
{
    PlatformGraphicsContext* ctx = context->platformContext();
    if (!ctx)
        return;
    const AffineTransform m(context->getCTM());
    const FloatQuad quad = m.mapQuad(rect);
    if (quad.isEmpty())
        return;
    WKCFloatPoint p[6];
    WKCFloatPoint_Set(&p[0], quad.p1().x(), quad.p1().y());
    WKCFloatPoint_Set(&p[1], quad.p2().x(), quad.p2().y());
    WKCFloatPoint_Set(&p[2], quad.p3().x(), quad.p3().y());
    WKCFloatPoint_Set(&p[3], quad.p4().x(), quad.p4().y());
    WKCFloatPoint_SetPoint(&p[4], &p[0]);
    WKCFloatPoint_Set(&p[5], FLT_MIN, FLT_MIN);

    WKCPeerPattern* pt = (WKCPeerPattern *)platformGradient();
    if (!pt)
        return;

    context->save();
    wkcDrawContextSetFillColorPeer(ctx, (0xff)<<24);
    wkcDrawContextSetPatternPeer(ctx, pt);
    wkcDrawContextDrawPolygonPeer(ctx, 6, p);
    wkcDrawContextSetPatternPeer(ctx, 0);
    context->restore();
}

void Gradient::setPlatformGradientSpaceTransform(const AffineTransform& gradientSpaceTransformation)
{
    if (m_gradient) {
#if 0
        // from GradiantCairo.cpp
        cairo_matrix_t matrix = gradientSpaceTransformation;
        cairo_matrix_invert(&matrix);
        cairo_pattern_set_matrix(m_gradient, &matrix);
#endif
    }
}


} //namespace

#endif /* !USE(WKC_CAIRO) */

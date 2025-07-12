/*
 * Copyright (C) 2007 Kevin Ollivier <kevino@theolliviers.com>
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

#include "GraphicsContext.h"

#include "TransformationMatrix.h"
#include "ImageBuffer.h"
#include "ShadowBlur.h"
#include "FloatRect.h"
#include "Font.h"
#include "Gradient.h"
#include "ImageObserver.h"
#include "ImageWKC.h"
#include "IntRect.h"
#include "FloatQuad.h"
#include "Pattern.h"
#include "PlatformPathWKC.h"
#include <wtf/MathExtras.h>

#include <wkc/wkcpeer.h>
#include <wkc/wkcgpeer.h>

#include "NotImplemented.h"

#define X_CSS_SHADOW_IMPL 1

namespace WebCore {

class GraphicsContextPlatformPrivateData {
public:
    GraphicsContextPlatformPrivateData()
        : m_opacity(1.0)
        , m_clip_context(C_INVALID)
        , m_clip_transform()
        , m_transform()
        , m_itransform()
        {}
    virtual ~GraphicsContextPlatformPrivateData() {}

    float m_opacity;
    enum m_context_type {
        C_INVALID = 0,
        C_UNKNOWN = 1,
        C_CANVAS_2D = 2,
    };
    enum m_context_type m_clip_context;
    Path m_clip;
    AffineTransform m_clip_transform;
    AffineTransform m_transform;
    AffineTransform m_itransform;
};

class GraphicsContextPlatformPrivate : public GraphicsContextPlatformPrivateData {
public:
    GraphicsContextPlatformPrivate()
      : m_drawcontext(NULL) {}
    virtual ~GraphicsContextPlatformPrivate()
    {
        while (!m_backupData.isEmpty())
            restore(false);
    };

    void save()
    {
        if (m_drawcontext) {
            wkcDrawContextSaveStatePeer(m_drawcontext);
        }
        m_backupData.append(*static_cast<GraphicsContextPlatformPrivateData*>(this));
    }

    void restore(bool restoreclip)
    {
        if (m_backupData.isEmpty())
            return;

        if (m_drawcontext) {
            wkcDrawContextRestoreStatePeer(m_drawcontext);
        }

        GraphicsContextPlatformPrivateData::operator=(m_backupData.last());
        m_backupData.removeLast();

        if (restoreclip && m_drawcontext) {
            PlatformPathWKC* pp = (PlatformPathWKC *)m_clip.platformPath();
            if (C_CANVAS_2D == m_clip_context) {
                wkcDrawContextClearClipPolygonPeer(m_drawcontext);
                wkcDrawContextCanvasClipPathBeginPeer(m_drawcontext);
            } else {
                wkcDrawContextClearClipPolygonPeer(m_drawcontext);
            }
            pp->clipPath(m_drawcontext, &m_clip_transform);
            if (C_CANVAS_2D == m_clip_context) {
                wkcDrawContextCanvasClipPathEndPeer(m_drawcontext);
            } else {
                /*EMPTY*/
            }
        }
    }

    bool mapRect(const FloatRect& rect, WKCFloatPoint* p)
    {
        FloatQuad q = m_transform.mapQuad(rect);
        FloatRect br = q.boundingBox();
        int d = 0;

        if (q.isEmpty()) {
            return false;
        }
        if (br.width() < 1.f || br.height() < 1.f) {
            return false;
        }

        p[0].fX = q.p1().x();
        p[0].fY = q.p1().y();
        p[1].fX = q.p2().x();
        p[1].fY = q.p2().y();
        p[2].fX = q.p3().x();
        p[2].fY = q.p3().y();
        p[3].fX = q.p4().x();
        p[3].fY = q.p4().y();

        d = (p[1].fX-p[0].fX)*(p[2].fY-p[0].fY) - (p[2].fX-p[0].fX) * (p[1].fY-p[0].fY);
        if (d==0) {
            return false;
        }
        d = (p[3].fX-p[0].fX)*(p[2].fY-p[0].fY) - (p[2].fX-p[0].fX) * (p[3].fY-p[0].fY);
        if (d==0) {
            return false;
        }
        return true;
    }

public:
    void* m_drawcontext;
    Vector<GraphicsContextPlatformPrivateData> m_backupData;
};

void
GraphicsContext::platformInit(PlatformGraphicsContext* context)
{
    m_data = new GraphicsContextPlatformPrivate;
    setPaintingDisabled(!context);
    m_data->m_drawcontext = (void *)context;
    if (context) {
        // Make sure the context starts in sync with our state.
        setPlatformFillColor(fillColor(), ColorSpaceDeviceRGB);
        setPlatformStrokeColor(strokeColor(), ColorSpaceDeviceRGB);
    }
}

void
GraphicsContext::platformDestroy()
{
    if (m_data && m_data->m_drawcontext) {
        wkcDrawContextFlushPeer(m_data->m_drawcontext);
    }
    delete m_data;
}

PlatformGraphicsContext* GraphicsContext::platformContext() const
{
    return (PlatformGraphicsContext *)m_data->m_drawcontext;
}

void GraphicsContext::savePlatformState()
{
    m_data->save();
}

void GraphicsContext::restorePlatformState()
{
    m_data->restore(true);
}

static inline unsigned int platformColor(const Color& color)
{
    return (color.alpha()<<24) | (color.red()<<16) | (color.green()<<8) | (color.blue());
}

static inline int platformStyle(StrokeStyle style)
{
    switch (style) {
    case NoStroke:
        return WKC_STROKESTYLE_NO;
    case SolidStroke:
        return WKC_STROKESTYLE_SOLID;
    case DottedStroke:
        return WKC_STROKESTYLE_DOTTED;
    case DashedStroke:
        return WKC_STROKESTYLE_DASHED;
    }
    return 0;
}

static inline int platformTextDrawingMode(TextDrawingModeFlags mode)
{
    int ret = WKC_FONT_DRAWINGMODE_INVISIBLE;
    if (mode&TextModeFill)
        ret |= WKC_FONT_DRAWINGMODE_FILL;
    if (mode&TextModeStroke)
        ret |= WKC_FONT_DRAWINGMODE_STROKE;
#if 0
    if (mode&TextModeClip)
        ret |= WKC_FONT_DRAWINGMODE_CLIP;
#endif
    return ret;
}

static inline void platformPoint(const FloatPoint& in, WKCFloatPoint& out)
{
    out.fX = in.x();
    out.fY = in.y();
}

static inline void platformPoint(const IntPoint& in, WKCFloatPoint& out)
{
    out.fX = in.x();
    out.fY = in.y();
}

static inline void platformRect(const FloatRect& in, WKCFloatRect& out)
{
    out.fX = in.x();
    out.fY = in.y();
    out.fWidth = in.width();
    out.fHeight = in.height();
}

static inline void platformRect(const IntRect& in, WKCFloatRect& out)
{
    out.fX = in.x();
    out.fY = in.y();
    out.fWidth = in.width();
    out.fHeight = in.height();
}

static inline void platformSize(const IntSize& in, WKCFloatSize& out)
{
    out.fWidth = in.width();
    out.fHeight = in.height();
}

// Draws a filled rectangle with a stroked border.
void GraphicsContext::drawRect(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    WKCFloatRect r;
    platformRect(rect, r);
    wkcDrawContextDrawRectPeer(m_data->m_drawcontext, &r);
}

// This is only used to draw borders.
void GraphicsContext::drawLine(const IntPoint& point1, const IntPoint& point2)
{
    if (paintingDisabled())
        return;

    WKCFloatPoint p1, p2;
    platformPoint(point1, p1);
    platformPoint(point2, p2);
    wkcDrawContextSetPenStylePeer(m_data->m_drawcontext, platformColor(strokeColor()), strokeThickness(), platformStyle(strokeStyle()));
    wkcDrawContextDrawLinePeer(m_data->m_drawcontext, &p1, &p2);
}

// This method is only used to draw the little circles used in lists.
void GraphicsContext::drawEllipse(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    WKCFloatRect r;
    platformRect(rect, r);
    wkcDrawContextSetPenStylePeer(m_data->m_drawcontext, platformColor(strokeColor()), strokeThickness(), platformStyle(strokeStyle()));
    wkcDrawContextDrawEllipsePeer(m_data->m_drawcontext, &r);
}

#if 0
void GraphicsContext::strokeArc(const IntRect& rect, int startAngle, int angleSpan)
{
    if (paintingDisabled())
        return;

    WKCFloatRect r;
    platformRect(rect, r);
    wkcDrawContextSetPenStylePeer(m_data->m_drawcontext, platformColor(strokeColor()), strokeThickness(), platformStyle(strokeStyle()));
    wkcDrawContextStrokeArcPeer(m_data->m_drawcontext, &r, startAngle, angleSpan);
}
#endif

void GraphicsContext::drawConvexPolygon(size_t npoints, const FloatPoint* points, bool shouldAntialias)
{
    WKCFloatPoint* p = 0;
    WKCFloatPoint quad[4];
    bool allocated = false;

    if (paintingDisabled())
        return;

    if (npoints <= 1)
        return;

    if (npoints<=4) {
        p = quad;
        allocated = false;
    } else {
        p = new WKCFloatPoint[npoints];
        if (!p) return;
        allocated = true;
    }
    for (size_t i=0; i<npoints; i++) {
        platformPoint(points[i], p[i]);
    }

    wkcDrawContextSetPenStylePeer(m_data->m_drawcontext, platformColor(strokeColor()), strokeThickness(), platformStyle(strokeStyle()));
    wkcDrawContextDrawConvexPolygonPeer(m_data->m_drawcontext, npoints, p, shouldAntialias ? 1:0);

    if (allocated) {
        delete [] p;
    }
}

static void drawBoxShadow(GraphicsContext* context, const FloatRect& rect, const FloatSize& offset, const float blur, const Color& color, const IntSize& topLeft=IntSize(), const IntSize& topRight=IntSize(), const IntSize& bottomLeft=IntSize(), const IntSize& bottomRight=IntSize())
{
    void* dc = context->platformContext();
    if (wkcDrawContextGetOffscreenTypePeer(dc)==WKC_OFFSCREEN_TYPE_POLYGON)
        return;

    // skip the shadowing if the shadowed area is too big (bigger than 150x150)
    // the shadowing can be very slow for very big rectangles, e.g. 1600ms for 1000x1800
    if ( (rect.width()*rect.height()) > (150*150) )
        return;

    ShadowBlur shadow(FloatSize(blur, blur), offset, color, ColorSpaceDeviceRGB);
    
    const IntSize bufferSize(rect.width()+blur*2, rect.height()+blur*2);
    const FloatRect bufferRect(FloatPoint(), bufferSize);
    OwnPtr<ImageBuffer> shadowBuffer = ImageBuffer::create(bufferSize);
    if (!shadowBuffer) return;
    GraphicsContext *shadowContext = shadowBuffer->context();

    shadowContext->clearRect(bufferRect);
    shadowContext->setFillColor(Color::black, ColorSpaceDeviceRGB);
    IntRect shadowRect(blur, blur, rect.width(), rect.height());
    if (!topLeft.isEmpty() || !topRight.isEmpty() || !bottomLeft.isEmpty() || !bottomRight.isEmpty()) {
        shadowContext->fillRoundedRect(shadowRect, topLeft, topRight, bottomLeft, bottomRight, Color::black, ColorSpaceDeviceRGB);
    } else {
        shadowContext->fillRect(shadowRect);
    }

    IntRect br(bufferRect);
    RefPtr<Uint8ClampedArray> layerData = shadowBuffer->getUnmultipliedImageData(br);
    if (!layerData) return;
    shadow.blurLayerImage(layerData->data(), bufferSize, bufferSize.width() * 4);
    shadowBuffer->putByteArray(Unmultiplied, layerData.get(), bufferSize, br, IntPoint());

    shadowContext->setCompositeOperation(CompositeSourceIn);
    shadowContext->setFillColor(color, ColorSpaceDeviceRGB);
    shadowContext->fillRect(bufferRect);

    context->drawImageBuffer(shadowBuffer.get(), ColorSpaceDeviceRGB, IntPoint(rect.x()+offset.width()-blur, rect.y()+offset.height()-blur));
}

static void drawPathShadow(GraphicsContext* context, const Path& path, const FloatSize& offset, const float blur, const Color& color)
{
    void* dc = context->platformContext();
    if (wkcDrawContextGetOffscreenTypePeer(dc)==WKC_OFFSCREEN_TYPE_POLYGON)
        return;

    ShadowBlur shadow(FloatSize(blur, blur), offset, color, ColorSpaceDeviceRGB);
    Path shadowPath(path);
    FloatRect bounds(shadowPath.boundingRect());
    shadowPath.translate(FloatSize(blur-bounds.x(), blur-bounds.y()));

    const IntSize bufferSize(bounds.width()+blur*2, bounds.height()+blur*2);
    const FloatRect bufferRect(FloatPoint(), bufferSize);

    OwnPtr<ImageBuffer> shadowBuffer = ImageBuffer::create(bufferSize);
    if (!shadowBuffer) return;
    GraphicsContext *shadowContext = shadowBuffer->context();

    shadowContext->clearRect(bufferRect);
    shadowContext->setFillColor(Color::black, ColorSpaceDeviceRGB);

    shadowContext->fillPath(shadowPath);

    IntRect br(bufferRect);
    RefPtr<Uint8ClampedArray> layerData = shadowBuffer->getUnmultipliedImageData(br);
    if (!layerData) return;
    shadow.blurLayerImage(layerData->data(), bufferSize, bufferSize.width() * 4);
    shadowBuffer->putByteArray(Unmultiplied, layerData.get(), bufferSize, br, IntPoint());

    shadowContext->setCompositeOperation(CompositeSourceIn);
    shadowContext->setFillColor(color, ColorSpaceDeviceRGB);
    shadowContext->fillRect(bufferRect);

    context->drawImageBuffer(shadowBuffer.get(), ColorSpaceDeviceRGB, IntPoint(bounds.x()+offset.width()-blur, bounds.y()+offset.height()-blur));
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    int type = 0;
    type = wkcDrawContextGetOffscreenTypePeer(m_data->m_drawcontext);

    m_data->save();
    if (type==WKC_OFFSCREEN_TYPE_IMAGEBUF) {
        WKCFloatPoint p[6];
        if (m_data->mapRect(rect, p)) {
            setPlatformFillColor(color, colorSpace);
            WKCFloatPoint_SetPoint(&p[4], &p[0]);
            WKCFloatPoint_Set(&p[5], FLT_MIN, FLT_MIN);
            wkcDrawContextDrawPolygonPeer(m_data->m_drawcontext, 6, p);
        }
    } else {
        if (hasShadow()) {
            if (!m_state.shadowBlur) {
                // Optimize non-blurry shadows, drawing without shadowBuffer.
                WKCFloatRect r;
                platformRect(rect, r);
                r.fX += m_state.shadowOffset.width();
                r.fY += m_state.shadowOffset.height();
                setPlatformStrokeStyle(NoStroke);
                wkcDrawContextFillRectPeer(m_data->m_drawcontext, &r, platformColor(m_state.shadowColor));
            } else {
#if 0 == X_CSS_SHADOW_IMPL
                drawBoxShadow(this, rect, m_state.shadowOffset, m_state.shadowBlur, m_state.shadowColor);
#elif 1 == X_CSS_SHADOW_IMPL
                IntRect shadow_edge_rect(rect.x() + m_state.shadowOffset.width(),
                                         rect.y() + m_state.shadowOffset.height(),
                                         rect.width(),
                                         rect.height() );
                WKCFloatRect shadow_edge_r;
                platformRect(shadow_edge_rect, shadow_edge_r);
                WKCFloatSize radii[4];
                WKCFloatSize_Set(&radii[0], 0.0f, 0.0f);
                WKCFloatSize_Set(&radii[1], 0.0f, 0.0f);
                WKCFloatSize_Set(&radii[2], 0.0f, 0.0f);
                WKCFloatSize_Set(&radii[3], 0.0f, 0.0f);
                WKCFloatSize shadow_offset;
                WKCFloatSize_Set(&shadow_offset, m_state.shadowOffset.width(), m_state.shadowOffset.height());
                float shadow_blur = m_state.shadowBlur;
                unsigned int shadow_color = platformColor(m_state.shadowColor);

                m_data->save();

                Path path;
                path.addRect(shadow_edge_rect);

                wkcDrawContextSetShadowPeer(m_data->m_drawcontext, 2, &shadow_edge_r, NULL, radii,
                                            &shadow_offset, shadow_blur, shadow_color, true );
                /*
                 * fillPath() modifies the lower layer context to draw the original shape.
                 * we cannot use it because we have to modify lower layer context to draw the shadow shape.
                 */
                {
                    wkcDrawContextSetPatternPeer(m_data->m_drawcontext, NULL);
                    wkcDrawContextSetDrawAccuratePeer(m_data->m_drawcontext, true);
                    int savedFillRule = wkcDrawContextGetFillRulePeer(m_data->m_drawcontext);
                    int x_fillRule = WKC_FILLRULE_WINDING;
                    if (fillRule() == RULE_EVENODD) {
                        x_fillRule = WKC_FILLRULE_EVENODD;
                    }
                    wkcDrawContextSetFillRulePeer(m_data->m_drawcontext, x_fillRule);
                    m_data->save();
                    PlatformPathWKC* pp = (PlatformPathWKC *)path.platformPath();
                    pp->fillPath(m_data->m_drawcontext, &m_data->m_transform);
                    m_data->restore(false);
                    wkcDrawContextSetFillRulePeer(m_data->m_drawcontext, savedFillRule);
                    wkcDrawContextSetDrawAccuratePeer(m_data->m_drawcontext, false);
                }
                wkcDrawContextClearShadowPeer(m_data->m_drawcontext);

                m_data->restore(false);
#endif /* X_CSS_SHADOW_IMPL */
            }
        } else { // ToDo check: When webkit(r89860) drawing box-shadow, 'rect' is laid out of clipped area and so no need to draw/fill.
            WKCFloatRect r;
            platformRect(rect, r);
            // FIXME: Because drawRect already fills a rect and draws its border, we make sure border is not painted here.
            setPlatformStrokeStyle(NoStroke);
            wkcDrawContextFillRectPeer(m_data->m_drawcontext, &r, platformColor(color));
        }
    }
    m_data->restore(false);
}

void GraphicsContext::fillRoundedRect(const IntRect& rect, const IntSize& topLeft, const IntSize& topRight, const IntSize& bottomLeft, const IntSize& bottomRight, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    m_data->save();

    Path path;
    path.addRoundedRect(rect, topLeft, topRight, bottomLeft, bottomRight);
    setPlatformFillColor(platformColor(color), colorSpace);
    fillPath(path);

    m_data->restore(false);
}

/*
 * see the comment in ./webkit/WebCore/platform/graphics/cg/GraphicsContextCG.cpp::fillRectWithRoundedHole().
 * we now share the same assumption.
 */
void GraphicsContext::fillRectWithRoundedHole(const IntRect& rect, const RoundedRect& roundedHoleRect, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    Path path;
    path.addRect(rect);

    if (!roundedHoleRect.radii().isZero())
        path.addRoundedRect(roundedHoleRect);
    else
        path.addRect(roundedHoleRect.rect());

    WindRule oldFillRule = fillRule();
    Color oldFillColor = fillColor();
    ColorSpace oldFillColorSpace = fillColorSpace();
    
    setFillRule(RULE_EVENODD);
    setFillColor(color, colorSpace);

    if (hasShadow()) {
        path.translate(FloatSize(m_state.shadowOffset.width(), m_state.shadowOffset.height()));
        IntRect shadow_edge_rect(rect.x() + m_state.shadowOffset.width(),
                                 rect.y() + m_state.shadowOffset.height(),
                                 rect.width(),
                                 rect.height() );
        WKCFloatRect shadow_edge_r;
        platformRect(shadow_edge_rect, shadow_edge_r);
        IntRect shadow_hole_rect(roundedHoleRect.rect().x() + m_state.shadowOffset.width(),
                                 roundedHoleRect.rect().y() + m_state.shadowOffset.height(),
                                 roundedHoleRect.rect().width(),
                                 roundedHoleRect.rect().height() );
        WKCFloatRect shadow_hole_r;
        platformRect(shadow_hole_rect, shadow_hole_r);
        WKCFloatSize radii[4];
        IntSize topLeft = roundedHoleRect.radii().topLeft();
        IntSize topRight = roundedHoleRect.radii().topRight();
        IntSize bottomLeft = roundedHoleRect.radii().bottomLeft();
        IntSize bottomRight = roundedHoleRect.radii().bottomRight();
        WKCFloatSize_Set(&radii[0], topLeft.width(), topLeft.height());
        WKCFloatSize_Set(&radii[1], topRight.width(), topRight.height());
        WKCFloatSize_Set(&radii[2], bottomRight.width(), bottomRight.height());
        WKCFloatSize_Set(&radii[3], bottomLeft.width(), bottomLeft.height());
        WKCFloatSize shadow_offset;
        WKCFloatSize_Set(&shadow_offset, m_state.shadowOffset.width(), m_state.shadowOffset.height());
        float shadow_blur = m_state.shadowBlur;
        unsigned int shadow_color = platformColor(m_state.shadowColor);

        m_data->save();

        wkcDrawContextSetShadowPeer(m_data->m_drawcontext, 2, &shadow_edge_r, &shadow_hole_r, radii,
                                    &shadow_offset, shadow_blur, shadow_color, true );
        /*
         * fillPath() modifies the lower layer context to draw the original shape.
         * we cannot use it because we have to modify lower layer context to draw the shadow shape.
         */
        {
            wkcDrawContextSetPatternPeer(m_data->m_drawcontext, NULL);
            wkcDrawContextSetDrawAccuratePeer(m_data->m_drawcontext, true);
            int savedFillRule = wkcDrawContextGetFillRulePeer(m_data->m_drawcontext);
            int x_fillRule = WKC_FILLRULE_WINDING;
            if (fillRule() == RULE_EVENODD) {
                x_fillRule = WKC_FILLRULE_EVENODD;
            }
            wkcDrawContextSetFillRulePeer(m_data->m_drawcontext, x_fillRule);
            m_data->save();
            PlatformPathWKC* pp = (PlatformPathWKC *)path.platformPath();
            pp->fillPath(m_data->m_drawcontext, &m_data->m_transform);
            m_data->restore(false);
            wkcDrawContextSetFillRulePeer(m_data->m_drawcontext, savedFillRule);
            wkcDrawContextSetDrawAccuratePeer(m_data->m_drawcontext, false);
        }
        wkcDrawContextClearShadowPeer(m_data->m_drawcontext);

        m_data->restore(false);

        setFillRule(oldFillRule);
        setFillColor(oldFillColor, oldFillColorSpace);
        return;
    }

    fillPath(path);
    
    setFillRule(oldFillRule);
    setFillColor(oldFillColor, oldFillColorSpace);
}

void GraphicsContext::drawFocusRing(const Vector<IntRect>& rects, int width, int /*offset*/, const Color& color)
{
    if (paintingDisabled())
        return;

    // currently, no other implementations supports offset...

    int count = rects.size();
    if (!count) return;

    m_data->save();

    wkcDrawContextSetPenStylePeer(m_data->m_drawcontext, platformColor(color), width, WKC_STROKESTYLE_SOLID);
    wkcDrawContextSetFillColorPeer(m_data->m_drawcontext,0);

    FloatRect fr;
    for (int i=0; i<count; i++) {
        fr.unite(rects[i]);
    }
    fr.inflate(width);
    WKCFloatRect r = {0};
    platformRect(fr, r);
    wkcDrawContextDrawRectPeer(m_data->m_drawcontext, &r);

    m_data->restore(false);
}

void GraphicsContext::drawFocusRing(const Path& paths, int width, int offset, const Color& color)
{
    if (paintingDisabled())
        return;

    PlatformPathWKC* pp = (PlatformPathWKC *)paths.platformPath();

    m_data->save();
    wkcDrawContextSetPenStylePeer(m_data->m_drawcontext, platformColor(color), width, WKC_STROKESTYLE_DOTTED);
    pp->strokePath(m_data->m_drawcontext, &m_data->m_transform);
    m_data->restore(false);
}

void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    WKCFloatRect r;
    platformRect(rect, r);
    wkcDrawContextClipPeer(m_data->m_drawcontext, &r);
}

void GraphicsContext::clipOut(const Path& path)
{
    if (paintingDisabled())
        return;

    if (path.isEmpty())
        return;
    PlatformPathWKC* pp = (PlatformPathWKC *)path.platformPath();
    pp->clipOutPath(m_data->m_drawcontext, &m_data->m_transform);
}

void GraphicsContext::clipOut(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    WKCFloatRect r;
    platformRect(rect, r);
    wkcDrawContextClipOutRectPeer(m_data->m_drawcontext, &r);
}

void GraphicsContext::drawLineForText(const FloatPoint& origin, float width, bool printing)
{
    if (paintingDisabled())
        return;

    if (hasShadow()) {
        const FloatRect rect(origin, FloatSize(width, strokeThickness()));
        if (!m_state.shadowBlur) {
            // Optimize non-blurry shadows, drawing without shadowBuffer.
            WKCFloatRect r;
            platformRect(rect, r);
            r.fX += m_state.shadowOffset.width();
            r.fY += m_state.shadowOffset.height();
            setPlatformStrokeStyle(NoStroke);
            wkcDrawContextFillRectPeer(m_data->m_drawcontext, &r, platformColor(m_state.shadowColor));
        } else {
            drawBoxShadow(this, rect, m_state.shadowOffset, m_state.shadowBlur, m_state.shadowColor);
        }
    }

    WKCFloatPoint p;
    platformPoint(origin, p);
    // FIXME: Paint the underline of composition string with solid stroke.
    // That might have a bad effect if text-decoration-style property in CSS3 were implemented into WebCore.
    setPlatformStrokeStyle(SolidStroke);
    wkcDrawContextDrawLineForTextPeer(m_data->m_drawcontext, &p, width, printing ? 1:0);
}

void GraphicsContext::drawLineForDocumentMarker(const FloatPoint&, float width, DocumentMarkerLineStyle)
{
    if (paintingDisabled())
        return;
    // Ugh!: support it!
    // 110621 ACCESS Co.,Ltd.
    notImplemented();
}

void GraphicsContext::clip(const Path& path, WindRule)
{
    if (paintingDisabled())
        return;
    if (path.isEmpty()) {
        clip(FloatRect());
        return;
    }
    m_data->m_clip_context = GraphicsContextPlatformPrivateData::C_UNKNOWN;
    m_data->m_clip = path;
    m_data->m_clip_transform = m_data->m_transform;
    PlatformPathWKC* pp = (PlatformPathWKC *)path.platformPath();
    wkcDrawContextClearClipPolygonPeer(m_data->m_drawcontext);
    pp->clipPath(m_data->m_drawcontext, &m_data->m_transform);
}

void GraphicsContext::canvasClip(const Path& path, WindRule)
{
    if (paintingDisabled())
        return;
    if (path.isEmpty()) {
        clip(FloatRect());
        return;
    }
    m_data->m_clip_context = GraphicsContextPlatformPrivateData::C_CANVAS_2D;
    m_data->m_clip = path;
    m_data->m_clip_transform = m_data->m_transform;
    PlatformPathWKC* pp = (PlatformPathWKC *)path.platformPath();
    wkcDrawContextCanvasClipPathBeginPeer(m_data->m_drawcontext);
    pp->clipPath(m_data->m_drawcontext, &m_data->m_transform);
    wkcDrawContextCanvasClipPathEndPeer(m_data->m_drawcontext);
}

void GraphicsContext::clipConvexPolygon(size_t numPoints, const FloatPoint*, bool antialias)
{
    // Ugh!: support it!
    // 110621 ACCESS Co.,Ltd.
    notImplemented();
}

AffineTransform GraphicsContext::getCTM(IncludeDeviceScale) const
{ 
    return m_data->m_transform;
}

void GraphicsContext::setCTM(const AffineTransform& m)
{ 
    m_data->m_transform = m;
    m_data->m_itransform = m_data->m_transform.inverse();
    wkcDrawContextSetMatrixPeer(m_data->m_drawcontext, m_data->m_transform.a(), m_data->m_transform.b(), m_data->m_transform.c(), m_data->m_transform.d(), m_data->m_transform.e(), m_data->m_transform.f());
    wkcDrawContextSetInvertMatrixPeer(m_data->m_drawcontext, m_data->m_itransform.a(), m_data->m_itransform.b(), m_data->m_itransform.c(), m_data->m_itransform.d(), m_data->m_itransform.e(), m_data->m_itransform.f());
}

void GraphicsContext::translate(float tx, float ty) 
{
    m_data->m_transform.translate(tx, ty);
    m_data->m_itransform = m_data->m_transform.inverse();
    wkcDrawContextSetMatrixPeer(m_data->m_drawcontext, m_data->m_transform.a(), m_data->m_transform.b(), m_data->m_transform.c(), m_data->m_transform.d(), m_data->m_transform.e(), m_data->m_transform.f());
    wkcDrawContextSetInvertMatrixPeer(m_data->m_drawcontext, m_data->m_itransform.a(), m_data->m_itransform.b(), m_data->m_itransform.c(), m_data->m_itransform.d(), m_data->m_itransform.e(), m_data->m_itransform.f());
}

void GraphicsContext::rotate(float angle) 
{ 
    m_data->m_transform.rotate(rad2deg(angle));
    m_data->m_itransform = m_data->m_transform.inverse();
    wkcDrawContextSetMatrixPeer(m_data->m_drawcontext, m_data->m_transform.a(), m_data->m_transform.b(), m_data->m_transform.c(), m_data->m_transform.d(), m_data->m_transform.e(), m_data->m_transform.f());
    wkcDrawContextSetInvertMatrixPeer(m_data->m_drawcontext, m_data->m_itransform.a(), m_data->m_itransform.b(), m_data->m_itransform.c(), m_data->m_itransform.d(), m_data->m_itransform.e(), m_data->m_itransform.f());
}

void GraphicsContext::scale(const FloatSize& scale) 
{
    m_data->m_transform.scaleNonUniform(scale.width(), scale.height());
    m_data->m_itransform = m_data->m_transform.inverse();
    wkcDrawContextSetMatrixPeer(m_data->m_drawcontext, m_data->m_transform.a(), m_data->m_transform.b(), m_data->m_transform.c(), m_data->m_transform.d(), m_data->m_transform.e(), m_data->m_transform.f());
    wkcDrawContextSetInvertMatrixPeer(m_data->m_drawcontext, m_data->m_itransform.a(), m_data->m_itransform.b(), m_data->m_itransform.c(), m_data->m_itransform.d(), m_data->m_itransform.e(), m_data->m_itransform.f());
}

void GraphicsContext::concatCTM(const AffineTransform& transform)
{
    m_data->m_transform *= transform;
    m_data->m_itransform = m_data->m_transform.inverse();
    wkcDrawContextSetMatrixPeer(m_data->m_drawcontext, m_data->m_transform.a(), m_data->m_transform.b(), m_data->m_transform.c(), m_data->m_transform.d(), m_data->m_transform.e(), m_data->m_transform.f());
    wkcDrawContextSetInvertMatrixPeer(m_data->m_drawcontext, m_data->m_itransform.a(), m_data->m_itransform.b(), m_data->m_itransform.c(), m_data->m_itransform.d(), m_data->m_itransform.e(), m_data->m_itransform.f());
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& frect, RoundingMode mode)
{
    FloatRect result;
    double x = frect.x();
    double y = frect.y();
    x = round(x);
    y = round(y);
    result.setX(static_cast<float>(x));
    result.setY(static_cast<float>(y));
    x = frect.width();
    y = frect.height();
    x = round(x);
    y = round(y);
    result.setWidth(static_cast<float>(x));
    result.setHeight(static_cast<float>(y));
    return result;
}

void GraphicsContext::setURLForRect(const URL&, const IntRect&)
{
    notImplemented();
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator op, BlendMode)
{
    if (paintingDisabled())
        return;

    int ope;
    switch (op) {
    case CompositeClear:
        ope = WKC_COMPOSITEOPERATION_CLEAR;
        break;
    case CompositeCopy:
        ope = WKC_COMPOSITEOPERATION_COPY;
        break;
    case CompositeSourceOver:
        ope = WKC_COMPOSITEOPERATION_SOURCEOVER;
        break;
    case CompositeSourceIn:
        ope = WKC_COMPOSITEOPERATION_SOURCEIN;
        break;
    case CompositeSourceOut:
        ope = WKC_COMPOSITEOPERATION_SOURCEOUT;
        break;
    case CompositeSourceAtop:
        ope = WKC_COMPOSITEOPERATION_SOURCEATOP;
        break;
    case CompositeDestinationOver:
        ope = WKC_COMPOSITEOPERATION_DESTINATIONOVER;
        break;
    case CompositeDestinationIn:
        ope = WKC_COMPOSITEOPERATION_DESTINATIONIN;
        break;
    case CompositeDestinationOut:
        ope = WKC_COMPOSITEOPERATION_DESTINATIONOUT;
        break;
    case CompositeDestinationAtop:
        ope = WKC_COMPOSITEOPERATION_DESTINATIONATOP;
        break;
    case CompositeXOR:
        ope = WKC_COMPOSITEOPERATION_XOR;
        break;
    case CompositePlusDarker:
        ope = WKC_COMPOSITEOPERATION_PLUSDARKER;
        break;
    case CompositePlusLighter:
        ope = WKC_COMPOSITEOPERATION_PLUSLIGHTER;
        break;
    default:
        return;
    }
    wkcDrawContextSetCompositeOperationPeer(m_data->m_drawcontext, ope);
}

void GraphicsContext::setPlatformStrokeColor(const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    wkcDrawContextSetStrokeColorPeer(m_data->m_drawcontext, platformColor(color));
}

void GraphicsContext::setPlatformTextDrawingMode(TextDrawingModeFlags mode)
{
    if (paintingDisabled())
        return;

    wkcDrawContextSetTextDrawingModePeer(m_data->m_drawcontext, platformTextDrawingMode(mode));
}

void GraphicsContext::setPlatformStrokeStyle(StrokeStyle strokeStyle)
{
    if (paintingDisabled())
        return;
    wkcDrawContextSetStrokeStylePeer(m_data->m_drawcontext, platformStyle(strokeStyle));
}

void GraphicsContext::setPlatformStrokeThickness(float thickness)
{
    if (paintingDisabled())
        return;

    wkcDrawContextSetStrokeThicknessPeer(m_data->m_drawcontext, thickness);
}

void GraphicsContext::setPlatformFillColor(const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    wkcDrawContextSetFillColorPeer(m_data->m_drawcontext, platformColor(color));
}

void GraphicsContext::setPlatformShouldAntialias(bool enable)
{
    if (paintingDisabled())
        return;
    wkcDrawContextSetShouldAntialiasPeer(m_data->m_drawcontext, enable ? 1 : 0);
}

void GraphicsContext::setImageInterpolationQuality(InterpolationQuality)
{
//    notImplemented();
}

InterpolationQuality GraphicsContext::imageInterpolationQuality() const
{
    return InterpolationDefault;
}

PlatformPatternPtr
Pattern::createPlatformPattern(const WebCore::AffineTransform& userSpaceTransformation) const
{
    WKC_DEFINE_STATIC_PTR(WKCPeerPattern*, pattern, new WKCPeerPattern);
    ::memset(pattern, 0, sizeof(WKCPeerPattern));

    Image* tile = tileImage();
    if (!tile) return 0;
    ImageWKC* img = (ImageWKC *)tile->nativeImageForCurrentFrame();
    if (!img) return 0;

    pattern->fType = WKC_PATTERN_IMAGE;
    switch (img->type()) {
    case ImageWKC::EColorARGB8888:
        pattern->u.fImage.fType = WKC_IMAGETYPE_ARGB8888;
        break;
    case ImageWKC::EColorRGB565:
        //notImplemented();
        goto error_end;
        break;
    default:
        goto error_end;
    }
    if (img->hasAlpha()) {
        pattern->u.fImage.fType |= WKC_IMAGETYPE_FLAG_HASALPHA;
#if 0
        if (img->hasTrueAlpha()) {
            pattern->u.fImage.fType |= WKC_IMAGETYPE_FLAG_HASTRUEALPHA;
        }
#endif
    }
    
    pattern->u.fImage.fBitmap = img->bitmap();
    pattern->u.fImage.fRowBytes = img->rowbytes();
    pattern->u.fImage.fMask = img->mask();
    pattern->u.fImage.fMaskRowBytes = img->maskrowbytes();
    WKCFloatRect_SetXYWH(&pattern->u.fImage.fSrcRect, 0, 0, img->size().width(), img->size().height());
    WKCFloatSize_Set(&pattern->u.fImage.fScale,  (float)img->scalex(), (float)img->scaley());
    if (pattern->u.fImage.fScale.fWidth==0 || pattern->u.fImage.fScale.fHeight==0) {
        goto error_end;
    }
    WKCFloatSize_Set(&pattern->u.fImage.fiScale, 1.f/pattern->u.fImage.fScale.fWidth, 1.f/pattern->u.fImage.fScale.fHeight);
    WKCFloatPoint_Set(&pattern->u.fImage.fPhase, 0.f, 0.f);
    WKCFloatSize_Set(&pattern->u.fImage.fiTransform, 1.f, 1.f);

    pattern->u.fImage.fRepeatX = m_repeatX;
    pattern->u.fImage.fRepeatY = m_repeatY;

    pattern->u.fImage.fFontId = (void *)&m_patternSpaceTransformation;

    return (PlatformPatternPtr)pattern;

error_end:
    if (ImageObserver* observer = tile->imageObserver())
        observer->didDraw(tile);
    return 0;
}

static void
tidyPattern(Pattern* pattern)
{
    if (!pattern)
        return;
    Image* tile = pattern->tileImage();
    if (!tile)
        return;
    if (ImageObserver* observer = tile->imageObserver())
        observer->didDraw(tile);
}

void GraphicsContext::tidyPattern_i(Pattern* pattern)
{
    tidyPattern(pattern);
}

void GraphicsContext::fillPath(const Path& path)
{
    if (paintingDisabled())
        return;

    if (!m_data->m_opacity)
        return;

    WKCPeerPattern* pt = 0;
    Pattern* pattern = fillPattern();
    Gradient* gradient = fillGradient();
    AffineTransform affine;
    if (pattern) {
        pt = (WKCPeerPattern *)pattern->createPlatformPattern(affine);
        if (!pt) return;
    } else if (gradient) {
        pt = (WKCPeerPattern *)gradient->platformGradient();
        if (!pt) return;
    } else {
        Color c = fillColor();
        if (!c.alpha())
            return;
    }
    wkcDrawContextSetPatternPeer(m_data->m_drawcontext, pt);
    wkcDrawContextSetDrawAccuratePeer(m_data->m_drawcontext, true);
    int savedFillRule = wkcDrawContextGetFillRulePeer(m_data->m_drawcontext);
    int x_fillRule = WKC_FILLRULE_WINDING;
    if (fillRule() == RULE_EVENODD) {
        x_fillRule = WKC_FILLRULE_EVENODD;
    }
    wkcDrawContextSetFillRulePeer(m_data->m_drawcontext, x_fillRule);

    m_data->save();

    if (pattern) {
        AffineTransform* affine = static_cast<AffineTransform *>(pt->u.fImage.fFontId);
        concatCTM(*affine);
        pt->u.fImage.fFontId = 0;
    }

#if 0 == X_CSS_SHADOW_IMPL
    if (hasShadow() && wkcDrawContextGetOffscreenTypePeer(m_data->m_drawcontext) != WKC_OFFSCREEN_TYPE_IMAGEBUF)
        drawPathShadow(this, path, m_state.shadowOffset, m_state.shadowBlur, m_state.shadowColor);
#endif /* X_CSS_SHADOW_IMPL */

    PlatformPathWKC* pp = (PlatformPathWKC *)path.platformPath();
    pp->fillPath(m_data->m_drawcontext, &m_data->m_transform);

    m_data->restore(false);

    wkcDrawContextSetFillRulePeer(m_data->m_drawcontext, savedFillRule);
    wkcDrawContextSetDrawAccuratePeer(m_data->m_drawcontext, false);
    wkcDrawContextSetPatternPeer(m_data->m_drawcontext, 0);
    if (pattern)
        tidyPattern(pattern);
}

void GraphicsContext::strokePath(const Path& path)
{
    if (paintingDisabled())
        return;

    if (!m_data->m_opacity)
        return;

    WKCPeerPattern* pt = 0;
    Pattern* pattern = strokePattern();
    Gradient* gradient = strokeGradient();
    AffineTransform affine;
    if (pattern) {
        pt = (WKCPeerPattern *)pattern->createPlatformPattern(affine);
        if (!pt) return;
    } else if (gradient) {
        pt = (WKCPeerPattern *)gradient->platformGradient();
        if (!pt) return;
    } else {
        Color c = strokeColor();
        if (!c.alpha())
            return;
    }
    wkcDrawContextSetPatternPeer(m_data->m_drawcontext, pt);
    wkcDrawContextSetDrawAccuratePeer(m_data->m_drawcontext, true);

    m_data->save();
    wkcDrawContextSetPenStylePeer(m_data->m_drawcontext, platformColor(strokeColor()), strokeThickness(), platformStyle(strokeStyle()));
    PlatformPathWKC* pp = (PlatformPathWKC *)path.platformPath();
    pp->strokePath(m_data->m_drawcontext, &m_data->m_transform);

    m_data->restore(false);

    wkcDrawContextSetDrawAccuratePeer(m_data->m_drawcontext, false);
    wkcDrawContextSetPatternPeer(m_data->m_drawcontext, 0);
    if (pattern)
        tidyPattern(pattern);
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->save();

    WKCPeerPattern* pt = 0;
    Pattern* pattern = fillPattern();
    Gradient* gradient = fillGradient();
    AffineTransform affine;
    WKCFloatPoint p[6];

    if (pattern) {
        pt = (WKCPeerPattern *)pattern->createPlatformPattern(affine);
        if (!pt)
            goto error_exit;
    } else if (gradient) {
        pt = (WKCPeerPattern *)gradient->platformGradient();
        if (!pt)
            goto error_exit;
    } else {
        Color c = fillColor();
        if (!c.alpha() || !m_data->m_opacity)
            goto error_exit;
    }
    wkcDrawContextSetPatternPeer(m_data->m_drawcontext, pt);

    if (m_data->mapRect(rect, p)) {
        WKCFloatPoint_SetPoint(&p[4], &p[0]);
        WKCFloatPoint_Set(&p[5], FLT_MIN, FLT_MIN);
        wkcDrawContextDrawPolygonPeer(m_data->m_drawcontext, 6, p);
    }
    wkcDrawContextSetPatternPeer(m_data->m_drawcontext, 0);
    if (pattern)
        tidyPattern(pattern);

error_exit:
    m_data->restore(false);
}

void GraphicsContext::setPlatformShadow(const FloatSize& size, float blur, Color const& color, ColorSpace) 
{
    if (paintingDisabled())
        return;

    int type = 0;
    WKCFloatSize ws = { size.width(), size.height() };
    /*
     * see the comments in GraphicsContextSkia.cpp:GraphicsContext::setPlatformShadow().
     * yes, we (who are implementing graphics porting layer) all know it is ugly.  but this is the life.
     */
    if (m_state.shadowsIgnoreTransforms) {
      type = 1;
      ws.fHeight = - ws.fHeight;
    }
    /*
     * FIXME: someday we might also need to pass the value of "shadowsUseLegacyRadius".
     */
    wkcDrawContextSetShadowPeer(m_data->m_drawcontext, type, NULL, NULL, NULL, &ws, blur, platformColor(color), m_state.shadowsIgnoreTransforms);
}

void GraphicsContext::clearPlatformShadow() 
{
    if (paintingDisabled())
        return;
    wkcDrawContextClearShadowPeer(m_data->m_drawcontext);
}

void GraphicsContext::beginPlatformTransparencyLayer(float opacity) 
{ 
    if (paintingDisabled())
        return;
    m_data->save();
    m_data->m_opacity *= opacity;
    unsigned char alpha = (unsigned char)(m_data->m_opacity * 255);
    wkcDrawContextBeginTransparencyLayerPeer(m_data->m_drawcontext, alpha);
}

void GraphicsContext::endPlatformTransparencyLayer() 
{ 
    if (paintingDisabled())
        return;
    wkcDrawContextEndTransparencyLayerPeer(m_data->m_drawcontext);
    m_data->restore(false);
}

bool GraphicsContext::supportsTransparencyLayers()
{
    return true;
}

void GraphicsContext::clearRect(const FloatRect& rect) 
{
    if (paintingDisabled())
        return;

    WKCFloatPoint p[6];
    if (m_data->mapRect(rect, p)) {
        WKCFloatPoint_SetPoint(&p[4], &p[0]);
        WKCFloatPoint_Set(&p[5], FLT_MIN, FLT_MIN);
        wkcDrawContextClearPolygonPeer(m_data->m_drawcontext, 6, p);
    }
}

void GraphicsContext::strokeRect(const FloatRect& rect, float)
{ 
    if (paintingDisabled())
        return;

    int type = 0;
    type = wkcDrawContextGetOffscreenTypePeer(m_data->m_drawcontext);

    m_data->save();

    wkcDrawContextSetPenStylePeer(m_data->m_drawcontext, platformColor(strokeColor()), strokeThickness(), platformStyle(strokeStyle()));
    if (type==WKC_OFFSCREEN_TYPE_IMAGEBUF) {
        WKCPeerPattern* pt = 0;
        Pattern* pattern = strokePattern();
        Gradient* gradient = strokeGradient();
        AffineTransform affine;
        if (pattern && pattern->tileImage()) {
            pt = (WKCPeerPattern *)pattern->createPlatformPattern(affine);
            if (!pt)
                goto error_exit;
            
        } else if (gradient) {
            pt = (WKCPeerPattern *)gradient->platformGradient();
            if (!pt)
                goto error_exit;
        } else {
            Color c = strokeColor();
            if (!c.alpha() || !m_data->m_opacity)
                goto error_exit;
        }
        wkcDrawContextSetPatternPeer(m_data->m_drawcontext, pt);

        WKCFloatPoint p[6];
        FloatRect fr(rect);

        bool drawjoin = true;
        if (fr.width()==0 && fr.height()!=0) {
            fr.setWidth(1);
            drawjoin = false;
        } else if (fr.height()==0 && fr.width()!=0) {
            fr.setHeight(1);
            drawjoin = false;
        }

        if (m_data->mapRect(fr, p)) {
            WKCFloatPoint_SetPoint(&p[4], &p[0]);
            wkcDrawContextDrawPolylinePeer(m_data->m_drawcontext, 5, p, true, drawjoin);
        }

        wkcDrawContextSetPatternPeer(m_data->m_drawcontext, 0);
        if (pattern)
            tidyPattern(pattern);
    } else {
        WKCFloatRect r;
        platformRect(rect, r);
        wkcDrawContextStrokeRectPeer(m_data->m_drawcontext, &r);
    }

error_exit:
    m_data->restore(false);
}

void GraphicsContext::setLineCap(LineCap cap) 
{
    int type = 0;
    switch (cap) {
    case ButtCap:
        type = WKC_LINECAP_BUTTCAP;
        break;
    case RoundCap:
        type = WKC_LINECAP_ROUNDCAP;
        break;
    case SquareCap:
        type = WKC_LINECAP_SQUARECAP;
        break;
    default:
        return;
    }
    wkcDrawContextSetLineCapPeer(m_data->m_drawcontext, type);
}

void GraphicsContext::setLineJoin(LineJoin join)
{
    int type = 0;
    switch (join) {
    case MiterJoin:
        type = WKC_LINEJOIN_MITERJOIN;
        break;
    case RoundJoin:
        type = WKC_LINEJOIN_ROUNDJOIN;
        break;
    case BevelJoin:
        type = WKC_LINEJOIN_BEVELJOIN;
        break;
    default:
        return;
    }
    wkcDrawContextSetLineJoinPeer(m_data->m_drawcontext, type);
}

void GraphicsContext::setMiterLimit(float lim)
{
    wkcDrawContextSetMiterLimitPeer(m_data->m_drawcontext, lim);
}

void GraphicsContext::setAlpha(float alpha)
{
    if (alpha<0.f)
        alpha = 0.f;
    else if (alpha>1.f)
        alpha = 1.f;
    wkcDrawContextSetAlphaPeer(m_data->m_drawcontext, (int)(255.0 * alpha));
}

#if 0
void GraphicsContext::addInnerRoundedRectClip(const IntRect& rect, int thickness)
{
    if (paintingDisabled())
        return;

    clip(rect);

    Path p;
    FloatRect r(rect);
    // Add outer ellipse
    p.addEllipse(r);
    // Add inner ellipse
    r.inflate(-thickness);
    p.addEllipse(r);

    clip(p);
}
#endif

void GraphicsContext::setLineDash(const DashArray& dashes, float dashOffset)
{
    if (dashes.isEmpty()) {
        wkcDrawContextSetLineDashPeer(m_data->m_drawcontext, 0, 0, 0);
        return;
    }
    DashArray ldash = dashes;

    bool allzero = true;
    for (DashArray::iterator it = ldash.begin(); it != ldash.end(); ++it) {
        if (*it) allzero = false;
    }
    if (allzero) {
        wkcDrawContextSetLineDashPeer(m_data->m_drawcontext, 0, 0, 0);
        setStrokeStyle(SolidStroke);
        return;
    }
    if (ldash.size() == 1)
        ldash.append(dashes[0]);

    setStrokeStyle(DashedStroke);
    wkcDrawContextSetLineDashPeer(m_data->m_drawcontext, &ldash[0], ldash.size(), dashOffset);
}

void GraphicsContext::clipPath(const Path& path, WindRule /*rule*/)
{
    m_data->m_clip_context = GraphicsContextPlatformPrivateData::C_UNKNOWN;  /* FIXME: impl C_SVG */
    m_data->m_clip = path;
    m_data->m_clip_transform = m_data->m_transform;
    PlatformPathWKC* pp = (PlatformPathWKC *)path.platformPath();
    wkcDrawContextClearClipPolygonPeer(m_data->m_drawcontext);
    pp->clipPath(m_data->m_drawcontext, &m_data->m_transform);
}


}

#endif // !USE(WKC_CAIRO)

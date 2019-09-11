/*
 * Copyright (c) 2011-2017 ACCESS CO., LTD. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "helpers/WKCGraphicsLayer.h"
#include "helpers/privates/WKCGraphicsLayerPrivate.h"

#include "FloatRect.h"
#include "GraphicsLayer.h"
#include "GraphicsContext.h"
#include "Image.h"

#include "CalculationValue.h"
#include "Length.h"
#include "GraphicsLayerClient.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include <wtf/HashSet.h>

#include <wkc/wkcgpeer.h>

#include "helpers/privates/WKCHelpersEnumsPrivate.h"

typedef HashSet<WKC::GraphicsLayerPrivate*> LayerSet;
WKC_DEFINE_GLOBAL_HASHSETPTR(WKC::GraphicsLayerPrivate*, gLayerSet, 0);

namespace WebCore {
WKC::GraphicsLayerPrivate* GraphicsLayerWKC_wkc(const WebCore::GraphicsLayer* layer);
}

namespace WKC {

GraphicsLayerPrivate::GraphicsLayerPrivate(WebCore::GraphicsLayer* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_offscreenLayer(0)
    , m_offscreenLayerIsImage(false)
    , m_opticalzoom(1.f)
    , m_dirtyrects()
    , m_needsDisplay(false)
    , m_isFixed(false)
    , m_marked(false)
    , m_top(0)
    , m_right(0)
    , m_bottom(0)
    , m_left(0)
    , m_marginTop(0)
    , m_marginRight(0)
    , m_marginBottom(0)
    , m_marginLeft(0)
    , m_topType(LengthTypeUndefined)
    , m_rightType(LengthTypeUndefined)
    , m_bottomType(LengthTypeUndefined)
    , m_leftType(LengthTypeUndefined)
    , m_marginTopType(LengthTypeUndefined)
    , m_marginRightType(LengthTypeUndefined)
    , m_marginBottomType(LengthTypeUndefined)
    , m_marginLeftType(LengthTypeUndefined)
    , m_solidColor()
{
    WKCSize_Set(&m_offscreenSize, 0, 0);
    WKCRect_MakeEmpty(&m_fixedRect);
    WKCPoint_Set(&m_renderLayerPos, 0, 0);
    WKCRect_MakeEmpty(&m_lastPaintedRect);

    if (!gLayerSet)
        gLayerSet = new LayerSet();

    gLayerSet->add(this);
}

GraphicsLayerPrivate::~GraphicsLayerPrivate()
{
    gLayerSet->remove(this);

    if (m_offscreenLayer)
        wkcLayerDeletePeer(m_offscreenLayer);
}

void
GraphicsLayerPrivate::disposeOffscreen()
{
    WKCSize_Set(&m_offscreenSize, 0, 0);
    WKCRect_MakeEmpty(&m_lastPaintedRect);

    if (m_offscreenLayer) {
        wkcLayerDeletePeer(m_offscreenLayer);
        m_offscreenLayer = 0;
    }
}

bool
GraphicsLayerPrivate::ensureOffscreen(int ow, int oh)
{
    if (m_offscreenLayer && (ow==m_offscreenSize.fWidth && oh==m_offscreenSize.fHeight))
        return true;

    WKCSize wsize = {ow, oh};
    if (m_offscreenLayer) {
        if (!wkcLayerResizePeer(m_offscreenLayer, &wsize)) {
            m_offscreenLayer = 0;
            goto fail;
        }
    } else {
        m_offscreenLayer = wkcLayerNewPeer(WKC_LAYER_TYPE_OFFSCREEN, &wsize, m_opticalzoom);
        if (!m_offscreenLayer)
            goto fail;
    }

    WKCSize_SetSize(&m_offscreenSize, &wsize);
    return true;

fail:
    wkcMemoryNotifyMemoryAllocationErrorPeer(ow * oh * 4 * m_opticalzoom, WKC_MEMORYALLOC_TYPE_LAYER); // The size may not be accurate, since the error caused in peer.
    disposeOffscreen();
    return false;
}

void
GraphicsLayerPrivate::clear(const WKCRect& rect, void* alternative_drawcontext)
{
    if (!alternative_drawcontext && !m_offscreenLayer)
        return;

    if (WKCRect_IsEmpty(&rect))
        return;
    if (m_offscreenLayerIsImage)
        return;
#if 0
    if (!m_needsDisplay)
        return;
#endif

    PlatformLayer* layer = m_webcore->platformLayer();
    if (layer) {
        int type = wkcLayerGetTypePeer(static_cast<void *>(layer));
        if (type != WKC_LAYER_TYPE_OFFSCREEN)
            return;
    }

    void* offscreen_layer = 0;
    void* dc = 0;
    if (alternative_drawcontext) {
        offscreen_layer = 0;
        dc = alternative_drawcontext;
    } else {
        offscreen_layer = m_offscreenLayer;
        dc = wkcLayerGetDrawContextPeer(offscreen_layer);
    }

    if (!dc)
        return;
    WebCore::GraphicsContext gc(static_cast<PlatformGraphicsContext *>(dc));
    gc.save();
#if USE(WKC_CAIRO)
    const WKCFloatPoint dummy = {0};
    wkcDrawContextSetOpticalZoomPeer(dc, m_opticalzoom, &dummy);
#endif
    gc.clip(rect);

    //FIXME: Handle partial repaints, by clearing only the rect invalidated by setNeedsDisplayInRect
    WebCore::FloatRect floatRect(rect.fX, rect.fY, rect.fWidth, rect.fHeight);
    gc.setCompositeOperation(WebCore::CompositeSourceOver);
    gc.clearRect(floatRect);
    gc.restore();

    if (offscreen_layer)
        wkcLayerReleaseDrawContextPeer(offscreen_layer, dc);

    m_needsDisplay = false;
}

void
GraphicsLayerPrivate::paint(const WKCRect& rect)
{
    WKCPoint null_offset = { 0, 0 };
    paint(rect, null_offset, 1.0f, 0, 0);
}

void
GraphicsLayerPrivate::paint(const WKCRect& rect, const WKCPoint& offset, float opacity, GraphicsLayerPrivate* alternative_target, void* alternative_drawcontext)
{
    if (alternative_target && alternative_drawcontext)
        return;
    if (!((alternative_target && alternative_target->m_offscreenLayer) || alternative_drawcontext || m_offscreenLayer != 0))
        return;

    if (m_offscreenLayerIsImage)
        return;

    PlatformLayer* layer = m_webcore->platformLayer();
    if (layer) {
        int type = wkcLayerGetTypePeer(static_cast<void *>(layer));
        if (type != WKC_LAYER_TYPE_OFFSCREEN)
            return;
    }

    if (WKCRect_IsEmpty(&rect))
        return;

    void* offscreen_layer = 0;
    void* dc = 0;
    if (alternative_target) {
        offscreen_layer = alternative_target->m_offscreenLayer;
        dc = wkcLayerGetDrawContextPeer(offscreen_layer);
    } else if (alternative_drawcontext) {
        offscreen_layer = 0;
        dc = alternative_drawcontext;
    } else {
        offscreen_layer = m_offscreenLayer;
        dc = wkcLayerGetDrawContextPeer(offscreen_layer);
    }

    if (!dc)
        return;
    WebCore::GraphicsContext gc(static_cast<PlatformGraphicsContext *>(dc));
    gc.save();
#if USE(WKC_CAIRO)
    const WKCFloatPoint dummy = {0};
    wkcDrawContextSetOpticalZoomPeer(dc, m_opticalzoom, &dummy);
#endif
    const WKCRect real_rect = { rect.fX - offset.fX, rect.fY - offset.fY, rect.fWidth, rect.fHeight };
    gc.clip(real_rect);
    gc.translate(-offset.fX, -offset.fY);
    if (opacity < 1.0f)
        gc.beginTransparencyLayer(opacity);

    //FIXME: Handle partial repaints, by clearing only the rect invalidated by setNeedsDisplayInRect
    WebCore::FloatRect floatRect(rect.fX, rect.fY, rect.fWidth, rect.fHeight);
#if 0
    gc.clearRect(floatRect);
#endif

    m_webcore->paintGraphicsLayerContents(gc, floatRect);
    if (opacity < 1.0f)
        gc.endTransparencyLayer();
    gc.restore();

    if (offscreen_layer)
        wkcLayerReleaseDrawContextPeer(offscreen_layer, dc);

    WKCRect_SetXYWH(&m_lastPaintedRect, rect.fX, rect.fY, rect.fWidth, rect.fHeight);
    m_needsDisplay = false;
}

WKCRect
GraphicsLayerPrivate::lastPaintedRect() const
{
    return m_lastPaintedRect;
}

void
GraphicsLayerPrivate::setContentsToImage(WebCore::Image* img)
{
    // This method must not be called in current our environment...
    // Need to be rewriten like paint() when used.
    ASSERT(false);

    if (!ensureOffscreen(img->width(), img->height()))
        return;

    void* dc = wkcLayerGetDrawContextPeer(m_offscreenLayer);
    if (!dc)
        return;
    WebCore::GraphicsContext gc(static_cast<PlatformGraphicsContext *>(dc));
    gc.save();
#if USE(WKC_CAIRO)
    const WKCFloatPoint dummy = {0};
    wkcDrawContextSetOpticalZoomPeer(dc, m_opticalzoom, &dummy);
#endif
    gc.clip(WebCore::IntRect(0, 0, img->width(), img->height()));
    gc.clearRect(WebCore::IntRect(0, 0, img->width(), img->height()));
    gc.drawImage(img, WebCore::ColorSpaceDeviceRGB, WebCore::IntPoint(0,0));
    gc.restore();
    wkcLayerReleaseDrawContextPeer(m_offscreenLayer, dc);
    m_offscreenLayerIsImage = true;
}

void
GraphicsLayerPrivate::setContentsToSolidColor(const WebCore::Color& c)
{
    m_solidColor = Color((ColorPrivate*)&c);
}

Color 
GraphicsLayerPrivate::contentsSolidColor() const
{
    return m_solidColor;
}

void
GraphicsLayerPrivate::layerDidDisplay(void* layer)
{
    if (!layer)
        return;
    int type = wkcLayerGetTypePeer(layer);
    if (type == WKC_LAYER_TYPE_3DCANVAS) {
        wkcLayerDidDisplayPeer(layer);
    }

    m_dirtyrects.clear();
}

void*
GraphicsLayerPrivate::platformLayer() const
{
    return static_cast<void*>(m_webcore->platformLayer());
}

size_t
GraphicsLayerPrivate::dirtyRectsSize() const
{
    return m_dirtyrects.size();
}

void
GraphicsLayerPrivate::dirtyRects(int index, WKCFloatRect& out_rect) const
{
    if (index>=m_dirtyrects.size()) {
        WKCFloatRect_MakeEmpty(&out_rect);
        return;
    }
    WKCFloatRect_SetRect(&out_rect, &(m_dirtyrects[index]));
}

bool
GraphicsLayerPrivate::addDirtyRect(const WKCFloatRect& rect)
{
    if (rect.fWidth==0 || rect.fHeight==0)
        return false;

    size_t count = m_dirtyrects.size();
    for (int i=0; i<count; i++) {
        if (WKCFloatRect_ContainRect(&m_dirtyrects[i], &rect))
            return false;
    }

    if (count > 0) {
        // Peephole optimization: ad-hoc but effective...
        WKCFloatRect& lastAdded = m_dirtyrects.last();
        if (WKCFloatRect_ContainRect(&rect, &lastAdded)) {
            m_dirtyrects.last() = rect;
            return true;
        }
    }

    m_dirtyrects.append(rect);
    return true;
}

bool
GraphicsLayerPrivate::addDirtyRect()
{
    m_dirtyrects.clear();
    const WKCFloatRect rect = {0, 0, m_webcore->size().width(), m_webcore->size().height() };
    m_dirtyrects.append(rect);
    return true;
}

void
GraphicsLayerPrivate::resetDirtyRects()
{
    m_dirtyrects.clear();
}

bool
GraphicsLayerPrivate::contentsAreVisible() const
{
    return m_webcore->contentsAreVisible();
}

bool
GraphicsLayerPrivate::drawsContent() const
{
    return m_webcore->drawsContent();
}

GraphicsLayer*
GraphicsLayerPrivate::parent() const
{
    WebCore::GraphicsLayer* layer = m_webcore->parent();
    if (!layer)
        return 0;
    return &(GraphicsLayerWKC_wkc(layer)->wkc());
}

size_t
GraphicsLayerPrivate::childrenSize() const
{
    return m_webcore->children().size();
}

GraphicsLayer*
GraphicsLayerPrivate::children(int index)
{
    WebCore::GraphicsLayer* layer = m_webcore->children().at(index);
    if (!layer)
        return 0;
    return &(GraphicsLayerWKC_wkc(layer)->wkc());
}

const WKCFloatSize
GraphicsLayerPrivate::size() const
{
    return m_webcore->size();
}

const WKCFloatPoint
GraphicsLayerPrivate::position() const
{
    return m_webcore->position();
}

const WKCFloatPoint
GraphicsLayerPrivate::anchorPoint(float& z) const
{
    const WebCore::FloatPoint3D& p = m_webcore->anchorPoint();
    z = p.z();
    return WebCore::FloatPoint(p.x(), p.y());
}

float
GraphicsLayerPrivate::opacity() const
{
    return m_webcore->opacity();
}

BlendMode
GraphicsLayerPrivate::blendMode() const
{
    WebCore::GraphicsLayerClient& client = m_webcore->client();
    WebCore::RenderLayer* renderLayer = client.owningLayerPtr();
    if (!renderLayer)
        return toWKCBlendMode(WebCore::BlendModeNormal);
    return toWKCBlendMode(renderLayer->blendMode());
}

void
GraphicsLayerPrivate::transform(double* m) const
{
    m[0] = m_webcore->transform().m11();
    m[1] = m_webcore->transform().m12();
    m[2] = m_webcore->transform().m13();
    m[3] = m_webcore->transform().m14();
    m[4] = m_webcore->transform().m21();
    m[5] = m_webcore->transform().m22();
    m[6] = m_webcore->transform().m23();
    m[7] = m_webcore->transform().m24();
    m[8] = m_webcore->transform().m31();
    m[9] = m_webcore->transform().m32();
    m[10] = m_webcore->transform().m33();
    m[11] = m_webcore->transform().m34();
    m[12] = m_webcore->transform().m41();
    m[13] = m_webcore->transform().m42();
    m[14] = m_webcore->transform().m43();
    m[15] = m_webcore->transform().m44();
}

void
GraphicsLayerPrivate::childrenTransform(double* m) const
{
    m[0] = m_webcore->childrenTransform().m11();
    m[1] = m_webcore->childrenTransform().m12();
    m[2] = m_webcore->childrenTransform().m13();
    m[3] = m_webcore->childrenTransform().m14();
    m[4] = m_webcore->childrenTransform().m21();
    m[5] = m_webcore->childrenTransform().m22();
    m[6] = m_webcore->childrenTransform().m23();
    m[7] = m_webcore->childrenTransform().m24();
    m[8] = m_webcore->childrenTransform().m31();
    m[9] = m_webcore->childrenTransform().m32();
    m[10] = m_webcore->childrenTransform().m33();
    m[11] = m_webcore->childrenTransform().m34();
    m[12] = m_webcore->childrenTransform().m41();
    m[13] = m_webcore->childrenTransform().m42();
    m[14] = m_webcore->childrenTransform().m43();
    m[15] = m_webcore->childrenTransform().m44();
}

void
GraphicsLayerPrivate::flushCompositingState(const WebCore::FloatRect& clipRect, bool viewportIsStable)
{
#if ENABLE(COMPOSITED_FIXED_ELEMENTS)
    size_t children_size = childrenSize();
    for (size_t i = 0; i < children_size; i++) {
        WebCore::GraphicsLayer* layer = m_webcore->children().at(i);
        if (!layer) {
            ASSERT(false);
            continue;
        }
        GraphicsLayerPrivate* child = GraphicsLayerWKC_wkc(layer);
        child->flushCompositingState(clipRect, viewportIsStable);
    }

    updateFixedPosition();

    m_webcore->client().didCommitChangesForLayer(m_webcore);
#endif
}

void
GraphicsLayerPrivate::layerDidAttachToTree(GraphicsLayerPrivate* /*parent*/)
{
    if (m_offscreenLayer)
        wkcLayerDidAttachToTreePeer(m_offscreenLayer);
}

void
GraphicsLayerPrivate::layerDidDetachFromTree()
{
    if (m_offscreenLayer)
        wkcLayerDidDetachFromTreePeer(m_offscreenLayer);
}

void
GraphicsLayerPrivate::setOpticalZoom(float zoom)
{
    if (m_opticalzoom == zoom)
        return;
    m_opticalzoom = zoom;

    if (!m_offscreenLayer) {
        // ensureOffscreen() set optical zoom.
        return;
    }
    if (!wkcLayerSetOpticalZoomPeer(m_offscreenLayer, zoom)) {
        m_offscreenLayer = 0;
        wkcMemoryNotifyMemoryAllocationErrorPeer(m_offscreenSize.fWidth * m_offscreenSize.fHeight * 4 * m_opticalzoom, WKC_MEMORYALLOC_TYPE_LAYER); // The size may not be accurate, since the error caused in peer.
        disposeOffscreen(); 
        return;
    }
    addDirtyRect();
}

#if ENABLE(COMPOSITED_FIXED_ELEMENTS)

// Currently, we can use WebCore::Length directly since the length values are only used in this file.
// But here we introduce the own LengthType, since the length values may be exposed to applications.
static bool
setLength(WebCore::Length len, LengthType& type, float& value)
{
    LengthType old_type = type;
    float old_value = value;

    switch (len.type()) {
    case WebCore::Percent:
        type = LengthTypePercent;
        value = len.percent();
        break;
    case WebCore::Fixed:
        type = LengthTypeFixed;
        value = len.value();
        break;
    case WebCore::Auto:
        type = LengthTypeUndefined;
        value = 0;
        break;
    case WebCore::Calculated:
        type = LengthTypeCalculated;
        value = len.calculationValue().evaluate(std::numeric_limits<float>::max());
        break;
    default:
        ASSERT(false);
        type = LengthTypeUndefined;
        value = 0;
        break;
    }

    return (old_type != type) || (old_value != value);
}

static float
calcLength(LengthType type, float value, float base, float zoom)
{
    switch (type) {
    case LengthTypePercent:
        return value * base / 100.0f;  // Ignore `zoom' since the `value' is a percentage value and `zoom' is already multiplied to `base'....
    case LengthTypeFixed:
        return (value * zoom);
    case LengthTypeCalculated:
        return (value * zoom);
    default:
        ASSERT(value == 0);
        return 0;
    }
}

void
GraphicsLayerPrivate::updateFixedPosition()
{
    WebCore::GraphicsLayerClient& client = m_webcore->client();
    WebCore::RenderLayer* renderLayer = client.owningLayerPtr();
    if (!renderLayer)
        return;
    WebCore::RenderView& view = static_cast<WebCore::RenderView&>(renderLayer->renderer());

    bool modified = false;
    if (view.isPositioned() && view.style().position() == WebCore::FixedPosition) {
        modified |= setLength(view.style().left(), m_leftType, m_left);
        modified |= setLength(view.style().top(), m_topType, m_top);
        modified |= setLength(view.style().right(), m_rightType, m_right);
        modified |= setLength(view.style().bottom(), m_bottomType, m_bottom);

        modified |= setLength(view.style().marginLeft(), m_marginLeftType, m_marginLeft);
        modified |= setLength(view.style().marginTop(), m_marginTopType, m_marginTop);
        modified |= setLength(view.style().marginRight(), m_marginRightType, m_marginRight);
        modified |= setLength(view.style().marginBottom(), m_marginBottomType, m_marginBottom);

        int w = view.width();
        int h = view.height();
        int paintingOffsetX = - m_webcore->offsetFromRenderer().width();
        int paintingOffsetY = - m_webcore->offsetFromRenderer().height();

        modified |= (m_fixedRect.fX != paintingOffsetX) || (m_fixedRect.fY != paintingOffsetY);
        modified |= (m_fixedRect.fWidth != w) || (m_fixedRect.fHeight != h);
        modified |= (m_renderLayerPos.fX != view.x()) || (m_renderLayerPos.fY != view.y());
        modified |= !m_isFixed;

        WKCRect_SetXYWH(&m_fixedRect, paintingOffsetX, paintingOffsetY, w, h);
        WKCPoint_Set(&m_renderLayerPos, view.x(), view.y());
        m_isFixed = true;
    } else {
        modified |= m_isFixed;
        m_isFixed = false;
    }
#if 0
    if (modified)
        m_webcore->client().notifyFlushRequired(m_webcore);
#endif
}

bool
GraphicsLayer::ensureOffscreen(int ow, int oh)
{
    return m_private.ensureOffscreen(ow, oh);
}

void
GraphicsLayer::disposeOffscreen()
{
    m_private.disposeOffscreen();
}

bool
GraphicsLayer::isLeftDefined() const
{
    return m_private.isLeftDefined();
}

bool
GraphicsLayer::isRightDefined() const
{
    return m_private.isRightDefined();
}

bool
GraphicsLayer::isTopDefined() const
{
    return m_private.isTopDefined();
}

bool
GraphicsLayer::isBottomDefined() const
{
    return m_private.isBottomDefined();
}

#endif // ENABLE(COMPOSITED_FIXED_ELEMENTS)

bool
GraphicsLayerPrivate::preserves3D() const
{
    return m_webcore->preserves3D();
}

bool
GraphicsLayerPrivate::masksToBounds() const
{
    return m_webcore->masksToBounds();
}

bool
GraphicsLayerPrivate::isRootClippingLayer() const
{
    return m_webcore->isRootClippingLayer();
}

Color
GraphicsLayerPrivate::backgroundColor() const
{
    const WebCore::Color c = m_webcore->backgroundColor();
    return Color((ColorPrivate*)&c);
}

bool
GraphicsLayerPrivate::contentsOpaque() const
{
    return m_webcore->contentsOpaque();
}

bool
GraphicsLayerPrivate::backfaceVisibility() const
{
    return m_webcore->backfaceVisibility();
}

void
GraphicsLayerPrivate::setNeedsDisplay()
{
    m_needsDisplay = true;
}

GraphicsLayer*
GraphicsLayerPrivate::maskLayer()
{
    WebCore::GraphicsLayer* layer = m_webcore->maskLayer();
    GraphicsLayer* wkc_layer = NULL;
    if (layer)
        wkc_layer = &(GraphicsLayerWKC_wkc(layer)->wkc());
    return wkc_layer;
}

const char*
GraphicsLayerPrivate::name()
{
    m_name = m_webcore->name();
    return m_name.utf8().data();
}

void
GraphicsLayerPrivate::markDescendants()
{
    size_t children_size = childrenSize();
    for (size_t i = 0; i < children_size; i++) {
        WebCore::GraphicsLayer* layer = m_webcore->children().at(i);
        if (!layer) {
            ASSERT(false);
            continue;
        }
        GraphicsLayerPrivate* child = GraphicsLayerWKC_wkc(layer);
        child->markDescendants();
    }

    if (m_offscreenLayer)
        m_marked = true;
}

void
GraphicsLayerPrivate::disposeAllButDescendantsOf(GraphicsLayerPrivate* in_layer)
{
    if (!gLayerSet) {
        // we have no layer yet.
        ASSERT(!in_layer);
        return;
    }

    if (in_layer)
        in_layer->markDescendants();

    LayerSet::iterator it = gLayerSet->begin();
    LayerSet::iterator end = gLayerSet->end();
    for (; it != end; ++it) {
        if (!(*it)->m_marked) {
            (*it)->disposeOffscreen();
        }
        (*it)->m_marked = false;
    }
}

bool
GraphicsLayerPrivate::isVideo() const
{
    WebCore::GraphicsLayerClient& client = m_webcore->client();
    WebCore::RenderLayer* renderLayer = client.owningLayerPtr();
    if (renderLayer) {
        return renderLayer->renderer().isVideo();
    }

    return false;
}

int
GraphicsLayerPrivate::zIndex() const
{
    WebCore::GraphicsLayerClient& client = m_webcore->client();
    WebCore::RenderLayer* renderLayer = client.owningLayerPtr();
    if (renderLayer) {
        return renderLayer->zIndex();
    }

    return 0;
}


////////////////////////////////////////////////////////////////////////////////

GraphicsLayer::GraphicsLayer(GraphicsLayerPrivate& parent)
    : m_private(parent)
{
}

GraphicsLayer::~GraphicsLayer()
{
}

bool
GraphicsLayer::contentsAreVisible() const
{
    return m_private.contentsAreVisible();
}

bool
GraphicsLayer::drawsContent() const
{
    return m_private.drawsContent();
}

void
GraphicsLayer::clear(const WKCRect& rect, void* alternative_drawcontext)
{
    m_private.clear(rect, alternative_drawcontext);
}

void
GraphicsLayer::paint(const WKCRect& rect)
{
    m_private.paint(rect);
}

void
GraphicsLayer::paint(const WKCRect& rect, const WKCPoint& offset, float opacity, GraphicsLayer* alternative_target, void* alternative_drawcontext)
{
    GraphicsLayerPrivate* alternative_target_private = alternative_target ? &alternative_target->m_private : 0;
    m_private.paint(rect, offset, opacity, alternative_target_private, alternative_drawcontext);
}

WKCRect
GraphicsLayer::lastPaintedRect() const
{
    return m_private.lastPaintedRect();
}

void
GraphicsLayer::layerDidDisplay(void* layer)
{
    m_private.layerDidDisplay(layer);
}

void*
GraphicsLayer::platformLayer() const
{
    return m_private.platformLayer();
}

size_t
GraphicsLayer::dirtyRectsSize() const
{
    return m_private.dirtyRectsSize();
}

void
GraphicsLayer::dirtyRects(int index, WKCFloatRect& rect) const
{
    m_private.dirtyRects(index, rect);
}

void
GraphicsLayer::resetDirtyRects()
{
    m_private.resetDirtyRects();
}

GraphicsLayer*
GraphicsLayer::parent() const
{
    return m_private.parent();
}

size_t
GraphicsLayer::childrenSize() const
{
    return m_private.childrenSize();
}

GraphicsLayer*
GraphicsLayer::children(int index)
{
    return m_private.children(index);
}

const WKCFloatSize
GraphicsLayer::size() const
{
    return m_private.size();
}

const WKCFloatPoint
GraphicsLayer::position() const
{
    return m_private.position();
}

const WKCFloatPoint
GraphicsLayer::anchorPoint(float& z) const
{
    return m_private.anchorPoint(z);
}

float
GraphicsLayer::opacity() const
{
    return m_private.opacity();
}

BlendMode
GraphicsLayer::blendMode() const
{
    return m_private.blendMode();
}

void
GraphicsLayer::transform(double* m) const
{
    m_private.transform(m);
}

void
GraphicsLayer::childrenTransform(double* m) const
{
    m_private.childrenTransform(m);
}

void
GraphicsLayer::setOpticalZoom(float zoom)
{
    m_private.setOpticalZoom(zoom);
}

bool
GraphicsLayer::isFixedPosition() const
{
#if ENABLE(COMPOSITED_FIXED_ELEMENTS)
    return m_private.isFixed();
#else
    return false;
#endif
}

bool
GraphicsLayer::preserves3D() const
{
    return m_private.preserves3D();
}

bool
GraphicsLayer::masksToBounds() const
{
    return m_private.masksToBounds();
}

bool
GraphicsLayer::isRootClippingLayer() const
{
    return m_private.isRootClippingLayer();
}

Color
GraphicsLayer::backgroundColor() const
{
    return m_private.backgroundColor();
}

bool
GraphicsLayer::contentsOpaque() const
{
    return m_private.contentsOpaque();
}

bool
GraphicsLayer::backfaceVisibility() const
{
    return m_private.backfaceVisibility();
}

void
GraphicsLayer::setNeedsDisplay() const
{
    m_private.setNeedsDisplay();
}

GraphicsLayer*
GraphicsLayer::maskLayer()
{
    return m_private.maskLayer();
}

Color 
GraphicsLayer::contentsSolidColor() const
{
    return m_private.contentsSolidColor();
}

const char*
GraphicsLayer::name() const
{
    return m_private.name();
}

void
GraphicsLayer::disposeAllButDescendantsOf(GraphicsLayer* in_layer)
{
    GraphicsLayerPrivate* p = in_layer ? &in_layer->m_private : 0;
    GraphicsLayerPrivate::disposeAllButDescendantsOf(p);
}

bool
GraphicsLayer::isVideo() const
{
    return m_private.isVideo();
}

int
GraphicsLayer::zIndex() const
{
    return m_private.zIndex();
}

} // namespace


/*
 * Copyright (c) 2011-2021 ACCESS CO., LTD. All rights reserved.
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
#include "RenderLayerCompositor.h"
#include "RenderView.h"
#include "RenderVideo.h"
#include "ScrollingConstraints.h"
#include <wtf/HashSet.h>

#include <wkc/wkcgpeer.h>

#include "helpers/privates/WKCHelpersEnumsPrivate.h"

typedef HashSet<WKC::GraphicsLayerPrivate*> LayerSet;
WKC_DEFINE_GLOBAL_TYPE_ZERO(HashSet<WKC::GraphicsLayerPrivate*>*, gLayerSet);

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
    , m_isFixedPositioned(false)
    , m_isStickilyPositioned(false)
    , m_marked(false)
    , m_propertyTopValue(0)
    , m_propertyRightValue(0)
    , m_propertyBottomValue(0)
    , m_propertyLeftValue(0)
    , m_marginTop(0)
    , m_marginRight(0)
    , m_marginBottom(0)
    , m_marginLeft(0)
    , m_propertyTopType(LengthType::Undefined)
    , m_propertyRightType(LengthType::Undefined)
    , m_propertyBottomType(LengthType::Undefined)
    , m_propertyLeftType(LengthType::Undefined)
    , m_marginTopType(LengthType::Undefined)
    , m_marginRightType(LengthType::Undefined)
    , m_marginBottomType(LengthType::Undefined)
    , m_marginLeftType(LengthType::Undefined)
    , m_solidColor()
{
    WKCSize_Set(&m_offscreenSize, 0, 0);
    WKCRect_MakeEmpty(&m_stickyBoxRect);
    WKCRect_MakeEmpty(&m_stickyContainingBlockRect);
    WKCSize_Set(&m_stickyPositionOffset, 0, 0);
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
    WebCore::FloatRect floatRect(rect.fX, rect.fY, rect.fWidth, rect.fHeight);
    gc.clip(floatRect);

    //FIXME: Handle partial repaints, by clearing only the rect invalidated by setNeedsDisplayInRect
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
    const WebCore::FloatRect real_rect(rect.fX - offset.fX, rect.fY - offset.fY, rect.fWidth, rect.fHeight);
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
    gc.clip(WebCore::FloatRect(0, 0, img->width(), img->height()));
    gc.clearRect(WebCore::FloatRect(0, 0, img->width(), img->height()));
    gc.drawImage(*img, WebCore::FloatPoint(0,0));
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

void*
GraphicsLayerPrivate::platformPlayer() const
{
    WebCore::GraphicsLayerClient& client = m_webcore->client();
    WebCore::RenderLayer* renderLayer = client.owningLayerPtr();
    if (!renderLayer)
        return nullptr;
    if (is<WebCore::RenderVideo>(renderLayer->renderer()) && downcast<WebCore::RenderVideo>(renderLayer->renderer()).shouldDisplayVideo()) {
        auto* mediaElement = downcast<WebCore::HTMLMediaElement>(renderLayer->renderer().element());
        return mediaElement->platformPlayer();
    }
    return nullptr;
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

const WKCFloatRect
GraphicsLayerPrivate::contentsRect() const
{
    return m_webcore->contentsRect();
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
GraphicsLayerPrivate::flushCompositingState(const WebCore::FloatRect& clipRect)
{
    m_webcore->flushCompositingStateForThisLayerOnly();

    size_t children_size = childrenSize();
    for (size_t i = 0; i < children_size; i++) {
        WebCore::GraphicsLayer* layer = m_webcore->children().at(i);
        if (!layer) {
            ASSERT(false);
            continue;
        }
        GraphicsLayerPrivate* child = GraphicsLayerWKC_wkc(layer);
        child->flushCompositingState(clipRect);
    }

    updatePosition();

    m_webcore->client().didCommitChangesForLayer(m_webcore);
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
        disposeOffscreen();
        return;
    }
    addDirtyRect();
}

// Currently, we can use WebCore::Length directly since the length values are only used in this file.
// But here we introduce the own LengthType, since the length values may be exposed to applications.
static bool
setLength(WebCore::Length len, LengthType& type, float& value)
{
    LengthType old_type = type;
    float old_value = value;

    switch (len.type()) {
    case WebCore::Percent:
        type = LengthType::Percent;
        value = len.percent();
        break;
    case WebCore::Fixed:
        type = LengthType::Fixed;
        value = len.value();
        break;
    case WebCore::Auto:
        type = LengthType::Undefined;
        value = 0;
        break;
    case WebCore::Calculated:
        type = LengthType::Calculated;
        value = len.calculationValue().evaluate(std::numeric_limits<float>::max());
        break;
    default:
        ASSERT(false);
        type = LengthType::Undefined;
        value = 0;
        break;
    }

    return (old_type != type) || (old_value != value);
}

void
GraphicsLayerPrivate::updatePosition()
{
    m_isFixedPositioned = false;
    m_isStickilyPositioned = false;

    WKCRect_MakeEmpty(&m_stickyBoxRect);
    WKCRect_MakeEmpty(&m_stickyContainingBlockRect);
    WKCSize_Set(&m_stickyPositionOffset, 0, 0);

    m_propertyTopType = LengthType::Undefined;
    m_propertyRightType = LengthType::Undefined;
    m_propertyBottomType = LengthType::Undefined;
    m_propertyLeftType = LengthType::Undefined;

    m_propertyTopValue = 0.0f;
    m_propertyRightValue = 0.0f;
    m_propertyBottomValue = 0.0f;
    m_propertyLeftValue = 0.0f;

    WebCore::GraphicsLayerClient& client = m_webcore->client();
    WebCore::RenderLayer* renderLayer = client.owningLayerPtr();
    if (!renderLayer)
        return;

    m_isFixedPositioned = renderLayer->renderer().isFixedPositioned();
    m_isStickilyPositioned = renderLayer->renderer().isStickilyPositioned();

    if (!m_isFixedPositioned && !m_isStickilyPositioned)
        return;

    if (m_isStickilyPositioned) {
        if (renderLayer->renderBox()) {
            WebCore::StickyPositionViewportConstraints constraints;
            WebCore::FloatRect constrainingRect = renderLayer->renderBox()->constrainingRectForStickyPosition();
            renderLayer->renderBox()->computeStickyPositionConstraints(constraints, constrainingRect);
            m_stickyBoxRect = WebCore::enclosingIntRect(constraints.stickyBoxRect());
            m_stickyContainingBlockRect = WebCore::enclosingIntRect(constraints.containingBlockRect());
            m_stickyPositionOffset = WebCore::expandedIntSize(constraints.computeStickyOffset(constrainingRect));
        }
    }

    const WebCore::RenderStyle& style = renderLayer->renderer().style();

    setLength(style.left(), m_propertyLeftType, m_propertyLeftValue);
    setLength(style.top(), m_propertyTopType, m_propertyTopValue);
    setLength(style.right(), m_propertyRightType, m_propertyRightValue);
    setLength(style.bottom(), m_propertyBottomType, m_propertyBottomValue);
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
GraphicsLayer::isPropertyLeftDefined() const
{
    return m_private.isPropertyLeftDefined();
}

bool
GraphicsLayer::isPropertyRightDefined() const
{
    return m_private.isPropertyRightDefined();
}

bool
GraphicsLayer::isPropertyTopDefined() const
{
    return m_private.isPropertyTopDefined();
}

bool
GraphicsLayer::isPropertyBottomDefined() const
{
    return m_private.isPropertyBottomDefined();
}

WKCRect
GraphicsLayer::stickyBoxRect() const
{
    return m_private.stickyBoxRect();
}

WKCRect
GraphicsLayer::stickyContainingBlockRect() const
{
    return m_private.stickyContainingBlockRect();
}

WKCSize
GraphicsLayer::stickyPositionOffset() const
{
    return m_private.stickyPositionOffset();
}

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
    return m_webcore->contentsLayerPurpose() == WebCore::GraphicsLayer::ContentsLayerForMedia;
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

const char*
GraphicsLayerPrivate::name()
{
    m_name = m_webcore->name();
    return m_name.utf8().data();
}

const char*
GraphicsLayerPrivate::logReasonsForCompositing()
{
    WebCore::GraphicsLayerClient& client = m_webcore->client();
    WebCore::RenderLayer* renderLayer = client.owningLayerPtr();
    if (!renderLayer)
        return "";
    WebCore::RenderView* view = renderLayer->renderer().document().renderView();
    if (!view)
        return "";
    return view->compositor().logReasonsForCompositing(*renderLayer);
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

void*
GraphicsLayer::platformPlayer() const
{
    return m_private.platformPlayer();
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

const WKCFloatRect
GraphicsLayer::contentsRect() const
{
    return m_private.contentsRect();
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
GraphicsLayer::isFixedPositioned() const
{
    return m_private.isFixedPositioned();
}

bool
GraphicsLayer::isStickilyPositioned() const
{
    return m_private.isStickilyPositioned();
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

const char*
GraphicsLayer::name() const
{
    return m_private.name();
}

const char*
GraphicsLayer::logReasonsForCompositing()
{
    return m_private.logReasonsForCompositing();
}

} // namespace


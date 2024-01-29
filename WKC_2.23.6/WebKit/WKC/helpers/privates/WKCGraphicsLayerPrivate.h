/*
 * Copyright (c) 2011-2022 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_GRAPHICSLAYER_H_
#define _WKC_HELPERS_PRIVATE_GRAPHICSLAYER_H_

#include "helpers/WKCGraphicsLayer.h"

#include "helpers/WKCString.h"
#include <wkc/wkcgpeer.h>

#include "Vector.h"

namespace WebCore {
class GraphicsLayer;
class Image;
class FloatRect;
class Color;
} // namespace

namespace WKC {

enum class LengthType {
    Undefined,
    Percent,
    Fixed,
    Calculated
};

class GraphicsLayer;

class GraphicsLayerWrap : public GraphicsLayer {
WTF_MAKE_FAST_ALLOCATED;
public:
    GraphicsLayerWrap(GraphicsLayerPrivate& parent) : GraphicsLayer(parent) {}
    ~GraphicsLayerWrap() {}
};

class GraphicsLayerPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    GraphicsLayerPrivate(WebCore::GraphicsLayer*);
    ~GraphicsLayerPrivate();

    WebCore::GraphicsLayer* webcore() const { return m_webcore; }
    GraphicsLayer& wkc() { return m_wkc; }

    bool preserves3D() const;
    bool masksToBounds() const;
    bool isRootClippingLayer() const;
    bool contentsAreVisible() const;
    bool drawsContent() const;
    Color backgroundColor() const;
    bool contentsOpaque() const;
    bool backfaceVisibility() const;
    float opacity() const;
    BlendMode blendMode() const;
    bool requiresCompositingForWillChange() const;

    void clear(const WKCRect&, void*);
    void paint(const WKCRect&);
    void paint(const WKCRect&, const WKCPoint&, float, GraphicsLayerPrivate*, void*);
    WKCRect lastPaintedRect() const;

    void layerDidDisplay(void*);
    void* platformLayer() const;
    void* platformPlayer() const;

    size_t dirtyRectsSize() const;  // incremented by addDirtyRect() and reset by paint()
    void dirtyRects(int, WKCFloatRect&) const;
    bool addDirtyRect(const WKCFloatRect&);
    void addDirtyRect();
    void resetDirtyRects();

    GraphicsLayer* parent() const;
    size_t childrenSize() const;
    GraphicsLayer* children(int);

    const WKCFloatSize size() const;
    const WKCFloatPoint position() const;
    const WKCFloatPoint anchorPoint(float&) const;
    const WKCFloatRect contentsRect() const;
    void transform(double*) const;
    void childrenTransform(double*) const;

    void setNeedsDisplay();
    GraphicsLayer* maskLayer();

    void flushCompositingState(const WebCore::FloatRect& clipRect);

    void layerDidAttachToTree(GraphicsLayerPrivate* parent);
    void layerDidDetachFromTree();

    void setOpticalZoom(float zoom);

    void setIsFixedPositioned(bool isFixedPositioned) { m_isFixedPositioned = isFixedPositioned; }
    bool isFixedPositioned() const { return m_isFixedPositioned; }

    void setIsStickilyPositioned(bool isStickilyPositioned) { m_isStickilyPositioned = isStickilyPositioned; }
    bool isStickilyPositioned() const { return m_isStickilyPositioned; }

    void setPropertyLeftType(LengthType type) { m_propertyLeftType = type; }
    void setPropertyRightType(LengthType type) { m_propertyRightType = type; }
    void setPropertyTopType(LengthType type) { m_propertyTopType = type; }
    void setPropertyBottomType(LengthType type) { m_propertyBottomType = type; }

    bool isPropertyLeftDefined() const { return m_propertyLeftType != LengthType::Undefined; }
    bool isPropertyRightDefined() const { return m_propertyRightType != LengthType::Undefined; }
    bool isPropertyTopDefined() const { return m_propertyTopType != LengthType::Undefined; }
    bool isPropertyBottomDefined() const { return m_propertyBottomType != LengthType::Undefined; }

    WKCRect stickyBoxRect() const { return m_stickyBoxRect; }
    WKCRect stickyContainingBlockRect() const { return m_stickyContainingBlockRect; }
    WKCSize stickyPositionOffset() const { return m_stickyPositionOffset; }

    bool isVideo() const;
    int zIndex() const;

    // for debug
    const char* name();
    const char* logReasonsForCompositing();

public:
    bool ensureOffscreen(int, int);
    void disposeOffscreen();

    inline void* offscreenLayer() const { return m_offscreenLayer; }
    void setContentsToImage(WebCore::Image*);
    void setContentsToSolidColor(const WebCore::Color&);
    Color contentsSolidColor() const;

    static void disposeAllButDescendantsOf(GraphicsLayerPrivate*);

private:
    void updatePosition();
    void markDescendants();

private:
    WebCore::GraphicsLayer* m_webcore;
    GraphicsLayerWrap m_wkc;

    WKCSize m_offscreenSize;
    void* m_offscreenLayer;
    bool m_offscreenLayerIsImage;
    float m_opticalzoom;

    WTF::Vector<WKCFloatRect> m_dirtyrects;

    bool m_needsDisplay;

    bool m_isFixedPositioned;
    bool m_isStickilyPositioned;
    WKCRect m_stickyBoxRect;
    WKCRect m_stickyContainingBlockRect;
    WKCSize m_stickyPositionOffset;
    float m_propertyTopValue, m_propertyRightValue, m_propertyBottomValue, m_propertyLeftValue;
    float m_marginTop, m_marginRight, m_marginBottom, m_marginLeft;
    LengthType m_propertyTopType, m_propertyRightType, m_propertyBottomType, m_propertyLeftType;
    LengthType m_marginTopType, m_marginRightType, m_marginBottomType, m_marginLeftType;
    Color m_solidColor;

    bool m_marked; // used only by disposeOffscreenLayer() and markDescendants()

    WKCRect m_lastPaintedRect;
    String m_name;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_GRAPHICSLAYER_H_

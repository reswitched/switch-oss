/*
 * Copyright (c) 2011-2016 ACCESS CO., LTD. All rights reserved.
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

enum LengthType {
    LengthTypeUndefined,
    LengthTypePercent,
    LengthTypeFixed,
    LengthTypeCalculated
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

    void clear(const WKCRect&, void*);
    void paint(const WKCRect&);
    void paint(const WKCRect&, const WKCPoint&, float, GraphicsLayerPrivate*, void*);
    WKCRect lastPaintedRect() const;

    void layerDidDisplay(void*);
    void* platformLayer() const;

    size_t dirtyRectsSize() const;  // incremented by addDirtyRect() and reset by paint()
    void dirtyRects(int, WKCFloatRect&) const;
    bool addDirtyRect(const WKCFloatRect&);
    bool addDirtyRect();
    void resetDirtyRects();

    GraphicsLayer* parent() const;
    size_t childrenSize() const;
    GraphicsLayer* children(int);

    const WKCFloatSize size() const;
    const WKCFloatPoint position() const;
    const WKCFloatPoint anchorPoint(float&) const;
    void transform(double*) const;
    void childrenTransform(double*) const;

    void setNeedsDisplay();
    GraphicsLayer* maskLayer();

    void flushCompositingState(const WebCore::FloatRect& clipRect, bool viewportIsStable);

    void layerDidAttachToTree(GraphicsLayerPrivate* parent);
    void layerDidDetachFromTree();

    void setOpticalZoom(float zoom);

    void setIsFixed(bool isFixed) { m_isFixed = isFixed; }
    bool isFixed() const { return m_isFixed; }

    void setLeftType(LengthType type) { m_leftType = type; }
    void setRightType(LengthType type) { m_rightType = type; }
    void setTopType(LengthType type) { m_topType = type; }
    void setBottomType(LengthType type) { m_bottomType = type; }

    bool isLeftDefined() const { return m_leftType != LengthTypeUndefined; }     // May true only when isFixed() is true
    bool isRightDefined() const { return m_rightType != LengthTypeUndefined; }   // Ditto.
    bool isTopDefined() const { return m_topType != LengthTypeUndefined; }       // 
    bool isBottomDefined() const { return m_bottomType != LengthTypeUndefined; } // 

    const char* name();

    bool isVideo() const;

public:
    bool ensureOffscreen(int, int);
    void disposeOffscreen();

    inline void* offscreenLayer() const { return m_offscreenLayer; }
    void setContentsToImage(WebCore::Image*);
    void setContentsToSolidColor(const WebCore::Color&);
    Color contentsSolidColor() const;

    static void disposeAllButDescendantsOf(GraphicsLayerPrivate*);

private:
    void updateFixedPosition();
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

    bool m_isFixed;
    float m_top, m_right, m_bottom, m_left;
    float m_marginTop, m_marginRight, m_marginBottom, m_marginLeft;
    LengthType m_topType, m_rightType, m_bottomType, m_leftType;
    LengthType m_marginTopType, m_marginRightType, m_marginBottomType, m_marginLeftType;
    WKCRect m_fixedRect;
    WKCPoint m_renderLayerPos;
    Color m_solidColor;

    bool m_marked; // used only by disposeOffscreenLayer() and markDescendants()

    WKCRect m_lastPaintedRect;
    String m_name;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_GRAPHICSLAYER_H_

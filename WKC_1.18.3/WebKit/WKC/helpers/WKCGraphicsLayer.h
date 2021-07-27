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

#ifndef _WKC_HELPERS_WKC_GRAPHICSLAYER_H_
#define _WKC_HELPERS_WKC_GRAPHICSLAYER_H_

#include <wkc/wkcbase.h>
#include <helpers/WKCColor.h>

#include "helpers/WKCHelpersEnums.h"

namespace WKC {

class GraphicsLayerPrivate;

class WKC_API GraphicsLayer {
public:
    bool preserves3D() const;
    bool masksToBounds() const;
    bool contentsAreVisible() const;
    bool drawsContent() const;
    bool isRootClippingLayer() const;
    Color backgroundColor() const;
    bool contentsOpaque() const;
    bool backfaceVisibility() const;
    float opacity() const;
    BlendMode blendMode() const;

    void clear(const WKCRect&, void*);
    void paint(const WKCRect&);
    void paint(const WKCRect&, const WKCPoint&, float, GraphicsLayer*, void*);
    WKCRect lastPaintedRect() const;
    void layerDidDisplay(void*);
    void* platformLayer() const;
    void* platformPlayer() const;
    bool ensureOffscreen(int, int);
    void disposeOffscreen();

    size_t dirtyRectsSize() const;
    void dirtyRects(int, WKCFloatRect&) const;
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
    void setNeedsDisplay() const;
    GraphicsLayer* maskLayer();

    void setOpticalZoom(float zoom);

    bool isFixedPositioned() const;
    bool isStickilyPositioned() const;

    bool isPropertyLeftDefined() const;
    bool isPropertyRightDefined() const;
    bool isPropertyTopDefined() const;
    bool isPropertyBottomDefined() const;

    WKCRect stickyBoxRect() const;
    WKCRect stickyContainingBlockRect() const;
    WKCSize stickyPositionOffset() const;

    Color contentsSolidColor() const;

    bool isVideo() const;
    int zIndex() const;

    static void disposeAllButDescendantsOf(GraphicsLayer*);

    // for debug
    const char* name() const;
    const char* logReasonsForCompositing();

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    GraphicsLayer(GraphicsLayerPrivate&);
    ~GraphicsLayer();

private:
    GraphicsLayer(const GraphicsLayer&);
    GraphicsLayer& operator=(const GraphicsLayer&);

    GraphicsLayerPrivate& m_private;
};
} // namespace

#endif // _WKC_HELPERS_WKC_GRAPHICSLAYER_H_

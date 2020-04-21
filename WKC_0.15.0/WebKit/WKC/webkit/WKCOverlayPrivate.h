/*
 * WKCOverlayPrivate.h
 *
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (c) 2013, 2015 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef WKCOverlayPrivate_h
#define WKCOverlayPrivate_h

#if USE(ACCELERATED_COMPOSITING)
#include "GraphicsLayer.h"
#include "GraphicsLayerClient.h"
#endif

#include <wtf/Vector.h>

// class definition

namespace WebCore {
class FloatSize;
class GraphicsContext;
class IntRect;
}

namespace WKC {
class WKCWebViewPrivate;
class WKCOverlayIf;
class WKCOverlayGraphicsLayerClient;

enum FixedDirection {
    EFixedDirectionNone   = 0,
    EFixedDirectionLeft   = 1 << 0,
    EFixedDirectionRight  = 1 << 1,
    EFixedDirectionTop    = 1 << 2,
    EFixedDirectionBottom = 1 << 3
};

class WKCOverlayInternal : public WTF::RefCounted<WKCOverlayInternal> {
public:
    static WTF::PassRefPtr<WKCOverlayInternal> create(WKCWebViewPrivate*, WKCOverlayIf*);

    ~WKCOverlayInternal();

    WKCOverlayIf* overlay() const { return m_overlay; }
    void setZOrder(int zOrder) { m_zOrder = zOrder; }
    void setFixedDirection(int fixedDirectionFlag) { m_fixedDirectionFlag = fixedDirectionFlag; }
    void clear();
    void update(const WebCore::IntRect& rect, bool immediate);
    void paintOffscreen(WebCore::GraphicsContext&);

private:
    WKCOverlayInternal(WKCWebViewPrivate*, WKCOverlayIf*);

    void invalidateAll();

    WKCWebViewPrivate* m_view;
    WKCOverlayIf* m_overlay;
#if USE(ACCELERATED_COMPOSITING)
    WTF::RefPtr<WKCOverlayGraphicsLayerClient> m_layerClient;
    std::unique_ptr<WebCore::GraphicsLayer> m_layer;
#endif
    int m_zOrder;
    int m_fixedDirectionFlag;
    bool m_isUpdating;
};

class WKCOverlayList : public WTF::RefCounted<WKCOverlayList> {
public:
    static PassRefPtr<WKCOverlayList> create(WKCWebViewPrivate*);

    ~WKCOverlayList();

    bool empty() const { return !m_list.size(); }
    bool add(WKCOverlayIf* overlay, int zOrder, int fixedDirectionFlag);
    bool remove(WKCOverlayIf* overlay);
    void update(const WebCore::IntRect& rect, bool immediate);
    void paintOffscreen(WebCore::GraphicsContext& ctx);

private:
    explicit WKCOverlayList(WKCWebViewPrivate*);

    size_t find(WKCOverlayIf*);

    WKCWebViewPrivate* m_view;
    WTF::Vector<RefPtr<WKCOverlayInternal> > m_list;
    bool m_isUpdating;
};

} // namespace

#endif  // WKCOverlayPrivate_h

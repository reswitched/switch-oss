/*
 * Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_RENDERVIEW_H_
#define _WKC_HELPERS_PRIVATE_RENDERVIEW_H_

#include "helpers/WKCRenderView.h"

namespace WebCore {
class RenderView;
} // namespace

namespace WKC {

class RenderLayer;
class RenderLayerPrivate;

class RenderViewWrap : public RenderView {
WTF_MAKE_FAST_ALLOCATED;
public:
    RenderViewWrap(RenderViewPrivate& parent) : RenderView(parent) {}
    ~RenderViewWrap() {}
};

class RenderViewPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    RenderViewPrivate(WebCore::RenderView*);
    ~RenderViewPrivate();

    WebCore::RenderView* webcore() const { return m_webcore; }
    RenderView& wkc() { return m_wkc; }

    void setNeedsLayout(bool);
    RenderLayer* layer();
    WKCRect documentRect() const;

private:
    WebCore::RenderView* m_webcore;
    RenderViewWrap m_wkc;

    RenderLayerPrivate* m_layer;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_RENDERVIEW_H_


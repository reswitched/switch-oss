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

#ifndef _WKC_HELPERS_PRIVATE_RENDERLAYER_H_
#define _WKC_HELPERS_PRIVATE_RENDERLAYER_H_

#include "helpers/WKCRenderLayer.h"

namespace WebCore {
class RenderLayer;
} // namespace

namespace WKC {

class HitTestRequest;
class HitTestResult;

class RenderLayerWrap : public RenderLayer {
WTF_MAKE_FAST_ALLOCATED;
public:
    RenderLayerWrap(RenderLayerPrivate& parent) : RenderLayer(parent) {}
    ~RenderLayerWrap() {}
};

class RenderLayerPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    RenderLayerPrivate(WebCore::RenderLayer*);
    ~RenderLayerPrivate();

    WebCore::RenderLayer* webcore() const { return m_webcore; }
    RenderLayer& wkc() { return m_wkc; }

    bool hitTest(const HitTestRequest&, HitTestResult&);

private:
    WebCore::RenderLayer* m_webcore;
    RenderLayerWrap m_wkc;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_RENDERLAYER_H_


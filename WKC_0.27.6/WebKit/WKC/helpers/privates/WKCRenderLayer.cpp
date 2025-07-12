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

#include "config.h"

#include "helpers/WKCRenderLayer.h"
#include "helpers/privates/WKCRenderLayerPrivate.h"

#include "RenderLayer.h"

#include "helpers/WKCHitTestRequest.h"
#include "helpers/WKCHitTestResult.h"
#include "helpers/privates/WKCHitTestRequestPrivate.h"
#include "helpers/privates/WKCHitTestResultPrivate.h"

namespace WKC {
RenderLayerPrivate::RenderLayerPrivate(WebCore::RenderLayer* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

RenderLayerPrivate::~RenderLayerPrivate()
{
}

bool
RenderLayerPrivate::hitTest(const HitTestRequest& req, HitTestResult& result)
{
    return m_webcore->hitTest(*req.priv()->webcore(), const_cast<WebCore::HitTestResult&>(result.priv()->webcore()));
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

RenderLayer::RenderLayer(RenderLayerPrivate& parent)
    : m_private(parent)
{
}

RenderLayer::~RenderLayer()
{
}

bool
RenderLayer::hitTest(const HitTestRequest& req, HitTestResult& res)
{
    return m_private.hitTest(req, res);
}

} // namespace

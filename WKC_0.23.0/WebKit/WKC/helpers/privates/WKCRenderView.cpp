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

#include "helpers/WKCRenderView.h"
#include "helpers/privates/WKCRenderViewPrivate.h"

#include "RenderView.h"

#include "helpers/privates/WKCRenderLayerPrivate.h"

namespace WKC {

RenderViewPrivate::RenderViewPrivate(WebCore::RenderView* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_layer(0)
{
}

RenderViewPrivate::~RenderViewPrivate()
{
    delete m_layer;
}

void
RenderViewPrivate::setNeedsLayout(bool flag)
{
    m_webcore->setNeedsLayout(flag ? WebCore::MarkOnlyThis : WebCore::MarkContainingBlockChain);
}

RenderLayer*
RenderViewPrivate::layer()
{
    WebCore::RenderLayer* l = m_webcore->layer();
    if (!l)
        return 0;

    if (!m_layer || m_layer->webcore()!=l) {
        delete m_layer;
        m_layer = new RenderLayerPrivate(l);
    }
    return &m_layer->wkc();
}

WKCRect
RenderViewPrivate::documentRect() const
{
    return m_webcore->documentRect();
}

////////////////////////////////////////////////////////////////////////////////

RenderView::RenderView(RenderViewPrivate& parent)
    : m_private(parent)
{
}

RenderView::~RenderView()
{
}

void
RenderView::setNeedsLayout(bool flag)
{
    m_private.setNeedsLayout(flag);
}

RenderLayer*
RenderView::layer() const
{
    return m_private.layer();
}

WKCRect
RenderView::documentRect() const
{
    return m_private.documentRect();
}

} // namespace

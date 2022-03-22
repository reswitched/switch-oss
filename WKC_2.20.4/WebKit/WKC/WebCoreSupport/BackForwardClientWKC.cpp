/*
 * Copyright (c) 2013-2019 ACCESS CO., LTD. All rights reserved.
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

#include "HistoryItem.h"

#include "BackForwardClientWKC.h"
#include "WKCWebViewPrivate.h"

#include "helpers/BackForwardClientIf.h"

#include "helpers/privates/WKCHistoryItemPrivate.h"

namespace WKC {

BackForwardClientWKC::BackForwardClientWKC(WKCWebViewPrivate* view)
    : m_view(view)
    , m_appClient(0)
{
}

BackForwardClientWKC::~BackForwardClientWKC()
{
    if (m_appClient) {
        m_view->clientBuilders().deleteBackForwardClient(m_appClient);
        m_appClient = 0;
    }
}

BackForwardClientWKC*
BackForwardClientWKC::create(WKCWebViewPrivate* view)
{
    BackForwardClientWKC* self = 0;
    self = new BackForwardClientWKC(view);
    if (!self) return nullptr;
    if (!self->construct()) {
        delete self;
        return nullptr;
    }
    return self;
}

bool
BackForwardClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createBackForwardClient(m_view->parent());
    if (!m_appClient) return false;
    return true;
}

void
BackForwardClientWKC::addItem(Ref<WebCore::HistoryItem>&& prpItem)
{
    HistoryItemPrivate wobj(prpItem.ptr());
    m_appClient->addItem(&wobj.wkc());
}

void
BackForwardClientWKC::goToItem(WebCore::HistoryItem& item)
{
    HistoryItemPrivate wobj(&item);
    m_appClient->goToItem(&wobj.wkc());
}

RefPtr<WebCore::HistoryItem>
BackForwardClientWKC::itemAtIndex(int index)
{
    HistoryItem* item = m_appClient->itemAtIndex(index);
    if (item)
        return item->priv().webcore();
    return 0;
}

unsigned
BackForwardClientWKC::backListCount() const
{
    return m_appClient->backListCount();
}

unsigned
BackForwardClientWKC::forwardListCount() const
{
    return m_appClient->forwardListCount();
}

void
BackForwardClientWKC::close()
{
    m_appClient->close();
}


} // namespace


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

#include "helpers/WKCBackForwardList.h"

#include "BackForwardListWKC.h"
#include "HistoryItem.h"

#include "helpers/WKCHistoryItem.h"
#include "helpers/privates/WKCBackForwardListPrivate.h"
#include "helpers/privates/WKCHistoryItemPrivate.h"

namespace WKC {

BackForwardListPrivate::BackForwardListPrivate(WebCore::BackForwardList* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_itemAtIndex(0)
{
}

BackForwardListPrivate::~BackForwardListPrivate()
{
    delete m_itemAtIndex;
}

HistoryItem*
BackForwardListPrivate::itemAtIndex(int idx)
{
    RefPtr<WebCore::HistoryItem> i = m_webcore->itemAtIndex(idx);
    if (!i)
        return 0;

    if (!m_itemAtIndex || m_itemAtIndex->webcore()!=i) {
        delete m_itemAtIndex;
        m_itemAtIndex = new HistoryItemPrivate(i.get());
    }
    return &m_itemAtIndex->wkc();
}

void
BackForwardListPrivate::addItem(HistoryItem* item)
{
    if (!item)
        return;

    m_webcore->addItem(WTF::Ref<WebCore::HistoryItem>(*item->priv().webcore()));
}

int
BackForwardListPrivate::backListCount()
{
    return m_webcore->backListCount();
}

int
BackForwardListPrivate::forwardListCount()
{
    return m_webcore->forwardListCount();
}

void
BackForwardListPrivate::setCapacity(int count)
{
    static_cast<WebCore::BackForwardList *>(m_webcore)->setCapacity(count);
}

////////////////////////////////////////////////////////////////////////////////

BackForwardList::BackForwardList(BackForwardListPrivate& parent)
    : m_private(parent)
{
}

BackForwardList::~BackForwardList()
{
}


HistoryItem*
BackForwardList::itemAtIndex(int index)
{
    return m_private.itemAtIndex(index);
}

void
BackForwardList::addItem(HistoryItem* item)
{
    m_private.addItem(item);
}


int
BackForwardList::backListCount()
{
    return m_private.backListCount();
}

int
BackForwardList::forwardListCount()
{
    return m_private.forwardListCount();
}

void
BackForwardList::setCapacity(int count)
{
    m_private.setCapacity(count);
}

} // namespace

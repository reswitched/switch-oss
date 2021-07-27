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

#include "config.h"

#include "helpers/WKCHTMLCollection.h"
#include "helpers/privates/WKCHTMLCollectionPrivate.h"

#include "HTMLCollection.h"

#include "helpers/privates/WKCNodePrivate.h"

namespace WKC {

HTMLCollectionPrivate::HTMLCollectionPrivate(RefPtr<WebCore::HTMLCollection>& parent)
     : m_webcore(parent.get())
     , m_wkc(*this)
     , m_refptr(parent)
     , m_node(0)
{
}

HTMLCollectionPrivate::~HTMLCollectionPrivate()
{
    delete m_node;
}

void
HTMLCollectionPrivate::release()
{
    delete this;
}

unsigned
HTMLCollectionPrivate::length() const
{
    return m_webcore->length();
}

Node*
HTMLCollectionPrivate::item(unsigned index)
{
    WebCore::Node* node = m_webcore->item(index);
    if (!node)
        return 0;
    if (!m_node || m_node->webcore() != node) {
        delete m_node;
        m_node = NodePrivate::create(node);
    }
    return &m_node->wkc();
}

Node*
HTMLCollectionPrivate::firstItem()
{
    return this->item(0);
}

////////////////////////////////////////////////////////////////////////////////

HTMLCollection::HTMLCollection(HTMLCollectionPrivate& parent)
     : m_private(parent)
{
}

HTMLCollection::~HTMLCollection()
{
}

void
HTMLCollection::release()
{
    return m_private.release();
}

unsigned
HTMLCollection::length() const
{
    return m_private.length();
}

Node*
HTMLCollection::item(unsigned index)
{
    return m_private.item(index);
}

Node*
HTMLCollection::firstItem()
{
    return m_private.firstItem();
}

} // namespace

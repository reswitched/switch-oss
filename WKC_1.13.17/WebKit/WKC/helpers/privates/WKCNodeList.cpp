/*
 * Copyright (c) 2013-2016 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCNodeList.h"
#include "helpers/privates/WKCNodeListPrivate.h"

#include "NodeList.h"

#include "helpers/privates/WKCNodePrivate.h"

namespace WKC {

// Private Implementation

NodeListPrivate::NodeListPrivate(RefPtr<WebCore::NodeList> parent)
     : m_webcore(parent.get())
     , m_wkc(*this)
     , m_refptr(parent)
     , m_node(0)
{
}

NodeListPrivate::~NodeListPrivate()
{
    delete m_node;
}

void
NodeListPrivate::release()
{
    delete this;
}

unsigned
NodeListPrivate::length() const
{
    return m_webcore->length();
}

Node*
NodeListPrivate::item(unsigned index)
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

// Implementation

NodeList::NodeList(NodeListPrivate& parent)
     : m_private(parent)
{
}

NodeList::~NodeList()
{
}

void
NodeList::release()
{
    return m_private.release();
}

unsigned
NodeList::length() const
{
    return m_private.length();
}

Node*
NodeList::item(unsigned index) const
{
    return m_private.item(index);
}

} // namespace

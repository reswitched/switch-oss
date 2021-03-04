/*
 * Copyright (c) 2011-2018 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCEventTarget.h"
#include "helpers/privates/WKCEventTargetPrivate.h"

#include "helpers/privates/WKCNodePrivate.h"

#include "EventTarget.h"
#include "Node.h"

namespace WKC {

EventTargetPrivate::EventTargetPrivate(WebCore::EventTarget* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_node(0)
{
}

EventTargetPrivate::~EventTargetPrivate()
{
    delete m_node;
}

Node*
EventTargetPrivate::toNode()
{
    if (!m_webcore->isNode())
        return 0;

    WebCore::Node* n = downcast<WebCore::Node>(m_webcore);
    if (!m_node || m_node->webcore()!=n) {
        delete m_node;
        m_node = NodePrivate::create(n);
    }
    return &m_node->wkc();
}

////////////////////////////////////////////////////////////////////////////////

EventTarget::EventTarget(EventTargetPrivate& parent)
    : m_private(parent)
{
}

EventTarget::~EventTarget()
{
}

Node*
EventTarget::toNode()
{
    return m_private.toNode();
}

} // namespace

/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2004, 2008 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "Traversal.h"

#include "Node.h"
#include "NodeFilter.h"

namespace WebCore {

NodeIteratorBase::NodeIteratorBase(PassRefPtr<Node> rootNode, unsigned whatToShow, PassRefPtr<NodeFilter> nodeFilter, bool expandEntityReferences)
    : m_root(rootNode)
    , m_whatToShow(whatToShow)
    , m_filter(nodeFilter)
    , m_expandEntityReferences(expandEntityReferences)
{
}

short NodeIteratorBase::acceptNode(JSC::ExecState* state, Node* node) const
{
    // FIXME: To handle XML properly we would have to check m_expandEntityReferences.

    // The bit twiddling here is done to map DOM node types, which are given as integers from
    // 1 through 14, to whatToShow bit masks.
    if (!(((1 << (node->nodeType() - 1)) & m_whatToShow)))
        return NodeFilter::FILTER_SKIP;
    if (!m_filter)
        return NodeFilter::FILTER_ACCEPT;
    return m_filter->acceptNode(state, node);
}

} // namespace WebCore

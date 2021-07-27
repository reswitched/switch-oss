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

#ifndef _WKC_HELPERS_PRIVATE_NODELIST_H_
#define _WKC_HELPERS_PRIVATE_NODELIST_H_

#include "helpers/WKCNodeList.h"

#include <NodeList.h>
#include <wtf/RefPtr.h>

namespace WKC {
class Node;
class NodePrivate;

class NodeListWrap : public NodeList {
WTF_MAKE_FAST_ALLOCATED;
public:
    NodeListWrap(NodeListPrivate& parent) : NodeList(parent) {}
    ~NodeListWrap() {}
};

class NodeListPrivate
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    NodeListPrivate(RefPtr<WebCore::NodeList>);
    virtual ~NodeListPrivate();

    void release();

    WebCore::NodeList* webcore() const { return m_webcore; }
    NodeList& wkc() { return m_wkc; }

    // DOM methods & attributes for NodeList
    virtual unsigned length() const;
    virtual Node* item(unsigned index);

private:
    WebCore::NodeList* m_webcore;
    NodeListWrap m_wkc;
    RefPtr<WebCore::NodeList> m_refptr;

    NodePrivate* m_node;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_NODELIST_H_

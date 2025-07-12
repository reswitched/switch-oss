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

#ifndef _WKC_HELPERS_PRIVATE_HTMLCOLLECTION_H_
#define _WKC_HELPERS_PRIVATE_HTMLCOLLECTION_H_

#include "helpers/WKCHTMLCollection.h"

#include <HTMLCollection.h>

namespace WKC {
class HTMLCollection;
class NodePrivate;

class HTMLCollectionWrap : public HTMLCollection {
WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLCollectionWrap(HTMLCollectionPrivate& parent) : HTMLCollection(parent) {}
    ~HTMLCollectionWrap() {}
};

class HTMLCollectionPrivate
{
WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLCollectionPrivate(RefPtr<WebCore::HTMLCollection>&);
    ~HTMLCollectionPrivate();

    void release();

    WebCore::HTMLCollection* webcore() const { return m_webcore; }
    HTMLCollection& wkc() { return m_wkc; }

    unsigned length() const;
    Node* firstItem();
    Node* item(unsigned index);

private:
    WebCore::HTMLCollection* m_webcore;
    HTMLCollectionWrap m_wkc;
    RefPtr<WebCore::HTMLCollection> m_refptr;

    NodePrivate* m_node;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_HTMLCOLLECTION_H_

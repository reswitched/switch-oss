/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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

#ifndef HTMLNameCollection_h
#define HTMLNameCollection_h

#include "HTMLCollection.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

class Document;

class HTMLNameCollection : public HTMLCollection {
public:
    virtual ~HTMLNameCollection();

    Document& document() { return downcast<Document>(ownerNode()); }

protected:
    HTMLNameCollection(Document&, CollectionType, const AtomicString& name);

    AtomicString m_name;
};

class WindowNameCollection final : public HTMLNameCollection {
public:
    static Ref<WindowNameCollection> create(Document& document, CollectionType type, const AtomicString& name)
    {
        return adoptRef(*new WindowNameCollection(document, type, name));
    }

    bool elementMatches(const Element& element) const { return elementMatches(element, m_name.impl()); }

    static bool elementMatchesIfIdAttributeMatch(const Element&) { return true; }
    static bool elementMatchesIfNameAttributeMatch(const Element&);
    static bool elementMatches(const Element&, const AtomicStringImpl*);

private:
    WindowNameCollection(Document& document, CollectionType type, const AtomicString& name)
        : HTMLNameCollection(document, type, name)
    {
        ASSERT(type == WindowNamedItems);
    }
};

class DocumentNameCollection final : public HTMLNameCollection {
public:
    static Ref<DocumentNameCollection> create(Document& document, CollectionType type, const AtomicString& name)
    {
        return adoptRef(*new DocumentNameCollection(document, type, name));
    }

    static bool elementMatchesIfIdAttributeMatch(const Element&);
    static bool elementMatchesIfNameAttributeMatch(const Element&);
    bool elementMatches(const Element& element) const { return elementMatches(element, m_name.impl()); }

    static bool elementMatches(const Element&, const AtomicStringImpl*);

private:
    DocumentNameCollection(Document& document, CollectionType type, const AtomicString& name)
        : HTMLNameCollection(document, type, name)
    {
        ASSERT(type == DocumentNamedItems);
    }
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_HTMLCOLLECTION(WindowNameCollection, WindowNamedItems)
SPECIALIZE_TYPE_TRAITS_HTMLCOLLECTION(DocumentNameCollection, DocumentNamedItems)

#endif // HTMLNameCollection_h

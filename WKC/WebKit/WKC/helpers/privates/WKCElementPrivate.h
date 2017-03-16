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

#ifndef _WKC_HELPERS_PRIVATE_ELEMENT_H_
#define _WKC_HELPERS_PRIVATE_ELEMENT_H_

#include "helpers/WKCElement.h"
#include "WKCNodePrivate.h"

namespace WebCore {
class Element;
} // namespace

namespace WKC {
class Attribute;
class AttributePrivate;
} // namespace

namespace WKC {

class ElementWrap : public Element {
WTF_MAKE_FAST_ALLOCATED;
public:
    ElementWrap(ElementPrivate& parent) : Element(parent) {}
    ~ElementWrap() {}
};

class ElementPrivate : public NodePrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    static ElementPrivate* create(WebCore::Element*);

    ElementPrivate(WebCore::Element*);
    virtual ~ElementPrivate();

    WebCore::Element* webcore() const;
    Element& wkc() { return m_wkc; }

    bool isFormControlElement() const;
    void setChangedSinceLastFormControlChangeEvent(bool);

    bool hasAttributes() const;
    bool hasAttribute(const String& name) const;
    Attribute* getAttributeItem(const QualifiedName* name);

    bool isFocusable() const;
    void focus(bool restorePreviousSelection = true);
    void updateFocusAppearance(bool restorePreviousSelection);
    void blur();
    unsigned countChildNodes() const;
    Node* traverseToChildAt(unsigned);

private:
    ElementWrap m_wkc;

    AttributePrivate* m_attr;
    NodePrivate* m_traverseChildAt;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_ELEMENT_H_


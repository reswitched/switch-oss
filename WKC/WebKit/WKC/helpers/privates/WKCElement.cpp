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

#include "helpers/WKCElement.h"
#include "helpers/privates/WKCElementPrivate.h"

#include "Element.h"
#include "HTMLFormControlElement.h"

#include "helpers/WKCNode.h"
#include "helpers/WKCQualifiedName.h"
#include "helpers/WKCString.h"
#include "helpers/privates/WKCAttributePrivate.h"
#include "helpers/privates/WKCHTMLElementPrivate.h"
#include "helpers/privates/WKCNodePrivate.h"
#include "helpers/privates/WKCQualifiedNamePrivate.h"

namespace WKC {

ElementPrivate*
ElementPrivate::create(WebCore::Element* parent)
{
    if (!parent)
        return 0;

    if (parent->isHTMLElement()) {
        return HTMLElementPrivate::create(static_cast<WebCore::HTMLElement*>(parent));
    }
    return new ElementPrivate(parent);
}

ElementPrivate::ElementPrivate(WebCore::Element* parent)
    : NodePrivate(parent)
    , m_wkc(*this)
    , m_attr(0)
    , m_traverseChildAt(0)
{
}

ElementPrivate::~ElementPrivate()
{
    delete m_traverseChildAt;
    delete m_attr;
}

WebCore::Element*
ElementPrivate::webcore() const
{
    return static_cast<WebCore::Element*>(NodePrivate::webcore());
}

bool
ElementPrivate::isFormControlElement() const
{
    return webcore()->isFormControlElement();
}

void
ElementPrivate::setChangedSinceLastFormControlChangeEvent(bool flag)
{
    if (isFormControlElement()) {
        WebCore::HTMLFormControlElement* el = WTF::downcast<WebCore::HTMLFormControlElement>(webcore());
        el->setChangedSinceLastFormControlChangeEvent(flag);
    }
}

bool
ElementPrivate::hasAttributes() const
{
    return webcore()->hasAttributes();
}

bool
ElementPrivate::hasAttribute(const String& name) const
{
    WTF::String sn(name);
    WTF::AtomicString n(sn.impl());
    return webcore()->hasAttribute(n);
}

Attribute*
ElementPrivate::getAttributeItem(const QualifiedName* name)
{
    const WebCore::Attribute *attr = webcore()->findAttributeByName(*(name->priv()->webcore()));
    if (!attr) {
        return 0;
    }
    if (!m_attr || m_attr->webcore() != attr) {
        delete m_attr;
        m_attr = new AttributePrivate(attr);
    }
    return &m_attr->wkc();
}

bool
ElementPrivate::isFocusable() const
{
    return webcore()->isFocusable();
}

void
ElementPrivate::focus(bool restorePreviousSelection)
{
    webcore()->focus(restorePreviousSelection);
}

void
ElementPrivate::updateFocusAppearance(bool restorePreviousSelection)
{
    webcore()->updateFocusAppearance(restorePreviousSelection);
}

void
ElementPrivate::blur()
{
    webcore()->blur();
}

unsigned
ElementPrivate::countChildNodes() const
{
    return webcore()->countChildNodes();
}

Node*
ElementPrivate::traverseToChildAt(unsigned index)
{
    WebCore::Node* node = webcore()->traverseToChildAt(index);
    if (!node) {
        return 0;
    }
    if (!m_traverseChildAt || m_traverseChildAt->webcore() != node) {
        delete m_traverseChildAt;
        m_traverseChildAt = NodePrivate::create(node);
    }
    return &m_traverseChildAt->wkc();

}

////////////////////////////////////////////////////////////////////////////////

Element::Element(Element* other)
    : Node(other->priv())
{
}

Element::Element(ElementPrivate& parent)
    : Node(parent)
{
}

bool
Element::isFormControlElement() const
{
    return static_cast<ElementPrivate&>(priv()).isFormControlElement();
}

void
Element::setChangedSinceLastFormControlChangeEvent(bool flag)
{
    static_cast<ElementPrivate&>(priv()).setChangedSinceLastFormControlChangeEvent(flag);
}

bool
Element::hasAttributes() const
{
    return static_cast<ElementPrivate&>(priv()).hasAttributes();
}

bool
Element::hasAttribute(const String& name) const
{
    return static_cast<ElementPrivate&>(priv()).hasAttribute(name);
}

Attribute*
Element::getAttributeItem(const QualifiedName& name) const
{
    return static_cast<ElementPrivate&>(priv()).getAttributeItem(&name);
}

bool
Element::isFocusable() const
{
    return static_cast<ElementPrivate&>(priv()).isFocusable();
}

void
Element::focus(bool restorePreviousSelection)
{
    static_cast<ElementPrivate&>(priv()).focus(restorePreviousSelection);
}

void
Element::updateFocusAppearance(bool restorePreviousSelection)
{
    static_cast<ElementPrivate&>(priv()).updateFocusAppearance(restorePreviousSelection);
}

void
Element::blur()
{
    static_cast<ElementPrivate&>(priv()).blur();
}

unsigned
Element::countChildNodes() const
{
    return static_cast<ElementPrivate&>(priv()).countChildNodes();
}

Node*
Element::traverseToChildAt(unsigned index)
{
    return static_cast<ElementPrivate&>(priv()).traverseToChildAt(index);

}

} // namespace

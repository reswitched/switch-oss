/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010, 2012 Apple Inc. All rights reserved.
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
 */
#include "config.h"
#include "Attr.h"

#include "Event.h"
#include "ExceptionCode.h"
#include "ScopedEventQueue.h"
#include "StyleProperties.h"
#include "StyledElement.h"
#include "TextNodeTraversal.h"
#include "XMLNSNames.h"
#include <wtf/text/AtomicString.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

using namespace HTMLNames;

Attr::Attr(Element* element, const QualifiedName& name)
    : ContainerNode(element->document())
    , m_element(element)
    , m_name(name)
    , m_ignoreChildrenChanged(0)
{
}

Attr::Attr(Document& document, const QualifiedName& name, const AtomicString& standaloneValue)
    : ContainerNode(document)
    , m_element(0)
    , m_name(name)
    , m_standaloneValue(standaloneValue)
    , m_ignoreChildrenChanged(0)
{
}

RefPtr<Attr> Attr::create(Element* element, const QualifiedName& name)
{
    RefPtr<Attr> attr = adoptRef(new Attr(element, name));
    attr->createTextChild();
    return attr.release();
}

RefPtr<Attr> Attr::create(Document& document, const QualifiedName& name, const AtomicString& value)
{
    RefPtr<Attr> attr = adoptRef(new Attr(document, name, value));
    attr->createTextChild();
    return attr.release();
}

Attr::~Attr()
{
}

void Attr::createTextChild()
{
    ASSERT(refCount());
    if (!value().isEmpty()) {
        RefPtr<Text> textNode = document().createTextNode(value().string());

        // This does everything appendChild() would do in this situation (assuming m_ignoreChildrenChanged was set),
        // but much more efficiently.
        textNode->setParentNode(this);
        setFirstChild(textNode.get());
        setLastChild(textNode.get());
    }
}

void Attr::setPrefix(const AtomicString& prefix, ExceptionCode& ec)
{
    ec = 0;
    checkSetPrefix(prefix, ec);
    if (ec)
        return;

    if ((prefix == xmlnsAtom && namespaceURI() != XMLNSNames::xmlnsNamespaceURI)
        || static_cast<Attr*>(this)->qualifiedName() == xmlnsAtom) {
        ec = NAMESPACE_ERR;
        return;
    }

    const AtomicString& newPrefix = prefix.isEmpty() ? nullAtom : prefix;

    if (m_element)
        elementAttribute().setPrefix(newPrefix);
    m_name.setPrefix(newPrefix);
}

void Attr::setValue(const AtomicString& value)
{
    EventQueueScope scope;
    m_ignoreChildrenChanged++;
    removeChildren();
    if (m_element)
        elementAttribute().setValue(value);
    else
        m_standaloneValue = value;
    createTextChild();
    m_ignoreChildrenChanged--;

    invalidateNodeListAndCollectionCachesInAncestors(&m_name, m_element);
}

void Attr::setValue(const AtomicString& value, ExceptionCode&)
{
    AtomicString oldValue = this->value();
    if (m_element)
        m_element->willModifyAttribute(qualifiedName(), oldValue, value);

    setValue(value);

    if (m_element)
        m_element->didModifyAttribute(qualifiedName(), oldValue, value);
}

void Attr::setNodeValue(const String& v, ExceptionCode& ec)
{
    setValue(v, ec);
}

Ref<Node> Attr::cloneNodeInternal(Document& targetDocument, CloningOperation)
{
    Ref<Attr> clone = adoptRef(*new Attr(targetDocument, qualifiedName(), value()));
    cloneChildNodes(clone);
    return WTF::move(clone);
}

// DOM Section 1.1.1
bool Attr::childTypeAllowed(NodeType type) const
{
    switch (type) {
        case TEXT_NODE:
        case ENTITY_REFERENCE_NODE:
            return true;
        default:
            return false;
    }
}

void Attr::childrenChanged(const ChildChange&)
{
    if (m_ignoreChildrenChanged > 0)
        return;

    invalidateNodeListAndCollectionCachesInAncestors(&qualifiedName(), m_element);

    StringBuilder valueBuilder;
    TextNodeTraversal::appendContents(*this, valueBuilder);

    AtomicString oldValue = value();
    AtomicString newValue = valueBuilder.toAtomicString();
    if (m_element)
        m_element->willModifyAttribute(qualifiedName(), oldValue, newValue);

    if (m_element)
        elementAttribute().setValue(newValue);
    else
        m_standaloneValue = newValue;

    if (m_element)
        m_element->attributeChanged(qualifiedName(), oldValue, newValue);
}

bool Attr::isId() const
{
    return qualifiedName().matches(HTMLNames::idAttr);
}

CSSStyleDeclaration* Attr::style()
{
    // This function only exists to support the Obj-C bindings.
    if (!is<StyledElement>(m_element))
        return nullptr;
    m_style = MutableStyleProperties::create();
    downcast<StyledElement>(*m_element).collectStyleForPresentationAttribute(qualifiedName(), value(), *m_style);
    return m_style->ensureCSSStyleDeclaration();
}

const AtomicString& Attr::value() const
{
    if (m_element)
        return m_element->getAttribute(qualifiedName());
    return m_standaloneValue;
}

Attribute& Attr::elementAttribute()
{
    ASSERT(m_element);
    ASSERT(m_element->elementData());
    return *m_element->ensureUniqueElementData().findAttributeByName(qualifiedName());
}

void Attr::detachFromElementWithValue(const AtomicString& value)
{
    ASSERT(m_element);
    ASSERT(m_standaloneValue.isNull());
    m_standaloneValue = value;
    m_element = 0;
}

void Attr::attachToElement(Element* element)
{
    ASSERT(!m_element);
    m_element = element;
    m_standaloneValue = nullAtom;
}

}

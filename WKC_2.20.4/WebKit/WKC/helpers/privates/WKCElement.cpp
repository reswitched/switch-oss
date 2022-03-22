/*
 * Copyright (c) 2011-2020 ACCESS CO., LTD. All rights reserved.
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
#include "helpers/privates/WKCHelpersEnumsPrivate.h"

#include "Element.h"
#include "HTMLFormControlElement.h"
#include "RenderElement.h"
#include "RenderLayer.h"

#include "helpers/WKCNode.h"
#include "helpers/WKCQualifiedName.h"
#include "helpers/WKCString.h"
#include "helpers/privates/WKCAtomStringPrivate.h"
#include "helpers/privates/WKCAttributePrivate.h"
#include "helpers/privates/WKCHTMLElementPrivate.h"
#include "helpers/privates/WKCNodePrivate.h"
#include "helpers/privates/WKCQualifiedNamePrivate.h"

#include "utils/nxDebugPrint.h"

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
    , m_lang()
    , m_atomicstring_priv(0)
{
}

ElementPrivate::~ElementPrivate()
{
    delete m_traverseChildAt;
    delete m_attr;
    if (m_atomicstring_priv)
        delete m_atomicstring_priv;
}

WebCore::Element*
ElementPrivate::webcore() const
{
    return static_cast<WebCore::Element*>(NodePrivate::webcore());
}

bool
ElementPrivate::isFormControlElement() const
{
    if (!webcore()->isFormControlElement()) {
        return false;
    }

    // Returns true only if WKC has a implementation for its element.
    if (webcore()->hasTagName(WebCore::HTMLNames::inputTag)) {
        return true;
    }
    if (webcore()->hasTagName(WebCore::HTMLNames::buttonTag)) {
        return true;
    }
    if (webcore()->hasTagName(WebCore::HTMLNames::textareaTag)) {
        return true;
    }
    if (webcore()->hasTagName(WebCore::HTMLNames::selectTag)) {
        return true;
    }

    nxLog_e("\"%s\" has not yet been implemented as HTMLFormControlElement in WKC layer.", webcore()->tagName().utf8().data());

    return false;
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
    WTF::AtomString n(sn.impl());
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

const AtomString&
ElementPrivate::computeInheritedLanguage()
{
    m_lang = webcore()->computeInheritedLanguage();

    if (m_atomicstring_priv)
        delete m_atomicstring_priv;

    m_atomicstring_priv = new AtomStringPrivate(&m_lang);
    return m_atomicstring_priv->wkc();
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
ElementPrivate::updateFocusAppearance(SelectionRestorationMode restore, SelectionRevealMode reveal)
{
    WebCore::SelectionRestorationMode restore_webcore;
    WebCore::SelectionRevealMode reveal_webcore;
    restore_webcore = toWebCoreSelectionRestorationMode(restore);
    reveal_webcore = toWebCoreSelectionRevealMode(reveal);

    webcore()->updateFocusAppearance(restore_webcore, reveal_webcore);
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

int
ElementPrivate::zIndex() const
{
    WebCore::RenderElement* renderer = webcore()->renderer();
    if (!renderer)
        return 0;

    WebCore::RenderLayer* layer = renderer->enclosingLayer();
    if (!layer)
        return 0;

    return layer->zIndex();
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

const AtomString&
Element::computeInheritedLanguage() const
{
    return static_cast<ElementPrivate&>(priv()).computeInheritedLanguage();
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
Element::updateFocusAppearance(SelectionRestorationMode restore, SelectionRevealMode reveal)
{
    static_cast<ElementPrivate&>(priv()).updateFocusAppearance(restore, reveal);
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

int
Element::zIndex() const
{
    return static_cast<ElementPrivate&>(priv()).zIndex();
}

} // namespace

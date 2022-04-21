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

#include "config.h"
#include "helpers/WKCHTMLFormElement.h"
#include "helpers/privates/WKCHTMLFormElementPrivate.h"

#include "HTMLFormElement.h"

namespace WKC {

HTMLFormElementPrivate::HTMLFormElementPrivate(WebCore::HTMLFormElement* parent)
    : HTMLElementPrivate(parent)
    , m_wkc(*this)
    , m_item(0)
{
}

HTMLFormElementPrivate::~HTMLFormElementPrivate()
{
    delete m_item;
}

WebCore::HTMLFormElement*
HTMLFormElementPrivate::webcore() const
{
    return static_cast<WebCore::HTMLFormElement*>(HTMLElementPrivate::webcore());
}

void
HTMLFormElementPrivate::reset()
{
    webcore()->reset();
}

unsigned
HTMLFormElementPrivate::length() const
{
    return webcore()->length();
}

Node*
HTMLFormElementPrivate::item(unsigned index)
{
    WebCore::Node* node = webcore()->item(index);
    if (!node)
        return 0;

    if (!m_item || m_item->webcore() != node) {
        delete m_item;
        m_item = NodePrivate::create(node);
    }
    return &m_item->wkc();
}

bool
HTMLFormElementPrivate::shouldAutocomplete() const
{
    return webcore()->shouldAutocomplete();
}

////////////////////////////////////////////////////////////////////////////////

HTMLFormElement::HTMLFormElement(HTMLFormElementPrivate& parent)
    : HTMLElement(parent)
{
}

void
HTMLFormElement::reset()
{
    static_cast<HTMLFormElementPrivate&>(priv()).reset();
}

unsigned
HTMLFormElement::length() const
{
    return static_cast<HTMLFormElementPrivate&>(priv()).length();
}

Node*
HTMLFormElement::item(unsigned index)
{
    return static_cast<HTMLFormElementPrivate&>(priv()).item(index);
}

bool
HTMLFormElement::shouldAutocomplete() const
{
    return static_cast<HTMLFormElementPrivate&>(priv()).shouldAutocomplete();
}

} // namespace

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

#include "helpers/WKCHTMLFormControlElement.h"
#include "helpers/privates/WKCHTMLFormControlElementPrivate.h"

#include "HTMLFormControlElement.h"
#include "helpers/privates/WKCAtomStringPrivate.h"
#include "helpers/privates/WKCHTMLFormElementPrivate.h"

namespace WKC {

HTMLFormControlElementPrivate::HTMLFormControlElementPrivate(WebCore::HTMLFormControlElement* parent)
    : HTMLElementPrivate(parent)
    , m_wkc(*this)
    , m_formElement(0)
    , m_type()
    , m_atomicstring_priv(0)
{
}

HTMLFormControlElementPrivate::~HTMLFormControlElementPrivate()
{
    delete m_formElement;
    if (m_atomicstring_priv)
        delete m_atomicstring_priv;
}

WebCore::HTMLFormControlElement*
HTMLFormControlElementPrivate::webcore() const
{
    return static_cast<WebCore::HTMLFormControlElement*>(HTMLElementPrivate::webcore());
}

HTMLFormElement*
HTMLFormControlElementPrivate::form()
{
    WebCore::HTMLFormElement* form = webcore()->form();
    if (!form)
        return 0;
    if (!m_formElement || m_formElement->webcore() != form) {
        delete m_formElement;
        m_formElement = new HTMLFormElementPrivate(form);
    }
    return reinterpret_cast<HTMLFormElement*>(&m_formElement->wkc());
}

void
HTMLFormControlElementPrivate::dispatchFormControlInputEvent()
{
    webcore()->dispatchFormControlInputEvent();
}

void
HTMLFormControlElementPrivate::dispatchFormControlChangeEvent()
{
    webcore()->dispatchFormControlChangeEvent();
}

const AtomString&
HTMLFormControlElementPrivate::type()
{
    m_type = webcore()->type();

    if (m_atomicstring_priv)
        delete m_atomicstring_priv;

    m_atomicstring_priv = new AtomStringPrivate(&m_type);
    return m_atomicstring_priv->wkc();
}

bool
HTMLFormControlElementPrivate::isDisabledFormControl()
{
    return webcore()->isDisabledFormControl();
}

bool
HTMLFormControlElementPrivate::isReadOnly()
{
    return webcore()->isReadOnly();
}

bool
HTMLFormControlElementPrivate::isDisabledOrReadOnly()
{
    return webcore()->isDisabledOrReadOnly();
}

////////////////////////////////////////////////////////////////////////////////

HTMLFormControlElement::HTMLFormControlElement(HTMLFormControlElementPrivate& parent)
    : HTMLElement(parent)
{
}

void
HTMLFormControlElement::dispatchFormControlInputEvent()
{
    return static_cast<HTMLFormControlElementPrivate&>(priv()).dispatchFormControlInputEvent();
}

void
HTMLFormControlElement::dispatchFormControlChangeEvent()
{
    return static_cast<HTMLFormControlElementPrivate&>(priv()).dispatchFormControlChangeEvent();
}

const AtomString&
HTMLFormControlElement::type() const
{
    return static_cast<HTMLFormControlElementPrivate&>(priv()).type();
}

bool
HTMLFormControlElement::isDisabledFormControl() const
{
    return static_cast<HTMLFormControlElementPrivate&>(priv()).isDisabledFormControl();
}

bool
HTMLFormControlElement::isReadOnly() const
{
    return static_cast<HTMLFormControlElementPrivate&>(priv()).isReadOnly();
}

bool
HTMLFormControlElement::isDisabledOrReadOnly() const
{
    return static_cast<HTMLFormControlElementPrivate&>(priv()).isDisabledOrReadOnly();
}

HTMLFormElement*
HTMLFormControlElement::form() const
{
    return reinterpret_cast<HTMLFormControlElementPrivate&>(priv()).form();
}

} // namespace

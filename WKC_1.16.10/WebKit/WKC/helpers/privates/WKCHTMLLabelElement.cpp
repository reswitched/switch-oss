/*
 * Copyright (c) 2020 ACCESS CO., LTD. All rights reserved.
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

#include "WKCHTMLLabelElementPrivate.h"
#include "helpers/WKCHTMLLabelElement.h"

#include "HTMLLabelElement.h"

namespace WKC{

HTMLLabelElementPrivate::HTMLLabelElementPrivate(WebCore::HTMLLabelElement* parent)
    : HTMLElementPrivate(parent)
    , m_wkc(*this)
    , m_htmlElement(nullptr)
{
}

HTMLLabelElementPrivate::~HTMLLabelElementPrivate()
{
    delete m_htmlElement;
}

HTMLElement*
HTMLLabelElementPrivate::control()
{
    auto control = static_cast<WebCore::HTMLLabelElement*>(webcore())->control();
    if (!control)
        return nullptr;
    WebCore::HTMLElement* tmp = control.get();
    if (!m_htmlElement || m_htmlElement->webcore() != tmp) {
        delete m_htmlElement;
        m_htmlElement = HTMLElementPrivate::create(tmp);
    }
    return &m_htmlElement->wkc();
}

HTMLLabelElement::HTMLLabelElement(HTMLLabelElementPrivate& parent)
    : HTMLElement(parent)
{
}

HTMLLabelElement::~HTMLLabelElement()
{
}

HTMLElement*
HTMLLabelElement::control() const
{
    return static_cast<HTMLLabelElementPrivate&>(priv()).control();
}
} // namespace
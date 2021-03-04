/*
 * Copyright (c) 2011-2018 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCHTMLTextAreaElement.h"
#include "helpers/privates/WKCHTMLTextAreaElementPrivate.h"

#include "HTMLTextAreaElement.h"

#include "helpers/WKCString.h"

namespace WKC {

HTMLTextAreaElementPrivate::HTMLTextAreaElementPrivate(WebCore::HTMLTextAreaElement* parent)
    : HTMLFormControlElementPrivate(parent)
    , m_wkc(*this)
{
}

HTMLTextAreaElementPrivate::~HTMLTextAreaElementPrivate()
{
}

WebCore::HTMLTextAreaElement*
HTMLTextAreaElementPrivate::webcore() const
{
    return static_cast<WebCore::HTMLTextAreaElement*>(HTMLFormControlElementPrivate::webcore());
}

String
HTMLTextAreaElementPrivate::value() const
{
    return webcore()->value();
}

int
HTMLTextAreaElementPrivate::maxLength() const
{
    return webcore()->maxLength();
}

void
HTMLTextAreaElementPrivate::setValue(const String& str)
{
    webcore()->setValue(str);
}

////////////////////////////////////////////////////////////////////////////////

HTMLTextAreaElement::HTMLTextAreaElement(HTMLTextAreaElementPrivate& parent)
    : HTMLFormControlElement(parent)
{
}

const String
HTMLTextAreaElement::value() const
{
    return static_cast<HTMLTextAreaElementPrivate&>(priv()).value();
}

int
HTMLTextAreaElement::maxLength() const
{
    return static_cast<HTMLTextAreaElementPrivate&>(priv()).maxLength();
}

void
HTMLTextAreaElement::setValue(const String& str)
{
    static_cast<HTMLTextAreaElementPrivate&>(priv()).setValue(str);
}
} // namespace

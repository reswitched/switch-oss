/*
 * Copyright (c) 2011-2022 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCHTMLAreaElement.h"
#include "helpers/privates/WKCHTMLAreaElementPrivate.h"

#include "IntRect.h"
#include "HTMLAreaElement.h"
#include "HTMLImageElement.h"

#include "helpers/WKCRenderObject.h"
#include "helpers/privates/WKCRenderObjectPrivate.h"
#include "helpers/privates/WKCHTMLImageElementPrivate.h"

namespace WKC {

HTMLAreaElementPrivate::HTMLAreaElementPrivate(WebCore::HTMLAreaElement* parent)
    : HTMLElementPrivate(parent)
    , m_wkc(*this)
    , m_imageElement(0)
{
}

HTMLAreaElementPrivate::~HTMLAreaElementPrivate()
{
    delete m_imageElement;
}

WebCore::HTMLAreaElement*
HTMLAreaElementPrivate::webcore() const
{
    return static_cast<WebCore::HTMLAreaElement*>(HTMLElementPrivate::webcore());
}

HTMLImageElement*
HTMLAreaElementPrivate::imageElement()
{
    WebCore::HTMLImageElement* i = webcore()->imageElement();
    if (!i)
        return 0;
    if (!m_imageElement || m_imageElement->webcore() != i) {
        delete m_imageElement;
        m_imageElement = new HTMLImageElementPrivate(i);
    }

    return &m_imageElement->wkc();
}

WKCFloatRect
HTMLAreaElementPrivate::getRect(RenderObject* wobj)
{
    WebCore::RenderObject* obj = 0;
    if (wobj) {
        obj = const_cast<WebCore::RenderObject*>(wobj->priv().webcore());
    }
    return WebCore::FloatRect(webcore()->computeRect(obj));
}

////////////////////////////////////////////////////////////////////////////////

HTMLAreaElement::HTMLAreaElement(HTMLAreaElementPrivate& parent)
    : HTMLElement(parent)
{
}

HTMLImageElement*
HTMLAreaElement::imageElement() const
{
    return static_cast<HTMLAreaElementPrivate&>(priv()).imageElement();
}

WKCFloatRect
HTMLAreaElement::getRect(RenderObject* object)
{
    return static_cast<HTMLAreaElementPrivate&>(priv()).getRect(object);
}

} // namespace

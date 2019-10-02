/*
 * Copyright (c) 2011-2017 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCHTMLElement.h"
#include "helpers/privates/WKCHTMLElementPrivate.h"
#include "helpers/WKCString.h"
#include "helpers/privates/WKCHTMLInputElementPrivate.h"
#include "helpers/privates/WKCHTMLButtonElementPrivate.h"
#include "helpers/privates/WKCHTMLTextAreaElementPrivate.h"
#include "helpers/privates/WKCHTMLAreaElementPrivate.h"
#include "helpers/privates/WKCHTMLFormElementPrivate.h"
#include "helpers/privates/WKCHTMLMediaElementPrivate.h"
#include "helpers/privates/WKCHTMLSelectElementPrivate.h"
#include "helpers/privates/WKCHTMLFrameElementPrivate.h"
#include "helpers/privates/WKCHTMLIFrameElementPrivate.h"

#include "HTMLElement.h"
#include "HTMLInputElement.h"
#include "HTMLButtonElement.h"
#include "HTMLTextAreaElement.h"
#include "HTMLAreaElement.h"
#include "HTMLFormElement.h"
#include "HTMLSelectElement.h"
#include "HTMLIFrameElement.h"
#include "HTMLFrameElement.h"

namespace WKC {

HTMLElementPrivate*
HTMLElementPrivate::create(WebCore::HTMLElement* parent)
{
    if (!parent)
        return 0;

    if (parent->hasTagName(WebCore::HTMLNames::inputTag)) {
        return new HTMLInputElementPrivate(static_cast<WebCore::HTMLInputElement*>(parent));
    } else if (parent->hasTagName(WebCore::HTMLNames::buttonTag)) {
        return new HTMLButtonElementPrivate(static_cast<WebCore::HTMLButtonElement*>(parent));
    } else if (parent->hasTagName(WebCore::HTMLNames::textareaTag)) {
        return new HTMLTextAreaElementPrivate(static_cast<WebCore::HTMLTextAreaElement*>(parent));
    } else if (parent->hasTagName(WebCore::HTMLNames::areaTag)) {
        return new HTMLAreaElementPrivate(static_cast<WebCore::HTMLAreaElement*>(parent));
    } else if (parent->hasTagName(WebCore::HTMLNames::iframeTag)) {
        return new HTMLIFrameElementPrivate(static_cast<WebCore::HTMLIFrameElement*>(parent));
    } else if (parent->hasTagName(WebCore::HTMLNames::formTag)) {
        return new HTMLFormElementPrivate(static_cast<WebCore::HTMLFormElement*>(parent));
    } else if (parent->hasTagName(WebCore::HTMLNames::frameTag)) {
        return new HTMLFrameElementPrivate(static_cast<WebCore::HTMLFrameElement*>(parent));
    } else if (parent->hasTagName(WebCore::HTMLNames::selectTag)) {
        return new HTMLSelectElementPrivate(static_cast<WebCore::HTMLSelectElement*>(parent));
    } else if (parent->hasTagName(WebCore::HTMLNames::videoTag)) {
        // TODO: If you implement HTMLVideoElement, you need to create HTMLVideoElement instead of HTMLMediaElement.
        return new HTMLMediaElementPrivate(reinterpret_cast<WebCore::HTMLMediaElement*>(parent));
    }
    return new HTMLElementPrivate(parent);
}

HTMLElementPrivate::HTMLElementPrivate(WebCore::HTMLElement* parent)
    : ElementPrivate(parent)
    , m_webcore(parent)
    , m_wkc(*this)
{
}

HTMLElementPrivate::~HTMLElementPrivate()
{
}

void
HTMLElementPrivate::setInnerText(const String& text, int& ec)
{
    auto result = webcore()->setInnerText(text);
    if (result.hasException()) {
        ec = result.releaseException().code();
    }
}

HTMLElement::HTMLElement(HTMLElementPrivate& parent)
    : Element(parent)
{
}

HTMLElement::~HTMLElement()
{
}

void
HTMLElement::setInnerText(const String& text, int& ec)
{
    ((HTMLElementPrivate&)priv()).setInnerText(text, ec);
}

} // namespace

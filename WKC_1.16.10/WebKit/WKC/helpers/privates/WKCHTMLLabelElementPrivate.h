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

#ifndef _WKC_HELPERS_PRIVATE_HTMLLABELELEMENT_H_
#define _WKC_HELPERS_PRIVATE_HTMLLABELELEMENT_H_

#include "helpers/WKCHTMLLabelElement.h"
#include "WKCHTMLElementPrivate.h"

namespace WebCore {
    class HTMLLabelElement;
} // namespace

namespace WKC {
class HTMLLabelElementWrap : public HTMLLabelElement {
WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLLabelElementWrap(HTMLLabelElementPrivate& parent) : HTMLLabelElement(parent) {}
    ~HTMLLabelElementWrap() {}
};

class HTMLLabelElementPrivate : public HTMLElementPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLLabelElementPrivate(WebCore::HTMLLabelElement*);
    virtual ~HTMLLabelElementPrivate();
    HTMLElement* control();

private:
    HTMLLabelElementWrap m_wkc;
    HTMLElementPrivate* m_htmlElement;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_HTMLLABELELEMENT_H_
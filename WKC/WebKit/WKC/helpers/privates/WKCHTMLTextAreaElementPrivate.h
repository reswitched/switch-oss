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

#ifndef _WKC_HELPERS_PRIVATE_HTMLTEXTAREAELEMENT_H_
#define _WKC_HELPERS_PRIVATE_HTMLTEXTAREAELEMENT_H_

#include "helpers/WKCHTMLTextAreaElement.h"
#include "helpers/privates/WKCHTMLFormControlElementPrivate.h"

namespace WebCore {
class HTMLTextAreaElement;
} // namespace

namespace WKC {

class HTMLTextAreaElementWrap : public HTMLTextAreaElement {
WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLTextAreaElementWrap(HTMLTextAreaElementPrivate& parent) : HTMLTextAreaElement(parent) {}
    ~HTMLTextAreaElementWrap() {}
};

class HTMLTextAreaElementPrivate : public HTMLFormControlElementPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLTextAreaElementPrivate(WebCore::HTMLTextAreaElement*);
    virtual ~HTMLTextAreaElementPrivate();

    WebCore::HTMLTextAreaElement* webcore() const;
    HTMLTextAreaElement& wkc() { return m_wkc; }

    String value() const;
    bool readOnly() const;
    bool disabled() const;
    int maxLength() const;
    void setValue(const String&);

private:
    HTMLTextAreaElementWrap m_wkc;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_HTMLTEXTAREAELEMENT_H_


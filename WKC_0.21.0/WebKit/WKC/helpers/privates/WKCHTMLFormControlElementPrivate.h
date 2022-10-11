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

#ifndef _WKC_HELPERS_PRIVATE_HTMLFORMCONTROLELEMENT_H_
#define _WKC_HELPERS_PRIVATE_HTMLFORMCONTROLELEMENT_H_

#include "helpers/WKCHTMLFormControlElement.h"
#include "helpers/privates/WKCHTMLElementPrivate.h"
#include "AtomicString.h"

namespace WebCore {
class HTMLFormControlElement;
} // namespace

namespace WKC {
class AtomicStringPrivate;
class HTMLFormElementPrivate;

class HTMLFormControlElementWrap : public HTMLFormControlElement {
WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLFormControlElementWrap(HTMLFormControlElementPrivate& parent) : HTMLFormControlElement(parent) {}
    ~HTMLFormControlElementWrap() {}
};

class HTMLFormControlElementPrivate : public HTMLElementPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLFormControlElementPrivate(WebCore::HTMLFormControlElement*);
    virtual ~HTMLFormControlElementPrivate();

    WebCore::HTMLFormControlElement* webcore() const;
    HTMLFormControlElement& wkc() { return m_wkc; }

    HTMLFormElement* form();

    void dispatchFormControlInputEvent();
    void dispatchFormControlChangeEvent();
    const AtomicString& type();
    bool isDisabledFormControl();
    bool isReadOnly();
    bool isDisabledOrReadOnly();

private:
    HTMLFormControlElementWrap m_wkc;
    HTMLFormElementPrivate* m_formElement;
    WTF::AtomicString m_type;
    AtomicStringPrivate* m_atomicstring_priv;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_HTMLFORMCONTROLELEMENT_H_


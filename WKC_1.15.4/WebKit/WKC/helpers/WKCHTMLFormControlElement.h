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

#ifndef _WKC_HELPERS_WKC_HTMLFORMCONTROLELEMENT_H_
#define _WKC_HELPERS_WKC_HTMLFORMCONTROLELEMENT_H_

#include <wkc/wkcbase.h>

#include "helpers/WKCHTMLElement.h"

namespace WKC {
class AtomicString;
class HTMLFormControlElementPrivate;
class HTMLFormElement;

class WKC_API HTMLFormControlElement : public HTMLElement {
public:
    void dispatchFormControlInputEvent();
    void dispatchFormControlChangeEvent();
    const AtomicString& type() const;
    bool isDisabledFormControl() const;
    bool isReadOnly() const;
    bool isDisabledOrReadOnly() const;
    HTMLFormElement* form() const;

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    HTMLFormControlElement(HTMLFormControlElementPrivate&);
    virtual ~HTMLFormControlElement() {}

private:
    HTMLFormControlElement(const HTMLFormControlElement&);
    HTMLFormControlElement& operator=(const HTMLFormControlElement&);
};
} // namespace

#endif // _WKC_HELPERS_WKC_HTMLFORMCONTROLELEMENT_H_

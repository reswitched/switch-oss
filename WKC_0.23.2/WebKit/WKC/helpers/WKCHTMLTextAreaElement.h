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

#ifndef _WKC_HELPERS_WKC_HTMLTEXTAREAELEMENT_H_
#define _WKC_HELPERS_WKC_HTMLTEXTAREAELEMENT_H_

#include <wkc/wkcbase.h>

#include "helpers/WKCHTMLFormControlElement.h"

namespace WKC {
class HTMLTextAreaElementPrivate;
class RenderObject;
class String;

class WKC_API HTMLTextAreaElement : public WKC::HTMLFormControlElement {
public:
    const String value() const;
    int maxLength() const;
    void setValue(const String&);

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    HTMLTextAreaElement(HTMLTextAreaElementPrivate&);
    virtual ~HTMLTextAreaElement() {}

private:
    HTMLTextAreaElement(const HTMLTextAreaElement&);
    HTMLTextAreaElement& operator=(const HTMLTextAreaElement&);
};
} // namespace

#endif // _WKC_HELPERS_WKC_HTMLTEXTAREAELEMENT_H_

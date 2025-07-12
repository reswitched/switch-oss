/*
 * Copyright (c) 2017 ACCESS CO., LTD. All rights reserved.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef _WKC_HELPERS_WKC_HTMLBUTTONELEMENT_H_
#define _WKC_HELPERS_WKC_HTMLBUTTONELEMENT_H_

#include <wkc/wkcbase.h>

#include "WKCHTMLFormControlElement.h"

namespace WKC {
class HTMLButtonElementPrivate;

class WKC_API HTMLButtonElement : public WKC::HTMLFormControlElement {
public:

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    HTMLButtonElement(HTMLButtonElementPrivate&);
    virtual ~HTMLButtonElement() {}

private:
    HTMLButtonElement(const HTMLButtonElement&);
    HTMLButtonElement& operator=(const HTMLButtonElement&);
};
} // namespace

#endif // _WKC_HELPERS_WKC_HTMLBUTTONELEMENT_H_

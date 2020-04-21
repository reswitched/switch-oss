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

#ifndef _WKC_HELPERS_PRIVATE_CLIPBOARD_H_
#define _WKC_HELPERS_PRIVATE_CLIPBOARD_H_

#include "helpers/WKCClipboard.h"

namespace WebCore {
class Clipboard;
} // namespace

namespace WKC {

class ClipboardWrap : public Clipboard {
WTF_MAKE_FAST_ALLOCATED;
public:
    ClipboardWrap(ClipboardPrivate& parent) : Clipboard(parent) {}
    ~ClipboardWrap() {}
};

class ClipboardPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    ClipboardPrivate(WebCore::Clipboard*);
    ~ClipboardPrivate();

    WebCore::Clipboard* webcore() const { return m_webcore; }
    Clipboard& wkc() { return m_wkc; }

private:
    WebCore::Clipboard* m_webcore;
    ClipboardWrap m_wkc;

};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_CLIPBOARD_H_


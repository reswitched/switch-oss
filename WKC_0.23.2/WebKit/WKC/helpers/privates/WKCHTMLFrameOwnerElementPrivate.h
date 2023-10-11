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

#ifndef _WKC_HELPERS_PRIVATE_HTMLFRAMEOWNERELEMENT_H_
#define _WKC_HELPERS_PRIVATE_HTMLFRAMEOWNERELEMENT_H_

#include "helpers/WKCHTMLFrameOwnerElement.h"
#include "helpers/privates/WKCHTMLElementPrivate.h"

namespace WebCore {
class HTMLFrameOwnerElement;
} // namespace

namespace WKC {

class Frame;
class FramePrivate;

class HTMLFrameOwnerElementWrap : public HTMLFrameOwnerElement {
WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLFrameOwnerElementWrap(HTMLFrameOwnerElementPrivate& parent) : HTMLFrameOwnerElement(parent) {}
    ~HTMLFrameOwnerElementWrap() {}
};

class HTMLFrameOwnerElementPrivate : public HTMLElementPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    HTMLFrameOwnerElementPrivate(WebCore::HTMLFrameOwnerElement*);
    virtual ~HTMLFrameOwnerElementPrivate();

    WebCore::HTMLFrameOwnerElement* webcore() const;
    HTMLFrameOwnerElement& wkc() { return m_wkc; }

    Frame* contentFrame();

private:
    HTMLFrameOwnerElementWrap m_wkc;

    FramePrivate* m_contentFrame;
};
} // namespace

#endif // _WKC_HELPERS_PRIVATE_FRAMEOWNERFrameOwnerELEMENT_H_


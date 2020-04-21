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

#include "config.h"

#include "helpers/WKCHTMLFrameOwnerElement.h"
#include "helpers/privates/WKCHTMLFrameOwnerElementPrivate.h"

#include "IntRect.h"
#include "HTMLFrameOwnerElement.h"

#include "helpers/WKCRenderObject.h"
#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/privates/WKCRenderObjectPrivate.h"

namespace WKC {

HTMLFrameOwnerElementPrivate::HTMLFrameOwnerElementPrivate(WebCore::HTMLFrameOwnerElement* parent)
    : HTMLElementPrivate(parent)
    , m_wkc(*this)
    , m_contentFrame(0)
{
}

HTMLFrameOwnerElementPrivate::~HTMLFrameOwnerElementPrivate()
{
    delete m_contentFrame;
}

WebCore::HTMLFrameOwnerElement*
HTMLFrameOwnerElementPrivate::webcore() const
{
    return static_cast<WebCore::HTMLFrameOwnerElement*>(HTMLElementPrivate::webcore());
}

Frame*
HTMLFrameOwnerElementPrivate::contentFrame()
{
    WebCore::Frame* f = webcore()->contentFrame();
    if (!f)
        return 0;

    if (!m_contentFrame || m_contentFrame->webcore()!=f) {
        delete m_contentFrame;
        m_contentFrame = new FramePrivate(f);
    }
    return &m_contentFrame->wkc();
}

////////////////////////////////////////////////////////////////////////////////

HTMLFrameOwnerElement::HTMLFrameOwnerElement(HTMLFrameOwnerElementPrivate& parent)
    : HTMLElement(parent)
{
}

Frame*
HTMLFrameOwnerElement::contentFrame() const
{
    return static_cast<HTMLFrameOwnerElementPrivate&>(priv()).contentFrame();
}

} // namespace

/*
 * DropDownListClientWKC.cpp
 *
 * Copyright (c) 2010 ACCESS CO., LTD. All rights reserved.
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
 */


#include "config.h"
#include "DropDownListClientWKC.h"
#include "WKCWebViewPrivate.h"

#include "FrameView.h"
#include "PopupMenuClient.h"

#include "helpers/DropDownListClientIf.h"
#include "helpers/privates/WKCFrameViewPrivate.h"

// implementations

namespace WKC {

DropDownListClientWKC::DropDownListClientWKC(WKCWebViewPrivate* view)
     : m_view(view),
       m_appClient(0),
       m_destroyed(false)
{
}

DropDownListClientWKC::~DropDownListClientWKC()
{
    if (m_appClient && !m_destroyed) {
        m_destroyed = true;
        m_view->clientBuilders().deleteDropDownListClient(m_appClient);
        m_appClient = 0;
    }
}

DropDownListClientWKC*
DropDownListClientWKC::create(WKCWebViewPrivate* view)
{
    DropDownListClientWKC* self = 0;
    self = new DropDownListClientWKC(view);
    if (!self) return 0;
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
DropDownListClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createDropDownListClient(m_view->parent());
    if (!m_appClient) return false;
    return true;
}


void
DropDownListClientWKC::show(const WebCore::IntRect& r, WebCore::FrameView* view, int index, WKC::PopupMenuClient *client)
{
    FrameViewPrivate v(view);
    m_appClient->show(r, &v.wkc(), index, client);
}

void
DropDownListClientWKC::hide(WKC::PopupMenuClient *client)
{
    m_appClient->hide(client);
}

void
DropDownListClientWKC::updateFromElement(WKC::PopupMenuClient *client)
{
    m_appClient->updateFromElement(client);
}


} // namespace

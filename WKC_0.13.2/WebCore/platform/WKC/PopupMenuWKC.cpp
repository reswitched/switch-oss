/*
 * This file is part of the popup menu implementation for <select> elements in WebCore.
 *
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (c) 2010, 2012 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"
#include "PopupMenu.h"

#include "CString.h"
#include "FrameView.h"
#include "HostWindow.h"
#include "WTFString.h"
#include "RenderMenuList.h"

#include "PopupMenuWKC.h"

#include "WKCWebViewPrivate.h"
#include "DropDownListClientWKC.h"
#include "helpers/privates/WKCPopupMenuClientPrivate.h"

namespace WebCore {

PopupMenuWKC::PopupMenuWKC(PopupMenuClient* client)
    : m_popupClient(client)
{
    if (client) {
        m_wkc = new WKC::PopupMenuClientPrivate(client);
    } else {
        m_wkc = 0;
    }
    m_visible = false;
}

PopupMenuWKC::~PopupMenuWKC()
{
    hide();
    delete m_wkc;
}

void PopupMenuWKC::show(const IntRect& rect, FrameView* view, int index)
{
    if (!client())
        return;

    if (m_visible) {
        hide();
    }

    PlatformPageClient pageclient;
    pageclient = view->hostWindow()->platformPageClient();
    if (pageclient) {
        WKC::WKCWebViewPrivate *webview = (WKC::WKCWebViewPrivate*)pageclient;
        m_visible = true;
        webview->dropdownlistclient()->show(rect, view, index, &m_wkc->wkc());
    }
}

void PopupMenuWKC::hide()
{
    if (!m_visible)
        return;

    PopupMenuClient* menuclient = client();
    if (!menuclient) {
        if (m_visible && m_wkc) {
            HostWindow* hostwindow = m_wkc->webcore()->hostWindow();
            if (hostwindow) {
                WKC::WKCWebViewPrivate* webview = static_cast<WKC::WKCWebViewPrivate *>(hostwindow->platformPageClient());
                webview->dropdownlistclient()->hide(&m_wkc->wkc());
            }
        }
        m_visible = false;
        return;
    }

    PlatformPageClient pageclient = menuclient->hostWindow()->platformPageClient();
    if (pageclient) {
        WKC::WKCWebViewPrivate *webview = (WKC::WKCWebViewPrivate*)pageclient;
        webview->dropdownlistclient()->hide(&m_wkc->wkc());
        client()->popupDidHide();
    }

    m_visible = false;
}

void PopupMenuWKC::updateFromElement()
{
    if (client()) {
        PlatformPageClient pageclient;
        pageclient = client()->hostWindow()->platformPageClient();
        if (pageclient) {
            WKC::WKCWebViewPrivate *webview = (WKC::WKCWebViewPrivate*)pageclient;
            webview->dropdownlistclient()->updateFromElement(&m_wkc->wkc());
        }
        client()->setTextFromItem(client()->selectedIndex());
    }
}

void PopupMenuWKC::disconnectClient()
{
	m_popupClient = 0;
}

}


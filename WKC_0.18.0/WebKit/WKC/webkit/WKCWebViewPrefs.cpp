/*
 * WKCWebViewPrefs.cpp
 *
 * Copyright (c) 2011, 2015 ACCESS CO., LTD. All rights reserved.
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

#include "WKCWebViewPrefs.h"
#include "WKCWebViewPrivate.h"

#include "Page.h"
#include "Frame.h"
#include "FrameView.h"

namespace WKC {

class WKCWebView;
class WKCWebViewPrivate;

WKCWebViewPrefs::WKCWebViewPrefs(WKCWebView* view)
    : m_view(view)
    , m_privateView(view->m_private)
{
}

WKCWebViewPrefs::~WKCWebViewPrefs()
{
}

WKCWebViewPrefs*
WKCWebViewPrefs::create(WKCWebView* view)
{
    WKCWebViewPrefs* self = 0;

    self = (WKCWebViewPrefs *)WTF::fastMalloc(sizeof(WKCWebViewPrefs));
    self = new (self) WKCWebViewPrefs(view);
    if (!self) return 0;
    if (!self->construct()) {
        self->~WKCWebViewPrefs();
        WTF::fastFree(self);
        return 0;
    }

    return self;
}

bool
WKCWebViewPrefs::construct()
{
    return true;
}

void
WKCWebViewPrefs::deleteWKCWebViewPrefs(WKCWebViewPrefs *self)
{
    if (!self)
        return;
    self->~WKCWebViewPrefs();
    WTF::fastFree(self);
}

void
WKCWebViewPrefs::setJavaScriptURLsAreAllowed(bool flag)
{
//    m_privateView->core()->setJavaScriptURLsAreAllowed(flag);
}

bool
WKCWebViewPrefs::transparent()
{
    return m_privateView->transparent();
}

void
WKCWebViewPrefs::setTransparent(bool flag)
{
    m_privateView->setTransparent(flag);
}

void
WKCWebViewPrefs::forceTerminate()
{
}

void
WKCWebViewPrefs::resetVariables()
{
}

} // namespace

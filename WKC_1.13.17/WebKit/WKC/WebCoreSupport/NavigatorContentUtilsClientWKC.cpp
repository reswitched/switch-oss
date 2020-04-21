/*
 *  Copyright (c) 2014, 2015 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include "NavigatorContentUtilsClientWKC.h"
#include "WKCWebViewPrivate.h"
#include "helpers/NavigatorContentUtilsClientIf.h"
#include "helpers/WKCString.h"

#if ENABLE(NAVIGATOR_CONTENT_UTILS)
namespace WKC {

NavigatorContentUtilsClientWKC::NavigatorContentUtilsClientWKC(WKCWebViewPrivate* view)
    : m_view(view)
    , m_appClient(0)
{
}

NavigatorContentUtilsClientWKC::~NavigatorContentUtilsClientWKC()
{
    if (m_appClient) {
        m_view->clientBuilders().deleteNavigatorContentUtilsClient(m_appClient);
        m_appClient = 0;
    }
}

NavigatorContentUtilsClientWKC*
NavigatorContentUtilsClientWKC::create(WKCWebViewPrivate* view)
{
    NavigatorContentUtilsClientWKC* self;
    self = new NavigatorContentUtilsClientWKC(view);
    if (!self)
        return 0;
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
NavigatorContentUtilsClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createNavigatorContentUtilsClient(m_view->parent());
    return true;
}

void
NavigatorContentUtilsClientWKC::registerProtocolHandler(const WTF::String& scheme, const WebCore::URL& baseURL, const WebCore::URL& url, const WTF::String& title)
{
    m_appClient->registerProtocolHandler(scheme, baseURL.string(), url.string(), title);
}

#if ENABLE(CUSTOM_SCHEME_HANDLER)
WebCore::NavigatorContentUtilsClient::CustomHandlersState
NavigatorContentUtilsClientWKC::isProtocolHandlerRegistered(const WTF::String& scheme, const WebCore::URL& baseURL, const WebCore::URL& url)
{
    return (WebCore::NavigatorContentUtilsClient::CustomHandlersState)m_appClient->isProtocolHandlerRegistered(scheme, baseURL.string(), url.string());
}

void
NavigatorContentUtilsClientWKC::unregisterProtocolHandler(const WTF::String& scheme, const WebCore::URL& baseURL, const WebCore::URL& url)
{
    m_appClient->unregisterProtocolHandler(scheme, baseURL.string(), url.string());
}
#endif

} // namespace

#endif


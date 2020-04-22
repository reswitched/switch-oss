/*
 *  Copyright (c) 2014 ACCESS CO., LTD. All rights reserved.
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

#ifndef NavigatorContentUtilsClientWKC_h
#define NavigatorContentUtilsClientWKC_h

#include "NavigatorContentUtilsClient.h"

#if ENABLE(NAVIGATOR_CONTENT_UTILS)
namespace WKC {

class NavigatorContentUtilsClientIf;
class WKCWebViewPrivate;


class NavigatorContentUtilsClientWKC : public WebCore::NavigatorContentUtilsClient
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    static NavigatorContentUtilsClientWKC* create(WKCWebViewPrivate*);
    virtual ~NavigatorContentUtilsClientWKC();

    virtual void registerProtocolHandler(const WTF::String& scheme, const WebCore::URL& baseURL, const WebCore::URL&, const WTF::String& title);

#if ENABLE(CUSTOM_SCHEME_HANDLER)
    virtual WebCore::NavigatorContentUtilsClient::CustomHandlersState isProtocolHandlerRegistered(const WTF::String& scheme, const WebCore::URL& baseURL, const WebCore::URL&);
    virtual void unregisterProtocolHandler(const WTF::String& scheme, const WebCore::URL& baseURL, const WebCore::URL&);
#endif

private:
    NavigatorContentUtilsClientWKC(WKCWebViewPrivate*);
    bool construct();

private:
    WKCWebViewPrivate* m_view;
    NavigatorContentUtilsClientIf* m_appClient;
};

} // namespace
#endif

#endif // NavigatorContentUtilsClientWKC_h



/*
*  Copyright (c) 2015 ACCESS CO., LTD. All rights reserved.
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

#ifndef WebNfcClientWKC_h
#define WebNfcClientWKC_h

#include "WebNfcClient.h"

#if ENABLE(WKC_WEB_NFC)

namespace WebCore {
class WebNfcController;
class WebNfcMessage;
} // namespace WebCore

namespace WKC {

class WebNfcClientIf;
class WKCWebViewPrivate;
class WebNfcControllerPrivate;
class String;

class WebNfcClientWKC : public WebCore::WebNfcClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static WebNfcClientWKC* create(WKCWebViewPrivate*);
    virtual ~WebNfcClientWKC();

    virtual void setController(WebCore::WebNfcController*);
    virtual void webNfcControllerDestroyed();

    virtual bool requestPermission();
    virtual void requestAdapter(int id);
    virtual void startRequestMessage();
    virtual void stopRequestMessage();
    virtual void send(int id, PassRefPtr<WebCore::WebNfcMessage> message, const String& target);

private:
    WebNfcClientWKC(WKCWebViewPrivate*);
    bool construct();

    WKCWebViewPrivate* m_view;
    WebNfcClientIf* m_appClient;
    WebNfcControllerPrivate* m_controller;
};

} // namespace WKC

#endif // ENABLE(WKC_WEB_NFC)
#endif // WebNfcClientWKC_h

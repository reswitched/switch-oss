/*
 *  WebNfcClient.h
 *
 *  Copyright(c) 2015 ACCESS CO., LTD. All rights reserved.
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


#ifndef WebNfcClient_h
#define WebNfcClient_h

#if ENABLE(WKC_WEB_NFC)

#include <wtf/RefPtr.h>

namespace WKC {
class String;
} // namespace WKC

namespace WebCore {

class Page;
class WebNfcController;
class WebNfcMessage;

class WebNfcClient {
public:
    virtual ~WebNfcClient() { }

    virtual void setController(WebNfcController*) = 0;
    virtual void webNfcControllerDestroyed() = 0;

    virtual bool requestPermission() = 0;
    virtual void requestAdapter(int id) = 0;
    virtual void startRequestMessage() = 0;
    virtual void stopRequestMessage() = 0;
    virtual void send(int id, RefPtr<WebNfcMessage> massage, const WKC::String& target) = 0;
};

void provideWebNfcTo(Page*, WebNfcClient*);

} // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)
#endif // WebNfcClient_h

/*
 * Copyright (c) 2015-2016 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_WKC_WEBNFCCONTROLLERPRIVATE_H_
#define _WKC_HELPERS_WKC_WEBNFCCONTROLLERPRIVATE_H_

#if ENABLE(WKC_WEB_NFC)

#include "helpers/WKCWebNfcController.h"

#include "WebNfcController.h"

namespace WKC {

class WebNfcControllerWrap : public WebNfcController {
    WTF_MAKE_FAST_ALLOCATED;
public:
    WebNfcControllerWrap(WebNfcControllerPrivate& parent) : WebNfcController(parent)
    {}
    ~WebNfcControllerWrap()
    {}
};

class WebNfcControllerPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    WebNfcControllerPrivate(WebCore::WebNfcController* parent);
    ~WebNfcControllerPrivate();

    WebCore::WebNfcController* webcore() const { return m_webcore; }
    WebNfcController* wkc() { return &m_wkc; }

    void updateAdapter(int, int);
    void updateMessage(WebNfcMessage*);
    void notifySendResult(int, int);

private:
    WebCore::WebNfcController* m_webcore;
    WebNfcControllerWrap m_wkc;

    WebNfcMessage* m_message;
};

} // namespace WKC

#endif // ENABLE(WKC_WEB_NFC)

#endif // _WKC_HELPERS_WKC_WEBNFCCONTROLLERPRIVATE_H_

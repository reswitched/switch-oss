/*
 * Copyright (c) 2010-2016 ACCESS CO., LTD. All rights reserved.
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
#include "CookieJar.h"

#include "Cookie.h"
#include "Document.h"
#include "URL.h"
#include "WTFString.h"
#include "StringHash.h"
#include "ResourceHandleManagerWKC.h"
#include "FrameLoaderClient.h"
#include "FrameLoaderClientWKC.h"
#include "HTMLFrameOwnerElement.h"

#include <wtf/HashMap.h>

#include <wkc/wkcpeer.h>

namespace WebCore {

#if 0
static String optimize_url(const URL& url)
{
    if (url.isNull())
        return String();

    URL opt_url = KURL(ParsedURLString, url.baseAsString());

    return opt_url.protocol() + "://" + opt_url.host() + opt_url.path();
}
#endif

static bool isHttpOnly(const WTF::String& value)
{
    WTF::Vector<String> values;

    value.split(L';', values);
    for (int i = 0; i < values.size(); i++) {
        String s = values[i].stripWhiteSpace();
        if (equalIgnoringCase(s, "HttpOnly")) {
            return true;
        }
    }

    return false;
}

void setCookies(Document* document, const URL& url, const WTF::String& value)
{
    WTF::String firstparty_host;
    WTF::String cookie_domain;
    size_t pos;

    if (document) {
        Document* top = document;
        HTMLFrameOwnerElement* element = 0;
        while (element = top->ownerElement()) {
            top = &element->document();
        }
        firstparty_host = top->url().host();
    }
    if (firstparty_host.isEmpty()) {
        firstparty_host = url.host();
    }

    // Skip first cookie-pair because it is NOT attibure-value pair. 
    // See [RFC 6265] Sec. 4.1.1.
    pos = value.find(";");

    if (WTF::notFound != pos) {
        String str = value.substring(pos, value.length() - pos);

        if (isHttpOnly(str)) {
            return;
        }
        if (WTF::notFound != str.find("domain=", 0, false)) {
            cookie_domain = value.substring(value.find("domain=", 0, false) + strlen("domain="));
            size_t len1 = cookie_domain.find(",");
            size_t len2 = cookie_domain.find(";");
            size_t len  = WTF::notFound;

            if (WTF::notFound == len1) {
                len = len2;
            } else if (WTF::notFound == len2) {
                len = len1;
            } else {
                len = (len1 < len2) ? len1 : len2;
            }
            if (len != WTF::notFound) {
                cookie_domain = cookie_domain.left(len);
            }
            cookie_domain = cookie_domain.stripWhiteSpace();

            if (wkcNetCheckCorrectIPAddressPeer(firstparty_host.utf8().data()) && firstparty_host != cookie_domain) {
                return;
            }
        }
    }

    if (cookie_domain.isEmpty()) {
        cookie_domain = url.host();
    }

    if (ResourceHandleManager::sharedInstance()->cookieCallback(document, true, url.string().utf8().data(), firstparty_host, cookie_domain))
        return; // ret = 0 means accept

    URL opt_url = URL(ParsedURLString, url.baseAsString());
    ResourceHandleManager::sharedInstance()->setCookie(opt_url.host(), opt_url.path(), value);
}

WTF::String cookiesInternal(const Document* document, const URL& url, bool includehttponly)
{
    URL opt_url = URL(ParsedURLString, url.baseAsString());
    bool secure = url.protocolIs("https");
    WTF::String firstparty_host;
    WTF::String cookie_domain;

    if (document) {
        Document& top = document->topDocument();
        firstparty_host = top.url().host();
    }
    if (firstparty_host.isEmpty()) {
        firstparty_host = url.host();
    }

    cookie_domain = url.host();

    if (ResourceHandleManager::sharedInstance()->cookieCallback(document, false, url.string().utf8().data(), firstparty_host, cookie_domain))
        return WTF::String(); // ret = 0 means accept

    return ResourceHandleManager::sharedInstance()->getCookie(opt_url.host(), opt_url.path(), secure, includehttponly);
}

WTF::String cookies(const Document* document, const URL& url)
{
    return cookiesInternal(document, url, false);
}

bool cookiesEnabled(const Document* /*document*/)
{
    return true;
}

#if ENABLE(WEB_SOCKETS)
String cookieRequestHeaderFieldValue(const Document* document, const URL& url)
{
    return cookiesInternal(document, url, true);
}
#endif

// below functions are only for Inspectors

bool getRawCookies(const Document*, const URL& url, Vector<Cookie>& rawCookies)
{
    return ResourceHandleManager::sharedInstance()->getRawCookies(url.host(), url.path(), url.protocolIs("https"), rawCookies);
}

void deleteCookie(const Document*, const URL& url, const WTF::String& name)
{
    ResourceHandleManager::sharedInstance()->deleteCookie(url.host(), url.path(), url.protocolIs("https"), name);
}

void setCookieStoragePrivateBrowsingEnabled(bool enabled)
{
    // FIXME: Not yet implemented
}

}

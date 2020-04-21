/*
 * Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
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
#include "CookieRequestHeaderFieldProxy.h"
#include "CookiesStrategy.h"
#include "Document.h"
#include "URL.h"
#include "WTFString.h"
#include "StringHash.h"
#include "ResourceHandleManagerWKC.h"
#include "FrameLoaderClient.h"
#include "FrameLoaderClientWKC.h"
#include "HTMLFrameOwnerElement.h"
#include "NotImplemented.h"

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
        if (equalIgnoringASCIICase(s, "HttpOnly")) {
            return true;
        }
    }

    return false;
}

void setCookiesFromDOM(const NetworkStorageSession& session, const URL& firstParty, const SameSiteInfo&, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, const String& value)
{
    WTF::String firstparty_host;
    WTF::String cookie_domain;
    size_t pos;

    firstparty_host = firstParty.host().toString();
    if (firstparty_host.isEmpty()) {
        firstparty_host = url.host().toString();
    }

    // Skip first cookie-pair because it is NOT attibure-value pair. 
    // See [RFC 6265] Sec. 4.1.1.
    pos = value.find(";");

    if (WTF::notFound != pos) {
        String str = value.substring(pos, value.length() - pos);

        if (isHttpOnly(str)) {
            return;
        }
        if (WTF::notFound != str.findIgnoringASCIICase("domain=", 0)) {
            cookie_domain = value.substring(value.findIgnoringASCIICase("domain=", 0) + strlen("domain="));
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
        cookie_domain = url.host().toString();
    }

    if (ResourceHandleManager::sharedInstance()->cookieCallback(session, true, url.string().utf8().data(), firstparty_host, cookie_domain))
        return; // ret = 0 means accept

    URL opt_url = URL(ParsedURLString, url.baseAsString());
    ResourceHandleManager::sharedInstance()->setCookie(opt_url.host().toString(), opt_url.path(), value);
}

void setCookiesFromHTTPResponse(const NetworkStorageSession&, const URL&, const String&)
{
    notImplemented();
}

std::pair<String, bool> cookiesInternal(const NetworkStorageSession& session, const URL& firstParty, const URL& url, bool includehttponly)
{
    URL opt_url = URL(ParsedURLString, url.baseAsString());
    bool secure = url.protocolIs("https");
    WTF::String firstparty_host;
    WTF::String cookie_domain;

    firstparty_host = firstParty.host().toString();
    if (firstparty_host.isEmpty()) {
        firstparty_host = url.host().toString();
    }

    cookie_domain = url.host().toString();

    if (ResourceHandleManager::sharedInstance()->cookieCallback(session, false, url.string().utf8().data(), firstparty_host, cookie_domain))
        return { WTF::String(), false }; // ret = 0 means accept

    return { ResourceHandleManager::sharedInstance()->getCookie(opt_url.host().toString(), opt_url.path(), secure, includehttponly), false };
}

std::pair<String, bool> cookiesForDOM(const NetworkStorageSession& session, const URL& firstParty, const SameSiteInfo&, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, IncludeSecureCookies)
{
    return cookiesInternal(session, firstParty, url, false);
}

bool cookiesEnabled(const NetworkStorageSession&)
{
    return true;
}

std::pair<String, bool> cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const URL& firstParty, const SameSiteInfo&, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, IncludeSecureCookies)
{
    return cookiesInternal(session, firstParty, url, true);
}

std::pair<String, bool> cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const CookieRequestHeaderFieldProxy& proxy)
{
    return cookiesInternal(session, proxy.firstParty, proxy.url, true);
}

// below functions are only for Inspectors

bool getRawCookies(const NetworkStorageSession&, const URL&, const SameSiteInfo&, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, Vector<Cookie>& rawCookies)
{
    return ResourceHandleManager::sharedInstance()->getRawCookies(url.host().toString(), url.path(), url.protocolIs("https"), rawCookies);
}

void deleteCookie(const NetworkStorageSession&, const URL& url, const String& name)
{
    ResourceHandleManager::sharedInstance()->deleteCookie(url.host().toString(), url.path(), url.protocolIs("https"), name);
}

void getHostnamesWithCookies(const NetworkStorageSession&, HashSet<String>& hostname)
{
    notImplemented();
}

void deleteCookiesForHostnames(const NetworkStorageSession&, const Vector<String>& cookieHostNames)
{
    notImplemented();
}

void deleteAllCookies(const NetworkStorageSession&)
{
    notImplemented();
}

void deleteAllCookiesModifiedSince(const NetworkStorageSession&, WallTime)
{
    notImplemented();
}

}

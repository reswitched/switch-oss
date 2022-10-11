/*
 *  Copyright (C) 2008 Xan Lopez <xan@gnome.org>
 *  Copyright (C) 2009 Igalia S.L.
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#if USE(SOUP)

#include "CookieJarSoup.h"

#include "Cookie.h"
#include "GUniquePtrSoup.h"
#include "URL.h"
#include "NetworkingContext.h"
#include "PlatformCookieJar.h"
#include "SoupNetworkSession.h"
#include <wtf/glib/GRefPtr.h>
#include <wtf/text/CString.h>

namespace WebCore {

static SoupCookieJar* cookieJarForSession(const NetworkStorageSession& session)
{
    return session.soupNetworkSession().cookieJar();
}

static GRefPtr<SoupCookieJar>& defaultCookieJar()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(GRefPtr<SoupCookieJar>, cookieJar, ());
    return cookieJar;
}

SoupCookieJar* soupCookieJar()
{
    if (GRefPtr<SoupCookieJar>& jar = defaultCookieJar())
        return jar.get();

    SoupCookieJar* jar = soup_cookie_jar_new();
    soup_cookie_jar_set_accept_policy(jar, SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY);
    setSoupCookieJar(jar);
    return jar;
}

SoupCookieJar* createPrivateBrowsingCookieJar()
{
    SoupCookieJar* jar = soup_cookie_jar_new();

    soup_cookie_jar_set_accept_policy(jar, SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY);

    return jar;
}

void setSoupCookieJar(SoupCookieJar* jar)
{
    defaultCookieJar() = jar;
}

static inline bool httpOnlyCookieExists(const GSList* cookies, const gchar* name, const gchar* path)
{
    for (const GSList* iter = cookies; iter; iter = g_slist_next(iter)) {
        SoupCookie* cookie = static_cast<SoupCookie*>(iter->data);
        if (!strcmp(soup_cookie_get_name(cookie), name)
            && !g_strcmp0(soup_cookie_get_path(cookie), path)) {
            if (soup_cookie_get_http_only(cookie))
                return true;
            break;
        }
    }
    return false;
}

void setCookiesFromDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url, const String& value)
{
    SoupCookieJar* jar = cookieJarForSession(session);
    if (!jar)
        return;

    GUniquePtr<SoupURI> origin = url.createSoupURI();
    GUniquePtr<SoupURI> firstPartyURI = firstParty.createSoupURI();

    // Get existing cookies for this origin.
    GSList* existingCookies = soup_cookie_jar_get_cookie_list(jar, origin.get(), TRUE);

    Vector<String> cookies;
    value.split('\n', cookies);
    const size_t cookiesCount = cookies.size();
    for (size_t i = 0; i < cookiesCount; ++i) {
        GUniquePtr<SoupCookie> cookie(soup_cookie_parse(cookies[i].utf8().data(), origin.get()));
        if (!cookie)
            continue;

        // Make sure the cookie is not httpOnly since such cookies should not be set from JavaScript.
        if (soup_cookie_get_http_only(cookie.get()))
            continue;

        // Make sure we do not overwrite httpOnly cookies from JavaScript.
        if (httpOnlyCookieExists(existingCookies, soup_cookie_get_name(cookie.get()), soup_cookie_get_path(cookie.get())))
            continue;

        soup_cookie_jar_add_cookie_with_first_party(jar, firstPartyURI.get(), cookie.release());
    }

    soup_cookies_free(existingCookies);
}

static String cookiesForSession(const NetworkStorageSession& session, const URL& url, bool forHTTPHeader)
{
    SoupCookieJar* jar = cookieJarForSession(session);
    if (!jar)
        return String();

    GUniquePtr<SoupURI> uri = url.createSoupURI();
    GUniquePtr<char> cookies(soup_cookie_jar_get_cookies(jar, uri.get(), forHTTPHeader));
    return String::fromUTF8(cookies.get());
}

String cookiesForDOM(const NetworkStorageSession& session, const URL&, const URL& url)
{
    return cookiesForSession(session, url, false);
}

String cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const URL& /*firstParty*/, const URL& url)
{
    return cookiesForSession(session, url, true);
}

bool cookiesEnabled(const NetworkStorageSession& session, const URL& /*firstParty*/, const URL& /*url*/)
{
    return !!cookieJarForSession(session);
}

bool getRawCookies(const NetworkStorageSession& session, const URL& /*firstParty*/, const URL& url, Vector<Cookie>& rawCookies)
{
    rawCookies.clear();
    SoupCookieJar* jar = cookieJarForSession(session);
    if (!jar)
        return false;

    GUniquePtr<SoupURI> uri = url.createSoupURI();
    GUniquePtr<GSList> cookies(soup_cookie_jar_get_cookie_list(jar, uri.get(), TRUE));
    if (!cookies)
        return false;

    for (GSList* iter = cookies.get(); iter; iter = g_slist_next(iter)) {
        SoupCookie* cookie = static_cast<SoupCookie*>(iter->data);
        rawCookies.append(Cookie(String::fromUTF8(cookie->name), String::fromUTF8(cookie->value), String::fromUTF8(cookie->domain),
            String::fromUTF8(cookie->path), cookie->expires ? static_cast<double>(soup_date_to_time_t(cookie->expires)) * 1000 : 0,
            cookie->http_only, cookie->secure, !cookie->expires));
        soup_cookie_free(cookie);
    }

    return true;
}

void deleteCookie(const NetworkStorageSession& session, const URL& url, const String& name)
{
    SoupCookieJar* jar = cookieJarForSession(session);
    if (!jar)
        return;

    GUniquePtr<SoupURI> uri = url.createSoupURI();
    GUniquePtr<GSList> cookies(soup_cookie_jar_get_cookie_list(jar, uri.get(), TRUE));
    if (!cookies)
        return;

    CString cookieName = name.utf8();
    bool wasDeleted = false;
    for (GSList* iter = cookies.get(); iter; iter = g_slist_next(iter)) {
        SoupCookie* cookie = static_cast<SoupCookie*>(iter->data);
        if (!wasDeleted && cookieName == cookie->name) {
            soup_cookie_jar_delete_cookie(jar, cookie);
            wasDeleted = true;
        }
        soup_cookie_free(cookie);
    }
}

void getHostnamesWithCookies(const NetworkStorageSession& session, HashSet<String>& hostnames)
{
    SoupCookieJar* cookieJar = cookieJarForSession(session);
    GUniquePtr<GSList> cookies(soup_cookie_jar_all_cookies(cookieJar));
    for (GSList* item = cookies.get(); item; item = g_slist_next(item)) {
        SoupCookie* cookie = static_cast<SoupCookie*>(item->data);
        if (cookie->domain)
            hostnames.add(String::fromUTF8(cookie->domain));
        soup_cookie_free(cookie);
    }
}

void deleteCookiesForHostnames(const NetworkStorageSession& session, const Vector<String>& hostnames)
{
    for (const auto& hostname : hostnames) {
        CString hostNameString = hostname.utf8();
        SoupCookieJar* cookieJar = cookieJarForSession(session);
        GUniquePtr<GSList> cookies(soup_cookie_jar_all_cookies(cookieJar));
        for (GSList* item = cookies.get(); item; item = g_slist_next(item)) {
            SoupCookie* cookie = static_cast<SoupCookie*>(item->data);
            if (soup_cookie_domain_matches(cookie, hostNameString.data()))
                soup_cookie_jar_delete_cookie(cookieJar, cookie);
            soup_cookie_free(cookie);
        }
    }
}

void deleteAllCookies(const NetworkStorageSession& session)
{
    SoupCookieJar* cookieJar = cookieJarForSession(session);
    GUniquePtr<GSList> cookies(soup_cookie_jar_all_cookies(cookieJar));
    for (GSList* item = cookies.get(); item; item = g_slist_next(item)) {
        SoupCookie* cookie = static_cast<SoupCookie*>(item->data);
        soup_cookie_jar_delete_cookie(cookieJar, cookie);
        soup_cookie_free(cookie);
    }
}

void deleteAllCookiesModifiedSince(const NetworkStorageSession&, std::chrono::system_clock::time_point)
{
}

}

#endif

/*
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

#include "NetworkStorageSession.h"

#include "Cookie.h"
#include "CookieJarWKC.h"
#include "CookieRequestHeaderFieldProxy.h"
#include "HTTPCookieAcceptPolicy.h"

namespace WebCore {

using namespace PAL;

NetworkStorageSession::NetworkStorageSession(SessionID sessionID)
    : m_sessionID(sessionID)
{
}

NetworkStorageSession::~NetworkStorageSession()
{
}

static std::unique_ptr<NetworkStorageSession>& defaultSession()
{
    static NeverDestroyed<std::unique_ptr<NetworkStorageSession>> session;
    if (session.isNull())
        session.construct();
    return session;
}

NetworkStorageSession& NetworkStorageSession::defaultStorageSession()
{
    if (!defaultSession())
        defaultSession() = std::make_unique<NetworkStorageSession>(SessionID::defaultSessionID());
    return *defaultSession();
}

NetworkStorageSession* NetworkStorageSession::storageSession(PAL::SessionID sessionID)
{
    ASSERT(sessionID == SessionID::defaultSessionID());
    return &defaultStorageSession();
}

void NetworkStorageSession::setCookiesFromDOM(const URL& firstParty, const SameSiteInfo& sameSiteInfo, const URL& url, std::optional<FrameIdentifier> frameID, std::optional<PageIdentifier> pageID, ShouldAskITP, const String& value, ShouldRelaxThirdPartyCookieBlocking) const
{
    WebCore::setCookiesFromDOM(*this, firstParty, sameSiteInfo, url, frameID, pageID, value);
}

bool NetworkStorageSession::getRawCookies(const URL& firstParty, const SameSiteInfo& sameSiteInfo, const URL& url, std::optional<FrameIdentifier> frameID, std::optional<PageIdentifier> pageID, ShouldAskITP, ShouldRelaxThirdPartyCookieBlocking, Vector<Cookie>& rawCookies) const
{
    return WebCore::getRawCookies(*this, firstParty, sameSiteInfo, url, frameID, pageID, rawCookies);
}

void NetworkStorageSession::deleteCookie(const URL& url, const String& name) const
{
    WebCore::deleteCookie(*this, url, name);
}

std::pair<String, bool> NetworkStorageSession::cookiesForDOM(const URL& firstParty, const SameSiteInfo& sameSiteInfo, const URL& url, std::optional<FrameIdentifier> frameID, std::optional<PageIdentifier> pageID, IncludeSecureCookies includeSecureCookies, ShouldAskITP, ShouldRelaxThirdPartyCookieBlocking) const
{
    return WebCore::cookiesForDOM(*this, firstParty, sameSiteInfo, url, frameID, pageID, includeSecureCookies);
}

HTTPCookieAcceptPolicy  NetworkStorageSession::cookieAcceptPolicy() const
{
    return WebCore::cookiesEnabled(*this) ? HTTPCookieAcceptPolicy::AlwaysAccept : HTTPCookieAcceptPolicy::Never;
}

std::pair<String, bool> NetworkStorageSession::cookieRequestHeaderFieldValue(const URL& firstParty, const SameSiteInfo& sameSiteInfo, const URL& url, std::optional<FrameIdentifier> frameID, std::optional<PageIdentifier> pageID, IncludeSecureCookies includeSecureCookies, ShouldAskITP, ShouldRelaxThirdPartyCookieBlocking) const
{
    return WebCore::cookieRequestHeaderFieldValue(*this, firstParty, sameSiteInfo, url, frameID, pageID, includeSecureCookies);
}

std::pair<String, bool> NetworkStorageSession::cookieRequestHeaderFieldValue(const CookieRequestHeaderFieldProxy& headerFieldProxy) const
{
    return WebCore::cookieRequestHeaderFieldValue(*this, headerFieldProxy);
}

void NetworkStorageSession::setCookie(const Cookie& cookie)
{
    WebCore::setCookie(cookie);
}

}

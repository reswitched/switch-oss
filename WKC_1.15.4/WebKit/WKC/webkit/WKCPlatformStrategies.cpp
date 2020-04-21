/*
 * WKCPlatformStrategies.cpp
 * Copyright (C) 2010, 2011, 2016 Apple Inc. All rights reserved.
 * Copyright (c) 2018-2019 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "WKCPlatformStrategies.h"

#include "WebKitLegacy/WebCoreSupport/WebResourceLoadScheduler.h"
#include "BlobRegistryImpl.h"
#include "NetworkStorageSession.h"
#include "Page.h"
#include "PageGroup.h"
#include "PlatformCookieJar.h"

using namespace WebCore;

namespace WKC {

void PlatformStrategiesWKC::initialize()
{
    WKC_DEFINE_STATIC_TYPE(PlatformStrategiesWKC*, platformStrategies, 0);
    if (!platformStrategies) {
        platformStrategies = new PlatformStrategiesWKC();
        setPlatformStrategies(platformStrategies);
    }
}

PlatformStrategiesWKC::PlatformStrategiesWKC()
{
}

CookiesStrategy* PlatformStrategiesWKC::createCookiesStrategy()
{
    return this;
}

LoaderStrategy* PlatformStrategiesWKC::createLoaderStrategy()
{
    return new WebResourceLoadScheduler;
}

PasteboardStrategy* PlatformStrategiesWKC::createPasteboardStrategy()
{
    return nullptr;
}

BlobRegistry* PlatformStrategiesWKC::createBlobRegistry()
{
    return new BlobRegistryImpl;
}

std::pair<WTF::String, bool> PlatformStrategiesWKC::cookiesForDOM(const NetworkStorageSession& session, const URL& firstParty, const WebCore::SameSiteInfo& info, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, WebCore::IncludeSecureCookies includeSecureCookies)
{
    return WebCore::cookiesForDOM(session, firstParty, info, url, frameID, pageID, includeSecureCookies);
}

void PlatformStrategiesWKC::setCookiesFromDOM(const NetworkStorageSession& session, const URL& firstParty, const WebCore::SameSiteInfo& info, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, const WTF::String& cookieString)
{
    WebCore::setCookiesFromDOM(session, firstParty, info, url, frameID, pageID, cookieString);
}

bool PlatformStrategiesWKC::cookiesEnabled(const NetworkStorageSession& session)
{
    return WebCore::cookiesEnabled(session);
}

std::pair<WTF::String, bool> PlatformStrategiesWKC::cookieRequestHeaderFieldValue(const WebCore::NetworkStorageSession& session, const WebCore::URL& firstParty, const WebCore::SameSiteInfo& info, const WebCore::URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, WebCore::IncludeSecureCookies includeSecureCookies)
{
    return WebCore::cookieRequestHeaderFieldValue(session, firstParty, info, url, frameID, pageID, includeSecureCookies);
}

std::pair<WTF::String, bool> PlatformStrategiesWKC::cookieRequestHeaderFieldValue(PAL::SessionID sessionID, const WebCore::URL& firstParty, const WebCore::SameSiteInfo& info, const WebCore::URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, WebCore::IncludeSecureCookies includeSecureCookies)
{
    auto& session = sessionID.isEphemeral() ? *NetworkStorageSession::storageSession(PAL::SessionID::legacyPrivateSessionID()) : NetworkStorageSession::defaultStorageSession();
    return WebCore::cookieRequestHeaderFieldValue(session, firstParty, info, url, frameID, pageID, includeSecureCookies);
}

bool PlatformStrategiesWKC::getRawCookies(const NetworkStorageSession& session, const URL& firstParty, const WebCore::SameSiteInfo& info, const URL& url, std::optional<uint64_t> frameID, std::optional<uint64_t> pageID, Vector<Cookie>& rawCookies)
{
    return WebCore::getRawCookies(session, firstParty, info, url, frameID, pageID, rawCookies);
}

void PlatformStrategiesWKC::deleteCookie(const NetworkStorageSession& session, const URL& url, const WTF::String& cookieName)
{
    WebCore::deleteCookie(session, url, cookieName);
}

} // namespace

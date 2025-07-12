/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2018 Sony Interactive Entertainment Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "AuthenticationChallenge.h"

#include "ResourceError.h"
#include "ResourceHandleManagerWKC.h"
#include "ResourceHandleInternalWKC.h"

namespace WebCore {

AuthenticationChallenge::AuthenticationChallenge(ResourceHandle* sourceHandle, unsigned previousFailureCount, const ResourceResponse& response, AuthenticationClient* client)
    : AuthenticationChallengeBase(protectionSpaceFromHandle(sourceHandle, response), Credential(), previousFailureCount, response, ResourceError())
    , m_sourceHandle(sourceHandle)
    , m_authenticationClient(client)
{
}

ProtectionSpace::ServerType AuthenticationChallenge::protectionSpaceServerTypeFromURI(const WTF::URL& url, bool isForProxy)
{
    if (url.protocolIs("https"))
        return isForProxy ? ProtectionSpace::ServerType::ProxyHTTP /* use an http proxy for now */ : ProtectionSpace::ServerType::HTTPS;
    if (url.protocolIs("http"))
        return isForProxy ? ProtectionSpace::ServerType::ProxyHTTP : ProtectionSpace::ServerType::HTTP;
    if (url.protocolIs("ftp"))
        return isForProxy ? ProtectionSpace::ServerType::ProxyFTP : ProtectionSpace::ServerType::FTP;
    return isForProxy ? ProtectionSpace::ServerType::ProxyHTTP : ProtectionSpace::ServerType::HTTP;
}

ProtectionSpace AuthenticationChallenge::protectionSpaceFromHandle(ResourceHandle* sourceHandle, const ResourceResponse& response)
{
    int httpStatusCode = response.httpStatusCode();

    if (httpStatusCode != 401 && httpStatusCode != 407)
        return ProtectionSpace();

    auto isProxyAuth = (httpStatusCode == 407);
    const auto& url = response.url();
    auto port = isProxyAuth ? ResourceHandleManager::sharedInstance()->proxyPort() : determinePort(url);
    auto serverType = protectionSpaceServerTypeFromURI(url, isProxyAuth);
    auto authenticationScheme = authenticationSchemeFromHnadle(sourceHandle, isProxyAuth);

    return ProtectionSpace(isProxyAuth ? ResourceHandleManager::sharedInstance()->proxyHost() : url.host().toString(), port.value_or(0), serverType, parseRealm(response), authenticationScheme);
}

std::optional<uint16_t> AuthenticationChallenge::determinePort(const WTF::URL& url)
{
    if (auto port = url.port())
        return *port;

    if (auto port = defaultPortForProtocol(url.protocol()))
        return *port;

    return std::nullopt;
}

ProtectionSpace::AuthenticationScheme AuthenticationChallenge::authenticationSchemeFromHnadle(ResourceHandle* sourceHandle, bool isForProxy)
{
    ResourceHandleInternal* d = sourceHandle->getInternal();
    long curlAuth = 0;

    curl_easy_getinfo(d->m_handle, (isForProxy ? CURLINFO_PROXYAUTH_AVAIL : CURLINFO_HTTPAUTH_AVAIL), &curlAuth);

    if (curlAuth & CURLAUTH_NTLM)
        return ProtectionSpace::AuthenticationScheme::NTLM;
    if (curlAuth & CURLAUTH_NEGOTIATE)
        return ProtectionSpace::AuthenticationScheme::Negotiate;
    if (curlAuth & CURLAUTH_DIGEST)
        return ProtectionSpace::AuthenticationScheme::HTTPDigest;
    if (curlAuth & CURLAUTH_BASIC)
        return ProtectionSpace::AuthenticationScheme::HTTPBasic;
    return ProtectionSpace::AuthenticationScheme::Unknown;
}

String AuthenticationChallenge::parseRealm(const ResourceResponse& response)
{
    static NeverDestroyed<String> wwwAuthenticate(MAKE_STATIC_STRING_IMPL("www-authenticate"));
    if (wwwAuthenticate.isNull())
        wwwAuthenticate.construct(MAKE_STATIC_STRING_IMPL("www-authenticate"));

    static NeverDestroyed<String> proxyAuthenticate(MAKE_STATIC_STRING_IMPL("proxy-authenticate"));
    if (proxyAuthenticate.isNull())
        proxyAuthenticate.construct(MAKE_STATIC_STRING_IMPL("proxy-authenticate"));

    static NeverDestroyed<String> realmString(MAKE_STATIC_STRING_IMPL("realm="));
    if (realmString.isNull())
        realmString.construct(MAKE_STATIC_STRING_IMPL("realm="));


    String realm;
    int httpStatusCode = response.httpStatusCode();

    auto authHeader = response.httpHeaderField(httpStatusCode == 401 ? wwwAuthenticate : proxyAuthenticate);
    auto realmPos = authHeader.findIgnoringASCIICase(realmString);
    if (realmPos != notFound) {
        realm = authHeader.substring(realmPos + realmString.get().length());
        realm = realm.left(realm.find(','));
        removeLeadingAndTrailingQuotes(realm);
    }
    return realm;
}

void AuthenticationChallenge::removeLeadingAndTrailingQuotes(String& value)
{
    auto length = value.length();
    if (value.startsWith('"') && value.endsWith('"') && length > 1)
        value = value.substring(1, length - 2);
}

} // namespace

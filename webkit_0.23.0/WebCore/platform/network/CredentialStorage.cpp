/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "CredentialStorage.h"

#include "NetworkStorageSession.h"
#include "URL.h"
#include <wtf/NeverDestroyed.h>

#if PLATFORM(IOS)
#include "WebCoreThread.h"
#endif

namespace WebCore {

CredentialStorage& CredentialStorage::defaultCredentialStorage()
{
    return NetworkStorageSession::defaultStorageSession().credentialStorage();
}

static String originStringFromURL(const URL& url)
{
    if (url.port())
        return url.protocol() + "://" + url.host() + ':' + String::number(url.port()) + '/';

    return url.protocol() + "://" + url.host() + '/';
}

static String protectionSpaceMapKeyFromURL(const URL& url)
{
    ASSERT(url.isValid());

    // Remove the last path component that is not a directory to determine the subtree for which credentials will apply.
    // We keep a leading slash, but remove a trailing one.
    String directoryURL = url.string().substring(0, url.pathEnd());
    unsigned directoryURLPathStart = url.pathStart();
    ASSERT(directoryURL[directoryURLPathStart] == '/');
    if (directoryURL.length() > directoryURLPathStart + 1) {
        size_t index = directoryURL.reverseFind('/');
        ASSERT(index != notFound);
        directoryURL = directoryURL.substring(0, (index != directoryURLPathStart) ? index : directoryURLPathStart + 1);
    }

    return directoryURL;
}

void CredentialStorage::set(const Credential& credential, const ProtectionSpace& protectionSpace, const URL& url)
{
    ASSERT(protectionSpace.isProxy() || protectionSpace.authenticationScheme() == ProtectionSpaceAuthenticationSchemeClientCertificateRequested || url.protocolIsInHTTPFamily());
    ASSERT(protectionSpace.isProxy() || protectionSpace.authenticationScheme() == ProtectionSpaceAuthenticationSchemeClientCertificateRequested || url.isValid());

    m_protectionSpaceToCredentialMap.set(protectionSpace, credential);

#if PLATFORM(IOS)
    if (protectionSpace.authenticationScheme() != ProtectionSpaceAuthenticationSchemeClientCertificateRequested)
        saveToPersistentStorage(protectionSpace, credential);
#endif

    if (!protectionSpace.isProxy() && protectionSpace.authenticationScheme() != ProtectionSpaceAuthenticationSchemeClientCertificateRequested) {
        m_originsWithCredentials.add(originStringFromURL(url));

        ProtectionSpaceAuthenticationScheme scheme = protectionSpace.authenticationScheme();
        if (scheme == ProtectionSpaceAuthenticationSchemeHTTPBasic || scheme == ProtectionSpaceAuthenticationSchemeDefault) {
            // The map can contain both a path and its subpath - while redundant, this makes lookups faster.
            m_pathToDefaultProtectionSpaceMap.set(protectionSpaceMapKeyFromURL(url), protectionSpace);
        }
    }
}

Credential CredentialStorage::get(const ProtectionSpace& protectionSpace)
{
    return m_protectionSpaceToCredentialMap.get(protectionSpace);
}

void CredentialStorage::remove(const ProtectionSpace& protectionSpace)
{
    m_protectionSpaceToCredentialMap.remove(protectionSpace);
}

HashMap<String, ProtectionSpace>::iterator CredentialStorage::findDefaultProtectionSpaceForURL(const URL& url)
{
    ASSERT(url.protocolIsInHTTPFamily());
    ASSERT(url.isValid());

    // Don't spend time iterating the path for origins that don't have any credentials.
    if (!m_originsWithCredentials.contains(originStringFromURL(url)))
        return m_pathToDefaultProtectionSpaceMap.end();

    String directoryURL = protectionSpaceMapKeyFromURL(url);
    unsigned directoryURLPathStart = url.pathStart();
    while (true) {
        PathToDefaultProtectionSpaceMap::iterator iter = m_pathToDefaultProtectionSpaceMap.find(directoryURL);
        if (iter != m_pathToDefaultProtectionSpaceMap.end())
            return iter;

        if (directoryURL.length() == directoryURLPathStart + 1)  // path is "/" already, cannot shorten it any more
            return m_pathToDefaultProtectionSpaceMap.end();

        size_t index = directoryURL.reverseFind('/', directoryURL.length() - 2);
        ASSERT(index != notFound);
        directoryURL = directoryURL.substring(0, (index == directoryURLPathStart) ? index + 1 : index);
        ASSERT(directoryURL.length() > directoryURLPathStart);
        ASSERT(directoryURL.length() == directoryURLPathStart + 1 || directoryURL[directoryURL.length() - 1] != '/');
    }
}

bool CredentialStorage::set(const Credential& credential, const URL& url)
{
    ASSERT(url.protocolIsInHTTPFamily());
    ASSERT(url.isValid());
    PathToDefaultProtectionSpaceMap::iterator iter = findDefaultProtectionSpaceForURL(url);
    if (iter == m_pathToDefaultProtectionSpaceMap.end())
        return false;
    ASSERT(m_originsWithCredentials.contains(originStringFromURL(url)));
    m_protectionSpaceToCredentialMap.set(iter->value, credential);
    return true;
}

Credential CredentialStorage::get(const URL& url)
{
    PathToDefaultProtectionSpaceMap::iterator iter = findDefaultProtectionSpaceForURL(url);
    if (iter == m_pathToDefaultProtectionSpaceMap.end())
        return Credential();
    return m_protectionSpaceToCredentialMap.get(iter->value);
}

void CredentialStorage::clearCredentials()
{
    m_protectionSpaceToCredentialMap.clear();
    m_originsWithCredentials.clear();
    m_pathToDefaultProtectionSpaceMap.clear();
}

} // namespace WebCore

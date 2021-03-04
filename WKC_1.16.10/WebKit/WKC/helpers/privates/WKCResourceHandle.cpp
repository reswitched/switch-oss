/*
 * Copyright (c) 2011-2016 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"

#include "helpers/WKCResourceHandle.h"
#include "helpers/privates/WKCResourceHandlePrivate.h"

#include "ResourceHandle.h"
#include "ResourceHandleInternalWKC.h"
#include "helpers/WKCAuthenticationChallenge.h"
#include "helpers/WKCCredential.h"

#include "helpers/privates/WKCCredentialPrivate.h"
#include "helpers/privates/WKCAuthenticationChallengePrivate.h"

namespace WKC {
ResourceHandlePrivate::ResourceHandlePrivate(WebCore::ResourceHandle* parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

ResourceHandlePrivate::~ResourceHandlePrivate()
{
}

void
ResourceHandlePrivate::receivedCredential(const AuthenticationChallengePrivate& auth, const CredentialPrivate& cred)
{
    m_webcore->receivedCredential(auth.webcore(), cred.webcore());
}

void
ResourceHandlePrivate::receivedRequestToContinueWithoutCredential(const AuthenticationChallengePrivate& auth)
{
    m_webcore->receivedRequestToContinueWithoutCredential(auth.webcore());
}

long
ResourceHandlePrivate::SSLVerifyOpenSSLResult() const
{
    WebCore::ResourceHandleInternal* i = m_webcore->getInternal();
    if (!i)
        return 0;
    return i->m_SSLVerifyPeerResult;
}

long
ResourceHandlePrivate::SSLVerifycURLResult() const
{
    WebCore::ResourceHandleInternal* i = m_webcore->getInternal();
    if (!i)
        return 0;
    return i->m_SSLVerifyHostResult;
}

const char*
ResourceHandlePrivate::url() const
{
    WebCore::ResourceHandleInternal* i = m_webcore->getInternal();
    if (!i)
        return 0;
    return i->m_url;
}

const char*
ResourceHandlePrivate::urlhost() const
{
    WebCore::ResourceHandleInternal* i = m_webcore->getInternal();
    if (!i)
        return 0;
    return i->m_urlhost;
}

int
ResourceHandlePrivate::contentComposition() const
{
    WebCore::ResourceHandleInternal* i = m_webcore->getInternal();
    if (!i)
        return 0;
    return i->m_composition;
}

void*
ResourceHandlePrivate::httphandle()
{
    WebCore::ResourceHandleInternal* i = m_webcore->getInternal();
    if (!i)
        return 0;
    return i->m_handle;
}

bool
ResourceHandlePrivate::isSSL() const
{
    WebCore::ResourceHandleInternal* i = m_webcore->getInternal();
    if (!i)
        return 0;
    return i->m_isSSL;
}

bool
ResourceHandlePrivate::isEVSSL() const
{
    WebCore::ResourceHandleInternal* i = m_webcore->getInternal();
    if (!i)
        return 0;
    return i->m_isEVSSL;
}

int
ResourceHandlePrivate::secureState() const
{
    WebCore::ResourceHandleInternal* i = m_webcore->getInternal();
    if (!i)
        return 0;
    return i->m_secureState;
}

int
ResourceHandlePrivate::secureLevel() const
{
    WebCore::ResourceHandleInternal* i = m_webcore->getInternal();
    if (!i)
        return 0;
    return i->m_secureLevel;
}

long long
ResourceHandlePrivate::expectedContentLength() const
{
    WebCore::ResourceHandleInternal* i = m_webcore->getInternal();
    if (!i)
        return 0;
    return i->m_response.expectedContentLength();
}

////////////////////////////////////////////////////////////////////////////////

ResourceHandle::ResourceHandle(ResourceHandlePrivate& parent)
    : m_private(parent)
{
}

ResourceHandle::~ResourceHandle()
{
}

void
ResourceHandle::receivedCredential(const AuthenticationChallenge& challenge, const Credential& cred)
{
    m_private.receivedCredential(challenge.priv(), *cred.priv());
}

void
ResourceHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& challenge)
{
    m_private.receivedRequestToContinueWithoutCredential(challenge.priv());
}

long
ResourceHandle::SSLVerifyOpenSSLResult() const
{
    return m_private.SSLVerifyOpenSSLResult();
}

long
ResourceHandle::SSLVerifycURLResult() const
{
    return m_private.SSLVerifycURLResult();
}

const char*
ResourceHandle::url() const
{
    return m_private.url();
}

const char*
ResourceHandle::urlhost() const
{
    return m_private.urlhost();
}

int
ResourceHandle::contentComposition() const
{
    return m_private.contentComposition();
}

void*
ResourceHandle::httphandle()
{
    return m_private.httphandle();
}

bool
ResourceHandle::isSSL() const
{
    return m_private.isSSL();
}

bool
ResourceHandle::isEVSSL() const
{
    return m_private.isEVSSL();
}

int
ResourceHandle::secureState() const
{
    return m_private.secureState();
}

int
ResourceHandle::secureLevel() const
{
    return m_private.secureLevel();
}

long long
ResourceHandle::expectedContentLength() const
{
    return m_private.expectedContentLength();
}

} // namespace

/*
 * Copyright (c) 2010-2021 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
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
#include "ResourceRequest.h"
#include "ResourceHandleManagerWKC.h"

namespace WebCore {

void
ResourceRequest::doUpdatePlatformHTTPBody()
{
}

void
ResourceRequest::doUpdateResourceHTTPBody()
{
}

ResourceRequest& ResourceRequest::operator=(int)
{
    setURL(URL());
    removeCredentials();
    clearHTTPAuthorization();
    clearHTTPContentType();
    clearHTTPReferrer();
    clearHTTPOrigin();
    clearHTTPUserAgent();
    clearHTTPAccept();
    m_resourceRequestUpdated = false;
    m_platformRequestUpdated = true;
    m_resourceRequestBodyUpdated = false;
    m_platformRequestBodyUpdated = true;
    m_priority = ResourceLoadPriority::Low;

    m_isMainResource = false;
    m_targetType = TargetIsSubresource;
    return *this;
}

void ResourceRequest::clearContentLength()
{
    updateResourceRequest(); 

    m_httpHeaderFields.remove(HTTPHeaderName::ContentLength);

    m_platformRequestUpdated = false;
}

void ResourceRequest::clearRequestBodyHeaders()
{
    updateResourceRequest(); 

    m_httpHeaderFields.remove(HTTPHeaderName::ContentEncoding);
    m_httpHeaderFields.remove(HTTPHeaderName::ContentLanguage);
    m_httpHeaderFields.remove("Content-Location"_s);
    m_httpHeaderFields.remove(HTTPHeaderName::ContentType);

    m_platformRequestUpdated = false;
}

unsigned initializeMaximumHTTPConnectionCountPerHost()
{
    // This is used by the loader to control the number of issued parallel load requests. 
    // Four seems to be a common default in HTTP frameworks.
    ResourceHandleManager* mgr = ResourceHandleManager::sharedInstance();
    if (mgr)
        return mgr->maximumHTTPConnectionCountPerHostWindow();
    else
        return 8;
}

} // namespace WebCore

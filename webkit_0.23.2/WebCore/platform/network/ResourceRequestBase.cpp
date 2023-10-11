/*
 * Copyright (C) 2003, 2006 Apple Inc.  All rights reserved.
 * Copyright (C) 2009, 2012 Google Inc. All rights reserved.
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
#include "ResourceRequestBase.h"

#include "HTTPHeaderNames.h"
#include "ResourceRequest.h"

namespace WebCore {

#if PLATFORM(WKC)
// We don't use WKC_DEFINE_GLOBAL_CLASS_OBJ for s_defaultTimeoutInterval because WKC_DEFINE_GLOBAL_CLASS_OBJ will reset s_defaultTimeoutInterval to 0, not to INT_MAX.
double ResourceRequestBase::s_defaultTimeoutInterval = INT_MAX;
void
ResourceRequestBase::forceTerminate()
{
    s_defaultTimeoutInterval = INT_MAX;
}
#elif !USE(SOUP) && (!PLATFORM(MAC) || USE(CFNETWORK))
double ResourceRequestBase::s_defaultTimeoutInterval = INT_MAX;
#else
// Will use NSURLRequest default timeout unless set to a non-zero value with setDefaultTimeoutInterval().
// For libsoup the timeout enabled with integer milliseconds. We set 0 as the default value to avoid integer overflow.
double ResourceRequestBase::s_defaultTimeoutInterval = 0;
#endif

#if PLATFORM(IOS)
bool ResourceRequestBase::s_defaultAllowCookies = true;
#endif

inline const ResourceRequest& ResourceRequestBase::asResourceRequest() const
{
    return *static_cast<const ResourceRequest*>(this);
}

std::unique_ptr<ResourceRequest> ResourceRequestBase::adopt(std::unique_ptr<CrossThreadResourceRequestData> data)
{
    auto request = std::make_unique<ResourceRequest>();
    request->setURL(data->url);
    request->setCachePolicy(data->cachePolicy);
    request->setTimeoutInterval(data->timeoutInterval);
    request->setFirstPartyForCookies(data->firstPartyForCookies);
    request->setHTTPMethod(data->httpMethod);
    request->setPriority(data->priority);
    request->setRequester(data->requester);

    request->updateResourceRequest();
    request->m_httpHeaderFields.adopt(WTF::move(data->httpHeaders));

    size_t encodingCount = data->responseContentDispositionEncodingFallbackArray.size();
    if (encodingCount > 0) {
        String encoding1 = data->responseContentDispositionEncodingFallbackArray[0];
        String encoding2;
        String encoding3;
        if (encodingCount > 1) {
            encoding2 = data->responseContentDispositionEncodingFallbackArray[1];
            if (encodingCount > 2)
                encoding3 = data->responseContentDispositionEncodingFallbackArray[2];
        }
        ASSERT(encodingCount <= 3);
        request->setResponseContentDispositionEncodingFallbackArray(encoding1, encoding2, encoding3);
    }
    request->setHTTPBody(data->httpBody);
    request->setAllowCookies(data->allowCookies);
    request->doPlatformAdopt(WTF::move(data));
    return request;
}

std::unique_ptr<CrossThreadResourceRequestData> ResourceRequestBase::copyData() const
{
    auto data = std::make_unique<CrossThreadResourceRequestData>();
    data->url = url().isolatedCopy();
    data->cachePolicy = m_cachePolicy;
    data->timeoutInterval = timeoutInterval();
    data->firstPartyForCookies = firstPartyForCookies().isolatedCopy();
    data->httpMethod = httpMethod().isolatedCopy();
    data->httpHeaders = httpHeaderFields().copyData();
    data->priority = m_priority;
    data->requester = m_requester;

    data->responseContentDispositionEncodingFallbackArray.reserveInitialCapacity(m_responseContentDispositionEncodingFallbackArray.size());
    size_t encodingArraySize = m_responseContentDispositionEncodingFallbackArray.size();
    for (size_t index = 0; index < encodingArraySize; ++index) {
        data->responseContentDispositionEncodingFallbackArray.append(m_responseContentDispositionEncodingFallbackArray[index].isolatedCopy());
    }
    if (m_httpBody)
        data->httpBody = m_httpBody->deepCopy();
    data->allowCookies = m_allowCookies;
    return asResourceRequest().doPlatformCopyData(WTF::move(data));
}

bool ResourceRequestBase::isEmpty() const
{
    updateResourceRequest(); 
    
    return m_url.isEmpty(); 
}

bool ResourceRequestBase::isNull() const
{
    updateResourceRequest(); 
    
    return m_url.isNull();
}

const URL& ResourceRequestBase::url() const 
{
    updateResourceRequest(); 
    
    return m_url;
}

void ResourceRequestBase::setURL(const URL& url)
{ 
    updateResourceRequest(); 

    m_url = url; 
    
    m_platformRequestUpdated = false;
}

void ResourceRequestBase::removeCredentials()
{
    updateResourceRequest(); 

    if (m_url.user().isEmpty() && m_url.pass().isEmpty())
        return;

    m_url.setUser(String());
    m_url.setPass(String());

    m_platformRequestUpdated = false;
}

ResourceRequestCachePolicy ResourceRequestBase::cachePolicy() const
{
    updateResourceRequest(); 
    
    return m_cachePolicy;
}

void ResourceRequestBase::setCachePolicy(ResourceRequestCachePolicy cachePolicy)
{
    updateResourceRequest(); 

    if (m_cachePolicy == cachePolicy)
        return;
    
    m_cachePolicy = cachePolicy;
    
    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

double ResourceRequestBase::timeoutInterval() const
{
    updateResourceRequest(); 
    
    return m_timeoutInterval; 
}

void ResourceRequestBase::setTimeoutInterval(double timeoutInterval) 
{
    updateResourceRequest(); 
    
    if (m_timeoutInterval == timeoutInterval)
        return;

    m_timeoutInterval = timeoutInterval;
    
    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

const URL& ResourceRequestBase::firstPartyForCookies() const
{
    updateResourceRequest(); 
    
    return m_firstPartyForCookies;
}

void ResourceRequestBase::setFirstPartyForCookies(const URL& firstPartyForCookies)
{ 
    updateResourceRequest(); 

    if (m_firstPartyForCookies == firstPartyForCookies)
        return;

    m_firstPartyForCookies = firstPartyForCookies;
    
    m_platformRequestUpdated = false;
}

const String& ResourceRequestBase::httpMethod() const
{
    updateResourceRequest(); 
    
    return m_httpMethod; 
}

void ResourceRequestBase::setHTTPMethod(const String& httpMethod) 
{
    updateResourceRequest(); 

    if (m_httpMethod == httpMethod)
        return;

    m_httpMethod = httpMethod;
    
    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

const HTTPHeaderMap& ResourceRequestBase::httpHeaderFields() const
{
    updateResourceRequest(); 

    return m_httpHeaderFields; 
}

String ResourceRequestBase::httpHeaderField(const String& name) const
{
    updateResourceRequest(); 
    
    return m_httpHeaderFields.get(name);
}

String ResourceRequestBase::httpHeaderField(HTTPHeaderName name) const
{
    updateResourceRequest(); 
    
    return m_httpHeaderFields.get(name);
}

void ResourceRequestBase::setHTTPHeaderField(const String& name, const String& value)
{
    updateResourceRequest();

    m_httpHeaderFields.set(name, value);
    
    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::setHTTPHeaderField(HTTPHeaderName name, const String& value)
{
    updateResourceRequest();

    m_httpHeaderFields.set(name, value);

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::clearHTTPAuthorization()
{
    updateResourceRequest(); 

    if (!m_httpHeaderFields.remove(HTTPHeaderName::Authorization))
        return;

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

String ResourceRequestBase::httpContentType() const
{
    return httpHeaderField(HTTPHeaderName::ContentType);
}

void ResourceRequestBase::setHTTPContentType(const String& httpContentType)
{
    setHTTPHeaderField(HTTPHeaderName::ContentType, httpContentType);
}

void ResourceRequestBase::clearHTTPContentType()
{
    updateResourceRequest(); 

    m_httpHeaderFields.remove(HTTPHeaderName::ContentType);

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

String ResourceRequestBase::httpReferrer() const
{
    return httpHeaderField(HTTPHeaderName::Referer);
}

void ResourceRequestBase::setHTTPReferrer(const String& httpReferrer)
{
    setHTTPHeaderField(HTTPHeaderName::Referer, httpReferrer);
}

void ResourceRequestBase::clearHTTPReferrer()
{
    updateResourceRequest(); 

    m_httpHeaderFields.remove(HTTPHeaderName::Referer);

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

String ResourceRequestBase::httpOrigin() const
{
    return httpHeaderField(HTTPHeaderName::Origin);
}

void ResourceRequestBase::setHTTPOrigin(const String& httpOrigin)
{
    setHTTPHeaderField(HTTPHeaderName::Origin, httpOrigin);
}

void ResourceRequestBase::clearHTTPOrigin()
{
    updateResourceRequest(); 

    m_httpHeaderFields.remove(HTTPHeaderName::Origin);

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

String ResourceRequestBase::httpUserAgent() const
{
    return httpHeaderField(HTTPHeaderName::UserAgent);
}

void ResourceRequestBase::setHTTPUserAgent(const String& httpUserAgent)
{
    setHTTPHeaderField(HTTPHeaderName::UserAgent, httpUserAgent);
}

void ResourceRequestBase::clearHTTPUserAgent()
{
    updateResourceRequest(); 

    m_httpHeaderFields.remove(HTTPHeaderName::UserAgent);

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

String ResourceRequestBase::httpAccept() const
{
    return httpHeaderField(HTTPHeaderName::Accept);
}

void ResourceRequestBase::setHTTPAccept(const String& httpAccept)
{
    setHTTPHeaderField(HTTPHeaderName::Accept, httpAccept);
}

void ResourceRequestBase::clearHTTPAccept()
{
    updateResourceRequest(); 

    m_httpHeaderFields.remove(HTTPHeaderName::Accept);

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::setResponseContentDispositionEncodingFallbackArray(const String& encoding1, const String& encoding2, const String& encoding3)
{
    updateResourceRequest(); 
    
    m_responseContentDispositionEncodingFallbackArray.clear();
    m_responseContentDispositionEncodingFallbackArray.reserveInitialCapacity(!encoding1.isNull() + !encoding2.isNull() + !encoding3.isNull());
    if (!encoding1.isNull())
        m_responseContentDispositionEncodingFallbackArray.uncheckedAppend(encoding1);
    if (!encoding2.isNull())
        m_responseContentDispositionEncodingFallbackArray.uncheckedAppend(encoding2);
    if (!encoding3.isNull())
        m_responseContentDispositionEncodingFallbackArray.uncheckedAppend(encoding3);
    
    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

FormData* ResourceRequestBase::httpBody() const
{
    updateResourceRequest(UpdateHTTPBody);

    return m_httpBody.get();
}

void ResourceRequestBase::setHTTPBody(PassRefPtr<FormData> httpBody)
{
    updateResourceRequest();

    m_httpBody = httpBody;

    m_resourceRequestBodyUpdated = true;

    if (url().protocolIsInHTTPFamily())
        m_platformRequestBodyUpdated = false;
}

bool ResourceRequestBase::allowCookies() const
{
    updateResourceRequest(); 
    
    return m_allowCookies;
}

void ResourceRequestBase::setAllowCookies(bool allowCookies)
{
    updateResourceRequest(); 

    if (m_allowCookies == allowCookies)
        return;

    m_allowCookies = allowCookies;
    
    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

ResourceLoadPriority ResourceRequestBase::priority() const
{
    updateResourceRequest();

    return m_priority;
}

void ResourceRequestBase::setPriority(ResourceLoadPriority priority)
{
    updateResourceRequest();

    if (m_priority == priority)
        return;

    m_priority = priority;

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::addHTTPHeaderField(const String& name, const String& value) 
{
    updateResourceRequest();

    m_httpHeaderFields.add(name, value);

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

void ResourceRequestBase::setHTTPHeaderFields(HTTPHeaderMap headerFields)
{
    updateResourceRequest();

    m_httpHeaderFields = WTF::move(headerFields);

    if (url().protocolIsInHTTPFamily())
        m_platformRequestUpdated = false;
}

bool equalIgnoringHeaderFields(const ResourceRequestBase& a, const ResourceRequestBase& b)
{
    if (a.url() != b.url())
        return false;
    
    if (a.cachePolicy() != b.cachePolicy())
        return false;
    
    if (a.timeoutInterval() != b.timeoutInterval())
        return false;
    
    if (a.firstPartyForCookies() != b.firstPartyForCookies())
        return false;
    
    if (a.httpMethod() != b.httpMethod())
        return false;
    
    if (a.allowCookies() != b.allowCookies())
        return false;
    
    if (a.priority() != b.priority())
        return false;

    if (a.requester() != b.requester())
        return false;

    FormData* formDataA = a.httpBody();
    FormData* formDataB = b.httpBody();
    
    if (!formDataA)
        return !formDataB;
    if (!formDataB)
        return !formDataA;
    
    if (*formDataA != *formDataB)
        return false;
    
    return true;
}

bool ResourceRequestBase::compare(const ResourceRequest& a, const ResourceRequest& b)
{
    if (!equalIgnoringHeaderFields(a, b))
        return false;
    
    if (a.httpHeaderFields() != b.httpHeaderFields())
        return false;
        
    return ResourceRequest::platformCompare(a, b);
}

static const HTTPHeaderName conditionalHeaderNames[] = {
    HTTPHeaderName::IfMatch,
    HTTPHeaderName::IfModifiedSince,
    HTTPHeaderName::IfNoneMatch,
    HTTPHeaderName::IfRange,
    HTTPHeaderName::IfUnmodifiedSince
};

bool ResourceRequestBase::isConditional() const
{
    for (auto headerName : conditionalHeaderNames) {
        if (m_httpHeaderFields.contains(headerName))
            return true;
    }

    return false;
}

void ResourceRequestBase::makeUnconditional()
{
    for (auto headerName : conditionalHeaderNames)
        m_httpHeaderFields.remove(headerName);
}

double ResourceRequestBase::defaultTimeoutInterval()
{
    return s_defaultTimeoutInterval;
}

void ResourceRequestBase::setDefaultTimeoutInterval(double timeoutInterval)
{
    s_defaultTimeoutInterval = timeoutInterval;
}

void ResourceRequestBase::updatePlatformRequest(HTTPBodyUpdatePolicy bodyPolicy) const
{
    if (!m_platformRequestUpdated) {
        ASSERT(m_resourceRequestUpdated);
        const_cast<ResourceRequest&>(asResourceRequest()).doUpdatePlatformRequest();
        m_platformRequestUpdated = true;
    }

    if (!m_platformRequestBodyUpdated && bodyPolicy == UpdateHTTPBody) {
        ASSERT(m_resourceRequestBodyUpdated);
        const_cast<ResourceRequest&>(asResourceRequest()).doUpdatePlatformHTTPBody();
        m_platformRequestBodyUpdated = true;
    }
}

void ResourceRequestBase::updateResourceRequest(HTTPBodyUpdatePolicy bodyPolicy) const
{
    if (!m_resourceRequestUpdated) {
        ASSERT(m_platformRequestUpdated);
        const_cast<ResourceRequest&>(asResourceRequest()).doUpdateResourceRequest();
        m_resourceRequestUpdated = true;
    }

    if (!m_resourceRequestBodyUpdated && bodyPolicy == UpdateHTTPBody) {
        ASSERT(m_platformRequestBodyUpdated);
        const_cast<ResourceRequest&>(asResourceRequest()).doUpdateResourceHTTPBody();
        m_resourceRequestBodyUpdated = true;
    }
}

#if !PLATFORM(COCOA) && !USE(CFNETWORK) && !USE(SOUP) && !PLATFORM(WKC)
unsigned initializeMaximumHTTPConnectionCountPerHost()
{
    // This is used by the loader to control the number of issued parallel load requests. 
    // Four seems to be a common default in HTTP frameworks.
    return 4;
}
#endif

#if PLATFORM(IOS)
void ResourceRequestBase::setDefaultAllowCookies(bool allowCookies)
{
    s_defaultAllowCookies = allowCookies;
}

bool ResourceRequestBase::defaultAllowCookies()
{
    return s_defaultAllowCookies;
}
#endif

}

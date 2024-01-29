/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "ApplicationCache.h"

#include "ApplicationCacheGroup.h"
#include "ApplicationCacheResource.h"
#include "ApplicationCacheStorage.h"
#include "ResourceRequest.h"
#include "SecurityOrigin.h"
#include <algorithm>
#include <stdio.h>
#include <wtf/text/CString.h>

#if PLATFORM(WKC)
template<> inline std::pair<std::pair<WebCore::URL, WebCore::URL>*, ptrdiff_t>
std::get_temporary_buffer<std::pair<WebCore::URL, WebCore::URL>>(ptrdiff_t count) _NOEXCEPT
{
    std::pair<WebCore::URL, WebCore::URL>* p = (std::pair<WebCore::URL, WebCore::URL>*)WTF::fastMalloc(sizeof(std::pair<WebCore::URL, WebCore::URL>) * count);
    return (std::pair<std::pair<WebCore::URL, WebCore::URL>*, ptrdiff_t>(p, count));
}
template<> inline void
std::return_temporary_buffer<std::pair<WebCore::URL, WebCore::URL>>(std::pair<WebCore::URL, WebCore::URL>* buf)
{
    WTF::fastFree(buf);
}
#endif

namespace WebCore {
 
static inline bool fallbackURLLongerThan(const std::pair<URL, URL>& lhs, const std::pair<URL, URL>& rhs)
{
    return lhs.first.string().length() > rhs.first.string().length();
}

ApplicationCache::ApplicationCache()
    : m_group(0)
    , m_manifest(0)
    , m_estimatedSizeInStorage(0)
    , m_storageID(0)
{
}

ApplicationCache::~ApplicationCache()
{
    if (m_group)
        m_group->cacheDestroyed(this);
}
    
void ApplicationCache::setGroup(ApplicationCacheGroup* group)
{
    ASSERT(!m_group || group == m_group);
    m_group = group;
}

bool ApplicationCache::isComplete()
{
    return m_group && m_group->cacheIsComplete(this);
}

void ApplicationCache::setManifestResource(PassRefPtr<ApplicationCacheResource> manifest)
{
    ASSERT(manifest);
    ASSERT(!m_manifest);
    ASSERT(manifest->type() & ApplicationCacheResource::Manifest);
    
    m_manifest = manifest.get();
    
    addResource(manifest);
}
    
void ApplicationCache::addResource(PassRefPtr<ApplicationCacheResource> resource)
{
    ASSERT(resource);
    
    const String& url = resource->url();
    
    ASSERT(!m_resources.contains(url));
    
    if (m_storageID) {
        ASSERT(!resource->storageID());
        ASSERT(resource->type() & ApplicationCacheResource::Master);
        
        // Add the resource to the storage.
        ApplicationCacheStorage::singleton().store(resource.get(), this);
    }

    m_estimatedSizeInStorage += resource->estimatedSizeInStorage();

    m_resources.set(url, resource);
}

unsigned ApplicationCache::removeResource(const String& url)
{
    HashMap<String, RefPtr<ApplicationCacheResource>>::iterator it = m_resources.find(url);
    if (it == m_resources.end())
        return 0;

    // The resource exists, get its type so we can return it.
    unsigned type = it->value->type();

    m_estimatedSizeInStorage -= it->value->estimatedSizeInStorage();

    m_resources.remove(it);

    return type;
}    
    
ApplicationCacheResource* ApplicationCache::resourceForURL(const String& url)
{
    ASSERT(!URL(ParsedURLString, url).hasFragmentIdentifier());
    return m_resources.get(url);
}    

bool ApplicationCache::requestIsHTTPOrHTTPSGet(const ResourceRequest& request)
{
    if (!request.url().protocolIsInHTTPFamily())
        return false;
    
    if (!equalIgnoringCase(request.httpMethod(), "GET"))
        return false;

    return true;
}    

ApplicationCacheResource* ApplicationCache::resourceForRequest(const ResourceRequest& request)
{
    // We only care about HTTP/HTTPS GET requests.
    if (!requestIsHTTPOrHTTPSGet(request))
        return 0;

    URL url(request.url());
    if (url.hasFragmentIdentifier())
        url.removeFragmentIdentifier();

    return resourceForURL(url);
}

void ApplicationCache::setOnlineWhitelist(const Vector<URL>& onlineWhitelist)
{
    ASSERT(m_onlineWhitelist.isEmpty());
    m_onlineWhitelist = onlineWhitelist; 
}

bool ApplicationCache::isURLInOnlineWhitelist(const URL& url)
{
    size_t whitelistSize = m_onlineWhitelist.size();
    for (size_t i = 0; i < whitelistSize; ++i) {
        if (protocolHostAndPortAreEqual(url, m_onlineWhitelist[i]) && url.string().startsWith(m_onlineWhitelist[i].string()))
            return true;
    }
    return false;
}

void ApplicationCache::setFallbackURLs(const FallbackURLVector& fallbackURLs)
{
    ASSERT(m_fallbackURLs.isEmpty());
    m_fallbackURLs = fallbackURLs;
    // FIXME: What's the right behavior if we have 2 or more identical namespace URLs?
    std::stable_sort(m_fallbackURLs.begin(), m_fallbackURLs.end(), fallbackURLLongerThan);
}

bool ApplicationCache::urlMatchesFallbackNamespace(const URL& url, URL* fallbackURL)
{
    size_t fallbackCount = m_fallbackURLs.size();
    for (size_t i = 0; i < fallbackCount; ++i) {
        if (protocolHostAndPortAreEqual(url, m_fallbackURLs[i].first) && url.string().startsWith(m_fallbackURLs[i].first.string())) {
            if (fallbackURL)
                *fallbackURL = m_fallbackURLs[i].second;
            return true;
        }
    }
    return false;
}

void ApplicationCache::clearStorageID()
{
    m_storageID = 0;
    
    for (const auto& resource : m_resources.values())
        resource->clearStorageID();
}
    
#ifndef NDEBUG
void ApplicationCache::dump()
{
    for (const auto& urlAndResource : m_resources) {
        printf("%s ", urlAndResource.key.utf8().data());
        ApplicationCacheResource::dumpType(urlAndResource.value->type());
    }
}
#endif

}

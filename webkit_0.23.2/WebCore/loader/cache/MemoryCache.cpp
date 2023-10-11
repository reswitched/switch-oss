/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "MemoryCache.h"

#include "BitmapImage.h"
#include "CachedImage.h"
#include "CachedImageClient.h"
#include "CachedResource.h"
#include "CachedResourceHandle.h"
#include "Document.h"
#include "FrameLoader.h"
#include "FrameLoaderTypes.h"
#include "FrameView.h"
#include "Image.h"
#include "Logging.h"
#include "PublicSuffix.h"
#include "SharedBuffer.h"
#include "WorkerGlobalScope.h"
#include "WorkerLoaderProxy.h"
#include "WorkerThread.h"
#include <stdio.h>
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/TemporaryChange.h>
#include <wtf/text/CString.h>

namespace WebCore {

static const int cDefaultCacheCapacity = 8192 * 1024;
static const double cMinDelayBeforeLiveDecodedPrune = 1; // Seconds.
static const float cTargetPrunePercentage = .95f; // Percentage of capacity toward which we prune, to avoid immediately pruning again.
static const auto defaultDecodedDataDeletionInterval = std::chrono::seconds { 0 };

#if PLATFORM(WKC)
static double gMinDelayBeforeLiveDecodedPrune = cMinDelayBeforeLiveDecodedPrune;
#endif

MemoryCache& MemoryCache::singleton()
{
#if !PLATFORM(WKC)
    ASSERT(WTF::isMainThread());
    static NeverDestroyed<MemoryCache> memoryCache;
    return memoryCache;
#else
    WKC_DEFINE_STATIC_PTR(MemoryCache*, memoryCache, 0);
    if (!memoryCache)
        memoryCache = new MemoryCache();
    return *memoryCache;
#endif
}

MemoryCache::MemoryCache()
    : m_disabled(false)
    , m_inPruneResources(false)
    , m_capacity(cDefaultCacheCapacity)
    , m_minDeadCapacity(0)
    , m_maxDeadCapacity(cDefaultCacheCapacity)
    , m_deadDecodedDataDeletionInterval(defaultDecodedDataDeletionInterval)
    , m_liveSize(0)
    , m_deadSize(0)
    , m_pruneTimer(*this, &MemoryCache::pruneTimerFired)
{
    static_assert(sizeof(long long) > sizeof(unsigned), "Numerical overflow can happen when adjusting the size of the cached memory.");
}

auto MemoryCache::sessionResourceMap(SessionID sessionID) const -> CachedResourceMap*
{
    ASSERT(sessionID.isValid());
    return m_sessionResources.get(sessionID);
}

auto MemoryCache::ensureSessionResourceMap(SessionID sessionID) -> CachedResourceMap&
{
    ASSERT(sessionID.isValid());
    auto& map = m_sessionResources.add(sessionID, nullptr).iterator->value;
    if (!map)
        map = std::make_unique<CachedResourceMap>();
    return *map;
}

URL MemoryCache::removeFragmentIdentifierIfNeeded(const URL& originalURL)
{
    if (!originalURL.hasFragmentIdentifier())
        return originalURL;
    // Strip away fragment identifier from HTTP URLs.
    // Data URLs must be unmodified. For file and custom URLs clients may expect resources 
    // to be unique even when they differ by the fragment identifier only.
    if (!originalURL.protocolIsInHTTPFamily())
        return originalURL;
    URL url = originalURL;
    url.removeFragmentIdentifier();
    return url;
}

bool MemoryCache::add(CachedResource& resource)
{
    if (disabled())
        return false;

    ASSERT(WTF::isMainThread());

#if ENABLE(CACHE_PARTITIONING)
    auto key = std::make_pair(resource.url(), resource.cachePartition());
#else
    auto& key = resource.url();
#endif
    ensureSessionResourceMap(resource.sessionID()).set(key, &resource);
    resource.setInCache(true);
    
    resourceAccessed(resource);
    
    LOG(ResourceLoading, "MemoryCache::add Added '%s', resource %p\n", resource.url().string().latin1().data(), &resource);
    return true;
}

void MemoryCache::revalidationSucceeded(CachedResource& revalidatingResource, const ResourceResponse& response)
{
    ASSERT(revalidatingResource.resourceToRevalidate());
    CachedResource& resource = *revalidatingResource.resourceToRevalidate();
    ASSERT(!resource.inCache());
    ASSERT(resource.isLoaded());

    // Calling remove() can potentially delete revalidatingResource, which we use
    // below. This mustn't be the case since revalidation means it is loaded
    // and so canDelete() is false.
    ASSERT(!revalidatingResource.canDelete());

    remove(revalidatingResource);

    auto& resources = ensureSessionResourceMap(resource.sessionID());
#if ENABLE(CACHE_PARTITIONING)
    auto key = std::make_pair(resource.url(), resource.cachePartition());
#else
    auto& key = resource.url();
#endif
#if !PLATFORM(WKC)
    ASSERT(!resources.get(key));
#endif
    resources.set(key, &resource);
    resource.setInCache(true);
    resource.updateResponseAfterRevalidation(response);
    insertInLRUList(resource);
    long long delta = resource.size();
    if (resource.decodedSize() && resource.hasClients())
        insertInLiveDecodedResourcesList(resource);
    if (delta)
        adjustSize(resource.hasClients(), delta);
    
    revalidatingResource.switchClientsToRevalidatedResource();
    ASSERT(!revalidatingResource.m_deleted);
    // this deletes the revalidating resource
    revalidatingResource.clearResourceToRevalidate();
}

void MemoryCache::revalidationFailed(CachedResource& revalidatingResource)
{
    ASSERT(WTF::isMainThread());
    LOG(ResourceLoading, "Revalidation failed for %p", &revalidatingResource);
    ASSERT(revalidatingResource.resourceToRevalidate());
    revalidatingResource.clearResourceToRevalidate();
}

CachedResource* MemoryCache::resourceForRequest(const ResourceRequest& request, SessionID sessionID)
{
    auto* resources = sessionResourceMap(sessionID);
    if (!resources)
        return nullptr;
    return resourceForRequestImpl(request, *resources);
}

CachedResource* MemoryCache::resourceForRequestImpl(const ResourceRequest& request, CachedResourceMap& resources)
{
    ASSERT(WTF::isMainThread());
    URL url = removeFragmentIdentifierIfNeeded(request.url());

#if ENABLE(CACHE_PARTITIONING)
    auto key = std::make_pair(url, request.cachePartition());
#else
    auto& key = url;
#endif
    return resources.get(key);
}

unsigned MemoryCache::deadCapacity() const 
{
    // Dead resource capacity is whatever space is not occupied by live resources, bounded by an independent minimum and maximum.
    unsigned capacity = m_capacity - std::min(m_liveSize, m_capacity); // Start with available capacity.
    capacity = std::max(capacity, m_minDeadCapacity); // Make sure it's above the minimum.
    capacity = std::min(capacity, m_maxDeadCapacity); // Make sure it's below the maximum.
    return capacity;
}

unsigned MemoryCache::liveCapacity() const 
{ 
    // Live resource capacity is whatever is left over after calculating dead resource capacity.
    return m_capacity - deadCapacity();
}

#if USE(CG)
// FIXME: Remove the USE(CG) once we either make NativeImagePtr a smart pointer on all platforms or
// remove the usage of CFRetain() in MemoryCache::addImageToCache() so as to make the code platform-independent.
static CachedImageClient& dummyCachedImageClient()
{
    static NeverDestroyed<CachedImageClient> client;
    return client;
}

bool MemoryCache::addImageToCache(NativeImagePtr image, const URL& url, const String& domainForCachePartition)
{
    ASSERT(image);
    SessionID sessionID = SessionID::defaultSessionID();
    removeImageFromCache(url, domainForCachePartition); // Remove cache entry if it already exists.

    RefPtr<BitmapImage> bitmapImage = BitmapImage::create(image, nullptr);
    if (!bitmapImage)
        return false;

    std::unique_ptr<CachedImage> cachedImage = std::make_unique<CachedImage>(url, bitmapImage.get(), CachedImage::ManuallyCached, sessionID);

    // Actual release of the CGImageRef is done in BitmapImage.
    CFRetain(image);
    cachedImage->addClient(&dummyCachedImageClient());
    cachedImage->setDecodedSize(bitmapImage->decodedSize());
#if ENABLE(CACHE_PARTITIONING)
    cachedImage->resourceRequest().setDomainForCachePartition(domainForCachePartition);
#endif
    return add(*cachedImage.release());
}

void MemoryCache::removeImageFromCache(const URL& url, const String& domainForCachePartition)
{
    auto* resources = sessionResourceMap(SessionID::defaultSessionID());
    if (!resources)
        return;

#if ENABLE(CACHE_PARTITIONING)
    auto key = std::make_pair(url, ResourceRequest::partitionName(domainForCachePartition));
#else
    UNUSED_PARAM(domainForCachePartition);
    auto& key = url;
#endif
    CachedResource* resource = resources->get(key);
    if (!resource)
        return;

    // A resource exists and is not a manually cached image, so just remove it.
    if (!is<CachedImage>(*resource) || !downcast<CachedImage>(*resource).isManuallyCached()) {
        remove(*resource);
        return;
    }

    // Removing the last client of a CachedImage turns the resource
    // into a dead resource which will eventually be evicted when
    // dead resources are pruned. That might be immediately since
    // removing the last client triggers a MemoryCache::prune, so the
    // resource may be deleted after this call.
    downcast<CachedImage>(*resource).removeClient(&dummyCachedImageClient());
}
#endif

void MemoryCache::pruneLiveResources(bool shouldDestroyDecodedDataForAllLiveResources)
{
    unsigned capacity = shouldDestroyDecodedDataForAllLiveResources ? 0 : liveCapacity();
    if (capacity && m_liveSize <= capacity)
        return;

    unsigned targetSize = static_cast<unsigned>(capacity * cTargetPrunePercentage); // Cut by a percentage to avoid immediately pruning again.

    pruneLiveResourcesToSize(targetSize, shouldDestroyDecodedDataForAllLiveResources);
}

void MemoryCache::pruneLiveResourcesToSize(unsigned targetSize, bool shouldDestroyDecodedDataForAllLiveResources)
{
    if (m_inPruneResources)
        return;
    TemporaryChange<bool> reentrancyProtector(m_inPruneResources, true);

    double currentTime = FrameView::currentPaintTimeStamp();
    if (!currentTime) // In case prune is called directly, outside of a Frame paint.
        currentTime = monotonicallyIncreasingTime();
    
    // Destroy any decoded data in live objects that we can.
    // Start from the head, since this is the least recently accessed of the objects.

    // The list might not be sorted by the m_lastDecodedAccessTime. The impact
    // of this weaker invariant is minor as the below if statement to check the
    // elapsedTime will evaluate to false as the currentTime will be a lot
    // greater than the current->m_lastDecodedAccessTime.
    // For more details see: https://bugs.webkit.org/show_bug.cgi?id=30209
#if !PLATFORM(WKC)
    auto it = m_liveDecodedResources.begin();
    while (it != m_liveDecodedResources.end()) {
#else
    auto copyLiveDecodedResources = m_liveDecodedResources;
    auto it = copyLiveDecodedResources.begin();
    while (it != copyLiveDecodedResources.end()) {
#endif
        auto* current = *it;

        // Increment the iterator now because the call to destroyDecodedData() below
        // may cause a call to ListHashSet::remove() and invalidate the current
        // iterator. Note that this is safe because unlike iteration of most
        // WTF Hash data structures, iteration is guaranteed safe against mutation
        // of the ListHashSet, except for removal of the item currently pointed to
        // by a given iterator.
        ++it;

#if PLATFORM(WKC)
        if (!m_liveDecodedResources.contains(current))
            continue;
#endif

        ASSERT(current->hasClients());
        if (current->isLoaded() && current->decodedSize()) {
            // Check to see if the remaining resources are too new to prune.
            double elapsedTime = currentTime - current->m_lastDecodedAccessTime;
#if !PLATFORM(WKC)
            if (!shouldDestroyDecodedDataForAllLiveResources && elapsedTime < cMinDelayBeforeLiveDecodedPrune)
#else
            if (!shouldDestroyDecodedDataForAllLiveResources && elapsedTime < gMinDelayBeforeLiveDecodedPrune)
#endif
                return;

            if (current->decodedDataIsPurgeable())
                continue;

            // Destroy our decoded data. This will remove us from m_liveDecodedResources, and possibly move us
            // to a different LRU list in m_allResources.
            current->destroyDecodedData();

            if (targetSize && m_liveSize <= targetSize)
                return;
        }
    }
}

void MemoryCache::pruneDeadResources()
{
    unsigned capacity = deadCapacity();
    if (capacity && m_deadSize <= capacity)
        return;

    unsigned targetSize = static_cast<unsigned>(capacity * cTargetPrunePercentage); // Cut by a percentage to avoid immediately pruning again.
    pruneDeadResourcesToSize(targetSize);
}

void MemoryCache::pruneDeadResourcesToSize(unsigned targetSize)
{
    if (m_inPruneResources)
        return;
    TemporaryChange<bool> reentrancyProtector(m_inPruneResources, true);
 
    if (targetSize && m_deadSize <= targetSize)
        return;

    bool canShrinkLRULists = true;
    for (int i = m_allResources.size() - 1; i >= 0; i--) {
        // Make a copy of the LRUList first (and ref the resources) as calling
        // destroyDecodedData() can alter the LRUList.
        Vector<CachedResourceHandle<CachedResource>> lruList;
        copyToVector(*m_allResources[i], lruList);

        // First flush all the decoded data in this queue.
        // Remove from the head, since this is the least frequently accessed of the objects.
        for (auto& resource : lruList) {
            if (!resource->inCache())
                continue;

            if (!resource->hasClients() && !resource->isPreloaded() && resource->isLoaded()) {
                // Destroy our decoded data. This will remove us from 
                // m_liveDecodedResources, and possibly move us to a different 
                // LRU list in m_allResources.
                resource->destroyDecodedData();

                if (targetSize && m_deadSize <= targetSize)
                    return;
            }
        }

        // Now evict objects from this list.
        // Remove from the head, since this is the least frequently accessed of the objects.
        for (auto& resource : lruList) {
            if (!resource->inCache())
                continue;

            if (!resource->hasClients() && !resource->isPreloaded() && !resource->isCacheValidator()) {
                remove(*resource);
                if (targetSize && m_deadSize <= targetSize)
                    return;
            }
        }
            
        // Shrink the vector back down so we don't waste time inspecting
        // empty LRU lists on future prunes.
        if (!m_allResources[i]->isEmpty())
            canShrinkLRULists = false;
        else if (canShrinkLRULists)
            m_allResources.shrink(i);
    }
}

void MemoryCache::setCapacities(unsigned minDeadBytes, unsigned maxDeadBytes, unsigned totalBytes)
{
    ASSERT(minDeadBytes <= maxDeadBytes);
    ASSERT(maxDeadBytes <= totalBytes);
    m_minDeadCapacity = minDeadBytes;
    m_maxDeadCapacity = maxDeadBytes;
    m_capacity = totalBytes;
    prune();
}

void MemoryCache::remove(CachedResource& resource)
{
    ASSERT(WTF::isMainThread());
    LOG(ResourceLoading, "Evicting resource %p for '%s' from cache", &resource, resource.url().string().latin1().data());
    // The resource may have already been removed by someone other than our caller,
    // who needed a fresh copy for a reload. See <http://bugs.webkit.org/show_bug.cgi?id=12479#c6>.
    if (auto* resources = sessionResourceMap(resource.sessionID())) {
#if ENABLE(CACHE_PARTITIONING)
        auto key = std::make_pair(resource.url(), resource.cachePartition());
#else
        auto& key = resource.url();
#endif
        if (resource.inCache()) {
            // Remove resource from the resource map.
            resources->remove(key);
            resource.setInCache(false);

            // If the resource map is now empty, remove it from m_sessionResources.
            if (resources->isEmpty())
                m_sessionResources.remove(resource.sessionID());

            // Remove from the appropriate LRU list.
            removeFromLRUList(resource);
            removeFromLiveDecodedResourcesList(resource);
            adjustSize(resource.hasClients(), -static_cast<long long>(resource.size()));
        } else
            ASSERT(resources->get(key) != &resource);
    }

    resource.deleteIfPossible();
}

auto MemoryCache::lruListFor(CachedResource& resource) -> LRUList&
{
    unsigned accessCount = std::max(resource.accessCount(), 1U);
    unsigned queueIndex = WTF::fastLog2(resource.size() / accessCount);
#ifndef NDEBUG
    resource.m_lruIndex = queueIndex;
#endif

    m_allResources.reserveCapacity(queueIndex + 1);
    while (m_allResources.size() <= queueIndex)
        m_allResources.uncheckedAppend(std::make_unique<LRUList>());
    return *m_allResources[queueIndex];
}

void MemoryCache::removeFromLRUList(CachedResource& resource)
{
    // If we've never been accessed, then we're brand new and not in any list.
    if (!resource.accessCount())
        return;

#if !ASSERT_DISABLED
    unsigned oldListIndex = resource.m_lruIndex;
#endif

    LRUList& list = lruListFor(resource);

    // Verify that the list we got is the list we want.
    ASSERT(resource.m_lruIndex == oldListIndex);

    bool removed = list.remove(&resource);
    ASSERT_UNUSED(removed, removed);
}

void MemoryCache::insertInLRUList(CachedResource& resource)
{
    ASSERT(resource.inCache());
    ASSERT(resource.accessCount() > 0);
    
    auto addResult = lruListFor(resource).add(&resource);
    ASSERT_UNUSED(addResult, addResult.isNewEntry);
}

void MemoryCache::resourceAccessed(CachedResource& resource)
{
    ASSERT(resource.inCache());
    
    // Need to make sure to remove before we increase the access count, since
    // the queue will possibly change.
    removeFromLRUList(resource);
    
    // If this is the first time the resource has been accessed, adjust the size of the cache to account for its initial size.
    if (!resource.accessCount())
        adjustSize(resource.hasClients(), resource.size());
    
    // Add to our access count.
    resource.increaseAccessCount();
    
    // Now insert into the new queue.
    insertInLRUList(resource);
}

void MemoryCache::removeResourcesWithOrigin(SecurityOrigin& origin)
{
#if ENABLE(CACHE_PARTITIONING)
    String originPartition = ResourceRequest::partitionName(origin.host());
#endif

    Vector<CachedResource*> resourcesWithOrigin;
    for (auto& resources : m_sessionResources.values()) {
        for (auto& keyValue : *resources) {
            auto& resource = *keyValue.value;
#if ENABLE(CACHE_PARTITIONING)
            auto& partitionName = keyValue.key.second;
            if (partitionName == originPartition) {
                resourcesWithOrigin.append(&resource);
                continue;
            }
#endif
            RefPtr<SecurityOrigin> resourceOrigin = SecurityOrigin::create(resource.url());
            if (resourceOrigin->equal(&origin))
                resourcesWithOrigin.append(&resource);
        }
    }

    for (auto* resource : resourcesWithOrigin)
        remove(*resource);
}

void MemoryCache::removeResourcesWithOrigins(SessionID sessionID, const HashSet<RefPtr<SecurityOrigin>>& origins)
{
    auto* resourceMap = sessionResourceMap(sessionID);
    if (!resourceMap)
        return;

#if ENABLE(CACHE_PARTITIONING)
    HashSet<String> originPartitions;

    for (auto& origin : origins)
        originPartitions.add(ResourceRequest::partitionName(origin->host()));
#endif

    Vector<CachedResource*> resourcesToRemove;
    for (auto& keyValuePair : *resourceMap) {
        auto& resource = *keyValuePair.value;

#if ENABLE(CACHE_PARTITIONING)
        auto& partitionName = keyValuePair.key.second;
        if (originPartitions.contains(partitionName)) {
            resourcesToRemove.append(&resource);
            continue;
        }
#endif

        if (origins.contains(SecurityOrigin::create(resource.url()).ptr()))
            resourcesToRemove.append(&resource);
    }

    for (auto& resource : resourcesToRemove)
        remove(*resource);
}

void MemoryCache::getOriginsWithCache(SecurityOriginSet& origins)
{
#if ENABLE(CACHE_PARTITIONING)
    static NeverDestroyed<String> httpString("http");
#endif
    for (auto& resources : m_sessionResources.values()) {
        for (auto& keyValue : *resources) {
            auto& resource = *keyValue.value;
#if ENABLE(CACHE_PARTITIONING)
            auto& partitionName = keyValue.key.second;
            if (!partitionName.isEmpty())
                origins.add(SecurityOrigin::create(httpString, partitionName, 0));
            else
#endif
            origins.add(SecurityOrigin::create(resource.url()));
        }
    }
}

HashSet<RefPtr<SecurityOrigin>> MemoryCache::originsWithCache(SessionID sessionID) const
{
    HashSet<RefPtr<SecurityOrigin>> origins;

    auto it = m_sessionResources.find(sessionID);
    if (it != m_sessionResources.end()) {
        for (auto& keyValue : *it->value) {
            auto& resource = *keyValue.value;
#if ENABLE(CACHE_PARTITIONING)
            auto& partitionName = keyValue.key.second;
            if (!partitionName.isEmpty())
                origins.add(SecurityOrigin::create("http", partitionName, 0));
            else
#endif
            origins.add(SecurityOrigin::create(resource.url()));
        }
    }

    return origins;
}

void MemoryCache::removeFromLiveDecodedResourcesList(CachedResource& resource)
{
    m_liveDecodedResources.remove(&resource);
}

void MemoryCache::insertInLiveDecodedResourcesList(CachedResource& resource)
{
    // Make sure we aren't in the list already.
    ASSERT(!m_liveDecodedResources.contains(&resource));
    m_liveDecodedResources.add(&resource);
}

void MemoryCache::addToLiveResourcesSize(CachedResource& resource)
{
    m_liveSize += resource.size();
    m_deadSize -= resource.size();
}

void MemoryCache::removeFromLiveResourcesSize(CachedResource& resource)
{
    m_liveSize -= resource.size();
    m_deadSize += resource.size();
}

void MemoryCache::adjustSize(bool live, long long delta)
{
    if (live) {
        ASSERT(delta >= 0 || (static_cast<long long>(m_liveSize) + delta >= 0));
        m_liveSize += delta;
    } else {
        ASSERT(delta >= 0 || (static_cast<long long>(m_deadSize) + delta >= 0));
        m_deadSize += delta;
    }
}

void MemoryCache::removeRequestFromSessionCaches(ScriptExecutionContext& context, const ResourceRequest& request)
{
    if (is<WorkerGlobalScope>(context)) {
        CrossThreadResourceRequestData* requestData = request.copyData().release();
#if !PLATFORM(WKC)
        downcast<WorkerGlobalScope>(context).thread().workerLoaderProxy().postTaskToLoader([requestData] (ScriptExecutionContext& context) {
            auto request(ResourceRequest::adopt(std::unique_ptr<CrossThreadResourceRequestData>(requestData)));
            MemoryCache::removeRequestFromSessionCaches(context, *request);
        });
#else
        std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [requestData](ScriptExecutionContext& context) {
            auto request(ResourceRequest::adopt(std::unique_ptr<CrossThreadResourceRequestData>(requestData)));
            MemoryCache::removeRequestFromSessionCaches(context, *request);
        });
        downcast<WorkerGlobalScope>(context).thread().workerLoaderProxy().postTaskToLoader(p);
#endif
        return;
    }

    auto& memoryCache = MemoryCache::singleton();
    for (auto& resources : memoryCache.m_sessionResources) {
        if (CachedResource* resource = memoryCache.resourceForRequestImpl(request, *resources.value))
            memoryCache.remove(*resource);
    }
}

void MemoryCache::TypeStatistic::addResource(CachedResource& resource)
{
    count++;
    size += resource.size();
    liveSize += resource.hasClients() ? resource.size() : 0;
    decodedSize += resource.decodedSize();
}

MemoryCache::Statistics MemoryCache::getStatistics()
{
    Statistics stats;

    for (auto& resources : m_sessionResources.values()) {
        for (auto* resource : resources->values()) {
            switch (resource->type()) {
            case CachedResource::ImageResource:
                stats.images.addResource(*resource);
                break;
            case CachedResource::CSSStyleSheet:
                stats.cssStyleSheets.addResource(*resource);
                break;
            case CachedResource::Script:
                stats.scripts.addResource(*resource);
                break;
#if ENABLE(XSLT)
            case CachedResource::XSLStyleSheet:
                stats.xslStyleSheets.addResource(*resource);
                break;
#endif
#if ENABLE(SVG_FONTS)
            case CachedResource::SVGFontResource:
#endif
            case CachedResource::FontResource:
                stats.fonts.addResource(*resource);
                break;
            default:
                break;
            }
        }
    }
    return stats;
}

void MemoryCache::setDisabled(bool disabled)
{
    m_disabled = disabled;
    if (!m_disabled)
        return;

    while (!m_sessionResources.isEmpty()) {
        auto& resources = *m_sessionResources.begin()->value;
        ASSERT(!resources.isEmpty());
        remove(*resources.begin()->value);
    }
}

void MemoryCache::evictResources()
{
    if (disabled())
        return;

    setDisabled(true);
    setDisabled(false);
}

void MemoryCache::evictResources(SessionID sessionID)
{
    if (disabled())
        return;

    auto it = m_sessionResources.find(sessionID);
    if (it == m_sessionResources.end())
        return;
    auto& resources = *it->value;

    for (int i = 0, size = resources.size(); i < size; ++i)
        remove(*resources.begin()->value);

    ASSERT(!m_sessionResources.contains(sessionID));
}

bool MemoryCache::needsPruning() const
{
    return m_liveSize + m_deadSize > m_capacity || m_deadSize > m_maxDeadCapacity;
}

void MemoryCache::prune()
{
    if (!needsPruning())
        return;
        
    pruneDeadResources(); // Prune dead first, in case it was "borrowing" capacity from live.
    pruneLiveResources();
}

void MemoryCache::pruneTimerFired()
{
    prune();
}

void MemoryCache::pruneSoon()
{
     if (m_pruneTimer.isActive())
        return;
     if (!needsPruning())
         return;
     m_pruneTimer.startOneShot(0);
}

#ifndef NDEBUG
void MemoryCache::dumpStats()
{
    Statistics s = getStatistics();
    printf("%-13s %-13s %-13s %-13s %-13s\n", "", "Count", "Size", "LiveSize", "DecodedSize");
    printf("%-13s %-13s %-13s %-13s %-13s\n", "-------------", "-------------", "-------------", "-------------", "-------------");
    printf("%-13s %13d %13d %13d %13d\n", "Images", s.images.count, s.images.size, s.images.liveSize, s.images.decodedSize);
    printf("%-13s %13d %13d %13d %13d\n", "CSS", s.cssStyleSheets.count, s.cssStyleSheets.size, s.cssStyleSheets.liveSize, s.cssStyleSheets.decodedSize);
#if ENABLE(XSLT)
    printf("%-13s %13d %13d %13d %13d\n", "XSL", s.xslStyleSheets.count, s.xslStyleSheets.size, s.xslStyleSheets.liveSize, s.xslStyleSheets.decodedSize);
#endif
    printf("%-13s %13d %13d %13d %13d\n", "JavaScript", s.scripts.count, s.scripts.size, s.scripts.liveSize, s.scripts.decodedSize);
    printf("%-13s %13d %13d %13d %13d\n", "Fonts", s.fonts.count, s.fonts.size, s.fonts.liveSize, s.fonts.decodedSize);
    printf("%-13s %-13s %-13s %-13s %-13s\n\n", "-------------", "-------------", "-------------", "-------------", "-------------");
}

void MemoryCache::dumpLRULists(bool includeLive) const
{
    printf("LRU-SP lists in eviction order (Kilobytes decoded, Kilobytes encoded, Access count, Referenced):\n");

    int size = m_allResources.size();
    for (int i = size - 1; i >= 0; i--) {
        printf("\n\nList %d: ", i);
        for (auto* resource : *m_allResources[i]) {
            if (includeLive || !resource->hasClients())
                printf("(%.1fK, %.1fK, %uA, %dR); ", resource->decodedSize() / 1024.0f, (resource->encodedSize() + resource->overheadSize()) / 1024.0f, resource->accessCount(), resource->hasClients());
        }
    }
}
#endif

#if PLATFORM(WKC)
void
MemoryCache::setMinDelayBeforeLiveDecodedPrune(double second)
{
    gMinDelayBeforeLiveDecodedPrune = second;
}
#endif

} // namespace WebCore

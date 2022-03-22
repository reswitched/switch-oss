/*
 *  Copyright (c) 2011-2021 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(WKC_HTTPCACHE)

#include "HTTPCacheWKC.h"
#include "SharedBuffer.h"
#include "URL.h"
#include "CString.h"
#include "FileSystem.h"
#include "ResourceResponse.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternalWKC.h"
#include <wtf/MathExtras.h>
#include <wtf/MD5.h>
#include <wkc/wkcclib.h>
#include <wkc/wkcpeer.h>
#include <wkc/wkcfilepeer.h>
#include <wkc/wkcmpeer.h>

#include "LoggingWKC.h"
#include "WKCEnums.h"

#ifdef __WKC_IMPLICIT_INCLUDE_SYSSTAT
#include <sys/stat.h>
#endif

using namespace std;

namespace WebCore {

using namespace FileSystem;

// The size of a cache file is calculated as rounded up to a multiple of ContentSizeAlignment.
static constexpr long long ContentSizeAlignment = 16 * 1024;

static long long
roundUpToMultipleOf(long long x, long long powerOf2)
{
    return (x + powerOf2 - 1) & ~(powerOf2 - 1);
}

// HTTPCacheResource

HTTPCachedResource::HTTPCachedResource(const String& filename, const String& url)
    : m_alignedContentLength(0)
    , m_resourceData(nullptr)
    , m_resourceSize(0)
    , m_serverpushed(false)
    , m_used(false)
    // Persistent Data
    , m_expectedContentLength(0)
    , m_contentLength(0)
    , m_httpStatusCode(0)
    , m_hasVaryingRequestHeaders(false)
    , m_noCache(false)
    , m_mustRevalidate(false)
    , m_isSSL(false)
    , m_isEVSSL(false)
    , m_secureState(WKC::ESecureStateWhite)
    , m_secureLevel(WKC::ESecureLevelNonSSL)
    , m_expires(0)
    , m_maxAge(0)
    , m_date(0)
    , m_lastModified(0)
    , m_responseTime(0)
    , m_age(0)
    , m_lastUsedTime(MonotonicTime::now().secondsSinceEpoch().value())
    , m_url(url)
    , m_mimeType()
    , m_textEncodingName()
    , m_suggestedFilename()
    , m_fileName(filename)
    , m_httpStatusText()
    , m_lastModifiedHeader()
    , m_eTagHeader()
    , m_accessControlAllowOriginHeader()
    , m_contentTypeHeader()
    , m_acceptLanguage()
{
}

HTTPCachedResource::~HTTPCachedResource()
{
    if (m_resourceData) {
        m_resourceData->deref();
        m_resourceData = nullptr;
    }
}

// copy from computeFreshnessLifetimeForHTTPFamily()
double HTTPCachedResource::freshnessLifetime()
{
    // Freshness Lifetime:
    // http://tools.ietf.org/html/rfc7234#section-4.2.1
    if (isfinite(m_maxAge))
        return m_maxAge;

    double dateValue = isfinite(m_date) ? m_date : m_responseTime;

    if (isfinite(m_expires))
        return m_expires - dateValue;

    switch (m_httpStatusCode) {
    case 301: // Moved Permanently
    case 410: // Gone
        // These are semantically permanent and so get long implicit lifetime.
        return 365 * 24 * 60 * 60;
    default:
        // Heuristic Freshness:
        // http://tools.ietf.org/html/rfc7234#section-4.2.2
        if (isfinite(m_lastModified))
            return (dateValue - m_lastModified) * 0.1;
    }

    // If no cache headers are present, the specification leaves the decision to the UA. Other browsers seem to opt for 0.
    return 0;
}

// from CacheValidation.cpp: computeCurrentAge()
double HTTPCachedResource::computeCurrentAge()
{
    // Age calculation:
    // http://tools.ietf.org/html/rfc7234#section-4.2.3
    // No compensation for latency as that is not terribly important in practice.

    double apparentAge = isfinite(m_date) ? std::max(0.0, m_responseTime - m_date) : 0;
    double ageValue = isfinite(m_age) ? m_age : 0;
    double correctedInitialAge = std::max(apparentAge, ageValue);
    double residentTime = MonotonicTime::now().secondsSinceEpoch().value() - m_responseTime;
    return correctedInitialAge + residentTime;
}

bool HTTPCachedResource::isExpired()
{
    if (m_responseTime==0)
        return false;
    double diff = computeCurrentAge();
    return (diff > freshnessLifetime());
}

bool HTTPCachedResource::needRevalidate()
{
    bool expired = isExpired();
    return m_noCache || expired;
}

bool HTTPCachedResource::setResourceResponse(const ResourceResponse &response, Vector<std::pair<String, String>> varyingRequestHeaders, bool serverpush)
{
    m_mimeType = response.mimeType();
    m_expectedContentLength = response.expectedContentLength();
    m_textEncodingName = response.textEncodingName();
    m_suggestedFilename = response.suggestedFilename();
    m_httpStatusCode = response.httpStatusCode();
    m_httpStatusText = response.httpStatusText();
    m_responseTime = MonotonicTime::now().secondsSinceEpoch().value();
    m_date = response.date() ? (double)response.date().value().secondsSinceEpoch().value() : numeric_limits<double>::quiet_NaN();
    m_lastModified = response.lastModified() ? (double)response.lastModified().value().secondsSinceEpoch().value() : numeric_limits<double>::quiet_NaN();
    m_lastModifiedHeader = response.httpHeaderField(String("Last-Modified"));
    m_eTagHeader = response.httpHeaderField(String("ETag"));
    m_age = response.age() ? (double)response.age().value().value() : numeric_limits<double>::quiet_NaN();
    m_accessControlAllowOriginHeader = response.httpHeaderField(String("Access-Control-Allow-Origin"));
    m_contentTypeHeader = response.httpHeaderField(HTTPHeaderName::ContentType);
    m_serverpushed = serverpush;

    ResourceHandle* job = response.resourceHandle();
    ResourceHandleInternal* d = job->getInternal();
    m_isSSL = d->m_isSSL;
    m_isEVSSL = d->m_isEVSSL;
    m_secureState = d->m_secureState;
    m_secureLevel = d->m_secureLevel;

    m_noCache = response.cacheControlContainsNoCache();
    m_mustRevalidate = response.cacheControlContainsMustRevalidate();
    m_maxAge = response.cacheControlMaxAge() ? (double)response.cacheControlMaxAge().value().value() : numeric_limits<double>::quiet_NaN();
    m_expires = response.expires() ? (double)response.expires().value().secondsSinceEpoch().value() : numeric_limits<double>::quiet_NaN();

    // Vary header only supports Accept-Language for now.
    m_hasVaryingRequestHeaders = false;
    m_acceptLanguage = String();
    for (auto& varyingRequestHeader : varyingRequestHeaders) {
        if (equalIgnoringASCIICase(varyingRequestHeader.first, "Accept-Language")) {
            m_hasVaryingRequestHeaders = true;
            m_acceptLanguage = varyingRequestHeader.second;
            break;
        }
    }

    return true;
}

bool HTTPCachedResource::writeFile(const String& filepath)
{
    void *fd = 0;
    bool result = false;

    if (!m_resourceData)
        return false;

    String fileFullPath = pathByAppendingComponent(filepath, m_fileName);

    fd = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_CACHE, fileFullPath.utf8().data(), "wb");
    if (!fd)
        return result;

    wkc_uint64 len = 0, length = 0, written = 0, position = 0;

    for (len = m_resourceData->size(); len > 0;  len -= length) {
        auto data = m_resourceData->getSomeData(position);
        length = data.size();
        if (!length)
            goto error_end;

        written = wkcFileFWritePeer(data.data(), sizeof(char), length, fd);
        if (written < length)
            goto error_end;
        position += length;
    }
    LOG(HTTPCache, "cache write %s", m_url.utf8().data());

    m_resourceData->deref();
    m_resourceData = nullptr;

    result = true;
error_end:
    if (fd)
        wkcFileFClosePeer(fd);

    return result;
}

bool HTTPCachedResource::readFile(char *out_buf, const String& filepath)
{
    char *buf = out_buf;
    if (m_resourceData) {
        wkc_uint64 len = 0, length = 0, position = 0;
        for (len = m_contentLength; len > 0;  len -= length) {
            auto data = m_resourceData->getSomeData(position);
            length = data.size();
            if (!length)
                return false;

            memcpy(buf, data.data(), length);
            position += length;
            buf += length;
        }
        if (position < m_contentLength)
            return false;
        return true;
    }

    void *fd = 0;
    bool result = false;

    String fileFullPath = pathByAppendingComponent(filepath, m_fileName);

    fd = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_CACHE, fileFullPath.utf8().data(), "rb");
    if (!fd)
        goto error_end;

    int len, read;
    for (len = m_contentLength; len > 0; len -= read) {
        read = wkcFileFReadPeer(buf, sizeof(char), len, fd);
        if (read < 0)
            break;
        buf += read; 
    }
    LOG(HTTPCache, "cache read %s", m_url.utf8().data());

    result = true;
error_end:
    if (fd)
        wkcFileFClosePeer(fd);

    m_used = false;

    return result;
}

#define ROUNDUP(x, y)   (((x)+((y)-1))/(y)*(y))
#define ROUNDUP_UNIT    4

void HTTPCachedResource::calcResourceSize()
{
    size_t size = sizeof(m_expectedContentLength) // long long
        + sizeof(m_contentLength)  // long long
        + sizeof(m_httpStatusCode) // int
        + sizeof(int)              // bool m_hasVaryingRequestHeaders;
        + sizeof(int)              // bool m_noCache;
        + sizeof(int)              // bool m_mustRevalidate;
        + sizeof(int)              // bool m_isSSL;
        + sizeof(int)              // bool m_isEVSSL;
        + sizeof(m_secureState)    // int
        + sizeof(m_secureLevel)    // int
        + sizeof(m_expires)        // double
        + sizeof(m_maxAge)         // double
        + sizeof(m_date)           // double
        + sizeof(m_lastModified)   // double
        + sizeof(m_responseTime)   // double
        + sizeof(m_age)            // double
        + sizeof(m_lastUsedTime)   // double
        + sizeof(int) + ROUNDUP(m_url.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_mimeType.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_textEncodingName.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_suggestedFilename.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_fileName.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_httpStatusText.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_lastModifiedHeader.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_eTagHeader.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_accessControlAllowOriginHeader.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_contentTypeHeader.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_acceptLanguage.utf8().length(), ROUNDUP_UNIT);

    m_resourceSize = ROUNDUP(size, ROUNDUP_UNIT);
}

int writeString(char *buffer, const String &str)
{
    CString cstr = str.utf8();
    int length = cstr.length();

    (*(int*)buffer) = length; buffer += sizeof(int);

    if (length>0)
        memcpy(buffer, cstr.data(), length);
    return ROUNDUP(length + sizeof(int), ROUNDUP_UNIT);
}

int HTTPCachedResource::serialize(char *buffer)
{
    char *buf = buffer;

    // number field
    (*(long long*)buffer) = m_expectedContentLength; buffer += sizeof(long long);
    (*(long long*)buffer) = m_contentLength; buffer += sizeof(long long);
    (*(int*)buffer) = m_httpStatusCode; buffer += sizeof(int);
    (*(int*)buffer) = m_hasVaryingRequestHeaders ? 1 : 0; buffer += sizeof(int);
    (*(int*)buffer) = m_noCache ? 1 : 0; buffer += sizeof(int);
    (*(int*)buffer) = m_mustRevalidate ? 1 : 0; buffer += sizeof(int);
    (*(int*)buffer) = m_isSSL ? 1 : 0; buffer += sizeof(int);
    (*(int*)buffer) = m_isEVSSL ? 1 : 0; buffer += sizeof(int);
    (*(int*)buffer) = m_secureState; buffer += sizeof(int);
    (*(int*)buffer) = m_secureLevel; buffer += sizeof(int);
    (*(double*)buffer) = m_expires; buffer += sizeof(double);
    (*(double*)buffer) = m_maxAge; buffer += sizeof(double);
    (*(double*)buffer) = m_date; buffer += sizeof(double);
    (*(double*)buffer) = m_lastModified; buffer += sizeof(double);
    (*(double*)buffer) = m_responseTime; buffer += sizeof(double);
    (*(double*)buffer) = m_age; buffer += sizeof(double);
    (*(double*)buffer) = m_lastUsedTime; buffer += sizeof(double);
    // string field
    buffer += writeString(buffer, m_url);
    buffer += writeString(buffer, m_mimeType);
    buffer += writeString(buffer, m_textEncodingName);
    buffer += writeString(buffer, m_suggestedFilename);
    buffer += writeString(buffer, m_fileName);
    buffer += writeString(buffer, m_httpStatusText);
    buffer += writeString(buffer, m_lastModifiedHeader);
    buffer += writeString(buffer, m_eTagHeader);
    buffer += writeString(buffer, m_accessControlAllowOriginHeader);
    buffer += writeString(buffer, m_contentTypeHeader);
    buffer += writeString(buffer, m_acceptLanguage);

    return ROUNDUP(buffer - buf, ROUNDUP_UNIT);
}

int readString(char *buffer, String &str)
{
    int length;
    
    length= (*(int*)buffer); buffer += sizeof(int);
    str = String::fromUTF8(buffer, length);

    return ROUNDUP(length + sizeof(int), ROUNDUP_UNIT);
}

int HTTPCachedResource::deserialize(char *buffer)
{
    char *buf = buffer;

    // number field
    m_expectedContentLength = (*(long long*)buffer); buffer += sizeof(long long);
    m_contentLength = (*(long long*)buffer); buffer += sizeof(long long);
    m_alignedContentLength = roundUpToMultipleOf(m_contentLength, ContentSizeAlignment);
    m_httpStatusCode = (*(int*)buffer); buffer += sizeof(int);
    m_hasVaryingRequestHeaders = (*(int*)buffer); buffer += sizeof(int);
    m_noCache = (*(int*)buffer); buffer += sizeof(int);
    m_mustRevalidate = (*(int*)buffer); buffer += sizeof(int);
    m_isSSL = (*(int*)buffer); buffer += sizeof(int);
    m_isEVSSL = (*(int*)buffer); buffer += sizeof(int);
    m_secureState = (*(int*)buffer); buffer += sizeof(int);
    m_secureLevel = (*(int*)buffer); buffer += sizeof(int);
    m_expires = (*(double*)buffer); buffer += sizeof(double);
    m_maxAge = (*(double*)buffer); buffer += sizeof(double);
    m_date = (*(double*)buffer); buffer += sizeof(double);
    m_lastModified = (*(double*)buffer); buffer += sizeof(double);
    m_responseTime = (*(double*)buffer); buffer += sizeof(double);
    m_age = (*(double*)buffer); buffer += sizeof(double);
    m_lastUsedTime = (*(double*)buffer); buffer += sizeof(double);
    // string field
    buffer += readString(buffer, m_url);
    buffer += readString(buffer, m_mimeType);
    buffer += readString(buffer, m_textEncodingName);
    buffer += readString(buffer, m_suggestedFilename);
    buffer += readString(buffer, m_fileName);
    buffer += readString(buffer, m_httpStatusText);
    buffer += readString(buffer, m_lastModifiedHeader);
    buffer += readString(buffer, m_eTagHeader);
    buffer += readString(buffer, m_accessControlAllowOriginHeader);
    buffer += readString(buffer, m_contentTypeHeader);
    buffer += readString(buffer, m_acceptLanguage);

    calcResourceSize();

    return ROUNDUP(buffer - buf, ROUNDUP_UNIT);
}

void HTTPCachedResource::setResourceData(SharedBuffer* resourceData)
{
    if (!resourceData) {
        m_contentLength = 0;
        m_alignedContentLength = 0;
    } else {
        resourceData->ref();
        m_resourceData = resourceData;
        m_contentLength = resourceData->size();
        m_alignedContentLength = roundUpToMultipleOf(m_contentLength, ContentSizeAlignment);
    }
}

bool HTTPCachedResource::varyHeaderValuesMatch(const ResourceRequest& request)
{
    if (!m_hasVaryingRequestHeaders)
        return true;

    // Vary header only supports Accept-Language for now.
    if (equalIgnoringASCIICase(request.httpHeaderField(String("Accept-Language")), m_acceptLanguage))
        return true;

    return false;
}

// HTTPCache

#define DEFAULT_FAT_FILENAME        "cache.fat"
#define DEFAULT_FILEPATH            "cache/"
#define DEFAULT_CONTENTENTRIES_LIMIT        (1024)
#define DEFAULT_CONTENTENTRYINFOSIZE_LIMIT  (4 * 1024)
#define DEFAULT_CONTENTSIZE_LIMIT           (10 * 1024 * 1024)
#define DEFAULT_TOTALCONTENTSSIZE_LIMIT     (10 * 1024 * 1024)

static WKC::CanCacheToDiskProc gCanCacheToDiskCallback;

void
HTTPCache::setCanCacheToDiskCallback(WKC::CanCacheToDiskProc proc)
{
    gCanCacheToDiskCallback = proc;
}

HTTPCache::HTTPCache()
    : m_fatFileName(DEFAULT_FAT_FILENAME)
    , m_filePath(DEFAULT_FILEPATH)
{
    m_disabled = false;
    m_fileNumber = 0;
    m_totalResourceSize = 0;

    m_totalContentsSize = 0;
    m_maxContentEntries = DEFAULT_CONTENTENTRIES_LIMIT;
    m_maxContentEntryInfoSize = DEFAULT_CONTENTENTRYINFOSIZE_LIMIT;
    m_maxContentSize = DEFAULT_CONTENTSIZE_LIMIT;
    m_minContentSize = 0;
    m_maxTotalContentsSize = DEFAULT_TOTALCONTENTSSIZE_LIMIT;

    m_resources.clear();
    m_resourceList.clear();
}

HTTPCache::~HTTPCache()
{
    reset();
}

void HTTPCache::reset()
{
    HTTPCachedResource *resource;

    for (size_t num = 0; num < m_resourceList.size(); num++) {
        resource = m_resourceList[num];
        delete resource;
    }
    m_resourceList.clear();
    m_resources.clear();

    m_totalResourceSize = 0;
    m_totalContentsSize = 0;
}

void
HTTPCache::appendResourceInSizeOrder(Vector<HTTPCachedResource*>& resourceList, HTTPCachedResource* resource)
{
    size_t len = resourceList.size();
    for (size_t i = 0; i < len; i++) {
        if (resource->contentLength() >= resourceList[i]->contentLength()) {
            resourceList.insert(i, resource);
            return;
        }
    }
    resourceList.append(resource);
}

void
HTTPCache::appendResourceInLastUsedTimeOrder(Vector<HTTPCachedResource*>& resourceList, HTTPCachedResource* resource)
{
    size_t len = resourceList.size();
    for (int i = len - 1; i >= 0; i--) {
        if (resource->lastUsedTime() >= resourceList[i]->lastUsedTime()) {
            resourceList.insert(i + 1, resource);
            return;
        }
    }
    resourceList.insert(0, resource);
}

HTTPCachedResource* HTTPCache::createHTTPCachedResource(URL &url, SharedBuffer* resourceData, ResourceRequest& request, ResourceResponse &response, Vector<std::pair<String, String>> varyingRequestHeaders, bool serverpush)
{
    if (disabled())
        return 0;

    long long originalContentLength = resourceData->size();
    long long alignedContentLength = roundUpToMultipleOf(originalContentLength, ContentSizeAlignment);

    if (m_maxContentSize < alignedContentLength)
        return 0;   // size over
    if (m_minContentSize > originalContentLength)
        return 0;   // too small to cache
    if (m_maxTotalContentsSize < alignedContentLength)
        return 0;   // size over
    if (m_resourceList.size() >= m_maxContentEntries)
        if (purgeOldest()==0)
            return 0; // entry limit over
    // Checking the total size and calling to purgeBySize must be done after calling gCanCacheToDiskCallback.

    const URL kurl = removeFragmentIdentifierIfNeeded(url);
    HTTPCachedResource *resource = new HTTPCachedResource(makeFileName(), kurl.string());
    if (!resource)
        return 0;

    updateCachedResource(resource, resourceData, request, response, varyingRequestHeaders, serverpush);
    return resource;
}

bool HTTPCache::addCachedResource(HTTPCachedResource *resource)
{
    if (disabled())
        return false;

    if (resource->resourceSize() > m_maxContentEntryInfoSize) {
        return false; // entry info size over
    }

    if (m_resourceList.size() >= m_maxContentEntries)
        if (purgeOldest()==0)
            return false; // entry limit over

    m_resources.set(resource->url(), resource);
    m_resourceList.append(resource);
    m_totalResourceSize += resource->resourceSize();
    m_totalContentsSize += resource->alignedContentLength();

    return true;
}

void HTTPCache::updateResourceLastUsedTime(HTTPCachedResource *resource)
{
    resource->setLastUsedTime(MonotonicTime::now().secondsSinceEpoch().value());

    if (!m_resources.get(resource->url())) {
        return;
    }

    for (size_t i = 0; i < m_resourceList.size(); i++) {
        if (m_resourceList[i] == resource) {
            m_resourceList.remove(i);
            appendResourceInLastUsedTimeOrder(m_resourceList, resource);
            return;
        }
    }
}

void HTTPCache::updateCachedResource(HTTPCachedResource *resource, SharedBuffer* resourceData, ResourceRequest& request, ResourceResponse &response, Vector<std::pair<String, String>> varyingRequestHeaders, bool serverpush)
{
    resource->setResourceResponse(response, varyingRequestHeaders, serverpush);
    resource->setResourceData(resourceData);
    updateResourceLastUsedTime(resource);
}

void HTTPCache::removeResource(HTTPCachedResource *resource)
{
    m_resources.remove(resource->url());
    for (size_t num = 0; num < m_resourceList.size(); num++) {
        if (m_resourceList[num] == resource) {
            m_resourceList.remove(num);
            break;
        }
    }
    m_totalResourceSize -= resource->resourceSize();
    m_totalContentsSize -= resource->alignedContentLength();
}

void HTTPCache::removeResourceByNumber(int listNumber)
{
    HTTPCachedResource *resource = m_resourceList[listNumber];

    LOG(HTTPCache, "remove cache file %s", resource->fileName().utf8().data());

    m_resources.remove(resource->url());
    m_resourceList.remove(listNumber);
    m_totalResourceSize -= resource->resourceSize();
    m_totalContentsSize -= resource->alignedContentLength();

    const String fileFullPath = pathByAppendingComponent(m_filePath, resource->fileName());
    wkcFileUnlinkPeer(fileFullPath.utf8().data());

    delete resource;
}

void HTTPCache::remove(HTTPCachedResource *resource)
{
    removeResource(resource);

    LOG(HTTPCache, "remove cache file %s", resource->fileName().utf8().data());

    const String fileFullPath = pathByAppendingComponent(m_filePath, resource->fileName());
    wkcFileUnlinkPeer(fileFullPath.utf8().data());

    delete resource;
}

void HTTPCache::detach(HTTPCachedResource *resource)
{
    removeResource(resource);

    if (resource->fileName().isEmpty())
        return;
    const String fileFullPath = pathByAppendingComponent(m_filePath, resource->fileName());
    wkcFileUnlinkPeer(fileFullPath.utf8().data());
}

void HTTPCache::removeAll()
{
    reset();
    m_fileNumber = 0;

    Vector<String> items(listDirectory(m_filePath.utf8().data(), "*.dcf"));

    size_t count = items.size();
    for (size_t i=0; i<count; i++) {
        String fileFullPath = pathByAppendingComponent(m_filePath, items[i]);
        if (fileFullPath.length() > 0)
            wkcFileUnlinkPeer(fileFullPath.utf8().data());
    }

    String fileFullPath = pathByAppendingComponent(m_filePath, m_fatFileName);
    if (fileFullPath.length()>0)
        wkcFileUnlinkPeer(fileFullPath.utf8().data());
}

void HTTPCache::sanitizeCaches()
{
    Vector<String> items(listDirectory(m_filePath.utf8().data(), "*.dcf"));
    Vector<String> indir;
    Vector<String> infat;

    for (size_t i = 0; i<items.size(); i++)
        indir.append(pathByAppendingComponent(m_filePath, items[i]));
    for (size_t i = 0; i<m_resourceList.size(); i++)
        infat.append(pathByAppendingComponent(m_filePath, m_resourceList[i]->fileName()));

    for (auto it = indir.begin(); it != indir.end(); ++it) {
        String& item = *it;
        if (item.isEmpty())
            continue;
        if (item == m_filePath)
            continue;
        size_t pos = infat.find(item);
        if (pos==notFound) {
            wkcFileUnlinkPeer(item.utf8().data());
            item = String();
        }
    }

    for (int i=infat.size()-1; i>=0; i--) {
        size_t pos = indir.find(infat[i]);
        if (pos == notFound)
            removeResource(m_resourceList[i]);
    }
}

void HTTPCache::setDisabled(bool disabled)
{
    m_disabled = disabled;
    if (!disabled)
        return;
}

void HTTPCache::setMaxContentEntries(int limit)
{
    if (limit<=0)
        limit = 0x7fffffff; // INT_MAX
    m_maxContentEntries = limit;
}

void HTTPCache::setMaxContentEntryInfoSize(int limit)
{
    if (limit<=0)
        limit = 0x7fffffff; // INT_MAX
    m_maxContentEntryInfoSize = limit;
}

void HTTPCache::setMaxContentSize(long long limit)
{
    if (limit<=0)
        limit = 0x7fffffffffffffffLL; // LONG_LONG_MAX
    m_maxContentSize = limit;
}

void HTTPCache::setMinContentSize(long long limit)
{
    if (limit<0)
        limit = 0;
    m_minContentSize = limit;
}

void HTTPCache::setMaxTotalCacheSize(long long limit)
{
    if (limit<=0)
        limit = 0x7fffffffffffffffLL; // LONG_LONG_MAX
    m_maxTotalContentsSize = limit;

    if (limit < m_totalContentsSize) {
        purgeBySize(m_totalContentsSize - limit);
        writeFATFile();
    }
}

void HTTPCache::setFilePath(const char* path)
{
    if (!path || !strlen(path))
        return;

    // TODO: If path is same as the default one, readFATFile will never be called even if HTTPCache is enabled...
    if (m_filePath == path)
        return;

    m_filePath = String::fromUTF8(path);

    // ensure the destination path is exist
    (void)makeAllDirectories(m_filePath);

    readFATFile();
}

WTF::URL HTTPCache::removeFragmentIdentifierIfNeeded(const WTF::URL& originalURL)
{
    if (!originalURL.hasFragmentIdentifier())
        return originalURL;
    // Strip away fragment identifier from HTTP URLs.
    // Data URLs must be unmodified. For file and custom URLs clients may expect resources 
    // to be unique even when they differ by the fragment identifier only.
    if (!originalURL.protocolIsInHTTPFamily())
        return originalURL;
    WTF::URL url = originalURL;
    url.removeFragmentIdentifier();
    return url;
}

HTTPCachedResource* HTTPCache::resourceForURL(const WTF::URL& resourceURL)
{
    WTF::URL url = removeFragmentIdentifierIfNeeded(resourceURL);
    HTTPCachedResource* resource = m_resources.get(url);

    return resource;
}

bool HTTPCache::equalHTTPCachedResourceURL(HTTPCachedResource *resource, WTF::URL& resourceURL)
{
    WTF::URL url = removeFragmentIdentifierIfNeeded(resourceURL);
    if (equalIgnoringASCIICase(resource->url(), url.string()))
        return true;

    return false;
}

String HTTPCache::makeFileName()
{
    String fileName;

    fileName = makeString(pad('0', 8, m_fileNumber), ".dcf");
    if (m_fileNumber==WKC_INT_MAX)
        m_fileNumber = 0;
    else
        m_fileNumber++;

    return fileName;
}

long long HTTPCache::purgeBySizeInternal(long long size)
{
    HTTPCachedResource *resource;
    long long purgedSize = 0;

    for (size_t num = 0; num < m_resourceList.size(); ) {
        resource = m_resourceList[num];
        if (resource->isUsed())
            num++;
        else {
            purgedSize += resource->alignedContentLength();
            removeResourceByNumber(num);
            if (purgedSize >= size)
                break;
        }
    }
    return purgedSize;
}

long long HTTPCache::purgeBySize(long long size)
{
    long long purgedSize = purgeBySizeInternal(size);
    if (purgedSize < size) {
        for (size_t num = 0; num < m_resourceList.size(); num++)
            m_resourceList[num]->setUsed(false);
        purgedSize +=purgeBySizeInternal(size - purgedSize);
    }
    return purgedSize;
}

long long HTTPCache::purgeOldest()
{
    HTTPCachedResource *resource;
    long long purgedSize = 0;

    for (size_t num = 0; num < m_resourceList.size(); ) {
        resource = m_resourceList[num];
        if (resource->isUsed())
            num++;
        else {
            purgedSize += resource->alignedContentLength();
            removeResourceByNumber(num);
            break;
        }
    }
    return purgedSize;
}

bool HTTPCache::write(HTTPCachedResource *resource)
{
    resource->calcResourceSize();

    if (resource->resourceSize() > m_maxContentEntryInfoSize) {
        return false; // entry info size over
    }

    if (m_resourceList.size() >= m_maxContentEntries)
        if (purgeOldest()==0)
            return false; // entry limit over

    long long alignedContentLength = resource->alignedContentLength();

    if (gCanCacheToDiskCallback && !gCanCacheToDiskCallback(resource->url().utf8().data(),
                                                            resource->computeCurrentAge(),
                                                            resource->freshnessLifetime(),
                                                            resource->contentLength(),
                                                            alignedContentLength,
                                                            resource->mimeType().utf8().data()))
        return false;

    if (m_totalContentsSize + alignedContentLength > m_maxTotalContentsSize) {
        long long sizeToRemove = m_totalContentsSize + alignedContentLength - m_maxTotalContentsSize;
        if (purgeBySize(sizeToRemove) < sizeToRemove)
            return false; // exceeded the total size
    }

    if (!resource->writeFile(m_filePath))
        return false;
    
    m_resources.add(resource->url(), resource);
    appendResourceInLastUsedTimeOrder(m_resourceList, resource);
    m_totalResourceSize += resource->resourceSize();
    m_totalContentsSize += alignedContentLength;

    return true;
}

bool HTTPCache::read(HTTPCachedResource *resource, char *buf)
{
    bool ret = resource->readFile(buf, m_filePath);
    if (ret) {
        updateResourceLastUsedTime(resource);
    }
    return ret;
}

void HTTPCache::serializeFATData(char *buffer)
{
    for (size_t num = 0; num < m_resourceList.size(); num++)
        buffer += m_resourceList[num]->serialize(buffer);
}

bool HTTPCache::deserializeFATData(char *buffer, int length)
{
    int read = 0;
    HTTPCachedResource *resource = 0;

    for (; length > 0; length -= read) {
        resource = new HTTPCachedResource(makeFileName(), String());
        if (!resource)
            return false;

        read = resource->deserialize(buffer);
        buffer += read;
        addCachedResource(resource);
    }    

    sanitizeCaches();

    return true;
}

#define DEFAULT_CACHEFAT_FILENAME   "cache.fat"
#define CACHEFAT_FORMAT_VERSION     10  // Number of int. Increment this if you changed the content format in the fat file.

#define MD5_DIGESTSIZE 16

void HTTPCache::writeDigest(unsigned char *data, int length)
{
    MD5 md5;
    md5.addBytes(data + MD5_DIGESTSIZE, length - MD5_DIGESTSIZE);
    MD5::Digest digest;
    md5.checksum(digest);
    memcpy(data, digest.data(), MD5_DIGESTSIZE);
}

bool HTTPCache::writeFATFile()
{
    char *buf = 0, *buffer = 0;

    int totalSize = 0;
    int headerSize = 0;

    headerSize = sizeof(int) * 2 + MD5_DIGESTSIZE;
    totalSize = headerSize + m_totalResourceSize;

    WTF::TryMallocReturnValue rv = tryFastZeroedMalloc(totalSize);
    if (!rv.getValue(buf))
        return false;

    buffer = buf + MD5_DIGESTSIZE;
    (*(int*)buffer) = CACHEFAT_FORMAT_VERSION; buffer += sizeof(int);
    (*(int*)buffer) = m_fileNumber; buffer += sizeof(int);

    serializeFATData(buffer);

    writeDigest((unsigned char*)buf, totalSize);

    String fileFullPath = pathByAppendingComponent(m_filePath, m_fatFileName);

    void *fd = 0;
    int len, written = 0;
    fd = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_CACHE, fileFullPath.utf8().data(), "wb");
    if (!fd)
        goto error_end;

    buffer = buf;
    for (len = totalSize; len > 0;  len -= written) {
        written = wkcFileFWritePeer(buffer, sizeof(char), len, fd);
        if (written <= 0)
            break;
        buffer += written;
    }
    if (len > 0)
        goto error_end;

    wkcFileFClosePeer(fd);
    fastFree(buf);
    return true;

error_end:
    if (fd) {
        wkcFileFClosePeer(fd);
        wkcFileUnlinkPeer(fileFullPath.utf8().data());
    }

    fastFree(buf);
    return false;
}

bool HTTPCache::verifyDigest(unsigned char *data, int length)
{
    if (length <= MD5_DIGESTSIZE)
        return false;

    bool result = false;
    MD5 md5;
    md5.addBytes(data + MD5_DIGESTSIZE, length - MD5_DIGESTSIZE);
    MD5::Digest digest;
    md5.checksum(digest);

    if (memcmp(data, digest.data(), MD5_DIGESTSIZE) == 0)
        result = true;

    return result;
}

bool HTTPCache::readFATFile()
{
    void *fd = 0;
    bool result = false, remove = false;
    char *buf = 0, *buffer = 0;
    int format_version = 0;
    int len = 0;

    String fileFullPath = pathByAppendingComponent(m_filePath, m_fatFileName);

    fd = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_CACHE, fileFullPath.utf8().data(), "rb");
    if (!fd)
        return false;

    struct stat st;
    if (wkcFileFStatPeer(fd, &st)) {
        LOG(HTTPCache, "FAT file read error: failed to stat");
        wkcFileFClosePeer(fd);
        return false;
    }
    if (st.st_size <= MD5_DIGESTSIZE + sizeof(int)*2) {
        LOG(HTTPCache, "FAT file read error: size is too small");
        wkcFileFClosePeer(fd);
        return false;
    }

    WTF::TryMallocReturnValue rv = tryFastZeroedMalloc(st.st_size);
    if (!rv.getValue(buf)) {
        LOG(HTTPCache, "FAT file read error: no memory");
        goto error_end;
    }

    len = wkcFileFReadPeer(buf, sizeof(char), st.st_size, fd);
    wkcFileFClosePeer(fd);
    fd = 0;
    if (len<0 || len!=st.st_size) {
        LOG(HTTPCache, "FAT file read error: failed to read");
        goto error_end;
    }

    buffer = buf;

    if (!verifyDigest((unsigned char*)buffer, len)) {
        LOG(HTTPCache, "FAT file read error: MD5 check");
        remove = true;
        goto error_end;
    }

    buffer += MD5_DIGESTSIZE;

    format_version = (*(int*)buffer); buffer += sizeof(int);
    if (format_version != CACHEFAT_FORMAT_VERSION) {
        LOG(HTTPCache, "FAT file format is old : %d", format_version);
        remove = true;
        goto error_end;
    }

    m_fileNumber = (*(int*)buffer); buffer += sizeof(int);
    len -= sizeof(int) + sizeof(int) + MD5_DIGESTSIZE;

    if (len<=0) {
        LOG(HTTPCache, "FAT file read error: size is too small");
        remove = true;
        goto error_end;
    }

    if (deserializeFATData(buffer, len))
        result = true;
    else
        remove = true;

error_end:
    if (fd)
        wkcFileFClosePeer(fd);

    if (remove)
        removeAll();

    fastFree(buf);
    return result;
}

void
HTTPCache::dumpResourceList()
{
    int num = m_resourceList.size();

    if (num == 0) {
        WTFLogAlways("There are no HTTP cache files.");
        return;
    }

    WTFLogAlways("");
    for (int i = 0; i < num; i++){
        HTTPCachedResource *resource = m_resourceList[i];
        WTFLogAlways("[HTTP Cache %d]", i + 1);
        WTFLogAlways("  file: %s", resource->fileName().utf8().data());
        WTFLogAlways("  URL: %s", resource->url().utf8().data());
        WTFLogAlways("  last used time: %lf", resource->lastUsedTime());
        WTFLogAlways("  HTTP Status code: %d", resource->httpStatusCode());
        WTFLogAlways("  mime type: %s", resource->mimeType().utf8().data());
        WTFLogAlways("  original content length: %ld", resource->contentLength());
        WTFLogAlways("  aligned content length: %ld", resource->alignedContentLength());
        WTFLogAlways("  expires: %lf max-age: %lf", resource->expires(), resource->maxAge());
        WTFLogAlways("  Last-Modified: %s", resource->lastModifiedHeader().utf8().data());
        WTFLogAlways("  eTag: %s", resource->eTagHeader().utf8().data());
        WTFLogAlways("  hasVaryingRequestHeaders: %s", resource->hasVaryingRequestHeaders() ? "true" : "false");
        WTFLogAlways("  Accept-Language: %s", resource->acceptLanguage().utf8().data());
        WTFLogAlways("  isSSL: %s", resource->isSSL() ? "true" : "false");
        switch (resource->secureState()){
        case WKC::SSLSecureState::ESecureStateRed:
            WTFLogAlways("  SSL Secure state: Red (Danger)");
            break;
        case WKC::SSLSecureState::ESecureStateWhite:
            WTFLogAlways("  SSL Secure state: White (non-ssl)");
            break;
        case WKC::SSLSecureState::ESecureStateBlue:
            WTFLogAlways("  SSL Secure state: Blue (Normal SSL)");
            break;
        case WKC::SSLSecureState::ESecureStateGreen:
            WTFLogAlways("  SSL Secure state: Green (EV SSL)");
            break;
        default:
            break;
        }
        switch (resource->secureLevel()) {
        case WKC::SSLSecureLevel::ESecureLevelUnahthorized:
            WTFLogAlways("  SSL Secure level: Unauthorized");
            break;
        case WKC::SSLSecureLevel::ESecureLevelNonSSL:
            WTFLogAlways("  SSL Secure level: non-ssl");
            break;
        case WKC::SSLSecureLevel::ESecureLevelInsecure:
            WTFLogAlways("  SSL Secure level: Insecure");
            break;
        case WKC::SSLSecureLevel::ESecureLevelSecure:
            WTFLogAlways("  SSL Secure level: Secure");
            break;
        default:
            break;
        }
        WTFLogAlways("");
    }
}

} // namespace

#endif // ENABLE(WKC_HTTPCACHE)

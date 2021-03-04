/*
 *  Copyright (c) 2011-2020 ACCESS CO., LTD. All rights reserved.
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

#include "WKCEnums.h"

#ifdef __WKC_IMPLICIT_INCLUDE_SYSSTAT
#include <sys/stat.h>
#endif

// debug print
#define NO_NXLOG
#include "utils/nxDebugPrint.h"

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

HTTPCachedResource::HTTPCachedResource(const String& filename)
{
    m_used = false;
    m_httpequivflags = 0;
    m_resourceData = nullptr;
    m_lastUsedTime = 0;

    m_expectedContentLength = 0;
    m_httpStatusCode = 0;
    m_noCache = false;
    m_mustRevalidate = false;
    m_expires = 0;
    m_maxAge = 0;
    m_date = 0;
    m_lastModified = 0;
    m_responseTime = 0;
    m_age = 0;
    m_isSSL = false;
    m_secureState = WKC::ESecureStateWhite; /* non-ssl */
    m_secureLevel = WKC::ESecureLevelNonSSL; /* non-ssl */

    m_contentLength = 0;
    m_alignedContentLength = 0;
    m_resourceSize = 0;

    m_serverpushed = false;

    m_fileName = filename;
}

HTTPCachedResource::HTTPCachedResource(const String& filename, const URL &url, const ResourceResponse &response, bool serverpush)
{
    m_used = false;
    m_serverpushed = serverpush;
    m_httpequivflags = 0;
    m_lastUsedTime = MonotonicTime::now().secondsSinceEpoch().value();

    m_contentLength = 0;
    m_alignedContentLength = 0;
    m_resourceSize = 0;

    m_url = url.string();
    setResourceResponse(response, true);

    m_fileName = filename;
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

void HTTPCachedResource::update(bool noCache, bool mustRevalidate, double expires, double maxAge, int httpequivflags)
{
    if ((!(m_httpequivflags&EHTTPEquivNoCache)) || (httpequivflags&EHTTPEquivNoCache))
        m_noCache = noCache;
    if ((!(m_httpequivflags&EHTTPEquivMustRevalidate)) || (httpequivflags&EHTTPEquivMustRevalidate))
        m_mustRevalidate = mustRevalidate;

    if ((!(m_httpequivflags&EHTTPEquivMaxAge)) || (httpequivflags&EHTTPEquivMaxAge)) {
        if (isfinite(maxAge)) {
            if (!isfinite(m_maxAge) || (m_maxAge > maxAge))
                m_maxAge = maxAge;
        }
    }
    if (isfinite(expires)) {
        if (!isfinite(m_expires) || (m_expires > expires))
            m_expires = expires;
    }

    m_httpequivflags |= httpequivflags;
}

bool HTTPCachedResource::setResourceResponse(const ResourceResponse &response, bool setresponsetime)
{
    m_mimeType = response.mimeType();
    m_expectedContentLength = response.expectedContentLength();
    m_textEncodingName = response.textEncodingName();
    m_suggestedFilename = response.suggestedFilename();
    m_httpStatusCode = response.httpStatusCode();
    m_httpStatusText = response.httpStatusText();

    if (setresponsetime)
        m_responseTime = MonotonicTime::now().secondsSinceEpoch().value();

    m_expires = response.expires() ? (double)response.expires().value().secondsSinceEpoch().value() : numeric_limits<double>::quiet_NaN();
    m_date = response.date() ? (double)response.date().value().secondsSinceEpoch().value() : numeric_limits<double>::quiet_NaN();
    m_lastModified = response.lastModified() ? (double)response.lastModified().value().secondsSinceEpoch().value() : numeric_limits<double>::quiet_NaN();
    m_lastModifiedHeader = response.httpHeaderField(String("Last-Modified"));
    m_eTagHeader = response.httpHeaderField(String("ETag"));
    m_age = response.age() ? (double)response.age().value().value() : numeric_limits<double>::quiet_NaN();
    m_accessControlAllowOriginHeader = response.httpHeaderField(String("Access-Control-Allow-Origin"));
    m_contentTypeHeader = response.httpHeaderField(HTTPHeaderName::ContentType);

    ResourceHandle* job = response.resourceHandle();
    ResourceHandleInternal* d = job->getInternal();
    m_isSSL = d->m_isSSL;
    m_isEVSSL = d->m_isEVSSL;
    m_secureState = d->m_secureState;
    m_secureLevel = d->m_secureLevel;

    if (!(m_httpequivflags&EHTTPEquivNoCache))
        m_noCache = response.cacheControlContainsNoCache();
    if (!(m_httpequivflags&EHTTPEquivMustRevalidate))
        m_mustRevalidate |= response.cacheControlContainsMustRevalidate();
    if (!(m_httpequivflags&EHTTPEquivMaxAge))
        m_maxAge = response.cacheControlMaxAge() ? (double)response.cacheControlMaxAge().value().value() : numeric_limits<double>::quiet_NaN();

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
    nxLog_i("cache write %s", m_url.utf8().data());

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
    nxLog_i("cache read %s", m_url.utf8().data());

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
    size_t size = sizeof(long long) * 2
        + sizeof(int) * 7
        + sizeof(double) * 7
        + sizeof(int) + ROUNDUP(m_url.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_mimeType.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_textEncodingName.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_suggestedFilename.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_fileName.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_httpStatusText.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_lastModifiedHeader.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_eTagHeader.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_accessControlAllowOriginHeader.utf8().length(), ROUNDUP_UNIT)
        + sizeof(int) + ROUNDUP(m_contentTypeHeader.utf8().length(), ROUNDUP_UNIT);

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

HTTPCachedResource* HTTPCache::createHTTPCachedResource(URL &url, SharedBuffer* resourceData, ResourceResponse &response, bool noCache, bool mustRevalidate, double expires, double maxAge, bool serverpush)
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
    HTTPCachedResource *resource = new HTTPCachedResource(makeFileName(), url, response, serverpush);
    if (!resource)
        return 0;
    
    resource->update(noCache, mustRevalidate, expires, maxAge);
    resource->setResourceData(resourceData);
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

void HTTPCache::updateCachedResource(HTTPCachedResource *resource, SharedBuffer* resourceData, ResourceResponse &response, bool noCache, bool mustRevalidate, double expires, double maxAge, bool serverpush)
{
    resource->setResourceResponse(response, true);
    resource->setResourceData(resourceData);
    resource->update(noCache, mustRevalidate, expires, maxAge, serverpush);
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

    nxLog_in("remove cache file %s", resource->fileName().utf8().data());

    m_resources.remove(resource->url());
    m_resourceList.remove(listNumber);
    m_totalResourceSize -= resource->resourceSize();
    m_totalContentsSize -= resource->alignedContentLength();

    const String fileFullPath = pathByAppendingComponent(m_filePath, resource->fileName());
    wkcFileUnlinkPeer(fileFullPath.utf8().data());

    delete resource;

    nxLog_out("");
}

void HTTPCache::remove(HTTPCachedResource *resource)
{
    removeResource(resource);

    nxLog_in("remove cache file %s", resource->fileName().utf8().data());

    const String fileFullPath = pathByAppendingComponent(m_filePath, resource->fileName());
    wkcFileUnlinkPeer(fileFullPath.utf8().data());

    delete resource;

    nxLog_out("");
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

URL HTTPCache::removeFragmentIdentifierIfNeeded(const URL& originalURL)
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

HTTPCachedResource* HTTPCache::resourceForURL(const URL& resourceURL)
{
    URL url = removeFragmentIdentifierIfNeeded(resourceURL);
    HTTPCachedResource* resource = m_resources.get(url);

    return resource;
}

bool HTTPCache::equalHTTPCachedResourceURL(HTTPCachedResource *resource, URL& resourceURL)
{
    URL url = removeFragmentIdentifierIfNeeded(resourceURL);
    if (equalIgnoringASCIICase(resource->url(), url.string()))
        return true;

    return false;
}

String HTTPCache::makeFileName()
{
    String fileName;

    fileName = String::format("%08d.dcf", m_fileNumber);
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
        resource = new HTTPCachedResource(makeFileName());
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
#define CACHEFAT_FORMAT_VERSION     9  // Number of int. Increment this if you changed the content format in the fat file.

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
        nxLog_e("FAT file read error: failed to stat");
        wkcFileFClosePeer(fd);
        return false;
    }
    if (st.st_size <= MD5_DIGESTSIZE + sizeof(int)*2) {
        nxLog_e("FAT file read error: size is too small");
        wkcFileFClosePeer(fd);
        return false;
    }

    WTF::TryMallocReturnValue rv = tryFastZeroedMalloc(st.st_size);
    if (!rv.getValue(buf)) {
        nxLog_e("FAT file read error: no memory");
        goto error_end;
    }

    len = wkcFileFReadPeer(buf, sizeof(char), st.st_size, fd);
    wkcFileFClosePeer(fd);
    fd = 0;
    if (len<0 || len!=st.st_size) {
        nxLog_e("FAT file read error: failed to read");
        goto error_end;
    }

    buffer = buf;

    if (!verifyDigest((unsigned char*)buffer, len)) {
        nxLog_e("FAT file read error: MD5 check");
        remove = true;
        goto error_end;
    }

    buffer += MD5_DIGESTSIZE;

    format_version = (*(int*)buffer); buffer += sizeof(int);
    if (format_version != CACHEFAT_FORMAT_VERSION) {
        nxLog_e("FAT file format is old : %d", format_version);
        remove = true;
        goto error_end;
    }

    m_fileNumber = (*(int*)buffer); buffer += sizeof(int);
    len -= sizeof(int) + sizeof(int) + MD5_DIGESTSIZE;

    if (len<=0) {
        nxLog_e("FAT file read error: size is too small");
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

    nxLog_e("HTTP Cache info");
    nxLog_e("----------");
    nxLog_e(" total content size: %ld", m_totalContentsSize);
    for (int i = 0; i < num; i++){
        HTTPCachedResource *resource = m_resourceList[i];
        nxLog_e("----------");
        nxLog_e("Cache %d: %s", i, resource->fileName().utf8().data());
        nxLog_e(" URL: %s", resource->url().utf8().data());
        nxLog_e(" last used time: %lf", resource->lastUsedTime());
        nxLog_e(" HTTP Status code: %d", resource->httpStatusCode());
        nxLog_e(" mime type: %s", resource->mimeType().utf8().data());
        nxLog_e(" original content length: %ld", resource->contentLength());
        nxLog_e(" aligned content length: %ld", resource->alignedContentLength());
        nxLog_e(" expires: %lf max-age: %lf", resource->expires(), resource->maxAge());
        nxLog_e(" Last-Modified: %s", resource->lastModifiedHeader().utf8().data());
        nxLog_e(" eTag: %s", resource->eTagHeader().utf8().data());
        nxLog_e(" isSSL: %s", resource->isSSL() ? "true" : "false");
        switch (resource->secureState()){
        case WKC::SSLSecureState::ESecureStateRed:
            nxLog_e(" SSL Secure state: Red (Danger)");
            break;
        case WKC::SSLSecureState::ESecureStateWhite:
            nxLog_e(" SSL Secure state: White (non-ssl)");
            break;
        case WKC::SSLSecureState::ESecureStateBlue:
            nxLog_e(" SSL Secure state: Blue (Normal SSL)");
            break;
        case WKC::SSLSecureState::ESecureStateGreen:
            nxLog_e(" SSL Secure state: Green (EV SSL)");
            break;
        default:
            break;
        }
        switch (resource->secureLevel()) {
        case WKC::SSLSecureLevel::ESecureLevelUnahthorized:
            nxLog_e(" SSL Secure level: Unauthorized");
            break;
        case WKC::SSLSecureLevel::ESecureLevelNonSSL:
            nxLog_e(" SSL Secure level: non-ssl");
            break;
        case WKC::SSLSecureLevel::ESecureLevelInsecure:
            nxLog_e(" SSL Secure level: Insecure");
            break;
        case WKC::SSLSecureLevel::ESecureLevelSecure:
            nxLog_e(" SSL Secure level: Secure");
            break;
        default:
            break;
        }
        nxLog_e("----------");
    }
    nxLog_e("HTTP Cache info end.");
}

} // namespace

#endif // ENABLE(WKC_HTTPCACHE)

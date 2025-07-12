/*
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
#include <wtf/SHA1.h>
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

RefPtr<HTTPCachedResource>
HTTPCachedResource::create(const String& filename, const String& url)
{
    return adoptRef(*new HTTPCachedResource(filename, url));
}

HTTPCachedResource::HTTPCachedResource(const String& filename, const String& url)
    : m_alignedContentLength(0)
    , m_resourceData(nullptr)
    , m_resourceSize(0)
    , m_serverpushed(false)
    , m_fileExists(false)
    , m_noStore(false)
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
    , m_url(url.isolatedCopy())
    , m_mimeType()
    , m_textEncodingName()
    , m_suggestedFilename()
    , m_fileName(filename.isolatedCopy())
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
    LOG_FUNCTION(HTTPCache, "Destroyed HTTPCachedResource: %p", this);
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
    m_mimeType = response.mimeType().isolatedCopy();
    m_expectedContentLength = response.expectedContentLength();
    m_textEncodingName = response.textEncodingName().isolatedCopy();
    m_suggestedFilename = response.suggestedFilename().isolatedCopy();
    m_httpStatusCode = response.httpStatusCode();
    m_httpStatusText = response.httpStatusText().isolatedCopy();
    m_responseTime = MonotonicTime::now().secondsSinceEpoch().value();
    m_date = response.date() ? (double)response.date().value().secondsSinceEpoch().value() : numeric_limits<double>::quiet_NaN();
    m_lastModified = response.lastModified() ? (double)response.lastModified().value().secondsSinceEpoch().value() : numeric_limits<double>::quiet_NaN();
    m_lastModifiedHeader = response.httpHeaderField(HTTPHeaderName::LastModified).isolatedCopy();
    m_eTagHeader = response.httpHeaderField(HTTPHeaderName::ETag).isolatedCopy();
    m_age = response.age() ? (double)response.age().value().value() : numeric_limits<double>::quiet_NaN();
    m_accessControlAllowOriginHeader = response.httpHeaderField(HTTPHeaderName::AccessControlAllowOrigin).isolatedCopy();
    m_contentTypeHeader = response.httpHeaderField(HTTPHeaderName::ContentType).isolatedCopy();
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
    m_noStore = response.cacheControlContainsNoStore();  // must be false.

    // Vary header only supports Accept-Language for now.
    m_hasVaryingRequestHeaders = false;
    m_acceptLanguage = String();
    for (auto& varyingRequestHeader : varyingRequestHeaders) {
        if (equalIgnoringASCIICase(varyingRequestHeader.first, httpHeaderNameString(HTTPHeaderName::AcceptLanguage))) {
            m_hasVaryingRequestHeaders = true;
            m_acceptLanguage = varyingRequestHeader.second.isolatedCopy();
            break;
        }
    }

    return true;
}

void HTTPCachedResource::combineHeaders(const ResourceResponse &originalResponse, Vector<std::pair<String, String>> varyingRequestHeaders)
{
    m_responseTime = MonotonicTime::now().secondsSinceEpoch().value();

    ResourceResponse response;
    updateResponseHeadersAfterRevalidation(response, originalResponse);
    if (response.hasHTTPHeader(HTTPHeaderName::Date))
        m_date = (double)response.date().value().secondsSinceEpoch().value();
    if (response.hasHTTPHeader(HTTPHeaderName::Age))
        m_age = (double)response.age().value().value();
    if (response.hasHTTPHeader(HTTPHeaderName::AccessControlAllowOrigin))
        m_accessControlAllowOriginHeader = response.httpHeaderField(HTTPHeaderName::AccessControlAllowOrigin).isolatedCopy();
    if (response.hasHTTPHeader(HTTPHeaderName::LastModified)) {
        m_lastModified = (double)response.lastModified().value().secondsSinceEpoch().value();
        m_lastModifiedHeader = response.httpHeaderField(HTTPHeaderName::LastModified).isolatedCopy();
    }
    if (response.hasHTTPHeader(HTTPHeaderName::Expires))
        m_expires = (double)response.expires().value().secondsSinceEpoch().value();
    if (response.hasHTTPHeader(HTTPHeaderName::CacheControl)) {
        CacheControlDirectives cacheControlDirectives = response.cacheControlDirectives();
        if (cacheControlDirectives.noCache)
            m_noCache = response.cacheControlContainsNoCache();
        else
            m_noCache = false;
        if (cacheControlDirectives.mustRevalidate)
            m_mustRevalidate = response.cacheControlContainsMustRevalidate();
        else
            m_mustRevalidate = false;
        if (cacheControlDirectives.maxAge)
            m_maxAge = (double)response.cacheControlMaxAge().value().value();
        else
            m_maxAge = numeric_limits<double>::quiet_NaN();
        if (cacheControlDirectives.noStore)
            m_noStore = response.cacheControlContainsNoStore();
        else
            m_noStore = false;
    }

    // Vary header only supports Accept-Language for now.
    for (auto& varyingRequestHeader : varyingRequestHeaders) {
        if (equalIgnoringASCIICase(varyingRequestHeader.first, httpHeaderNameString(HTTPHeaderName::AcceptLanguage))) {
            m_hasVaryingRequestHeaders = true;
            m_acceptLanguage = varyingRequestHeader.second.isolatedCopy();
            break;
        }
    }
}

bool HTTPCachedResource::writeFile(const String& filepath)
{
    void *fd = 0;
    bool result = false;

    if (!m_resourceData) {
        LOG_FUNCTION(HTTPCache, "resourceData is null (HTTPCachedResource: %p)", this);
        return false;
    }

    String fileFullPath = pathByAppendingComponent(filepath, m_fileName);

    // Do not call malloc/free/lock/unlock between wkcFileFOpenPeer and wkcFileFClosePeer.
    // If they are called during restarting BrowserEngine,
    // longjump will be called and the thread exits.
    fd = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_CACHE, fileFullPath.utf8().data(), "wb");
    if (!fd)
        return result;
    setFileExists(true);

    size_t len = 0;
    wkc_uint64 written = 0;
    auto buffer = m_resourceData->data();

    for (len = m_resourceData->size(); len > 0;  len -= written) {
        written = wkcFileFWritePeer(buffer, sizeof(char), len, fd);
        if (written <= 0)
            break;
        buffer += written;
    }
    if (len > 0)
        goto error_end;

    result = true;

error_end:
    if (fd)
        wkcFileFClosePeer(fd);
    if (result) {
        m_resourceData->deref();
        m_resourceData = nullptr;
        LOG_FUNCTION(HTTPCache, "cache write %s", m_url.utf8().data());
    }

    return result;
}

RefPtr<SharedBuffer> HTTPCachedResource::readFile(const String& filepath)
{
    String fileFullPath = pathByAppendingComponent(filepath, m_fileName);
    auto buffer = SharedBuffer::createWithContentsOfFile(fileFullPath);
    if (!buffer)
        LOG_FUNCTION(HTTPCache, "readFile failed: %s (HTTPCachedResource: %p)", fileFullPath.utf8().data(), this);
    return buffer;
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
    if (m_resourceData) {
        m_resourceData->deref();
        m_resourceData = nullptr;
    }
    if (resourceData) {
        m_resourceData = resourceData;
        m_resourceData->ref();
        m_contentLength = resourceData->size();
        m_alignedContentLength = roundUpToMultipleOf(m_contentLength, ContentSizeAlignment);
    }
}

bool HTTPCachedResource::varyHeaderValuesMatch(const ResourceRequest& request)
{
    if (!m_hasVaryingRequestHeaders)
        return true;

    // Vary header only supports Accept-Language for now.
    if (equalIgnoringASCIICase(request.httpHeaderField(HTTPHeaderName::AcceptLanguage), m_acceptLanguage))
        return true;

    return false;
}

bool HTTPCachedResource::unlinkFile(const String& path)
{
    if (m_fileExists && !path.isEmpty() && !m_fileName.isEmpty()) {
        const String fileFullPath = pathByAppendingComponent(path, m_fileName);
        wkcFileUnlinkPeer(fileFullPath.utf8().data());
        m_fileExists = false;
        return true;
    }
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
    // Do not delete cache files when using this method.
    for (size_t num = 0; num < m_resourceList.size(); num++) {
        auto& resource = m_resourceList[num];
        LOG_FUNCTION(HTTPCache, "delete HTTPCachedResource: %p", resource.get());
    }
    m_resourceList.clear();
    m_resources.clear();

    m_totalResourceSize = 0;
    m_totalContentsSize = 0;
}

void
HTTPCache::appendResourceInSizeOrder(Vector<RefPtr<HTTPCachedResource>>& resourceList, RefPtr<HTTPCachedResource>& resource)
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
HTTPCache::appendResourceInLastUsedTimeOrder(Vector<RefPtr<HTTPCachedResource>>& resourceList, RefPtr<HTTPCachedResource>& resource)
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

RefPtr<HTTPCachedResource> HTTPCache::createHTTPCachedResource(URL &url, SharedBuffer* resourceData, ResourceRequest& request, ResourceResponse &response, Vector<std::pair<String, String>> varyingRequestHeaders, bool serverpush)
{
    if (disabled())
        return nullptr;

    long long originalContentLength = resourceData->size();
    long long alignedContentLength = roundUpToMultipleOf(originalContentLength, ContentSizeAlignment);

    if (m_maxContentSize < alignedContentLength)
        return nullptr;   // size over
    if (m_minContentSize > originalContentLength)
        return nullptr;   // too small to cache
    if (m_maxTotalContentsSize < alignedContentLength)
        return nullptr;   // size over
    if (m_resourceList.size() >= m_maxContentEntries)
        if (purgeOldest()==0)
            return nullptr; // entry limit over
    // Checking the total size and calling to purgeBySize must be done after calling gCanCacheToDiskCallback.

    const URL kurl = removeFragmentIdentifierIfNeeded(url);
    RefPtr<HTTPCachedResource> resource = HTTPCachedResource::create(makeFileName(), kurl.string());
    if (!resource)
        return nullptr;

    updateCachedResource(resource, resourceData, request, response, WTFMove(varyingRequestHeaders), serverpush);
    return resource;
}

bool HTTPCache::addCachedResource(RefPtr<HTTPCachedResource>& resource)
{
    if (disabled())
        return false;

    if (resource->resourceSize() > m_maxContentEntryInfoSize) {
        return false; // entry info size over
    }

    if (m_resourceList.size() >= m_maxContentEntries)
        if (purgeOldest()==0)
            return false; // entry limit over

    RecursiveLock::Holder lock(m_lock);
    m_resources.set(resource->url(), resource);
    m_resourceList.append(resource);
    m_totalResourceSize += resource->resourceSize();
    m_totalContentsSize += resource->alignedContentLength();

    return true;
}

void HTTPCache::updateResourceLastUsedTime(RefPtr<HTTPCachedResource>& resource)
{
    resource->setLastUsedTime(MonotonicTime::now().secondsSinceEpoch().value());

    RecursiveLock::Holder lock(m_lock);
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

void HTTPCache::updateCachedResource(RefPtr<HTTPCachedResource>& resource, SharedBuffer* resourceData, ResourceRequest& request, ResourceResponse &response, Vector<std::pair<String, String>> varyingRequestHeaders, bool serverpush)
{
    resource->setResourceResponse(response, WTFMove(varyingRequestHeaders), serverpush);
    resource->setResourceData(resourceData);
    updateResourceLastUsedTime(resource);
}

void HTTPCache::combineHeaders(RefPtr<HTTPCachedResource>& resource, const ResourceResponse &response, Vector<std::pair<String, String>> varyingRequestHeaders)
{
    if (response.httpStatusCode() != 304)
        return;

    RecursiveLock::Holder lock(m_lock);
    m_totalResourceSize -= resource->resourceSize();
    resource->combineHeaders(response, WTFMove(varyingRequestHeaders));
    resource->calcResourceSize();
    m_totalResourceSize += resource->resourceSize();
    // HTTPCache::updateResourceLastUsedTime() is called in HTTPCache::read().
}

void HTTPCache::removeResource(RefPtr<HTTPCachedResource> resource)
{
    RecursiveLock::Holder lock(m_lock);
    if (!m_resources.remove(resource->url()))
        return;

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
    RecursiveLock::Holder lock(m_lock);
    auto resource = m_resourceList[listNumber];

    LOG_FUNCTION(HTTPCache, "remove cache file %s (HTTPCachedResource: %p)", resource->fileName().utf8().data(), resource.get());

    m_resources.remove(resource->url());
    m_resourceList.remove(listNumber);
    m_totalResourceSize -= resource->resourceSize();
    m_totalContentsSize -= resource->alignedContentLength();

    resource->unlinkFile(m_filePath);
}

void HTTPCache::remove(RefPtr<HTTPCachedResource>& resource)
{
    removeResource(resource);

    LOG_FUNCTION(HTTPCache, "remove cache file %s (HTTPCachedResource: %p)", resource->fileName().utf8().data(), resource.get());

    resource->unlinkFile(m_filePath);
}

void HTTPCache::detach(RefPtr<HTTPCachedResource>& resource)
{
    removeResource(resource);

    LOG_FUNCTION(HTTPCache, "detach cache file %s (HTTPCachedResource: %p)", resource->fileName().utf8().data(), resource.get());

    resource->unlinkFile(m_filePath);
}

void HTTPCache::removeAll()
{
    reset();
    m_fileNumber = 0;

    Vector<String> items(listDirectory(m_filePath));
    items.removeAllMatching([](const String& s) {return !s.endsWith(".dcf"); });

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
    Vector<String> items(listDirectory(m_filePath));
    items.removeAllMatching([](const String& s) {return !s.endsWith(".dcf"); });
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

    unsigned int inconsistency = 0;
    for (int i=infat.size()-1; i>=0; i--) {
        size_t pos = indir.find(infat[i]);
        if (pos == notFound) {
            removeResource(m_resourceList[i]);
            inconsistency++;
        }
        else
            m_resourceList[i]->setFileExists(true);
    }
    if (inconsistency > 0) {
        LOG_FUNCTION(HTTPCache, "FAT file had inconsistent data : %u", inconsistency);
        if (!writeFATFile())
            LOG_FUNCTION(HTTPCache, "FAT file writing failed");
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

RefPtr<HTTPCachedResource> HTTPCache::resourceForURL(const WTF::URL& resourceURL)
{
    WTF::URL url = removeFragmentIdentifierIfNeeded(resourceURL);
    RecursiveLock::Holder lock(m_lock);
    return m_resources.get(url.string());
}

bool HTTPCache::equalHTTPCachedResourceURL(RefPtr<HTTPCachedResource>& resource, WTF::URL& resourceURL)
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
    RecursiveLock::Holder lock(m_lock);
    long long purgedSize = 0;

    for (size_t num = 0; num < m_resourceList.size(); ) {
        auto& resource = m_resourceList[num];
        if (resource->refCount() > 2) // don't remove using resource (m_resources, m_resourceList, and others have resource)
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
    RecursiveLock::Holder lock(m_lock);
    long long purgedSize = purgeBySizeInternal(size);
    if (purgedSize < size) {
        for (size_t num = 0; num < m_resourceList.size(); num++) {
            auto& resource = m_resourceList[num];
            purgedSize += resource->alignedContentLength();
            removeResourceByNumber(num);
            if (purgedSize >= size)
                break;
        }
    }
    return purgedSize;
}

long long HTTPCache::purgeOldest()
{
    RecursiveLock::Holder lock(m_lock);
    long long purgedSize = 0;

    for (size_t num = 0; num < m_resourceList.size(); ) {
        auto& resource = m_resourceList[num];
        if (resource->refCount() > 2) // don't remove using resource (m_resources, m_resourceList, and others have resource)
            num++;
        else {
            purgedSize += resource->alignedContentLength();
            removeResourceByNumber(num);
            break;
        }
    }
    return purgedSize;
}

bool HTTPCache::write(RefPtr<HTTPCachedResource>& resource)
{
    resource->calcResourceSize();

    if (resource->resourceSize() > m_maxContentEntryInfoSize) {
        LOG_FUNCTION(HTTPCache, "entry info size over (HTTPCachedResource: %p)", resource.get());
        return false;
    }

    if (m_resourceList.size() >= m_maxContentEntries) {
        if (purgeOldest()==0) {
            LOG_FUNCTION(HTTPCache, "entry limit over (HTTPCachedResource: %p)", resource.get());
            return false;
        }
    }

    long long alignedContentLength = resource->alignedContentLength();

    if (gCanCacheToDiskCallback && !gCanCacheToDiskCallback(resource->url().utf8().data(),
                                                            resource->computeCurrentAge(),
                                                            resource->freshnessLifetime(),
                                                            resource->contentLength(),
                                                            alignedContentLength,
                                                            resource->mimeType().utf8().data(),
                                                            resource->noCache())) {
        LOG_FUNCTION(HTTPCache, "CanCacheToDiskCallback returned false (HTTPCachedResource: %p)", resource.get());
        return false;
    }

    {
        // Locking including writeFile() negatively impacts main thread performance.
        RecursiveLock::Holder lock(m_lock);
        if (m_totalContentsSize + alignedContentLength > m_maxTotalContentsSize) {
            long long sizeToRemove = m_totalContentsSize + alignedContentLength - m_maxTotalContentsSize;
            if (purgeBySize(sizeToRemove) < sizeToRemove) {
                LOG_FUNCTION(HTTPCache, "exceeded the total size (HTTPCachedResource: %p)", resource.get());
                return false;
            }
        }
    }

    if (!resource->writeFile(m_filePath)) {
        LOG_FUNCTION(HTTPCache, "writeFile failed (HTTPCachedResource: %p)", resource.get());
        return false;
    }

    RecursiveLock::Holder lock(m_lock);
    m_resources.add(resource->url(), resource);
    appendResourceInLastUsedTimeOrder(m_resourceList, resource);
    m_totalResourceSize += resource->resourceSize();
    m_totalContentsSize += alignedContentLength;

    return true;
}

RefPtr<SharedBuffer> HTTPCache::read(RefPtr<HTTPCachedResource>& resource)
{
    RefPtr<SharedBuffer> buffer = resource->readFile(m_filePath);
    if (!buffer) {
        return nullptr;
    }
    updateResourceLastUsedTime(resource);
    LOG_FUNCTION(HTTPCache, "cache read %s (HTTPCachedResource: %p)", resource->url().utf8().data(), resource.get());
    return buffer;
}

void HTTPCache::serializeFATData(char *buffer)
{
    RecursiveLock::Holder lock(m_lock);
    for (size_t num = 0; num < m_resourceList.size(); num++)
        buffer += m_resourceList[num]->serialize(buffer);
}

bool HTTPCache::deserializeFATData(char *buffer, int length)
{
    int read = 0;

    for (; length > 0; length -= read) {
        auto resource = HTTPCachedResource::create(makeFileName(), String());
        if (!resource)
            return false;

        read = resource->deserialize(buffer);
        buffer += read;
        addCachedResource(resource);
    }

    return true;
}

#define DEFAULT_CACHEFAT_FILENAME   "cache.fat"
#define CACHEFAT_FORMAT_VERSION     11  // Number of int. Increment this if you changed the content format in the fat file.

void HTTPCache::writeDigest(unsigned char *data, int length)
{
    SHA1 sha1;
    sha1.addBytes(data + SHA1::hashSize, length - SHA1::hashSize);
    SHA1::Digest digest;
    sha1.computeHash(digest);
    memcpy(data, digest.data(), SHA1::hashSize);
}

bool HTTPCache::writeFATFile()
{
    char *buf = 0, *buffer = 0;

    int totalSize = 0;
    int headerSize = 0;

    headerSize = sizeof(int) * 2 + SHA1::hashSize;
    totalSize = headerSize + m_totalResourceSize;

    WTF::TryMallocReturnValue rv = tryFastZeroedMalloc(totalSize);
    if (!rv.getValue(buf))
        return false;

    buffer = buf + SHA1::hashSize;
    (*(int*)buffer) = CACHEFAT_FORMAT_VERSION; buffer += sizeof(int);
    (*(int*)buffer) = m_fileNumber; buffer += sizeof(int);

    serializeFATData(buffer);

    writeDigest((unsigned char*)buf, totalSize);

    String fileFullPath = pathByAppendingComponent(m_filePath, m_fatFileName);

    void *fd = 0;
    size_t len = 0;
    wkc_uint64 written = 0;
    // Do not call malloc/free/lock/unlock between wkcFileFOpenPeer and wkcFileFClosePeer.
    // If they are called during restarting BrowserEngine,
    // longjump will be called and the thread exits.
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
    if (length <= SHA1::hashSize)
        return false;

    bool result = false;
    SHA1 sha1;
    sha1.addBytes(data + SHA1::hashSize, length - SHA1::hashSize);
    SHA1::Digest digest;
    sha1.computeHash(digest);

    if (memcmp(data, digest.data(), SHA1::hashSize) == 0)
        result = true;

    return result;
}

bool HTTPCache::readFATFile()
{
    void* fd = 0;
    bool result = false;
    bool clear = true;
    char* buf = nullptr;
    char* fatp = nullptr;
    int format_version = 0;
    int len = 0;

    String fileFullPath = pathByAppendingComponent(m_filePath, m_fatFileName);

    fd = wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_CACHE, fileFullPath.utf8().data(), "rb");
    if (!fd) {
        LOG_FUNCTION(HTTPCache, "FAT file read error: not found");
        goto exit;
    }

    struct stat st;
    if (wkcFileFStatPeer(fd, &st)) {
        LOG_FUNCTION(HTTPCache, "FAT file read error: failed to stat");
        goto exit;
    }
    if (st.st_size < SHA1::hashSize + sizeof(int)*2) {
        LOG_FUNCTION(HTTPCache, "FAT file read error: size is too small");
        goto exit;
    }

    buf = static_cast<char*>(fastMalloc(st.st_size));
    ASSERT(buf);

    len = wkcFileFReadPeer(buf, sizeof(char), st.st_size, fd);
    wkcFileFClosePeer(fd);
    fd = 0;
    if (len < 0 || len != st.st_size) {
        LOG_FUNCTION(HTTPCache, "FAT file read error: failed to read");
        goto exit;
    }

    fatp = buf;

    if (!verifyDigest((unsigned char*)fatp, len)) {
        LOG_FUNCTION(HTTPCache, "FAT file read error: SHA1 check");
        goto exit;
    }

    fatp += SHA1::hashSize;

    format_version = (*(int*)fatp); fatp += sizeof(int);
    if (format_version != CACHEFAT_FORMAT_VERSION) {
        LOG_FUNCTION(HTTPCache, "FAT file format is old : %d", format_version);
        goto exit;
    }

    m_fileNumber = (*(int*)fatp); fatp += sizeof(int);
    len -= sizeof(int) + sizeof(int) + SHA1::hashSize;

    if (len < 0) {
        LOG_FUNCTION(HTTPCache, "FAT file read error: size is too small");
        goto exit;
    }

    if (!deserializeFATData(fatp, len)) {
        LOG_FUNCTION(HTTPCache, "FAT file read error: failed to deserialize");
        goto exit;
    }
    // run the method below after closing the file since it may write to the file.
    sanitizeCaches();

    result = true;
    clear = false;

exit:
    if (fd)
        wkcFileFClosePeer(fd);

    if (clear)
        removeAll();

    fastFree(buf);
    return result;
}

void
HTTPCache::dumpResourceList()
{
    RecursiveLock::Holder lock(m_lock);
    int num = m_resourceList.size();

    if (num == 0) {
        WTFLogAlways("There are no HTTP cache files.");
        return;
    }

    WTFLogAlways("");
    for (int i = 0; i < num; i++){
        auto& resource = m_resourceList[i];
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
        WTFLogAlways("  RefCount: %u", resource->refCount());
        WTFLogAlways("");
    }
}

} // namespace

#endif // ENABLE(WKC_HTTPCACHE)

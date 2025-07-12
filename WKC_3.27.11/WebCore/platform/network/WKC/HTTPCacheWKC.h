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

#ifndef HTTPCacheWKC_h
#define HTTPCacheWKC_h

#include "WTFString.h"
#include "SharedBuffer.h"
#include "ResourceResponse.h"
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include "WKCWebView.h"

namespace WTF {
class URL;
}

namespace WebCore {

class RecursiveLock {
public:
    class Holder {
    public:
        Holder(RecursiveLock& lk) : m_lock(lk) { m_lock.lock(); }
        ~Holder() { m_lock.unlock(); }
        Holder() = delete;
    private:
        RecursiveLock& m_lock;
    };

    RecursiveLock(const RecursiveLock&) = delete;
    RecursiveLock& operator=(const RecursiveLock&) = delete;
    RecursiveLock() { m_lock = wkcMutexNewPeer(); }
    ~RecursiveLock() { wkcMutexDeletePeer(m_lock); }
#if OS(WINDOWS_WKC)
    // For some reason, recursive locking is not possible in the Win version.
    void lock() {
        void* tid = wkcThreadCurrentThreadPeer();
        if (m_owner == tid) {
            ++m_count;
            return;
        }
        wkcMutexLockPeer(m_lock);
        m_owner = tid;
        m_count = 1;
    }
    void unlock() {
        ASSERT(m_owner == wkcThreadCurrentThreadPeer());
        if (--m_count == 0) {
            m_owner = nullptr;
            wkcMutexUnlockPeer(m_lock);
        }
    }

private:
    void* m_lock;
    void* m_owner{nullptr};
    int m_count{0};
#else
    void lock() { wkcMutexLockPeer(m_lock); }
    void unlock() { wkcMutexUnlockPeer(m_lock); }

private:
    void* m_lock;
#endif
};

class ResourceResponse;

class HTTPCachedResource : public ThreadSafeRefCounted<HTTPCachedResource> {
WTF_MAKE_FAST_ALLOCATED;
public:
    static RefPtr<HTTPCachedResource> create(const String& filename, const String& url);
    ~HTTPCachedResource();

    bool setResourceResponse(const ResourceResponse &response, Vector<std::pair<String, String>> varyingRequestHeaders, bool serverpush);
    void combineHeaders(const ResourceResponse &response, Vector<std::pair<String, String>> varyingRequestHeaders);
    double freshnessLifetime();
    bool isExpired();
    bool needRevalidate();

    inline const String& url() const { return m_url; }
    inline const String& mimeType() const { return m_mimeType; }
    inline long long expectedContentLength() const { return m_expectedContentLength; }
    inline const String& suggestedFilename() const { return m_suggestedFilename; }
    inline const String& textEncodingName() const { return m_textEncodingName; }
    inline long long contentLength() const { return m_contentLength; }
    inline long long alignedContentLength() const { return m_alignedContentLength; }
    inline const String& fileName() const { return m_fileName; }
    inline int httpStatusCode() const { return m_httpStatusCode; }
    inline const String& httpStatusText() const { return m_httpStatusText; }
    inline bool hasVaryingRequestHeaders() const { return m_hasVaryingRequestHeaders; }
    inline bool noStore() const { return m_noStore; }
    inline bool noCache() const { return m_noCache; }
    inline bool mustRevalidate() const { return m_mustRevalidate; }
    inline double expires() const { return m_expires; }
    inline double maxAge() const { return m_maxAge; }
    inline const String& lastModifiedHeader() const { return m_lastModifiedHeader; }
    inline const String& eTagHeader() const { return m_eTagHeader; }
    inline const String& accessControlAllowOriginHeader() const { return m_accessControlAllowOriginHeader; }
    inline const String& contentTypeHeader() const { return m_contentTypeHeader; }
    inline const String& acceptLanguage() const { return m_acceptLanguage; }
    inline double lastUsedTime() const { return m_lastUsedTime; }
    inline void setLastUsedTime(double time) { m_lastUsedTime = time; }

    inline bool isSSL() const { return m_isSSL; }
    inline bool isEVSSL() const { return m_isEVSSL; }
    inline int secureState() const { return m_secureState; }
    inline int secureLevel() const { return m_secureLevel; }

    inline bool serverPushed() const { return m_serverpushed; }

    bool writeFile(const String& filepath);
    RefPtr<SharedBuffer> readFile(const String& filepath);

    void calcResourceSize();
    inline int resourceSize() const { return m_resourceSize; }

    int serialize(char *buffer);
    int deserialize(char *buffer);

    SharedBuffer* getResourceData() const { return m_resourceData; }
    void setResourceData(SharedBuffer* resourceData);

    bool varyHeaderValuesMatch(const ResourceRequest& request);

    double computeCurrentAge();

protected:
    HTTPCachedResource(const String& filename, const String& url);

    friend class HTTPCache;
    void setFileExists(bool b) { m_fileExists = b; }
    bool unlinkFile(const String& path);

private:
    long long m_alignedContentLength;
    SharedBuffer* m_resourceData;
    int m_resourceSize;
    bool m_serverpushed;
    bool m_fileExists;
    bool m_noStore;

    // Persistent Data
    long long m_expectedContentLength;
    long long m_contentLength;
    int m_httpStatusCode;
    bool m_hasVaryingRequestHeaders;
    bool m_noCache;
    bool m_mustRevalidate;
    bool m_isSSL;
    bool m_isEVSSL;
    int m_secureState;
    int m_secureLevel;
    double m_expires;
    double m_maxAge;
    double m_date;
    double m_lastModified;
    double m_responseTime;
    double m_age;
    double m_lastUsedTime;
    String m_url;
    String m_mimeType;
    String m_textEncodingName;
    String m_suggestedFilename;
    String m_fileName;
    String m_httpStatusText;
    String m_lastModifiedHeader;
    String m_eTagHeader;
    String m_accessControlAllowOriginHeader;
    String m_contentTypeHeader;
    String m_acceptLanguage;
};

typedef HashMap<String, RefPtr<HTTPCachedResource>> HTTPCachedResourceMap;
typedef HashMap<String, RefPtr<HTTPCachedResource>>::iterator HTTPCachedResourceMapIterator;

class HTTPCache {
public:
    HTTPCache();
    ~HTTPCache();
    void reset();

    static void setCanCacheToDiskCallback(WKC::CanCacheToDiskProc proc);
    static void appendResourceInSizeOrder(Vector<RefPtr<HTTPCachedResource>>& resourceList, RefPtr<HTTPCachedResource>& resource);
    static void appendResourceInLastUsedTimeOrder(Vector<RefPtr<HTTPCachedResource>>& resourceList, RefPtr<HTTPCachedResource>& resource);

    RefPtr<HTTPCachedResource> createHTTPCachedResource(URL &url, SharedBuffer* resourceData, ResourceRequest& request, ResourceResponse &response, Vector<std::pair<String, String>> varyingRequestHeaders, bool serverpush);
    bool addCachedResource(RefPtr<HTTPCachedResource>& resource);
    void updateCachedResource(RefPtr<HTTPCachedResource>& resource, SharedBuffer* resourceData, ResourceRequest& request, ResourceResponse &response, Vector<std::pair<String, String>> varyingRequestHeaders, bool serverpush);
    void combineHeaders(RefPtr<HTTPCachedResource>& resource, const ResourceResponse &response, Vector<std::pair<String, String>> varyingRequestHeaders);
    void updateResourceLastUsedTime(RefPtr<HTTPCachedResource>& resource);
    void removeResource(RefPtr<HTTPCachedResource> resource);
    void removeResourceByNumber(int listNumber);
    void remove(RefPtr<HTTPCachedResource>& resource);
    void detach(RefPtr<HTTPCachedResource>& resource);
    void removeAll();

    void setDisabled(bool disabled);
    bool disabled() const { return m_disabled; }
    void setMaxContentEntries(int limit);
    void setMaxContentEntryInfoSize(int limit);
    void setMaxContentSize(long long limit);
    void setMinContentSize(long long limit);
    void setMaxTotalCacheSize(long long limit);
    void setFilePath(const char* path);

    WTF::URL removeFragmentIdentifierIfNeeded(const WTF::URL& originalURL);
    RefPtr<HTTPCachedResource> resourceForURL(const WTF::URL& resourceURL);
    bool equalHTTPCachedResourceURL(RefPtr<HTTPCachedResource>& resource, WTF::URL& resourceURL);

    String makeFileName();
    long long purgeBySizeInternal(long long size);
    long long purgeBySize(long long size);
    long long purgeOldest();
    bool write(RefPtr<HTTPCachedResource>& resource);
    RefPtr<SharedBuffer> read(RefPtr<HTTPCachedResource>& resource);

    void serializeFATData(char *buffer);
    bool deserializeFATData(char *buffer, int length);
    bool writeFATFile();
    bool readFATFile();
    void sanitizeCaches();

    void writeDigest(unsigned char *data, int length);
    bool verifyDigest(unsigned char *data, int length);

    // for debug
    void dumpResourceList();

private:
    bool m_disabled;
    HTTPCachedResourceMap m_resources;
    Vector<RefPtr<HTTPCachedResource>> m_resourceList;
    RecursiveLock m_lock;  // protect the above 2 containers

    int m_fileNumber;
    int m_totalResourceSize;

    String m_fatFileName;
    String m_filePath;

    int m_maxContentEntries;
    int m_maxContentEntryInfoSize;
    long long m_maxContentSize;
    long long m_minContentSize;
    long long m_maxTotalContentsSize;

    long long m_totalContentsSize;
};

} // namespace


#endif //HTTPCacheWKC_h

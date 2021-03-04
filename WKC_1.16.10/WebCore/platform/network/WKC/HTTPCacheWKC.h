/*
 *  Copyright (c) 2011-2019 ACCESS CO., LTD. All rights reserved.
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


namespace WebCore {

class URL;
class ResourceResponse;

class HTTPCachedResource {
WTF_MAKE_FAST_ALLOCATED;
public:
    HTTPCachedResource(const String& filename);
    HTTPCachedResource(const String& filename, const URL &url, const ResourceResponse &response, bool serverpush);
    ~HTTPCachedResource();

    bool setResourceResponse(const ResourceResponse &response, bool setresponsetime);
    double freshnessLifetime();
    bool isExpired();
    bool needRevalidate();
    enum {
        EHTTPEquivNone           = 0x00000000,
        EHTTPEquivNoCache        = 0x00000001,
        EHTTPEquivMustRevalidate = 0x00000002,
        EHTTPEquivMaxAge         = 0x00000004,
    };
    void update(bool noCache, bool mustRevalidate, double expires, double maxAge, int httpequivflags = 0);

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
    inline bool noCache() const { return m_noCache; }
    inline bool mustRevalidate() const { return m_mustRevalidate; }
    inline double expires() const { return m_expires; }
    inline double maxAge() const { return m_maxAge; }
    inline const String& lastModifiedHeader() const { return m_lastModifiedHeader; }
    inline const String& eTagHeader() const { return m_eTagHeader; }
    inline const String& accessControlAllowOriginHeader() const { return m_accessControlAllowOriginHeader; }
    inline const String& contentTypeHeader() const { return m_contentTypeHeader; }
    inline double lastUsedTime() const { return m_lastUsedTime; }
    inline void setLastUsedTime(double time) { m_lastUsedTime = time; }

    inline bool isSSL() const { return m_isSSL; }
    inline bool isEVSSL() const { return m_isEVSSL; }
    inline int secureState() const { return m_secureState; }
    inline int secureLevel() const { return m_secureLevel; }

    inline bool serverPushed() const { return m_serverpushed; }

    bool writeFile(const String& filepath);
    bool readFile(char *buf, const String& filepath);

    void calcResourceSize();
    inline int resourceSize() const { return m_resourceSize; }

    int serialize(char *buffer);
    int deserialize(char *buffer);

    void setResourceData(SharedBuffer* resourceData);

    double computeCurrentAge();

    inline bool isUsed() const { return m_used; }
    inline void setUsed(bool used) { m_used = used; }

private:
    String m_url;
    String m_mimeType;
    long long m_expectedContentLength;
    String m_textEncodingName;
    String m_suggestedFilename;
    int m_httpStatusCode;
    String m_httpStatusText;
    bool m_noCache;
    bool m_mustRevalidate;
    double m_expires;
    double m_maxAge;
    double m_date;
    double m_lastModified;
    double m_responseTime;
    double m_age;
    double m_lastUsedTime;
    String m_lastModifiedHeader;
    String m_eTagHeader;
    String m_fileName;
    String m_accessControlAllowOriginHeader;
    String m_contentTypeHeader;

    bool m_isSSL;
    bool m_isEVSSL;
    int m_secureState;
    int m_secureLevel;

    SharedBuffer* m_resourceData;
    long long m_contentLength;
    long long m_alignedContentLength;
    int m_resourceSize;
    bool m_used;

    bool m_serverpushed;

    int m_httpequivflags;
};

typedef HashMap<String, HTTPCachedResource*> HTTPCachedResourceMap;
typedef HashMap<String, HTTPCachedResource*>::iterator HTTPCachedResourceMapIterator;

class HTTPCache {
public:
    HTTPCache();
    ~HTTPCache();
    void reset();

    static void setCanCacheToDiskCallback(WKC::CanCacheToDiskProc proc);
    static void appendResourceInSizeOrder(Vector<HTTPCachedResource*>& resourceList, HTTPCachedResource* resource);
    static void appendResourceInLastUsedTimeOrder(Vector<HTTPCachedResource*>& resourceList, HTTPCachedResource* resource);

    HTTPCachedResource* createHTTPCachedResource(URL &url, SharedBuffer* resourceData, ResourceResponse &response, bool noCache, bool mustRevalidate, double expires, double maxAge, bool serverpush);
    bool addCachedResource(HTTPCachedResource *resource);
    void updateCachedResource(HTTPCachedResource *resource, SharedBuffer* resourceData, ResourceResponse &response, bool noCache, bool mustRevalidate, double expires, double maxAge, bool serverpush);
    void updateResourceLastUsedTime(HTTPCachedResource *resource);
    void removeResource(HTTPCachedResource *resource);
    void removeResourceByNumber(int listNumber);
    void remove(HTTPCachedResource *resource);
    void detach(HTTPCachedResource *resource);
    void removeAll();

    void setDisabled(bool disabled);
    bool disabled() const { return m_disabled; }
    void setMaxContentEntries(int limit);
    void setMaxContentEntryInfoSize(int limit);
    void setMaxContentSize(long long limit);
    void setMinContentSize(long long limit);
    void setMaxTotalCacheSize(long long limit);
    void setFilePath(const char* path);

    URL removeFragmentIdentifierIfNeeded(const URL& originalURL);
    HTTPCachedResource* resourceForURL(const URL& resourceURL);
    bool equalHTTPCachedResourceURL(HTTPCachedResource *resource, URL& resourceURL);

    String makeFileName();
    long long purgeBySizeInternal(long long size);
    long long purgeBySize(long long size);
    long long purgeOldest();
    bool write(HTTPCachedResource *resource);
    bool read(HTTPCachedResource *resource, char *buf);

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
    Vector<HTTPCachedResource*> m_resourceList;

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

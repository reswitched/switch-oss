/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Appcelerator Inc.
 * Copyright (C) 2009 Brent Fulgham <bfulgham@webkit.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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

#include "ResourceHandleManagerWKC.h"
#include "ResourceHandleManagerWKCSSL.h"

#include "Base64.h"
#include "BlobRegistryImpl.h"
#include "Cookie.h"
#include "CookieJar.h"
#include "CString.h"
#include "DataURLDecoder.h"
#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include "NetworkStorageSession.h"
#include "NotImplemented.h"
#include "ResourceError.h"
#include "ResourceHandle.h"
#include "ResourceHandleInternalWKC.h"
#include "SynchronousLoaderClient.h"
#include "TextEncoding.h"
#include "FrameLoaderClientWKC.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "Page.h"
#include "Settings.h"
#include "SharedBuffer.h"
#include "SocketStreamClient.h"
#include "HTTPCacheWKC.h"
#include "LoggingWKC.h"

#include <errno.h>
#include <stdio.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

#include <wkc/wkcpeer.h>
#include <wkc/wkcsocket.h>


#if !PLATFORM(WIN_OS)
#include <sys/param.h>
#undef MAX_PATH
#define MAX_PATH MAXPATHLEN
#endif

#if COMPILER(MSVC)
#define strncasecmp _strnicmp
#endif

#ifndef TRUE
# define TRUE (1)
#endif
#ifndef FALSE
# define FALSE (0)
#endif

// customize difinition
#define ENABLE_WKC_HTTPCACHE_EXCLUDE_ROOT_CONTENT 1 // exclude https root content from HTTP cache.

using namespace std;

namespace WebCore {

const double pollTimeSeconds = 0.001;

#define HTTP_DEFAULT_MAX_CONNECTIONS   4L
#define HTTP_RESERVED_CONNECTIONS      2L /* Synchronous XHR + OCSP */
#define WEBSOCKET_MAX_CONNECTIONS      4L
#define WEBSOCKET_RESERVED_CONNECTIONS 1L /* WebInspector */
#define DEFAULT_TCP_MAX_CONNECTIONS   (HTTP_DEFAULT_MAX_CONNECTIONS + HTTP_RESERVED_CONNECTIONS + WEBSOCKET_MAX_CONNECTIONS + WEBSOCKET_RESERVED_CONNECTIONS)
#define DEFAUILT_MAX_HOST_CONNECTIONS  6L
#define HTTP_DEFAULT_MAX_PIPELINE_CONNECTIONS   100L

static WKC::WillAcceptCookieProc gWillAcceptCookieProc;

//
// Mutex
//
WKC_DEFINE_GLOBAL_TYPE_ZERO(void*, gShareMutex);
WKC_DEFINE_GLOBAL_TYPE_ZERO(void*, gCookieMutex);
WKC_DEFINE_GLOBAL_TYPE_ZERO(void*, gDnsMutex);
WKC_DEFINE_GLOBAL_TYPE_ZERO(void*, gSSLSessionMutex);
WKC_DEFINE_GLOBAL_TYPE_ZERO(void*, gConnectMutex);

enum {
    RHM_HEADERCALLBACK_DATA,
    RHM_WRITECALLBACK_DATA
};

static void sharedResourceMutexFinalize(void)
{
    if (gShareMutex) {
        wkcMutexDeletePeer(gShareMutex);
        gShareMutex = 0;
    }
    if (gCookieMutex) {
        wkcMutexDeletePeer(gCookieMutex);
        gCookieMutex = 0;
    }
    if (gDnsMutex) {
        wkcMutexDeletePeer(gDnsMutex);
        gDnsMutex = 0;
    }
    if (gSSLSessionMutex) {
        wkcMutexDeletePeer(gSSLSessionMutex);
        gSSLSessionMutex = 0;
    }
    if (gConnectMutex) {
        wkcMutexDeletePeer(gConnectMutex);
        gConnectMutex = 0;
    }
}

static void sharedResourceMutexInitialize(void)
{
    sharedResourceMutexFinalize();
    gShareMutex       = wkcMutexNewPeer();
    gCookieMutex      = wkcMutexNewPeer();
    gDnsMutex         = wkcMutexNewPeer();
    gSSLSessionMutex  = wkcMutexNewPeer();
    gConnectMutex     = wkcMutexNewPeer();
}

static void* sharedResourceMutex(curl_lock_data data)
{
    switch (data) {
    case CURL_LOCK_DATA_SHARE:
        return gShareMutex;
    case CURL_LOCK_DATA_COOKIE:
        return gCookieMutex;
    case CURL_LOCK_DATA_DNS:
        return gDnsMutex;
    case CURL_LOCK_DATA_SSL_SESSION:
        return gSSLSessionMutex;
    case CURL_LOCK_DATA_CONNECT:
        return gConnectMutex;
    default:
        ASSERT_NOT_REACHED();
        return NULL;
    }
}

// libcurl does not implement its own thread synchronization primitives.
// these two functions provide mutexes for cookies, and for the global DNS
// cache.
static void curl_lock_callback(CURL* handle, curl_lock_data data, curl_lock_access access, void* userPtr)
{
    if (void* mutex = sharedResourceMutex(data))
        wkcMutexLockPeer(mutex);
}

static void curl_unlock_callback(CURL* handle, curl_lock_data data, void* userPtr)
{
    if (void* mutex = sharedResourceMutex(data))
        wkcMutexUnlockPeer(mutex);
}

//
// Utilities
//
static char *fastStrdup(const char *str)
{
    char *new_str;
    size_t size;

    if (!str) {
        return NULL;
    }
    size = strlen(str) + 1;

    new_str = (char *)fastMalloc(size);
    if (!new_str) {
        return NULL;
    }
    memcpy(new_str, str, size);

    return new_str;
}

static String hostAndPort(const URL& kurl)
{
    String url;

    if (auto port = kurl.port()) {
        char port_buf[8];
        snprintf(port_buf, 7, "%d", port.value());
        url = kurl.host().toString() + ":" + port_buf;
    }
    else
        url = kurl.host().toString();

    return url;
}

WKC::FrameLoaderClientWKC* frameloaderclientwkc(ResourceHandle* job)
{
    if (!job || !job->frameloaderclient())
        return 0;

    if (!job || !job->getInternal() || job->getInternal()->m_cancelled)
        return 0;

    if (!job->mainFrame() || !job->mainFrame()->view() || !job->mainFrame()->page() || !job->frame() || !job->frame()->page() || !job->frame()->document())
        return 0;

    WebCore::FrameLoaderClient* flcwkc = (WebCore::FrameLoaderClient *)job->frameloaderclient();
    if (!flcwkc || !flcwkc->byWKC())
        return 0;

    return (WKC::FrameLoaderClientWKC*)(flcwkc);
}


static long ProtectionSpaceAuthSchemeTocURLType(ProtectionSpaceAuthenticationScheme scheme)
{
    switch (scheme) {
    case ProtectionSpaceAuthenticationSchemeDefault:
        return CURLAUTH_ANY;
    case ProtectionSpaceAuthenticationSchemeHTTPBasic:
        return CURLAUTH_BASIC;
    case ProtectionSpaceAuthenticationSchemeHTTPDigest:
        return CURLAUTH_DIGEST;
    case ProtectionSpaceAuthenticationSchemeHTMLForm:
        return CURLAUTH_NONE;
    case ProtectionSpaceAuthenticationSchemeNTLM:
        return CURLAUTH_NTLM;
    case ProtectionSpaceAuthenticationSchemeNegotiate:
        return CURLAUTH_GSSNEGOTIATE;
    case ProtectionSpaceAuthenticationSchemeClientCertificateRequested:
    case ProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested:
    case ProtectionSpaceAuthenticationSchemeUnknown:
    default:
        return CURLAUTH_NONE;
    }
}

//
//  Constructor and Destructor
//
ResourceHandleManager::ResourceHandleManager()
    : m_downloadTimer(*this, &ResourceHandleManager::downloadTimerCallback)
    , m_rhmssl(0)
    , m_proxyCredential()
    , m_proxyAuthScheme(ProtectionSpaceAuthenticationSchemeDefault)
    , m_cookiesDeleting(false)
    , m_cookieMaxEntries(20) /* default */
    , m_tryProxySingleConnect(false)
#if ENABLE(WKC_JAVASCRIPT_SEQUENTIAL)
    , m_jsSequential(false)
#endif // WKC_JAVASCRIPT_SEQUENTIAL
    , m_httpConnections(HTTP_DEFAULT_MAX_CONNECTIONS)
    , m_httpPipelineRequests(HTTP_DEFAULT_MAX_PIPELINE_CONNECTIONS)
    , m_maxWebSocketConnections(WEBSOCKET_MAX_CONNECTIONS)
    , m_harmfulSiteFilter(false)
    , m_redirectInWKC(false)
    , m_useHTTP2(true)
    , m_DNSCacheTimeout(60 * 5) // 5 minutes
    , m_serverResponseTimeout(300)
    , m_connectTimeout(20)
    , m_acceptEncoding()
    , m_DoNotTrack(false)
#if ENABLE(WKC_HTTPCACHE)
    , m_readCacheTimer(*this, &ResourceHandleManager::readCacheTimerCallback)
    , m_writeCacheTimer(*this, &ResourceHandleManager::writeCacheTimerCallback)
#endif
    , m_curlHandlesInMulti(0)
    , m_threadRunning(false)
    , m_threadWillSuspended(false)
    , m_threadSuspended(false)
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    curl_global_init(CURL_GLOBAL_ALL);
    m_curlMultiHandle = curl_multi_init();
    curl_multi_setopt(m_curlMultiHandle, CURLMOPT_MAX_TOTAL_CONNECTIONS, m_httpConnections);

    sharedResourceMutexInitialize();

    m_curlShareHandle = curl_share_init();
    if (!m_curlShareHandle) {
        wkcMemoryNotifyNoMemoryPeer(sizeof(void*));
        return;
    }
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_LOCKFUNC, curl_lock_callback);
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_UNLOCKFUNC, curl_unlock_callback);

    /* customized option */
    curl_share_setopt(m_curlShareHandle, CURLSHOPT_COOKIE_MAX_ENTRIES, m_cookieMaxEntries);

    /* for dispatchSynchronousJob() */
    m_curlMultiSyncHandle = curl_multi_init();
    curl_multi_setopt(m_curlMultiSyncHandle, CURLMOPT_MAX_TOTAL_CONNECTIONS, 1L);

    m_mainThreadId = wkcThreadCurrentThreadPeer();
    LOG_FUNCTION(ResourceHandleManager, "m_mainThreadId:%p", m_mainThreadId);

    LOG_FUNCTION(ResourceHandleManager, "out");
}

ResourceHandleManager::~ResourceHandleManager()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    threadTerminate();
    removeAllRunningJobs();
#if ENABLE(WKC_HTTPCACHE)
    resetHTTPCache();
#endif

    delete m_rhmssl;
    m_rhmssl = 0;

    curl_multi_cleanup(m_curlMultiHandle);
    curl_share_cleanup(m_curlShareHandle);
    curl_multi_cleanup(m_curlMultiSyncHandle);
    sharedResourceMutexFinalize();
    curl_global_cleanup();

    LOG_FUNCTION(ResourceHandleManager, "out");
}

ResourceHandleManager* ResourceHandleManager::create()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    ResourceHandleManager* self;

    self = new ResourceHandleManager();
    if (!self)
        goto exit_func;

    if (!self->construct()) {
        delete self;
        self = 0;
        goto exit_func;
    }

exit_func:
    LOG_FUNCTION(ResourceHandleManager, "return=%p", self);
    return self;
}

static void*
curl_handle_thread(void* data)
{
    ResourceHandleManager* self = static_cast<ResourceHandleManager*>(data);

    self->networkThread();

    return ((void*)0);
}

bool ResourceHandleManager::construct()
{
    m_rhmssl = ResourceHandleManagerSSL::create(this, m_curlMultiHandle, m_curlShareHandle);
    if (!m_rhmssl)
        return false;

    m_threadID = wkcThreadCreatePeer(curl_handle_thread, (void*)this, "WKC: NetworkThread");
    if (!m_threadID) {
        return false;
    }

    return true;
}

// Singleton
WKC_DEFINE_GLOBAL_CLASS_OBJ_ZERO(ResourceHandleManager*, ResourceHandleManager, m_sharedInstance);

bool ResourceHandleManager::createSharedInstance()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    if (m_sharedInstance)
        return true;

    m_sharedInstance = create();
    if (!m_sharedInstance)
        return false;

    LOG_FUNCTION(ResourceHandleManager, "out");
    return true;
}

ResourceHandleManager* ResourceHandleManager::sharedInstance()
{
    return m_sharedInstance;
}

void ResourceHandleManager::deleteSharedInstance()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    if (m_sharedInstance)
        delete m_sharedInstance;

    m_sharedInstance = 0;

    LOG_FUNCTION(ResourceHandleManager, "out");
}

bool ResourceHandleManager::isExistSharedInstance()
{
    return m_sharedInstance ? true : false;
}

//
// Force Terminate
//
void ResourceHandleManager::forceTerminateInstance()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    ResourceHandleManager* mgr = ResourceHandleManager::sharedInstance();
    if (mgr)
        mgr->forceTerminate();

    ResourceRequestBase::forceTerminate();

    LOG_FUNCTION(ResourceHandleManager, "out");
}

void ResourceHandleManager::forceTerminate()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    threadTerminate();
    ResourceHandleManagerSSL::resetVariables();

#if ENABLE(WKC_HTTPCACHE)
    // should not call the function because they call delete / deref and memory related codes...
//    resetHTTPCache();
#endif

    m_curlMultiHandle = 0;
    m_curlShareHandle = 0;
    m_curlMultiSyncHandle = 0;
    m_mainThreadId = 0;
    sharedResourceMutexFinalize();
    curl_global_reset_variables();

    m_sharedInstance = 0;

    LOG_FUNCTION(ResourceHandleManager, "out");
}

//
// settings
//
void ResourceHandleManager::setMaxHTTPConnections(long number)
{
/* dispatchSynchronousJob() reserves one connection  */
/* OCSP reserves one connection  */

    if (number < 0)
        return;

    if (0 == number) {
        /* infinity */
        m_httpConnections = number;
        m_maxWebSocketConnections = INT_MAX;
    }
    else if (HTTP_RESERVED_CONNECTIONS < number) {
        m_httpConnections = number - HTTP_RESERVED_CONNECTIONS;
    }
    else
        return;

    callOnNetworkThread([this]() {
        curl_multi_setopt(m_curlMultiHandle, CURLMOPT_MAX_TOTAL_CONNECTIONS, m_httpConnections);
        curl_multi_setopt(m_curlMultiHandle, CURLMOPT_MAX_HOST_CONNECTIONS, DEFAUILT_MAX_HOST_CONNECTIONS);
    });
}

void ResourceHandleManager::setMaxHTTPPipelineRequests(long number)
{
    if (number < 0)
        return;
    if (0 == number) {
        /* infinity */
        m_httpPipelineRequests = INT_MAX;
    } else
        m_httpPipelineRequests = number;

    callOnNetworkThread([this]() {
        curl_multi_setopt(m_curlMultiHandle, CURLMOPT_MAX_PIPELINE_LENGTH, m_httpPipelineRequests);
    });
}

void ResourceHandleManager::setMaxWebSocketConnections(long number)
{
/* WebInspector reserves one connection  */

    if (number < 0)
        return;

    if (0 == number) {
        /* infinity */
        m_maxWebSocketConnections = INT_MAX;
    }
    else if (WEBSOCKET_RESERVED_CONNECTIONS < number) {
        m_maxWebSocketConnections = number - WEBSOCKET_RESERVED_CONNECTIONS;
    }

}

void ResourceHandleManager::setMaxCookieEntries(long number)
{
    if (0 < number) {
        m_cookieMaxEntries = number;
        curl_share_setopt(m_curlShareHandle, CURLSHOPT_COOKIE_MAX_ENTRIES, number);
    }
}

void ResourceHandleManager::setHarmfulSiteFilter(bool enable)
{
    m_harmfulSiteFilter = enable;
}

void ResourceHandleManager::setRedirectInWKC(bool enable)
{
    m_redirectInWKC = enable;
}

// from nsHttpConnection.cpp.
static const char* cBlackListedServer[] =
{
    "EFAServer/",
    "Microsoft-IIS/4.",
    "Microsoft-IIS/5.",
    "Netscape-Enterprise/3.",
    "Netscape-Enterprise/4.",
    "Netscape-Enterprise/5.",
    "Netscape-Enterprise/6.",
    "WebLogic 3.",
    "WebLogic 4.",
    "WebLogic 5.",
    "WebLogic 6.",
    NULL
};

static int _serverPushCallback(CURL *parent, CURL *easy, size_t num_headers, struct curl_pushheaders *headers, void *userp);

void ResourceHandleManager::setUseHTTP2(bool enable)
{
    m_useHTTP2 = enable;
    callOnNetworkThread([this]() {
        if (m_useHTTP2) {
            curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PIPELINING, (long)CURLPIPE_MULTIPLEX);
            curl_multi_setopt(m_curlMultiSyncHandle, CURLMOPT_PIPELINING, (long)CURLPIPE_MULTIPLEX);
            curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PIPELINING_SERVER_BL, cBlackListedServer);
            curl_multi_setopt(m_curlMultiSyncHandle, CURLMOPT_PIPELINING_SERVER_BL, cBlackListedServer);
#if 0
            if (!m_httpCache.disabled()) {
                /* for server_push */
                curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PUSHFUNCTION, _serverPushCallback);
                curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PUSHDATA, this);
            }
#endif
        }
        else {
            curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PIPELINING, (long)CURLPIPE_NOTHING);
            curl_multi_setopt(m_curlMultiSyncHandle, CURLMOPT_PIPELINING, (long)CURLPIPE_NOTHING);
            if (!m_httpCache.disabled()) {
                /* for server_push */
                curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PUSHFUNCTION, nullptr);
                curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PUSHDATA, nullptr);
            }
        }
    });
}

void ResourceHandleManager::setDNSCacheTimeout(int sec)
{
    m_DNSCacheTimeout = sec;
}

void ResourceHandleManager::setServerResponseTimeout(int sec)
{
    m_serverResponseTimeout = sec;
}

void ResourceHandleManager::setConnectTimeout(int sec)
{
    m_connectTimeout = sec;
}

void ResourceHandleManager::setAcceptEncoding(const char* encodings)
{
    m_acceptEncoding = String::fromUTF8(encodings);
}

//
// handling HTTP
//
static String guessMIMETypeByURL(const URL& kurl, long httpCode)
{
    if (httpCode == 200) {
        String byPath = MIMETypeRegistry::getMIMETypeForPath(kurl.lastPathComponent());
        if (byPath == "application/octet-stream") {
            if (!kurl.path().length())
                return "text/html";
            if (kurl.lastPathComponent().isEmpty())
                return "text/html";
            return byPath;
        }
        else
            return byPath;
    }
    else if (httpCode < 200 || httpCode == 204 || httpCode == 205 ||httpCode == 304) {
        // These response code MUST NOT has response body.
        return "";
    }
    else {
        // Other status code like 301,401,407 may return with text html body.
        return "text/html";
    }
}

static String guessMIMETypeByContent(unsigned char* data, int len)
{
    return wkcGuessMIMETypeByContentPeer((const unsigned char*)data, len);
}

static void handleLocalReceiveResponse(CURL* handle, ResourceHandle* job, ResourceHandleInternal* d)
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    // since the code in headerCallback will not have run for local files
    // the code to set the URL and fire didReceiveResponseAsync is never run,
    // which means the ResourceLoader's response does not contain the URL.
    // Run the code here for local files to resolve the issue.
    // TODO: See if there is a better approach for handling this.
    const char* hdr;
    CURLcode err = curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &hdr);
    ASSERT(CURLE_OK == err);
    (void)err;
    d->m_response.setURL(URL(ParsedURLString, hdr));

    long httpCode = 0;
    err = curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &httpCode);
    ASSERT(CURLE_OK == err);

    if (d->m_response.mimeType().isEmpty()) {
        d->m_response.setMimeType(extractMIMETypeFromMediaType(d->m_response.httpHeaderField(String("Content-Type"))).convertToASCIILowercase());
        if (d->m_response.mimeType().isEmpty()) {
            d->m_response.setMimeType(guessMIMETypeByURL(d->m_response.url(), httpCode));
        }
    }

    if (d->client())
        d->client()->didReceiveResponseAsync(job, ResourceResponse(d->m_response), [](){});
    if (d->m_cancelled)
        return;

    d->m_response.setResponseFired(true);

    LOG_FUNCTION(ResourceHandleManager, "out");
}

/*
 * This is being called for HTTP body in the response. 
 *
 * called with data after all headers have been processed via headerCallback
 */
static size_t writeCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
    LOG_FUNCTION(ResourceHandleManager, "ptr:%p size:%d nmemb:%d data(=job):%p", ptr, size, nmemb, data);

    ResourceHandle* job = static_cast<ResourceHandle*>(data);
    ResourceHandleInternal* d = job->getInternal();
    unsigned char* ptrData = static_cast<unsigned char*>(ptr);

    size_t totalSize = size*nmemb;
    size_t dataSize = totalSize;
    unsigned length = static_cast<unsigned>(totalSize);

    if (d->m_cancelled) {
        LOG_FUNCTION(ResourceHandleManager, "canceled");
        return 0;
    }

    RefPtr<Uint8Array> writeData = Uint8Array::create(length+1);
    if (!writeData) {
        LOG_FUNCTION(ResourceHandleManager, "no memory");
        return 0;
    }
    memcpy(writeData->data(), ptrData, length);
    writeData->set(length, RHM_WRITECALLBACK_DATA);

    wkcMutexLockPeer(d->m_curlDataMutex);
    d->m_curlData.append(writeData);
    wkcMutexUnlockPeer(d->m_curlDataMutex);

    LOG_FUNCTION(ResourceHandleManager, "out");
    return totalSize;
}

static int xferinfoCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    ResourceHandle* job = static_cast<ResourceHandle*>(clientp);
    ResourceHandleInternal* d = job->getInternal();

    if (ulnow > d->m_bytesSent) {
        wkcMutexLockPeer(d->m_curlDataMutex);
        d->m_datasent = true;
        d->m_bytesSent = static_cast<unsigned long long>(ulnow);
        d->m_totalBytesToBeSent = static_cast<unsigned long long>(ultotal);
        wkcMutexUnlockPeer(d->m_curlDataMutex);
    }

    return 0;
}

size_t ResourceHandleManager::downloadTimerWriteCallback(ResourceHandle* job, ResourceHandleInternal* d, const unsigned char* ptrData, size_t totalSize)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p totalSize:%d", job, totalSize);

    size_t dataSize = totalSize;

#if LIBCURL_VERSION_NUM > 0x071200
    // We should never be called when deferred loading is activated.
    ASSERT(!d->m_defersLoading);
#endif
    if (d->m_cancelled) {
        return 0;
    }

    // this shouldn't be necessary but apparently is. CURL writes the data
    // of html page even if it is a redirect that was handled internally
    // can be observed e.g. on gmail.com
    CURL* h = d->m_handle;
    long httpCode = 0;
    CURLcode err = curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &httpCode);
    ASSERT(CURLE_OK == err);
    (void)err;
    if (100 <= httpCode && httpCode < 200) {
        LOG_FUNCTION(ResourceHandleManager, "100 <= httpCode < 200");
        return 0;
    }
    else if (300 <= httpCode && httpCode < 400) {
#if 0
        String location = d->m_response.httpHeaderField(String("location"));
        if (location.isEmpty()) {
            LOG_FUNCTION(ResourceHandleManager, "redirect httpCode:%d but location is empty", httpCode);
            return 0;
        }
        LOG_FUNCTION(ResourceHandleManager, "Redirect");
        return totalSize;
#else
        if (d->m_redirectCount > d->m_redirectMaxCount)
            return totalSize;
        if (d->m_doRedirectChallenge)
            return totalSize;
        if (!ptrData) {
            return 0;
        }
        // would be HTTP/0.9...
        if (!d->m_response.responseFired()) {
            d->m_response.setHTTPVersion(String("HTTP/0.9"));
            d->m_response.setHTTPStatusCode(200);
            d->m_response.setURL(d->m_firstRequest.url());
            d->m_response.setExpectedContentLength(0);
            d->m_response.setTextEncodingName(String());
            d->m_response.setSuggestedFilename(String());
            d->m_response.setMimeType(guessMIMETypeByContent((unsigned char *)ptrData, totalSize));

            if (d->client())
                d->client()->didReceiveResponseAsync(job, ResourceResponse(d->m_response), [](){});
            if (d->m_cancelled)
                return 0;

            d->m_response.setResponseFired(true);
        }
#endif
    }
    else if (401 == httpCode || 407 == httpCode) {
        d->m_receivedData->append((const char*)ptrData, totalSize);
        LOG_FUNCTION(ResourceHandleManager, "Authenticate httpCode:%d", httpCode);
        return totalSize;
    }

    if (!d->m_response.responseFired()) {
        if (d->m_response.mimeType().isEmpty()) {
            int len = (dataSize < (DATA_KEEP_LENGTH - d->m_recvDataLength)) ? dataSize : (DATA_KEEP_LENGTH - d->m_recvDataLength);
            memcpy(d->m_recvData + d->m_recvDataLength, ptrData, len);
            d->m_recvDataLength += len;
            String MIMEType = guessMIMETypeByContent(d->m_recvData, d->m_recvDataLength);
            if (MIMEType.isEmpty()) {
                if (d->m_recvDataLength < DATA_KEEP_LENGTH && d->m_recvDataLength < d->m_response.expectedContentLength()) {
                    return totalSize;
                }
                else
                    MIMEType = "application/octet-stream";
            }
            d->m_response.setMimeType(MIMEType);
            ptrData += len;
            dataSize -= len;
        }
        handleLocalReceiveResponse(h, job, d);
        if (d->m_cancelled)
            return 0;

        if (d->m_recvDataLength) {
            WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(job);
            if (fl &&!fl->dispatchWillReceiveData(job, d->m_recvDataLength)) {
                return 0;
            }
            if (d->client() && !d->client()->willReceiveData(job, d->m_recvDataLength)) {
                return 0;
            }
            if (d->client())
                d->client()->didReceiveData(job, (const char *)d->m_recvData, d->m_recvDataLength, 0);
            if (d->m_cancelled)
                return 0;
            d->m_recvDataLength = 0;
        }
    }

    WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(job);
    if (dataSize && fl && !fl->dispatchWillReceiveData(job, dataSize)) {
        return 0;
    }

    if (d->client() && !d->client()->willReceiveData(job, dataSize)) {
        return 0;
    }

    if (dataSize && d->client())
        d->client()->didReceiveData(job, (const char *)ptrData, dataSize, 0);
    if (d->m_cancelled)
        return 0;

    LOG_FUNCTION(ResourceHandleManager, "out");
    return totalSize;
}

static bool isAppendableHeader(const String &key)
{
    static const char* appendableHeaders[] = {
        "access-control-allow-headers",
        "access-control-allow-methods",
        "access-control-allow-origin",
        "access-control-expose-headers",
        "allow",
        "cache-control",
        "connection",
        "content-encoding",
        "content-language",
        "if-match",
        "if-none-match",
        "keep-alive",
        "pragma",
        "proxy-authenticate",
        "public",
        "server",
        "te",
        "trailer",
        "transfer-encoding",
        "upgrade",
        "user-agent",
        "vary",
        "via",
        "warning",
        "www-authenticate",
        0
    };

    // Custom headers start with 'X-', and need no further checking.
    if (key.startsWithIgnoringASCIICase("x-")) {
        return true;
    }

    for (unsigned i = 0; appendableHeaders[i]; ++i) {
        if (equalIgnoringASCIICase(key, appendableHeaders[i])) {
            return true;
        }
    }

    return false;
}

static void
setResourceLoadTiming(ResourceHandle* job)
{
    ResourceHandleInternal* d = job->getInternal();
    if (!d)
        return;
    CURL* h = d->m_handle;
    if (!h)
        return;
    Frame* frame = (Frame *)job->frame();
    if (!frame || !frame->loader().documentLoader())
        return;
    NetworkLoadMetrics& t = frame->loader().documentLoader()->response().deprecatedNetworkLoadMetrics();

    double dnslookupTime = 0;
    double connectTime = 0;
    double appConnectTime = 0;
    double startTransfertTime = 0;
    double preTransferTime = 0;
    
    curl_easy_getinfo(h, CURLINFO_NAMELOOKUP_TIME, &dnslookupTime);
    curl_easy_getinfo(h, CURLINFO_CONNECT_TIME, &connectTime);
    curl_easy_getinfo(h, CURLINFO_APPCONNECT_TIME, &appConnectTime);
    curl_easy_getinfo(h, CURLINFO_STARTTRANSFER_TIME, &startTransfertTime);
    curl_easy_getinfo(h, CURLINFO_PRETRANSFER_TIME, &preTransferTime);

    t.domainLookupStart = Seconds(0);
    t.domainLookupEnd = Seconds(dnslookupTime);
    t.connectStart = Seconds(dnslookupTime);
    t.connectEnd = Seconds(connectTime);
    t.requestStart = Seconds(connectTime);
    if (d->m_isSSL) {
        t.secureConnectionStart = Seconds(connectTime);
        t.responseStart = Seconds(appConnectTime);
    } else {
        t.responseStart = Seconds(preTransferTime);
    }
}

static size_t headerCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
    ResourceHandle* job = static_cast<ResourceHandle*>(data);
    ResourceHandleInternal* d = job->getInternal();
    if (d->m_cancelled)
        return 0;

    unsigned char* ptrData = static_cast<unsigned char*>(ptr);
    size_t totalSize = size * nmemb;
    unsigned length = static_cast<unsigned>(totalSize);
    const unsigned char* p = reinterpret_cast<const unsigned char*>(ptr);

    if (strncmp((char*)ptrData, "\r\n", 2) == 0) {
        long httpCode = 0;
        (void)curl_easy_getinfo(d->m_handle, CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode == 0) {
            d->m_blankLinesFromProxy++;
        }
    }

    RefPtr<Uint8Array> writeData = Uint8Array::create(length+1);
    if (!writeData) {
        LOG_FUNCTION(ResourceHandleManager, "no memory");
        return 0;
    }
    memcpy(writeData->data(), ptrData, length);
    writeData->set(length, RHM_HEADERCALLBACK_DATA);

    wkcMutexLockPeer(d->m_curlDataMutex);
    d->m_curlData.append(writeData);
    wkcMutexUnlockPeer(d->m_curlDataMutex);

    return totalSize;
}

size_t ResourceHandleManager::downloadTimerHeaderCallback(ResourceHandle* job, ResourceHandleInternal* d, const unsigned char* p, size_t totalSize)
{
#if LIBCURL_VERSION_NUM > 0x071200
    // We should never be called when deferred loading is activated.
    ASSERT(!d->m_defersLoading);
#endif
    if (d->m_cancelled) {
        return 0;
    }

    WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(job);
    if (fl && !fl->dispatchWillReceiveData(job, totalSize)) {
        LOG_FUNCTION(ResourceHandleManager, "can not receive header");
        return 0;
    }

    if (d->client() && !d->client()->willReceiveData(job, totalSize)) {
        return 0;
    }

    String header(p, totalSize);

    /*
     * a) We can finish and send the ResourceResponse
     * b) We will add the current header to the HTTPHeaderMap of the ResourceResponse
     *
     * The HTTP standard requires to use \r\n but for compatibility it recommends to
     * accept also \n.
     */
    if (header == String("\r\n") || header == String("\n")) {
        LOG_FUNCTION(ResourceHandleManager, "HTTP Header Received: job:%p d:%p p:%p totalSize:%d", job, d, p, totalSize);

        CURL* h = d->m_handle;

        const char* hdr;
        (void)curl_easy_getinfo(h, CURLINFO_EFFECTIVE_URL, &hdr);
        d->m_response.setURL(URL(ParsedURLString, hdr));

        long httpCode = 0;
        (void)curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &httpCode);

        if (d->m_blankLinesFromProxy > 0) {
            d->m_blankLinesFromProxy--;
            httpCode = 0;
        }
        // SSL
        if (0 == httpCode && d->m_isSSL) {
            long connectCode = 0;
            (void)curl_easy_getinfo(h, CURLINFO_HTTP_CONNECTCODE, &connectCode);
            if (connectCode == 200) {
                LOG_FUNCTION(ResourceHandleManager, "SSL/TLS && connectCode is 200");
                d->m_response.setHTTPStatusCode(0);
                return totalSize;
            }
            else if (connectCode == 407) {
                /* should callback like didReceiveAuthenticationChallenge() for Proxy Authentication */;
                /* If httpCode is 407 then it will calls didReceiveAuthenticationChallenge(). */;
                httpCode = 407;
            }
            else if (connectCode >= 300 && connectCode < 400) {
                /* Proxy's redirecting.... Is it avaiable??? */;
                /* should do something else */;
            }
            else {
                /* should do something else */;
                httpCode = connectCode;
            }
        }
        d->m_response.setHTTPStatusCode(httpCode);

        // 100 continue
        if (100 == httpCode) {
            LOG_FUNCTION(ResourceHandleManager, "httpCode is 100");
            return totalSize;
        }

        double contentLength = 0;
        (void)curl_easy_getinfo(h, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &contentLength);
        d->m_response.setExpectedContentLength(static_cast<long long int>(contentLength));

        d->m_response.setMimeType(extractMIMETypeFromMediaType(d->m_response.httpHeaderField(String("Content-Type"))).convertToASCIILowercase());
        d->m_response.setTextEncodingName(extractCharsetFromMediaType(d->m_response.httpHeaderField(String("Content-Type"))));
        d->m_response.setSuggestedFilename(filenameFromHTTPContentDisposition(d->m_response.httpHeaderField(String("Content-Disposition"))));

        if (d->m_response.mimeType().isEmpty()) {
            d->m_response.setMimeType(guessMIMETypeByURL(d->m_response.url(), httpCode));
        }

        if ((401 == httpCode || 407 == httpCode) && d->m_response.isHTTP()) {
            LOG_FUNCTION(ResourceHandleManager, "httpCode is %d", httpCode);
            return totalSize;
        }

        if (d->client() && !d->client()->validateCrossOriginResourcePolicy(d->m_response))
            return totalSize;

        // HTTP redirection
        if (300 <= httpCode && httpCode < 400) {
            String location = d->m_response.httpHeaderField(String("location"));
            if (!location.isEmpty()) {
                if (!didReceiveRedirect(job, httpCode, WTFMove(location))) {
                    return 0;
                }
                LOG_FUNCTION(ResourceHandleManager, "redirect");
                return totalSize;
            }
        }

#if ENABLE(WKC_HTTPCACHE)
        if (httpCode == 304) {
            // We must not call didReceiveResponseAsync() when cache is alive, it must be called cache loading.
            return totalSize;
        }
#endif

        if (!d->m_response.mimeType().isEmpty()) {
            if (d->client())
                d->client()->didReceiveResponseAsync(job, ResourceResponse(d->m_response), [](){});
            if (d->m_cancelled) {
                LOG_FUNCTION(ResourceHandleManager, "canceled");
                return 0;
            }

            d->m_response.setResponseFired(true);
        }
    }
    else {
        LOG_FUNCTION(ResourceHandleManager, "HTTP Header Received: job:%p d:%p p:%p totalSize:%d", job, d, p, totalSize);

        size_t splitPos = header.find(":");
        if (splitPos != notFound) {
            String name = header.left(splitPos);
            String value = header.substring(splitPos+1).stripWhiteSpace();

            if (isAppendableHeader(name)) {
                d->m_response.addHTTPHeaderField(name, value);
            }
            else {
                d->m_response.setHTTPHeaderField(name, value);
            }
        }
        /* find status text and set it */
        else if (header.findIgnoringASCIICase("HTTP/", 0) != notFound) {
            WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(job);
            long httpcode = 0;
            CURL* h = d->m_handle;
            int ret = curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &httpcode);

            if (d->m_isSSL && fl) {
                long connectCode = 0;
                (void)curl_easy_getinfo(h, CURLINFO_HTTP_CONNECTCODE, &connectCode);
                if (httpcode
                    || (connectCode != 0 && connectCode != 407 && header.findIgnoringASCIICase("Connection established", 0) == notFound)) {
                    rhmssl()->SSLHandshakeInfo(job);
                    fl->notifySSLHandshakeStatus(job, WKC::EHandshakeSuccess);
                    d->m_SSLHandshaked = true;
                }
            }

            if (ret == CURLE_OK) {
                char buf[4];
                size_t pos;
                int len;
                len = snprintf(buf, 3, "%ld", httpcode);
                if (len > 0) {
                    buf[len] = '\0';
                    pos = header.find(buf);
                    if (pos != notFound) {
                        pos += len;
                        d->m_response.setHTTPStatusText(header.substring(pos).stripWhiteSpace());
                    }
                }
            }
        }
    }

    LOG_FUNCTION(ResourceHandleManager, "out");
    return totalSize;
}

/* This is called to obtain HTTP POST, PUT or OPTIONS data.
   Iterate through FormData elements and upload files.
   Carefully respect the given buffer size and fill the rest of the data at the next calls.
*/
static size_t readCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
    LOG_FUNCTION(ResourceHandleManager, "ptr:%p size:%d nmemb:%d data(=job):%p", ptr, size, nmemb, data);

    ResourceHandle* job = static_cast<ResourceHandle*>(data);
    ResourceHandleInternal* d = job->getInternal();

    if (d->m_cancelled)
        return 0;

#if LIBCURL_VERSION_NUM > 0x071200
    // We should never be called when deferred loading is activated.
    ASSERT(!d->m_defersLoading);
#endif

    if (!size || !nmemb)
        return 0;
    
    if (!d->m_formDataStream.hasMoreElements()) {
        LOG(FormData, "(d:%p) No more elements.", d);
        return 0;
    }

    size_t sent = d->m_formDataStream.read(ptr, size, nmemb);
    if (sent == 0)
        sent = CURL_READFUNC_NODATA;

    LOG_FUNCTION(ResourceHandleManager, "out");
    return sent;
}

/* This is called to obtain HTTP POST, PUT or OPTIONS data and to receive redirect. */
static curlioerr ioctlCallback(CURL *handle, int cmd, void *clientp)
{
    LOG_FUNCTION(ResourceHandleManager, "handle:%p cmd:%d clientp:%p", handle, cmd, clientp);

    ResourceHandle* job = static_cast<ResourceHandle*>(clientp);
    ResourceHandleInternal* d = job->getInternal();
    if (!d || !d->m_handle)
        return CURLIOE_OK;

    // erase "Content-Type" header
    struct curl_slist *cur_headers = d->m_customHeaders;
    struct curl_slist *new_headers = NULL;
    while (cur_headers) {
        if (cur_headers->data) {
            if (strncasecmp(cur_headers->data, "Content-Type:", 13)) {
                new_headers = curl_slist_append(new_headers, cur_headers->data);
            }
        }
        cur_headers = cur_headers->next;
    }
    if (new_headers) {
        curl_slist_free_all(d->m_customHeaders);
        curl_easy_setopt(d->m_handle, CURLOPT_HTTPHEADER, new_headers);
        d->m_customHeaders = new_headers;
    }

    /* no operation */
    LOG_FUNCTION(ResourceHandleManager, "out");
    return CURLIOE_OK;
}

WKC::CurlDebugProc gCurlDebugProc;

void
ResourceHandleManager::setCurlDebugCallback(WKC::CurlDebugProc proc)
{
    gCurlDebugProc = proc;
}

int debugCallback(CURL *handle, curl_infotype type,  char *data, size_t size, void *userptr)
{
    if (gCurlDebugProc) {
        gCurlDebugProc(handle, type, data, size);
        return 0;
    }

    if (!userptr)
        return 0;

    ResourceHandle* job = static_cast<ResourceHandle*>(userptr);
    ResourceHandleInternal* d = job->getInternal();
    if (!d || !d->m_handle || d->m_handle != handle)
        return 0;

    int len = size;
    char* buff = NULL;

    if (type == CURLINFO_TEXT || type == CURLINFO_HEADER_IN || type == CURLINFO_HEADER_OUT) {
        buff = (char*)fastMalloc(size+1);
        if (!buff)
            return 0;
        memcpy(buff, data, size);
        buff[size] = 0x00;
    }

    switch (type) {
    case CURLINFO_TEXT:
        if (2 < len) {
            if (buff[len-2] == '\r' || buff[len-2] == '\n') {
                buff[len-2] = 0x00;
            }
            else {
                buff[len-1] = 0x00;
            }
            LOG_FUNCTION(ResourceHandleManager, "[CURLINFO_TEXT] %s", buff);
        }
        break;

    case CURLINFO_HEADER_IN:
        if (2 < len) {
            if (buff[len-2] == '\r' || buff[len-2] == '\n') {
                buff[len-2] = 0x00;
            }
            else if (buff[len-1] == '\r' || buff[len-1] == '\n') {
                buff[len-1] = 0x00;
            }
            LOG_FUNCTION(ResourceHandleManager, "[CURLINFO_HEADER_IN](job:%p): %s", job, buff);
        }
        break;

    case CURLINFO_HEADER_OUT:
        if (4 < len) {
            if (buff[len-4] == '\r' || buff[len-4] == '\n') {
                buff[len-4] = 0x00;
            }
            else if (buff[len-3] == '\r' || buff[len-3] == '\n') {
                buff[len-3] = 0x00;
            }
            else if (buff[len-2] == '\r' || buff[len-2] == '\n') {
                buff[len-2] = 0x00;
            }
            else if (buff[len-1] == '\r' || buff[len-1] == '\n') {
                buff[len-1] = 0x00;
            }
            LOG_FUNCTION(ResourceHandleManager, "[CURLINFO_HEADER_OUT](job:%p) %s:\n=====\n%s\n=====", job, d->m_url, buff);
        }
        break;

    case CURLINFO_DATA_IN:
        // data may be binary
        LOG_FUNCTION(ResourceHandleManager, "[CURLINFO_DATA_IN](job:%p)", job);
        break;

    case CURLINFO_DATA_OUT:
        // data may be binary
        LOG_FUNCTION(ResourceHandleManager, "[CURLINFO_DATA_OUT](job:%p)", job);
        break;

    case CURLINFO_SSL_DATA_IN:
        // data is binary
        LOG_FUNCTION(ResourceHandleManager, "[CURLINFO_SSL_DATA_IN](job:%p)", job);
        break;

    case CURLINFO_SSL_DATA_OUT:
        // data is binary
        LOG_FUNCTION(ResourceHandleManager, "[CURLINFO_SSL_DATA_OUT](job:%p)", job);
        break;

    default:
        LOG_FUNCTION(ResourceHandleManager, "curl log: unknown");
        break;
    }

    if (buff)
        fastFree(buff);
    return 0;
}

void ResourceHandleManager::sendDataFromBufferToWebCore(ResourceHandle* job, ResourceHandleInternal* d)
{
    ResourceHandleInternalProtector protector(d);

    if (d->m_cancelled) {
        return;
    }

    wkcMutexLockPeer(d->m_curlDataMutex);
    bool didSendData = d->m_datasent;
    unsigned long long bytesSent = d->m_bytesSent;
    unsigned long long totalBytesToBeSent = d->m_totalBytesToBeSent;
    d->m_datasent = false;
    wkcMutexUnlockPeer(d->m_curlDataMutex);

    if (didSendData) {
        if (d->client()) {
            d->client()->didSendData(job, bytesSent, totalBytesToBeSent);
        }
    }

    while (!d->m_curlData.isEmpty()) {
        wkcMutexLockPeer(d->m_curlDataMutex);
        RefPtr<Uint8Array> dataArray = d->m_curlData.takeFirst();
        wkcMutexUnlockPeer(d->m_curlDataMutex);
        unsigned char* data = dataArray->data();
        unsigned len = dataArray->length();
        unsigned curlDataLen = len - 1;
        unsigned char type = data[len-1];
        size_t written = 0;
        switch(type) {
        case RHM_HEADERCALLBACK_DATA:
            written = ResourceHandleManager::downloadTimerHeaderCallback(job, d, data, curlDataLen);
            break;
        case RHM_WRITECALLBACK_DATA:
            written = ResourceHandleManager::downloadTimerWriteCallback(job, d, data, curlDataLen);
            break;
        default:
            ASSERT(false);
            break;
        }
        if (written == 0 && !d->m_cancelled) {
            cancel(job);
            if (d->client())
                d->client()->didFail(job, ResourceError(String(), CURLE_WRITE_ERROR, URL(ParsedURLString, String(d->m_url)), String("failed writing")));
        }
    }
}

void ResourceHandleManager::_downloadTimerCallback()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    int size = m_runningJobList.size();
    for (int i = 0; i < size; i++) {
        ResourceHandle* job = m_runningJobList[i];
        ResourceHandleInternal* d = job->getInternal();
        if (d->m_cancelled) {
            continue;
        }
        sendDataFromBufferToWebCore(job, d);
    }

    bool loop = true;
    while(loop) {
        loop = false;

        int size = m_runningJobList.size();
        for (int i = 0; i < size; i++) {
            ResourceHandle* job = m_runningJobList[i];
            ResourceHandleInternal* d = job->getInternal();

            if (finalizingResourceHandle(job, d)) {
                loop = true;
                break;
            }
        }
    }

    LOG_FUNCTION(ResourceHandleManager, "out");
}


bool ResourceHandleManager::finalizingResourceHandle(ResourceHandle* job, ResourceHandleInternal* d)
{
    if (d->m_isRunning) {
        return false;
    }

    // eventhough curl handle is removed from multi, buffer has data in some case.
    sendDataFromBufferToWebCore(job, d);

    if (d->m_redirectCount > d->m_redirectMaxCount) {
        char* url = 0;
        curl_easy_getinfo(d->m_handle, CURLINFO_EFFECTIVE_URL, &url);
        if (d->client())
            d->client()->didFail(job, ResourceError(String(), CURLE_TOO_MANY_REDIRECTS, URL(ParsedURLString, String(url)), String(curl_easy_strerror(CURLE_TOO_MANY_REDIRECTS)), ResourceError::Type::General, job));
        return finishLoadingResourceHandle(job, d);
    }

    if (d->m_cancelled || d->m_doRedirectChallenge) {
        return finishLoadingResourceHandle(job, d);
    }

    int httpCode = d->m_response.httpStatusCode();
    if (httpCode == 401 || 407 == httpCode) {
        didReceiveAuthenticationChallenge(job);
        if (d->m_cancelled || d->m_doAuthChallenge) {
            d->m_receivedData->clear();
            return finishLoadingResourceHandle(job, d);
        }
        if (d->m_msgDataResult != CURLE_OK) {
            if (d->client()){
                ResourceError err(String(), d->m_msgDataResult, URL(ParsedURLString, String(d->m_url)), String(curl_easy_strerror((CURLcode)d->m_msgDataResult)), ResourceError::Type::General, job);
                d->client()->didFail(job, err);
            }
            return finishLoadingResourceHandle(job, d);
        }
        if (d->client()) {
            d->client()->didReceiveResponseAsync(job, ResourceResponse(d->m_response), [](){});
            if (d->m_cancelled)
                d->m_client = nullptr;
            d->m_response.setResponseFired(true);
            const Frame* frame = job->frame();
            if (!frame || !frame->loader().documentLoader() || !frame->page() || (!job->mainFrame() && !job->frameloaderclient())) {
                // may be deleted
                d->m_receivedData->clear();
                return finishLoadingResourceHandle(job, d);
            } else {
                if (d->client()) // may be removed at above codes...
                    d->client()->didReceiveData(job, d->m_receivedData->data(), d->m_receivedData->size(), 0);
            }
        }
        d->m_receivedData->clear();
    }

    char* url = 0;
    switch (d->m_msgDataResult) {
    case CURLE_OK:
        if (contentComposition(job)==WKC::ERootFrameRootContentComposition)
            setResourceLoadTiming(job);
        /* Additionally premature termination of HTTP response
           data is treated like a normal termination, and the
           data will be shown to the user. */
        /* fall  through */
    case CURLE_PARTIAL_FILE:
#if ENABLE(WKC_HTTPCACHE)
        if (!(httpCode == 304 && d->m_utilizedHTTPCache) && !d->m_response.responseFired()) {
#else
        if (!d->m_response.responseFired()) {
#endif
            handleLocalReceiveResponse(d->m_handle, job, d);
            if (d->m_cancelled || d->m_doAuthChallenge || d->m_doRedirectChallenge) {
                return finishLoadingResourceHandle(job, d);
            }
        }
        if (d->client()) {
#if ENABLE(WKC_HTTPCACHE)
            if (!m_httpCache.disabled()) {
                URL url = job->firstRequest().url();
                int statusCode = d->m_response.httpStatusCode();
                if (statusCode==0 && d->m_serverpush)
                    statusCode = d->m_response.httpStatusText().toInt();
                switch (statusCode) {
                case 200:
                case 300:
                case 301:
                case 410:
#if ENABLE(WKC_HTTPCACHE_EXCLUDE_ROOT_CONTENT)
                    if (((!job->frame() || !job->frame()->loader().isHostedByObjectElement())) && !(d->m_isSSL && contentComposition(job) == WKC::ERootFrameRootContentComposition)) {
#else
                    if (!job->frame() || !job->frame()->loader().isHostedByObjectElement()) {
#endif
                        if (!(d->m_msgDataResult == CURLE_PARTIAL_FILE && d->client()->shouldFailOnPartialFile())) {
                            addHTTPCache(job, url, d->client()->resourceData(), d->m_response);
                        }
                    }
                    break;
                case 304:
                    {
                        if (!d->m_utilizedHTTPCache)
                            break;  // this case, do nothing since HTTP Cache does not set "If-Modified-Since".
                        if (job->firstRequest().cachePolicy() == ResourceRequestCachePolicy::ReloadIgnoringCacheData)
                            break;  // read from the memory cached resource

                        // read from the disk cached resource
                        HTTPCachedResource *resource = getHTTPCache(url);
                        if (resource)
                            scheduleLoadResourceFromHTTPCache(job);
                        else
                            d->client()->didFail(job, ResourceError(String(), CURLE_READ_ERROR, URL(ParsedURLString, String(d->m_url)), String("cache read error")));
                        return finishLoadingResourceHandle(job, d);
                    }
                default:
                    break;
                }
            }
#endif // ENABLE(WKC_HTTPCACHE)
            if (d->m_msgDataResult == CURLE_PARTIAL_FILE && d->client()->shouldFailOnPartialFile()) {
                d->client()->didFail(job, ResourceError(String(), CURLE_PARTIAL_FILE, URL(ParsedURLString, String(d->m_url)), String("data receiving error")));
            } else {
                d->client()->didFinishLoading(job);
            }
        }
        break;
    case CURLE_OPERATION_TIMEDOUT:
        if (d->m_response.responseFired()) {
            if (d->client())
                d->client()->didFinishLoading(job);
            break;
        }
        // !d->m_response.responseFired() means no receive header
        // so, should didFail(), through down!
    default:
        {
            WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(job);
            if (d->m_isSSL && !d->m_SSLHandshaked && fl) {
                m_rhmssl->SSLHandshakeInfo(job);
                bool isHandshakeError = (d->m_msgDataResult == CURLE_SSL_CACERT) ||
                                        (d->m_msgDataResult == CURLE_PEER_FAILED_VERIFICATION);
                if (isHandshakeError) {
                    if (fl->notifySSLHandshakeStatus(job, WKC::EHandshakeFail)) {
                        // true means try to re-connect even though current SSL is failed
                        d->m_sslReconnect = true;
                        break;
                    }
                }
                d->m_SSLHandshaked = true;
            }
            curl_easy_getinfo(d->m_handle, CURLINFO_EFFECTIVE_URL, &url);
            LOG_FUNCTION(ResourceHandleManager, "Curl ERROR for url='%s', error: '[%d]%s'", url, d->m_msgDataResult, curl_easy_strerror((CURLcode)d->m_msgDataResult));
            if (d->client()) {
                if (d->m_msgDataResult == CURLE_OPERATION_TIMEDOUT)
                    d->client()->didFail(job, ResourceError(String(), d->m_msgDataResult, URL(ParsedURLString, String(url)), String(curl_easy_strerror((CURLcode)d->m_msgDataResult)), ResourceError::Type::Timeout, job));
                else
                    d->client()->didFail(job, ResourceError(String(), d->m_msgDataResult, URL(ParsedURLString, String(url)), String(curl_easy_strerror((CURLcode)d->m_msgDataResult)), ResourceError::Type::General, job));
            }
        }
    }
    return finishLoadingResourceHandle(job, d);
}

void ResourceHandleManager::downloadTimerCallback()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    (void)startScheduledJobs();

    _downloadTimerCallback();

    cancelScheduledJobs();

    bool started = startScheduledJobs(); // new jobs might have been added in the meantime

    LOG_FUNCTION(ResourceHandleManager, "out");
}


static CURLcode cookie_callback(curlcookiedirect direct, CURL *curl, const char *domain, bool tailmatch, void *data)
{
    LOG_FUNCTION(ResourceHandleManager, "(%s, %p, %s, %s, %p)", (direct == CURLCOOKIEDIRECT_RECEIVE)?"recv":"send", curl, domain, (tailmatch)?"True":"False", data);

    ResourceHandle* job = static_cast<ResourceHandle*>(data);
    ResourceHandleInternal* d = job->getInternal();

    bool isIn = (direct == CURLCOOKIEDIRECT_RECEIVE)? true : false;

    if (d->m_cancelled) {
        return CURLE_COOKIE_DENY;
    }

    if (!job->firstRequest().allowCookies()) {
        return CURLE_COOKIE_DENY;
    }

    String firstparty_host = d->m_firstRequest.firstPartyForCookies().host().toString();
    String cookie_domain;
    if (wkcNetCheckCorrectIPAddressPeer(firstparty_host.utf8().data())) {
        if (firstparty_host != domain) {
            return CURLE_COOKIE_DENY;
        }
        cookie_domain = domain;
    } else {
        if (domain && strlen(domain) > 0) {
            cookie_domain = String((tailmatch && domain[0] != '.')?".":"") + domain;
        }
    }

    LOG_FUNCTION(ResourceHandleManager, "out");
    return static_cast<CURLcode>(ResourceHandleManager::sharedInstance()->cookieCallbackInternal(isIn, d->m_url, firstparty_host, cookie_domain));
}

int ResourceHandleManager::cookieCallback(const NetworkStorageSession& session, bool income, const char* url, const String& firstparty_host, const String& cookie_domain)
{
    return cookieCallbackInternal(income, url, firstparty_host, cookie_domain);
}

int ResourceHandleManager::cookieCallbackInternal(bool income, const char* url, const String& firstparty_host, const String& cookie_domain)
{
    if (gWillAcceptCookieProc) {
        if (gWillAcceptCookieProc(income, url, firstparty_host.utf8().data(), cookie_domain.utf8().data()))
            return static_cast<int>(CURLE_COOKIE_ACCEPT);
    }
    return static_cast<int>(CURLE_COOKIE_DENY);
}

void ResourceHandleManager::clearCookies()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    m_cookiesDeleting = true;
    doClearCookies();

    LOG_FUNCTION(ResourceHandleManager, "out");
}

void ResourceHandleManager::doClearCookies()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    if (!m_cookiesDeleting) return;

    if (0 == m_runningJobList.size()) {
        wkcMutexLockPeer(gCookieMutex);

        CURLSHcode shcode = curl_share_setopt(m_curlShareHandle, CURLSHOPT_UNSHARE, CURL_LOCK_DATA_COOKIE);
        if (CURLSHE_OK == shcode) {
            curl_share_setopt(m_curlShareHandle, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);
            curl_share_setopt(m_curlShareHandle, CURLSHOPT_COOKIE_MAX_ENTRIES, m_cookieMaxEntries);
            m_cookiesDeleting = false;
        }

        wkcMutexUnlockPeer(gCookieMutex);
    }

    LOG_FUNCTION(ResourceHandleManager, "out");
}

static WKC::ConnectionFilteringProc gConnectionFilteringProc;

void
ResourceHandleManager::setConnectionFilteringCallback(WKC::ConnectionFilteringProc proc)
{
    gConnectionFilteringProc = proc;
}

// DNS resolution has been done before this is called
bool ResourceHandleManager::filter_callback(CURL *easy, const struct sockaddr_in *resolved_address)
{
    if (resolved_address->sin_family != AF_INET) {
        // only IPv4 is supported
        return true;
    }

    if (gConnectionFilteringProc) {
        return gConnectionFilteringProc(static_cast<wkc_uint32>(resolved_address->sin_addr.s_addr));
    }

    return true;
}

static int _serverPushCallback(CURL *parent, CURL *easy, size_t num_headers, struct curl_pushheaders *headers, void *userp)
{
    LOG_FUNCTION(ResourceHandleManager, "(%d, %p, %p)", num_headers, parent, easy);
    ResourceHandleManager* self = static_cast<ResourceHandleManager*>(userp);
    return self->serverPushCallback(parent, easy, num_headers, headers);
}

class ServerPushResourceHandleClient : public ResourceHandleClient
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    ServerPushResourceHandleClient()
        : ResourceHandleClient()
    {
        m_buffer = SharedBuffer::create();
    }
    virtual ~ServerPushResourceHandleClient()
    {}

    virtual void didReceiveData(ResourceHandle*, const char* data, unsigned len, int /*encodedDataLength*/) override
    {
        m_buffer->append(data, len);
    }

    virtual void willSendRequestAsync(ResourceHandle*, ResourceRequest&& request, ResourceResponse&&, CompletionHandler<void(ResourceRequest&&)>&& completionHandler)
    {
        completionHandler(WTFMove(request));
    }

    virtual void didReceiveResponseAsync(ResourceHandle* handle, ResourceResponse&& response, CompletionHandler<void()>&& completion) override
    {
        handle->getInternal()->m_response = WTFMove(response);
        completion();
    }

    virtual void didFinishLoading(ResourceHandle*) override
    {
    }

    virtual SharedBuffer* resourceData() const override
    {
        return m_buffer.get();
    }

private:
    RefPtr<SharedBuffer> m_buffer;
};

int
ResourceHandleManager::serverPushCallback(CURL *parent, CURL *easy, size_t num_headers, struct curl_pushheaders *headers)
{
#if ENABLE(WKC_HTTPCACHE)
    if (!parent || !easy || !num_headers)
        return CURL_PUSH_DENY;

    ResourceHandle* pjob = 0;
    CURLcode err = curl_easy_getinfo(parent, CURLINFO_PRIVATE, &pjob);
    if (err != CURLE_OK || !pjob)
        return CURL_PUSH_DENY;

    ResourceHandleInternal* pd = pjob->getInternal();

    const char* method = curl_pushheader_byname(headers, ":method");
    const char* scheme = curl_pushheader_byname(headers, ":scheme");
    const char* host = curl_pushheader_byname(headers, "host");
    const char* path = curl_pushheader_byname(headers, ":path");
    if (!host || !path)
        return CURL_PUSH_DENY;
    if (!method)
        method = pd->m_firstRequest.httpMethod().latin1().data();
    if (!scheme)
        scheme = pd->m_firstRequest.url().protocol().toString().latin1().data();

    String url = String::format("%s://%s%s", scheme, host, path);
    URL kurl(ParsedURLString, url);

    HTTPCachedResource *resource = m_httpCache.resourceForURL(kurl);
    if (resource)
        return CURL_PUSH_DENY; // already in cache

    ServerPushResourceHandleClient* client = new ServerPushResourceHandleClient();
    ResourceRequest req;
    req.setURL(kurl);
    req.setHTTPMethod(method);
    RefPtr<ResourceHandle> handle = ResourceHandle::create(pd->m_context.get(), req, client, false, false, false);

    ResourceHandle* job = handle.leakRef();
    ResourceHandleInternal* d = job->getInternal();
    d->m_handle = easy;
    d->m_cancelled = false;
    d->m_datasent = false;
    d->m_bytesSent = 0;
    d->m_totalBytesToBeSent = 0;
    d->m_serverpush = true;
    job->setFrame(nullptr);
    job->setMainFrame(nullptr, nullptr);

    curl_easy_setopt(d->m_handle, CURLOPT_PRIVATE, job);
    curl_easy_setopt(d->m_handle, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_WRITEDATA, job);
    curl_easy_setopt(d->m_handle, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_WRITEHEADER, job);
    curl_easy_setopt(d->m_handle, CURLOPT_XFERINFOFUNCTION, xferinfoCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_XFERINFODATA, job);
    curl_easy_setopt(d->m_handle, CURLOPT_NOPROGRESS, 0);

    d->m_urlhost = fastStrdup(hostAndPort(kurl).latin1().data());
    d->m_response.setResourceHandle(job);

    return CURL_PUSH_OK;
#else
    return CURL_PUSH_DENY;
#endif
}

void ResourceHandleManager::setProxyInfo(const String& host, unsigned long port, ProxyType type, const String& username, const String& password, const String& filters)
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    m_proxyHost = host;
    m_proxyPort = (port)?port:80;
    m_proxyCredential = Credential(username, password, CredentialPersistenceNone);
    m_proxyAuthScheme = ProtectionSpaceAuthenticationSchemeDefault;
    m_proxyType = type;

    m_proxyFilters.setProxyFilters(filters);

    /* remove all jobs */
    removeAllScheduledJobs();
    removeAllRunningJobs();
#if ENABLE(WKC_JAVASCRIPT_SEQUENTIAL)
    m_jsSequential = false;
#endif // WKC_JAVASCRIPT_SEQUENTIAL

    /* renewal cURL Multi */
    callOnNetworkThread([this]() {
        curl_multi_cleanup(m_curlMultiHandle);
        m_curlMultiHandle = curl_multi_init();
        m_curlHandlesInMulti = 0;
        curl_multi_setopt(m_curlMultiHandle, CURLMOPT_MAX_TOTAL_CONNECTIONS, m_httpConnections);
        curl_multi_setopt(m_curlMultiHandle, CURLMOPT_MAX_HOST_CONNECTIONS, DEFAUILT_MAX_HOST_CONNECTIONS);
        if (m_useHTTP2) {
            curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PIPELINING, (long)CURLPIPE_MULTIPLEX);
            curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PIPELINING_SERVER_BL, cBlackListedServer);
            curl_multi_setopt(m_curlMultiHandle, CURLMOPT_MAX_PIPELINE_LENGTH, m_httpPipelineRequests);
#if 0
            if (!m_httpCache.disabled()) {
                /* for server_push */
                curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PUSHFUNCTION, _serverPushCallback);
                curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PUSHDATA, this);
            }
#endif
        } else {
            curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PIPELINING, (long)CURLPIPE_NOTHING);
            if (!m_httpCache.disabled()) {
                /* for server_push */
                curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PUSHFUNCTION, nullptr);
                curl_multi_setopt(m_curlMultiHandle, CURLMOPT_PUSHDATA, nullptr);
            }
        }
    });

    if (0 == host.length()) {
        m_proxyHost = String("");
        m_tryProxySingleConnect = false;
    }
    else {
        m_tryProxySingleConnect = true;
    }

    LOG_FUNCTION(ResourceHandleManager, "out");
}

bool ResourceHandleManager::finishLoadingResourceHandle(ResourceHandle* job, ResourceHandleInternal* d)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    ASSERT(d->m_handle);
    if (!d->m_handle)
        return false;

    if (d->m_isProtected)
        return false;

    removeRunningJob(job);

    if (!d->m_cancelled) {
        if (d->m_doAuthChallenge) {
            appendScheduledJob(job);
            return true;
        }

        if (d->m_sslReconnect) {
            d->m_permitSend = ResourceHandleInternal::PERMIT;
            d->m_sslReconnect = false;
            curl_easy_setopt(d->m_handle, CURLOPT_SSL_VERIFYHOST, 0);
            curl_easy_setopt(d->m_handle, CURLOPT_SSL_VERIFYPEER, 0);
            appendScheduledJob(job);
            return true;
        }

        if (d->m_doRedirectChallenge) {
            if (m_redirectInWKC) {
                d->m_permitSend = ResourceHandleInternal::SUSPEND;
                d->m_doRedirectChallenge = false;
                appendScheduledJob(job);
                return true;
            }
        }
    }

    curl_easy_cleanup(d->m_handle);
    d->m_handle = 0;
    doClearCookies();
    if (d->m_serverpush) {
        delete job->client();
    }
    job->deref();

    LOG_FUNCTION(ResourceHandleManager, "Async load finishd [%p]", job);

    return true;
}

bool ResourceHandleManager::setupMethod(ResourceHandle* job, struct curl_slist** headers, int method)
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    ResourceHandleInternal* d = job->getInternal();

    FormData* formData = d->m_formDataStream.formData();
    if (!formData) {
        LOG(FormData, "(d:%p) No form data.", d);
        // do not send content-length header for CORS preflight
        if (method != HTTPMethod_OPTIONS && !job->firstRequest().httpHeaderFields().contains(HTTPHeaderName::AccessControlRequestMethod))
            *headers = curl_slist_append(*headers, "Content-Length: 0");
        return false;
    }

    Vector<FormDataElement> elements = formData->elements();
    size_t numElements = elements.size();
    LOG(FormData, "(d:%p) The number of the form data elements: %d", d, numElements);
    if (!numElements) {
        *headers = curl_slist_append(*headers, "Content-Length: 0");
        return false;
    }

    // Do not stream for simple POST/OPTIONS data
    if (HTTPMethod_POST == method || HTTPMethod_OPTIONS == method) {
        if (numElements == 1 && elements[0].m_type != FormDataElement::Type::EncodedFile
            && elements[0].m_type != FormDataElement::Type::EncodedBlob) {
            d->m_postBytes = formData->flatten();
            if (d->m_postBytes.size() != 0) {
                curl_easy_setopt(d->m_handle, CURLOPT_POST, TRUE);
                curl_easy_setopt(d->m_handle, CURLOPT_POSTFIELDSIZE, d->m_postBytes.size());
                curl_easy_setopt(d->m_handle, CURLOPT_POSTFIELDS, d->m_postBytes.data());
            } else {
                *headers = curl_slist_append(*headers, "Content-Length: 0");
            }
            LOG(FormData, "(d:%p) Post simple data: %zu", d, d->m_postBytes.size());
            return false;
        }
    }

    // Obtain the total size
    // The size of a curl_off_t could be different in WebKit and in cURL depending on
    // compilation flags of both. For CURLOPT_POSTFIELDSIZE_LARGE/CURLOPT_INFILESIZE_LARGE
    // we have to pass the right size or random data will be used as the size.
    WKC_DEFINE_STATIC_INT(expectedSizeOfCurlOffT, 0);
    if (!expectedSizeOfCurlOffT) {
        curl_version_info_data *infoData = curl_version_info(CURLVERSION_NOW);
        if (infoData->features & CURL_VERSION_LARGEFILE)
            expectedSizeOfCurlOffT = sizeof(long long);
        else
            expectedSizeOfCurlOffT = sizeof(int);
    }

#if COMPILER(MSVC)
    // work around compiler error in Visual Studio 2005.  It can't properly
    // handle math with 64-bit constant declarations.
#pragma warning(disable: 4307)
#endif
    static const long long maxCurlOffT = (1LL << (expectedSizeOfCurlOffT * 8 - 1)) - 1;
    curl_off_t size = 0;
    bool chunkedTransfer = false;
    d->m_sendingBlobDataList.resize(numElements);
    for (size_t i = 0; i < numElements; i++) {
        d->m_sendingBlobDataList[i] = nullptr;
        FormDataElement element = elements[i];
        if (element.m_type == FormDataElement::Type::EncodedFile) {
            long long fileSizeResult = element.lengthInBytes();
            if (fileSizeResult > maxCurlOffT) {
                // File size is too big for specifying it to cURL
                chunkedTransfer = true;
                LOG(FormData, "(d:%p) The data size of the elements[%d]: %zu (type: %d, chunked: true, length: %" PRIi64 ", offset: %" PRIi64 ", name: %s)", d, i, fileSizeResult, element.m_type, element.m_fileLength, element.m_fileStart, element.m_filename.utf8().data());
                break;
            } else if (fileSizeResult < 0) {
                chunkedTransfer = true;
                LOG(FormData, "(d:%p) The data size of the elements[%d]: %zu (type: %d, chunked: true, length: %" PRIi64 ", offset: %" PRIi64 ", name: %s)", d, i, fileSizeResult, element.m_type, element.m_fileLength, element.m_fileStart, element.m_filename.utf8().data());
                break;
            }
            size += fileSizeResult;
            LOG(FormData, "(d:%p) The data size of the elements[%d]: %zu (type: %d, chunked: false, length: %" PRIi64 ", offset: %" PRIi64 ", name: %s)", d, i, fileSizeResult, element.m_type, element.m_fileLength, element.m_fileStart, element.m_filename.utf8().data());
        } else if (element.m_type == FormDataElement::Type::EncodedBlob) {
            BlobData* blobData = static_cast<BlobRegistryImpl&>(blobRegistry()).getBlobDataFromURL(element.m_url);
            if (!blobData) {
                LOG(FormData, "(d:%p) The elements[%d] has no blob data.", d, i);
                break;
            }
            for (size_t j=0; j<blobData->items().size(); j++) {
                const BlobDataItem& item = blobData->items()[j];
                size += item.length();
                LOG(FormData, "(d:%p) The data size of the elements[%d](items[%d]): %zu (type: %d(%d))", d, i, j, item.length(), element.m_type, item.type());
            }
            // We hold a strong reference to the blob data here, and it will be referred to in FormDataStream::read.
            // This is a workaround for the fact that the blob data may be deleted by GC before sent.
            d->m_sendingBlobDataList[i] = blobData;
        } else {
            size += elements[i].m_data.size();
            LOG(FormData, "(d:%p) The data size of the elements[%d]: %zu (type: %d)", d, i, elements[i].m_data.size(), element.m_type);
        }
    }
    LOG(FormData, "(d:%p) The total size of the form data: %zu", d, size);

    // cURL guesses that we want chunked encoding as long as we specify the header
    if (chunkedTransfer) {
        *headers = curl_slist_append(*headers, "Transfer-Encoding: chunked");
    }
    else {
        CURLoption option = CURLOPT_LASTENTRY;
        if (HTTPMethod_POST == method || HTTPMethod_OPTIONS == method) {
            if (sizeof(long long) == expectedSizeOfCurlOffT)
                option = CURLOPT_POSTFIELDSIZE_LARGE;
            else
                option = CURLOPT_POSTFIELDSIZE;
        }
        else if (HTTPMethod_PUT == method) {
            if (sizeof(long long) == expectedSizeOfCurlOffT)
                option = CURLOPT_INFILESIZE_LARGE;
            else
                option = CURLOPT_INFILESIZE;
        }
        if (sizeof(long long) == expectedSizeOfCurlOffT)
          curl_easy_setopt(d->m_handle, option, (long long)size);
        else
          curl_easy_setopt(d->m_handle, option, (int)size);
    }

    curl_easy_setopt(d->m_handle, CURLOPT_READFUNCTION, readCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_READDATA, job);

    return true;
}

void ResourceHandleManager::setupPUT(ResourceHandle* job, struct curl_slist** headers)
{
    LOG_FUNCTION(ResourceHandleManager, "out");

    ResourceHandleInternal* d = job->getInternal();
    curl_easy_setopt(d->m_handle, CURLOPT_UPLOAD, TRUE);
    curl_easy_setopt(d->m_handle, CURLOPT_INFILESIZE, 0);

    (void)setupMethod(job, headers, HTTPMethod_PUT);
}

/* Calculate the length of the POST.
   Force chunked data transfer if size of files can't be obtained.
 */
void ResourceHandleManager::setupPOST(ResourceHandle* job, struct curl_slist** headers)
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    ResourceHandleInternal* d = job->getInternal();
    curl_easy_setopt(d->m_handle, CURLOPT_POSTFIELDSIZE, 0);

    if (!setupMethod(job, headers, HTTPMethod_POST)) {
        if (d->m_postBytes.size() == 0)
            curl_easy_setopt(d->m_handle, CURLOPT_CUSTOMREQUEST, "POST");
        return;
    }

    curl_easy_setopt(d->m_handle, CURLOPT_POST, TRUE);
    curl_easy_setopt(d->m_handle, CURLOPT_IOCTLFUNCTION, ioctlCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_IOCTLDATA, job);
}

/* Calculate the length of the OPTIONS.
   Force chunked data transfer if size of files can't be obtained.
 */
void ResourceHandleManager::setupOPTIONS(ResourceHandle* job, struct curl_slist** headers)
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    ResourceHandleInternal* d = job->getInternal();

    /* Specify OPTIONS method */
    curl_easy_setopt(d->m_handle, CURLOPT_CUSTOMREQUEST, "OPTIONS");

    /* The code below is the same as setupPOST(), except comments */
    curl_easy_setopt(d->m_handle, CURLOPT_POSTFIELDSIZE, 0);

    if (!setupMethod(job, headers, HTTPMethod_OPTIONS))
        return;

    curl_easy_setopt(d->m_handle, CURLOPT_POST, TRUE);
    curl_easy_setopt(d->m_handle, CURLOPT_IOCTLFUNCTION, ioctlCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_IOCTLDATA, job);
}

void ResourceHandleManager::add(ResourceHandle* job)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    ResourceHandleInternal* d = job->getInternal();
    URL kurl = job->firstRequest().url();

    d->m_matchProxyFilter = m_proxyFilters.isMatchProxyFilter(kurl.host().toString());

    LOG_FUNCTION(ResourceHandleManager, "Async load add [%p] %s", job, kurl.string().latin1().data());

#if ENABLE(WKC_HTTPCACHE)
    HTTPCachedResource* existingResource = getHTTPCache(kurl);
    if (canReadHTTPCache(job, existingResource)) {
        switch (job->firstRequest().cachePolicy()) {
        case ResourceRequestCachePolicy::ReturnCacheDataDontLoad: // fall through
        case ResourceRequestCachePolicy::ReturnCacheDataElseLoad:
            scheduleLoadResourceFromHTTPCache(job);
            return;
        case ResourceRequestCachePolicy::RefreshAnyCacheData:
            makeConditionalRequest(job, existingResource);
            break;
        default:
            if (!existingResource->needRevalidate() || existingResource->serverPushed()) {
                scheduleLoadResourceFromHTTPCache(job);
                return;
            }
            makeConditionalRequest(job, existingResource);
            break;
        }
    }
#endif // ENABLE(WKC_HTTPCACHE)

    if (job->firstRequest().cachePolicy() == ResourceRequestCachePolicy::ReturnCacheDataDontLoad) {
        job->ref();
        callOnMainThread([this, job, d] {
            d->client()->didFail(job, ResourceError(String(), CURLE_READ_ERROR, job->firstRequest().url(), String("cache read error")));
            job->deref();
        });
        return;
    }

    // we can be called from within curl, so to avoid re-entrancy issues
    // schedule this job to be added the next time we enter curl download loop
    job->ref();
    appendScheduledJob(job);

    if (!m_downloadTimer.isActive()) {
        m_downloadTimer.startOneShot(Seconds(pollTimeSeconds));
    }

    LOG_FUNCTION(ResourceHandleManager, "out");
}

void ResourceHandleManager::didReceiveAuthenticationChallenge(ResourceHandle* job)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    ResourceHandleInternal* d = job->getInternal();

    AuthenticationChallenge challenge(job, 0, d->m_response, job);

    job->didReceiveAuthenticationChallenge(challenge);

    LOG_FUNCTION(ResourceHandleManager, "out");
}

void ResourceHandleManager::clearAuthCredential(ResourceHandle* job)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    ResourceHandleInternal* d = job->getInternal();
    if (!d) return;

    d->m_user = String();
    d->m_pass = String();
    d->m_initialCredential = Credential();
    d->m_authScheme = ProtectionSpaceAuthenticationSchemeDefault;

    curl_easy_setopt(d->m_handle, CURLOPT_USERNAME, NULL);
    curl_easy_setopt(d->m_handle, CURLOPT_PASSWORD, NULL);

    LOG_FUNCTION(ResourceHandleManager, "out");
}

bool ResourceHandleManager::startScheduledJobs()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    ResourceHandle* job;
    bool started = false;

    int size = m_scheduledJobList.size();
    if (0 == size)
        return false;

    while (1) {
        job = shiftScheduledJob();
        if (!job)
            break;

        ResourceHandleInternal* d = job->getInternal();
        if (!d)
            continue;
        if (d && d->m_cancelled){
            if (d->m_permitSend == ResourceHandleInternal::CANCEL)
                if (d->client())
                    d->client()->didFail(job, ResourceError(String(), CURLE_GOT_NOTHING, URL(ParsedURLString, String(d->m_url)), String("prohibit"), ResourceError::Type::Cancellation, 0));
            d->m_handle = 0;
            job->deref();
            continue;
        }

        URL kurl = job->firstRequest().url();

        if (kurl.protocolIsData()) {
            job->handleDataURL();
            job->cancel();
            job->deref();
            continue;
        }

        if (!startJob(job, d)) {
            job->deref();
            continue;
        }
        started = true;
    }
    if (started) {
        resumeNetworkThread();
    }

    LOG_FUNCTION(ResourceHandleManager, "ret: true");
    return true;
}

void ResourceHandleManager::cancelScheduledJobs()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    ResourceHandle* job;
    ResourceHandleInternal* d;
    bool loop = true;
    int size;

    while (loop) {
        loop = false;
        size = m_runningJobList.size();
        for (int i = 0; i < size; i++) {
            job = m_runningJobList[i];
            d = job->getInternal();
            if (d && d->m_cancelled) {
                m_runningJobList.remove(i);

#if ENABLE(WKC_JAVASCRIPT_SEQUENTIAL)
                if (d->m_isJSRequestatFirst)
                    m_jsSequential = false;
#endif // WKC_JAVASCRIPT_SEQUENTIAL

#if ENABLE(WKC_HTTPCACHE)
                if (!m_httpCache.disabled()) {
                    URL kurl = job->firstRequest().url();
                    HTTPCachedResource *resource = m_httpCache.resourceForURL(kurl);
                    if (resource) {
                        resource->setUsed(false);
                    }
                }
#endif // WKC_HTTPCACHE
                callOnNetworkThread([this, job, d]() {
                    if (d->m_handleIsInMulti) {
                        curl_multi_remove_handle(m_curlMultiHandle, d->m_handle);
                        m_curlHandlesInMulti--;
                        d->m_handleIsInMulti = false;
                        d->m_isRunning = false;
                    }
                    curl_easy_cleanup(d->m_handle);
                    d->m_handle = 0;
                    callOnMainThread([job] {
                        job->deref();
                    });
                });
                loop = true;
                break;
            }
        }
    }

    doClearCookies();
    LOG_FUNCTION(ResourceHandleManager, "out");
 }

void ResourceHandleManager::dispatchSynchronousJob(ResourceHandle* rjob)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", rjob);

    URL kurl = rjob->firstRequest().url();

    if (kurl.protocolIsData()) {
        rjob->handleDataURL();
        return;
    }

    ResourceHandleInternal* d = rjob->getInternal();
    bool failed = false;

#if LIBCURL_VERSION_NUM > 0x071200
    // If defersLoading is true and we call curl_easy_perform
    // on a paused handle, libcURL would do the transfert anyway
    // and we would assert so force defersLoading to be false.
    d->m_defersLoading = false;
#endif

    d->m_url = fastStrdup(kurl.string().latin1().data());
    d->m_composition = contentComposition(rjob);
    d->m_matchProxyFilter = m_proxyFilters.isMatchProxyFilter(kurl.host().toString());

    if (m_harmfulSiteFilter) {
        WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(rjob);
        if (!d->m_cancelled && fl) {
            int ret = fl->dispatchWillPermitSendRequest(rjob, d->m_url, d->m_composition, true, d->m_response);
            if (1 != ret) {
                d->m_permitSend = ResourceHandleInternal::CANCEL;
                d->m_cancelled = true;
                if (d->client())
                    d->client()->didFail(rjob, ResourceError(String(), CURLE_GOT_NOTHING, URL(ParsedURLString, String(d->m_url)), String("prohibit"), ResourceError::Type::Cancellation, 0));
                return;
            }
        }
        else {
            d->m_permitSend = ResourceHandleInternal::CANCEL;
            d->m_cancelled = true;
            if (d->client())
                d->client()->didFail(rjob, ResourceError(String(), CURLE_GOT_NOTHING, URL(ParsedURLString, String(d->m_url)), String("prohibit"), ResourceError::Type::Cancellation, 0));
            return;
        }
    }

    initializeHandle(rjob);
    if (!d->m_handle) {
        LOG_FUNCTION(ResourceHandleManager, "No Memory or something error");
        if (d->client())
            d->client()->didFail(rjob, ResourceError(String(), CURLE_OUT_OF_MEMORY, URL(ParsedURLString, String(d->m_url)), String("no memory")));
        return;
    }
    d->m_isSynchronous = true;

    if (d->client())
        d->client()->willSendRequestAsync(rjob, ResourceRequest(rjob->firstRequest()), ResourceResponse(d->m_response), [](ResourceRequest&&){});

    LOG_FUNCTION(ResourceHandleManager, "Sync load start [%p] %s", rjob, d->m_url);

    curl_multi_add_handle(m_curlMultiSyncHandle, d->m_handle);

    bool isMainThread = (wkcThreadCurrentThreadPeer() == m_mainThreadId) ? true : false;
    bool retry;
    do {
        retry = false;

        int runningHandles = 0;
        while (curl_multi_perform(m_curlMultiSyncHandle, &runningHandles) == CURLM_CALL_MULTI_PERFORM) { }

        wkc_usleep(1000);

        int messagesInQueue;
        CURLMsg* msg = curl_multi_info_read(m_curlMultiSyncHandle, &messagesInQueue);
        if (!msg) {
            retry = true;
            continue;
        }

        // find the node which has same d->m_handle as completed transfer
        CURL* handle = msg->easy_handle;
        ASSERT(handle);
        ResourceHandle* job = 0;
        (void)curl_easy_getinfo(handle, CURLINFO_PRIVATE, &job);
        ASSERT(job);
        ResourceHandleInternal* d = job->getInternal();

        CURLcode ret = msg->data.result;

        sendDataFromBufferToWebCore(job, d);

        curl_multi_remove_handle(m_curlMultiSyncHandle, d->m_handle);

        long httpCode = 0;
        (void)curl_easy_getinfo(d->m_handle, CURLINFO_RESPONSE_CODE, &httpCode);

        // HTTP redirection
        if (m_redirectInWKC && 300 <= httpCode && httpCode < 400) {
            if (d->m_redirectCount > d->m_redirectMaxCount) {
                if (d->client())
                    d->client()->didFail(job, ResourceError(String(), CURLE_TOO_MANY_REDIRECTS, URL(ParsedURLString, String(d->m_url)), String(curl_easy_strerror(CURLE_TOO_MANY_REDIRECTS)), ResourceError::Type::General, 0));
                failed = true;
                break;
            }
            if (d->m_doRedirectChallenge) {
                d->m_doRedirectChallenge = false;

                if (m_harmfulSiteFilter) {
                    WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(job);
                    if (!d->m_cancelled && fl) {
                        kurl = job->firstRequest().url();
                        d->m_url = fastStrdup(kurl.string().latin1().data());
                        initializeHandle(job);
                        fl = frameloaderclientwkc(job);
                        int ret = 0;
                        if (fl)
                            ret = fl->dispatchWillPermitSendRequest(job, d->m_url, d->m_composition, true, d->m_response);
                        if (1 != ret) {
                            d->m_permitSend = ResourceHandleInternal::CANCEL;
                            d->m_cancelled = true;
                            if (d->client())
                                d->client()->didFail(job, ResourceError(String(), CURLE_GOT_NOTHING, URL(ParsedURLString, String(d->m_url)), String("prohibit"), ResourceError::Type::Cancellation, 0));
                            failed = true;
                            break;
                        }
                    }
                    else {
                        d->m_permitSend = ResourceHandleInternal::CANCEL;
                        d->m_cancelled = true;
                        if (d->client())
                            d->client()->didFail(job, ResourceError(String(), CURLE_GOT_NOTHING, URL(ParsedURLString, String(d->m_url)), String("prohibit"), ResourceError::Type::Cancellation, 0));
                        failed = true;
                        break;
                    }
                } else {
                    kurl = job->firstRequest().url();
                    d->m_url = fastStrdup(kurl.string().latin1().data());
                    initializeHandle(job);
                }

                curl_multi_add_handle(m_curlMultiSyncHandle, d->m_handle);
                retry = true;
                continue;
            }
        }
        if ((401 == httpCode || 407 == httpCode) && d->m_response.isHTTP()) {
            didReceiveAuthenticationChallenge(job);
            if (!d->m_cancelled) {
                if (d->m_doAuthChallenge) {
                    initializeHandle(job);
                    curl_multi_add_handle(m_curlMultiSyncHandle, d->m_handle);
                    retry = true;
                    continue;
                }
            }
            if (401 == httpCode) {
                if (d->client()) {
                    d->client()->didReceiveResponseAsync(job, ResourceResponse(d->m_response), [](){});
                    d->m_response.setResponseFired(true);
                    d->client()->didReceiveData(job, d->m_receivedData->data(), d->m_receivedData->size(), 0);
                }
            }
        }

        if (300 > httpCode && d->m_response.isHTTP()) {
            if (d->client()) {
                d->client()->didReceiveResponseAsync(job, ResourceResponse(d->m_response), [](){});
                d->m_response.setResponseFired(true);
                d->client()->didReceiveData(job, d->m_receivedData->data(), d->m_receivedData->size(), 0);
            }
        }

        if (!retry && ret != 0) {
            char* url = 0;
            WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(job);
            if (d->m_isSSL && !d->m_SSLHandshaked && fl) {
                m_rhmssl->SSLHandshakeInfo(job);
                fl->notifySSLHandshakeStatus(job, WKC::EHandshakeFail);
                d->m_SSLHandshaked = true;
            }
            curl_easy_getinfo(d->m_handle, CURLINFO_EFFECTIVE_URL, &url);
            LOG_FUNCTION(ResourceHandleManager, "cURL ERROR for url='%s', error: '[%d]%s'", url, ret, curl_easy_strerror(ret));
            if (d->client())
                d->client()->didFail(job, ResourceError(String(), ret, URL(ParsedURLString, String(url)), String(curl_easy_strerror(ret))));
            failed = true;
        }
    } while (retry);

    setResourceLoadTiming(rjob);

    if (!failed && d->client())
        d->client()->didReceiveResponseAsync(rjob, ResourceResponse(d->m_response), [](){});

    curl_easy_cleanup(d->m_handle);

    if (isMainThread) {
        curl_multi_refresh_timer(m_curlMultiHandle);
    }

    LOG_FUNCTION(ResourceHandleManager, "Sync load finished [%p]", rjob);
}

bool ResourceHandleManager::startJob(ResourceHandle* job, ResourceHandleInternal* d)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    initializeHandle(job);
    if (!d->m_handle) {
        LOG_FUNCTION(ResourceHandleManager, "No Memory or Something error");
        if (d->client())
            d->client()->didFail(job, ResourceError(String(), CURLE_OUT_OF_MEMORY, URL(ParsedURLString, String(d->m_url)), String("no memory")));
        return false;
    }

    WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(job);
    if (d->m_isSSL && fl) {
        fl->notifySSLHandshakeStatus(job, WKC::EHandshakeInitialize);
    }

    appendRunningJob(job);

    callOnNetworkThread([this, job, d]() {
        CURLMcode ret = curl_multi_add_handle(m_curlMultiHandle, d->m_handle);
        // don't call perform, because events must be async
        // timeout will occur and do curl_multi_perform
        if (ret == CURLE_OK || ret == CURLM_CALL_MULTI_PERFORM) {
            m_curlHandlesInMulti++;
            d->m_handleIsInMulti = true;
        } else {
            callOnMainThread([this, job, d] {
                WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(job);
                if (d->m_isSSL && fl) {
                    fl->notifySSLHandshakeStatus(job, WKC::EHandshakeFail);
                }
                job->cancel();
                d->m_isRunning = false;
            });
        }
    });

    LOG_FUNCTION(ResourceHandleManager, "out");
    return true;
}

void ResourceHandleManager::initializeHandle(ResourceHandle* job)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    ResourceHandleInternal* d = job->getInternal();

    // at first, malloc memory
    if (!d->m_handle) {
        ASSERT(0 == d->m_redirectCount);
        d->m_handle = curl_easy_init();
        if (!d->m_handle) {
            return;
        }
    }
    d->m_cancelled = false;
    d->m_datasent = false;
    d->m_bytesSent = 0;
    d->m_totalBytesToBeSent = 0;
    d->m_doAuthChallenge = false;

    URL kurl = job->firstRequest().url();
    String url = kurl.string();

    // Remove any fragment part, otherwise curl will send it as part of the request.
    kurl.removeFragmentIdentifier();

    if (kurl.isLocalFile()) {
        String query = kurl.query();
        // Remove any query part sent to a local file.
        if (!query.isEmpty()) {
            size_t queryIndex = url.find(query);
            if (queryIndex != notFound)
                url = url.left(queryIndex - 1);
        }
        // Determine the MIME type based on the path.
        d->m_response.setMimeType(MIMETypeRegistry::getMIMETypeForPath(kurl.path()));
        d->m_fileLoading = true;
    }
    if (kurl.protocolIsData()) {
        d->m_dataLoading = true;
    }

#if LIBCURL_VERSION_NUM > 0x071200
    if (d->m_defersLoading) {
        CURLcode error = curl_easy_pause(d->m_handle, CURLPAUSE_ALL);
        // If we did not pause the handle, we would ASSERT in the
        // header callback. So just assert here.
        ASSERT(error == CURLE_OK);
        if (error != CURLE_OK) {
            curl_easy_cleanup(d->m_handle);
            d->m_handle = NULL;
            return;
        }
    }
#endif

    if (gCurlDebugProc) {
        curl_easy_setopt(d->m_handle, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(d->m_handle, CURLOPT_DEBUGFUNCTION, debugCallback);
        curl_easy_setopt(d->m_handle, CURLOPT_DEBUGDATA, job);
    }
#ifdef CURL_DEBUG_CALLBACK
    curl_easy_setopt(d->m_handle, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(d->m_handle, CURLOPT_DEBUGFUNCTION, debugCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_DEBUGDATA, job);
#endif

    curl_easy_setopt(d->m_handle, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    if (m_useHTTP2) {
        curl_easy_setopt(d->m_handle, CURLOPT_SSL_ENABLE_NPN, (long)1);
        curl_easy_setopt(d->m_handle, CURLOPT_SSL_ENABLE_ALPN, (long)1);
    }

    curl_easy_setopt(d->m_handle, CURLOPT_PRIVATE, job);
    curl_easy_setopt(d->m_handle, CURLOPT_ERRORBUFFER, m_curlErrorBuffer);
    curl_easy_setopt(d->m_handle, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_WRITEDATA, job);
    curl_easy_setopt(d->m_handle, CURLOPT_HEADERFUNCTION, headerCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_WRITEHEADER, job);
    curl_easy_setopt(d->m_handle, CURLOPT_XFERINFOFUNCTION, xferinfoCallback);
    curl_easy_setopt(d->m_handle, CURLOPT_XFERINFODATA, job);
    curl_easy_setopt(d->m_handle, CURLOPT_NOPROGRESS, 0);
    if (d->m_firstRequest.sendAutoHTTPReferfer())
        curl_easy_setopt(d->m_handle, CURLOPT_AUTOREFERER, 1);
    curl_easy_setopt(d->m_handle, CURLOPT_SHARE, m_curlShareHandle);
    curl_easy_setopt(d->m_handle, CURLOPT_DNS_CACHE_TIMEOUT, m_DNSCacheTimeout);
    // cookie
    curl_easy_setopt(d->m_handle, CURLOPT_COOKIEJAR, NULL); // enable the cookie engine 
    curl_easy_setopt(d->m_handle, CURLOPT_COOKIEFUNCTIONDATA, job);
    curl_easy_setopt(d->m_handle, CURLOPT_COOKIEFUNCTION, cookie_callback);
    // filtering
    curl_easy_setopt(d->m_handle, CURLOPT_CONNECT_FILTERING_FUNCTION, filter_callback);
    // redirect
    if (!m_redirectInWKC) {
        curl_easy_setopt(d->m_handle, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(d->m_handle, CURLOPT_MAXREDIRS, 10);
    }
    // only one connection for each handle
    curl_easy_setopt(d->m_handle, CURLOPT_MAXCONNECTS, 1);

    // Set connect timeout.
    curl_easy_setopt(d->m_handle, CURLOPT_SERVER_RESPONSE_TIMEOUT, m_serverResponseTimeout);
    curl_easy_setopt(d->m_handle, CURLOPT_CONNECTTIMEOUT, m_connectTimeout);
    if (job->firstRequest().timeoutInterval() != std::numeric_limits<double>::infinity()) {
        double timeout = job->firstRequest().timeoutInterval();
        long ltimeout = timeout * 1000;
        curl_easy_setopt(d->m_handle, CURLOPT_SERVER_RESPONSE_TIMEOUT, (long)(ltimeout / 1000));
        curl_easy_setopt(d->m_handle, CURLOPT_CONNECTTIMEOUT_MS, ltimeout);
        curl_easy_setopt(d->m_handle, CURLOPT_TIMEOUT_MS, ltimeout);
    }

    long httpver = CURL_HTTP_VERSION_1_1;
    if (equalIgnoringASCIICase(kurl.protocol(), "https")) {
        d->m_isSSL = true;
        m_rhmssl->initializeHandleSSL(job);
        if (m_useHTTP2)
            httpver = CURL_HTTP_VERSION_2TLS;
    }
    curl_easy_setopt(d->m_handle, CURLOPT_HTTP_VERSION, httpver);

    // set accept-encoding:
    if (m_acceptEncoding.length())
        curl_easy_setopt(d->m_handle, CURLOPT_ENCODING, m_acceptEncoding.utf8().data());
    else
        curl_easy_setopt(d->m_handle, CURLOPT_ENCODING, NULL); // don't send accept-encoding

    // url must remain valid through the request
    //ASSERT(!d->m_url);

    // url is in ASCII so latin1() will only convert it to char* without character translation.
    // ~ResourceHandleInternal() will fastFree d->m_url.
    d->m_urlhost = fastStrdup(hostAndPort(kurl).latin1().data());
    curl_easy_setopt(d->m_handle, CURLOPT_URL, d->m_url);

    struct curl_slist* headers = 0;

    if (job->firstRequest().httpHeaderFields().size() > 0) {
        HTTPHeaderMap customHeaders = job->firstRequest().httpHeaderFields();
        HTTPHeaderMap::const_iterator end = customHeaders.end();
        for (HTTPHeaderMap::const_iterator it = customHeaders.begin(); it != end; ++it) {
            String key = it->key;
            String value = it->value;
            if (key == "Accept-Encoding") {
                curl_easy_setopt(d->m_handle, CURLOPT_ACCEPT_ENCODING, value.latin1().data());
            }
            else {
                String headerString(key);
                if (value.length()) {
                    headerString.append(": ");
                    headerString.append(value);
                } else {
                    headerString.append(";");
                }
                if (equalIgnoringASCIICase(key, "Last-Event-ID")) {
                    CString headerUtf8 = headerString.utf8();
                    headers = curl_slist_append(headers, headerUtf8.data());
                } else {
                    CString headerLatin1 = headerString.latin1();
                    headers = curl_slist_append(headers, headerLatin1.data());
                }
            }
        }
    }

    if ("GET" == job->firstRequest().httpMethod())
        curl_easy_setopt(d->m_handle, CURLOPT_HTTPGET, TRUE);
    else if ("POST" == job->firstRequest().httpMethod())
        setupPOST(job, &headers);
    else if ("PUT" == job->firstRequest().httpMethod())
        setupPUT(job, &headers);
    else if ("HEAD" == job->firstRequest().httpMethod())
        curl_easy_setopt(d->m_handle, CURLOPT_NOBODY, TRUE);
    else if ("OPTIONS" == job->firstRequest().httpMethod())
        setupOPTIONS(job, &headers);
    else {
        setupPUT(job, &headers);
        curl_easy_setopt(d->m_handle, CURLOPT_CUSTOMREQUEST, job->firstRequest().httpMethod().utf8().data());
    }

    // Do Not Track
    if (m_DoNotTrack) {
        headers = curl_slist_append(headers, "DNT: 1");
    }

    if (headers) {
        curl_easy_setopt(d->m_handle, CURLOPT_HTTPHEADER, headers);
        d->m_customHeaders = headers;
    }

    // if file/data scheme loading, do not set proxy/authenticate
    if (d->m_fileLoading || d->m_dataLoading) {
        d->m_response.setResourceHandle(job);
        if (d->m_fileLoading) {
            handleLocalReceiveResponse(d->m_handle, job, d);
        }
        LOG_FUNCTION(ResourceHandleManager, "out1");
        return;
    }

    // Proxy
    if (!d->m_matchProxyFilter) {
        if (m_proxyHost.length()) {
            curl_easy_setopt(d->m_handle, CURLOPT_PROXY, m_proxyHost.utf8().data());
            curl_easy_setopt(d->m_handle, CURLOPT_PROXYPORT, m_proxyPort);

            if (HTTP10 == m_proxyType) {
                curl_easy_setopt(d->m_handle, CURLOPT_PROXYTYPE, CURL_HTTP_VERSION_1_0);  // cURL uses only for proxy CONNECT.
                curl_easy_setopt(d->m_handle, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
            } else
                curl_easy_setopt(d->m_handle, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);

            if (m_proxyAuthScheme != ProtectionSpaceAuthenticationSchemeDefault)
                setProxyUserPassword(d->m_handle, m_proxyCredential, m_proxyAuthScheme);

            curl_easy_setopt(d->m_handle, CURLOPT_NOPROXY, "127.0.0.1, localhost");
        }
    }

    // HTTP Authenticate
    if (job->shouldUseCredentialStorage()) {
        if (!d->m_user.isEmpty() && !d->m_pass.isEmpty()) {
            // We will get credential in ResourceHandle::didReceiveAuthenticationChallenge and store it.
            d->m_initialCredential = Credential();
            d->m_authScheme = ProtectionSpaceAuthenticationSchemeDefault;
        } else if (d->m_initialCredential.isEmpty()) {
            // <rdar://problem/7174050> - For URLs that match the paths of those previously challenged for HTTP Basic authentication, 
            // try and reuse the credential preemptively, as allowed by RFC 2617.
            d->m_initialCredential = CredentialStorage::defaultCredentialStorage().get(d->m_firstRequest.cachePartition(), d->m_firstRequest.url());
            d->m_authScheme = ProtectionSpaceAuthenticationSchemeHTTPBasic;
        }
    }
    setHttpUserPassword(d->m_handle, d->m_initialCredential, d->m_authScheme);

    d->m_response.setResourceHandle(job);

    LOG_FUNCTION(ResourceHandleManager, "out2");
}

void ResourceHandleManager::cancel(ResourceHandle* job)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    if (removeScheduledJob(job))
        return;

    ResourceHandleInternal* d = job->getInternal();
    if (!d)
        return;

    d->m_cancelled = true;
    if (!d->m_handle)
        return;

    if (!m_downloadTimer.isActive()) {
        m_downloadTimer.startOneShot(Seconds(pollTimeSeconds));
    }

    LOG_FUNCTION(ResourceHandleManager, "out");
}

/*
 *  Cookie
 */
void ResourceHandleManager::setWillAcceptCookieCallback(WKC::WillAcceptCookieProc proc)
{
    gWillAcceptCookieProc = proc;
}

void ResourceHandleManager::setCookie(const String &domain, const String &path, const String &cookie)
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    wkcMutexLockPeer(gCookieMutex);

    curl_share_set_cookie(m_curlShareHandle, domain.utf8().data(), path.utf8().data(), cookie.utf8().data(), COOKIETYPE_HEADERLINE);

    wkcMutexUnlockPeer(gCookieMutex);
}

String ResourceHandleManager::getCookie(const String &domain, const String &path, bool secure, bool includehttponly)
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    wkcMutexLockPeer(gCookieMutex);
    int len = 0;
    char* buf = 0;

    len = curl_share_get_cookie(m_curlShareHandle, domain.utf8().data(), path.utf8().data(), secure ? TRUE : FALSE, includehttponly ? TRUE : FALSE, NULL, 0);
    if (len>0) {
        buf = (char *)fastMalloc(len+1);
        if (buf) {
            buf[0] = 0x0;
            len = curl_share_get_cookie(m_curlShareHandle, domain.utf8().data(), path.utf8().data(), secure ? TRUE : FALSE, includehttponly ? TRUE : FALSE, buf, len+1);
        }
    }

    wkcMutexUnlockPeer(gCookieMutex);

    if (buf) {
        String result = String::fromUTF8(buf);
        fastFree(buf);
        return result;
    } else {
        return String();
    }
}

int ResourceHandleManager::CookieSerialize(char* buff, int bufflen)
{
    LOG_FUNCTION(ResourceHandleManager, "(%p, %d)", buff, bufflen);

    wkcMutexLockPeer(gCookieMutex);

    struct curl_slist* cookies = 0;
    struct curl_slist* current;
    int total_len = 0;
    char* pbuff = buff;

    CURLSHcode ret = curl_share_cookie_list(m_curlShareHandle, &cookies);
    if (ret != CURLSHE_OK) {
        wkcMutexUnlockPeer(gCookieMutex);
        return -1;
    }

    current = cookies;
    while (current) {
        total_len += strlen(current->data) + 2; // 2: length of '\r' and '\n'.
        if (!current->next) {
            total_len++; // for a null terminator.
        }
        current = current->next;
    }

    if (!buff)
        goto CookieSerializeEnd;

    if (bufflen <= 0) {
        total_len = 0;
        goto CookieSerializeEnd;
    }

    current = cookies;
    while (current) {
        int datalen = strlen(current->data);
        // Discard older cookies if the size of cookies exceeds the buffer size.
        if (bufflen < total_len) {
            total_len -= datalen + 2;
            current = current->next;
            continue;
        }
        memcpy(pbuff, current->data, datalen);
        pbuff += datalen;
        *pbuff = '\r'; pbuff++;
        *pbuff = '\n'; pbuff++;
        current = current->next;
    }
    *pbuff = '\0'; pbuff++;
    total_len = pbuff - buff;

CookieSerializeEnd:
    curl_slist_free_all(cookies);
    wkcMutexUnlockPeer(gCookieMutex);

    LOG_FUNCTION(ResourceHandleManager, "out");
    return total_len;
}

void ResourceHandleManager::CookieDeserialize(const char* buff, bool restart)
{
    LOG_FUNCTION(ResourceHandleManager, "buff:%p, restart:%s", buff, restart ? "true" : "false");

    wkcMutexLockPeer(gCookieMutex);

    while (*buff == '\r' || *buff == '\n')
        buff++;

    size_t buffLen;
    while (0 < (buffLen = strlen(buff))) {
        size_t span = strcspn(buff, "\x0D\x0A");
        if (0 == span)
            span = buffLen;
        char* cookie = (char*)fastMalloc(span + 1);
        if (!cookie)
            goto CookieDeserializeEnd;
        memcpy(cookie, buff, span);
        cookie[span] = 0x0;

        curl_share_set_cookie(m_curlShareHandle, NULL, NULL, cookie, (restart)?COOKIETYPE_COOKIELIST_ALL:COOKIETYPE_COOKIELIST);

        fastFree(cookie);

        buffLen -= span;
        buff += span;

        while (*buff == '\r' || *buff == '\n')
            buff++;
    }

CookieDeserializeEnd:
    wkcMutexUnlockPeer(gCookieMutex);

    LOG_FUNCTION(ResourceHandleManager, "out");
}

bool ResourceHandleManager::getRawCookies(const String &domain, const String &path, bool secure, Vector<Cookie>& rawCookies)
{
    rawCookies.clear();

    wkcMutexLockPeer(gCookieMutex);

    struct CookieJarIterator* it = curl_share_create_cookies_iterator(m_curlShareHandle, domain.utf8().data(), path.utf8().data(), secure);
    if (!it) {
        wkcMutexUnlockPeer(gCookieMutex);
        return false;
    }

    rawCookies.reserveCapacity(it->length);

    while (CURLSHE_OK == curl_share_advance_cookies_iterator(m_curlShareHandle, it)) {
        String name = String::fromUTF8(it->cookie.name);
        String value = String::fromUTF8(it->cookie.value);
        // All domains are prefixed with a dot if they allow tailmatching. See also get_netscape_format() in cURL/lib/cookie.c.
        String ndomain;
        if (it->cookie.tailmatch && it->cookie.domain && it->cookie.domain[0] != '.')
            ndomain = "." + String::fromUTF8(it->cookie.domain);
        else
            ndomain = String::fromUTF8(it->cookie.domain);
        String npath = String::fromUTF8(it->cookie.path);
        double expires = static_cast<double>(it->cookie.expires) * 1000;
        bool isHttpOnly = it->cookie.httponly;
        bool isSecure = it->cookie.secure;
        bool isSession = !expires;

        // FIXME: 
        Cookie cookie;
        cookie.name = name;
        cookie.value = value;
        cookie.domain = ndomain;
        cookie.path = npath;
        cookie.created = MonotonicTime::now().secondsSinceEpoch().value();
        cookie.expires = expires;
        cookie.httpOnly = isHttpOnly;
        cookie.secure = isSecure;
        cookie.session = isSession;
        rawCookies.uncheckedAppend(cookie);
    }

    curl_share_delete_cookies_iterator(m_curlShareHandle, it);

    wkcMutexUnlockPeer(gCookieMutex);

    return true;
}

void ResourceHandleManager::deleteCookie(const String &domain, const String &path, bool secure, const String &name)
{
    wkcMutexLockPeer(gCookieMutex);

    struct CookieJarIterator* it = curl_share_create_cookies_iterator(m_curlShareHandle, domain.utf8().data(), path.utf8().data(), secure);
    if (!it) {
        wkcMutexUnlockPeer(gCookieMutex);
        return;
    }

    while (CURLSHE_OK == curl_share_advance_cookies_iterator(m_curlShareHandle, it)) {
        if (!equalIgnoringASCIICase(name, it->cookie.name))
            continue;
        // Set the cookie expired to delete.
        String expiredCookie = "Set-Cookie: " + name + "=; expires=Thu, 01-Jan-1970 00:00:00 GMT; path=" + path + "; domain=" + domain + ";";
        curl_share_set_cookie(m_curlShareHandle, domain.utf8().data(), path.utf8().data(), expiredCookie.utf8().data(), COOKIETYPE_HEADERLINE);
        break;
    }

    curl_share_delete_cookies_iterator(m_curlShareHandle, it);

    wkcMutexUnlockPeer(gCookieMutex);
}

//
// Reset Variables
//
void ResourceHandleManager::resetVariables()
{
    ResourceHandleManagerSSL::resetVariables();

    m_sharedInstance = 0;
}

void ResourceHandleManager::cancelByFrame(Frame* frame)
{
    int size = m_scheduledJobList.size();
    for (int i = size - 1; i >= 0; i--) {
        ResourceHandle* job = m_scheduledJobList[i];
        if (job->frame() == frame)
            job->cancel();
    }
    size = m_runningJobList.size();
    for (int i = size - 1; i >= 0; i--) {
        ResourceHandle* job = m_runningJobList[i];
        if (job->frame() == frame)
            job->cancel();
    }
}

void ResourceHandleManager::removeAllScheduledJobs()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    ResourceHandle* job;
    int num = m_scheduledJobList.size();

    while (num--) {
        job = m_scheduledJobList[num];
        m_scheduledJobList.remove(num);
        ResourceHandleInternal* d = job->getInternal();
        if (d && !d->m_cancelled && d->client()) {
            d->client()->didFail(job, ResourceError(String(), CURLE_GOT_NOTHING, URL(ParsedURLString, String(d->m_url)), String("aborted"), ResourceError::Type::Cancellation, 0));
            d->m_cancelled = true;
        }
        job->deref();
    }
}

//
// Running job control
//
void ResourceHandleManager::appendScheduledJob(ResourceHandle* job)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    ResourceHandleInternal* d = job->getInternal();

#if ENABLE(WKC_JAVASCRIPT_SEQUENTIAL)
    if (job->firstRequest().url().lastPathComponent().endsWith(".js"))
        job->getInternal()->m_isJSRequestatFirst = true;
#endif // WKC_JAVASCRIPT_SEQUENTIAL

    URL kurl = job->firstRequest().url();
    if (kurl.isLocalFile() || kurl.protocolIsData()) {
        d->m_permitSend = ResourceHandleInternal::PERMIT;
    }
    d->m_url = fastStrdup(kurl.string().latin1().data());
    d->m_composition = contentComposition(job);

    m_scheduledJobList.append(job);

    canPermitRequest(job);

    LOG_FUNCTION(ResourceHandleManager, "out");
}

bool ResourceHandleManager::removeScheduledJob(ResourceHandle* job)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    int size = m_scheduledJobList.size();
    for (int i = 0; i < size; i++) {
        if (job == m_scheduledJobList[i]) {
            m_scheduledJobList.remove(i);
            job->deref();
            LOG_FUNCTION(ResourceHandleManager, "out1: return true");
            return true;
        }
    }

#if ENABLE(WKC_HTTPCACHE)
    size = m_readCacheJobList.size();
    for (int i = 0; i < size; i++) {
        if (job == m_readCacheJobList[i]) {
            m_readCacheJobList.remove(i);
            job->deref();
            LOG_FUNCTION(ResourceHandleManager, "out2: return true");
            return true;
        }
    }
#endif

    LOG_FUNCTION(ResourceHandleManager, "out3: return false");
    return false;
}

ResourceHandle* ResourceHandleManager::shiftScheduledJob()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    ResourceHandle* job = 0;
    int i;
    int size = m_scheduledJobList.size();
    for (i = 0; i < size; i++) {
        job = m_scheduledJobList[i];

        ResourceHandleInternal* d = job->getInternal();
        if (!d)
            continue;
        if (d->m_cancelled) {
            m_scheduledJobList.remove(i);
            LOG_FUNCTION(ResourceHandleManager, "out1: return job:%p", job);
            return job;
        }

        switch (d->m_permitSend) {
        case ResourceHandleInternal::CANCEL:
            m_scheduledJobList.remove(i);
            return job;
        case ResourceHandleInternal::SUSPEND:
        case ResourceHandleInternal::ASKING:
            continue;
        case ResourceHandleInternal::PERMIT:
            break;
        default:
            continue;
        }

        if (d->m_fileLoading|| d->m_dataLoading || d->m_matchProxyFilter) {
            m_scheduledJobList.remove(i);
            LOG_FUNCTION(ResourceHandleManager, "out2: return job:%p", job);
            return job;
        }
        else {
            if (m_tryProxySingleConnect && m_runningJobList.size() > 0) {
                LOG_FUNCTION(ResourceHandleManager, "single connecting");
                continue;
            }

#if ENABLE(WKC_JAVASCRIPT_SEQUENTIAL)
            if (!m_jsSequential) {
                m_scheduledJobList.remove(i);
                LOG_FUNCTION(ResourceHandleManager, "out3: return job:%p", job);
                return job;
            }
            else if (!d->m_isJSRequestatFirst) {
                m_scheduledJobList.remove(i);
                LOG_FUNCTION(ResourceHandleManager, "out4: return job:%p", job);
                return job;
            }
#else
            m_scheduledJobList.remove(i);
            LOG_FUNCTION(ResourceHandleManager, "out5: return job:%p", job);
            return job;
#endif // WKC_JAVASCRIPT_SEQUENTIAL
        }
    }

    LOG_FUNCTION(ResourceHandleManager, "return null");
    return (ResourceHandle*)0;
}

//
// Running job control
//
void ResourceHandleManager::appendRunningJob(ResourceHandle* job)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    ResourceHandleInternal* d = job->getInternal();
    if (d) {
        d->m_isRunning = true;
#if ENABLE(WKC_JAVASCRIPT_SEQUENTIAL)
        if (d->m_isJSRequestatFirst)
            m_jsSequential = true;
#endif // WKC_JAVASCRIPT_SEQUENTIAL
    }

    LOG_FUNCTION(ResourceHandleManager, "Start Loading [%p]", job);
    m_runningJobList.append(job);

    LOG_FUNCTION(ResourceHandleManager, "out");
}

bool ResourceHandleManager::removeRunningJob(ResourceHandle* job)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    ResourceHandleInternal* d = job->getInternal();
    bool ret = false;
    int size = m_runningJobList.size();
    for (int i = 0; i < size; i++) {
        if (job == m_runningJobList[i]) {
            LOG_FUNCTION(ResourceHandleManager, "Finish Loading [%p]", job);
            m_runningJobList.remove(i);
            ret = true;
            break;
        }
    }

    if (ret && d) {
#if ENABLE(WKC_JAVASCRIPT_SEQUENTIAL)
        if (d->m_isJSRequestatFirst)
            m_jsSequential = false;
#endif // WKC_JAVASCRIPT_SEQUENTIAL
        if (d->m_isSSL)
            m_rhmssl->removeRunningJobSSL(job);
        d->m_isRunning = false;
    }

    LOG_FUNCTION(ResourceHandleManager, "return %s", ret ? "true" : "false");
    return ret;
}

ResourceHandle* ResourceHandleManager::shiftRunningJob()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    /* only it is called removeAllRunningJobs() */

    int size = m_runningJobList.size();
    ResourceHandle* job = 0;
    if (0 < size) {
        job = m_runningJobList[0];
        m_runningJobList.remove(0);
    }
    if (job) {
        ResourceHandleInternal* d = job->getInternal();
        if (d)
            d->m_isRunning = false;
    }

    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);
    return job;
}

void ResourceHandleManager::setHttpUserPassword(CURL* handle, const Credential& credential, ProtectionSpaceAuthenticationScheme authScheme)
{
    ASSERT(handle);

    if (credential.isEmpty())
        return;

    curl_easy_setopt(handle, CURLOPT_HTTPAUTH, ProtectionSpaceAuthSchemeTocURLType(authScheme));
    curl_easy_setopt(handle, CURLOPT_USERNAME, credential.user().utf8().data());
    curl_easy_setopt(handle, CURLOPT_PASSWORD, credential.password().utf8().data());
}

void ResourceHandleManager::setProxyUserPassword(CURL* handle, const Credential& credential, ProtectionSpaceAuthenticationScheme authScheme)
{
    ASSERT(handle);

    if (credential.isEmpty())
        return;

    curl_easy_setopt(handle, CURLOPT_PROXYAUTH,     ProtectionSpaceAuthSchemeTocURLType(authScheme));
    curl_easy_setopt(handle, CURLOPT_PROXYUSERNAME, credential.user().utf8().data());
    curl_easy_setopt(handle, CURLOPT_PROXYPASSWORD, credential.password().utf8().data());
}

void ResourceHandleManager::removeAllRunningJobs()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    ResourceHandle* job;

    while ((job = shiftRunningJob())) {
        ResourceHandleInternal* d = job->getInternal();
        if (d && d->m_handle) {
            if (d && !d->m_cancelled && d->client()) {
                d->client()->didFail(job, ResourceError(String(), CURLE_GOT_NOTHING, URL(ParsedURLString, String(d->m_url)), String("aborted"), ResourceError::Type::Cancellation, 0));
                d->m_cancelled = true;
            }
            callOnNetworkThread([this, job, d]() {
                if (d->m_handleIsInMulti) {
                    curl_multi_remove_handle(m_curlMultiHandle, d->m_handle);
                    m_curlHandlesInMulti--;
                    d->m_handleIsInMulti = false;
                    d->m_isRunning = false;
                }
                curl_easy_cleanup(d->m_handle);
                d->m_handle = 0;
                callOnMainThread([job, d] {
                    job->deref();
                });
            });
        } else {
            job->deref();
        }
    }

    LOG_FUNCTION(ResourceHandleManager, "out");
}

//
// can permit send callback
//
void ResourceHandleManager::canPermitRequest(ResourceHandle* job)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    if (!job)
        return;

    ResourceHandleInternal* d = job->getInternal();
    if (d->m_cancelled)
        return;

    if (!m_harmfulSiteFilter) {
        d->m_permitSend = ResourceHandleInternal::PERMIT;
        return;
    }

    if (ResourceHandleInternal::SUSPEND == d->m_permitSend) {
        d->m_permitSend = ResourceHandleInternal::ASKING;  // set asking

        WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(job);
        if (d->m_composition != WKC::EErrorContentComposition && fl) {
            int ret = fl->dispatchWillPermitSendRequest(job, d->m_url, d->m_composition, false, d->m_response);
            if (1 == ret) {
                d->m_permitSend = ResourceHandleInternal::PERMIT;
            }
            else if (-1 == ret) {
                d->m_permitSend = ResourceHandleInternal::CANCEL;
                d->m_cancelled = true;
            }
        }
        else {
            d->m_permitSend = ResourceHandleInternal::CANCEL;
            d->m_cancelled = true;
        }
    }

    LOG_FUNCTION(ResourceHandleManager, "out");
}

void ResourceHandleManager::permitRequest(void* handle, bool permit)
{
    LOG_FUNCTION(ResourceHandleManager, "handle:%p permit:%s", handle, permit ? "true" : "false");

    ResourceHandle* requset = (ResourceHandle*)handle;

    ResourceHandle* job;
    ResourceHandleInternal* d;
    bool timer = false;

    int size = m_scheduledJobList.size();
    if (0 == size)
        return;

    int i;
    for (i = 0; i < size; i++) {
        job = m_scheduledJobList[i];
        if (!job || job != requset)
            continue;

        d = job->getInternal();
        if (!d)
            continue;
        if (d->m_cancelled)
            continue;

        if (d->m_permitSend != ResourceHandleInternal::ASKING)
            continue;

        if (permit) {
            d->m_permitSend = ResourceHandleInternal::PERMIT;
        }
        else {
            d->m_permitSend = ResourceHandleInternal::CANCEL;
            d->m_cancelled = true;
        }
        timer = true;
    }

    if (timer && !m_downloadTimer.isActive()) {
        m_downloadTimer.startOneShot(Seconds(pollTimeSeconds));
    }

    LOG_FUNCTION(ResourceHandleManager, "out");
}

static bool shouldUseGet(const ResourceRequestBase& request, long httpCode)
{
    switch(httpCode){
    case 301:
    case 302:
        if (equalIgnoringASCIICase(request.httpMethod(), "POST"))
            return true;
        break;
    case 303:
        if (!equalIgnoringASCIICase(request.httpMethod(), "GET") &&
            !equalIgnoringASCIICase(request.httpMethod(), "HEAD")) {
            return true;
        }
        break;
    }
    return false;
}

//
// Do Redirect myself
//
bool ResourceHandleManager::didReceiveRedirect(ResourceHandle* job, long httpCode, const String&& location)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    ResourceHandleInternal* d = job->getInternal();

    if (m_redirectInWKC) {
        d->m_redirectCount++;
        if (d->m_redirectCount > d->m_redirectMaxCount) {
            return true;
        }
    }

    // modify and reuse the firstRequest for redirects.
    ResourceRequest& redirectedRequest = job->firstRequest();

    URL newURL = URL(job->firstRequest().url(), location);
    if (!newURL.hasFragmentIdentifier() && job->firstRequest().url().hasFragmentIdentifier()) {
        newURL.setFragmentIdentifier(job->firstRequest().url().fragmentIdentifier());
    }
    bool crossOrigin = !protocolHostAndPortAreEqual(d->m_firstRequest.url(), newURL);

    redirectedRequest.setURL(newURL);

    // delete previous CURLOPT_CUSTOMREQUEST setting (when previous request is POST with nothing have a body)
    curl_easy_setopt(d->m_handle, CURLOPT_CUSTOMREQUEST, NULL);

    // delete previous http authentication credential.
    clearAuthCredential(job);

    if (WKC::EInclusionContentComposition < d->m_composition) {
        d->m_firstRequest.setFirstPartyForCookies(newURL);
    }

    if (shouldUseGet(redirectedRequest, httpCode)) {
        redirectedRequest.setHTTPMethod("GET"_s);
        redirectedRequest.setHTTPBody(nullptr);
        redirectedRequest.clearContentLength();
        redirectedRequest.clearRequestBodyHeaders();
    }

    if (!redirectedRequest.url().protocolIs("https") && protocolIs(redirectedRequest.httpReferrer(), "https"))
        redirectedRequest.clearHTTPReferrer();

    if (crossOrigin) {
        redirectedRequest.clearHTTPAuthorization();
        redirectedRequest.clearHTTPOrigin();
        if (d->m_isSynchronous)
            reinterpret_cast<SynchronousLoaderClient*>(d->client())->setAllowStoredCredentials(false);
    }

    if (d->client() && !d->m_isSynchronous)
        d->client()->willSendRequestAsync(job, ResourceRequest(redirectedRequest), ResourceResponse(d->m_response), [](ResourceRequest&& request) {});

    if (d->m_cancelled) {
        LOG_FUNCTION(ResourceHandleManager, "canceled");
        return false;
    }

    d->m_response.setHTTPHeaderField(String("Location"), String());

    String x_frame_options = d->m_response.httpHeaderField(String("X-Frame-Options"));
    if (!x_frame_options.isEmpty()) {
        d->m_response.setHTTPHeaderField(String("X-Frame-Options"), String());
    }
    String content_security_policy = d->m_response.httpHeaderField(String("Content-Security-Policy"));
    if (!content_security_policy.isEmpty()) {
        d->m_response.setHTTPHeaderField(String("Content-Security-Policy"), String());
    }

    if (m_redirectInWKC) {
        if (d->m_url) {
            fastFree(d->m_url);
            d->m_url = 0;
        }
        if (d->m_urlhost) {
            fastFree(d->m_urlhost);
            d->m_urlhost = 0;
        }
        d->m_formDataStream.refresh();
        long httpver = CURL_HTTP_VERSION_1_1;
        if (equalIgnoringASCIICase(newURL.protocol(), "https")) {
            d->m_isSSL = true;
            d->m_SSLHandshaked = false;
            if (m_useHTTP2)
                httpver = CURL_HTTP_VERSION_2TLS;
        }
        curl_easy_setopt(d->m_handle, CURLOPT_HTTP_VERSION, httpver);

        d->m_doRedirectChallenge = true;
    }
    else {
        /* redirect in cURL */
        if (d->m_url) {
            fastFree(d->m_url);
        }
        d->m_url  = fastStrdup(newURL.string().latin1().data());
        if (d->m_urlhost) {
            fastFree(d->m_urlhost);
        }
        d->m_urlhost = fastStrdup(hostAndPort(newURL).latin1().data());

        if (equalIgnoringASCIICase(newURL.protocol(), "https")) {
            d->m_isSSL = true;
            d->m_SSLHandshaked = false;
            m_rhmssl->initializeHandleSSL(job);
        }

#if ENABLE(WKC_HTTPCACHE)
        curl_easy_setopt(d->m_handle, CURLOPT_TIMECONDITION, CURL_TIMECOND_NONE);
        if (!m_httpCache.disabled() && job->firstRequest().cachePolicy() != ResourceRequestCachePolicy::ReloadIgnoringCacheData) {
            HTTPCachedResource *resource = m_httpCache.resourceForURL(newURL);
            if (resource) {
                const String& lastModified = resource->lastModifiedHeader();
                const String& etag = resource->eTagHeader();
                if (!lastModified.isEmpty() || !etag.isEmpty()) {
                    job->getInternal()->m_utilizedHTTPCache = true;
                }
                if (!lastModified.isEmpty()) {
                    double s = parseHTTPDate(lastModified).value().secondsSinceEpoch().value();
                    if (isfinite(s)) {
                        curl_easy_setopt(d->m_handle, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
                        curl_easy_setopt(d->m_handle, CURLOPT_TIMEVALUE, (wkc_time64_t)(s));
                    }
                }
            }
        }
#endif // ENABLE(WKC_HTTPCACHE)
    }

    LOG_FUNCTION(ResourceHandleManager, "out");
    return true;
}

unsigned ResourceHandleManager::maximumHTTPConnectionCountPerHostWindow()
{
    static const unsigned unlimitedRequestCount = 256; // Must unlimitedRequestCount << WKC_MAX_MUTEXES
    return unlimitedRequestCount;
}

//
// Check this content is inclusion or root
//
int ResourceHandleManager::contentComposition(ResourceHandle* job)
{
    LOG_FUNCTION(ResourceHandleManager, "job:%p", job);

    const Frame* frame = job->frame();
    if (!frame || !frame->loader().documentLoader() || !frame->page())
        return WKC::EErrorContentComposition;
    if (!job->mainFrame() && !job->frameloaderclient())
        return WKC::EErrorContentComposition;

    FrameLoader& frameloader = frame->loader();
    DocumentLoader* documentloader = frameloader.documentLoader();
    if (!documentloader)
        return WKC::EErrorContentComposition;

#if ENABLE(NETSCAPE_PLUGIN_API)
    if (documentloader->isLoadingPlugIns()) {
        return WKC::EOtherContentComposition;
    }
#endif

    if (job->firstRequest().requester() == ResourceRequest::Requester::XHR)
        return WKC::EInclusionContentComposition;

    Page* page = frame->page();
    if (page && frame==&page->mainFrame()) {
        if (documentloader->isLoadingSubresources())
            return WKC::EInclusionContentComposition;
        else
            return WKC::ERootFrameRootContentComposition;
    }
    else {
        if (documentloader->isLoadingSubresources())
            return WKC::EInclusionContentComposition;
        else
            return WKC::ESubFrameRootContentComposition;
    }

// for debug
// DP(("  => isLoadingMainResource(): %s", ((loader->isLoadingMainResource())?"true":"false")));
// DP(("  => isLoadingSubresources(): %s", ((loader->isLoadingSubresources())?"true":"false")));
// DP(("  => isLoadingMainFrame(): %s", ((loader->frameLoader()->isLoadingMainFrame())?"true":"false")));
// DP(("  => subframeIsLoading() : %s", ((loader->frameLoader()->subframeIsLoading())?"true":"false")));
    LOG_FUNCTION(ResourceHandleManager, "out");
    return WKC::EInclusionContentComposition;
}

bool ResourceHandleManager::oneShotDownloadTimer(void)
{
    if (!m_downloadTimer.isActive()) {
        m_downloadTimer.startOneShot(Seconds(pollTimeSeconds));
        return true;
    }
    return false;
}

//
// SSL wrapper
//
void ResourceHandleManager::SSLEnableProtocols(unsigned int versions)
{
    m_rhmssl->SSLEnableProtocols(versions);
}

void ResourceHandleManager::SSLEnableOnlineCertChecks(bool enableOCSP, bool enableCRLDP)
{
    m_rhmssl->SSLEnableOnlineCertChecks(enableOCSP, enableCRLDP);
}

void* ResourceHandleManager::SSLRegisterRootCA(const char* cert, int cert_len)
{
    return m_rhmssl->SSLRegisterRootCA(cert, cert_len);
}

int ResourceHandleManager::SSLUnregisterRootCA(void* certid)
{
    return m_rhmssl->SSLUnregisterRootCA(certid);
}

void* ResourceHandleManager::SSLRegisterRootCAByDER(const char* cert, int cert_len)
{
    return m_rhmssl->SSLRegisterRootCAByDER(cert, cert_len);
}

void ResourceHandleManager::SSLRootCADeleteAll(void)
{
    m_rhmssl->SSLRootCADeleteAll();
}

void* ResourceHandleManager::SSLRegisterCRL(const char* crl, int crl_len)
{
    return m_rhmssl->SSLRegisterCRL(crl, crl_len);
}

int ResourceHandleManager::SSLUnregisterCRL(void* crlid)
{
    return m_rhmssl->SSLUnregisterCRL(crlid);
}

void ResourceHandleManager::SSLCRLDeleteAll(void)
{
    m_rhmssl->SSLCRLDeleteAll();
}

void* ResourceHandleManager::SSLRegisterClientCert(const unsigned char* pkcs12, int pkcs12_len, const unsigned char* pass, int pass_len)
{
    return m_rhmssl->SSLRegisterClientCert(pkcs12, pkcs12_len, pass, pass_len);
}

void* ResourceHandleManager::SSLRegisterClientCertByDER(const unsigned char* cert, int cert_len, const unsigned char* key, int key_len)
{
    return m_rhmssl->SSLRegisterClientCertByDER(cert, cert_len, key, key_len);
}

int ResourceHandleManager::SSLUnregisterClientCert(void* certid)
{
    return m_rhmssl->SSLUnregisterClientCert(certid);
}

void ResourceHandleManager::SSLClientCertDeleteAll(void)
{
    m_rhmssl->SSLClientCertDeleteAll();
}

bool ResourceHandleManager::SSLRegisterBlackCert(const char* in_issuerCommonName, const char* serialNumber)
{
    return m_rhmssl->SSLRegisterBlackCert(in_issuerCommonName, serialNumber);
}

bool ResourceHandleManager::SSLRegisterBlackCertByDER(const char* cert, int cert_len)
{
    return m_rhmssl->SSLRegisterBlackCertByDER(cert, cert_len);
}

void ResourceHandleManager::SSLBlackCertDeleteAll(void)
{
    m_rhmssl->SSLBlackCertDeleteAll();
}

bool ResourceHandleManager::isCertificateBlack(void* data)
{
    return m_rhmssl->isCertificateBlack(data);
}

bool ResourceHandleManager::SSLRegisterUntrustedCertByDER(const char* cert, int cert_len)
{
    return m_rhmssl->SSLRegisterUntrustedCertByDER(cert, cert_len);
}

bool ResourceHandleManager::SSLRegisterEVSSLOID(const char *issuerCommonName, const char *OID, const char *sha1FingerPrint, const char *SerialNumber)
{
    return m_rhmssl->SSLRegisterEVSSLOID(issuerCommonName, OID, sha1FingerPrint, SerialNumber);
}

void ResourceHandleManager::SSLEVSSLOIDDeleteAll(void)
{
    m_rhmssl->SSLEVSSLOIDDeleteAll();
}

const char** ResourceHandleManager::getServerCertChain(const char* url, int& outCertNum)
{
    return m_rhmssl->getServerCertChain(url, outCertNum);
}

void ResourceHandleManager::freeServerCertChain(const char** chain, int num)
{
    m_rhmssl->freeServerCertChain(chain, num);
}

void ResourceHandleManager::setAllowServerHost(const char *host_w_port)
{
    m_rhmssl->setAllowServerHost(host_w_port);
}

bool ResourceHandleManager::isMatchProxyFilter(const String& host)
{
    return m_proxyFilters.isMatchProxyFilter(host);
}

//
//  Proxy Filter
//
ResourceHandleManager::ProxyFilter::ProxyFilter()
    : m_proxyFilters(0)
{
}

ResourceHandleManager::ProxyFilter::~ProxyFilter()
{
    allClean();
    Filter* filter;
    int num = m_proxyFilters.size();
    while (num--) {
        filter = m_proxyFilters[num];
        m_proxyFilters.remove(num);
        delete filter;
    }
    if (!m_proxyFilters.isEmpty())
        m_proxyFilters.clear();
}

// public
void ResourceHandleManager::ProxyFilter::setProxyFilters(const String& filters)
{
    allClean();

    if (filters.isNull()) {
        return;
    }

    Vector<String> proxyFilters;
    filters.convertToASCIILowercase().split(',', proxyFilters);

    int size = proxyFilters.size();
    for (int i = 0; i < size; i++) {
        String filter = proxyFilters[i].stripWhiteSpace();
        if (filter.contains(' '))
            continue;

        if (setDomain(filter)) {
            continue;
        }
        else if (setIPAddress(filter)) {
            continue;
        }
        else if (setIPNetwork(filter)) {
            continue;
        }
        else if (setFQDN(filter)) {
            continue;
        }
    }
}

bool ResourceHandleManager::ProxyFilter::isMatchProxyFilter(const String& host)
{
    String lowerHost = host.convertToASCIILowercase();
    int hostLen = host.length();

    // remove IPv6 []
    if (host.startsWith("[") && host.endsWith("]")) {
        lowerHost = host.substring(1, host.length() - 2);
        hostLen = lowerHost.length();
    }

    int size = m_proxyFilters.size();
    for (int i = 0; i < size; i++) {
        Filter* filter = m_proxyFilters[i];
        if (FQDN == filter->m_type || IPAddress == filter->m_type) {
            if (filter->m_filter == lowerHost) {
                return true;
            }
        }
        else if (Domain == filter->m_type) {
            int filterLen = filter->m_filter.length();
            if (hostLen <= filterLen) {
                continue;
            }
            if (filter->m_filter == lowerHost.right(filterLen)) {
                return true;
            }
        }
        else {
            //  filter.m_type is IPMask
            unsigned char ipaddr[16];
            if (0 == convertIPStringToChar(host, ipaddr)) {
                continue;
            }
            bool ret = true;
            for (int j = 0; j < 16; j++) {
                if ((ipaddr[j] & filter->m_maskd[j]) != filter->m_maskd[j]) {
                    ret = false;
                    break;
                }
            }
            if (ret) {
                return true;
            }
        }
    }

    return false;
}

// private
void ResourceHandleManager::ProxyFilter::allClean()
{
    Filter* filter;
    int num = m_proxyFilters.size();
    while (num--) {
        filter = m_proxyFilters[num];
        m_proxyFilters.remove(num);
        delete filter;
    }
    if (!m_proxyFilters.isEmpty())
        m_proxyFilters.clear();
}

bool ResourceHandleManager::ProxyFilter::append(Filter* newfilter)
{
    int size = m_proxyFilters.size();
    for (int i = 0; i < size; i++) {
        Filter* filter = m_proxyFilters[i];
        if ((filter->m_type == newfilter->m_type)
            && (filter->m_filter == newfilter->m_filter)
            && (!memcmp((const void*)filter->m_maskd, (const void*)newfilter->m_maskd, 16))) {
            // has the same filter already
            return false;
        }
    }

    m_proxyFilters.append(newfilter);
    return true;
}

bool ResourceHandleManager::ProxyFilter::setFQDN(String filter)
{
    if (!filter.contains('.'))
        return false;

    if (hasProhibitCharactor(filter))
        return false;

    if (filter.endsWith("."))
        filter.left(filter.length() - 1);

    char buf[512];
    buf[0] = '.';
#ifdef WKC_ENABLE_I18N_IDN_SUPPORT
    const UChar* c = filter.charactersWithNullTermination().data();
    int ret = wkcI18NIDNfromUnicodePeer(c, filter.length(), (unsigned char*)(buf + 1), sizeof(buf) - 2);
#else
    CString cs = filter.utf8();
    const char* c = cs.data();
    int ret = 0;
    if (c) {
        strncpy(buf + 1, c, 510);
        ret = 1;
    }
#endif // WKC_ENABLE_I18N_IDN_SUPPORT
    if (c && 0 < ret) {
        Filter *newfilter = new Filter(FQDN, buf+1, NULL);
        if (newfilter && !append(newfilter)) {
            delete newfilter;
        }
    }

    Filter *newfilter = new Filter(Domain, buf, NULL);
    if (newfilter && !append(newfilter)) {
        delete newfilter;
    }

    return true;
}

bool ResourceHandleManager::ProxyFilter::setDomain(String filter)
{
    if (!filter.startsWith("."))
        return false;

    if (hasProhibitCharactor(filter))
        return false;

    if (filter.endsWith("."))
        filter.left(filter.length() - 1);

    char buf[512];
    buf[0] = '.';
#ifdef WKC_ENABLE_I18N_IDN_SUPPORT
    const UChar* r = filter.right(filter.length() - 1).charactersWithNullTermination().data();
    int ret = wkcI18NIDNfromUnicodePeer(r, filter.length() - 1, (unsigned char*)(buf + 1), sizeof(buf) - 2);
#else
    CString c = filter.right(filter.length() - 1).utf8();
    const char* r = c.data();
    int ret = 0;
    if (r) {
        strncpy(buf + 1, r, 510);
        ret = 1;
    }
#endif // WKC_ENABLE_I18N_IDN_SUPPORT
    if (r && 0 < ret) {
        Filter *newfilter = new Filter(Domain, buf, NULL);
        if (newfilter && !append(newfilter)) {
            delete newfilter;
        }
    }

    Filter *newfilter = new Filter(FQDN, buf+1, NULL);
    if (newfilter && !append(newfilter)) {
        delete newfilter;
    }

    return true;
}

bool ResourceHandleManager::ProxyFilter::setIPAddress(String filter)
{
    if (filter.contains('/'))
        return false;

    unsigned char ipaddr[16];
    int ipver = convertIPStringToChar(filter, ipaddr);
    if (4 != ipver && 6 != ipver)
        return 0;

    // remove IPv6 []
    if (filter.startsWith("[") && filter.endsWith("]")) {
        filter = filter.substring(1, filter.length() - 2);
    }

    Filter *newfilter = new Filter(IPAddress, filter, NULL);
    if (newfilter && !append(newfilter)) {
        delete newfilter;
    }

    return true;
}

bool ResourceHandleManager::ProxyFilter::setIPNetwork(String filter)
{
    Vector<String> IPs;
    unsigned char ipaddr[16];

    if (!filter.contains('/'))
        return false;

    filter.split('/', IPs);
    if (2 != IPs.size())
        return false;

    int ipver = convertIPStringToChar(IPs[0], ipaddr);
    if (4 != ipver && 6 != ipver)
        return false;

    bool isOK = false;
    int mask = IPs[1].toInt(&isOK);
    if (!isOK)
        return false;
    if (4 == ipver) {
        if (mask < 8 || 32 < mask)
            return false;
    }
    else {
        // 6 == ipver
        if (mask != 48 && mask != 64)
            return false;
    }

    for (int i = 0; i < 16; i++) {
        if (mask < 0)  mask = 0;
        if (mask < 8)  ipaddr[i] = (unsigned char)(ipaddr[i] & (0xff << (8 - mask)));
        mask -= 8;
    }

    Filter *newfilter = new Filter(IPMask, "", ipaddr);
    if (newfilter && !append(newfilter)) {
        delete newfilter;
    }

    return true;
}

#define CONV(c)  (((c) & 0x10 ? (c) : (c) + 9) & 0x0f)

int ResourceHandleManager::ProxyFilter::convertIPStringToChar(String host, unsigned char *ip)
{
    // host must be lower

    int hostLen = host.length();
    Vector<String> IPs;

    memset(ip, 0x0, 16);

    if (host.contains(':')) {
        // Maybe IPv6
        // remove IPv6 []
        if (host.startsWith("[") && host.endsWith("]")) {
            host = host.substring(1, host.length() - 2);
        }
        host.replace("::", ":#:");
        host.split(':', IPs);
        int size = IPs.size();
        if (8 < size)
            return 0;
        int pos = 0;
        for (int i = 0; i < size; i++) {
            String part = IPs[i];
            int len = part.length();
            if (4 < len)
                return 0;
            if ("#" == part) {
                pos = 8 - (size - i) + 1;
                continue;
            }
            if (!IPs[i].isAllASCII())
                return 0;
            ip[pos*2 + 0] = ((3 < len) ? (CONV(*(part.substring(len - 4, 1).utf8().data())) << 4) : 0)
                            | ((2 < len) ?  CONV(*(part.substring(len - 3, 1).utf8().data())) : 0);
            ip[pos*2 + 1] = ((1 < len) ? (CONV(*(part.substring(len - 2, 1).utf8().data())) << 4) : 0)
                            | (CONV(*(part.substring(len - 1, 1).utf8().data())));
            pos++;
        }
        return 6;
    }
    else if (host.contains('.')) {
        // Maybe IPv4
        if (hostLen < 7)
            return 0;
        host.split('.', IPs);
        int size = IPs.size();
        if (4 != size)
            return 0;
        for (int i = 0; i < size; i++) {
            bool isOK = false;
            int num = IPs[i].toInt(&isOK);
            if (!isOK)
                return false;
            if (num < 0 || 255 < num)
                return false;
            ip[i] = (unsigned char)(num);
        }
        return 4;
    }

    return 0;
}

bool ResourceHandleManager::ProxyFilter::hasProhibitCharactor(String filter)
{
    // Prohibit Charactor Rules
    Vector<String> Parts;
    filter.split('.', Parts);
    int size = Parts.size();
    if (size < 2)
        return true;
    for (int i = 0; i < size; i++) {
        if (Parts[i].startsWith("-"))
            return true;
        if (Parts[i].endsWith("-"))
            return true;
        int len = Parts[i].length();
        for (int j = 0; j < len; j++) {
            String C = Parts[i].substring(j, 1);
            // following charactor should not use FQDN
            if (("!" == C) || ("\"" == C) || ("!" == C) || ("$" == C) || ("%" == C)
                || ("&" == C) || ("'" == C) || ("(" == C) || (")" == C) || ("*" == C)
                || ("+" == C) || ("," == C) || ("." == C) || ("/" == C) || (":" == C)
                || (";" == C) || ("<" == C) || ("=" == C) || (">" == C) || ("?" == C)
                || ("@" == C) || ("[" == C) || ("\\" == C) || ("]" == C) || ("^" == C)
                || ("`" == C) || ("{" == C) || ("|" == C) || ("}" == C) || ("~" == C)) {
                return true;
            }
        }
    }
    return false;
}

#if ENABLE(WKC_HTTPCACHE)
void ResourceHandleManager::clearHTTPCache()
{
    if (m_httpCache.disabled())
        return;

    ResourceHandle* job;
    int num = m_readCacheJobList.size();

    m_readCacheTimer.stop();
    while (num--) {
        job = m_readCacheJobList[num];
        m_readCacheJobList.remove(num);
        job->deref();
    }

    HTTPCachedResource *resource;
    num = m_writeCacheList.size();

    m_writeCacheTimer.stop();
    while (num--) {
        resource = m_writeCacheList[num];
        m_writeCacheList.remove(num);
        delete resource;
    }

    m_httpCache.removeAll();
    m_httpCache.purgeBySize(0);
    m_httpCache.writeFATFile();
    m_httpCache.readFATFile();
}

HTTPCachedResource* ResourceHandleManager::updateCacheResource(URL &url, SharedBuffer* resourceData, ResourceRequest& request, ResourceResponse &response, bool noStore, Vector<std::pair<String, String>> varyingRequestHeaders, bool serverpush)
{
    HTTPCachedResource *resource = m_httpCache.resourceForURL(url);
    if (resource) {
        if (noStore) {
            m_httpCache.remove(resource);
            return NULL;
        } else {
            m_httpCache.updateCachedResource(resource, resourceData, request, response, varyingRequestHeaders, serverpush);
            m_httpCache.detach(resource);
            return resource;
        }
    }

    for(int num = 0; num < m_writeCacheList.size(); num++) {
        if (m_httpCache.equalHTTPCachedResourceURL(m_writeCacheList[num], url)) {
            if (noStore) {
                m_writeCacheList.remove(num);
                return NULL;
            }
            resource = m_writeCacheList[num];
            m_httpCache.updateCachedResource(resource, resourceData, request, response, varyingRequestHeaders, serverpush);
            m_writeCacheList.remove(num);
            break;
        }
    }
    return resource;
}


bool ResourceHandleManager::addHTTPCache(ResourceHandle *handle, URL &url, SharedBuffer* resourceData, ResourceResponse &response)
{
    if (!resourceData)
        return false;

    if (!url.protocolIsInHTTPFamily())
        return false;

    if (handle->firstRequest().cachePolicy() == ResourceRequestCachePolicy::DoNotUseAnyCache) {
        return false;
    }
    if (handle->firstRequest().cachePolicy() == ResourceRequestCachePolicy::UseProtocolCachePolicy) {
        /*
         * RFC2616 Hypertext Transfer Protocol -- HTTP/1.1
         * 9.2 OPTIONS Responses to this method are not cacheable.
         */
        if (handle->firstRequest().httpMethod() == "OPTIONS") {
            return false;
        }
    }

    ResourceHandleInternal* d = handle->getInternal();

    ResourceRequest& request = handle->firstRequest();
    bool noStore = response.cacheControlContainsNoStore();
    bool serverpush = d->m_serverpush;
    Vector<std::pair<String, String>> varyingRequestHeaders;

    // We don't cache it if the Vary header contains values other than Accept, Accept-Charset, Accept-Encoding, Accept-Language and User-Agent.
    if (response.httpHeaderFields().contains(HTTPHeaderName::Vary)) {
        varyingRequestHeaders = collectVaryingRequestHeaders(handle->firstRequest(), response);
        for (auto& varyingRequestHeader : varyingRequestHeaders) {
            if (!equalIgnoringASCIICase(varyingRequestHeader.first, "Accept")
             && !equalIgnoringASCIICase(varyingRequestHeader.first, "Accept-Charset")
             && !equalIgnoringASCIICase(varyingRequestHeader.first, "Accept-Encoding")
             && !equalIgnoringASCIICase(varyingRequestHeader.first, "Accept-Language")
             && !equalIgnoringASCIICase(varyingRequestHeader.first, "User-Agent")) {
                return false;
            }
        }
    }

    if (200 <= response.httpStatusCode() && response.httpStatusCode() < 400) {
        if (response.httpHeaderFields().contains(HTTPHeaderName::Location)) {
            String location = response.httpHeaderField(HTTPHeaderName::Location);
            if (!location.isEmpty()) {
                URL newURL = URL(handle->firstRequest().url(), location);
                if (!newURL.hasFragmentIdentifier() && handle->firstRequest().url().hasFragmentIdentifier()) {
                    newURL.setFragmentIdentifier(handle->firstRequest().url().fragmentIdentifier());
                }
                if (newURL.host() == handle->firstRequest().url().host()) {
                    noStore = true;
                    updateCacheResource(newURL, NULL, request, response, noStore, varyingRequestHeaders, false); // remove
                }
            }
        }
        if (response.httpHeaderFields().contains(String("Content-Location"))) {
            String location = response.httpHeaderField(String("Content-Location"));
            if (!location.isEmpty()) {
                URL newURL = URL(handle->firstRequest().url(), location);
                if (!newURL.hasFragmentIdentifier() && handle->firstRequest().url().hasFragmentIdentifier()) {
                    newURL.setFragmentIdentifier(handle->firstRequest().url().fragmentIdentifier());
                }
                if (newURL.host() == handle->firstRequest().url().host()) {
                    noStore = true;
                    updateCacheResource(newURL, NULL, request, response, noStore, varyingRequestHeaders, false); // remove
                }
            }
        }
        if (!isSafeMethod(handle)) {
            noStore = true;
        }
    }

    HTTPCachedResource *resource = updateCacheResource(url, resourceData, request, response, noStore, varyingRequestHeaders, serverpush);
    if (!resource && !noStore) {
        resource = m_httpCache.createHTTPCachedResource(url, resourceData, request, response, varyingRequestHeaders, serverpush);
        if (!resource)
            return false;
    }
    if (noStore)
        return false;

    // reset used flag
    resource->setUsed(false);

    HTTPCache::appendResourceInSizeOrder(m_writeCacheList, resource);

    if (!m_writeCacheTimer.isActive()) {
        m_writeCacheTimer.startOneShot(Seconds(pollTimeSeconds));
    }

    return true;
}

HTTPCachedResource* ResourceHandleManager::getHTTPCache(URL &url)
{
    HTTPCachedResource *resource = m_httpCache.resourceForURL(url);
    if (!resource) {
        for(int num = 0; num < m_writeCacheList.size(); num++) {
            if (m_httpCache.equalHTTPCachedResourceURL(m_writeCacheList[num], url)) {
                resource = m_writeCacheList[num];
                break;
            }
        }
    }
    return resource;
}

bool ResourceHandleManager::canReadHTTPCache(ResourceHandle* job, HTTPCachedResource* existingResource)
{
    if (m_httpCache.disabled()) {
        return false;
    }
    if (!existingResource) {
        return false;
    }
    if (job->firstRequest().cachePolicy() == ResourceRequestCachePolicy::ReloadIgnoringCacheData
        || job->firstRequest().cachePolicy() == ResourceRequestCachePolicy::DoNotUseAnyCache) {
        return false;
    }
    if (job->firstRequest().httpHeaderFields().contains(HTTPHeaderName::Range)) {
        return false;
    }
#if ENABLE(WKC_HTTPCACHE_EXCLUDE_ROOT_CONTENT)
    if (job->firstRequest().url().protocolIs("https") && contentComposition(job) == WKC::ERootFrameRootContentComposition) {
        return false;
    }
#endif
    if (!isSafeMethod(job)) {
        return false;
    }
    if (!existingResource->varyHeaderValuesMatch(job->firstRequest())) {
        return false;
    }
    return true;
}

void ResourceHandleManager::makeConditionalRequest(ResourceHandle *job, HTTPCachedResource* resource)
{
    // get from a network unless the cached resource is up-to-date.
    if (!resource->lastModifiedHeader().isEmpty())
        job->firstRequest().setHTTPHeaderField(String("If-Modified-Since"), resource->lastModifiedHeader());
    if (!resource->eTagHeader().isEmpty())
        job->firstRequest().setHTTPHeaderField(String("If-None-Match"), resource->eTagHeader());
    job->getInternal()->m_utilizedHTTPCache = true;
}

void ResourceHandleManager::scheduleLoadResourceFromHTTPCache(ResourceHandle *job)
{
    // read from the cached resource
    job->ref();
    ResourceHandleInternal* d = job->getInternal();
    d->m_composition = contentComposition(job);
    m_readCacheJobList.append(job);
    if (!m_readCacheTimer.isActive()) {
        m_readCacheTimer.startOneShot(Seconds(pollTimeSeconds));
    }
}

void ResourceHandleManager::readCacheTimerCallback()
{
    ResourceHandle* job;
    ResourceHandleClient* client;

    if (m_readCacheJobList.size() == 0)
        return;

    job = m_readCacheJobList.first();
    m_readCacheJobList.remove(0);
    URL kurl = job->firstRequest().url();

    HTTPCachedResource *resource = getHTTPCache(kurl);
    ResourceHandleInternal* d = job->getInternal();
    if (!d->m_url)
        d->m_url = fastStrdup(kurl.string().latin1().data());
    const Frame* frame = job->frame();
    WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(job);

    client = d->client();
    if (!client)
        goto cancel;

    client->wkcRef();

    if (fl && resource) {
        // this resource has been used
        resource->setUsed(true);
        if (!fl->dispatchWillReceiveData(job, resource->resourceSize()))
            goto cancel;
        if (!client->willReceiveData(job, resource->resourceSize()))
            goto cancel;
        d->m_response.setResourceHandle(job);
        d->m_response.setURL(kurl);
        d->m_response.setMimeType(resource->mimeType());
        d->m_response.setExpectedContentLength(resource->expectedContentLength());
        d->m_response.setTextEncodingName(resource->textEncodingName());
        d->m_response.setSuggestedFilename(resource->suggestedFilename());
        d->m_response.setHTTPStatusCode(resource->httpStatusCode());
        d->m_response.setHTTPStatusText(resource->httpStatusText());
        d->m_response.setHTTPHeaderField(String("Last-Modified"), resource->lastModifiedHeader());
        d->m_response.setSource(ResourceResponse::Source::DiskCache);
        if (!resource->accessControlAllowOriginHeader().isEmpty()) {
            d->m_response.setHTTPHeaderField(String("Access-Control-Allow-Origin"), resource->accessControlAllowOriginHeader());
        }
        d->m_response.setHTTPHeaderField(HTTPHeaderName::ContentType, resource->contentTypeHeader());
        client->didReceiveResponseAsync(job, ResourceResponse(d->m_response), [](){});
        if (!d->client() || d->m_cancelled)
            goto cancel;
        d->m_response.setResponseFired(true);
        if (frame!=job->frame()) {
            goto cancel;
        }
        d->m_isSSL = resource->isSSL();
        d->m_isEVSSL = resource->isEVSSL();
        d->m_secureState = resource->secureState();
        d->m_secureLevel = resource->secureLevel();
        WKC::FrameLoaderClientWKC* fl = frameloaderclientwkc(job);
        if (!fl || !fl->dispatchWillReceiveData(job, resource->contentLength()))
            goto cancel;

        if (!client->willReceiveData(job, resource->contentLength()))
            goto cancel;
        char *buf;
        buf = (char*)fastMalloc(resource->contentLength());
        if (buf) {
            if (m_httpCache.read(resource, buf)) {
                client->didReceiveData(job, buf, resource->contentLength(), 0);
                if (!d->client()) {
                    fastFree(buf);
                    goto cancel;
                }
                client->didFinishLoading(job);
                fl->didRestoreFromHTTPCache(job, kurl);
            } else {
                client->didFail(job, ResourceError(String(), CURLE_READ_ERROR, URL(ParsedURLString, String(d->m_url)), String("cache read error")));
                m_httpCache.removeAll();
            }
            fastFree(buf);
        }
    } else {
        goto cancel;
    }

    client->wkcDeref();

    job->deref();

    if (m_readCacheJobList.size() == 0)
        return;

    if (!m_readCacheTimer.isActive()) {
        m_readCacheTimer.startOneShot(Seconds(pollTimeSeconds));
    }

    return;

cancel:
    if (resource) {
        // reset used flag
        resource->setUsed(false);
    }
    if (!d->m_cancelled) {
        if (d->m_handle)
            cancel(job);
        if (client)
            client->didFail(job, ResourceError(String(), CURLE_READ_ERROR, URL(ParsedURLString, String(d->m_url)), String("cache read error")));
    }

    if (client)
        client->wkcDeref();

    job->deref();

    if (m_readCacheJobList.size() == 0)
        return;

    if (!m_readCacheTimer.isActive()) {
        m_readCacheTimer.startOneShot(Seconds(pollTimeSeconds));
    }
}

void ResourceHandleManager::writeCacheTimerCallback()
{
    if (m_writeCacheList.size() == 0)
        return;

    if (m_scheduledJobList.size() > 0 || m_runningJobList.size() > 0) {
        m_writeCacheTimer.startOneShot(Seconds(pollTimeSeconds));
        return;
    }

    HTTPCachedResource *resource;
    resource = m_writeCacheList.first();

    bool write = m_httpCache.write(resource);
    m_writeCacheList.remove(0);
    if (!write) {
        delete resource;
    }

    if (m_writeCacheList.size() == 0) {
        m_httpCache.writeFATFile();
        return;
    }

    if (!m_writeCacheTimer.isActive()) {
        m_writeCacheTimer.startOneShot(Seconds(pollTimeSeconds));
    }
}

bool ResourceHandleManager::isSafeMethod(ResourceHandle* job)
{
    if (equalIgnoringASCIICase(job->firstRequest().httpMethod(), "GET")
        || equalIgnoringASCIICase(job->firstRequest().httpMethod(), "HEAD")
        || equalIgnoringASCIICase(job->firstRequest().httpMethod(), "OPTIONS")
        || equalIgnoringASCIICase(job->firstRequest().httpMethod(), "TRACE")) {
        return true;
    }
    return false;
}

void ResourceHandleManager::resetHTTPCache()
{
    ResourceHandle* job;
    int num = m_readCacheJobList.size();

    m_readCacheTimer.stop();
    while (num--) {
        job = m_readCacheJobList[num];
        m_readCacheJobList.remove(num);
        job->deref();
    }

    HTTPCachedResource *resource;
    num = m_writeCacheList.size();

    m_writeCacheTimer.stop();
    while (num--) {
        resource = m_writeCacheList[num];
        m_writeCacheList.remove(num);
        delete resource;
    }

    m_httpCache.reset();
}

void
ResourceHandleManager::dumpHTTPCacheResourceList()
{
    httpCache()->dumpResourceList();
}

void
ResourceHandleManager::setCanCacheToDiskCallback(WKC::CanCacheToDiskProc proc)
{
    m_httpCache.setCanCacheToDiskCallback(proc);
}

#endif // ENABLE(WKC_HTTPCACHE)

//
// WebSocket's socket reserver
//
bool
ResourceHandleManager::reserveWebSocketConnection(SocketStreamClient* client)
{
    // If the WebSocket connection was full, try to close the connection waiting to Close message.
    if (m_socketStreamClientList.size() >= m_maxWebSocketConnections) {
        Vector<SocketStreamClient*> underClosing;
        for (int num = 0; num < m_socketStreamClientList.size(); num++) {
            if (m_socketStreamClientList[num] && m_socketStreamClientList[num]->isClosingSocketStreamChannel())
                underClosing.append(m_socketStreamClientList[num]);
        }
        for (int num = 0; num < underClosing.size(); num++) {
            if (underClosing[num])
                underClosing[num]->disconnect();
            // ResourceHandleManager::releaseWebSocketConnection() is called when a socket is actually closed by SocketStreamClient::platformClose(). 
            // Therefore, there is no need to call releaseWebSocketConnection() here.
        }
    }

    if (m_socketStreamClientList.size() >= m_maxWebSocketConnections)
        return false;

    m_socketStreamClientList.append(client);
    return true;
}

void
ResourceHandleManager::releaseWebSocketConnection(SocketStreamClient* client)
{
    for (int num = 0; num < m_socketStreamClientList.size(); num++) {
        if (m_socketStreamClientList[num] == client) {
            m_socketStreamClientList.remove(num);
            break;
        }
    }
}

bool
ResourceHandleManager::canStartWebSocketConnecting(SocketStreamClient* client)
{
    if (!client)
        return false;
    if (!m_socketStreamClientList.contains(client))
        return false;

    const String& target_host = client->url().host().toString();
    std::optional<uint16_t> target_port = client->url().port().value_or(80);

    for (size_t num = 0; num < m_socketStreamClientList.size(); num++) {
        SocketStreamClient* item = m_socketStreamClientList[num];
        if (item == client)
            continue;
        const URL& url = item->url();
        if (target_host!=url.host() || target_port!=url.port())
            continue;
        switch (item->socketState()) {
        case SocketStreamClient::Initialized:
            continue;
        case SocketStreamClient::CanReceive:
            continue;
        default:
            return false;
        }
    }
    return true;
}

int
ResourceHandleManager::getCurrentWebSocketConnectionsNum()
{
    return m_socketStreamClientList.size();
}

void
ResourceHandleManager::threadTerminate(void)
{
    if (m_threadRunning) {
        m_threadRunning = false;

        // join thread
        if (m_threadID) {
            wkcThreadJoinPeer(m_threadID, NULL);
            m_threadID = 0;
        }
    }
}

void
ResourceHandleManager::waitNetworkThreadSuspended(void)
{
    while (true){
        if (m_threadSuspended || !m_threadRunning) {
            break;
        }
        wkc_usleep(10);
    }
}

void
ResourceHandleManager::suspendNetworkThread(void)
{
    if (m_threadRunning) {
        m_threadWillSuspended = true;
        waitNetworkThreadSuspended();
    }
}

void
ResourceHandleManager::resumeNetworkThread(void)
{
    if (m_threadRunning) {
        m_threadSuspended = false;
    }
}

bool
ResourceHandleManager::isNetworkThread(void)
{
    return m_threadID == wkcThreadCurrentThreadPeer();
}

void
ResourceHandleManager::notifyRequestRestartInNetworkThread()
{
    m_threadRunning = false;
}

// Thread entry point for ResourceHandleManager class
void ResourceHandleManager::networkThread()
{
    const int SleepMicroSeconds = 10000;
    struct timeval timeout;
    bool downloadTimerRequested = true;
    timeout.tv_sec = 0;
    timeout.tv_usec = SleepMicroSeconds;

    m_threadRunning = true;
    while (m_threadRunning) {
        if (m_threadWillSuspended) {
            m_threadSuspended = true;
            m_threadWillSuspended = false;
        }
        if (m_threadSuspended) {
            wkc_usleep(SleepMicroSeconds);
            continue;
        }

        executeTasks();

        if (0 == m_curlHandlesInMulti) {
            wkc_usleep(SleepMicroSeconds);
            continue;
        }

        fd_set fdread;
        fd_set fdwrite;
        fd_set fdexcep;
        int maxfd = 0;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);
        curl_multi_fdset(m_curlMultiHandle, &fdread, &fdwrite, &fdexcep, &maxfd);
        // When the 3 file descriptors are empty, winsock will return -1
        // and bail out, stopping the file download. So make sure we
        // have valid file descriptors before calling select.

        if (maxfd >= 0) {
            int rc = wkcNetSelectPeer(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
            LOG_FUNCTION(ResourceHandleManager, "select rc: %d", rc);
            if (rc == -1 && errno != EINTR) {
                LOG_FUNCTION(ResourceHandleManager, "An error(%d) occurred in wkcNetSelectPeer.", errno);
                wkc_usleep(SleepMicroSeconds); // Avoid busy loop.
                continue;
            }
        } else {
            LOG_FUNCTION(ResourceHandleManager, "cURL Handle Thread sleep: %d ms", SleepMicroSeconds);
            wkc_usleep(SleepMicroSeconds);
        }

        int runningHandles = 0;
        while (curl_multi_perform(m_curlMultiHandle, &runningHandles) == CURLM_CALL_MULTI_PERFORM) { }
        LOG_FUNCTION(ResourceHandleManager, "runningHandles: %d", runningHandles);

        // check the curl messages indicating completed transfers
        // and free their resources
        while (true) {
            int messagesInQueue;
            CURLMsg* msg = curl_multi_info_read(m_curlMultiHandle, &messagesInQueue);
            if (!msg) {
                break;
            }

            // find the node which has same d->m_handle as completed transfer
            CURL* handle = msg->easy_handle;
            ASSERT(handle);
            ResourceHandle* job = NULL;
            CURLcode err = curl_easy_getinfo(handle, CURLINFO_PRIVATE, &job);
            ASSERT(CURLE_OK == err);
            if (CURLE_OK != err) {
                continue;
            }
            if (CURLMSG_DONE != msg->msg) {
                continue;
            }
            ResourceHandleInternal* d = job->getInternal();
            if (!d->m_serverpush)
                ASSERT(d->m_handle == handle);

            d->m_msgDataResult = (int)msg->data.result;
            curl_multi_remove_handle(m_curlMultiHandle, d->m_handle);
            m_curlHandlesInMulti--;
            d->m_handleIsInMulti = false;
            d->m_isRunning = false;
        }

        if (downloadTimerRequested) {
            downloadTimerRequested = false;
            callOnMainThread([this, &downloadTimerRequested] {
                if (!m_downloadTimer.isActive()) {
                    m_downloadTimer.startOneShot(Seconds(pollTimeSeconds));
                }
                downloadTimerRequested = true;
            });
        }
    }
}

void ResourceHandleManager::callOnNetworkThread(WTF::Function<void()>&& task)
{
    LockHolder lock(m_threadMutex);
    m_taskQueue.append(WTFMove(task));
}

void ResourceHandleManager::executeTasks()
{
    ASSERT(!isMainThread());

    Vector<WTF::Function<void()>> taskQueue;

    {
        LockHolder lock(m_threadMutex);
        taskQueue = WTFMove(m_taskQueue);
    }

    for (auto& task : taskQueue)
        task();
}

} // namespace WebCore

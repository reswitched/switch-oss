/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
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

#ifndef ResourceHandleManagerWKC_h
#define ResourceHandleManagerWKC_h

#include "Credential.h"
#include "CString.h"
#include "Frame.h"
#include "ProtectionSpaceBase.h"
#include "ResourceHandleClient.h"
#include "StoredCredentialsPolicy.h"
#include "Timer.h"
#include "WTFString.h"

#include "HTTPCacheWKC.h"
#include "WKCWebView.h"

#if PLATFORM(WIN)
#include <winsock2.h>
#include <windows.h>
#endif

#include <curl/curl.h>
#include <wtf/Vector.h>

namespace WebCore {

class ResourceHandleInternal;
class ResourceHandleManagerSSL;
class Document;
class ResourceBuffer;
class SocketStreamClient;
class NetworkStorageSession;
struct Cookie;

class ResourceHandleManager {
WTF_MAKE_FAST_ALLOCATED;
public:
    enum ProxyType {
        HTTP    = CURLPROXY_HTTP,
        HTTP10  = CURLPROXY_HTTP_1_0,
        Socks4  = CURLPROXY_SOCKS4,   // not support
        Socks4A = CURLPROXY_SOCKS4A, // not support
        Socks5  = CURLPROXY_SOCKS5,   // not support
        Socks5Hostname = CURLPROXY_SOCKS5_HOSTNAME   // not support
    };

    // For Singleton
    static bool createSharedInstance();
    static ResourceHandleManager* sharedInstance();
    static void deleteSharedInstance();
    static bool isExistSharedInstance();
    static void forceTerminateInstance();

    void add(ResourceHandle*);
    void cancel(ResourceHandle*);
    void setCookieJarFileName(const char* cookieJarFileName);

    // settings
    void setMaxHTTPConnections(long number);
    void setMaxHTTPPipelineRequests(long number);
    void setMaxWebSocketConnections(long number);
    void setMaxCookieEntries(long number);
    void setHarmfulSiteFilter(bool enable);
    void setRedirectInWKC(bool enable);
    void setDNSCacheTimeout(int sec);
    void setServerResponseTimeout(int sec);
    void setConnectTimeout(int sec);
    void setAcceptEncoding(const char* encodings);
    void setUseHTTP2(bool enable);

    // Authentication Challenge
    void setHttpUserPassword(CURL* handle, const Credential& credential, ProtectionSpaceAuthenticationScheme authScheme);
    void setProxyUserPassword(CURL* handle, const Credential& credential, ProtectionSpaceAuthenticationScheme authScheme);
    void didReceiveAuthenticationChallenge(ResourceHandle*);
    void clearAuthCredential(ResourceHandle*);

    void dispatchSynchronousJob(ResourceHandle*);

    void setProxyInfo(const String& host = "", unsigned long port = 0, ProxyType type = HTTP, const String& username = "", const String& password = "", const String& filters = "");
    static bool hasProxyUserPass();

    // Cookie
    void setWillAcceptCookieCallback(WKC::WillAcceptCookieProc proc);
    int cookieCallback(const NetworkStorageSession& session, bool income, const char* url, const String& firstparty_host, const String& cookie_domain);
    int cookieCallbackInternal(bool income, const char* url, const String& firstparty_host, const String& cookie_domain);
    void clearCookies();
    void setCookie(const String &domain, const String &path, const String &cookie);
    String getCookie(const String &domain, const String &path, bool secure, bool includehttponly=false);
    int CookieSerialize(char* buff, int bufflen);
    void CookieDeserialize(const char* buff, bool restart);
    bool getRawCookies(const String &domain, const String &path, bool secure, Vector<Cookie>& rawCookies);
    void deleteCookie(const String &domain, const String &path, bool secure, const String &name);

    // reset
    static void resetVariables();

    void cancelByFrame(Frame* frame);

    CURLSH* sharehandle() {return m_curlShareHandle;}
    CURLM*  multihandle() {return m_curlMultiHandle;}

    String proxyHost() {return m_proxyHost;}
    int proxyPort() {return m_proxyPort;}
    bool isProxy() {return (7 < m_proxyHost.length());}
    void setProxyTypeHTTP10() { m_proxyType = HTTP10; };
    void setProxyCredential(const Credential& credential) { m_proxyCredential = credential; }
    Credential& proxyCredential() { return m_proxyCredential; }
    void setProxyAuthScheme(ProtectionSpaceAuthenticationScheme authScheme) { m_proxyAuthScheme = authScheme; }
    ProtectionSpaceAuthenticationScheme proxyAuthScheme() { return m_proxyAuthScheme; }

    // Permit to send this url
    void permitRequest(void* handle, bool permit);

    // redirect
    bool didReceiveRedirect(ResourceHandle* job, long httpCode, const String&& location);

    // MaxHTTPConnection per Window
    unsigned maximumHTTPConnectionCountPerHostWindow();

    // ResourceHandleManagerSSL Class
    ResourceHandleManagerSSL* rhmssl(void){ return m_rhmssl; }

    // set Do Not Track
    void setDoNotTrack(bool enable) { m_DoNotTrack = enable; }

    // Content Composition
    int contentComposition(ResourceHandle* job);

    // OneShot m_downloadTimer
    bool oneShotDownloadTimer(void);

    // SSL rapper
    void SSLEnableProtocols(unsigned int versions);
    void SSLEnableOnlineCertChecks(bool enableOCSP, bool enableCRLDP);
    void* SSLRegisterRootCA(const char* cert, int cert_len);
    void* SSLRegisterRootCAByDER(const char* cert, int cert_len);
    int   SSLUnregisterRootCA(void* certid);
    void  SSLRootCADeleteAll(void);
    void* SSLRegisterCRL(const char* crl, int crl_len);
    int   SSLUnregisterCRL(void* crlid);
    void  SSLCRLDeleteAll(void);
    void* SSLRegisterClientCert(const unsigned char* pkcs12, int pkcs12_len, const unsigned char* pass, int pass_len);
    void* SSLRegisterClientCertByDER(const unsigned char* cert, int cert_len, const unsigned char* key, int key_len);
    int   SSLUnregisterClientCert(void* certid);
    void  SSLClientCertDeleteAll(void);
    bool SSLRegisterBlackCert(const char* in_issuerCommonName, const char* serialNumber);
    bool SSLRegisterBlackCertByDER(const char* cert, int cert_len);
    void SSLBlackCertDeleteAll(void);
    bool isCertificateBlack(void* data);
    bool SSLRegisterUntrustedCertByDER(const char* cert, int cert_len);
    bool SSLRegisterEVSSLOID(const char *issuerCommonName, const char *OID, const char *sha1FingerPrint, const char *SerialNumber);
    void SSLEVSSLOIDDeleteAll(void);
    const char** getServerCertChain(const char* url, int& outCertNum);
    void freeServerCertChain(const char** chain, int num);
    void setAllowServerHost(const char *host_w_port);

#if ENABLE(WKC_HTTPCACHE)
    // HTTPCache
    HTTPCachedResource* updateCacheResource(URL &url, SharedBuffer* resourceData, ResourceRequest& request, ResourceResponse &response, bool noStore, Vector<std::pair<String, String>> varyingRequestHeaders, bool serverpush);
    bool addHTTPCache(ResourceHandle *handle, URL &url, SharedBuffer* resourceData, ResourceResponse &resopnse);
    HTTPCachedResource* getHTTPCache(URL &url);
    bool canReadHTTPCache(ResourceHandle* job, HTTPCachedResource* existingResource);
    void makeConditionalRequest(ResourceHandle* job, HTTPCachedResource* resource);
    void scheduleLoadResourceFromHTTPCache(ResourceHandle *job);
    void readCacheTimerCallback();
    void writeCacheTimerCallback();
    HTTPCache* httpCache() { return &m_httpCache; }
    bool isSafeMethod(ResourceHandle* job);
    void clearHTTPCache();
    void resetHTTPCache();
    void dumpHTTPCacheResourceList();
    void setCanCacheToDiskCallback(WKC::CanCacheToDiskProc proc);
#endif

    void setConnectionFilteringCallback(WKC::ConnectionFilteringProc proc);
    bool isMatchProxyFilter(const String& host);

    // WebSocket's socket reserver
    bool reserveWebSocketConnection(SocketStreamClient*);
    void releaseWebSocketConnection(SocketStreamClient*);
    bool canStartWebSocketConnecting(SocketStreamClient*);
    int getCurrentWebSocketConnectionsNum();

    // HTTP/2 server push
    int serverPushCallback(CURL *parent, CURL *easy, size_t num_headers, struct curl_pushheaders *headers);

    // filtering
    static bool filter_callback(CURL *easy, const struct sockaddr_in *addr);

    void setCurlDebugCallback(WKC::CurlDebugProc proc);

    void callOnNetworkThread(WTF::Function<void()>&& task);
    void executeTasks();

private:
    ResourceHandleManager();
    ~ResourceHandleManager();
    static ResourceHandleManager* create();
    bool construct();
    void _downloadTimerCallback();
    void downloadTimerCallback();
    bool finishLoadingResourceHandle(ResourceHandle*, ResourceHandleInternal*);
    bool startJob(ResourceHandle*, ResourceHandleInternal*);
    bool startScheduledJobs();
    void cancelScheduledJobs();

    void initializeHandle(ResourceHandle*);

    enum HTTPMethod {
        HTTPMethod_GET,
        HTTPMethod_HEAD,
        HTTPMethod_POST,
        HTTPMethod_PUT,
        HTTPMethod_DELETE,
        HTTPMethod_OPTIONS,
        HTTPMethod_TRACE,
        HTTPMethod_CONNECT,
        HTTPMethod_PATCH,
        HTTPMethod_LINK,
        HTTPMethod_UNLINK,
    };
    bool setupMethod(ResourceHandle*, struct curl_slist**, int);
    void setupPOST(ResourceHandle*, struct curl_slist**);
    void setupPUT(ResourceHandle*, struct curl_slist**);
    void setupOPTIONS(ResourceHandle*, struct curl_slist**);

    Timer m_downloadTimer;
    CURLM* m_curlMultiHandle;
    CURLSH* m_curlShareHandle;
    char* m_cookieJarFileName;
    char m_curlErrorBuffer[CURL_ERROR_SIZE];

    CURLM* m_curlMultiSyncHandle;
    void* m_mainThreadId;

    // delete cookie
    void doClearCookies();

    // from add() to startScheduledJobs() or cancel()
    Vector<ResourceHandle*> m_scheduledJobList;
    void appendScheduledJob(ResourceHandle*);
    bool removeScheduledJob(ResourceHandle*);  // from cancel()
    ResourceHandle* shiftScheduledJob();
    void removeAllScheduledJobs();

    // from curl_multi_add_handle() to curl_multi_remove_handle()/curl_easy_cleanup()
    // max is m_httpConnections
    Vector<ResourceHandle*> m_runningJobList;
    void appendRunningJob(ResourceHandle* job);
    bool removeRunningJob(ResourceHandle* job);
    ResourceHandle* shiftRunningJob();  // for deleteing all jobs
    void removeAllRunningJobs();

    // Ask to application to permit to send this request
    void canPermitRequest(ResourceHandle* job);

private:
    class ProxyFilter {
    WTF_MAKE_FAST_ALLOCATED;
    public:
        ProxyFilter();
        ~ProxyFilter();
        void setProxyFilters(const String& filters);
        bool isMatchProxyFilter(const String& host);

    private:
        enum ProxyFilterType {
            NONE,
            FQDN,
            Domain,
            IPAddress,
            IPMask
        };
        class Filter {
        WTF_MAKE_FAST_ALLOCATED;
        public:
            Filter(ProxyFilterType type, String filter, const unsigned char* mask)
            : m_type(type), m_filter(filter)
            {
                if (mask)
                    memcpy(m_maskd, mask, 16);
                else
                    memset(m_maskd, 0x00, 16);
            }
            ~Filter() {}

            ProxyFilterType m_type;
            String m_filter;
            unsigned char m_maskd[16];
        };

        Vector<Filter*> m_proxyFilters;

    private:
        void allClean();
        bool append(Filter* filter);
        bool setFQDN(String filter);
        bool setDomain(String filter);
        bool setIPAddress(String filter);
        bool setIPNetwork(String filter);
        int  convertIPStringToChar(String host, unsigned char *ip);
        bool hasProhibitCharactor(String filter);
    };

private:
    ResourceHandleManagerSSL* m_rhmssl;

    // proxy
    String m_proxyHost;
    int    m_proxyPort;
    ProxyType m_proxyType;
    ProxyFilter m_proxyFilters;
    Credential m_proxyCredential;
    ProtectionSpaceAuthenticationScheme m_proxyAuthScheme;

    // cookie
    bool m_cookiesDeleting;
    int m_cookieMaxEntries;

    bool m_tryProxySingleConnect;

    // For Singleton
    void forceTerminate();
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY_ZERO(ResourceHandleManager*, m_sharedInstance);

#if ENABLE(WKC_JAVASCRIPT_SEQUENTIAL)
    // JavaScript Sequential
    bool m_jsSequential;
#endif // WKC_JAVASCRIPT_SEQUENTIAL

    // HTTP version
    HashMap<String, int> m_serverHTTPVersion;

    // Settings
    long  m_httpConnections;
    long  m_httpPipelineRequests;
    long  m_maxWebSocketConnections;
    bool m_harmfulSiteFilter;
    bool m_redirectInWKC;
    bool m_useHTTP2;
    int m_DNSCacheTimeout;
    int m_serverResponseTimeout;
    int m_connectTimeout;
    String m_acceptEncoding;

    // Do Not Track
    bool m_DoNotTrack;

#if ENABLE(WKC_HTTPCACHE)
    // HTTPCache
    HTTPCache m_httpCache;
    Timer m_readCacheTimer;
    Vector<ResourceHandle*> m_readCacheJobList;
    Timer m_writeCacheTimer;
    Vector<HTTPCachedResource*> m_writeCacheList;
#endif

public:
    void networkThread();
    void suspendNetworkThread(void);
    void waitNetworkThreadSuspended(void);
    void resumeNetworkThread(void);
    bool isNetworkThread(void);
    void notifyRequestRestartInNetworkThread(void);
private:
    void threadTerminate(void);
    void sendDataFromBufferToWebCore(ResourceHandle* job, ResourceHandleInternal* d);

private:
    size_t downloadTimerWriteCallback(ResourceHandle* job, ResourceHandleInternal* d, const unsigned char* ptrData, size_t totalSize);
    size_t downloadTimerHeaderCallback(ResourceHandle* job, ResourceHandleInternal* d, const unsigned char* p, size_t totalSize);
    bool finalizingResourceHandle(ResourceHandle* job, ResourceHandleInternal* d);

private:
    int m_curlHandlesInMulti;
    bool m_threadRunning;
    bool m_threadSuspended;
    bool m_threadWillSuspended;
    void* m_threadID;
    Lock m_threadMutex;
    Vector<Function<void()>> m_taskQueue;
    // SocketStreamClient
    Vector<SocketStreamClient*> m_socketStreamClientList;
};

}

#endif  // ResourceHandleManagerWKC_h

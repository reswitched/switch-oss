/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (c) 2011-2018 ACCESS CO., LTD. All rights reserved.
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

#ifndef ResourceHandleInternalWKC_h
#define ResourceHandleInternalWKC_h

#include "WKCEnums.h"

#include "SharedBuffer.h"
#include "ResourceHandle.h"
#include "ResourceHandleClient.h"
#include "ResourceRequest.h"
#include "AuthenticationChallenge.h"
#include "Timer.h"
#include "NetworkingContext.h"
#include "Uint8Array.h"
#include "Deque.h"

#include <curl/curl.h>
#include "FormDataStreamWKC.h"

namespace WebCore {
class ResourceHandleClient;
class SharedBuffer;

#define DATA_KEEP_LENGTH 512

class ResourceHandleInternal {
    WTF_MAKE_NONCOPYABLE(ResourceHandleInternal); WTF_MAKE_FAST_ALLOCATED;
public:
    ResourceHandleInternal(ResourceHandle* loader, NetworkingContext* context, const ResourceRequest& request, ResourceHandleClient* c, bool defersLoading, bool shouldContentSniff)
        : m_client(c)
        , m_firstRequest(request)
        , m_lastHTTPMethod(request.httpMethod())
        , m_didAuthChallenge(false)
        , m_status(0)
        , m_defersLoading(defersLoading)
        , m_shouldContentSniff(shouldContentSniff)
        , m_handle(0)
        , m_cancelled(false)
        , m_url(0)
        , m_urlhost(0)
        , m_fileLoading(false)
        , m_dataLoading(false)
        , m_matchProxyFilter(false)
        , m_composition(WKC::EInclusionContentComposition)
        , m_customHeaders(0)
        , m_formDataStream(loader)
        , m_isSSL(false)
        , m_SSLHandshaked(false)
        , m_isEVSSL(false)
        , m_allowSSLHost(false)
        , m_SSLVerifyPeerResult(0)
        , m_SSLVerifyHostResult(0)
        , m_sslCipherVersion(0)
        , m_sslCipherName(0)
        , m_sslCipherBits(0)
        , m_secureState(1)
        , m_secureLevel(2)
        , m_isSynchronous(false)
#if ENABLE(WKC_JAVASCRIPT_SEQUENTIAL)
        , m_isJSRequestatFirst(false)
#endif // WKC_JAVASCRIPT_SEQUENTIAL
        , m_dataSchemeDownloading(false)
        , m_permitSend(SUSPEND)
        , m_doRedirectChallenge(false)
        , m_redirectMaxCount(20)
        , m_redirectCount(0)
        , m_sslReconnect(false)
        , m_enableOCSP(true)
        , m_enableCRLDP(true)
        , m_recvDataLength(0)
#if ENABLE(WKC_HTTPCACHE)
        , m_httpequivFlags(0)
        , m_httpequivMaxAge(0)
        , m_utilizedHTTPCache(false)
#endif
        , m_serverpush(false)
        , m_scheduledFailureType(ResourceHandle::NoFailure)
        , m_failureTimer(*loader, &ResourceHandle::failureTimerFired)
        , m_context(context)
        , m_handleIsInMulti(false)
        , m_msgDataResult(0)
        , m_blankLinesFromProxy(0)
    {
        const URL& url = m_firstRequest.url();
        m_webAuthUser = url.user();
        m_webAuthPass = url.pass();
        m_webURLIncludingUserPass = (m_webAuthUser.length() != 0 || m_webAuthPass.length() != 0);
        m_webAuthScheme = ProtectionSpaceAuthenticationSchemeUnknown;
        m_webAuthRealm = "";
        m_webAuthURL = "";
        m_webAuthWellKnownWhenInitializeHandle = false;
        m_webAuthCallengeTimes = 0;

        m_proxyAuthUser = "";
        m_proxyAuthPass = "";
        m_proxyAuthScheme = ProtectionSpaceAuthenticationSchemeUnknown;
        m_proxyAuthURL = "";
        m_proxyAuthRealm = "";
        m_proxyAuthWellKnownWhenInitializeHandle = false;
        m_proxyAuthCallengeTimes = 0;

        m_firstRequest.removeCredentials();
        m_recvData[0] = 0x0;

        m_receivedData = SharedBuffer::create(0);
        m_curlDataMutex = wkcMutexNewPeer();
    }

    ~ResourceHandleInternal();

    ResourceHandleClient* client() { return m_client; }
    ResourceHandleClient* m_client;

    ResourceRequest m_firstRequest;
    String m_lastHTTPMethod;

    // Suggested credentials for the current redirection step.
    String m_webAuthUser;
    String m_webAuthPass;
    bool   m_webURLIncludingUserPass;
    ProtectionSpaceAuthenticationScheme m_webAuthScheme;
    String m_webAuthRealm;
    String m_webAuthURL;
    bool   m_webAuthWellKnownWhenInitializeHandle;
    int    m_webAuthCallengeTimes;

    String m_proxyAuthUser;
    String m_proxyAuthPass;
    ProtectionSpaceAuthenticationScheme m_proxyAuthScheme;
    String m_proxyAuthURL;
    String m_proxyAuthRealm;
    bool   m_proxyAuthWellKnownWhenInitializeHandle;
    int    m_proxyAuthCallengeTimes;

    bool m_didAuthChallenge;
    AuthenticationChallenge m_currentWebChallenge;

    Credential m_initialCredential;

    int m_status;

    bool m_defersLoading;
    bool m_shouldContentSniff;

    CURL* m_handle;
    bool m_cancelled;
    bool m_datasent;
    unsigned long long m_bytesSent;
    unsigned long long m_totalBytesToBeSent;
    char* m_url;
    char* m_urlhost;
    bool m_fileLoading;
    bool m_dataLoading;
    bool m_matchProxyFilter;
    int m_composition;
    struct curl_slist* m_customHeaders;
    ResourceResponse m_response;

    FormDataStream m_formDataStream;
    Vector<char> m_postBytes;

    bool m_isSSL;
    bool m_SSLHandshaked;
    bool m_isEVSSL;
    bool m_allowSSLHost;

    long  m_SSLVerifyPeerResult;  // OpenSSL verify_result
    long  m_SSLVerifyHostResult;  // cURL verify_result
    char* m_sslCipherVersion;
    char* m_sslCipherName;
    int  m_sslCipherBits;
    int  m_secureState;  // color
    int  m_secureLevel;  // how secure

    bool m_isSynchronous;

#if ENABLE(WKC_JAVASCRIPT_SEQUENTIAL)
    bool m_isJSRequestatFirst;  // original request is JavaScript
#endif // WKC_JAVASCRIPT_SEQUENTIAL

    bool m_dataSchemeDownloading;

    enum PermitType{
        CANCEL  = -1,
        SUSPEND = 0,
        ASKING  = 1,
        PERMIT  = 2
    };
    PermitType m_permitSend;

    // redirect
    bool m_doRedirectChallenge;
    int m_redirectMaxCount;
    int m_redirectCount;

    // SSL permit
    bool m_sslReconnect;

    // SSL Online Check
    bool m_enableOCSP;
    bool m_enableCRLDP;

    unsigned char m_recvData[DATA_KEEP_LENGTH];
    int m_recvDataLength;

#if ENABLE(WKC_HTTPCACHE)
    int m_httpequivFlags;
    int m_httpequivMaxAge;
    bool m_utilizedHTTPCache;
#endif

    bool m_serverpush;

    RefPtr<SharedBuffer> m_receivedData;

    Vector<RefPtr<BlobData>> m_sendingBlobDataList;

    ResourceHandle::FailureType m_scheduledFailureType;
    Timer m_failureTimer;

    RefPtr<NetworkingContext> m_context;

    Deque<RefPtr<Uint8Array>> m_curlData;
    void* m_curlDataMutex;
    bool m_handleIsInMulti;
    int m_msgDataResult;
    int m_blankLinesFromProxy;
};

} // namespace WebCore

#endif // ResourceHandleInternalWKC_h

/*
 * Copyright (C) 2009 Brent Fulgham.  All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 * Copyright (c) 2012-2017 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "URL.h"
#include "Logging.h"
#include "NotImplemented.h"
#include "SocketStreamHandleClient.h"
#include "SocketStreamHandle.h"
#include "SocketStreamError.h"
#include "ResourceHandleManagerWKC.h"
#include "ResourceHandleManagerWKCSSL.h"
#include "AuthenticationJarWKC.h"

#include <errno.h>

#include <curl/curl.h>

#include <wkc/wkcpeer.h>
#include <wkc/wkcsocket.h>
#include <peer_openssl.h>

#undef CURL_DEBUG_CALLBACK  // if define, must define the same one in ResourceHandleManagerWKC.cpp

#define BASE_INTERVAL  0.01  // 10msec

#if 0
#define _LOG(channel, ...)  WTFLog(&JOIN_LOG_CHANNEL_WITH_PREFIX(LOG_CHANNEL_PREFIX, channel), __VA_ARGS__)
#else
#define _LOG(channel, ...)  ((void)0)
#endif

namespace WebCore {

SocketStreamHandle::SocketStreamHandle(const URL& url, SocketStreamHandleClient* client, NetworkingContext& networkingContext)
    : SocketStreamHandleBase(url, client)
    , m_clientCallingFromTimer(false)
    , m_needClosing(false)
    , m_socketState(None)
    , m_socket(-1)
    , m_progressTimer(*this, &SocketStreamHandle::progressTimerFired)
    , m_interval(BASE_INTERVAL)
    , m_lastUpdate(0)
    , m_isProxy(false)
    , m_recvData(0)
    , m_handle(0)
    , m_multiHandle(0)
    , m_networkingContext(networkingContext)
    , m_sendAgainTimeout(20)
{
    LOG(Network, "SocketStreamHandle %p new client %p", this, m_client);
    construct();
}

SocketStreamHandle::~SocketStreamHandle()
{
    LOG(Network, "SocketStreamHandle %p delete", this);

    setClient(0);
    platformClose();
    fastFree(m_recvData);
}

#ifdef CURL_DEBUG_CALLBACK
extern int debugCallback(CURL *handle, curl_infotype type,  char *data, size_t size, void *userptr);
#endif

static String SSLhostAndPort(const URL& kurl)
{
    String url;

    if (kurl.hasPort())
        url = kurl.host().lower() + ":" + String::number(kurl.port());
    else
        url = kurl.host() + ":443";

    return url;
}


// static callback
int SocketStreamHandle::OpenSocketCallback(void* addr)
{
    struct curl_sockaddr *address = (struct curl_sockaddr *)addr;
    m_socket = wkcNetSocketPeer(address->family, address->socktype, address->protocol);
    return m_socket;
}

static curl_socket_t opensocketcallback(void *clientp, curlsocktype purpose, struct curl_sockaddr *address)
{
    (void)purpose;

    if (!clientp)
        return -1;

    SocketStreamHandle* handle = static_cast<SocketStreamHandle*>(clientp);
    return handle->OpenSocketCallback((void*)address);
}

bool SocketStreamHandle::isClosingSocketStreamChannel(void)
{
    if (m_client)
        return m_client->isClosing();
    else
        return false;
}

void SocketStreamHandle::construct(void)
{
    URL httpurl;
    CURLM* multiHandle = 0;
    CURL* handle = 0;
    AuthenticationJar* authJar = 0;

    _LOG(Network, "SocketStreamHandle %p construct", this);

    ResourceHandleManager* rhm = ResourceHandleManager::sharedInstance();
    if (!rhm) {
        _LOG(Network, "SocketStreamHandle %p construct: Could not get RHM", this);
        goto construct_end;
    }

    if (!rhm->reserveWebSocketConnection(this)) {
        _LOG(Network, "SocketStreamHandle %p construct: No more Connection", this);
        goto construct_end;
    }

    authJar = rhm->authJar();
    if (!authJar) {
        _LOG(Network, "SocketStreamHandle %p construct: Could not get authJar", this);
        rhm->releaseWebSocketConnection(this);
        goto construct_end;
    }

    multiHandle = curl_multi_init();
    if (!multiHandle) {
        _LOG(Network, "SocketStreamHandle %p construct: Could not get CURLM", this);
        rhm->releaseWebSocketConnection(this);
        goto construct_end;
    }
    curl_multi_setopt(multiHandle, CURLMOPT_MAXCONNECTS, 1L);

    handle = curl_easy_init();
    if (!handle) {
        _LOG(Network, "SocketStreamHandle %p construct: Could not get CURL", this);
        curl_multi_cleanup(multiHandle);
        rhm->releaseWebSocketConnection(this);
        goto construct_end;
    }

    m_recvData = fastMalloc(m_recvDataLen);
    if (!m_recvData) {
        _LOG(Network, "SocketStreamHandle %p construct: Could not malloc receive buffer", this);
        curl_multi_cleanup(multiHandle);
        curl_easy_cleanup(handle);
        rhm->releaseWebSocketConnection(this);
        goto construct_end;
    }

#ifdef CURL_DEBUG_CALLBACK
    curl_easy_setopt(m_handle, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(m_handle, CURLOPT_DEBUGFUNCTION, debugCallback);
#endif

    curl_easy_setopt(handle, CURLOPT_SHARE, rhm->sharehandle());

    // redirect
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 10);
    // only one connection for each handle
    curl_easy_setopt(handle, CURLOPT_MAXCONNECTS, 1);

    httpurl = m_url;
    if (equalIgnoringCase(m_url.protocol(), "wss")) {
        httpurl.setProtocol("https");

        if (wkcOsslCertfIsRegistPeer())
            curl_easy_setopt(handle, CURLOPT_CAINFO, WKCOSSL_CERT_FILE);
        if (wkcOsslCRLIsRegistPeer())
            curl_easy_setopt(handle, CURLOPT_CRLFILE, WKCOSSL_CRL_FILE);

        curl_easy_setopt(handle, CURLOPT_RANDOM_FILE, WKCOSSL_RANDFILE);

        curl_easy_setopt(handle, CURLOPT_SSLVERSION, CURL_SSLVERSION_DEFAULT);

        // CURLOPT_CERTINFO sets 0 but it can get the pointer of STACK_OF(X509)
        // using CURLINFO_SSL_CERTCHAIN by ACCESS Special customize
        curl_easy_setopt(handle, CURLOPT_CERTINFO, 0);

        if (rhm->rhmssl()->allowsServerHost(SSLhostAndPort(httpurl))) {
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
        }
        else {
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2);
            curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1);
        }
    }
    else {
        httpurl.setProtocol("http");
    }

    // url
    curl_easy_setopt(handle, CURLOPT_URL, httpurl.string().utf8().data());

    // Set proxy options if we have them.
    if (rhm->proxy().length() && !rhm->isMatchProxyFilter(m_url.host())) {
        curl_easy_setopt(handle, CURLOPT_PROXY, rhm->proxy().utf8().data());
        // Proxy Authenticate
        String user;
        String pass;
        ProtectionSpaceServerType           serverType;
        ProtectionSpaceAuthenticationScheme authScheme;
        if (authJar->getProxyUserPassword(rhm->proxy(), rhm->proxyPort(), serverType, authScheme, user, pass)) {
            curl_easy_setopt(handle, CURLOPT_PROXYUSERNAME, user.utf8().data());
            curl_easy_setopt(handle, CURLOPT_PROXYPASSWORD, pass.utf8().data());
            switch (authScheme) {
            case ProtectionSpaceAuthenticationSchemeHTTPBasic:
                curl_easy_setopt(handle, CURLOPT_PROXYAUTH, CURLAUTH_BASIC);
                break;
            case ProtectionSpaceAuthenticationSchemeHTTPDigest:
                curl_easy_setopt(handle, CURLOPT_PROXYAUTH, CURLAUTH_DIGEST);
                curl_multi_setopt(multiHandle, CURLMOPT_MAXCONNECTS, 2L);
                break;
            case ProtectionSpaceAuthenticationSchemeNTLM:
                curl_easy_setopt(handle, CURLOPT_PROXYAUTH, CURLAUTH_NTLM);
                break;
            default:
                break;
            }
        }
        else {
            curl_easy_setopt(handle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
        }
        curl_easy_setopt(handle, CURLOPT_HTTPPROXYTUNNEL, 1L);
        m_isProxy = true;
    }

    // connect only
    curl_easy_setopt(handle, CURLOPT_CONNECT_ONLY, 1L);

    // functions
    curl_easy_setopt(handle, CURLOPT_OPENSOCKETFUNCTION, opensocketcallback);
    curl_easy_setopt(handle, CURLOPT_OPENSOCKETDATA , (void*)this);

    // add handle
    curl_multi_add_handle(multiHandle, handle);

    m_multiHandle = (void*)multiHandle;
    m_handle      = (void*)handle;

    m_socketState = Initialized;

construct_end:
    if (m_socketState==Initialized) {
        if (m_client)
            m_client->willOpenSocketStream(this);
    }

    m_progressTimer.startOneShot(m_interval);
    return;
}

void SocketStreamHandle::nextProgress(bool refresh)
{
    _LOG(Network, "SocketStreamHandle %p nextProgress(%s)", this, (refresh)?"true":"false");

    if (!m_handle || !m_multiHandle) {
        return;
    }

    if (refresh) {
        m_lastUpdate = wkcGetTickCountPeer();
        m_interval = BASE_INTERVAL;
        if (m_progressTimer.isActive())
             m_progressTimer.stop();
    }
    else {
        int n = (wkcGetTickCountPeer() - m_lastUpdate)/1024 + 1;
        if (10 < n) n = 10;
        m_interval = BASE_INTERVAL * ((float)n);
    }

    if (!m_progressTimer.isActive()) {
        _LOG(Network, "SocketStreamHandle m_progressTimer.startOneShot(%f)", m_interval);
        m_progressTimer.startOneShot(m_interval);
    }
}

void
SocketStreamHandle::progressTimerFired()
{
    size_t inoutLen;

    _LOG(Network, "SocketStreamHandle %p progressTimerFired", this);

    if (m_socketState==None) {
        if (m_client) {
            m_client->didFailSocketStream(this, SocketStreamError(-1));
            m_client->didCloseSocketStream(this);
        }
        m_state = Closed;
        return;
    }

    if (m_needClosing) {
        RefPtr<SocketStreamHandle> protect(static_cast<SocketStreamHandle*>(this)); // platformClose calls the client, which may make the handle get deallocated immediately.
        platformClose();
        return;
    }

    switch (m_state) {
    case Connecting:
    {
        if (!ResourceHandleManager::sharedInstance()->canStartWebSocketConnecting(this)) {
            nextProgress(true);
            break;
        }
        if (!m_handle || !m_multiHandle) {
            m_state = Open;
            if (m_client) {
                m_clientCallingFromTimer = true;
                m_client->didOpenSocketStream(this);
                m_clientCallingFromTimer = false;
            }
            return;
        }

        _LOG(Network, "SocketStreamHandle::progressTimerFired(): m_state:Connecting");

        int runningHandles = 0;
        int messagesInQueue;
        while (curl_multi_perform((CURLM*)m_multiHandle, &runningHandles) == CURLM_CALL_MULTI_PERFORM) { }
        CURLMsg* msg = curl_multi_info_read((CURLM*)m_multiHandle, &messagesInQueue);
        if (!msg) {
            nextProgress(true);
            break;
        }
        if (CURLMSG_DONE != msg->msg || m_socket < 0) {
            m_state = Closed;
            if (m_client)
                m_client->didCloseSocketStream(this);
            return;
        }
        // socket opened
        RefPtr<SocketStreamHandle> protect(static_cast<SocketStreamHandle*>(this)); // platformClose calls the client, which may make the handle get deallocated immediately.
        m_state = Open;
        if (m_client) {
            m_clientCallingFromTimer = true;
            m_client->didOpenSocketStream(this);
            m_clientCallingFromTimer = false;
        }
        if (!m_needClosing)
            m_socketState = Connected;
        nextProgress(true);
        break;
    }

    case Open:
        while (m_handle && m_socket != -1) {
            fd_set rd;
            FD_ZERO(&rd);
            FD_SET(m_socket, &rd);
            struct timeval tv = {0};

            _LOG(Network, "SocketStreamHandle::progressTimerFired(): m_state:Open wkcNetSelectPeer()");
            int ret = wkcNetSelectPeer(m_socket+1, &rd, NULL, NULL, &tv);
            if (ret == 0) {
                nextProgress(false);
                break;
            }
            else if (ret == -1) {
                nextProgress(true);
                break;
            }

            m_socketState = CanReceive;

            _LOG(Network, "SocketStreamHandle::progressTimerFired(): m_state:Open curl_easy_recv()");
            inoutLen = 0;
            ret = curl_easy_recv((CURL*)m_handle, m_recvData, m_recvDataLen, &inoutLen);
            RefPtr<SocketStreamHandle> protect(static_cast<SocketStreamHandle*>(this)); // platformClose calls the client, which may make the handle get deallocated immediately.
            if (CURLE_OK == ret) {
                if (inoutLen == 0) {
                    // FIN recv
                    _LOG(Network, "SocketStreamHandle::progressTimerFired() Recv FIN");
                    m_state = Closed;
                    if (m_client)
                        m_client->didCloseSocketStream(this);
                    break;
                }
                _LOG(Network, "SocketStreamHandle::progressTimerFired() Recved(%d)", inoutLen);
                if (0 < inoutLen && m_client) {
                    m_clientCallingFromTimer = true;
                    m_client->didReceiveSocketStreamData(this, (const char*)m_recvData, inoutLen);
                    m_clientCallingFromTimer = false;
                }
                nextProgress(true);
            }
            else if (CURLE_AGAIN == ret) {
                _LOG(Network, "SocketStreamHandle::progressTimerFired() Recv AGAIN");
                nextProgress(true);
                break;
            }
            else if (CURLE_UNSUPPORTED_PROTOCOL == ret) {
                // FIN recv
                _LOG(Network, "SocketStreamHandle::progressTimerFired() Recv FIN");
                if (m_client)
                    m_client->didFailSocketStream(this, SocketStreamError(-1));
                break;
            }
            else {
                _LOG(Network, "SocketStreamHandle::progressTimerFired() Recv ERROR(%d)", ret);
                if (m_client)
                    m_client->didFailSocketStream(this, SocketStreamError(-1));
                break;
            }
        }
        break;

    case Closing:
        _LOG(Network, "SocketStreamHandle::progressTimerFired() m_state:Closing");
        break;

    case Closed:
        _LOG(Network, "SocketStreamHandle::progressTimerFired() m_state:Closed");
        break;

    default:
        _LOG(Network, "SocketStreamHandle::progressTimerFired() m_state:Unknown");
        break;
    }
}

int SocketStreamHandle::platformSend(const char* data, int len)
{
    _LOG(Network, "SocketStreamHandle %p platformSend(%p, %d), state=%d", this, data, len, m_state);

    if (Open != m_state || !m_handle || !m_multiHandle || m_socket < 0) {
        if (m_client)
            m_client->didFailSocketStream(this, SocketStreamError(-1));
        return -1;
    }

    // Here m_state must be Open

    CURLcode ret;
    int outLen = 0;
    do {
        size_t curl_out_len = 0;
        ret = curl_easy_send((CURL*)m_handle, data + outLen, len - outLen, &curl_out_len);
        outLen += (int)curl_out_len;
        if (CURLE_AGAIN == ret) {
            fd_set wd;
            FD_ZERO(&wd);
            FD_SET(m_socket, &wd);
            struct timeval tv;
            tv.tv_sec = m_sendAgainTimeout;
            tv.tv_usec = 0;

            int ret = wkcNetSelectPeer(m_socket + 1, NULL, &wd, NULL, &tv);
            if (ret == 0) {
                return -1;
            }
        }
    } while (CURLE_AGAIN == ret);

    if (CURLE_UNSUPPORTED_PROTOCOL == ret) {
        int recverror;
        do {
            // for check receiveing FIN
            // Expect m_socket is non-blocking socket.
            outLen = wkcNetRecvPeer(m_socket, (void*)m_recvData, 2048, 0);
            recverror = wkcNetGetLastErrorPeer();
            if (0 < outLen) {
                if (m_client) {
                    m_client->didReceiveSocketStreamData(this, (const char*)m_recvData, outLen);
                }
            }
            else if (0 == outLen) {
                // recv FIN, suppose to send all. Return len.
                return len;
            }
            else {
                if (EAGAIN == recverror) {
                    outLen = 1;
                }
                else {
                    return -1;
                }
            }
        } while (0 < outLen);
    }
    else if (CURLE_OK != ret) {
        return -1;
    }

    nextProgress(true);
    return outLen;
}

void SocketStreamHandle::platformClose()
{
    _LOG(Network, "SocketStreamHandle %p platformClose()", this);

    if (!m_handle || !m_multiHandle) {
        return;
    }

    if (m_clientCallingFromTimer) {
        m_needClosing = true;
        return;
    }

    if (m_needClosing)
        m_needClosing = false;

    CURL*  handle = (CURL*)m_handle;
    CURLM* multiHandle = (CURLM*)m_multiHandle;

    if (m_progressTimer.isActive())
        m_progressTimer.stop();

    _LOG(Network, "SocketStreamHandle::platformClose() cleanup cURL");
    curl_multi_remove_handle(multiHandle, handle);
    curl_easy_cleanup(handle);
    curl_multi_cleanup(multiHandle);
    if (0 < m_socket)
        wkcNetClosePeer(m_socket);

    m_handle = 0;
    m_multiHandle = 0;
    m_socket = -1;

    if (m_client)
        m_client->didCloseSocketStream(this);

    ResourceHandleManager* rhm = ResourceHandleManager::sharedInstance();
    if (rhm)
        rhm->releaseWebSocketConnection(this);

    _LOG(Network, "SocketStreamHandle::platformClose() done");
}

void SocketStreamHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    notImplemented();
}

void SocketStreamHandle::receivedCredential(const AuthenticationChallenge&, const Credential&)
{
    notImplemented();
}

void SocketStreamHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&)
{
    notImplemented();
}

void SocketStreamHandle::receivedCancellation(const AuthenticationChallenge&)
{
    notImplemented();
}

}  // namespace WebCore

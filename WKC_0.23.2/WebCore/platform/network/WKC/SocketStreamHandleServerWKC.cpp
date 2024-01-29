/*
 * Copyright (C) 2009 Brent Fulgham.  All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 * Copyright (c) 2012-2019 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(REMOTE_INSPECTOR)

#include "SocketStreamHandleBase.h"
#include "SocketStreamHandleServerWKC.h"

#include "CString.h"
#include "URL.h"
#include "Logging.h"
#include "ResourceHandleManagerWKC.h"
#include "SocketStreamHandleClient.h"
#include "SocketStreamError.h"

#include "NotImplemented.h"

#if COMPILER(MSVC)
//# include <winsock2.h>
//# include <ws2tcpip.h>
# ifndef EWOULDBLOCK
#  define EWOULDBLOCK WSAEWOULDBLOCK
# endif
#else
# include <sys/types.h>
# ifndef NO_USE_SYS_SOCKET_H
#  include <sys/socket.h>
# endif
# include <sys/ioctl.h>
# include <errno.h>
#endif
#include <wkc/wkcsocket.h>

namespace WebCore {

SocketStreamHandleServer::SocketStreamHandleServer(int socket, SocketStreamHandleClient* client)
    : SocketStreamHandleBase(URL(), client)
    , m_socket(socket)
    , m_progressTimer(*this, &SocketStreamHandleServer::progressTimerFired)
    , m_progressState(EConnected)
    , m_useproxy(false)
    , m_firstSend(true)
{
//    LOG(Network, "SocketStreamHandleServer %p new client %p", this, m_client);

    m_progressState = EConnected;
    m_progressTimer.startOneShot(0);
}

SocketStreamHandleServer::~SocketStreamHandleServer()
{
//    LOG(Network, "SocketStreamHandleServer %p delete", this);
    setClient(0);
    finalizeSocket();
}
#if 0
void
SocketStreamHandleServer::initializeSocket()
{
    m_progressState = EError;

    m_socket = wkcNetSocketPeer(PF_INET, SOCK_STREAM, 0);
    if (!m_socket) {
        LOG(Network, "failed to create socket %p", this);
        m_progressTimer.startOneShot(0);
        return;
    }

    char host[128] = {0};
    int port = 80;
    ::strncpy(host, m_url.host().utf8().data(), sizeof(host)-1);
    if (m_url.hasPort())
        port = m_url.port();
    else if (m_url.protocol()=="wss")
        port = 443;
    else
        port = 80;
    m_useproxy = false;

    ResourceHandleManager* mgr = ResourceHandleManager::sharedInstance();
    if (mgr) {
        String proxy = mgr->proxy();
        if (proxy.length()) {
            URL kp(ParsedURLString, proxy);
            ::strncpy(host, kp.host().utf8().data(), sizeof(host)-1);
            port = kp.port();
            m_useproxy = true;
        }
    }

    struct hostent* ent = 0;
    ent = wkcNetGetHostByNamePeer(host);
    if (!ent) {
        wkcNetClosePeer(m_socket);
        m_socket=0;
        LOG(Network, "failed to resolve name %p : %s", this, m_url.host().utf8().data());
        m_progressTimer.startOneShot(0);
        return;
    }

    struct sockaddr_in dest = {0};
    ::memcpy(&dest.sin_addr, ent->h_addr_list[0], ent->h_length);
    dest.sin_family = AF_INET;
    dest.sin_port = wkc_htons(port);
    int ret = wkcNetConnectPeer(m_socket, (struct sockaddr *)&dest, sizeof(dest));
    if (ret) {
        int lasterr = wkcNetGetLastErrorPeer();
        if (lasterr!=EWOULDBLOCK) {
            wkcNetClosePeer(m_socket);
            m_socket=0;
            LOG(Network, "failed to connect %p : %s", this, m_url.host().utf8().data());
            m_progressTimer.startOneShot(0);
            return;
        }
    }

    long v = 1;
    wkcNetIoctlSocketPeer(m_socket, FIONBIO, &v);

    m_firstSend = true;

    if (!m_useproxy) {
        m_progressState = EConnected;
        m_progressTimer.startOneShot(0);
        return;
    }

    // connect to proxy
    char buf[1024] = {0};
    port = 80;
    if (m_url.hasPort())
        port = m_url.port();
    // Ugh!: support proxy auth!
    // 120912 ACCESS Co.,Ltd.
    ::snprintf(buf, sizeof(buf), "CONNECT %s:%d HTTP/1.1\r\nHost: %s\r\n\r\n", m_url.host().utf8().data(), port, m_url.host().utf8().data());
    wkcNetSendPeer(m_socket, buf, ::strlen(buf), 0);

    m_progressState = EConnectingProxy;
    m_progressTimer.startOneShot(0.1);
}
#endif

void
SocketStreamHandleServer::finalizeSocket()
{
    if (!m_socket)
        return;
    wkcNetClosePeer(m_socket);
}

void
SocketStreamHandleServer::progressTimerFired()
{
    switch (m_progressState) {
    case EError:
        if (m_client)
            m_client->didFailSocketStream(reinterpret_cast<SocketStreamHandle*>(this), SocketStreamError(-1));
        break;

    case EConnectingDest:
        break;
#if 0
    case EConnectingProxy:
    {
        char buf[1024] = {0};
        fd_set rd;
        FD_ZERO(&rd);
        FD_SET(m_socket, &rd);
        struct timeval tv = {0};
        if (wkcNetSelectPeer(m_socket+1, &rd, NULL, NULL, &tv)==0) {
            m_progressTimer.startOneShot(0.1);
            break;
        }
        int len = wkcNetRecvPeer(m_socket, buf, sizeof(buf), 0);
        if (len<=0) {
            m_progressTimer.startOneShot(0.1);
            break;
        }
        String result(buf);
        if (result.find("Connection Established", 0, false)) {
            m_progressState = EConnected;
            m_progressTimer.startOneShot(0.1);
        } else {
            m_progressState = EError;
            m_progressTimer.startOneShot(0.1);
        }
        break;
    }
#endif
    case EConnected:
        m_state = Open;
        m_progressState = EReady;
        if (m_client)
            m_client->didOpenSocketStream(reinterpret_cast<SocketStreamHandle*>(this));
        m_progressTimer.startOneShot(0.1);
        break;

    case EReady:
    {
        char buf[1024] = {0};
        do {
            fd_set rd, wd;
            FD_ZERO(&rd);
            FD_ZERO(&wd);
            FD_SET(m_socket, &rd);
            FD_SET(m_socket, &wd);
            struct timeval tv = {0};
            int ret = wkcNetSelectPeer(m_socket+1, &rd, &wd, NULL, &tv);
            if (ret == 0 || (!FD_ISSET(m_socket, &rd) && !bufferedAmount()))
                break;
            if (FD_ISSET(m_socket, &rd)) {
                int len = wkcNetRecvPeer(m_socket, buf, sizeof(buf), 0);
                if (len<=0)
                    break;
                if (m_client)
                    m_client->didReceiveSocketStreamData(reinterpret_cast<SocketStreamHandle*>(this), buf, len);
            }
            if (bufferedAmount()) {
                if (FD_ISSET(m_socket, &wd)) {
                    if (!sendPendingData()) {
                        m_progressState = EError;
                        break;
                    }
                }
            }
        } while (1);
        m_progressTimer.startOneShot(0.1);
        break;
    }

    case ERequestClose:
        m_state = Closed;
        if (m_client)
            m_client->didCloseSocketStream(reinterpret_cast<SocketStreamHandle*>(this));
        break;

    default:
        break;
    }
}

int SocketStreamHandleServer::platformSend(const char* data, int len)
{
//    LOG(Network, "SocketStreamHandleServer %p platformSend", this);
    if (!data || !len)
        return 0;
    if (!m_socket)
        return 0;
    if (m_progressState!=EReady)
        return 0;

    len = wkcNetSendPeer(m_socket, data, len, 0);

    return len;
}

void SocketStreamHandleServer::platformClose()
{
//    LOG(Network, "SocketStreamHandleServer %p platformClose", this);

    if (m_progressTimer.isActive())
        m_progressTimer.stop();

    m_progressState = ERequestClose;
    m_progressTimer.startOneShot(0);
}

}  // namespace WebCore

#endif // ENABLE(REMOTE_INSPECTOR)

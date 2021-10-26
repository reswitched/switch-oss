/*
 * Copyright (C) 2009 Brent Fulgham.  All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 * Copyright (c) 2012-2020 ACCESS CO., LTD. All rights reserved.
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

#include "SocketStreamServer.h"

#include "CString.h"
#include "URL.h"
#include "Logging.h"
#include "ResourceHandleManagerWKC.h"
#include "SocketStreamHandleClient.h"
#include "SocketStreamError.h"

#include "NotImplemented.h"

#include <wkc/wkcsocket.h>

static const Seconds baseProgressInterval { 10_ms };

namespace WebCore {

SocketStreamServer::SocketStreamServer(int socket, SocketStreamHandleClient& client)
    : SocketStreamHandleImpl(URL(), client)
    , m_socket(socket)
    , m_progressTimer(*this, &SocketStreamServer::progressTimerFired)
    , m_progressState(EConnected)
    , m_useproxy(false)
    , m_firstSend(true)
{
    LOG(Network, "SocketStreamServer %p new client %p", this, &m_client);

    m_progressState = EConnected;
    m_progressTimer.startOneShot(baseProgressInterval);
}

SocketStreamServer::~SocketStreamServer()
{
    LOG(Network, "SocketStreamServer %p delete", this);
    finalizeSocket();
}

void
SocketStreamServer::finalizeSocket()
{
    if (!m_socket)
        return;
    wkcNetClosePeer(m_socket);
}

void
SocketStreamServer::progressTimerFired()
{
    switch (m_progressState) {
    case EError:
        m_client.didFailSocketStream(*this, SocketStreamError(-1));
        break;

    case EConnectingDest:
        break;
    case EConnected:
        m_state = Open;
        m_progressState = EReady;
        m_client.didOpenSocketStream(*this);
        m_progressTimer.startOneShot(baseProgressInterval);
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
                m_client.didReceiveSocketStreamData(*this, buf, len);
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
        m_progressTimer.startOneShot(baseProgressInterval);
        break;
    }

    case ERequestClose:
        m_state = Closed;
        m_client.didCloseSocketStream(*this);
        break;

    default:
        break;
    }
}

std::optional<size_t> SocketStreamServer::platformSendInternal(const uint8_t* data, size_t len)
{
    LOG(Network, "SocketStreamServer %p platformSend", this);
    if (!data || !len)
        return std::nullopt;
    if (!m_socket)
        return std::nullopt;
    if (m_progressState!=EReady)
        return std::nullopt;

    len = wkcNetSendPeer(m_socket, data, len, 0);

    if (len < 0)
        return std::nullopt;

    return len;
}

void SocketStreamServer::platformClose()
{
    LOG(Network, "SocketStreamServer %p platformClose", this);

    if (m_progressTimer.isActive())
        m_progressTimer.stop();

    m_progressState = ERequestClose;
    m_progressTimer.startOneShot(baseProgressInterval);
}

}  // namespace WebCore

#endif // ENABLE(REMOTE_INSPECTOR)

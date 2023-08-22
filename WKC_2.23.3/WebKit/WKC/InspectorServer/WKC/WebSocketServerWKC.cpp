/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (c) 2012-2019 ACCESS CO., LTD. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#if ENABLE(REMOTE_INSPECTOR)

#include "WebSocketServerWKC.h"

#include "HTTPRequest.h"
#include "WebSocketServer.h"
#include "SocketStreamServer.h"
#include <wtf/text/CString.h>

#ifdef _MSC_VER
# if _MSC_VER < 1700
#  include <WinSock2.h>
# endif
# define SHUT_RDWR  0x02 // from WinSock2.h
#elif defined(__clang__)
#else
# include <sys/socket.h>
#endif

#include <sys/types.h>
//#include <sys/ioctl.h>
#include <errno.h>
#include <wkc/wkcsocket.h>

using namespace WebCore;

namespace WKC {

static const Seconds pollTimeSeconds { 100_ms };  // TODO: adjust

void WebSocketServer::platformInitialize()
{
    m_tcpServerHandler = adoptRef(new WKCTcpServerHandler(this));
}

bool WebSocketServer::platformListen(const WTF::String& bindAddress, unsigned short port)
{
    return m_tcpServerHandler->listen(bindAddress, port);
}

void WebSocketServer::platformClose()
{
    m_tcpServerHandler->close();
}

WKCTcpServerHandler::WKCTcpServerHandler(WebSocketServer* webSocketServer)
    : m_serverSocket(-1)
    , m_webSocketServer(webSocketServer)
    , m_pollTimer(*this, &WKCTcpServerHandler::pollTimerCallback)
{
}

void WKCTcpServerHandler::handleNewConnection(int socket)
{
    m_webSocketServer->didAcceptConnection(socket);
}

bool WKCTcpServerHandler::listen(const WTF::String& bindAddress, unsigned short port)
{
    ASSERT(!bindAddress.isEmpty());
    ASSERT(port);

    m_serverSocket = wkcNetSocketPeer(PF_INET, SOCK_STREAM, 0);
    if (m_serverSocket < 0) {
        LOG_ERROR("wkcNetSocketPeer() failed");
        return false;
    }
    int val = 1;
    wkcNetSetSockOptPeer(m_serverSocket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    struct sockaddr_in addr = { 0 };
    addr.sin_family = PF_INET;
    addr.sin_port = wkc_htons(port);
    if (bindAddress == "localhost" || bindAddress=="127.0.0.1") {
        addr.sin_addr.s_addr = wkc_htonl(INADDR_ANY);
    }
    else {
        addr.sin_addr.s_addr = wkcNetInetAddrPeer(bindAddress.utf8().data());
    }
    if (wkcNetBindPeer(m_serverSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("wkcNetBindPeer() failed");
        goto error;
    }
    if (wkcNetListenPeer(m_serverSocket, SOMAXCONN) < 0) {
        LOG_ERROR("wkcNetListenPeer() failed");
        goto error;
    }

    startPolling();

    return true;

error:
    wkcNetShutdownPeer(m_serverSocket, SHUT_RDWR);
    wkcNetClosePeer(m_serverSocket);
    m_serverSocket = -1;
    return false;
}

void WKCTcpServerHandler::startPolling()
{
    if (!m_pollTimer.isActive()) {
        m_pollTimer.startOneShot(pollTimeSeconds);
    }
}

void WKCTcpServerHandler::pollTimerCallback()
{
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(m_serverSocket, &rfds);
    struct timeval tv = { 0 };

    int ret = wkcNetSelectPeer(m_serverSocket + 1, &rfds, NULL, NULL, &tv);
    if (ret < 0) {
        // TODO: handle error
        LOG_ERROR("wkcNetSelectPeer() failed");
        close();
        return;
    } else if (ret > 0) {
        int socket = wkcNetAcceptPeer(m_serverSocket, NULL, NULL);
        if (socket < 0) {
            if (wkcNetGetLastErrorPeer() == EMFILE) {
                startPolling();
                return;
            }
            // TODO: handle error
            LOG_ERROR("wkcNetAcceptPeer() failed");
            close();
            return;
        }
        handleNewConnection(socket);
    }

    startPolling();
}

void WKCTcpServerHandler::close()
{
    wkcNetClosePeer(m_serverSocket);
    m_serverSocket = -1;
}

}

#endif // ENABLE(REMOTE_INSPECTOR)

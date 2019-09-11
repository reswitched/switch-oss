/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (c) 2012, 2015 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(REMOTE_INSPECTOR)

#include "WebSocketServer.h"

#include "WebSocketServerConnection.h"
#include "SocketStreamHandleServerWKC.h"
#include <wtf/PassRefPtr.h>

#if PLATFORM(WKC)
#include "./WKC/WebSocketServerWKC.h"
#endif

using namespace WebCore;

namespace WKC {

WebSocketServer::WebSocketServer(WebSocketServerClient* client)
    : m_state(Closed)
    , m_client(client)
{
    platformInitialize();
}

WebSocketServer::~WebSocketServer()
{
    close();
}

bool WebSocketServer::listen(const WTF::String& bindAddress, unsigned short port)
{
    ASSERT(port);

    if (m_state == Listening)
        return false;

    bool isNowListening = platformListen(bindAddress, port);
    m_state = isNowListening ? Listening : Closed;
    return isNowListening;
}

void WebSocketServer::close()
{
    if (m_state == Closed)
        return;

    platformClose();
}

void WebSocketServer::didAcceptConnection(PassRefPtr<SocketStreamHandleServer> socketHandle)
{
    m_connections.append(adoptRef(new WebSocketServerConnection(socketHandle, m_client, this)));
}

void WebSocketServer::didCloseWebSocketServerConnection(WebSocketServerConnection* connection)
{
    Deque<RefPtr<WebSocketServerConnection> >::iterator end = m_connections.end();
    for (Deque<RefPtr<WebSocketServerConnection> >::iterator it = m_connections.begin(); it != end; ++it) {
        if (it->get() == connection) {
            m_connections.remove(it);
            return;
        }
    }
    ASSERT_NOT_REACHED();
}

}

#endif // ENABLE(REMOTE_INSPECTOR)

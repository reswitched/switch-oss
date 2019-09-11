/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (c) 2012-2019 ACCESS CO., LTD. All rights reserved.
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

#include "WebSocketServerConnection.h"

#include "WebSocketServer.h"
#include "WebSocketServerClient.h"
#include "HTTPRequest.h"
#include "NotImplemented.h"
#include "SocketStreamError.h"
#include "SocketStreamHandle.h"
#include "SocketStreamServer.h"
#include "WebSocketChannel.h"
#include "WebSocketHandshake.h"
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;

namespace WKC {

WebSocketServerConnection::WebSocketServerConnection(int socket, WebSocketServerClient* client, WebSocketServer* server)
    : m_identifier(0)
    , m_mode(HTTP)
    , m_socket(nullptr)
    , m_server(server)
    , m_client(client)
{
    m_socket = SocketStreamServer::create(socket, *this);
}

WebSocketServerConnection::~WebSocketServerConnection()
{
    shutdownNow();
}

void WebSocketServerConnection::shutdownNow()
{
    if (!m_socket)
        return;
    RefPtr<SocketStreamServer> socket = m_socket/*.release()*/;
    socket->close();
}

void WebSocketServerConnection::shutdownAfterSendOrNow()
{
    // If this ASSERT happens on any platform then their SocketStreamHandle::send
    // followed by a SocketStreamHandle::close is not guarenteed to have sent all
    // data. If this happens, we need to slightly change the design to include a
    // SocketStreamHandleClient::didSend, handle it here, and add an m_shutdownAfterSend
    // state on this WebSocketServerConnection.
    // ASSERT(!m_socket->bufferedAmount());

    shutdownNow();
}

void WebSocketServerConnection::sendWebSocketMessage(const WTF::String& message)
{
    CString payload = message.utf8();
    const bool final = true, compress = false, masked = false;
    WebSocketFrame frame(WebSocketFrame::OpCodeText, final, compress, masked, payload.data(), payload.length());

    Vector<char> frameData;
    frame.makeFrameData(frameData);

    m_socket->sendData(frameData.data(), frameData.size(), [] (bool success) {
        if (!success)
            LOG_ERROR("Failed to send WebSocket message.");
    });
}

void WebSocketServerConnection::sendHTTPResponseHeader(int statusCode, const WTF::String& statusText, const HTTPHeaderMap& headerFields)
{
    StringBuilder builder;
    builder.append("HTTP/1.1 ");
    builder.append(WTF::String::number(statusCode));
    builder.append(" ");
    builder.append(statusText);
    builder.append("\r\n");
    HTTPHeaderMap::const_iterator end = headerFields.end();
    for (HTTPHeaderMap::const_iterator it = headerFields.begin(); it != end; ++it) {
        builder.append(it->key);
        builder.append(": ");
        builder.append(it->value.utf8().data());
        builder.append("\r\n");
    }
    builder.append("\r\n");

    CString header = builder.toString().latin1();
    m_socket->sendData(header.data(), header.length(), [] (bool success) {
        if (!success)
            LOG_ERROR("Failed to send HTTP Response Header for WebSocket.");
    });
}

void WebSocketServerConnection::sendRawData(const char* data, size_t length)
{
    m_socket->sendData(data, length, [] (bool success) {
        if (!success)
            LOG_ERROR("Failed to send raw data.");
    });
}

void WebSocketServerConnection::didOpenSocketStream(SocketStreamHandle&)
{
}

void WebSocketServerConnection::didCloseSocketStream(SocketStreamHandle&)
{
    // Web Socket Mode.
    if (m_mode == WebSocket)
        m_client->didCloseWebSocketConnection(this);

    // Tell the server to get rid of this.
    m_server->didCloseWebSocketServerConnection(this);
}

void WebSocketServerConnection::didReceiveSocketStreamData(SocketStreamHandle&, const char* data, size_t length)
{
    // Each didReceiveData call adds more data to our buffer.
    // We clear the buffer when we have handled data from it.
    m_bufferedData.append(data, length);

    switch (m_mode) {
    case HTTP:
        readHTTPMessage();
        break;
    case WebSocket:
        readWebSocketFrames();
        break;
    default:
        // For any new modes added in the future.
        ASSERT_NOT_REACHED();
    }
}

void WebSocketServerConnection::didFailToReceiveSocketStreamData(SocketStreamHandle&)
{
}

void WebSocketServerConnection::didUpdateBufferedAmount(SocketStreamHandle&, size_t bufferedAmount)
{
}

void WebSocketServerConnection::didFailSocketStream(SocketStreamHandle&, const SocketStreamError&)
{
    // Possible read or write error.
}

void WebSocketServerConnection::readHTTPMessage()
{
    WTF::String failureReason;
    RefPtr<HTTPRequest> request = HTTPRequest::parseHTTPRequestFromBuffer(m_bufferedData.data(), m_bufferedData.size(), failureReason);
    if (!request)
        return;

    // Assume all the input has been read if we are reading an HTTP Request.
    m_bufferedData.clear();

    // If this is a WebSocket request, perform the WebSocket Handshake.
    const HTTPHeaderMap& headers = request->headerFields();
    WTF::String upgradeHeaderValue = headers.get(WTF::String("Upgrade"));
    if (upgradeHeaderValue == "websocket") {
        upgradeToWebSocketServerConnection(request);
        return;
    }
    if (upgradeHeaderValue == "WebSocket") {
        LOG_ERROR("WebSocket protocol version < Hybi-10 not supported. Upgrade your client.");
        return;
    }

    // Otherwise, this is an HTTP Request we don't know how to deal with.
    m_client->didReceiveUnrecognizedHTTPRequest(this, request);
}

void WebSocketServerConnection::upgradeToWebSocketServerConnection(RefPtr<HTTPRequest> request)
{
    ASSERT(request);
    ASSERT(m_mode == HTTP);
    m_mode = WebSocket;
    RefPtr<HTTPRequest> protectedRequest(request);

    // Ask the client if we should upgrade for this or not.
    if (!m_client->didReceiveWebSocketUpgradeHTTPRequest(this, protectedRequest)) {
        shutdownNow();
        return;
    }

    // Build and send the WebSocket handshake response.
    const HTTPHeaderMap& requestHeaders = protectedRequest->headerFields();
    WTF::String accept = WebSocketHandshake::getExpectedWebSocketAccept(requestHeaders.get(WTF::String("Sec-WebSocket-Key")));
    HTTPHeaderMap responseHeaders;
    responseHeaders.add(WTF::String("Upgrade"), requestHeaders.get(WTF::String("Upgrade")));
    responseHeaders.add(WTF::String("Connection"), requestHeaders.get(WTF::String("Connection")));
    responseHeaders.add(WTF::String("Sec-WebSocket-Accept"), accept);

    sendHTTPResponseHeader(101, "WebSocket Protocol Handshake", responseHeaders);

    m_client->didEstablishWebSocketConnection(this, protectedRequest);
}

void WebSocketServerConnection::readWebSocketFrames()
{
    while (true) {
        bool didReadOneFrame = readWebSocketFrame();
        if (!didReadOneFrame)
            break;
        if (m_bufferedData.isEmpty())
            break;
    }
}

bool WebSocketServerConnection::readWebSocketFrame()
{
    WebSocketFrame frame;
    const char* frameEnd;
    WTF::String errorString;
    WebSocketFrame::ParseFrameResult result = WebSocketFrame::parseFrame(m_bufferedData.data(), m_bufferedData.size(), frame, frameEnd, errorString);

    // Incomplete frame. Wait to receive more data.
    if (result == WebSocketFrame::FrameIncomplete)
        return false;

    if (result == WebSocketFrame::FrameError) {
        shutdownNow();
    } else if (frame.opCode == WebSocketFrame::OpCodeText) {
        // Delegate Text frames to our client.
        WTF::String msg = WTF::String::fromUTF8(frame.payload, frame.payloadLength);
        m_client->didReceiveWebSocketMessage(this, msg);
    } else
        notImplemented();

    // Remove the frame from our buffer.
    m_bufferedData.remove(0, frameEnd - m_bufferedData.data());

    return true;
}

}

#endif // ENABLE(REMOTE_INSPECTOR)

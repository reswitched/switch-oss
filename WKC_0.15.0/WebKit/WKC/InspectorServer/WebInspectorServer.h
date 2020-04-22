/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (c) 2012-2016 ACCESS CO., LTD. All rights reserved.
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

#ifndef WebInspectorServer_h
#define WebInspectorServer_h

#if ENABLE(REMOTE_INSPECTOR)

#include "WebSocketServer.h"
#include "WebSocketServerClient.h"
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

namespace WKC {

class WebInspectorServerClient;

class WebInspectorServer : public WebSocketServer, public WebSocketServerClient {
WTF_MAKE_FAST_ALLOCATED;
public:
    typedef HashMap<unsigned, WebInspectorServerClient*> ClientMap;
    static void createSharedInstance();
    static WebInspectorServer* sharedInstance();
    static void deleteSharedInstance();
    static void forceTerminate();

    static void setResourcePath(const char* in_path);

    // Page registry to manage known pages.
    int registerPage(WebInspectorServerClient* client);
    void unregisterPage(int pageId);
    void sendMessageOverConnection(unsigned pageIdForConnection, const WTF::String& message);

private:
    WebInspectorServer();
    ~WebInspectorServer();

    // WebSocketServerClient implementation. Events coming from remote connections.
    virtual void didReceiveUnrecognizedHTTPRequest(WebSocketServerConnection*, PassRefPtr<WebKit::HTTPRequest>);
    virtual bool didReceiveWebSocketUpgradeHTTPRequest(WebSocketServerConnection*, PassRefPtr<WebKit::HTTPRequest>);
    virtual void didEstablishWebSocketConnection(WebSocketServerConnection*, PassRefPtr<WebKit::HTTPRequest>);
    virtual void didReceiveWebSocketMessage(WebSocketServerConnection*, const WTF::String& message);
    virtual void didCloseWebSocketConnection(WebSocketServerConnection*);

    bool platformResourceForPath(const WTF::String& path, Vector<char>& data, WTF::String& contentType);
#if PLATFORM(WKC)
    void buildPageList(Vector<char>& data, WTF::String& contentType, bool jsonp = false);
#endif

    void closeConnection(WebInspectorServerClient*, WebSocketServerConnection*);

    unsigned m_nextAvailablePageId;
    ClientMap m_clientMap;
    HashMap<unsigned, WebSocketServerConnection*> m_connectionMap;
    WKC_DEFINE_GLOBAL_CLASS_OBJ_ENTRY(WebInspectorServer*, m_sharedInstance);
};

}

#endif // ENABLE(REMOTE_INSPECTOR)

#endif // WebInspectorServer_h

/*
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

#ifndef InspectorServerClientWKC_h
#define InspectorServerClientWKC_h

#if ENABLE(REMOTE_INSPECTOR)

#include "WebInspectorServerClient.h"
#include "WKCWebViewPrivate.h"

#include "HTTPRequest.h"

#include "InspectorFrontendChannel.h"

namespace WKC {
class WKCWebView;

class InspectorServerClientWKC : public WebInspectorServerClient, public Inspector::FrontendChannel {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static InspectorServerClientWKC* create(WKCWebViewPrivate*);
    ~InspectorServerClientWKC();

    // from WebInspectorServerClient
    virtual void remoteFrontendConnected();
    virtual void remoteFrontendDisconnected();
    virtual void dispatchMessageFromRemoteFrontend(const WTF::String& message);
    virtual WKCWebView* webView() { return m_view->parent(); }

    void sendMessageToRemoteFrontend(const WTF::String& message);
    void enableRemoteInspection();
    void disableRemoteInspection();
    bool hasRemoteFrontendConnected() { return m_remoteFrontendConnected; }

    // InspectorFrontendChannel
    virtual Inspector::FrontendChannel::ConnectionType connectionType() const;
    virtual void sendMessageToFrontend(const WTF::String& message);

private:
   InspectorServerClientWKC(WKCWebViewPrivate*);

   WKCWebViewPrivate* m_view;
   int m_remoteInspectionPageId;
   bool m_remoteFrontendConnected;
};

} // namespace

#endif // ENABLE(REMOTE_INSPECTOR)

#endif // InspectorServerClientWKC_h

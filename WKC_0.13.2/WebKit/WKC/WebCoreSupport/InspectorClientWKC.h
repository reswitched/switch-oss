/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (c) 2010-2016 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorClientWKC_h
#define InspectorClientWKC_h

#include "WKCOverlayIf.h"

#include "InspectorClient.h"

namespace WebCore {
    class Node;
    class Page;
}

namespace WTF {
    class String;
}

namespace WKC {

class InspectorClientIf;
class WKCWebViewPrivate;
class WKCWebInspector;
class InspectorServerClientWKC;

class InspectorClientWKC : public WebCore::InspectorClient, WKCOverlayIf {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static InspectorClientWKC* create(WKCWebViewPrivate* webView);
    ~InspectorClientWKC();

    virtual void inspectorDestroyed();
    void webViewDestroyed();

    virtual Inspector::FrontendChannel* openInspectorFrontend(WebCore::InspectorController *);
    virtual void closeInspectorFrontend();
    virtual void bringFrontendToFront();

    virtual WebCore::Page* createPage();

    virtual WTF::String localizedStringsURL();

    virtual WTF::String hiddenPanels();

    virtual void showWindow();
    virtual void closeWindow();

    virtual void attachWindow();
    virtual void detachWindow();

    virtual void setAttachedWindowHeight(unsigned height);

    virtual void highlight();
    virtual void hideHighlight();
    virtual void inspectedURLChanged(const WTF::String& newURL);

    virtual void populateSetting(const WTF::String& key, WTF::String* value);
    virtual void storeSetting(const WTF::String& key, const WTF::String& value);

    virtual void inspectorWindowObjectCleared();

    virtual bool sendMessageToFrontend(const WTF::String& message);

#if ENABLE(REMOTE_INSPECTOR)
    void setInspectorServerClient(InspectorServerClientWKC* serverClient) { m_inspectorServerClient = serverClient; }
#endif

    // WKCOverlayIf
    virtual void paintOverlay(WebCore::GraphicsContext&);

private:
    InspectorClientWKC(WKCWebViewPrivate* webView);
    bool construct();

private:
    WKCWebViewPrivate* m_view;
    WKC::InspectorClientIf* m_appClient;
    WKCWebViewPrivate* m_inspectedWebView;
    WKCWebInspector* m_webInspector;
#if ENABLE(REMOTE_INSPECTOR)
    InspectorServerClientWKC* m_inspectorServerClient;
#endif
};

} // namespace

#endif // InspectorClientWKC_h

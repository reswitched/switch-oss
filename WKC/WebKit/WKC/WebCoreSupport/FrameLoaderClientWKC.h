/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * All rights reserved.
 * Copyright (c) 2010-2016 ACCESS CO., LTD. All rights reserved.
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

#ifndef FrameLoaderClientWKC_h
#define FrameLoaderClientWKC_h

#include "FrameLoaderClient.h"
#include "FrameNetworkingContext.h"
#include "ResourceResponse.h"
#if ENABLE(NETSCAPE_PLUGIN_API)
#include "PluginView.h"
#endif
#include "WKCEnums.h"
#include "WTFString.h"

#ifdef WKC_ENABLE_CUSTOMJS
#include <wkc/wkccustomjs.h>
#endif // WKC_ENABLE_CUSTOMJS

namespace WKC {

class FrameLoaderClientIf;
class WKCWebFramePrivate;
class WKCWebFrame;
class WKCPolicyDecision;

class FrameLoaderClientWKC : public WebCore::FrameLoaderClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static FrameLoaderClientWKC* create(WKCWebFramePrivate* frame = 0);
    virtual ~FrameLoaderClientWKC();

    WKCWebFrame* webFrame() const;

    //
    // Inheritance FrameLoaderClient
    //
    /* hasHTMLView() */

    virtual void frameLoaderDestroyed() override;

    virtual bool hasWebView() const override;
    virtual void makeRepresentation(WebCore::DocumentLoader*) override;
    virtual void forceLayoutForNonHTML() override;
    virtual void setCopiesOnScroll() override;
    virtual void detachedFromParent2() override;
    virtual void detachedFromParent3() override;
    virtual void assignIdentifierToInitialRequest(unsigned long identifier, WebCore::DocumentLoader*, const WebCore::ResourceRequest&) override;
    virtual void dispatchWillSendRequest(WebCore::DocumentLoader*, unsigned long  identifier, WebCore::ResourceRequest&, const WebCore::ResourceResponse& redirectResponse) override;
    virtual bool shouldUseCredentialStorage(WebCore::DocumentLoader*, unsigned long identifier) override;
    virtual void dispatchDidReceiveAuthenticationChallenge(WebCore::DocumentLoader*, unsigned long identifier, const WebCore::AuthenticationChallenge&) override;
    virtual void dispatchDidCancelAuthenticationChallenge(WebCore::DocumentLoader*, unsigned long  identifier, const WebCore::AuthenticationChallenge&) override;
    virtual void dispatchDidReceiveResponse(WebCore::DocumentLoader*, unsigned long  identifier, const WebCore::ResourceResponse&) override;
    virtual void dispatchDidReceiveContentLength(WebCore::DocumentLoader*, unsigned long identifier, int lengthReceived) override;
    virtual void dispatchDidFinishLoading(WebCore::DocumentLoader*, unsigned long  identifier) override;
    virtual void dispatchDidFailLoading(WebCore::DocumentLoader*, unsigned long  identifier, const WebCore::ResourceError&) override;
    virtual bool dispatchDidLoadResourceFromMemoryCache(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&, int length) override;

    virtual void dispatchDidHandleOnloadEvents() override;
    virtual void dispatchDidReceiveServerRedirectForProvisionalLoad() override;
    virtual void dispatchDidCancelClientRedirect() override;
    virtual void dispatchWillPerformClientRedirect(const WebCore::URL&, double, double) override;
    virtual void dispatchDidNavigateWithinPage() override;
    virtual void dispatchDidChangeLocationWithinPage() override;
    virtual void dispatchDidPushStateWithinPage() override;
    virtual void dispatchDidReplaceStateWithinPage() override;
    virtual void dispatchDidPopStateWithinPage() override;
    virtual void dispatchWillClose() override;
    virtual void dispatchDidReceiveIcon() override;
    virtual void dispatchDidStartProvisionalLoad() override;
    virtual void dispatchDidReceiveTitle(const WebCore::StringWithDirection&) override;
    virtual void dispatchDidChangeIcons(WebCore::IconType) override;
    virtual void dispatchDidCommitLoad() override;
    virtual void dispatchDidFailProvisionalLoad(const WebCore::ResourceError&) override;
    virtual void dispatchDidFailLoad(const WebCore::ResourceError&) override;
    virtual void dispatchDidFinishDocumentLoad() override;
    virtual void dispatchDidFinishLoad() override;
    virtual void dispatchDidLayout() override;
    virtual void dispatchDidLayout(WebCore::LayoutMilestones) override;

    virtual WebCore::Frame* dispatchCreatePage(const WebCore::NavigationAction&) override;
    virtual void dispatchShow() override;

    virtual void dispatchDecidePolicyForResponse(const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, WebCore::FramePolicyFunction) override;
    virtual void dispatchDecidePolicyForNewWindowAction(const WebCore::NavigationAction&, const WebCore::ResourceRequest&, WTF::PassRefPtr<WebCore::FormState>, const WTF::String& frameName, WebCore::FramePolicyFunction) override;
    virtual void dispatchDecidePolicyForNavigationAction(const WebCore::NavigationAction&, const WebCore::ResourceRequest&, WTF::PassRefPtr<WebCore::FormState>, WebCore::FramePolicyFunction) override;
    virtual void cancelPolicyCheck() override;
    virtual void dispatchUnableToImplementPolicy(const WebCore::ResourceError&) override;

    virtual void dispatchWillSendSubmitEvent(WTF::PassRefPtr<WebCore::FormState>) override;
    virtual void dispatchWillSubmitForm(WTF::PassRefPtr<WebCore::FormState>, WebCore::FramePolicyFunction) override;

    virtual void revertToProvisionalState(WebCore::DocumentLoader*) override;
    virtual void setMainDocumentError(WebCore::DocumentLoader*, const WebCore::ResourceError&) override;

    virtual void setMainFrameDocumentReady(bool) override;

    virtual void startDownload(const WebCore::ResourceRequest&, const WTF::String& suggestedName) override;
    virtual void convertMainResourceLoadToDownload(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&) override;

    virtual void willChangeTitle(WebCore::DocumentLoader*) override;
    virtual void didChangeTitle(WebCore::DocumentLoader*) override;

    virtual void willReplaceMultipartContent() override;
    virtual void didReplaceMultipartContent() override;

    virtual void committedLoad(WebCore::DocumentLoader*, const char*, int) override;
    virtual void finishedLoading(WebCore::DocumentLoader*) override;

    virtual void updateGlobalHistory() override;
    virtual void updateGlobalHistoryRedirectLinks() override;
    virtual bool shouldGoToHistoryItem(WebCore::HistoryItem*) const override;
    virtual void updateGlobalHistoryItemForPage() override;

    // This frame has displayed inactive content (such as an image) from an
    // insecure source.  Inactive content cannot spread to other frames.
    virtual void didDisplayInsecureContent() override;
    // The indicated security origin has run active content (such as a
    // script) from an insecure source.  Note that the insecure content can
    // spread to other frames in the same origin.
    virtual void didRunInsecureContent(WebCore::SecurityOrigin*, const WebCore::URL&) override;
    virtual void didDetectXSS(const WebCore::URL&, bool didBlockEntirePage) override;

    virtual WebCore::ResourceError cancelledError(const WebCore::ResourceRequest&) override;
    virtual WebCore::ResourceError blockedError(const WebCore::ResourceRequest&) override;
    virtual WebCore::ResourceError cannotShowURLError(const WebCore::ResourceRequest&) override;
    virtual WebCore::ResourceError interruptedForPolicyChangeError(const WebCore::ResourceRequest&) override;
    virtual WebCore::ResourceError cannotShowMIMETypeError(const WebCore::ResourceResponse&) override;
    virtual WebCore::ResourceError fileDoesNotExistError(const WebCore::ResourceResponse&) override;
    virtual WebCore::ResourceError pluginWillHandleLoadError(const WebCore::ResourceResponse&) override;

    virtual bool shouldFallBack(const WebCore::ResourceError&) override;

    virtual bool canHandleRequest(const WebCore::ResourceRequest&) const override;
    virtual bool canShowMIMEType(const WTF::String&) const override;
    virtual bool canShowMIMETypeAsHTML(const WTF::String& MIMEType) const override;
    virtual bool representationExistsForURLScheme(const WTF::String&) const override;
    virtual WTF::String generatedMIMETypeForURLScheme(const WTF::String&) const override;

    virtual void frameLoadCompleted() override;
    virtual void saveViewStateToItem(WebCore::HistoryItem*) override;
    virtual void restoreViewState() override;
    virtual void provisionalLoadStarted() override;
    virtual void didFinishLoad() override;
    virtual void prepareForDataSourceReplacement() override;

    virtual WTF::PassRefPtr<WebCore::DocumentLoader> createDocumentLoader(const WebCore::ResourceRequest&, const WebCore::SubstituteData&) override;
    virtual void updateCachedDocumentLoader(WebCore::DocumentLoader&) override;

    virtual void setTitle(const WebCore::StringWithDirection& title, const WebCore::URL&) override;

    virtual WTF::String userAgent(const WebCore::URL&) override;

    virtual void savePlatformDataToCachedFrame(WebCore::CachedFrame*) override;
    virtual void transitionToCommittedFromCachedFrame(WebCore::CachedFrame*) override;
    virtual void transitionToCommittedForNewPage() override;

    virtual void didSaveToPageCache() override;
    virtual void didRestoreFromPageCache() override;

    virtual void dispatchDidBecomeFrameset(bool) override;

    virtual bool canCachePage() const override;

    virtual PassRefPtr<WebCore::Frame> createFrame(const WebCore::URL& url, const WTF::String& name, WebCore::HTMLFrameOwnerElement* ownerElement,
                                                           const WTF::String& referrer, bool allowsScrolling, int marginWidth, int marginHeight) override;
#if ENABLE(NETSCAPE_PLUGIN_API)
    virtual PassRefPtr<WebCore::Widget> createPlugin(const WebCore::IntSize&, WebCore::HTMLPlugInElement*, const WebCore::URL&, const WTF::Vector<WTF::String>&, const WTF::Vector<WTF::String>&, const WTF::String&, bool) override;
    virtual void recreatePlugin(WebCore::Widget*) override;
    virtual void redirectDataToPlugin(WebCore::Widget* pluginWidget) override;
    virtual void dispatchDidFailToStartPlugin(const WebCore::PluginView*) const override;
#endif
    virtual PassRefPtr<WebCore::Widget> createJavaAppletWidget(const WebCore::IntSize&, WebCore::HTMLAppletElement*, const WebCore::URL& baseURL, const WTF::Vector<WTF::String>& paramNames, const WTF::Vector<WTF::String>& paramValues) override;

    virtual WebCore::ObjectContentType objectContentType(const WebCore::URL&, const WTF::String& mimeType, bool shouldPreferPlugInsForImages) override;
    virtual WTF::String overrideMediaType() const override;

    virtual void dispatchDidClearWindowObjectInWorld(WebCore::DOMWrapperWorld&) override;

    virtual void registerForIconNotification(bool) override;

    virtual void didChangeScrollOffset() override;

    virtual bool allowScript(bool enabledPerSettings) override;
    virtual bool allowRunningInsecureContent(bool enabledPerSettings, WebCore::SecurityOrigin*, const WebCore::URL&);

    virtual bool shouldForceUniversalAccessFromLocalURL(const WebCore::URL&) override;

    virtual WTF::PassRefPtr<WebCore::FrameNetworkingContext> createNetworkingContext() override;

    virtual void dispatchGlobalObjectAvailable(WebCore::DOMWrapperWorld&) override;
    virtual void dispatchWillDisconnectDOMWindowExtensionFromGlobalObject(WebCore::DOMWindowExtension*) override;
    virtual void dispatchDidReconnectDOMWindowExtensionToGlobalObject(WebCore::DOMWindowExtension*) override;
    virtual void dispatchWillDestroyGlobalObjectForDOMWindowExtension(WebCore::DOMWindowExtension*) override;

#if ENABLE(WEBGL)
    virtual bool allowWebGL(bool enabledPerSettings) override;
    virtual void didLoseWebGLContext(int) override;
#endif

    virtual void forcePageTransitionIfNeeded() override;
    virtual bool isEmptyFrameLoaderClient() override { return false; }

    //
    // WKC extension
    //
public:
    bool byWKC(void) { return true; }
    bool notifySSLHandshakeStatus(WebCore::ResourceHandle* handle, int status);
    bool dispatchWillAcceptCookie(bool income, WebCore::ResourceHandle* handle, const WTF::String& url, const WTF::String& firstparty_host, const WTF::String& cookie_domain);
    bool dispatchWillReceiveData(WebCore::ResourceHandle*, int length);
    int  requestSSLClientCertSelect(WebCore::ResourceHandle* handle, const char* requester, void* certs, int num);
    int  dispatchWillPermitSendRequest(WebCore::ResourceHandle* handle, const char* url, int composition, bool isSync, const WebCore::ResourceResponse& redirectResponse);
    void didRestoreFromHTTPCache(WebCore::ResourceHandle* handle, const WebCore::URL& url);

#ifdef WKC_ENABLE_CUSTOMJS
    virtual bool dispatchWillCallCustomJS(WKCCustomJSAPIList* api, void** context); //override;
#endif // WKC_ENABLE_CUSTOMJS

    bool setMainFrame(WKCWebFramePrivate* frame);

private:
    FrameLoaderClientWKC(WKCWebFramePrivate*);
    bool construct();
    void notifyStatus(WKC::LoadStatus loadStatus);

private:
    WKCWebFramePrivate* m_frame;
    WKC::FrameLoaderClientIf* m_appClient;
    WebCore::ResourceResponse m_response;
    WKCPolicyDecision* m_policyDecision;

    // Plugin view to redirect data to
#if ENABLE(NETSCAPE_PLUGIN_API)
    WebCore::PluginView* m_pluginView;
    bool m_hasSentResponseToPlugin;
#endif
};

class FrameNetworkingContextWKC : public WebCore::FrameNetworkingContext
{
public:
    static FrameNetworkingContextWKC* create(WebCore::Frame*);
public:
    virtual WebCore::ResourceLoader* mainResourceLoader() const override;
    virtual WebCore::FrameLoaderClient* frameLoaderClient() const override;

public:
    WebCore::Frame* coreFrame(){ return frame(); }

private:
    FrameNetworkingContextWKC(WebCore::Frame*);
};

} // namespace

#endif // FrameLoaderClientWKC_h

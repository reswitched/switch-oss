/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * All rights reserved.
 * Copyright (c) 2010-2021 ACCESS CO., LTD. All rights reserved.
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

#if PLATFORM(WKC)
#include "LayoutMilestone.h"
#endif

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

    virtual void frameLoaderDestroyed() override final;

    virtual bool hasWebView() const override final;
    virtual void makeRepresentation(WebCore::DocumentLoader*) override final;
    virtual Optional<WebCore::PageIdentifier> pageID() const override final;
    virtual Optional<WebCore::FrameIdentifier> frameID() const override final;
    virtual void forceLayoutForNonHTML() override final;
    virtual void setCopiesOnScroll() override final;
    virtual void detachedFromParent2() override final;
    virtual void detachedFromParent3() override final;
    virtual void assignIdentifierToInitialRequest(unsigned long identifier, WebCore::DocumentLoader*, const WebCore::ResourceRequest&) override final;
    virtual void dispatchWillSendRequest(WebCore::DocumentLoader*, unsigned long  identifier, WebCore::ResourceRequest&, const WebCore::ResourceResponse& redirectResponse) override final;
    virtual bool shouldUseCredentialStorage(WebCore::DocumentLoader*, unsigned long identifier) override final;
    virtual void dispatchDidReceiveAuthenticationChallenge(WebCore::DocumentLoader*, unsigned long identifier, const WebCore::AuthenticationChallenge&) override final;
    virtual void dispatchDidReceiveResponse(WebCore::DocumentLoader*, unsigned long  identifier, const WebCore::ResourceResponse&) override final;
    virtual void dispatchDidReceiveContentLength(WebCore::DocumentLoader*, unsigned long identifier, int lengthReceived) override final;
    virtual void dispatchDidFinishLoading(WebCore::DocumentLoader*, unsigned long  identifier) override final;
    virtual void dispatchDidFailLoading(WebCore::DocumentLoader*, unsigned long  identifier, const WebCore::ResourceError&) override final;
    virtual bool dispatchDidLoadResourceFromMemoryCache(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&, int length) override final;

    virtual void dispatchDidDispatchOnloadEvents() override final;
    virtual void dispatchDidReceiveServerRedirectForProvisionalLoad() override final;
    virtual void dispatchDidCancelClientRedirect() override final;
    virtual void dispatchWillPerformClientRedirect(const WTF::URL&, double, WallTime, WebCore::LockBackForwardList) override final;
    virtual void dispatchDidNavigateWithinPage() override final;
    virtual void dispatchDidChangeLocationWithinPage() override final;
    virtual void dispatchDidPushStateWithinPage() override final;
    virtual void dispatchDidReplaceStateWithinPage() override final;
    virtual void dispatchDidPopStateWithinPage() override final;
    virtual void dispatchWillClose() override final;
    virtual void dispatchDidReceiveIcon() override final;
    virtual void dispatchDidStartProvisionalLoad() override final;
    virtual void dispatchDidReceiveTitle(const WebCore::StringWithDirection&) override final;
    virtual void dispatchDidCommitLoad(Optional<WebCore::HasInsecureContent>, Optional<WebCore::UsedLegacyTLS>) override final;
    virtual void dispatchDidFailProvisionalLoad(const WebCore::ResourceError&, WebCore::WillContinueLoading) override final;
    virtual void dispatchDidFailLoad(const WebCore::ResourceError&) override final;
    virtual void dispatchDidFinishDocumentLoad() override final;
    virtual void dispatchDidFinishLoad() override final;
    virtual void dispatchDidLayout() override final;
    virtual void dispatchDidReachLayoutMilestone(OptionSet<WebCore::LayoutMilestone>) override final;

    virtual WebCore::Frame* dispatchCreatePage(const WebCore::NavigationAction&) override final;
    virtual void dispatchShow() override final;

    virtual void dispatchDecidePolicyForResponse(const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, WebCore::PolicyCheckIdentifier, const WTF::String&, WebCore::FramePolicyFunction&&) override final;
    virtual void dispatchDecidePolicyForNewWindowAction(const WebCore::NavigationAction&, const WebCore::ResourceRequest&, WebCore::FormState*, const WTF::String& frameName, WebCore::PolicyCheckIdentifier, WebCore::FramePolicyFunction&&) override final;
    virtual void dispatchDecidePolicyForNavigationAction(const WebCore::NavigationAction&, const WebCore::ResourceRequest&, const WebCore::ResourceResponse& redirectResponse, WebCore::FormState*, WebCore::PolicyDecisionMode, WebCore::PolicyCheckIdentifier, WebCore::FramePolicyFunction&&) override final;
    virtual void cancelPolicyCheck() override final;
    virtual void dispatchUnableToImplementPolicy(const WebCore::ResourceError&) override final;

    virtual void dispatchWillSendSubmitEvent(WTF::Ref<WebCore::FormState>&&) override final;
    virtual void dispatchWillSubmitForm(WebCore::FormState&, CompletionHandler<void()>&&) override final;

    virtual void revertToProvisionalState(WebCore::DocumentLoader*) override final;
    virtual void setMainDocumentError(WebCore::DocumentLoader*, const WebCore::ResourceError&) override final;

    virtual void setMainFrameDocumentReady(bool) override final;

    virtual void startDownload(const WebCore::ResourceRequest&, const WTF::String& suggestedName) override final;
    virtual void convertMainResourceLoadToDownload(WebCore::DocumentLoader*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&) override final;

    virtual void willChangeTitle(WebCore::DocumentLoader*) override final;
    virtual void didChangeTitle(WebCore::DocumentLoader*) override final;

    virtual void willReplaceMultipartContent() override final;
    virtual void didReplaceMultipartContent() override final;

    virtual void committedLoad(WebCore::DocumentLoader*, const char*, int) override final;
    virtual void finishedLoading(WebCore::DocumentLoader*) override final;

    virtual void updateGlobalHistory() override final;
    virtual void updateGlobalHistoryRedirectLinks() override final;
    virtual bool shouldGoToHistoryItem(WebCore::HistoryItem&) const override final;

    // This frame has displayed inactive content (such as an image) from an
    // insecure source.  Inactive content cannot spread to other frames.
    virtual void didDisplayInsecureContent() override final;
    // The indicated security origin has run active content (such as a
    // script) from an insecure source.  Note that the insecure content can
    // spread to other frames in the same origin.
    virtual void didRunInsecureContent(WebCore::SecurityOrigin&, const WTF::URL&) override final;
    virtual void didDetectXSS(const WTF::URL&, bool didBlockEntirePage) override final;

    virtual WebCore::ResourceError cancelledError(const WebCore::ResourceRequest&) override final;
    virtual WebCore::ResourceError blockedError(const WebCore::ResourceRequest&) override final;
    virtual WebCore::ResourceError blockedByContentBlockerError(const WebCore::ResourceRequest&) override final;
    virtual WebCore::ResourceError cannotShowURLError(const WebCore::ResourceRequest&) override final;
    virtual WebCore::ResourceError interruptedForPolicyChangeError(const WebCore::ResourceRequest&) override final;
    virtual WebCore::ResourceError cannotShowMIMETypeError(const WebCore::ResourceResponse&) override final;
    virtual WebCore::ResourceError fileDoesNotExistError(const WebCore::ResourceResponse&) override final;
    virtual WebCore::ResourceError pluginWillHandleLoadError(const WebCore::ResourceResponse&) override final;

    virtual bool shouldFallBack(const WebCore::ResourceError&) override final;

    virtual bool canHandleRequest(const WebCore::ResourceRequest&) const override final;
    virtual bool canShowMIMEType(const WTF::String&) const override final;
    virtual bool canShowMIMETypeAsHTML(const WTF::String& MIMEType) const override final;
    virtual bool representationExistsForURLScheme(const WTF::String&) const override final;
    virtual WTF::String generatedMIMETypeForURLScheme(const WTF::String&) const override final;

    virtual void frameLoadCompleted() override final;
    virtual void saveViewStateToItem(WebCore::HistoryItem&) override final;
    virtual void restoreViewState() override final;
    virtual void provisionalLoadStarted() override final;
    virtual void didFinishLoad() override final;
    virtual void prepareForDataSourceReplacement() override final;

    virtual WTF::Ref<WebCore::DocumentLoader> createDocumentLoader(const WebCore::ResourceRequest&, const WebCore::SubstituteData&) override final;
    virtual void updateCachedDocumentLoader(WebCore::DocumentLoader&) override final;

    virtual void setTitle(const WebCore::StringWithDirection& title, const WTF::URL&) override final;

    virtual WTF::String userAgent(const WTF::URL&) override final;

    virtual void savePlatformDataToCachedFrame(WebCore::CachedFrame*) override final;
    virtual void transitionToCommittedFromCachedFrame(WebCore::CachedFrame*) override final;
    virtual void transitionToCommittedForNewPage() override final;

    virtual void didRestoreFromBackForwardCache() override final;

    virtual void dispatchDidBecomeFrameset(bool) override final;

    virtual bool canCachePage() const override final;

    virtual RefPtr<WebCore::Frame> createFrame(const WTF::URL& url, const WTF::String& name, WebCore::HTMLFrameOwnerElement& ownerElement, const WTF::String& referrer) override final;
#if ENABLE(NETSCAPE_PLUGIN_API)
    virtual RefPtr<WebCore::Widget> createPlugin(const WebCore::IntSize&, WebCore::HTMLPlugInElement*, const WTF::URL&, const WTF::Vector<WTF::String>&, const WTF::Vector<WTF::String>&, const WTF::String&, bool) override final;
    virtual void recreatePlugin(WebCore::Widget*) override final;
    virtual void redirectDataToPlugin(WebCore::Widget* pluginWidget) override final;
    virtual void dispatchDidFailToStartPlugin(const WebCore::PluginView*) const override final;
#endif
    virtual RefPtr<WebCore::Widget> createJavaAppletWidget(const WebCore::IntSize&, WebCore::HTMLAppletElement&, const WTF::URL& baseURL, const WTF::Vector<WTF::String>& paramNames, const WTF::Vector<WTF::String>& paramValues) override final;

    virtual WebCore::ObjectContentType objectContentType(const WTF::URL&, const WTF::String& mimeType) override final;
    virtual WTF::String overrideMediaType() const override final;

    virtual void dispatchDidClearWindowObjectInWorld(WebCore::DOMWrapperWorld&) override final;

    virtual void registerForIconNotification() override final;

    virtual void didChangeScrollOffset() override final;

    virtual bool allowScript(bool enabledPerSettings) override final;

    virtual bool shouldForceUniversalAccessFromLocalURL(const WTF::URL&) override final;

    virtual WTF::Ref<WebCore::FrameNetworkingContext> createNetworkingContext() override final;

    virtual void dispatchGlobalObjectAvailable(WebCore::DOMWrapperWorld&) override final;
    virtual void dispatchWillDisconnectDOMWindowExtensionFromGlobalObject(WebCore::DOMWindowExtension*) override final;
    virtual void dispatchDidReconnectDOMWindowExtensionToGlobalObject(WebCore::DOMWindowExtension*) override final;
    virtual void dispatchWillDestroyGlobalObjectForDOMWindowExtension(WebCore::DOMWindowExtension*) override final;

#if ENABLE(WEBGL)
    virtual bool allowWebGL(bool enabledPerSettings) override final;
    virtual void didLoseWebGLContext(int) override final;
#endif

    virtual void forcePageTransitionIfNeeded() override final;
    virtual bool isEmptyFrameLoaderClient() const override final { return false; }

    virtual void prefetchDNS(const WTF::String&) override final;

    //
    // WKC extension
    //
public:
    virtual bool byWKC(void) override final { return true; }
    bool notifySSLHandshakeStatus(WebCore::ResourceHandle* handle, int status);
    bool dispatchWillReceiveData(WebCore::ResourceHandle*, int length);
    int  requestSSLClientCertSelect(WebCore::ResourceHandle* handle, const char* requester, void* certs, int num);
    int  dispatchWillPermitSendRequest(WebCore::ResourceHandle* handle, const char* url, int composition, bool isSync, const WebCore::ResourceResponse& redirectResponse);
    void didRestoreFromHTTPCache(WebCore::ResourceHandle* handle, const WTF::URL& url);

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
    static Ref<FrameNetworkingContextWKC> create(WebCore::Frame*);
public:
    virtual WebCore::ResourceLoader* mainResourceLoader() const override final;
    virtual WebCore::FrameLoaderClient* frameLoaderClient() const override final;
    virtual WebCore::NetworkStorageSession* storageSession() const override final;

public:
    WebCore::Frame* coreFrame(){ return frame(); }

private:
    FrameNetworkingContextWKC(WebCore::Frame*);
};

} // namespace

#endif // FrameLoaderClientWKC_h

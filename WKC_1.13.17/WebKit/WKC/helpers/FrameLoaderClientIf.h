/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008 Collabora Ltd. All rights reserved.
 * All rights reserved.
 * Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCFrameLoaderClient_h
#define WKCFrameLoaderClient_h

#include <wkc/wkcbase.h>
#include "WKCEnums.h"
#include "WKCHelpersEnums.h"

#ifdef WKC_ENABLE_CUSTOMJS
#include <wkc/wkccustomjs.h>
#endif // WKC_ENABLE_CUSTOMJS

namespace WKC {

class DocumentLoader;
class ResourceRequest;
class ResourceResponse;
class AuthenticationChallenge;
class ResourceError;
class KURL;
class ResourceLoader;
class String;
class FormState;
class HTMLFormElement;
class HTMLFrameOwnerElement;
class HTMLPlugInElement;
class Widget;
class HTMLPluginElement;
class HTMLAppletElement;
class DOMWrapperWorld;
class HistoryItem;
class ResourceHandle;
class NavigationAction;
class SecurityOrigin;
class SubstituteData;
class CachedFrame;
class Frame;
class ClientCertificate;
class Page;

class FrameLoaderClientWKC;

/*@{*/

class WKC_API FramePolicyFunction {
public:
    void reply(WKC::PolicyAction);

private:
    friend class FrameLoaderClientWKC;
    friend class FramePolicyFunctionPrivate;
    FramePolicyFunction(void* parent, void* func);
    ~FramePolicyFunction();
private:
    void* m_parent;
    void* m_func;
};

/**
@brief
Class that notifies of an event during frame loading
In this description, only those functions that were extended by ACCESS for NetFront Browser NX are described, those inherited from WebCore::FrameLoaderClient are not described.
*/
class WKC_API FrameLoaderClientIf {
public:

    //
    // Inheritance FrameLoaderClient
    //
    /* hasHTMLView() */

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of discarding frame loader client
       @return None
       @endcond
    */
    virtual void frameLoaderDestroyed() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Check if WebView is held
       @retval !false Holds WebView
       @retval false Does not hold WebView
       @endcond
    */
    virtual bool hasWebView() const = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DocumentLoader* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void makeRepresentation(WKC::DocumentLoader*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void forceLayout() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void forceLayoutForNonHTML() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void setCopiesOnScroll() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void detachedFromParent2() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void detachedFromParent3() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param identifier (TBD) implement description
       @param  WKC::DocumentLoader* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void assignIdentifierToInitialRequest(unsigned long identifier, WKC::DocumentLoader*, const WKC::ResourceRequest&) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of sending request
       @param loader Pointer to WKC::DocumentLoader
       @param identifier Request identifier
       @param request Reference to request object
       @param redirectResponse Reference to redirect object
       @return None
       @details
       Notification is given before sending a request.@n
       If the request is sent due to redirect, the response data is set in redirectResponse.
       @endcond
    */
    virtual void dispatchWillSendRequest(WKC::DocumentLoader*, unsigned long  identifier, WKC::ResourceRequest&, const WKC::ResourceResponse& redirectResponse) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DocumentLoader* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual bool shouldUseCredentialStorage(WKC::DocumentLoader*, unsigned long identifier) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of HTTP authentication
       @param loader Pointer to WKC::DocumentLoader
       @param identifier Request identifier
       @param challenge Reference to authentication information
       @return None
       @details
       Notification is given when HTTP authentication is required for the requested URL.
       @endcond
    */
    virtual void dispatchDidReceiveAuthenticationChallenge(WKC::DocumentLoader*, unsigned long identifier, const WKC::AuthenticationChallenge&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of HTTP response
       @param loader Pointer to WKC::DocumentLoader
       @param identifier Request identifier
       @param response Reference to response object
       @return None
       @details
       Notification is given when HTTP response is received.
       @endcond
    */
    virtual void dispatchDidReceiveResponse(WKC::DocumentLoader*, unsigned long  identifier, const WKC::ResourceResponse&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DocumentLoader* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchDidReceiveContentLength(WKC::DocumentLoader*, unsigned long identifier, int lengthReceived) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DocumentLoader* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchDidFinishLoading(WKC::DocumentLoader*, unsigned long  identifier) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of error in getting inclusion
       @param loader Pointer to WKC::DocumentLoader
       @param identifier Request identifier
       @param error Reference to error object
       @return None
       @endcond
    */
    virtual void dispatchDidFailLoading(WKC::DocumentLoader*, unsigned long  identifier, const WKC::ResourceError&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DocumentLoader* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual bool dispatchDidLoadResourceFromMemoryCache(WKC::DocumentLoader*, const WKC::ResourceRequest&, const WKC::ResourceResponse&, int length) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchDidDispatchOnloadEvents() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchDidReceiveServerRedirectForProvisionalLoad() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchDidCancelClientRedirect() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::KURL& (TBD) implement description
       @param  double (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchWillPerformClientRedirect(const WKC::KURL&, double, double) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies completion of fragment jump within the same page
       @return None
       @endcond
    */
    virtual void dispatchDidChangeLocationWithinPage() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return None
       @endcond
    */
    virtual void dispatchDidNavigateWithinPage() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchDidPushStateWithinPage() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchDidReplaceStateWithinPage() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchDidPopStateWithinPage() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of discarding current document when getting page
       @return None
       @endcond
    */
    virtual void dispatchWillClose() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of getting favicon
       @return None
       @details
       @ref bbb-favicon
       @endcond
    */
    virtual void dispatchDidReceiveIcon() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Starts getting page
       @return None
       @endcond
    */
    virtual void dispatchDidStartProvisionalLoad() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of getting title
       @param title Reference to title string
       @return None
       @details
       Notification is given when <title> is specified in the obtained content.
       @endcond
    */
    virtual void dispatchDidReceiveTitle(const WKC::String&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies that getting page data starts
       @return None
       @details
       Notification is given when getting data is successfully started.@n
       If getting data cannot be started due to an error, notification of dispatchDidFailProvisionalLoad() will be given.
       @endcond
    */
    virtual void dispatchDidCommitLoad() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies that getting page data cannot be started
       @param error Reference to error object
       @return None
       @details
       Notification is given if an error occurs before notification of dispatchDidStartProvisionalLoad() is given.
       @endcond
    */
    virtual void dispatchDidFailProvisionalLoad(const WKC::ResourceError&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of error in getting page
       @param error Reference to error object
       @return None
       @details
       Notification is given if an error occurs before notification of dispatchDidFinishLoad() is given.
       @endcond
    */
    virtual void dispatchDidFailLoad(const WKC::ResourceError&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies that parsing page is completed
       @return None
       @details
       Notification is given when parsing page is completed.@n
       Notification is given before dispatchDidFinishLoad().
       @endcond
    */
    virtual void dispatchDidFinishDocumentLoad() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies that getting page is completed.
       @return None
       @details
       Notification is given when getting page is successfully completed.@n
       If getting page is not completed due to an error during processing, notification of dispatchDidFailLoad() will be given.
       @endcond
    */
    virtual void dispatchDidFinishLoad() = 0;
    virtual void dispatchDidLayout() = 0;
    enum {
        EDidFirstLayout                                    = 0x0001,
        EFirstVisuallyNonEmptyLayout                       = 0x0002,
        EDidHitRelevantRepaintedObjectsAreaThreshold       = 0x0004,
        EDidFirstFlushForHeaderLayer                       = 0x0008,
        EDidFirstLayoutAfterSuppressedIncrementalRendering = 0x0010,
        EDidFirstPaintAfterSuppressedIncrementalRendering  = 0x0020
    };
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of layouts state
       @return None
       @endcond
    */
    virtual void dispatchDidReachLayoutMilestone(int) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of generating page
       @retval WKC::Frame* Pointer to WKC::Frame
       @details
       Notification is given after dispatchDecidePolicyForNewWindowAction().@n
       Generates a new WKC::WKCWebView and returns a pointer to WKC::Frame as a return value.
       @endcond
    */
    virtual WKC::Frame* dispatchCreatePage() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of displaying page
       @return None
       @endcond
    */
    virtual void dispatchShow() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Checks how to process new window generation
       @param function Function pointer to WKC::FramePolicyFunction
       @param action Reference to WKC::NavigationAction
       @param request Reference to request object
       @param state Pointer to WKC::FormState
       @param frameName Reference to frame name
       @return None
       @details
       Notification is given at a transition event of <a> where the target attribute is specified.@n
       Set the following values in WKC::FramePolicyFunction() depending on the processing method:@n
       - WKC::PolicyUse Generates a window @n
       - WKC::PolicyIgnore Does not generate a window @n
       @attention
       Do not generate a window in this API.@n
       Generate a window using dispatchCreatePage() that is notified after exiting the function when WKC::PolicyUse is set.
       @endcond
    */
    virtual void dispatchDecidePolicyForNewWindowAction(WKC::FramePolicyFunction&, const WKC::NavigationAction&, const WKC::ResourceRequest&, WKC::FormState*, const WKC::String& frameName) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Checks how to process transition event
       @param function Function pointer to WKC::FramePolicyFunction
       @param action Reference to WKC::NavigationAction
       @param request Reference to request object
       @param state Pointer to WKC::FormState
       @return None
       @details
       Set the following values in WKC::FramePolicyFunction() depending on the processing method:@n
       - WKC::PolicyUse Performs page transition
       - WKC::PolicyIgnore Does not perform page transition
       @endcond
    */
    virtual void dispatchDecidePolicyForNavigationAction(WKC::FramePolicyFunction&, const WKC::NavigationAction&, const WKC::ResourceRequest&, const WKC::ResourceResponse& redirectResponse, WKC::FormState*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchDecidePolicyForResponse(WKC::FramePolicyFunction&, const WKC::ResourceResponse&, const WKC::ResourceRequest&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void cancelPolicyCheck() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchUnableToImplementPolicy(const WKC::ResourceError&) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchWillSendSubmitEvent(WKC::FormState*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of form submit request
       @param policy (TBD) implement description
       @param state (TBD) implement description
       @return None
       @details (TBD) implement description
       @endcond
    */
    virtual void dispatchWillSubmitForm(WKC::FramePolicyFunction&, WKC::FormState*) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DocumentLoader* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void revertToProvisionalState(WKC::DocumentLoader*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DocumentLoader* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void setMainDocumentError(WKC::DocumentLoader*, const WKC::ResourceError&) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param bool (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void setMainFrameDocumentReady(bool) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::ResourceRequest& (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void startDownload(const WKC::ResourceRequest&, const WKC::String& suggestedName) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DocumentLoader* (TBD) implement description
       @param WKC::ResourceRequest& (TBD) implement description
       @param WKC::ResourceResponse& (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void convertMainResourceLoadToDownload(WKC::DocumentLoader*, const WKC::ResourceRequest&, const WKC::ResourceResponse&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DocumentLoader* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void willChangeTitle(WKC::DocumentLoader*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DocumentLoader* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void didChangeTitle(WKC::DocumentLoader*) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DocumentLoader* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void committedLoad(WKC::DocumentLoader*, const char*, int) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DocumentLoader* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void finishedLoading(WKC::DocumentLoader*) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of visit history update
       @return None
       @endcond
    */
    virtual void updateGlobalHistory() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void updateGlobalHistoryRedirectLinks() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Checks whether to go to history item
       @param item History item to go to
       @retval !false Goes to history item
       @retval false Does not go to history item
       @endcond
    */
    virtual bool shouldGoToHistoryItem(WKC::HistoryItem*) const = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void updateGlobalHistoryItemForPage() = 0;

    // This frame has displayed inactive content (such as an image) from an
    // insecure source.  Inactive content cannot spread to other frames.
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void didDisplayInsecureContent() = 0;

    // The indicated security origin has run active content (such as a
    // script) from an insecure source.  Note that the insecure content can
    // spread to other frames in the same origin.
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void didRunInsecureContent(WKC::SecurityOrigin&, const WKC::KURL& uri) = 0;
    virtual void didDetectXSS(const WKC::KURL&, bool didBlockEntirePage) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of cancellation
       @param request Reference to request object
       @retval WKC::ResourceError Error object
       @details
       Notification is given when getting content is canceled.@n
       Notification is given by WKC::FrameLoaderClientIf::dispatchDidFailLoad() or WKC::FrameLoaderClientIf::dispatchDidFailLoading().
       It generates a WKC::ResourceError object and returns it as a return value.
       @endcond
    */
    virtual WKC::ResourceError cancelledError(const WKC::ResourceRequest&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of communication block
       @param request Reference to request object
       @retval WKC::ResourceError Error object
       @details
       Notification is given when communication starts with URL including prohibited ports.@n
       Notification is given by WKC::FrameLoaderClientIf::dispatchDidFailLoad() or WKC::FrameLoaderClientIf::dispatchDidFailLoading().
       It generates a WKC::ResourceError object and returns it as a return value.
       @endcond
    */
    virtual WKC::ResourceError blockedError(const WKC::ResourceRequest&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies of communication block
       @param request Reference to request object
       @retval WKC::ResourceError Error object
       @details
       Notification is given when main resource is blocked by content blocker.@n
       Notification is given by WKC::FrameLoaderClientIf::dispatchDidFailLoad() or WKC::FrameLoaderClientIf::dispatchDidFailLoading().
       It generates a WKC::ResourceError object and returns it as a return value.
       @endcond
    */
    virtual WKC::ResourceError blockedByContentBlockerError(const WKC::ResourceRequest&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies that content cannot be displayed
       @param request Reference to request object
       @retval WKC::ResourceError Error object
       @details
       This is called when false is returned by WKC::FrameLoaderClientIf::canHandleRequest().@n
       Notification is given by WKC::FrameLoaderClientIf::dispatchDidFailLoad() or WKC::FrameLoaderClientIf::dispatchDidFailLoading().
       It generates a WKC::ResourceError object and returns it as a return value.
       @endcond
    */
    virtual WKC::ResourceError cannotShowURLError(const WKC::ResourceRequest&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief 
       @param request Reference to request object
       @retval WKC::ResourceError Error object
       @details
       If WKC::PolicyIgnore is set in WKC::FrameLoaderClientIf::dispatchDecidePolicyForResponse(), this will be called when false is returned by WKC::FrameLoaderClientIf::canShowMIMEType(). @n
       Notification is given by WKC::FrameLoaderClientIf::dispatchDidFailLoad() or WKC::FrameLoaderClientIf::dispatchDidFailLoading().
       It generates a WKC::ResourceError object and returns it as a return value.
       @endcond
    */
    virtual WKC::ResourceError interruptForPolicyChangeError(const WKC::ResourceRequest&) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies that content cannot be displayed
       @param response Reference to request object
       @retval WKC::ResourceError Error object
       @details
       This is called when false is returned by WKC::FrameLoaderClientIf::canShowMIMEType().@n
       Notification is given by WKC::FrameLoaderClientIf::dispatchDidFailLoad() or WKC::FrameLoaderClientIf::dispatchDidFailLoading().
       It generates a WKC::ResourceError object and returns it as a return value.
       @endcond
    */
    virtual WKC::ResourceError cannotShowMIMETypeError(const WKC::ResourceResponse&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::ResourceResponse& (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual WKC::ResourceError fileDoesNotExistError(const WKC::ResourceResponse&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::ResourceResponse& (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual WKC::ResourceError pluginWillHandleLoadError(const WKC::ResourceResponse&) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Checks whether to display content for load failure
       @param (TBD) implement description
       @retval !false Allow
       @retval false Do not allow
       @endcond
    */
    virtual bool shouldFallBack(const WKC::ResourceError&) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Checks whether to send request
       @param request Reference to request object
       @retval !false Allow
       @retval false Do not allow
       @details
       When false is returned, notification of WKC::FrameLoaderClientIf::cannotShowURLError() is given.
       @endcond
    */
    virtual bool canHandleRequest(const WKC::ResourceRequest&) const = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Checks whether to display target MIME type content
       @param mime Reference to MIME-Type string
       @retval !false Allow
       @retval false Do not allow
       @details
       When false is returned, notification of WKC::FrameLoaderClientIf::cannotShowMIMETypeError() is given. @n
       For inclusion images, etc., notification is not given.
       @endcond
    */
    virtual bool canShowMIMEType(const WKC::String&) const = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual bool canShowMIMETypeAsHTML(const WKC::String&) const = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::String& (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual bool representationExistsForURLScheme(const WKC::String&) const = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::String& (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual WKC::String generatedMIMETypeForURLScheme(const WKC::String&) const = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void frameLoadCompleted() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::HistoryItem* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void saveViewStateToItem(WKC::HistoryItem&) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void restoreViewState() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void provisionalLoadStarted() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void didFinishLoad() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void prepareForDataSourceReplacement() = 0;

    /* createDocumentLoader() */
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param title (TBD) implement description
       @param kurl (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void setTitle(const WKC::String& title, const WKC::KURL&) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Requests user agent
       @param kurl Reference to string of URL to send request to
       @retval WKC::String User agent string
       @endcond
    */
    virtual WKC::String userAgent(const WKC::KURL&) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param frame (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void savePlatformDataToCachedFrame(WKC::CachedFrame*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param frame (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void transitionToCommittedFromCachedFrame(WKC::CachedFrame*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void transitionToCommittedForNewPage() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void didSaveToPageCache() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void didRestoreFromPageCache() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchDidBecomeFrameset(bool) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual bool canCachePage() const = 0;

    /* createFrame() */
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKCSize& (TBD) implement description
       @param  WKC::HTMLPlugInElement* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual WKC::Widget* createPlugin(const WKCSize&, WKC::HTMLPlugInElement*, const WKC::KURL&, const WKC::String**, const WKC::String**, const WKC::String&, bool) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param pluginWidget (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void redirectDataToPlugin(WKC::Widget* pluginWidget) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKCSize& (TBD) implement description
       @param  WKC::HTMLAppletElement* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual WKC::Widget* createJavaAppletWidget(const WKCSize&, WKC::HTMLAppletElement&, const WKC::KURL& baseURL, const WKC::String** paramNames, const WKC::String** paramValues) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param url (TBD) implement description
       @param mimeType (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual WKC::ObjectContentType objectContentType(const WKC::KURL&, const WKC::String&) = 0;
    /* overrideMediaType() */

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param WKC::DOMWrapperWorld* (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void dispatchDidClearWindowObjectInWorld(WKC::DOMWrapperWorld*) = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void registerForIconNotification() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual void didChangeScrollOffset() = 0;

    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief (TBD) implement description
       @param (TBD) implement description
       @return (TBD) implement description
       @endcond
    */
    virtual bool allowScript(bool enabledPerSettings) = 0;

    virtual bool allowWebGL(bool enabledPerSettings) = 0;
    virtual void didLoseWebGLContext(int ctx) = 0;

    virtual bool shouldForceUniversalAccessFromLocalURL(const KURL&) = 0;

    /* createNetworkingContext() */

    virtual void dispatchGlobalObjectAvailable(WKC::DOMWrapperWorld*) = 0;

    //
    // WKC extension
    //
    /**
       @brief Notifies of receiving data
       @param handle Pointer to WKC::ResourceHandle
       @param length Data length
       @return None
       @details
       Notification is given when data is received.@n
       Types of data include HTTP header, various raw resource data (HTML document, image, CSS, JS file), and Data:-specified data.@n
       In this notification processing, the reception of the corresponding data can be interrupted by calling WKC::ResourceLoader::cancel().
       @attention
       Extension of NetFront Browser NX by ACCESS CO., LTD.
    */
    virtual bool dispatchWillReceiveData(WKC::ResourceHandle* handle, int length) = 0;
    /**
       @brief Notifies of SSL state
       @param handle Pointer to WKC::ResourceHandle
       @param status SSL state
       @return false do nothing
       @return true reload request
       @details
       Notification is given when SSL state changes.@n
       If the certificate chain received within this function has a problem, prompt a user to check it.
       @attention
       Extension of NetFront Browser NX by ACCESS CO., LTD.
    */
    virtual bool notifySSLHandshakeStatus(WKC::ResourceHandle* handle, SSLHandshakeStatus status) = 0;
    /**
       @brief Selects SSL client certificate
       @param handle Pointer to WKC::ResourceHandle
       @param requester Client certificate requester information
       @param certs void pointer to client certificate list
       @param num Number of client certificates
       @retval -1 Reject
       @retval >=0 Index number of selected client certificate
       @details
       Notification is given when a client certificate is requested by a server and the registered candidate corresponds to it.@n
       Let the user choose which client certificate to use by this notification.
       @attention
       Extension of NetFront Browser NX by ACCESS CO., LTD.
    */
    virtual int  requestSSLClientCertSelect(WKC::ResourceHandle* handle, const char* requester, WKC::ClientCertificate* certs, int num) = 0;
    /**
       @brief Checks whether to send request.
       @param handle Pointer to void
       @param url Destination URL
       @param composition Whether destination URL is a root content of the root frame, a root content of the sub frame or other.
       @li Inclusion Content : WKC::EInclusionContent
       @li SubFrame Root Content : WKC::ESubFrameRootContent
       @li RootFrame Root Content : WKC::ERootFrameRootContent
       @param isSync Whether availability of sending is synchronously determined
       @retval 1 Allow sending
       @retval 0 Defer determination
       @retval -1 Do not allow sending
       @details
       Determines whether to send corresponding request before sending it.@n
       The determination must be made for synchronization where similar interface has WKC::FrameLoaderClientIf::canHandleRequest(), however, this interface can determine it asynchronously as long as isSync is false.@n
       When the determination is made asynchronously, notification must be given by specifying the handle and availability for WKC::WKCWebView::permitSendRequest().@n
       @n
       When isSync is true, immediately determine whether sending is possible and return other than 0.
       @attention
       Extension of NetFront Browser NX by ACCESS CO., LTD.
    */
    virtual int  dispatchWillPermitSendRequest(void* handle, const char* url, WKC::ContentComposition composition, bool isSync, const WKC::ResourceResponse& redirectResponse) = 0;
    /**
        @brief notify loading from HTTP cache.
        @param handle pointer to WKC::ResourceHandle.
        @param url content URL when request from network.
        @details
        this function is called when finished loading from HTTP cache.
        @attention
        Extension of NetFront Browser NX by ACCESS CO., LTD.
    */
    virtual void didRestoreFromHTTPCache(WKC::ResourceHandle* handle, const WKC::KURL& url) = 0;
#ifdef WKC_ENABLE_CUSTOMJS
    virtual bool dispatchWillCallCustomJS(WKCCustomJSAPIList* api, void** context) = 0;
#endif // WKC_ENABLE_CUSTOMJS
};

/*@}*/

} // namespace

#endif // WKCFrameLoaderClient_h

/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) Research In Motion Limited 2009. All rights reserved.
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
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

#include "config.h"
#include "SubframeLoader.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "ContentSecurityPolicy.h"
#include "DiagnosticLoggingClient.h"
#include "DiagnosticLoggingKeys.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "HTMLAppletElement.h"
#include "HTMLAudioElement.h"
#include "HTMLFrameElementBase.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "MIMETypeRegistry.h"
#include "MainFrame.h"
#include "Page.h"
#include "PluginData.h"
#include "PluginDocument.h"
#include "RenderEmbeddedObject.h"
#include "RenderView.h"
#include "ScriptController.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"
#include "Settings.h"

namespace WebCore {
    
using namespace HTMLNames;

SubframeLoader::SubframeLoader(Frame& frame)
    : m_containsPlugins(false)
    , m_frame(frame)
{
}

void SubframeLoader::clear()
{
    m_containsPlugins = false;
}

bool SubframeLoader::requestFrame(HTMLFrameOwnerElement& ownerElement, const String& urlString, const AtomicString& frameName, LockHistory lockHistory, LockBackForwardList lockBackForwardList)
{
    // Support for <frame src="javascript:string">
    URL scriptURL;
    URL url;
    if (protocolIsJavaScript(urlString)) {
        scriptURL = completeURL(urlString); // completeURL() encodes the URL.
        url = blankURL();
    } else
        url = completeURL(urlString);

    bool hasExistingFrame = ownerElement.contentFrame();
    Frame* frame = loadOrRedirectSubframe(ownerElement, url, frameName, lockHistory, lockBackForwardList);
    if (!frame)
        return false;

    // If we create a new subframe then an empty document is loaded into it synchronously and may
    // cause script execution (say, via a DOM load event handler) that can do anything, including
    // navigating the subframe. We only want to evaluate scriptURL if the frame has not been navigated.
    bool canExecuteScript = hasExistingFrame || (frame->loader().documentLoader() && frame->loader().documentLoader()->originalURL() == blankURL());
    if (!scriptURL.isEmpty() && canExecuteScript && ownerElement.isURLAllowed(scriptURL))
        frame->script().executeIfJavaScriptURL(scriptURL);

    return true;
}
    
bool SubframeLoader::resourceWillUsePlugin(const String& url, const String& mimeType, bool shouldPreferPlugInsForImages)
{
    URL completedURL;
    if (!url.isEmpty())
        completedURL = completeURL(url);

    bool useFallback;
    return shouldUsePlugin(completedURL, mimeType, shouldPreferPlugInsForImages, false, useFallback);
}

bool SubframeLoader::pluginIsLoadable(HTMLPlugInImageElement& pluginElement, const URL& url, const String& mimeType)
{
    if (MIMETypeRegistry::isJavaAppletMIMEType(mimeType)) {
        if (!m_frame.settings().isJavaEnabled())
            return false;
        if (document() && document()->securityOrigin().isLocal() && !m_frame.settings().isJavaEnabledForLocalFiles())
            return false;
    }

    if (document()) {
        if (document()->isSandboxed(SandboxPlugins))
            return false;

        if (!document()->securityOrigin().canDisplay(url)) {
            FrameLoader::reportLocalLoadFailed(&m_frame, url.string());
            return false;
        }

        String declaredMimeType = document()->isPluginDocument() && document()->ownerElement() ?
            document()->ownerElement()->fastGetAttribute(HTMLNames::typeAttr) :
            pluginElement.fastGetAttribute(HTMLNames::typeAttr);
        bool isInUserAgentShadowTree = pluginElement.isInUserAgentShadowTree();
        if (!document()->contentSecurityPolicy()->allowObjectFromSource(url, isInUserAgentShadowTree)
            || !document()->contentSecurityPolicy()->allowPluginType(mimeType, declaredMimeType, url, isInUserAgentShadowTree)) {
            RenderEmbeddedObject* renderer = pluginElement.renderEmbeddedObject();
            renderer->setPluginUnavailabilityReason(RenderEmbeddedObject::PluginBlockedByContentSecurityPolicy);
            return false;
        }

        if (!m_frame.loader().mixedContentChecker().canRunInsecureContent(document()->securityOrigin(), url))
            return false;
    }

    return true;
}

bool SubframeLoader::requestPlugin(HTMLPlugInImageElement& ownerElement, const URL& url, const String& mimeType, const Vector<String>& paramNames, const Vector<String>& paramValues, bool useFallback)
{
    // Application plug-ins are plug-ins implemented by the user agent, for example Qt plug-ins,
    // as opposed to third-party code such as Flash. The user agent decides whether or not they are
    // permitted, rather than WebKit.
    if ((!allowPlugins(AboutToInstantiatePlugin) && !MIMETypeRegistry::isApplicationPluginMIMEType(mimeType)))
        return false;

    if (!pluginIsLoadable(ownerElement, url, mimeType))
        return false;

    ASSERT(ownerElement.hasTagName(objectTag) || ownerElement.hasTagName(embedTag));
    return loadPlugin(ownerElement, url, mimeType, paramNames, paramValues, useFallback);
}

static String findPluginMIMETypeFromURL(Page* page, const String& url)
{
    if (!url)
        return String();

    size_t dotIndex = url.reverseFind('.');
    if (dotIndex == notFound)
        return String();

    String extension = url.substring(dotIndex + 1);

    const PluginData& pluginData = page->pluginData();

    Vector<MimeClassInfo> mimes;
    Vector<size_t> mimePluginIndices;
    pluginData.getWebVisibleMimesAndPluginIndices(mimes, mimePluginIndices);
    for (size_t i = 0; i < mimes.size(); ++i) {
        const MimeClassInfo& mimeClassInfo = mimes[i];
        for (size_t j = 0; j < mimeClassInfo.extensions.size(); ++j) {
            if (equalIgnoringCase(extension, mimeClassInfo.extensions[j]))
                return mimeClassInfo.type;
        }
    }

    return String();
}

static void logPluginRequest(Page* page, const String& mimeType, const String& url, bool success)
{
    if (!page)
        return;

    String newMIMEType = mimeType;
    if (!newMIMEType) {
        // Try to figure out the MIME type from the URL extension.
        newMIMEType = findPluginMIMETypeFromURL(page, url);
        if (!newMIMEType)
            return;
    }

    String pluginFile = page->pluginData().pluginFileForWebVisibleMimeType(newMIMEType);
    String description = !pluginFile ? newMIMEType : pluginFile;

    DiagnosticLoggingClient& diagnosticLoggingClient = page->mainFrame().diagnosticLoggingClient();
    diagnosticLoggingClient.logDiagnosticMessage(success ? DiagnosticLoggingKeys::pluginLoadedKey() : DiagnosticLoggingKeys::pluginLoadingFailedKey(), description, ShouldSample::No);

    if (!page->hasSeenAnyPlugin())
        diagnosticLoggingClient.logDiagnosticMessage(DiagnosticLoggingKeys::pageContainsAtLeastOnePluginKey(), emptyString(), ShouldSample::No);

    if (!page->hasSeenPlugin(description))
        diagnosticLoggingClient.logDiagnosticMessage(DiagnosticLoggingKeys::pageContainsPluginKey(), description, ShouldSample::No);

    page->sawPlugin(description);
}

bool SubframeLoader::requestObject(HTMLPlugInImageElement& ownerElement, const String& url, const AtomicString& frameName, const String& mimeType, const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    if (url.isEmpty() && mimeType.isEmpty())
        return false;

    URL completedURL;
    if (!url.isEmpty())
        completedURL = completeURL(url);

    bool hasFallbackContent = is<HTMLObjectElement>(ownerElement) && downcast<HTMLObjectElement>(ownerElement).hasFallbackContent();

    bool useFallback;
    if (shouldUsePlugin(completedURL, mimeType, ownerElement.shouldPreferPlugInsForImages(), hasFallbackContent, useFallback)) {
        bool success = requestPlugin(ownerElement, completedURL, mimeType, paramNames, paramValues, useFallback);
        logPluginRequest(document()->page(), mimeType, completedURL, success);
        return success;
    }

    // If the plug-in element already contains a subframe, loadOrRedirectSubframe will re-use it. Otherwise,
    // it will create a new frame and set it as the RenderWidget's Widget, causing what was previously 
    // in the widget to be torn down.
    return loadOrRedirectSubframe(ownerElement, completedURL, frameName, LockHistory::Yes, LockBackForwardList::Yes);
}

PassRefPtr<Widget> SubframeLoader::createJavaAppletWidget(const IntSize& size, HTMLAppletElement& element, const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    String baseURLString;
    String codeBaseURLString;

    for (size_t i = 0; i < paramNames.size(); ++i) {
        if (equalIgnoringCase(paramNames[i], "baseurl"))
            baseURLString = paramValues[i];
        else if (equalIgnoringCase(paramNames[i], "codebase"))
            codeBaseURLString = paramValues[i];
    }

    if (!codeBaseURLString.isEmpty()) {
        URL codeBaseURL = completeURL(codeBaseURLString);
        if (!element.document().securityOrigin().canDisplay(codeBaseURL)) {
            FrameLoader::reportLocalLoadFailed(&m_frame, codeBaseURL.string());
            return nullptr;
        }

        const char javaAppletMimeType[] = "application/x-java-applet";
        bool isInUserAgentShadowTree = element.isInUserAgentShadowTree();
        if (!element.document().contentSecurityPolicy()->allowObjectFromSource(codeBaseURL, isInUserAgentShadowTree)
            || !element.document().contentSecurityPolicy()->allowPluginType(javaAppletMimeType, javaAppletMimeType, codeBaseURL, isInUserAgentShadowTree))
            return nullptr;
    }

    if (baseURLString.isEmpty())
        baseURLString = m_frame.document()->baseURL().string();
    URL baseURL = completeURL(baseURLString);

    RefPtr<Widget> widget;
    if (allowPlugins(AboutToInstantiatePlugin))
        widget = m_frame.loader().client().createJavaAppletWidget(size, &element, baseURL, paramNames, paramValues);

    logPluginRequest(document()->page(), element.serviceType(), String(), widget);

    if (!widget) {
        RenderEmbeddedObject* renderer = element.renderEmbeddedObject();

        if (!renderer->isPluginUnavailable())
            renderer->setPluginUnavailabilityReason(RenderEmbeddedObject::PluginMissing);
        return nullptr;
    }

    m_containsPlugins = true;
    return widget;
}

Frame* SubframeLoader::loadOrRedirectSubframe(HTMLFrameOwnerElement& ownerElement, const URL& url, const AtomicString& frameName, LockHistory lockHistory, LockBackForwardList lockBackForwardList)
{
    Frame* frame = ownerElement.contentFrame();
    if (frame)
        frame->navigationScheduler().scheduleLocationChange(m_frame.document(), m_frame.document()->securityOrigin(), url, m_frame.loader().outgoingReferrer(), lockHistory, lockBackForwardList);
    else
        frame = loadSubframe(ownerElement, url, frameName, m_frame.loader().outgoingReferrer());

    if (!frame)
        return nullptr;

    ASSERT(ownerElement.contentFrame() == frame || !ownerElement.contentFrame());
    return ownerElement.contentFrame();
}

Frame* SubframeLoader::loadSubframe(HTMLFrameOwnerElement& ownerElement, const URL& url, const String& name, const String& referrer)
{
    Ref<Frame> protect(m_frame);

    bool allowsScrolling = true;
    int marginWidth = -1;
    int marginHeight = -1;
    if (is<HTMLFrameElementBase>(ownerElement)) {
        HTMLFrameElementBase& frameElementBase = downcast<HTMLFrameElementBase>(ownerElement);
        allowsScrolling = frameElementBase.scrollingMode() != ScrollbarAlwaysOff;
        marginWidth = frameElementBase.marginWidth();
        marginHeight = frameElementBase.marginHeight();
    }

    if (!ownerElement.document().securityOrigin().canDisplay(url)) {
        FrameLoader::reportLocalLoadFailed(&m_frame, url.string());
        return nullptr;
    }

    if (!SubframeLoadingDisabler::canLoadFrame(ownerElement))
        return nullptr;

    String referrerToUse = SecurityPolicy::generateReferrerHeader(ownerElement.document().referrerPolicy(), url, referrer);
    RefPtr<Frame> frame = m_frame.loader().client().createFrame(url, name, &ownerElement, referrerToUse, allowsScrolling, marginWidth, marginHeight);

    if (!frame)  {
        m_frame.loader().checkCallImplicitClose();
        return nullptr;
    }
    
    // All new frames will have m_isComplete set to true at this point due to synchronously loading
    // an empty document in FrameLoader::init(). But many frames will now be starting an
    // asynchronous load of url, so we set m_isComplete to false and then check if the load is
    // actually completed below. (Note that we set m_isComplete to false even for synchronous
    // loads, so that checkCompleted() below won't bail early.)
    // FIXME: Can we remove this entirely? m_isComplete normally gets set to false when a load is committed.
    frame->loader().started();
   
    auto* renderer = ownerElement.renderer();
    FrameView* view = frame->view();
    if (is<RenderWidget>(renderer) && view)
        downcast<RenderWidget>(*renderer).setWidget(view);
    
    m_frame.loader().checkCallImplicitClose();
    
    // Some loads are performed synchronously (e.g., about:blank and loads
    // cancelled by returning a null ResourceRequest from requestFromDelegate).
    // In these cases, the synchronous load would have finished
    // before we could connect the signals, so make sure to send the 
    // completed() signal for the child by hand and mark the load as being
    // complete.
    // FIXME: In this case the Frame will have finished loading before 
    // it's being added to the child list. It would be a good idea to
    // create the child first, then invoke the loader separately.
    if (frame->loader().state() == FrameStateComplete && !frame->loader().policyDocumentLoader())
        frame->loader().checkCompleted();

    return frame.get();
}

bool SubframeLoader::allowPlugins(ReasonForCallingAllowPlugins)
{
    return m_frame.settings().arePluginsEnabled();
}

bool SubframeLoader::shouldUsePlugin(const URL& url, const String& mimeType, bool shouldPreferPlugInsForImages, bool hasFallback, bool& useFallback)
{
    if (m_frame.loader().client().shouldAlwaysUsePluginDocument(mimeType)) {
        useFallback = false;
        return true;
    }

    // Allow other plug-ins to win over QuickTime because if the user has installed a plug-in that
    // can handle TIFF (which QuickTime can also handle) they probably intended to override QT.
    if (m_frame.page() && (mimeType == "image/tiff" || mimeType == "image/tif" || mimeType == "image/x-tiff")) {
        String pluginName = m_frame.page()->pluginData().pluginNameForWebVisibleMimeType(mimeType);
        if (!pluginName.isEmpty() && !pluginName.contains("QuickTime", false)) 
            return true;
    }
        
    ObjectContentType objectType = m_frame.loader().client().objectContentType(url, mimeType, shouldPreferPlugInsForImages);
    // If an object's content can't be handled and it has no fallback, let
    // it be handled as a plugin to show the broken plugin icon.
    useFallback = objectType == ObjectContentNone && hasFallback;
    return objectType == ObjectContentNone || objectType == ObjectContentNetscapePlugin || objectType == ObjectContentOtherPlugin;
}

Document* SubframeLoader::document() const
{
    return m_frame.document();
}

bool SubframeLoader::loadPlugin(HTMLPlugInImageElement& pluginElement, const URL& url, const String& mimeType, const Vector<String>& paramNames, const Vector<String>& paramValues, bool useFallback)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (useFallback)
        return false;

    RenderEmbeddedObject* renderer = pluginElement.renderEmbeddedObject();
    // FIXME: This code should not depend on renderer!
    if (!renderer)
        return false;

    pluginElement.subframeLoaderWillCreatePlugIn(url);

    IntSize contentSize = roundedIntSize(LayoutSize(renderer->contentWidth(), renderer->contentHeight()));
    bool loadManually = is<PluginDocument>(*document()) && !m_containsPlugins && downcast<PluginDocument>(*document()).shouldLoadPluginManually();

#if PLATFORM(IOS)
    // On iOS, we only tell the plugin to be in full page mode if the containing plugin document is the top level document.
    if (document()->ownerElement())
        loadManually = false;
#endif

    WeakPtr<RenderWidget> weakRenderer = renderer->createWeakPtr();
    // createPlugin *may* cause this renderer to disappear from underneath.
    RefPtr<Widget> widget = m_frame.loader().client().createPlugin(contentSize, &pluginElement, url, paramNames, paramValues, mimeType, loadManually);
    if (!weakRenderer)
        return false;

    if (!widget) {
        if (!renderer->isPluginUnavailable())
            renderer->setPluginUnavailabilityReason(RenderEmbeddedObject::PluginMissing);
        return false;
    }

    pluginElement.subframeLoaderDidCreatePlugIn(*widget);
    renderer->setWidget(widget);
    m_containsPlugins = true;
    return true;
#else
    return false;
#endif
}

URL SubframeLoader::completeURL(const String& url) const
{
    ASSERT(m_frame.document());
    return m_frame.document()->completeURL(url);
}

} // namespace WebCore

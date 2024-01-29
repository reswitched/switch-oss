/*
 * Copyright (C) 2008, 2011, 2012, 2014 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "HTMLPlugInImageElement.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Event.h"
#include "EventHandler.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameView.h"
#include "HTMLImageLoader.h"
#include "JSDocumentFragment.h"
#include "LocalizedStrings.h"
#include "Logging.h"
#include "MainFrame.h"
#include "MouseEvent.h"
#include "NodeList.h"
#include "NodeRenderStyle.h"
#include "Page.h"
#include "PlugInClient.h"
#include "PluginViewBase.h"
#include "RenderEmbeddedObject.h"
#include "RenderImage.h"
#include "RenderSnapshottedPlugIn.h"
#include "SchemeRegistry.h"
#include "ScriptController.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "StyleResolver.h"
#include "SubframeLoader.h"
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/JSBase.h>
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

using namespace HTMLNames;

typedef Vector<RefPtr<HTMLPlugInImageElement>> HTMLPlugInImageElementList;
typedef HashMap<String, String> MimeTypeToLocalizedStringMap;

static const int sizingTinyDimensionThreshold = 40;
static const float sizingFullPageAreaRatioThreshold = 0.96;
static const float autostartSoonAfterUserGestureThreshold = 5.0;

// This delay should not exceed the snapshot delay in PluginView.cpp
static const auto simulatedMouseClickTimerDelay = std::chrono::milliseconds { 750 };
static const auto removeSnapshotTimerDelay = std::chrono::milliseconds { 1500 };

static const String titleText(Page* page, String mimeType)
{
    DEPRECATED_DEFINE_STATIC_LOCAL(MimeTypeToLocalizedStringMap, mimeTypeToLabelTitleMap, ());
    String titleText = mimeTypeToLabelTitleMap.get(mimeType);
    if (!titleText.isEmpty())
        return titleText;

    titleText = page->chrome().client().plugInStartLabelTitle(mimeType);
    if (titleText.isEmpty())
        titleText = snapshottedPlugInLabelTitle();
    mimeTypeToLabelTitleMap.set(mimeType, titleText);
    return titleText;
};

static const String subtitleText(Page* page, String mimeType)
{
    DEPRECATED_DEFINE_STATIC_LOCAL(MimeTypeToLocalizedStringMap, mimeTypeToLabelSubtitleMap, ());
    String subtitleText = mimeTypeToLabelSubtitleMap.get(mimeType);
    if (!subtitleText.isEmpty())
        return subtitleText;

    subtitleText = page->chrome().client().plugInStartLabelSubtitle(mimeType);
    if (subtitleText.isEmpty())
        subtitleText = snapshottedPlugInLabelSubtitle();
    mimeTypeToLabelSubtitleMap.set(mimeType, subtitleText);
    return subtitleText;
};

HTMLPlugInImageElement::HTMLPlugInImageElement(const QualifiedName& tagName, Document& document, bool createdByParser, PreferPlugInsForImagesOption preferPlugInsForImagesOption)
    : HTMLPlugInElement(tagName, document)
    // m_needsWidgetUpdate(!createdByParser) allows HTMLObjectElement to delay
    // widget updates until after all children are parsed.  For HTMLEmbedElement
    // this delay is unnecessary, but it is simpler to make both classes share
    // the same codepath in this class.
    , m_needsWidgetUpdate(!createdByParser)
    , m_shouldPreferPlugInsForImages(preferPlugInsForImagesOption == ShouldPreferPlugInsForImages)
    , m_needsDocumentActivationCallbacks(false)
    , m_simulatedMouseClickTimer(*this, &HTMLPlugInImageElement::simulatedMouseClickTimerFired, simulatedMouseClickTimerDelay)
    , m_removeSnapshotTimer(*this, &HTMLPlugInImageElement::removeSnapshotTimerFired)
    , m_createdDuringUserGesture(ScriptController::processingUserGesture())
    , m_isRestartedPlugin(false)
    , m_needsCheckForSizeChange(false)
    , m_plugInWasCreated(false)
    , m_deferredPromotionToPrimaryPlugIn(false)
    , m_snapshotDecision(SnapshotNotYetDecided)
    , m_plugInDimensionsSpecified(false)
{
    setHasCustomStyleResolveCallbacks();
}

HTMLPlugInImageElement::~HTMLPlugInImageElement()
{
    if (m_needsDocumentActivationCallbacks)
        document().unregisterForDocumentSuspensionCallbacks(this);
}

void HTMLPlugInImageElement::setDisplayState(DisplayState state)
{
#if PLATFORM(COCOA)
    if (state == RestartingWithPendingMouseClick || state == Restarting) {
        m_isRestartedPlugin = true;
        m_snapshotDecision = NeverSnapshot;
        setNeedsStyleRecalc(SyntheticStyleChange);
        if (displayState() == DisplayingSnapshot)
            m_removeSnapshotTimer.startOneShot(removeSnapshotTimerDelay);
    }
#endif

    HTMLPlugInElement::setDisplayState(state);
}

RenderEmbeddedObject* HTMLPlugInImageElement::renderEmbeddedObject() const
{
    // HTMLObjectElement and HTMLEmbedElement may return arbitrary renderers
    // when using fallback content.
    return is<RenderEmbeddedObject>(renderer()) ? downcast<RenderEmbeddedObject>(renderer()) : nullptr;
}

bool HTMLPlugInImageElement::isImageType()
{
    if (m_serviceType.isEmpty() && protocolIs(m_url, "data"))
        m_serviceType = mimeTypeFromDataURL(m_url);

    if (Frame* frame = document().frame()) {
        URL completedURL = document().completeURL(m_url);
        return frame->loader().client().objectContentType(completedURL, m_serviceType, shouldPreferPlugInsForImages()) == ObjectContentImage;
    }

    return Image::supportsType(m_serviceType);
}

// We don't use m_url, as it may not be the final URL that the object loads,
// depending on <param> values.
bool HTMLPlugInImageElement::allowedToLoadFrameURL(const String& url)
{
    URL completeURL = document().completeURL(url);

    if (contentFrame() && protocolIsJavaScript(completeURL)
        && !document().securityOrigin().canAccess(contentDocument()->securityOrigin()))
        return false;

    return document().frame()->isURLAllowed(completeURL);
}

// We don't use m_url, or m_serviceType as they may not be the final values
// that <object> uses depending on <param> values.
bool HTMLPlugInImageElement::wouldLoadAsNetscapePlugin(const String& url, const String& serviceType)
{
    ASSERT(document().frame());
    URL completedURL;
    if (!url.isEmpty())
        completedURL = document().completeURL(url);

    FrameLoader& frameLoader = document().frame()->loader();
    if (frameLoader.client().objectContentType(completedURL, serviceType, shouldPreferPlugInsForImages()) == ObjectContentNetscapePlugin)
        return true;
    return false;
}

RenderPtr<RenderElement> HTMLPlugInImageElement::createElementRenderer(Ref<RenderStyle>&& style, const RenderTreePosition& insertionPosition)
{
#if !PLATFORM(WKC)
    ASSERT(!document().inPageCache());
#endif

    if (displayState() >= PreparingPluginReplacement)
        return HTMLPlugInElement::createElementRenderer(WTF::move(style), insertionPosition);

    // Once a PlugIn Element creates its renderer, it needs to be told when the Document goes
    // inactive or reactivates so it can clear the renderer before going into the page cache.
    if (!m_needsDocumentActivationCallbacks) {
        m_needsDocumentActivationCallbacks = true;
        document().registerForDocumentSuspensionCallbacks(this);
    }

    if (displayState() == DisplayingSnapshot) {
        auto renderSnapshottedPlugIn = createRenderer<RenderSnapshottedPlugIn>(*this, WTF::move(style));
        renderSnapshottedPlugIn->updateSnapshot(m_snapshotImage);
        return WTF::move(renderSnapshottedPlugIn);
    }

    // Fallback content breaks the DOM->Renderer class relationship of this
    // class and all superclasses because createObject won't necessarily
    // return a RenderEmbeddedObject or RenderWidget.
    if (useFallbackContent())
        return RenderElement::createFor(*this, WTF::move(style));

    if (isImageType())
        return createRenderer<RenderImage>(*this, WTF::move(style));

    return HTMLPlugInElement::createElementRenderer(WTF::move(style), insertionPosition);
}

bool HTMLPlugInImageElement::childShouldCreateRenderer(const Node& child) const
{
    if (is<RenderSnapshottedPlugIn>(renderer()) && !hasShadowRootParent(child))
        return false;

    return HTMLPlugInElement::childShouldCreateRenderer(child);
}

bool HTMLPlugInImageElement::willRecalcStyle(Style::Change change)
{
    // Make sure style recalcs scheduled by a child shadow tree don't trigger reconstruction and cause flicker.
    if (change == Style::NoChange && styleChangeType() == NoStyleChange)
        return true;

    // FIXME: There shoudn't be need to force render tree reconstruction here.
    // It is only done because loading and load event dispatching is tied to render tree construction.
    if (!useFallbackContent() && needsWidgetUpdate() && renderer() && !isImageType() && (displayState() != DisplayingSnapshot))
        setNeedsStyleRecalc(ReconstructRenderTree);
    return true;
}

void HTMLPlugInImageElement::didAttachRenderers()
{
    if (!isImageType()) {
        RefPtr<HTMLPlugInImageElement> element = this;
#if !PLATFORM(WKC)
        Style::queuePostResolutionCallback([element]{
            element->updateWidgetIfNecessary();
        });
#else
        std::function<void()> p(std::allocator_arg, WTF::voidFuncAllocator(), [element] {
            element->updateWidgetIfNecessary();
        });
        Style::queuePostResolutionCallback(WTF::move(p));
#endif
        return;
    }
    if (!renderer() || useFallbackContent())
        return;

    // Image load might complete synchronously and cause us to re-enter.
    RefPtr<HTMLPlugInImageElement> element = this;
#if !PLATFORM(WKC)
    Style::queuePostResolutionCallback([element]{
        element->startLoadingImage();
    });
#else
    std::function<void()> p(std::allocator_arg, WTF::voidFuncAllocator(), [element] {
        element->startLoadingImage();
    });
    Style::queuePostResolutionCallback(WTF::move(p));
#endif
}

void HTMLPlugInImageElement::willDetachRenderers()
{
    // FIXME: Because of the insanity that is HTMLPlugInImageElement::willRecalcStyle,
    // we can end up detaching during an attach() call, before we even have a
    // renderer. In that case, don't mark the widget for update.
    if (renderer() && !useFallbackContent()) {
        // Update the widget the next time we attach (detaching destroys the plugin).
        setNeedsWidgetUpdate(true);
    }

    HTMLPlugInElement::willDetachRenderers();
}

void HTMLPlugInImageElement::updateWidgetIfNecessary()
{
    document().updateStyleIfNeeded();

    if (!needsWidgetUpdate() || useFallbackContent() || isImageType())
        return;

    if (!renderEmbeddedObject() || renderEmbeddedObject()->isPluginUnavailable())
        return;

    updateWidget(CreateOnlyNonNetscapePlugins);
}

void HTMLPlugInImageElement::finishParsingChildren()
{
    HTMLPlugInElement::finishParsingChildren();
    if (useFallbackContent())
        return;

    setNeedsWidgetUpdate(true);
    if (inDocument())
        setNeedsStyleRecalc();
}

void HTMLPlugInImageElement::didMoveToNewDocument(Document* oldDocument)
{
    if (m_needsDocumentActivationCallbacks) {
        oldDocument->unregisterForDocumentSuspensionCallbacks(this);
        document().registerForDocumentSuspensionCallbacks(this);
    }

    if (m_imageLoader)
        m_imageLoader->elementDidMoveToNewDocument();

    HTMLPlugInElement::didMoveToNewDocument(oldDocument);
}

void HTMLPlugInImageElement::prepareForDocumentSuspension()
{
    if (renderer())
        Style::detachRenderTree(*this);

    HTMLPlugInElement::prepareForDocumentSuspension();
}

void HTMLPlugInImageElement::resumeFromDocumentSuspension()
{
    setNeedsStyleRecalc(ReconstructRenderTree);

    HTMLPlugInElement::resumeFromDocumentSuspension();
}

void HTMLPlugInImageElement::startLoadingImage()
{
    if (!m_imageLoader)
        m_imageLoader = std::make_unique<HTMLImageLoader>(*this);
    m_imageLoader->updateFromElement();
}

void HTMLPlugInImageElement::updateSnapshot(PassRefPtr<Image> image)
{
    if (displayState() > DisplayingSnapshot)
        return;

    m_snapshotImage = image;

    if (!renderer())
        return;
    auto& renderer = *this->renderer();

    if (is<RenderSnapshottedPlugIn>(renderer)) {
        downcast<RenderSnapshottedPlugIn>(renderer).updateSnapshot(image);
        return;
    }

    if (is<RenderEmbeddedObject>(renderer))
        renderer.repaint();
}

static DOMWrapperWorld& plugInImageElementIsolatedWorld()
{
#if !PLATFORM(WKC)
    static DOMWrapperWorld& isolatedWorld = DOMWrapperWorld::create(JSDOMWindow::commonVM()).leakRef();
    return isolatedWorld;
#else
    WKC_DEFINE_STATIC_PTR(DOMWrapperWorld*, isolatedWorld, &DOMWrapperWorld::create(JSDOMWindow::commonVM()).leakRef());
    return *isolatedWorld;
#endif
}

void HTMLPlugInImageElement::didAddUserAgentShadowRoot(ShadowRoot* root)
{
    HTMLPlugInElement::didAddUserAgentShadowRoot(root);
    if (displayState() >= PreparingPluginReplacement)
        return;

    Page* page = document().page();
    if (!page)
        return;

    // Reset any author styles that may apply as we only want explicit
    // styles defined in the injected user agents stylesheets to specify
    // the look-and-feel of the snapshotted plug-in overlay. 
    root->setResetStyleInheritance(true);
    
    String mimeType = loadedMimeType();

    DOMWrapperWorld& isolatedWorld = plugInImageElementIsolatedWorld();
    document().ensurePlugInsInjectedScript(isolatedWorld);

    ScriptController& scriptController = document().frame()->script();
    JSDOMGlobalObject* globalObject = JSC::jsCast<JSDOMGlobalObject*>(scriptController.globalObject(isolatedWorld));
    JSC::ExecState* exec = globalObject->globalExec();

    JSC::JSLockHolder lock(exec);

    JSC::MarkedArgumentBuffer argList;
    argList.append(toJS(exec, globalObject, root));
    argList.append(jsString(exec, titleText(page, mimeType)));
    argList.append(jsString(exec, subtitleText(page, mimeType)));
    
    // This parameter determines whether or not the snapshot overlay should always be visible over the plugin snapshot.
    // If no snapshot was found then we want the overlay to be visible.
    argList.append(JSC::jsBoolean(!m_snapshotImage));

    // It is expected the JS file provides a createOverlay(shadowRoot, title, subtitle) function.
    JSC::JSObject* overlay = globalObject->get(exec, JSC::Identifier::fromString(exec, "createOverlay")).toObject(exec);
    JSC::CallData callData;
    JSC::CallType callType = overlay->methodTable()->getCallData(overlay, callData);
    if (callType == JSC::CallTypeNone)
        return;

    JSC::call(exec, overlay, callType, callData, globalObject, argList);
    exec->clearException();
}

bool HTMLPlugInImageElement::partOfSnapshotOverlay(const Node* node) const
{
    DEPRECATED_DEFINE_STATIC_LOCAL(AtomicString, selector, (".snapshot-overlay", AtomicString::ConstructFromLiteral));
    ShadowRoot* shadow = userAgentShadowRoot();
    if (!shadow)
        return false;
    RefPtr<Element> snapshotLabel = shadow->querySelector(selector, ASSERT_NO_EXCEPTION);
    return node && snapshotLabel && (node == snapshotLabel.get() || node->isDescendantOf(snapshotLabel.get()));
}

void HTMLPlugInImageElement::removeSnapshotTimerFired()
{
    m_snapshotImage = nullptr;
    m_isRestartedPlugin = false;
    setNeedsStyleRecalc(SyntheticStyleChange);
    if (renderer())
        renderer()->repaint();
}

static void addPlugInsFromNodeListMatchingPlugInOrigin(HTMLPlugInImageElementList& plugInList, PassRefPtr<NodeList> collection, const String& plugInOrigin, const String& mimeType)
{
    for (unsigned i = 0, length = collection->length(); i < length; i++) {
        Node* node = collection->item(i);
        if (is<HTMLPlugInImageElement>(*node)) {
            HTMLPlugInImageElement& plugInImageElement = downcast<HTMLPlugInImageElement>(*node);
            const URL& loadedURL = plugInImageElement.loadedUrl();
            String otherMimeType = plugInImageElement.loadedMimeType();
            if (plugInOrigin == loadedURL.host() && mimeType == otherMimeType)
                plugInList.append(&plugInImageElement);
        }
    }
}

void HTMLPlugInImageElement::restartSimilarPlugIns()
{
    // Restart any other snapshotted plugins in the page with the same origin. Note that they
    // may be in different frames, so traverse from the top of the document.

    String plugInOrigin = m_loadedUrl.host();
    String mimeType = loadedMimeType();
    HTMLPlugInImageElementList similarPlugins;

    if (!document().page())
        return;

    for (Frame* frame = &document().page()->mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->loader().subframeLoader().containsPlugins())
            continue;
        
        if (!frame->document())
            continue;

        RefPtr<NodeList> plugIns = frame->document()->getElementsByTagName(embedTag.localName());
        if (plugIns)
            addPlugInsFromNodeListMatchingPlugInOrigin(similarPlugins, plugIns, plugInOrigin, mimeType);

        plugIns = frame->document()->getElementsByTagName(objectTag.localName());
        if (plugIns)
            addPlugInsFromNodeListMatchingPlugInOrigin(similarPlugins, plugIns, plugInOrigin, mimeType);
    }

    for (size_t i = 0, length = similarPlugins.size(); i < length; ++i) {
        HTMLPlugInImageElement* plugInToRestart = similarPlugins[i].get();
        if (plugInToRestart->displayState() <= HTMLPlugInElement::DisplayingSnapshot) {
            LOG(Plugins, "%p Plug-in looks similar to a restarted plug-in. Restart.", plugInToRestart);
            plugInToRestart->restartSnapshottedPlugIn();
        }
        plugInToRestart->m_snapshotDecision = NeverSnapshot;
    }
}

void HTMLPlugInImageElement::userDidClickSnapshot(PassRefPtr<MouseEvent> event, bool forwardEvent)
{
    if (forwardEvent)
        m_pendingClickEventFromSnapshot = event;

    String plugInOrigin = m_loadedUrl.host();
    if (document().page() && !SchemeRegistry::shouldTreatURLSchemeAsLocal(document().page()->mainFrame().document()->baseURL().protocol()) && document().page()->settings().autostartOriginPlugInSnapshottingEnabled())
        document().page()->plugInClient()->didStartFromOrigin(document().page()->mainFrame().document()->baseURL().host(), plugInOrigin, loadedMimeType(), document().page()->sessionID());

    LOG(Plugins, "%p User clicked on snapshotted plug-in. Restart.", this);
    restartSnapshottedPlugIn();
    if (forwardEvent)
        setDisplayState(RestartingWithPendingMouseClick);
    restartSimilarPlugIns();
}

void HTMLPlugInImageElement::setIsPrimarySnapshottedPlugIn(bool isPrimarySnapshottedPlugIn)
{
    if (!document().page() || !document().page()->settings().primaryPlugInSnapshotDetectionEnabled() || document().page()->settings().snapshotAllPlugIns())
        return;

    if (isPrimarySnapshottedPlugIn) {
        if (m_plugInWasCreated) {
            LOG(Plugins, "%p Plug-in was detected as the primary element in the page. Restart.", this);
            restartSnapshottedPlugIn();
            restartSimilarPlugIns();
        } else {
            LOG(Plugins, "%p Plug-in was detected as the primary element in the page, but is not yet created. Will restart later.", this);
            m_deferredPromotionToPrimaryPlugIn = true;
        }
    }
}

void HTMLPlugInImageElement::restartSnapshottedPlugIn()
{
    if (displayState() >= RestartingWithPendingMouseClick)
        return;

    setDisplayState(Restarting);
    setNeedsStyleRecalc(ReconstructRenderTree);
}

void HTMLPlugInImageElement::dispatchPendingMouseClick()
{
    ASSERT(!m_simulatedMouseClickTimer.isActive());
    m_simulatedMouseClickTimer.restart();
}

void HTMLPlugInImageElement::simulatedMouseClickTimerFired()
{
    ASSERT(displayState() == RestartingWithPendingMouseClick);
    ASSERT(m_pendingClickEventFromSnapshot);

    setDisplayState(Playing);
    dispatchSimulatedClick(m_pendingClickEventFromSnapshot.get(), SendMouseOverUpDownEvents, DoNotShowPressedLook);

    m_pendingClickEventFromSnapshot = nullptr;
}

static bool documentHadRecentUserGesture(Document& document)
{
    double lastKnownUserGestureTimestamp = document.lastHandledUserGestureTimestamp();

    if (document.frame() != &document.page()->mainFrame() && document.page()->mainFrame().document())
        lastKnownUserGestureTimestamp = std::max(lastKnownUserGestureTimestamp, document.page()->mainFrame().document()->lastHandledUserGestureTimestamp());

    if (monotonicallyIncreasingTime() - lastKnownUserGestureTimestamp < autostartSoonAfterUserGestureThreshold)
        return true;

    return false;
}

void HTMLPlugInImageElement::checkSizeChangeForSnapshotting()
{
    if (!m_needsCheckForSizeChange || m_snapshotDecision != MaySnapshotWhenResized || documentHadRecentUserGesture(document()))
        return;

    m_needsCheckForSizeChange = false;
    LayoutRect contentBoxRect = downcast<RenderBox>(*renderer()).contentBoxRect();
    int contentWidth = contentBoxRect.width();
    int contentHeight = contentBoxRect.height();

    if (contentWidth <= sizingTinyDimensionThreshold || contentHeight <= sizingTinyDimensionThreshold)
        return;

    LOG(Plugins, "%p Plug-in originally avoided snapshotting because it was sized %dx%d. Now it is %dx%d. Tell it to snapshot.\n", this, m_sizeWhenSnapshotted.width(), m_sizeWhenSnapshotted.height(), contentWidth, contentHeight);
    setDisplayState(WaitingForSnapshot);
    m_snapshotDecision = Snapshotted;
    Widget* widget = pluginWidget();
    if (is<PluginViewBase>(widget))
        downcast<PluginViewBase>(*widget).beginSnapshottingRunningPlugin();
}

static inline bool is100Percent(Length length)
{
    return length.isPercent() && length.percent() == 100;
}
    
static inline bool isSmallerThanTinySizingThreshold(const RenderEmbeddedObject& renderer)
{
    LayoutRect contentRect = renderer.contentBoxRect();
    return contentRect.width() <= sizingTinyDimensionThreshold || contentRect.height() <= sizingTinyDimensionThreshold;
}
    
bool HTMLPlugInImageElement::isTopLevelFullPagePlugin(const RenderEmbeddedObject& renderer) const
{
    Frame& frame = *document().frame();
    if (!frame.isMainFrame())
        return false;
    
    auto& style = renderer.style();
    IntSize visibleSize = frame.view()->visibleSize();
    LayoutRect contentRect = renderer.contentBoxRect();
    float contentWidth = contentRect.width();
    float contentHeight = contentRect.height();
    return is100Percent(style.width()) && is100Percent(style.height()) && contentWidth * contentHeight > visibleSize.area().unsafeGet() * sizingFullPageAreaRatioThreshold;
}
    
void HTMLPlugInImageElement::checkSnapshotStatus()
{
    if (!is<RenderSnapshottedPlugIn>(*renderer())) {
        if (displayState() == Playing)
            checkSizeChangeForSnapshotting();
        return;
    }
    
    // If width and height styles were previously not set and we've snapshotted the plugin we may need to restart the plugin so that its state can be updated appropriately.
    if (!document().page()->settings().snapshotAllPlugIns() && displayState() <= DisplayingSnapshot && !m_plugInDimensionsSpecified) {
        RenderSnapshottedPlugIn& renderer = downcast<RenderSnapshottedPlugIn>(*this->renderer());
        if (!renderer.style().logicalWidth().isSpecified() && !renderer.style().logicalHeight().isSpecified())
            return;
        
        m_plugInDimensionsSpecified = true;
        if (isTopLevelFullPagePlugin(renderer)) {
            m_snapshotDecision = NeverSnapshot;
            restartSnapshottedPlugIn();
        } else if (isSmallerThanTinySizingThreshold(renderer)) {
            m_snapshotDecision = MaySnapshotWhenResized;
            restartSnapshottedPlugIn();
        }
        return;
    }
    
    // Notify the shadow root that the size changed so that we may update the overlay layout.
    ensureUserAgentShadowRoot().dispatchEvent(Event::create(eventNames().resizeEvent, true, false));
}
    
void HTMLPlugInImageElement::subframeLoaderWillCreatePlugIn(const URL& url)
{
    LOG(Plugins, "%p Plug-in URL: %s", this, m_url.utf8().data());
    LOG(Plugins, "   Actual URL: %s", url.string().utf8().data());
    LOG(Plugins, "   MIME type: %s", loadedMimeType().utf8().data());

    m_loadedUrl = url;
    m_plugInWasCreated = false;
    m_deferredPromotionToPrimaryPlugIn = false;

    if (!document().page() || !document().page()->settings().plugInSnapshottingEnabled()) {
        m_snapshotDecision = NeverSnapshot;
        return;
    }

    if (displayState() == Restarting) {
        LOG(Plugins, "%p Plug-in is explicitly restarting", this);
        m_snapshotDecision = NeverSnapshot;
        setDisplayState(Playing);
        return;
    }

    if (displayState() == RestartingWithPendingMouseClick) {
        LOG(Plugins, "%p Plug-in is explicitly restarting but also waiting for a click", this);
        m_snapshotDecision = NeverSnapshot;
        return;
    }

    if (m_snapshotDecision == NeverSnapshot) {
        LOG(Plugins, "%p Plug-in is blessed, allow it to start", this);
        return;
    }

    bool inMainFrame = document().frame()->isMainFrame();

    if (document().isPluginDocument() && inMainFrame) {
        LOG(Plugins, "%p Plug-in document in main frame", this);
        m_snapshotDecision = NeverSnapshot;
        return;
    }

    if (ScriptController::processingUserGesture()) {
        LOG(Plugins, "%p Script is currently processing user gesture, set to play", this);
        m_snapshotDecision = NeverSnapshot;
        return;
    }

    if (m_createdDuringUserGesture) {
        LOG(Plugins, "%p Plug-in was created when processing user gesture, set to play", this);
        m_snapshotDecision = NeverSnapshot;
        return;
    }

    if (documentHadRecentUserGesture(document())) {
        LOG(Plugins, "%p Plug-in was created shortly after a user gesture, set to play", this);
        m_snapshotDecision = NeverSnapshot;
        return;
    }

    if (document().page()->settings().snapshotAllPlugIns()) {
        LOG(Plugins, "%p Plug-in forced to snapshot by user preference", this);
        m_snapshotDecision = Snapshotted;
        setDisplayState(WaitingForSnapshot);
        return;
    }

    if (document().page()->settings().autostartOriginPlugInSnapshottingEnabled() && document().page()->plugInClient() && document().page()->plugInClient()->shouldAutoStartFromOrigin(document().page()->mainFrame().document()->baseURL().host(), url.host(), loadedMimeType())) {
        LOG(Plugins, "%p Plug-in from (%s, %s) is marked to auto-start, set to play", this, document().page()->mainFrame().document()->baseURL().host().utf8().data(), url.host().utf8().data());
        m_snapshotDecision = NeverSnapshot;
        return;
    }

    if (m_loadedUrl.isEmpty() && !loadedMimeType().isEmpty()) {
        LOG(Plugins, "%p Plug-in has no src URL but does have a valid mime type %s, set to play", this, loadedMimeType().utf8().data());
        m_snapshotDecision = MaySnapshotWhenContentIsSet;
        return;
    }

    if (!SchemeRegistry::shouldTreatURLSchemeAsLocal(m_loadedUrl.protocol()) && !m_loadedUrl.host().isEmpty() && m_loadedUrl.host() == document().page()->mainFrame().document()->baseURL().host()) {
        LOG(Plugins, "%p Plug-in is served from page's domain, set to play", this);
        m_snapshotDecision = NeverSnapshot;
        return;
    }
    
    auto& renderer = downcast<RenderEmbeddedObject>(*this->renderer());
    LayoutRect contentRect = renderer.contentBoxRect();
    int contentWidth = contentRect.width();
    int contentHeight = contentRect.height();
    
    m_plugInDimensionsSpecified = renderer.style().logicalWidth().isSpecified() || renderer.style().logicalHeight().isSpecified();
    
    if (isTopLevelFullPagePlugin(renderer)) {
        LOG(Plugins, "%p Plug-in is top level full page, set to play", this);
        m_snapshotDecision = NeverSnapshot;
        return;
    }

    if (isSmallerThanTinySizingThreshold(renderer)) {
        LOG(Plugins, "%p Plug-in is very small %dx%d, set to play", this, contentWidth, contentHeight);
        m_sizeWhenSnapshotted = IntSize(contentWidth, contentHeight);
        m_snapshotDecision = MaySnapshotWhenResized;
        return;
    }

    if (!document().page()->plugInClient()) {
        LOG(Plugins, "%p There is no plug-in client. Set to wait for snapshot", this);
        m_snapshotDecision = NeverSnapshot;
        setDisplayState(WaitingForSnapshot);
        return;
    }

    LOG(Plugins, "%p Plug-in from (%s, %s) is not auto-start, sized at %dx%d, set to wait for snapshot", this, document().topDocument().baseURL().host().utf8().data(), url.host().utf8().data(), contentWidth, contentHeight);
    m_snapshotDecision = Snapshotted;
    setDisplayState(WaitingForSnapshot);
}

void HTMLPlugInImageElement::subframeLoaderDidCreatePlugIn(const Widget& widget)
{
    m_plugInWasCreated = true;

    if (is<PluginViewBase>(widget) && downcast<PluginViewBase>(widget).shouldAlwaysAutoStart()) {
        LOG(Plugins, "%p Plug-in should auto-start, set to play", this);
        m_snapshotDecision = NeverSnapshot;
        setDisplayState(Playing);
        return;
    }

    if (m_deferredPromotionToPrimaryPlugIn) {
        LOG(Plugins, "%p Plug-in was created, previously deferred promotion to primary. Will promote", this);
        setIsPrimarySnapshottedPlugIn(true);
        m_deferredPromotionToPrimaryPlugIn = false;
    }
}

void HTMLPlugInImageElement::defaultEventHandler(Event* event)
{
    RenderElement* r = renderer();
    if (r && r->isEmbeddedObject()) {
        if (displayState() == WaitingForSnapshot && is<MouseEvent>(*event) && event->type() == eventNames().clickEvent) {
            MouseEvent& mouseEvent = downcast<MouseEvent>(*event);
            if (mouseEvent.button() == (unsigned short)LeftButton) {
                userDidClickSnapshot(&mouseEvent, true);
                mouseEvent.setDefaultHandled();
                return;
            }
        }
    }
    HTMLPlugInElement::defaultEventHandler(event);
}

bool HTMLPlugInImageElement::requestObject(const String& url, const String& mimeType, const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    if (HTMLPlugInElement::requestObject(url, mimeType, paramNames, paramValues))
        return true;
    
    SubframeLoader& loader = document().frame()->loader().subframeLoader();
    return loader.requestObject(*this, url, getNameAttribute(), mimeType, paramNames, paramValues);
}

} // namespace WebCore

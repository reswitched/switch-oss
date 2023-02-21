/**
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006, 2014 Apple Inc.
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
 */

#include "config.h"
#include "HTMLPlugInElement.h"

#include "BridgeJSC.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "CSSPropertyNames.h"
#include "Document.h"
#include "Event.h"
#include "EventHandler.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameTree.h"
#include "HTMLNames.h"
#include "Logging.h"
#include "MIMETypeRegistry.h"
#include "Page.h"
#include "PluginData.h"
#include "PluginReplacement.h"
#include "PluginViewBase.h"
#include "RenderEmbeddedObject.h"
#include "RenderSnapshottedPlugIn.h"
#include "RenderWidget.h"
#include "RuntimeEnabledFeatures.h"
#include "ScriptController.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "SubframeLoader.h"
#include "Widget.h"

#if ENABLE(NETSCAPE_PLUGIN_API)
#include "npruntime_impl.h"
#endif

#if PLATFORM(COCOA)
#include "QuickTimePluginReplacement.h"
#include "YouTubePluginReplacement.h"
#endif

namespace WebCore {

using namespace HTMLNames;

HTMLPlugInElement::HTMLPlugInElement(const QualifiedName& tagName, Document& document)
    : HTMLFrameOwnerElement(tagName, document)
    , m_inBeforeLoadEventHandler(false)
    , m_swapRendererTimer(*this, &HTMLPlugInElement::swapRendererTimerFired)
#if ENABLE(NETSCAPE_PLUGIN_API)
    , m_NPObject(0)
#endif
    , m_isCapturingMouseEvents(false)
    , m_displayState(Playing)
{
    setHasCustomStyleResolveCallbacks();
}

HTMLPlugInElement::~HTMLPlugInElement()
{
    ASSERT(!m_instance); // cleared in detach()

#if ENABLE(NETSCAPE_PLUGIN_API)
    if (m_NPObject) {
        _NPN_ReleaseObject(m_NPObject);
        m_NPObject = 0;
    }
#endif
}

bool HTMLPlugInElement::canProcessDrag() const
{
    const PluginViewBase* plugin = is<PluginViewBase>(pluginWidget()) ? downcast<PluginViewBase>(pluginWidget()) : nullptr;
    return plugin ? plugin->canProcessDrag() : false;
}

bool HTMLPlugInElement::willRespondToMouseClickEvents()
{
    if (isDisabledFormControl())
        return false;
    auto renderer = this->renderer();
    return renderer && renderer->isWidget();
}

void HTMLPlugInElement::willDetachRenderers()
{
    m_instance = nullptr;

    if (m_isCapturingMouseEvents) {
        if (Frame* frame = document().frame())
            frame->eventHandler().setCapturingMouseEventsElement(nullptr);
        m_isCapturingMouseEvents = false;
    }

#if ENABLE(NETSCAPE_PLUGIN_API)
    if (m_NPObject) {
        _NPN_ReleaseObject(m_NPObject);
        m_NPObject = 0;
    }
#endif
}

void HTMLPlugInElement::resetInstance()
{
    m_instance = nullptr;
}

PassRefPtr<JSC::Bindings::Instance> HTMLPlugInElement::getInstance()
{
    Frame* frame = document().frame();
    if (!frame)
        return 0;

    // If the host dynamically turns off JavaScript (or Java) we will still return
    // the cached allocated Bindings::Instance.  Not supporting this edge-case is OK.
    if (m_instance)
        return m_instance;

    if (Widget* widget = pluginWidget())
        m_instance = frame->script().createScriptInstanceForWidget(widget);

    return m_instance;
}

bool HTMLPlugInElement::guardedDispatchBeforeLoadEvent(const String& sourceURL)
{
    // FIXME: Our current plug-in loading design can't guarantee the following
    // assertion is true, since plug-in loading can be initiated during layout,
    // and synchronous layout can be initiated in a beforeload event handler!
    // See <http://webkit.org/b/71264>.
    // ASSERT(!m_inBeforeLoadEventHandler);
    m_inBeforeLoadEventHandler = true;
    // static_cast is used to avoid a compile error since dispatchBeforeLoadEvent
    // is intentionally undefined on this class.
    bool beforeLoadAllowedLoad = static_cast<HTMLFrameOwnerElement*>(this)->dispatchBeforeLoadEvent(sourceURL);
    m_inBeforeLoadEventHandler = false;
    return beforeLoadAllowedLoad;
}

Widget* HTMLPlugInElement::pluginWidget(PluginLoadingPolicy loadPolicy) const
{
    if (m_inBeforeLoadEventHandler) {
        // The plug-in hasn't loaded yet, and it makes no sense to try to load if beforeload handler happened to touch the plug-in element.
        // That would recursively call beforeload for the same element.
        return nullptr;
    }

    RenderWidget* renderWidget = loadPolicy == PluginLoadingPolicy::Load ? renderWidgetLoadingPlugin() : this->renderWidget();
    if (!renderWidget)
        return nullptr;

    return renderWidget->widget();
}

bool HTMLPlugInElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == widthAttr || name == heightAttr || name == vspaceAttr || name == hspaceAttr || name == alignAttr)
        return true;
    return HTMLFrameOwnerElement::isPresentationAttribute(name);
}

void HTMLPlugInElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStyleProperties& style)
{
    if (name == widthAttr)
        addHTMLLengthToStyle(style, CSSPropertyWidth, value);
    else if (name == heightAttr)
        addHTMLLengthToStyle(style, CSSPropertyHeight, value);
    else if (name == vspaceAttr) {
        addHTMLLengthToStyle(style, CSSPropertyMarginTop, value);
        addHTMLLengthToStyle(style, CSSPropertyMarginBottom, value);
    } else if (name == hspaceAttr) {
        addHTMLLengthToStyle(style, CSSPropertyMarginLeft, value);
        addHTMLLengthToStyle(style, CSSPropertyMarginRight, value);
    } else if (name == alignAttr)
        applyAlignmentAttributeToStyle(value, style);
    else
        HTMLFrameOwnerElement::collectStyleForPresentationAttribute(name, value, style);
}

void HTMLPlugInElement::defaultEventHandler(Event* event)
{
    // Firefox seems to use a fake event listener to dispatch events to plug-in (tested with mouse events only).
    // This is observable via different order of events - in Firefox, event listeners specified in HTML attributes fires first, then an event
    // gets dispatched to plug-in, and only then other event listeners fire. Hopefully, this difference does not matter in practice.

    // FIXME: Mouse down and scroll events are passed down to plug-in via custom code in EventHandler; these code paths should be united.

    auto renderer = this->renderer();
    if (!is<RenderWidget>(renderer))
        return;

    if (is<RenderEmbeddedObject>(*renderer)) {
        if (downcast<RenderEmbeddedObject>(*renderer).isPluginUnavailable()) {
            downcast<RenderEmbeddedObject>(*renderer).handleUnavailablePluginIndicatorEvent(event);
            return;
        }

        if (is<RenderSnapshottedPlugIn>(*renderer) && displayState() < Restarting) {
            downcast<RenderSnapshottedPlugIn>(*renderer).handleEvent(event);
            HTMLFrameOwnerElement::defaultEventHandler(event);
            return;
        }

        if (displayState() < Playing)
            return;
    }

    RefPtr<Widget> widget = downcast<RenderWidget>(*renderer).widget();
    if (!widget)
        return;
    widget->handleEvent(event);
    if (event->defaultHandled())
        return;
    HTMLFrameOwnerElement::defaultEventHandler(event);
}

bool HTMLPlugInElement::isKeyboardFocusable(KeyboardEvent*) const
{
    // FIXME: Why is this check needed?
    if (!document().page())
        return false;

    Widget* widget = pluginWidget();
    if (!is<PluginViewBase>(widget))
        return false;

    return downcast<PluginViewBase>(*widget).supportsKeyboardFocus();
}

bool HTMLPlugInElement::isPluginElement() const
{
    return true;
}

bool HTMLPlugInElement::isUserObservable() const
{
    // No widget - can't be anything to see or hear here.
    Widget* widget = pluginWidget(PluginLoadingPolicy::DoNotLoad);
    if (!is<PluginViewBase>(widget))
        return false;

    PluginViewBase& pluginView = downcast<PluginViewBase>(*widget);

    // If audio is playing (or might be) then the plugin is detectable.
    if (pluginView.audioHardwareActivity() != AudioHardwareActivityType::IsInactive)
        return true;

    // If the plugin is visible and not vanishingly small in either dimension it is detectable.
    return pluginView.isVisible() && pluginView.width() > 2 && pluginView.height() > 2;
}

bool HTMLPlugInElement::supportsFocus() const
{
    if (HTMLFrameOwnerElement::supportsFocus())
        return true;

    if (useFallbackContent() || !is<RenderEmbeddedObject>(renderer()))
        return false;
    return !downcast<RenderEmbeddedObject>(*renderer()).isPluginUnavailable();
}

#if ENABLE(NETSCAPE_PLUGIN_API)

NPObject* HTMLPlugInElement::getNPObject()
{
    ASSERT(document().frame());
    if (!m_NPObject)
        m_NPObject = document().frame()->script().createScriptObjectForPluginElement(this);
    return m_NPObject;
}

#endif /* ENABLE(NETSCAPE_PLUGIN_API) */

RenderPtr<RenderElement> HTMLPlugInElement::createElementRenderer(Ref<RenderStyle>&& style, const RenderTreePosition& insertionPosition)
{
    if (m_pluginReplacement && m_pluginReplacement->willCreateRenderer())
        return m_pluginReplacement->createElementRenderer(*this, WTF::move(style), insertionPosition);

    return createRenderer<RenderEmbeddedObject>(*this, WTF::move(style));
}

void HTMLPlugInElement::swapRendererTimerFired()
{
    ASSERT(displayState() == PreparingPluginReplacement || displayState() == DisplayingSnapshot);
    if (userAgentShadowRoot())
        return;
    
    // Create a shadow root, which will trigger the code to add a snapshot container
    // and reattach, thus making a new Renderer.
    ensureUserAgentShadowRoot();
}

void HTMLPlugInElement::setDisplayState(DisplayState state)
{
    if (state == m_displayState)
        return;

    m_displayState = state;
    
    m_swapRendererTimer.stop();
    if ((state == DisplayingSnapshot || displayState() == PreparingPluginReplacement))
        m_swapRendererTimer.startOneShot(0);
}

void HTMLPlugInElement::didAddUserAgentShadowRoot(ShadowRoot* root)
{
    if (!m_pluginReplacement || !document().page() || displayState() != PreparingPluginReplacement)
        return;
    
    root->setResetStyleInheritance(true);
    if (m_pluginReplacement->installReplacement(root)) {
        setDisplayState(DisplayingPluginReplacement);
        setNeedsStyleRecalc(ReconstructRenderTree);
    }
}

#if PLATFORM(COCOA)
static void registrar(const ReplacementPlugin&);
#endif

static Vector<ReplacementPlugin*>& registeredPluginReplacements()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(Vector<ReplacementPlugin*>, registeredReplacements, ());
#if !PLATFORM(WKC)
    static bool enginesQueried = false;
#else
    WKC_DEFINE_STATIC_BOOL(enginesQueried, false);
#endif
    
    if (enginesQueried)
        return registeredReplacements;
    enginesQueried = true;

#if PLATFORM(COCOA)
    QuickTimePluginReplacement::registerPluginReplacement(registrar);
    YouTubePluginReplacement::registerPluginReplacement(registrar);
#endif
    
    return registeredReplacements;
}

#if PLATFORM(COCOA)
static void registrar(const ReplacementPlugin& replacement)
{
    registeredPluginReplacements().append(new ReplacementPlugin(replacement));
}
#endif

static ReplacementPlugin* pluginReplacementForType(const URL& url, const String& mimeType)
{
    Vector<ReplacementPlugin*>& replacements = registeredPluginReplacements();
    if (replacements.isEmpty())
        return nullptr;

    String extension;
    String lastPathComponent = url.lastPathComponent();
    size_t dotOffset = lastPathComponent.reverseFind('.');
    if (dotOffset != notFound)
        extension = lastPathComponent.substring(dotOffset + 1);

    String type = mimeType;
    if (type.isEmpty() && url.protocolIsData())
        type = mimeTypeFromDataURL(url.string());
    
    if (type.isEmpty() && !extension.isEmpty()) {
        for (auto* replacement : replacements) {
            if (replacement->supportsFileExtension(extension) && replacement->supportsURL(url))
                return replacement;
        }
    }
    
    if (type.isEmpty()) {
        if (extension.isEmpty())
            return nullptr;
        type = MIMETypeRegistry::getMediaMIMETypeForExtension(extension);
    }

    if (type.isEmpty())
        return nullptr;

    for (auto* replacement : replacements) {
        if (replacement->supportsType(type) && replacement->supportsURL(url))
            return replacement;
    }

    return nullptr;
}

bool HTMLPlugInElement::requestObject(const String& url, const String& mimeType, const Vector<String>& paramNames, const Vector<String>& paramValues)
{
    if (m_pluginReplacement)
        return true;

    URL completedURL;
    if (!url.isEmpty())
        completedURL = document().completeURL(url);

    ReplacementPlugin* replacement = pluginReplacementForType(completedURL, mimeType);
    if (!replacement || !replacement->isEnabledBySettings(document().settings()))
        return false;

    LOG(Plugins, "%p - Found plug-in replacement for %s.", this, completedURL.string().utf8().data());

    m_pluginReplacement = replacement->create(*this, paramNames, paramValues);
    setDisplayState(PreparingPluginReplacement);
    return true;
}

JSC::JSObject* HTMLPlugInElement::scriptObjectForPluginReplacement()
{
    if (m_pluginReplacement)
        return m_pluginReplacement->scriptObject();
    return nullptr;
}

}

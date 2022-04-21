/*
 * Copyright (C) 2007, 2008 Holger Hans Peter Freyther
 * Copyright (C) 2007, 2008 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (c) 2010-2020 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "Page.h"
#include "Frame.h"
#include "FrameView.h"
#include "WKCWebView.h"
#include "WKCWebFrame.h"
#include "WKCWebFramePrivate.h"
#include "ChromeClientWKC.h"
#include "PopupMenuWKC.h"
#include "SearchPopupMenuWKC.h"
#include "FileChooser.h"
#include "DateTimeChooser.h"
#include "Modules/geolocation/Geolocation.h"
#include "FrameLoadRequest.h"
#include "WindowFeatures.h"
#include "WTFString.h"
#include "DateTimeChooserClient.h"
#include "HTMLMediaElement.h"
#include "HTMLVideoElement.h"
#include "DisplayRefreshMonitorWKC.h"

#if ENABLE(INPUT_TYPE_COLOR)
# include "ColorChooser.h"
#endif

# include "GraphicsLayer.h"

#include <wkc/wkcgpeer.h>

#include "WKCWebViewPrivate.h"

#include "NotImplemented.h"

#include "helpers/ChromeClientIf.h"
#include "helpers/WKCPage.h"
#include "helpers/WKCString.h"
#include "helpers/WKCHitTestResult.h"
#include "helpers/WKCFrameLoadRequest.h"

#include "helpers/privates/WKCHelpersEnumsPrivate.h"
#include "helpers/privates/WKCHitTestResultPrivate.h"
#include "helpers/privates/WKCFrameLoadRequestPrivate.h"
#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/privates/WKCElementPrivate.h"
#include "helpers/privates/WKCFileChooserPrivate.h"
#include "helpers/privates/WKCGeolocationPrivate.h"
#include "helpers/privates/WKCNodePrivate.h"
#include "helpers/privates/WKCPagePrivate.h"
#include "helpers/privates/WKCSecurityOriginPrivate.h"
#include "helpers/WKCViewportArguments.h"

# include "helpers/privates/WKCGraphicsLayerPrivate.h"
namespace WebCore {
WKC::GraphicsLayerPrivate* GraphicsLayerWKC_wkc(const WebCore::GraphicsLayer* layer);
}

namespace WKC {

ChromeClientWKC::ChromeClientWKC(WKCWebViewPrivate* view)
     : m_view(view)
{
    m_appClient = 0;
}

ChromeClientWKC::~ChromeClientWKC()
{
    if (m_appClient) {
        m_view->clientBuilders().deleteChromeClient(m_appClient);
        m_appClient = 0;
    }
}

ChromeClientWKC*
ChromeClientWKC::create(WKCWebViewPrivate* view)
{
    ChromeClientWKC* self = 0;
    self = new ChromeClientWKC(view);
    if (!self) return 0;
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
ChromeClientWKC::construct()
{
    m_appClient = m_view->clientBuilders().createChromeClient(m_view->parent());
    if (!m_appClient) return false;
    return true;
}

void*
ChromeClientWKC::webView() const
{
    return m_view->parent();
}

void
ChromeClientWKC::chromeDestroyed()
{
    delete this;
}

void
ChromeClientWKC::setWindowRect(const WebCore::FloatRect& rect)
{
    WKCFloatRect r = { rect.x(), rect.y(), rect.width(), rect.height() };
    m_appClient->setWindowRect(r);
}
WebCore::FloatRect
ChromeClientWKC::windowRect()
{
    WKCFloatRect rr = m_appClient->windowRect();
    return WebCore::FloatRect(rr.fX, rr.fY, rr.fWidth, rr.fHeight);
}

WebCore::FloatRect
ChromeClientWKC::pageRect()
{
    WKCFloatRect rr = m_appClient->pageRect();
    return WebCore::FloatRect(rr.fX, rr.fY, rr.fWidth, rr.fHeight);
}

void
ChromeClientWKC::focus()
{
    m_appClient->focus();
}
void
ChromeClientWKC::unfocus()
{
    m_appClient->unfocus();
}

bool
ChromeClientWKC::canTakeFocus(WebCore::FocusDirection dir)
{
    return m_appClient->canTakeFocus(toWKCFocusDirection(dir));
}
void
ChromeClientWKC::takeFocus(WebCore::FocusDirection dir)
{
    m_appClient->takeFocus(toWKCFocusDirection(dir));
}

void
ChromeClientWKC::focusedElementChanged(WebCore::Element* element)
{
    if (!element) {
        m_appClient->focusedNodeChanged(0);
    } else {
        NodePrivate* n = NodePrivate::create(element);
        m_appClient->focusedNodeChanged(&n->wkc());
        delete n;
    }
}

WebCore::Page*
ChromeClientWKC::createWindow(WebCore::Frame& frame, const WebCore::FrameLoadRequest& request, const WebCore::WindowFeatures& features, const WebCore::NavigationAction& navact)
{
    FramePrivate fp(&frame);
    FrameLoadRequestPrivate req(request);
    WKC::Page* ret = m_appClient->createWindow(&fp.wkc(), req.wkc(), (const WKC::WindowFeatures &)features);
    if (!ret)
        return 0;
    return (WebCore::Page *)ret->priv().webcore();
}
void
ChromeClientWKC::show()
{
    m_appClient->show();
}

bool
ChromeClientWKC::canRunModal()
{
    return m_appClient->canRunModal();
}
void
ChromeClientWKC::runModal()
{
    m_appClient->runModal();
}

void
ChromeClientWKC::setToolbarsVisible(bool visible)
{
    m_appClient->setToolbarsVisible(visible);
}
bool
ChromeClientWKC::toolbarsVisible()
{
    return m_appClient->toolbarsVisible();
}

void
ChromeClientWKC::setStatusbarVisible(bool visible)
{
    m_appClient->setStatusbarVisible(visible);
}
bool
ChromeClientWKC::statusbarVisible()
{
    return m_appClient->statusbarVisible();
}

void
ChromeClientWKC::setScrollbarsVisible(bool visible)
{
    m_appClient->setScrollbarsVisible(visible);
}
bool
ChromeClientWKC::scrollbarsVisible()
{
    return m_appClient->scrollbarsVisible();
}

void
ChromeClientWKC::setMenubarVisible(bool visible)
{
    m_appClient->setMenubarVisible(visible);
}
bool
ChromeClientWKC::menubarVisible()
{
    return m_appClient->menubarVisible();
}

void
ChromeClientWKC::setResizable(bool flag)
{
    m_appClient->setResizable(flag);
}

void
ChromeClientWKC::addMessageToConsole(JSC::MessageSource source,
                                  JSC::MessageLevel level, const WTF::String& message,
                                  unsigned int lineNumber, unsigned int columnNumber, const WTF::String& sourceID)
{
    m_appClient->addMessageToConsole(toWKCMessageSource(source), WKC::MessageType::Log, toWKCMessageLevel(level), message, lineNumber, sourceID);
}

bool
ChromeClientWKC::canRunBeforeUnloadConfirmPanel()
{
    return m_appClient->canRunBeforeUnloadConfirmPanel();
}
bool
ChromeClientWKC::runBeforeUnloadConfirmPanel(const WTF::String& message, WebCore::Frame& frame)
{
    FramePrivate fp(&frame);
    return m_appClient->runBeforeUnloadConfirmPanel(message, &fp.wkc());
}

void
ChromeClientWKC::closeWindowSoon()
{
	wkcWebView()->stopLoading();
	wkcWebView()->mainFrame()->privateFrame()->core()->page()->setGroupName(WTF::String());
	m_appClient->closeWindowSoon();
}

void
ChromeClientWKC::runJavaScriptAlert(WebCore::Frame& frame, const WTF::String& string)
{
    FramePrivate fp(&frame);
    m_appClient->runJavaScriptAlert(&fp.wkc(), string);
}
bool
ChromeClientWKC::runJavaScriptConfirm(WebCore::Frame& frame, const WTF::String& string)
{
    FramePrivate fp(&frame);
    return m_appClient->runJavaScriptConfirm(&fp.wkc(), string);
}
bool
ChromeClientWKC::runJavaScriptPrompt(WebCore::Frame& frame, const WTF::String& message, const WTF::String& defaultvalue, WTF::String& out_result)
{
    WKC::String result("");
    FramePrivate fp(&frame);
    bool ret = m_appClient->runJavaScriptPrompt(&fp.wkc(), message, defaultvalue, result);
    out_result = result;
    return ret;
}
void
ChromeClientWKC::setStatusbarText(const WTF::String& string)
{
    m_appClient->setStatusbarText(string);
}
bool
ChromeClientWKC::shouldInterruptJavaScript()
{
    return m_appClient->shouldInterruptJavaScript();
}

bool
ChromeClientWKC::supportsImmediateInvalidation()
{
    return true;
}

void
ChromeClientWKC::invalidateContentsAndRootView(const WebCore::IntRect& rect)
{
    m_view->updateOverlay(rect, false);
    m_appClient->repaint(rect, true /*contentChanged*/, false, true /*repaintContentOnly*/);
}

void
ChromeClientWKC::invalidateRootView(const WebCore::IntRect& rect)
{
    m_view->updateOverlay(rect, true);
    m_appClient->repaint(rect, false /*contentChanged*/, true, false /*repaintContentOnly*/);
}

void
ChromeClientWKC::invalidateContentsForSlowScroll(const WebCore::IntRect& rect)
{
    m_appClient->repaint(rect, true /*contentChanged*/, true, true /*repaintContentOnly*/);
}

void
ChromeClientWKC::scroll(const WebCore::IntSize& scrollDelta, const WebCore::IntRect& rectToScroll, const WebCore::IntRect& clipRect)
{
    m_appClient->scroll(scrollDelta, rectToScroll, clipRect);
}
WebCore::IntPoint
ChromeClientWKC::screenToRootView(const WebCore::IntPoint& pos) const
{
    return pos;
}
WebCore::IntRect
ChromeClientWKC::rootViewToScreen(const WebCore::IntRect& rect) const
{
    return rect;
}
WebCore::IntPoint
ChromeClientWKC::accessibilityScreenToRootView(const WebCore::IntPoint&) const
{
    notImplemented();
    return { };
}

WebCore::IntRect
ChromeClientWKC::rootViewToAccessibilityScreen(const WebCore::IntRect&) const
{
    notImplemented();
    return { };
}

void
ChromeClientWKC::didFinishLoadingImageForElement(WebCore::HTMLImageElement&)
{
    notImplemented();
}

PlatformPageClient
ChromeClientWKC::platformPageClient() const
{
    // This code relate to the PopupMenuWKC.
    // If you modified this code, check to the PopupMenuWKC also.
    return (PlatformPageClient)m_view;
}
void
ChromeClientWKC::contentsSizeChanged(WebCore::Frame& frame, const WebCore::IntSize& size) const
{
    WKCSize s = { size.width(), size.height() };
    FramePrivate fp(&frame);
    m_appClient->contentsSizeChanged(&fp.wkc(), s);
}

void
ChromeClientWKC::intrinsicContentsSizeChanged(const WebCore::IntSize &) const
{
    notImplemented();
}

void
ChromeClientWKC::mouseDidMoveOverElement(const WebCore::HitTestResult& result, unsigned modifierFlags, const WTF::String& toolTip, WebCore::TextDirection)
{
    HitTestResultPrivate r(result);
    m_appClient->mouseDidMoveOverElement(r.wkc(), modifierFlags);
}

void
ChromeClientWKC::print(WebCore::Frame& frame)
{
    FramePrivate f(&frame);
    m_appClient->print(&f.wkc());
}

void
ChromeClientWKC::exceededDatabaseQuota(WebCore::Frame& frame, const WTF::String& string, WebCore::DatabaseDetails)
{
    FramePrivate f(&frame);
    m_appClient->exceededDatabaseQuota(&f.wkc(), string);
}

void
ChromeClientWKC::reachedMaxAppCacheSize(int64_t spaceNeeded)
{
    m_appClient->reachedMaxAppCacheSize(spaceNeeded);
}

void 
ChromeClientWKC::reachedApplicationCacheOriginQuota(WebCore::SecurityOrigin& origin, int64_t totalSpaceNeeded)
{
    SecurityOriginPrivate o(&origin);
    m_appClient->reachedApplicationCacheOriginQuota(&o.wkc(), totalSpaceNeeded);
}

void
ChromeClientWKC::runOpenPanel(WebCore::Frame& frame, WebCore::FileChooser& chooser)
{
    FramePrivate fp(&frame);
    FileChooserPrivate fc(&chooser);
    m_appClient->runOpenPanel(&fp.wkc(), &fc.wkc());
}

void
ChromeClientWKC::elementDidFocus(WebCore::Element& element)
{
    ElementPrivate* e = ElementPrivate::create(&element);
    m_appClient->elementDidFocus(&e->wkc());
    delete e;
}

void
ChromeClientWKC::elementDidBlur(WebCore::Element& element)
{
    ElementPrivate* e = ElementPrivate::create(&element);
    m_appClient->elementDidBlur(&e->wkc());
    delete e;
}

void
ChromeClientWKC::elementDidRefocus(WebCore::Element& element)
{
    ElementPrivate* e = ElementPrivate::create(&element);
    m_appClient->elementDidRefocus(&e->wkc());
    delete e;
}

bool
ChromeClientWKC::shouldPaintEntireContents() const
{
    return m_appClient->shouldPaintEntireContents();
}

void
ChromeClientWKC::setCursor(const WebCore::Cursor& handle)
{
    WKCPlatformCursor* p = static_cast<WKCPlatformCursor*>(handle.platformCursor());
    m_appClient->setCursor(p);
}

void
ChromeClientWKC::setCursorHiddenUntilMouseMoves(bool)
{
    notImplemented();
}

void
ChromeClientWKC::scrollRectIntoView(const WebCore::IntRect& rect) const
{
    m_appClient->scrollRectIntoView(rect);
}

void
ChromeClientWKC::focusedFrameChanged(WebCore::Frame* frame)
{
    if (!frame) {
        m_appClient->focusedFrameChanged(nullptr);
        return;
    }
    FramePrivate fp(frame);
    m_appClient->focusedFrameChanged(&fp.wkc());
}

WebCore::KeyboardUIMode
ChromeClientWKC::keyboardUIMode()
{
    WKC::KeyboardUIMode wmode = m_appClient->keyboardUIMode();
    unsigned mode = WebCore::KeyboardAccessDefault;

    if (wmode&WKC::KeyboardAccessFull) {
        mode |= WebCore::KeyboardAccessFull;
    }
    if (wmode&WKC::KeyboardAccessTabsToLinks) {
        mode |= WebCore::KeyboardAccessTabsToLinks;
    }
    return (WebCore::KeyboardUIMode)mode;
}

WebCore::Color
ChromeClientWKC::underlayColor() const
{
    unsigned int c = m_appClient->underlayColor();
    return WebCore::Color((int)(c>>16)&0xff, (int)(c>>8)&0xff, (int)(c&0xff), (int)(c>>24)&0xff);
}

bool
ChromeClientWKC::selectItemWritingDirectionIsNatural()
{
    return m_appClient->selectItemWritingDirectionIsNatural();
}

bool
ChromeClientWKC::selectItemAlignmentFollowsMenuWritingDirection()
{
    return m_appClient->selectItemAlignmentFollowsMenuWritingDirection();
}

RefPtr<WebCore::PopupMenu>
ChromeClientWKC::createPopupMenu(WebCore::PopupMenuClient& client) const
{
	return adoptRef(*new WebCore::PopupMenuWKC(&client));
}

RefPtr<WebCore::SearchPopupMenu>
ChromeClientWKC::createSearchPopupMenu(WebCore::PopupMenuClient& client) const
{
	return adoptRef(*new WebCore::SearchPopupMenuWKC(&client));
}

#if 0
bool
ChromeClientWKC::willAddTextFieldDecorationsTo(WebCore::HTMLInputElement*)
{
    // This function is called whenever a text field <input> is
    // created. The implementation should return true if it wants
    // to do something in addTextFieldDecorationsTo().
    return false;
}

void
ChromeClientWKC::addTextFieldDecorationsTo(WebCore::HTMLInputElement*)
{
}
#endif

void
ChromeClientWKC::wheelEventHandlersChanged(bool changed)
{
    m_appClient->wheelEventHandlersChanged(changed);
}

void
ChromeClientWKC::dispatchViewportPropertiesDidChange(const WebCore::ViewportArguments& arg) const
{
    WKC::ViewportArguments warg;

    warg.zoom = arg.zoom;
    warg.minZoom = arg.minZoom;
    warg.maxZoom = arg.maxZoom;
    warg.width = arg.width;
    warg.height = arg.height;
    warg.orientation = arg.orientation;
    warg.userZoom = arg.userZoom;

    m_appClient->dispatchViewportDataDidChange(warg);
}

void
ChromeClientWKC::dispatchMetaFocusRingVisibilityDidChange(const WTF::String& string) const
{
    m_appClient->dispatchMetaFocusRingVisibilityDidChange(string);
}

void
ChromeClientWKC::dispatchMetaInitialAutofocusDidChange(const WTF::String& string) const
{
    m_appClient->dispatchMetaInitialAutofocusDidChange(string);
}

bool
ChromeClientWKC::shouldUnavailablePluginMessageBeButton(WebCore::RenderEmbeddedObject::PluginUnavailabilityReason reason) const
{
    return m_appClient->shouldUnavailablePluginMessageBeButton((int)reason);
}

void
ChromeClientWKC::unavailablePluginButtonClicked(WebCore::Element& el, WebCore::RenderEmbeddedObject::PluginUnavailabilityReason reason) const
{
    WKC::ElementPrivate we(&el);
    m_appClient->unavailablePluginButtonClicked(&we.wkc(), (int)reason);
}

bool
ChromeClientWKC::supportsVideoFullscreen(WebCore::HTMLMediaElementEnums::VideoFullscreenMode)
{
    return true;
}

bool
ChromeClientWKC::supportsVideoFullscreenStandby()
{
    return true;
}

#if ENABLE(VIDEO)
void
ChromeClientWKC::enterVideoFullscreenForVideoElement(WebCore::HTMLVideoElement& element, WebCore::HTMLMediaElementEnums::VideoFullscreenMode, bool standby)
{
#if ENABLE(FULLSCREEN_API)
    element.webkitRequestFullscreen();
#endif
}
#endif

void
ChromeClientWKC::exitVideoFullscreenForVideoElement(WebCore::HTMLVideoElement& element)
{
    element.webkitExitFullscreen();
}

void
ChromeClientWKC::exitVideoFullscreenToModeWithoutAnimation(WebCore::HTMLVideoElement& element, WebCore::HTMLMediaElementEnums::VideoFullscreenMode)
{
    element.webkitExitFullscreen();
}

bool
ChromeClientWKC::requiresFullscreenForVideoPlayback()
{
    return m_appClient->requiresFullscreenForVideoPlayback();
}

void
ChromeClientWKC::notifyScrollerThumbIsVisibleInRect(const WebCore::IntRect& rect)
{
    m_appClient->notifyScrollerThumbIsVisibleInRect(rect);
}

void
ChromeClientWKC::recommendedScrollbarStyleDidChange(WebCore::ScrollbarStyle newStyle)
{
    m_appClient->recommendedScrollbarStyleDidChange((int)newStyle);
}

void
ChromeClientWKC::enableSuddenTermination()
{
    m_appClient->enableSuddenTermination();
}

void
ChromeClientWKC::disableSuddenTermination()
{
    m_appClient->disableSuddenTermination();
}

bool
ChromeClientWKC::isSVGImageChromeClient() const
{
    return false;
}

#if ENABLE(POINTER_LOCK)
WKC_DEFINE_GLOBAL_BOOL(gIsPointerLocked, false);

bool
ChromeClientWKC::requestPointerLock()
{
    gIsPointerLocked = true;
    return true;
}

void
ChromeClientWKC::requestPointerUnlock()
{
}

bool
ChromeClientWKC::isPointerLocked()
{
    return gIsPointerLocked;
}
#endif

WebCore::FloatSize
ChromeClientWKC::minimumWindowSize() const
{
    return m_appClient->minimumWindowSize();
}

bool
ChromeClientWKC::isEmptyChromeClient() const
{
    return false;
}

WTF::String
ChromeClientWKC::plugInStartLabelTitle(const WTF::String& mimeType) const
{
    return m_appClient->plugInStartLabelTitle(mimeType);
}

WTF::String
ChromeClientWKC::plugInStartLabelSubtitle(const WTF::String& mimeType) const
{
    return m_appClient->plugInStartLabelSubtitle(mimeType);
}

WTF::String
ChromeClientWKC::plugInExtraStyleSheet() const
{
    return m_appClient->plugInExtraStyleSheet();
}

WTF::String
ChromeClientWKC::plugInExtraScript() const
{
    return m_appClient->plugInExtraScript();
}

bool
ChromeClientWKC::shouldNotifyOnFormChanges()
{
    return false;
}

RefPtr<WebCore::Icon> ChromeClientWKC::createIconForFiles(const Vector<WTF::String>&)
{
    notImplemented();
    return nullptr;
}

// not implemented....

void
ChromeClientWKC::loadIconForFiles(const WTF::Vector<WTF::String>& icons, WebCore::FileIconLoader& loader)
{
    notImplemented();
}

#if ENABLE(INPUT_TYPE_COLOR)
std::unique_ptr<WebCore::ColorChooser>
ChromeClientWKC::createColorChooser(WebCore::ColorChooserClient&, const WebCore::Color&)
{
    notImplemented();
    return nullptr;
}
#endif

WebCore::GraphicsLayerFactory*
ChromeClientWKC::graphicsLayerFactory() const
{
    return 0;
}

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
RefPtr<WebCore::DisplayRefreshMonitor>
ChromeClientWKC::createDisplayRefreshMonitor(WebCore::PlatformDisplayID displayID) const
{
    return WebCore::DisplayRefreshMonitorWKC::create(displayID);
}
#endif

void
ChromeClientWKC::attachRootGraphicsLayer(WebCore::Frame& frame, WebCore::GraphicsLayer* layer)
{
    WKC::FramePrivate wf(&frame);
    if (layer) {
        // attach
        WKC::GraphicsLayerPrivate* wg = GraphicsLayerWKC_wkc(layer);
        m_appClient->attachRootGraphicsLayer(&wf.wkc(), &wg->wkc());
    } else {
        // detach
        m_appClient->attachRootGraphicsLayer(&wf.wkc(), 0);
    }
    m_view->setRootGraphicsLayer(layer);
}

void
ChromeClientWKC::attachViewOverlayGraphicsLayer(WebCore::GraphicsLayer* layer)
{
    if (layer) {
        // attach
        WKC::GraphicsLayerPrivate* wg = GraphicsLayerWKC_wkc(layer);
        m_appClient->attachViewOverlayGraphicsLayer(&wg->wkc());
    } else {
        // detach
        m_appClient->attachViewOverlayGraphicsLayer(0);
    }
}

void
ChromeClientWKC::setNeedsOneShotDrawingSynchronization()
{
    m_appClient->setNeedsOneShotDrawingSynchronization();
}

void
ChromeClientWKC::scheduleCompositingLayerFlush()
{
    m_appClient->scheduleCompositingLayerFlush();
}

bool
ChromeClientWKC::allowsAcceleratedCompositing() const
{
    return m_appClient->allowsAcceleratedCompositing();
}

WebCore::ChromeClient::CompositingTriggerFlags
ChromeClientWKC::allowedCompositingTriggers() const
{
    return static_cast<WebCore::ChromeClient::CompositingTriggerFlags>(m_appClient->allowedCompositingTriggers());
}

bool
ChromeClientWKC::layerTreeStateIsFrozen() const
{
    return m_appClient->layerTreeStateIsFrozen();
}

void
ChromeClientWKC::didAddHeaderLayer(WebCore::GraphicsLayer& layer)
{
    WKC::GraphicsLayerPrivate* wg = GraphicsLayerWKC_wkc(&layer);
    m_appClient->didAddHeaderLayer(&wg->wkc());
}

void
ChromeClientWKC::didAddFooterLayer(WebCore::GraphicsLayer& layer)
{
    WKC::GraphicsLayerPrivate* wg = GraphicsLayerWKC_wkc(&layer);
    m_appClient->didAddFooterLayer(&wg->wkc());
}

bool
ChromeClientWKC::shouldDispatchFakeMouseMoveEvents() const
{
    return m_appClient->shouldDispatchFakeMouseMoveEvents();
}


#if ENABLE(FULLSCREEN_API)
bool
ChromeClientWKC::supportsFullScreenForElement(const WebCore::Element& element, bool flag)
{
    if (element.isMediaElement()) {
        const WebCore::HTMLMediaElement& mediaElement = downcast<const WebCore::HTMLMediaElement>(element);
        RefPtr<WebCore::MediaPlayer> player = mediaElement.player();
        if (player) {
            return player->canEnterFullscreen();
        }
    }

    WebCore::Element& v = (WebCore::Element&)element;
    WKC::ElementPrivate el(&v);
    return m_appClient->supportsFullScreenForElement(&el.wkc(), flag);
}

void
ChromeClientWKC::enterFullScreenForElement(WebCore::Element& element)
{
    if (element.isMediaElement()) {
        WebCore::HTMLMediaElement& mediaElement = downcast<WebCore::HTMLMediaElement>(element);
        RefPtr<WebCore::MediaPlayer> player = mediaElement.player();
        if (player) {
            player->enterFullscreen();
        }
    }

    WKC::ElementPrivate el(&element);
    m_appClient->enterFullScreenForElement(&el.wkc());
}

void
ChromeClientWKC::exitFullScreenForElement(WebCore::Element* element)
{
    if (!element)
        return;

    if (element->isMediaElement()) {
        WebCore::HTMLMediaElement* mediaElement = downcast<WebCore::HTMLMediaElement>(element);
        RefPtr<WebCore::MediaPlayer> player = mediaElement->player();
        if (player) {
            player->exitFullscreen();
        }
    }

    WKC::ElementPrivate el(element);
    m_appClient->exitFullScreenForElement(&el.wkc());
}

void
ChromeClientWKC::setRootFullScreenLayer(WebCore::GraphicsLayer* layer)
{
    WKC::GraphicsLayerPrivate* wg = GraphicsLayerWKC_wkc(layer);
    m_appClient->setRootFullScreenLayer(&wg->wkc());
}
#endif

void
ChromeClientWKC::postAccessibilityNotification(WebCore::AccessibilityObject&, WebCore::AXObjectCache::AXNotification)
{
    notImplemented();
}

void
ChromeClientWKC::didAssociateFormControls(const WTF::Vector<WTF::RefPtr<WebCore::Element> >&, WebCore::Frame&)
{
    notImplemented();
}


WKCWebView*
ChromeClientWKC::wkcWebView() const
{
    return m_view->parent();
}

#if ENABLE(ORIENTATION_EVENTS)
int
ChromeClientWKC::deviceOrientation() const
{
    notImplemented();
    return 0;
}
#endif


} // namespace


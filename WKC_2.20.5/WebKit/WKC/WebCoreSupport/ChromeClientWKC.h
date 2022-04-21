/*
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (c) 2010-2021 ACCESS CO., LTD. All rights reserved.
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

#ifndef ChromeClientWKC_h
#define ChromeClientWKC_h

#include "ChromeClient.h"
#include "URL.h"

namespace WKC {
class ChromeClientIf;
class WKCWebViewPrivate;
class WKCWebView;

class ChromeClientWKC : public WebCore::ChromeClient
{
WTF_MAKE_FAST_ALLOCATED;
public:
    static ChromeClientWKC* create(WKCWebViewPrivate* view);
    virtual ~ChromeClientWKC() override final;

    void* webView() const;

    // callbacks
    virtual void chromeDestroyed() override final;

    virtual void setWindowRect(const WebCore::FloatRect&) override final;
    virtual WebCore::FloatRect windowRect() override final;

    virtual WebCore::FloatRect pageRect() override final;

    virtual void focus() override final;
    virtual void unfocus() override final;

    virtual bool canTakeFocus(WebCore::FocusDirection) override final;
    virtual void takeFocus(WebCore::FocusDirection) override final;

    virtual void focusedElementChanged(WebCore::Element*) override final;
    virtual void focusedFrameChanged(WebCore::Frame*) override final;

    virtual WebCore::Page* createWindow(WebCore::Frame&, const WebCore::FrameLoadRequest&, const WebCore::WindowFeatures&, const WebCore::NavigationAction&) override final;
    virtual void show() override final;

    virtual bool canRunModal() override final;
    virtual void runModal() override final;

    virtual void setToolbarsVisible(bool) override final;
    virtual bool toolbarsVisible() override final;

    virtual void setStatusbarVisible(bool) override final;
    virtual bool statusbarVisible() override final;

    virtual void setScrollbarsVisible(bool) override final;
    virtual bool scrollbarsVisible() override final;

    virtual void setMenubarVisible(bool) override final;
    virtual bool menubarVisible() override final;

    virtual void setResizable(bool) override final;

    virtual void addMessageToConsole(JSC::MessageSource, JSC::MessageLevel, const WTF::String& message, unsigned lineNumber, unsigned columnNumber, const WTF::String& sourceID) override final;

    virtual bool canRunBeforeUnloadConfirmPanel() override final;
    virtual bool runBeforeUnloadConfirmPanel(const WTF::String& message, WebCore::Frame&) override final;

    virtual void closeWindowSoon() override final;

    virtual void runJavaScriptAlert(WebCore::Frame&, const WTF::String&) override final;
    virtual bool runJavaScriptConfirm(WebCore::Frame&, const WTF::String&) override final;
    virtual bool runJavaScriptPrompt(WebCore::Frame&, const WTF::String& message, const WTF::String& defaultValue, WTF::String& result) override final;
    virtual void setStatusbarText(const WTF::String&) override final;
    virtual bool shouldInterruptJavaScript() override final;
    virtual WebCore::KeyboardUIMode keyboardUIMode() override final;

    virtual bool supportsImmediateInvalidation() override final;
    virtual void invalidateRootView(const WebCore::IntRect&) override final;
    virtual void invalidateContentsAndRootView(const WebCore::IntRect&) override final;
    virtual void invalidateContentsForSlowScroll(const WebCore::IntRect&) override final;
    virtual void scroll(const WebCore::IntSize&, const WebCore::IntRect&, const WebCore::IntRect&) override final;
#if USE(TILED_BACKING_STORE)
    virtual void delegatedScrollRequested(const WebCore::IntPoint&) override final;
#endif
    virtual WebCore::IntPoint screenToRootView(const WebCore::IntPoint&) const override final;
    virtual WebCore::IntRect rootViewToScreen(const WebCore::IntRect&) const override final;
    virtual WebCore::IntPoint accessibilityScreenToRootView(const WebCore::IntPoint&) const override final;
    virtual WebCore::IntRect rootViewToAccessibilityScreen(const WebCore::IntRect&) const override final;

    virtual void didFinishLoadingImageForElement(WebCore::HTMLImageElement&) override final;

    virtual PlatformPageClient platformPageClient() const override final;
    virtual void setCursor(const WebCore::Cursor&) override final;
    virtual void setCursorHiddenUntilMouseMoves(bool) override final;

    virtual void dispatchViewportPropertiesDidChange(const WebCore::ViewportArguments&) const override final;
    virtual void dispatchMetaFocusRingVisibilityDidChange(const WTF::String&) const override final;
    virtual void dispatchMetaInitialAutofocusDidChange(const WTF::String&) const override final;

    virtual void contentsSizeChanged(WebCore::Frame&, const WebCore::IntSize&) const override final;
    virtual void intrinsicContentsSizeChanged(const WebCore::IntSize&) const override final;
    virtual void scrollRectIntoView(const WebCore::IntRect&) const override final;

    virtual bool shouldUnavailablePluginMessageBeButton(WebCore::RenderEmbeddedObject::PluginUnavailabilityReason) const override final;
    virtual void unavailablePluginButtonClicked(WebCore::Element&, WebCore::RenderEmbeddedObject::PluginUnavailabilityReason) const override final;
    virtual void mouseDidMoveOverElement(const WebCore::HitTestResult&, unsigned modifierFlags, const WTF::String& toolTip, WebCore::TextDirection) override final;

    virtual void print(WebCore::Frame&) override final;

    virtual WebCore::Color underlayColor() const override final;

    virtual void exceededDatabaseQuota(WebCore::Frame&, const WTF::String& databaseName, WebCore::DatabaseDetails) override final;

    virtual void reachedMaxAppCacheSize(int64_t spaceNeeded) override final;

    virtual void reachedApplicationCacheOriginQuota(WebCore::SecurityOrigin&, int64_t totalSpaceNeeded) override final;

#if ENABLE(ORIENTATION_EVENTS)
    virtual int deviceOrientation() const override final;
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    virtual std::unique_ptr<WebCore::ColorChooser> createColorChooser(WebCore::ColorChooserClient&, const WebCore::Color&) override final;
#endif

    virtual void runOpenPanel(WebCore::Frame&, WebCore::FileChooser&) override final;
    virtual void loadIconForFiles(const WTF::Vector<WTF::String>&, WebCore::FileIconLoader&) override final;

    virtual void elementDidFocus(WebCore::Element&) override final;
    virtual void elementDidBlur(WebCore::Element&) override final;
    virtual void elementDidRefocus(WebCore::Element&) override final;
    
    virtual bool shouldPaintEntireContents() const override final;

    virtual WebCore::GraphicsLayerFactory* graphicsLayerFactory() const override final;

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    virtual RefPtr<WebCore::DisplayRefreshMonitor> createDisplayRefreshMonitor(WebCore::PlatformDisplayID) const override final;
#endif

    virtual void attachRootGraphicsLayer(WebCore::Frame&, WebCore::GraphicsLayer*) override final;
    virtual void attachViewOverlayGraphicsLayer(WebCore::GraphicsLayer*) override final;
    virtual void setNeedsOneShotDrawingSynchronization() override final;
    virtual void scheduleCompositingLayerFlush() override final;
    virtual bool allowsAcceleratedCompositing() const override final;

    virtual WebCore::ChromeClient::CompositingTriggerFlags allowedCompositingTriggers() const override final;
    
    virtual bool layerTreeStateIsFrozen() const override final;

    virtual void didAddHeaderLayer(WebCore::GraphicsLayer&) override final;
    virtual void didAddFooterLayer(WebCore::GraphicsLayer&) override final;

    virtual bool shouldDispatchFakeMouseMoveEvents() const override final;

    virtual bool supportsVideoFullscreen(WebCore::HTMLMediaElementEnums::VideoFullscreenMode) override final;
    virtual bool supportsVideoFullscreenStandby() override final;

#if ENABLE(VIDEO)
    virtual void enterVideoFullscreenForVideoElement(WebCore::HTMLVideoElement&, WebCore::HTMLMediaElementEnums::VideoFullscreenMode, bool standby) override final;
    virtual void setUpPlaybackControlsManager(WebCore::HTMLMediaElement&) override final {}
    virtual void clearPlaybackControlsManager() override final {}
#endif

    virtual void exitVideoFullscreenForVideoElement(WebCore::HTMLVideoElement&) override final;
    virtual void exitVideoFullscreenToModeWithoutAnimation(WebCore::HTMLVideoElement&, WebCore::HTMLMediaElementEnums::VideoFullscreenMode) override final;
    virtual bool requiresFullscreenForVideoPlayback() override final;

#if ENABLE(FULLSCREEN_API)
    virtual bool supportsFullScreenForElement(const WebCore::Element&, bool) override final;
    virtual void enterFullScreenForElement(WebCore::Element&) override final;
    virtual void exitFullScreenForElement(WebCore::Element*) override final;
    virtual void setRootFullScreenLayer(WebCore::GraphicsLayer*) override final;
#endif

#if USE(TILED_BACKING_STORE)
    virtual IntRect visibleRectForTiledBackingStore() const override final;
#endif

    virtual void enableSuddenTermination() override final;
    virtual void disableSuddenTermination() override final;

    virtual bool selectItemWritingDirectionIsNatural() override final;
    virtual bool selectItemAlignmentFollowsMenuWritingDirection() override final;
    virtual WTF::RefPtr<WebCore::PopupMenu> createPopupMenu(WebCore::PopupMenuClient&) const override final;
    virtual WTF::RefPtr<WebCore::SearchPopupMenu> createSearchPopupMenu(WebCore::PopupMenuClient&) const override final;

    virtual void postAccessibilityNotification(WebCore::AccessibilityObject&, WebCore::AXObjectCache::AXNotification) override final;

    virtual void notifyScrollerThumbIsVisibleInRect(const WebCore::IntRect&) override final;
    virtual void recommendedScrollbarStyleDidChange(WebCore::ScrollbarStyle /*newStyle*/) override final;

    virtual void wheelEventHandlersChanged(bool) override final;
        
    virtual bool isSVGImageChromeClient() const override final;

#if ENABLE(POINTER_LOCK)
    virtual bool requestPointerLock() override final;
    virtual void requestPointerUnlock() override final;
    virtual bool isPointerLocked() override final;
#endif

    virtual WebCore::FloatSize minimumWindowSize() const override final;

    virtual bool isEmptyChromeClient() const override final;

    virtual WTF::String plugInStartLabelTitle(const WTF::String& mimeType) const override final;
    virtual WTF::String plugInStartLabelSubtitle(const WTF::String& mimeType) const override final;
    virtual WTF::String plugInExtraStyleSheet() const override final;
    virtual WTF::String plugInExtraScript() const override final;

    virtual void didAssociateFormControls(const WTF::Vector<WTF::RefPtr<WebCore::Element> >&, WebCore::Frame&) override final;
    virtual bool shouldNotifyOnFormChanges() override final;

    virtual RefPtr<WebCore::Icon> createIconForFiles(const Vector<WTF::String>&) override final;

private:
    ChromeClientWKC(WKCWebViewPrivate* view);
    bool construct();
    WKCWebView* wkcWebView() const;

private:
    WKCWebViewPrivate* m_view;
    WKC::ChromeClientIf* m_appClient;
    URL m_hHoveredLinkURL;
};

} // namespace

#endif // ChromeClientWKC_h

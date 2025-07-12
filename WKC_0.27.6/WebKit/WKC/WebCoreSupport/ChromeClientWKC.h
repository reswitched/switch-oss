/*
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (c) 2010-2016 ACCESS CO., LTD. All rights reserved.
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
    virtual ~ChromeClientWKC() override;

    void* webView() const;

    // callbacks
    virtual void chromeDestroyed() override;

    virtual void setWindowRect(const WebCore::FloatRect&) override;
    virtual WebCore::FloatRect windowRect() override;

    virtual WebCore::FloatRect pageRect() override;

    virtual void focus() override;
    virtual void unfocus() override;

    virtual bool canTakeFocus(WebCore::FocusDirection) override;
    virtual void takeFocus(WebCore::FocusDirection) override;

    virtual void focusedElementChanged(WebCore::Element*) override;
    virtual void focusedFrameChanged(WebCore::Frame*) override;

    virtual WebCore::Page* createWindow(WebCore::Frame*, const WebCore::FrameLoadRequest&, const WebCore::WindowFeatures&, const WebCore::NavigationAction&) override;
    virtual void show() override;

    virtual bool canRunModal() override;
    virtual void runModal() override;

    virtual void setToolbarsVisible(bool) override;
    virtual bool toolbarsVisible() override;

    virtual void setStatusbarVisible(bool) override;
    virtual bool statusbarVisible() override;

    virtual void setScrollbarsVisible(bool) override;
    virtual bool scrollbarsVisible() override;

    virtual void setMenubarVisible(bool) override;
    virtual bool menubarVisible() override;

    virtual void setResizable(bool) override;

    virtual void addMessageToConsole(JSC::MessageSource, JSC::MessageLevel, const WTF::String& message, unsigned lineNumber, unsigned columnNumber, const WTF::String& sourceID) override;

    virtual bool canRunBeforeUnloadConfirmPanel() override;
    virtual bool runBeforeUnloadConfirmPanel(const WTF::String& message, WebCore::Frame*) override;

    virtual void closeWindowSoon() override;

    virtual void runJavaScriptAlert(WebCore::Frame*, const WTF::String&) override;
    virtual bool runJavaScriptConfirm(WebCore::Frame*, const WTF::String&) override;
    virtual bool runJavaScriptPrompt(WebCore::Frame*, const WTF::String& message, const WTF::String& defaultValue, WTF::String& result) override;
    virtual void setStatusbarText(const WTF::String&) override;
    virtual bool shouldInterruptJavaScript() override;
    virtual WebCore::KeyboardUIMode keyboardUIMode() override;

    virtual bool supportsImmediateInvalidation() override;
    virtual void invalidateRootView(const WebCore::IntRect&) override;
    virtual void invalidateContentsAndRootView(const WebCore::IntRect&) override;
    virtual void invalidateContentsForSlowScroll(const WebCore::IntRect&) override;
    virtual void scroll(const WebCore::IntSize&, const WebCore::IntRect&, const WebCore::IntRect&) override;
#if USE(TILED_BACKING_STORE)
    virtual void delegatedScrollRequested(const WebCore::IntPoint&) override;
#endif
    virtual WebCore::IntPoint screenToRootView(const WebCore::IntPoint&) const override;
    virtual WebCore::IntRect rootViewToScreen(const WebCore::IntRect&) const override;
    virtual PlatformPageClient platformPageClient() const override;
    virtual void scrollbarsModeDidChange() const override;
    virtual void setCursor(const WebCore::Cursor&) override;
    virtual void setCursorHiddenUntilMouseMoves(bool) override;
#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
    virtual void scheduleAnimation() override;
#endif

    virtual void dispatchViewportPropertiesDidChange(const WebCore::ViewportArguments&) const override;
    virtual void dispatchMetaFocusRingVisibilityDidChange(const WTF::String&) const override;

    virtual void contentsSizeChanged(WebCore::Frame*, const WebCore::IntSize&) const override;
    virtual void scrollRectIntoView(const WebCore::IntRect&) const override;

    virtual bool shouldUnavailablePluginMessageBeButton(WebCore::RenderEmbeddedObject::PluginUnavailabilityReason) const override;
    virtual void unavailablePluginButtonClicked(WebCore::Element*, WebCore::RenderEmbeddedObject::PluginUnavailabilityReason) const override;
    virtual void mouseDidMoveOverElement(const WebCore::HitTestResult&, unsigned modifierFlags) override;

    virtual void setToolTip(const WTF::String&, WebCore::TextDirection) override;

    virtual void print(WebCore::Frame*) override;

    virtual WebCore::Color underlayColor() const override;

    virtual void exceededDatabaseQuota(WebCore::Frame*, const WTF::String& databaseName, WebCore::DatabaseDetails) override;

    virtual void reachedMaxAppCacheSize(int64_t spaceNeeded) override;

    virtual void reachedApplicationCacheOriginQuota(WebCore::SecurityOrigin*, int64_t totalSpaceNeeded) override;

#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(DRAGGABLE_REGION)
    virtual void annotatedRegionsChanged() override;
#endif
            
    virtual bool shouldReplaceWithGeneratedFileForUpload(const WTF::String& path, WTF::String& generatedFilename) override;
    virtual WTF::String generateReplacementFile(const WTF::String& path) override;

#if ENABLE(ORIENTATION_EVENTS)
    virtual int deviceOrientation() const override;
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    virtual std::unique_ptr<WebCore::ColorChooser> createColorChooser(WebCore::ColorChooserClient*, const WebCore::Color&) override;
#endif

    virtual void runOpenPanel(WebCore::Frame*, WTF::PassRefPtr<WebCore::FileChooser>) override;
    virtual void loadIconForFiles(const WTF::Vector<WTF::String>&, WebCore::FileIconLoader*) override;

    virtual void elementDidFocus(const WebCore::Node*) override;
    virtual void elementDidBlur(const WebCore::Node*) override;
    
    virtual bool shouldPaintEntireContents() const override;

    virtual WebCore::GraphicsLayerFactory* graphicsLayerFactory() const override;

    virtual void attachRootGraphicsLayer(WebCore::Frame*, WebCore::GraphicsLayer*) override;
    virtual void attachViewOverlayGraphicsLayer(WebCore::Frame*, WebCore::GraphicsLayer*) override;
    virtual void setNeedsOneShotDrawingSynchronization() override;
    virtual void scheduleCompositingLayerFlush() override;
    virtual bool allowsAcceleratedCompositing() const override;

    virtual WebCore::ChromeClient::CompositingTriggerFlags allowedCompositingTriggers() const override;
    
    virtual bool layerTreeStateIsFrozen() const override;

    virtual void didAddHeaderLayer(WebCore::GraphicsLayer*) override;
    virtual void didAddFooterLayer(WebCore::GraphicsLayer*) override;

    virtual bool shouldDispatchFakeMouseMoveEvents() const override;

    virtual bool requiresFullscreenForVideoPlayback() override;

#if ENABLE(FULLSCREEN_API)
    virtual bool supportsFullScreenForElement(const WebCore::Element*, bool) override;
    virtual void enterFullScreenForElement(WebCore::Element*) override;
    virtual void exitFullScreenForElement(WebCore::Element*) override;
    virtual void setRootFullScreenLayer(WebCore::GraphicsLayer*) override;
#endif

#if USE(TILED_BACKING_STORE)
    virtual IntRect visibleRectForTiledBackingStore() const override;
#endif

    virtual void enableSuddenTermination() override;
    virtual void disableSuddenTermination() override;

#if ENABLE(TOUCH_EVENTS)
    virtual void needTouchEvents(bool) override;
#endif

    virtual bool selectItemWritingDirectionIsNatural() override;
    virtual bool selectItemAlignmentFollowsMenuWritingDirection() override;
    virtual bool hasOpenedPopup() const override;
    virtual WTF::PassRefPtr<WebCore::PopupMenu> createPopupMenu(WebCore::PopupMenuClient*) const override;
    virtual WTF::PassRefPtr<WebCore::SearchPopupMenu> createSearchPopupMenu(WebCore::PopupMenuClient*) const override;

    virtual void postAccessibilityNotification(WebCore::AccessibilityObject*, WebCore::AXObjectCache::AXNotification) override;

    virtual void notifyScrollerThumbIsVisibleInRect(const WebCore::IntRect&) override;
    virtual void recommendedScrollbarStyleDidChange(WebCore::ScrollbarStyle /*newStyle*/) override;

    virtual void wheelEventHandlersChanged(bool) override;
        
    virtual bool isSVGImageChromeClient() const override;

#if ENABLE(POINTER_LOCK)
    virtual bool requestPointerLock() override;
    virtual void requestPointerUnlock() override;
    virtual bool isPointerLocked() override;
#endif

    virtual WebCore::FloatSize minimumWindowSize() const override;

    virtual bool isEmptyChromeClient() const override;

    virtual WTF::String plugInStartLabelTitle(const WTF::String& mimeType) const override;
    virtual WTF::String plugInStartLabelSubtitle(const WTF::String& mimeType) const override;
    virtual WTF::String plugInExtraStyleSheet() const override;
    virtual WTF::String plugInExtraScript() const override;

    virtual void didAssociateFormControls(const WTF::Vector<WTF::RefPtr<WebCore::Element> >&) override;
    virtual bool shouldNotifyOnFormChanges() override;

private:
    ChromeClientWKC(WKCWebViewPrivate* view);
    bool construct();
    WKCWebView* wkcWebView() const;

private:
    WKCWebViewPrivate* m_view;
    WKC::ChromeClientIf* m_appClient;
    WebCore::URL m_hHoveredLinkURL;
};

} // namespace

#endif // ChromeClientWKC_h

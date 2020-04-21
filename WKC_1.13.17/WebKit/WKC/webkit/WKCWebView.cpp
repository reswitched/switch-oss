/*
    WKCWebView.cpp
    Copyright (C) 2007, 2008 Holger Hans Peter Freyther
    Copyright (C) 2007, 2008, 2009 Christian Dywan <christian@imendio.com>
    Copyright (C) 2007 Xan Lopez <xan@gnome.org>
    Copyright (C) 2007, 2008 Alp Toker <alp@atoker.com>
    Copyright (C) 2008 Jan Alonzo <jmalonzo@unpluggable.com>
    Copyright (C) 2008 Gustavo Noronha Silva <gns@gnome.org>
    Copyright (C) 2008 Nuanti Ltd.
    Copyright (C) 2008, 2009 Collabora Ltd.
    Copyright (C) 2009 Igalia S.L.
    Copyright (C) 2009 Movial Creative Technologies Inc.
    Copyright (C) 2009 Bobby Powers
    Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.h"

#include "WKCWebView.h"
#include "WKCWebViewPrivate.h"
#include "WKCWebFrame.h"
#include "WKCWebFramePrivate.h"
#include "WKCPlatformEvents.h"
#include "WKCClientBuilders.h"
#include "WKCWebViewPrefs.h"
#include "WKCPrefs.h"
#include "WKCMemoryInfo.h"
#include "WKCPlatformStrategies.h"

#include "ChromeClientWKC.h"
#if ENABLE(CONTEXT_MENUS)
#include "ContextMenuClientWKC.h"
#endif
#include "BackForwardClientWKC.h"
#include "EditorClientWKC.h"
#include "DragClientWKC.h"
#include "FrameLoaderClientWKC.h"
#include "InspectorClientWKC.h"
#include "DropDownListClientWKC.h"
#include "GeolocationClientWKC.h"
#include "ProgressTrackerClientWKC.h"
#if ENABLE(REMOTE_INSPECTOR)
#include "WebInspectorServer.h"
#include "InspectorController.h"
#include "InspectorServerClientWKC.h"
#endif
#if ENABLE(DEVICE_ORIENTATION)
#include "DeviceMotionClientWKC.h"
#include "DeviceOrientationClientWKC.h"
#endif
#if ENABLE(MEDIA_STREAM)
#include "UserMediaClientImplWKC.h"
#endif
#if ENABLE(NOTIFICATIONS)
#include "NotificationClientWKC.h"
#endif
#if ENABLE(NAVIGATOR_CONTENT_UTILS)
#include "NavigatorContentUtilsClientWKC.h"
#endif
#if ENABLE(WKC_WEB_NFC)
#include "WebNfcClientWKC.h"
#endif
#include "DatabaseProviderWKC.h"
#include "StorageNamespaceProviderWKC.h"
#include "VisitedLinkStoreWKC.h"
#if ENABLE(GAMEPAD)
#include "GamepadProvider.h"
#include "GamepadProviderWKC.h"
#endif
#include "RenderTreeAsText.h"

#include <wkc/wkcpeer.h>
#include <wkc/wkcfilepeer.h>
#include <wkc/wkcpasteboardpeer.h>
#include <wkc/wkcgpeer.h>
#include <wkc/wkcmpeer.h>
#include <wkc/wkcmediapeer.h>
#include <wkc/wkcpluginpeer.h>
#include <wkc/wkcglpeer.h>
#include <wkc/wkcsocket.h>

#include "WKCOverlayPrivate.h"

#include "helpers/WKCFrame.h"
#include "helpers/WKCHistoryItem.h"
#include "helpers/WKCNode.h"
#include "helpers/WKCSettings.h"
#include "helpers/privates/WKCElementPrivate.h"
#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/privates/WKCHelpersEnumsPrivate.h"
#include "helpers/privates/WKCHistoryItemPrivate.h"
#include "helpers/privates/WKCResourceHandlePrivate.h"
#include "helpers/privates/WKCNodePrivate.h"
#include "helpers/privates/WKCPagePrivate.h"
#include "helpers/privates/WKCEventHandlerPrivate.h"
#include "helpers/privates/WKCFocusControllerPrivate.h"
#include "helpers/privates/WKCPluginDatabasePrivate.h"
#include "helpers/privates/WKCHitTestResultPrivate.h"
#include "helpers/privates/WKCBackForwardListPrivate.h"

#include "Base64.h"
#include "CacheStorageProvider.h"
#include "Chrome.h"
#include "CommonVM.h"
#include "Cookie.h"
#include "DisplayRefreshMonitorManager.h"
#include "Page.h"
#include "PageConfiguration.h"
#include "PageGroup.h"
#include "Editor.h"
#include "EmptyClients.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameView.h"
#include "NodeList.h"
#include "HTMLCollection.h"
#include "HTMLElement.h"
#include "HTMLLinkElement.h"
#include "HistoryItem.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "ImageBufferData.h"
#include "ImageWKC.h"
#include "LibWebRTCProvider.h"
#include "LogInitialization.h"
#include "RenderView.h"
#include "RenderText.h"
#include "FocusController.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMouseEvent.h"
#include "PlatformTouchEvent.h"
#include "PlatformWheelEvent.h"
#include "ScriptController.h"
#include "StringImpl.h"
#include "DocumentLoader.h"
#include "ProgressTracker.h"
#include "HTMLAreaElement.h"
#include "HTMLInputElement.h"
#include "PageCache.h"
#include "TextEncodingRegistry.h"
#include "Settings.h"
#include "FloatRect.h"
#include "MemoryCache.h"
#include "Text.h"
#include "UserGestureIndicator.h"
#include "RenderTextControl.h"
#include "SocketProvider.h"
#include "SpatialNavigation.h"
#include "MouseEventWithHitTestResults.h"
#include "HTMLImageElement.h"
#include "BackForwardController.h"
#include "shadow/TextControlInnerElements.h"
#include "UserContentController.h"
#include "VisitedLinkStore.h"
#include "WebKitLegacy/Storage/WebStorageNamespaceProvider.h"
#include "SVGDocumentExtensions.h"
#include "VM.h"
#include "Watchdog.h"
#include "WorkQueue.h"

#include "InitializeThreading.h"
#include "PageGroup.h"
#include "Pasteboard.h"

#include "ImageSource.h"

#include "CookieJar.h"

#include "CSSValuePool.h"
#include "RenderLayer.h"
#include "FontCache.h"
#include "FontCascade.h"
#include "CrossOriginPreflightResultCache.h"
#include "ResourceHandleManagerWKC.h"
#include "MainThreadSharedTimer.h"
#include "GCController.h"
#include "SQLiteFileSystem.h"
#include "WorkerThread.h"

#include "StorageNamespace.h"
#include "Storage.h"
#include "StorageArea.h"

#include "RenderFrameSet.h"
#include "BitmapImage.h"

#include "Modules/websockets/WebSocket.h"

#include "RuntimeEnabledFeatures.h"

#include "shadow/SliderThumbElement.h"

#if ENABLE(CONTENT_EXTENSIONS)
#include "ContentExtensionCompiler.h"
#endif

#include <wkc/wkcheappeer.h>
#include "FastMalloc.h"

#include "NotImplemented.h"

#ifdef __MINGW32__
# ifdef LoadString
#  undef LoadString
# endif
#endif // __MINGW32__

#if USE(MUTEX_DEBUG_LOG)
CRITICAL_SECTION gCriticalSection;
static bool gCriticalSectionFlag = false;
#endif

namespace WebCore {
extern WebCore::IntRect scrollbarTrackRect(Scrollbar& scrollbar);
extern void scrollbarSplitTrack(Scrollbar& scrollbar, const WebCore::IntRect& track, WebCore::IntRect& startTrack, WebCore::IntRect& thumb, WebCore::IntRect& endTrack);
extern WebCore::IntRect scrollbarBackButtonRect(Scrollbar& scrollbar, ScrollbarPart part);
extern WebCore::IntRect scrollbarForwardButtonRect(Scrollbar& scrollbar, ScrollbarPart part);
extern void EventLoop_setCycleProc(bool(*)(void*), void*);
extern void initializeGamepads(int pads);
extern bool notifyGamepadEvent(int index, const WTF::String& id, long long timestamp, int naxes, const float* axes, int nbuttons, const float* buttons);
}

namespace WTF {
extern void finalizeMainThreadPlatform();
}

namespace JSC {
class JSGlobalData;
}

extern "C" {
void wkc_libxml2_resetVariables(void);
void wkc_libxslt_resetVariables(void);
void wkc_sqlite3_force_terminate(void);
void wkc_pixman_resetVariables(void);
cairo_public void wkc_cairo_resetVariables(void);
WKC_PEER_API int wkcFontEngineRegisterSystemFontPeer(int in_type, const unsigned char* in_data, unsigned int in_len);
#if ENABLE(GRAPHICS_CONTEXT_3D)
void ShResetVariables();
#endif
}

using namespace WTF;

#if 0
extern "C" void GUI_Printf(const char*, ...);
# define NX_DP(a) GUI_Printf a
#else
# define NX_DP(a) (void(0))
#endif

#if COMPILER(MSVC) && defined(_DEBUG)
#if 0
extern "C" void GUI_Printf(const char*, ...);
# define NX_MEM_DP LOG_ERROR
#else
# define NX_MEM_DP(...) ((void)0)
#endif
#else
# define NX_MEM_DP(...) ((void)0)
#endif /* COMPILER(MSVC) && defined(_DEBUG) */

namespace WKC {

WKC_WEB_VIEW_COMPILE_ASSERT(sizeof(WKCSocketStatistics) == sizeof(SocketStatistics), sizeof__WKCSocketStatistics);

WKC_DEFINE_GLOBAL_TYPE_ZERO(WTF::Vector<WKCWebViewPrivate*>*, gValidViews);

// private container
WKCWebViewPrivate::WKCWebViewPrivate(WKCWebView* parent, WKCClientBuilders& builders)
     : m_parent(parent),
       m_clientBuilders(builders)
{
    m_corePage = 0;
    m_wkcCorePage = 0;
    m_mainFrame = 0;
    m_inspector = 0;
    m_dropdownlist = 0;
    m_settings = 0;
    m_devicemotion = 0;
    m_deviceorientation = 0;
    m_notification = 0;
    m_progress = 0;

    m_offscreen = 0;
    m_drawContext = 0;
#if USE(WKC_CAIRO)
    m_offscreenFormat = 0;
    m_offscreenBitmap = 0;
    m_offscreenRowBytes = 0;
    m_offscreenSize.fWidth = 0;
    m_offscreenSize.fHeight = 0;
#endif

    m_isZoomFullContent = true;
    m_isTransparent = false;
    m_loadStatus = ELoadStatusNone;

    m_opticalZoomLevel = 1.f;
    WKCFloatPoint_Set(&m_opticalZoomOffset, 0, 0);

    m_encoding = 0;
    m_customEncoding = 0;

    m_focusedElement = 0;
    m_elementFromPoint = 0;

#if ENABLE(REMOTE_INSPECTOR)
    m_inspectorServerClient = 0;
    m_inspectorIsEnabled = false;
#endif

    m_editable = true;

    m_forceTerminated = false;

    m_activityState = WebCore::ActivityState::NoFlags;

    if (!gValidViews)
        gValidViews = new WTF::Vector<WKCWebViewPrivate*>();
    gValidViews->append(this);

    m_rootGraphicsLayer = 0;

    m_autoPlayIsEnabled = false;
}

WKCWebViewPrivate::~WKCWebViewPrivate()
{
    size_t pos = gValidViews->find(this);
    if (pos!=WTF::notFound) {
        gValidViews->remove(pos);
    }

    if (m_forceTerminated) {
        return;
    }

    delete m_elementFromPoint;

    if (m_offscreen) {
        wkcOffscreenDeletePeer(m_offscreen);
    }
    m_offscreen = 0;
    if (m_drawContext) {
        wkcDrawContextDeletePeer(m_drawContext);
    }
    m_drawContext = 0;

    if (m_encoding) {
        WTF::fastFree(m_encoding);
        m_encoding = 0;
    }
    if (m_customEncoding) {
        WTF::fastFree(m_customEncoding);
        m_customEncoding = 0;
    }

    if (m_corePage) {
        m_corePage->settings().setUsesPageCache(false);
        m_mainFrame->privateFrame()->core()->loader().detachFromParent();
    }
    if (m_wkcCorePage) {
        delete m_wkcCorePage;
    }
    if (m_corePage) {
        delete m_corePage;
        m_corePage = 0;
    }
    if (m_dropdownlist) {
        delete m_dropdownlist;
        m_dropdownlist = 0;
    }

    delete m_focusedElement;
    delete m_progress;

#if ENABLE(NAVIGATOR_CONTENT_UTILS)
//    delete m_navigatorcontentutils;
    m_navigatorcontentutils = 0;
#endif
#if ENABLE(NOTIFICATIONS)
    delete m_notification;
    m_notification = 0;
#endif
#if ENABLE(DEVICE_ORIENTATION)
    delete m_deviceorientation;
    delete m_devicemotion;
    m_deviceorientation = 0;
    m_devicemotion = 0;
#endif
    // m_mainFrame will be deleted automatically...

#if ENABLE(REMOTE_INSPECTOR)
    if (m_inspectorServerClient) {
        if (m_inspector) {
            m_inspector->setInspectorServerClient(0);
            m_inspector = 0; // it may be deleted by Page
        }
        delete m_inspectorServerClient;
        m_inspectorServerClient = 0;
    }
#endif

    if (m_settings) {
        m_settings->~WKCSettings();
        WTF::fastFree((void *)m_settings);
        m_settings = 0;
    }
    if (WebCore::ResourceHandleManager::isExistSharedInstance()) {
        WebCore::ResourceHandleManager::sharedInstance()->suspendNetworkThread();
    }
}

bool
WKCWebViewPrivate::isValidView(WKCWebViewPrivate* view)
{
    if (!gValidViews)
        return false;
    return gValidViews->find(view) != WTF::notFound;
}

WKCWebViewPrivate*
WKCWebViewPrivate::create(WKCWebView* parent, WKCClientBuilders& builders)
{
    WKCWebViewPrivate* self = 0;
    self = new WKCWebViewPrivate(parent, builders);
    if (!self) return 0;
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
WKCWebViewPrivate::construct()
{
    WKC::ChromeClientWKC* chrome = 0;
    WKC::BackForwardClientWKC* backforward = 0;
#if ENABLE(CONTEXT_MENUS)
    WKC::ContextMenuClientWKC* contextmenu = 0;
#endif
    WKC::EditorClientWKC* editor = 0;
    WKC::DragClientWKC* drag = 0;
#if ENABLE(GEOLOCATION)
    WKC::GeolocationClientWKC* geolocation = 0;
#endif
#if ENABLE(MEDIA_STREAM)
    WKC::UserMediaClientWKC* usermedia = 0;
#endif
#if ENABLE(WKC_WEB_NFC)
    WKC::WebNfcClientWKC* webNfcClient = 0;
#endif
    WKC::FrameLoaderClientWKC* mainframeloaderclient = 0;
    WebCore::Settings* settings = 0;

    WebCore::PageConfiguration cli(WebCore::createEmptyEditorClient(), WebCore::SocketProvider::create(), WebCore::LibWebRTCProvider::create(), WebCore::CacheStorageProvider::create());

    chrome = WKC::ChromeClientWKC::create(this);
    if (!chrome) goto error_end;
    backforward = WKC::BackForwardClientWKC::create(this);
    if (!backforward) goto error_end;
#if ENABLE(CONTEXT_MENUS)
    contextmenu = WKC::ContextMenuClientWKC::create(this);
    if (!contextmenu) goto error_end;
#endif
    editor = WKC::EditorClientWKC::create(this);
    if (!editor) goto error_end;
    drag = WKC::DragClientWKC::create(this);
    if (!drag) goto error_end;
#if ENABLE(GEOLOCATION)
    geolocation = WKC::GeolocationClientWKC::create(this);
    if (!geolocation) goto error_end;
#endif
#if ENABLE(DEVICE_ORIENTATION)
    m_devicemotion = WKC::DeviceMotionClientWKC::create(this);
    if (!m_devicemotion) goto error_end;
    m_deviceorientation = WKC::DeviceOrientationClientWKC::create(this);
    if (!m_deviceorientation) goto error_end;
#endif
#if ENABLE(MEDIA_STREAM)
    usermedia = WKC::UserMediaClientWKC::create(this);
    if (!usermedia) goto error_end;
#endif
#if ENABLE(NOTIFICATIONS)
    m_notification = WKC::NotificationClientWKC::create(this);
    if (!m_notification) goto error_end;
#endif
#if ENABLE(NAVIGATOR_CONTENT_UTILS)
    m_navigatorcontentutils = WKC::NavigatorContentUtilsClientWKC::create(this);
    if (!m_navigatorcontentutils) goto error_end;
#endif
#if ENABLE(WKC_WEB_NFC)
    webNfcClient = WKC::WebNfcClientWKC::create(this);
    if (!webNfcClient) goto error_end;
#endif
    m_progress = WKC::ProgressTrackerClientWKC::create(this);
    if (!m_progress) goto error_end;

    mainframeloaderclient = WKC::FrameLoaderClientWKC::create();
    if (!mainframeloaderclient) goto error_end;

    m_inspector = WKC::InspectorClientWKC::create(this);
    if (!m_inspector) goto error_end;

    WebCore::fillWithEmptyClients(cli);
    cli.chromeClient = chrome;
    cli.backForwardClient = adoptRef(*backforward);
#if ENABLE(CONTEXT_MENUS)
    cli.contextMenuClient = contextmenu;
#endif
    cli.editorClient = UniqueRef<EditorClientWKC>(*editor);
    cli.dragClient = drag;
    cli.inspectorClient = m_inspector;
    cli.progressTrackerClient = m_progress;
    cli.loaderClientForMainFrame = mainframeloaderclient;
    cli.alternativeTextClient = nullptr;
    cli.databaseProvider = WKC::DatabaseProviderWKC::create(this);
    cli.storageNamespaceProvider = WKC::StoragenamespaceProviderWKC::create(this);
    cli.userContentProvider = WebCore::UserContentController::create();
    cli.visitedLinkStore = WKC::VisitedLinkStoreWKC::create(this);

    m_corePage = new WebCore::Page(WTFMove(cli));
    if (!m_corePage) goto error_end;
    m_wkcCorePage = new PagePrivate(m_corePage);
#if ENABLE(GEOLOCATION)
    provideGeolocationTo(m_corePage, geolocation);
#endif
#if ENABLE(DEVICE_ORIENTATION)
    provideDeviceMotionTo(m_corePage, m_devicemotion);
    provideDeviceOrientationTo(m_corePage, m_deviceorientation);
#endif
#if ENABLE(MEDIA_STREAM)
    provideUserMediaTo(m_corePage, usermedia);
#endif
#if ENABLE(NOTIFICATIONS)
    provideNotification(m_corePage, m_notification);
#endif
#if ENABLE(NAVIGATOR_CONTENT_UTILS)
    provideNavigatorContentUtilsTo(m_corePage, std::unique_ptr<WebCore::NavigatorContentUtilsClient>(m_navigatorcontentutils));
#endif
#if ENABLE(WKC_WEB_NFC)
    provideWebNfcTo(m_corePage, webNfcClient);
#endif

    m_corePage->addLayoutMilestones(WebCore::DidFirstLayout | WebCore::DidFirstVisuallyNonEmptyLayout);
    m_corePage->setIsPrerender();

    m_mainFrame = WKC::WKCWebFrame::create(this, m_clientBuilders, 0, true);
    if (!m_mainFrame) goto error_end;
    if (!mainframeloaderclient->setMainFrame(m_mainFrame->privateFrame()))
        goto error_end;
    m_mainFrame->privateFrame()->core()->tree().setName("");
    m_mainFrame->privateFrame()->core()->init();

    m_dropdownlist = WKC::DropDownListClientWKC::create(this);
    if (!m_dropdownlist) goto error_end;

    settings = &m_corePage->settings();

    m_settings = (WKC::WKCSettings *)WTF::fastMalloc(sizeof(WKC::WKCSettings));
    m_settings = new (m_settings) WKC::WKCSettings(this);
    if (!m_settings) goto error_end;

    settings->setDefaultTextEncodingName("UTF-8");
    settings->setSerifFontFamily("Times New Roman");
    settings->setFixedFontFamily("Courier New");
    settings->setSansSerifFontFamily("Arial");
    settings->setStandardFontFamily("Times New Roman");
    settings->setLoadsImagesAutomatically(true);
    settings->setShrinksStandaloneImagesToFit(true);
    settings->setShouldPrintBackgrounds(true);
    settings->setScriptEnabled(true);
    settings->setSpatialNavigationEnabled(true);
    settings->setImagesEnabled(true);
    settings->setMediaEnabled(true);
    settings->setPluginsEnabled(false);
    settings->setLocalStorageEnabled(false);
    settings->setTextAreasAreResizable(true);
    settings->setUserStyleSheetLocation(WebCore::URL());
    settings->setDeveloperExtrasEnabled(false);
    settings->setCaretBrowsingEnabled(false);
    settings->setLocalStorageEnabled(false);
    settings->setXSSAuditorEnabled(false);
    settings->setJavaScriptCanOpenWindowsAutomatically(false);
    settings->setJavaScriptCanAccessClipboard(false);
    settings->setOfflineWebApplicationCacheEnabled(false);
    settings->setAllowUniversalAccessFromFileURLs(false);
    settings->setDOMPasteAllowed(false);
    settings->setNeedsSiteSpecificQuirks(false);
    settings->setUsesPageCache(false);
    settings->setDefaultFixedFontSize(14);
    settings->setDefaultFontSize(14);
    settings->setDownloadableBinaryFontsEnabled(true);
    settings->setShowDebugBorders(false);
    settings->setAuthorAndUserStylesEnabled(true);
//    settings->setAllowScriptsToCloseWindows(true);
    settings->setDNSPrefetchingEnabled(true);
    settings->setFixedBackgroundsPaintRelativeToDocument(true);

    settings->setCanvasUsesAcceleratedDrawing(false);
    settings->setAcceleratedDrawingEnabled(false);
    settings->setAcceleratedFiltersEnabled(false);

    settings->setAcceleratedCompositingEnabled(false);
    settings->setAcceleratedCompositingForFixedPositionEnabled(false);
    settings->setAcceleratedCompositingForOverflowScrollEnabled(false);

    settings->setExperimentalNotificationsEnabled(false);

    settings->setWebGLEnabled(true);
    settings->setWebGLErrorsToConsoleEnabled(true);

    settings->setWebAudioEnabled(false);

    settings->setAccelerated2dCanvasEnabled(true);
    settings->setMinimumAccelerated2dCanvasSize(64);
    
    settings->setLoadDeferringEnabled(true);
    settings->setPaginateDuringLayoutEnabled(true);

#if ENABLE(FULLSCREEN_API)
    settings->setFullScreenEnabled(true);
#endif

    settings->setAsynchronousSpellCheckingEnabled(false);

    settings->setUsePreHTML5ParserQuirks(false);

    settings->setForceCompositingMode(false);

    settings->setAllowDisplayOfInsecureContent(false);
    settings->setAllowRunningOfInsecureContent(false);

    settings->setMaximumHTMLParserDOMTreeDepth(WebCore::Settings::defaultMaximumHTMLParserDOMTreeDepth);

    settings->setHyperlinkAuditingEnabled(true);
    settings->setCrossOriginCheckInGetMatchedCSSRulesDisabled(true);

    settings->setLayoutFallbackWidth(1024);
    settings->setWebSecurityEnabled(true);
    settings->setXSSAuditorEnabled(true);

    settings->setHiddenPageCSSAnimationSuspensionEnabled(true);
    settings->setHiddenPageDOMTimerThrottlingEnabled(false);

    settings->setVideoPlaybackRequiresUserGesture(true);

    settings->setAnimatedImageAsyncDecodingEnabled(true);
    settings->setLargeImageAsyncDecodingEnabled(false);

#if ENABLE(VIDEO_TRACK)
     settings->setShouldDisplaySubtitles(true);
     settings->setShouldDisplayCaptions(true);
     settings->setShouldDisplayTextDescriptions(true);
#endif

#if ENABLE(TEXT_AUTOSIZING)
     settings->setTextAutosizingEnabled(true);
     settings->setTextAutosizingFontScaleFactor(1.f);
#endif

#if ENABLE(INDEXED_DATABASE)
     WebCore::RuntimeEnabledFeatures::sharedFeatures().setWebkitIndexedDBEnabled(true);
#endif
#if ENABLE(MEDIA_STREAM)
     WebCore::RuntimeEnabledFeatures::sharedFeatures().setMediaStreamEnabled(true);
#endif
#if ENABLE(GAMEPAD)
     WebCore::RuntimeEnabledFeatures::sharedFeatures().setGamepadsEnabled(true);
#endif

#if ENABLE(SERVICE_CONTROLS)
    settings->setImageControlsEnabled(true);
#endif

#if ENABLE(REMOTE_INSPECTOR)
    if (WebInspectorServer::sharedInstance()) {
        m_inspectorServerClient = WKC::InspectorServerClientWKC::create(this);
        m_inspectorServerClient->disableRemoteInspection();
        m_inspector->setInspectorServerClient(m_inspectorServerClient);
    }
#endif

     WebCore::RuntimeEnabledFeatures::sharedFeatures().setResourceTimingEnabled(true);
     WebCore::RuntimeEnabledFeatures::sharedFeatures().setUserTimingEnabled(true);

    m_corePage->setSessionID(PAL::SessionID::defaultSessionID());
    m_corePage->mainFrame().view()->setClipsRepaints(WKCWebView::clipsRepaints());

    WebCore::RuntimeEnabledFeatures::sharedFeatures().setFetchAPIKeepAliveEnabled(true);

    WebCore::RuntimeEnabledFeatures::sharedFeatures().setWebAnimationsCSSIntegrationEnabled(true);

    WebCore::RuntimeEnabledFeatures::sharedFeatures().setInteractiveFormValidationEnabled(true);

    return true;

error_end:
    if (m_settings) {
        m_settings->~WKCSettings();
        WTF::fastFree((void *)m_settings);
        m_settings = 0;
    }
    if (m_dropdownlist) {
        delete m_dropdownlist;
        m_dropdownlist = 0;
    }
    if (m_corePage) {
        delete m_corePage;
        m_corePage = 0;
        // m_mainFrame will be deleted automatically...
    } else {
#if ENABLE(NAVIGATOR_CONTENT_UTILS)
        delete m_navigatorcontentutils;
        m_navigatorcontentutils = 0;
#endif
        delete mainframeloaderclient;
        delete m_progress;
#if ENABLE(NOTIFICATIONS)
        delete m_notification;
        m_notification = 0;
#endif
#if ENABLE(MEDIA_STREAM)
        delete usermedia;
#endif
#if ENABLE(DEVICE_ORIENTATION)
        delete m_deviceorientation;
        delete m_devicemotion;
        m_deviceorientation = 0;
        m_devicemotion = 0;
#endif
#if ENABLE(GEOLOCATION)
        delete geolocation;
#endif
        delete drag;
        delete editor;
#if ENABLE(CONTEXT_MENUS)
        delete contextmenu;
#endif
#if ENABLE(WKC_WEB_NFC)
        delete webNfcClient;
#endif
        delete backforward;
        delete chrome;
    }
    if (m_inspector) {
        delete m_inspector;
        m_inspector = 0;
    }
    return false;
}

void
WKCWebViewPrivate::notifyForceTerminate()
{
    NX_DP(("WKCWebViewPrivate::notifyForceTerminate\n"));
    m_forceTerminated = true;
    if (m_mainFrame) {
        m_mainFrame->notifyForceTerminate();
    }
    if (WebCore::ResourceHandleManager::isExistSharedInstance()) {
        WebCore::ResourceHandleManager::sharedInstance()->suspendNetworkThread();
    }
}

WKC::WKCSettings*
WKCWebViewPrivate::settings()
{
    return m_settings;
}

WKC::Page*
WKCWebViewPrivate::wkcCore() const
{
    return &m_wkcCorePage->wkc();
}

// drawings
bool
WKCWebViewPrivate::setOffscreen(OffscreenFormat format, void* bitmap, int rowbytes, const WebCore::IntSize& offscreensize, const WebCore::IntSize& viewsize, bool fixedlayout, const WebCore::IntSize* desktopsize, bool needslayout)
{
    int pformat = 0;
    WKCSize size;

    if (!desktopsize)
        desktopsize = &offscreensize;

    if (m_drawContext) {
        wkcDrawContextDeletePeer(m_drawContext);
        m_drawContext = 0;
    }
    if (m_offscreen) {
        wkcOffscreenDeletePeer(m_offscreen);
        m_offscreen = 0;
    }

    switch (format) {
    case EOffscreenFormatPolygon:
        pformat = WKC_OFFSCREEN_TYPE_POLYGON;
        break;
    case EOffscreenFormatCairo16:
        pformat = WKC_OFFSCREEN_TYPE_CAIRO16;
        break;
    case EOffscreenFormatCairo32:
        pformat = WKC_OFFSCREEN_TYPE_CAIRO32;
        break;
    case EOffscreenFormatCairoSurface:
        pformat = WKC_OFFSCREEN_TYPE_CAIROSURFACE;
        break;
    default:
        return false;
    }
    size.fWidth = offscreensize.width();
    size.fHeight = offscreensize.height();

#if USE(WKC_CAIRO)
    m_offscreenFormat = pformat;
    m_offscreenBitmap = bitmap;
    m_offscreenRowBytes = rowbytes;
    m_offscreenSize = size;
#endif

    m_offscreen = wkcOffscreenNewPeer(pformat, bitmap, rowbytes, &size);
    if (!m_offscreen) return false;

    m_drawContext = wkcDrawContextNewPeer(m_offscreen);
    if (!m_drawContext) return false;

    if (needslayout) {
        m_desktopSize = *desktopsize;
        m_defaultDesktopSize = *desktopsize;
        m_defaultViewSize = viewsize;
        m_viewSize = viewsize;

        WebCore::Frame* frame = (WebCore::Frame *)&core()->mainFrame();
        if (!frame || !frame->view()) {
            return false;
        }

        if (fixedlayout) {
            frame->view()->setUseFixedLayout(true);
            frame->view()->setFixedLayoutSize(viewsize);
        } else {
            frame->view()->setUseFixedLayout(false);
        }

        frame->view()->resize(desktopsize->width(), desktopsize->height());
        frame->view()->forceLayout();
        frame->view()->adjustViewSize();
    }
    return true;
}

void
WKCWebViewPrivate::notifyResizeDesktopSize(const WebCore::IntSize& size, bool sendresizeevent)
{
    m_desktopSize = size;

    WebCore::Frame* frame = (WebCore::Frame *)&core()->mainFrame();
    if (!frame || !frame->view()) {
        return;
    }

    frame->view()->resize(size.width(), size.height());
    if (sendresizeevent)
        frame->view()->sendResizeEventIfNeeded();
    frame->view()->forceLayout();
    frame->view()->adjustViewSize();

    updateOverlay(WebCore::IntRect(), true);
}

void
WKCWebViewPrivate::notifyResizeViewSize(const WebCore::IntSize& size)
{
    WebCore::Frame* frame = (WebCore::Frame *)&core()->mainFrame();
    if (!frame) return;
    WebCore::FrameView* view = frame->view();
    if (!view) return;

    m_viewSize = size;
    view->setFixedLayoutSize(size);
}

const WebCore::IntSize&
WKCWebViewPrivate::desktopSize() const
{
    return m_desktopSize;
}

const WebCore::IntSize&
WKCWebViewPrivate::viewSize() const
{
    return m_viewSize;
}

const WebCore::IntSize&
WKCWebViewPrivate::defaultDesktopSize() const
{
    return m_defaultDesktopSize;
}

const WebCore::IntSize&
WKCWebViewPrivate::defaultViewSize() const
{
    return m_defaultViewSize;
}

void
WKCWebViewPrivate::notifyRelayout()
{
    WebCore::Frame* frame = (WebCore::Frame *)&core()->mainFrame();

    if (!frame || !frame->contentRenderer() || !frame->view()) {
        return;
    }

	frame->view()->updateLayoutAndStyleIfNeededRecursive();
}

void
WKCWebViewPrivate::notifyPaintOffscreenFrom(const WebCore::IntRect& rect, const WKCPoint& p)
{
#if USE(WKC_CAIRO)
    if (!recoverFromCairoError())
        return;
#else
    if (!m_offscreen)
        return;
#endif

    WebCore::Frame* frame = (WebCore::Frame *)&core()->mainFrame();

    if (!frame || !frame->contentRenderer() || !frame->view()) {
        return;
    }

    WebCore::GraphicsContext ctx((PlatformGraphicsContext *)m_drawContext);

    wkcOffscreenBeginPaintPeer(m_offscreen);
    float transX, transY;

    WebCore::IntRect scrolledRect = rect;
    scrolledRect.move(p.fX, p.fY);

    transX = p.fX;
    transY = p.fY;
    WebCore::FloatRect cr;
    cr = WebCore::FloatRect(scrolledRect.x() - transX, scrolledRect.y() - transY, scrolledRect.width(), scrolledRect.height());

    ctx.save();
#if USE(WKC_CAIRO)
    wkcDrawContextSetOpticalZoomPeer(m_drawContext, m_opticalZoomLevel, &m_opticalZoomOffset);
#endif
    ctx.clip(cr);
    ctx.translate(-transX, -transY);
    NX_DP(("Paint(%d, %d, %d, %d)", scrolledRect.x(), scrolledRect.y(), scrolledRect.width(), scrolledRect.height()));
    frame->view()->paintContents(ctx, scrolledRect);

    // WKCOverlayList::paintOffscreen will translate (-visibleRect.x(), -visibleRect.y()), so cancel out the offset here.
    WebCore::FloatRect visibleRect = frame->view()->visibleContentRect();
    ctx.translate(visibleRect.x(), visibleRect.y());
    paintOverlay(ctx);

    ctx.restore();

    wkcOffscreenEndPaintPeer(m_offscreen);
}

void
WKCWebViewPrivate::notifyPaintOffscreen(const WebCore::IntRect& rect)
{
#if USE(WKC_CAIRO)
    if (!recoverFromCairoError())
        return;
#else
    if (!m_offscreen)
        return;
#endif

    WebCore::Frame* frame = (WebCore::Frame *)&core()->mainFrame();

    if (!frame || !frame->contentRenderer() || !frame->view()) {
        return;
    }

    WebCore::GraphicsContext ctx((PlatformGraphicsContext *)m_drawContext);

    wkcOffscreenBeginPaintPeer(m_offscreen);
    WebCore::FloatRect cr(rect.x(), rect.y(), rect.width(), rect.height());
    ctx.save();
#if USE(WKC_CAIRO)
    wkcDrawContextSetOpticalZoomPeer(m_drawContext, m_opticalZoomLevel, &m_opticalZoomOffset);
#endif
    ctx.clip(cr);
    frame->view()->paint(ctx, rect);

    paintOverlay(ctx);

    ctx.restore();
    wkcOffscreenEndPaintPeer(m_offscreen);
}

#if USE(WKC_CAIRO)
#include <cairo.h>
void
WKCWebViewPrivate::notifyPaintToContext(const WebCore::IntRect& rect, void* context)
{
    WebCore::Frame* frame = (WebCore::Frame *)&core()->mainFrame();

    if (!frame || !frame->contentRenderer() || !frame->view()) {
        return;
    }

    PlatformGraphicsContext pc((cairo_t *)context);
    WebCore::GraphicsContext ctx(&pc);

    WebCore::FloatRect cr(rect.x(), rect.y(), rect.width(), rect.height());
    ctx.save();
    ctx.clip(cr);
    frame->view()->paint(ctx, rect);
    ctx.restore();
}
#endif

void
WKCWebViewPrivate::notifyScrollOffscreen(const WebCore::IntRect& rect, const WebCore::IntSize& diff)
{
    if (!m_offscreen) return;

    WKCRect r;
    WKCSize d;
    r.fX = rect.x();
    r.fY = rect.y();
    r.fWidth = rect.width();
    r.fHeight = rect.height();
    d.fWidth = diff.width();
    d.fHeight = diff.height();
    wkcOffscreenBeginPaintPeer(m_offscreen);
    wkcOffscreenScrollPeer(m_offscreen, &r, &d);
    wkcOffscreenEndPaintPeer(m_offscreen);
}

void
WKCWebViewPrivate::setTransparent(bool flag)
{
    m_isTransparent = flag;
    WebCore::Frame* frame = (WebCore::Frame *)&core()->mainFrame();
    if (!frame || !frame->view()) return;
    frame->view()->setTransparent(flag);
}

void
WKCWebViewPrivate::setOpticalZoom(float zoom_level, const WKCFloatPoint& offset)
{
    if (!m_offscreen) return;

    m_opticalZoomLevel = zoom_level;
    m_opticalZoomOffset = offset;
    wkcOffscreenSetOpticalZoomPeer(m_offscreen, zoom_level, &offset);
}

void
WKCWebViewPrivate::enterCompositingMode()
{
    WebCore::Frame* frame = &core()->mainFrame();
    frame->view()->enterCompositingMode();
}

void
WKCWebViewPrivate::setUseAntiAliasForDrawings(bool flag)
{
    if (!m_offscreen) return;
    wkcOffscreenSetUseAntiAliasForPolygonPeer(m_offscreen, flag);
}

void
WKCWebViewPrivate::setUseAntiAliasForCanvas(bool flag)
{
#if !USE(WKC_CAIRO)
    WebCore::ImageBufferData::setUseAA(flag);
#endif
}

void
WKCWebViewPrivate::setUseBilinearForScaledImages(bool flag)
{
    if (!m_offscreen) return;
    wkcOffscreenSetUseInterpolationForImagePeer(m_offscreen, flag);
}

void
WKCWebViewPrivate::setUseBilinearForCanvasImages(bool flag)
{
#if !USE(WKC_CAIRO)
    WebCore::ImageBufferData::setUseBilinear(flag);
#endif
}


void
WKCWebViewPrivate::setScrollPositionForOffscreen(const WebCore::IntPoint& scrollPosition)
{
    if (!m_offscreen) return;
    const WKCPoint pos = { scrollPosition.x(), scrollPosition.y() };
    wkcOffscreenSetScrollPositionPeer(m_offscreen, &pos);
}

void
WKCWebViewPrivate::notifyScrollPositionChanged()
{
    WebCore::FrameView* view = m_mainFrame->privateFrame()->core()->view();
    WebCore::IntPoint p;
    view->scrollPositionChanged(p, p);
}

WKC::Element*
WKCWebViewPrivate::getFocusedElement()
{
    WebCore::Document* doc = core()->focusController().focusedOrMainFrame().document();
    if (!doc)
        return 0;
    WebCore::Element* element = doc->focusedElement();

    if (!element)
        return 0;

    if (!m_focusedElement || m_focusedElement->webcore()!=element) {
        delete m_focusedElement;
        m_focusedElement = ElementPrivate::create(element);
    }
    return &m_focusedElement->wkc();
}

WKC::Element*
WKCWebViewPrivate::getElementFromPoint(int x, int y)
{
    WebCore::Element* element = 0;
    WebCore::Frame* frame = (WebCore::Frame *)&core()->mainFrame();

    WKC::WKCMouseEvent ev;
    ev.m_type = WKC::EMouseEventDown;
    ev.m_button = WKC::EMouseButtonLeft;
    ev.m_x = x;
    ev.m_y = y;
    ev.m_modifiers = WKC::EModifierNone;
    ev.m_timestampinsec = wkcGetTickCountPeer() / 1000;
    const WebCore::PlatformMouseEvent mouseEvent((void *)&ev);

    while (frame) {
        WebCore::Document* doc = frame->document();
        WebCore::FrameView* view = frame->view();
        WebCore::IntPoint documentPoint = view ? view->windowToContents(WebCore::IntPoint(x, y)) : WebCore::IntPoint(x, y);
        WebCore::RenderView* renderView = doc->renderView();
        WebCore::HitTestRequest request(WebCore::HitTestRequest::ReadOnly | WebCore::HitTestRequest::Active);
        WebCore::HitTestResult result(documentPoint);

        renderView->layer()->hitTest(request, result);

        const WebCore::MouseEventWithHitTestResults hr(mouseEvent, result);
        element = result.targetElement();
        frame = WebCore::EventHandler::subframeForHitTestResult(hr);
    }

    if (!element)
        return 0;

    if (!m_elementFromPoint || m_elementFromPoint->webcore()!=element) {
        delete m_elementFromPoint;
        m_elementFromPoint = ElementPrivate::create(element);
    }
    return &m_elementFromPoint->wkc();
}

#if USE(WKC_CAIRO)
bool
WKCWebViewPrivate::recoverFromCairoError()
{
    if (!m_offscreenBitmap)
        return false;

    if (!m_offscreen || wkcOffscreenIsErrorPeer(m_offscreen) ||
        !m_drawContext || wkcDrawContextIsErrorPeer(m_drawContext)) {
        if (m_drawContext) {
            wkcDrawContextDeletePeer(m_drawContext);
            m_drawContext = 0;
        }
        if (m_offscreen) {
            wkcOffscreenDeletePeer(m_offscreen);
            m_offscreen = 0;
        }

        m_offscreen = wkcOffscreenNewPeer(m_offscreenFormat, m_offscreenBitmap, m_offscreenRowBytes, &m_offscreenSize);
        if (!m_offscreen)
            return false;
        m_drawContext = wkcDrawContextNewPeer(m_offscreen);
        if (!m_drawContext) {
            wkcOffscreenDeletePeer(m_offscreen);
            m_offscreen = 0;
            return false;
        }
    }

    return true;
}
#endif

void
WKCWebViewPrivate::setSpatialNavigationEnabled(bool enable)
{
    if (!m_mainFrame)
        return;
    m_mainFrame->setSpatialNavigationEnabled(enable);
}

const unsigned short*
WKCWebViewPrivate::selectionText()
{
    WebCore::Frame& frame = core()->focusController().focusedOrMainFrame();

    RefPtr<WebCore::Range> range = frame.selection().toNormalizedRange();
    if (!range) {
        return 0; 
    }

    m_selectedText = range->text();

    return m_selectedText.charactersWithNullTermination().data();
}

void
WKCWebViewPrivate::setIsVisible(bool isVisible)
{
    if (isVisible) {
        m_activityState &= ~WebCore::ActivityState::IsVisuallyIdle;
        m_activityState |= WebCore::ActivityState::IsVisible | WebCore::ActivityState::IsVisibleOrOccluded;
        // Start SVG animations.
         for (WebCore::Frame* frame = &core()->mainFrame(); frame; frame = frame->tree().traverseNext()) {
            if (frame->document() && frame->document()->svgExtensions())
                frame->document()->accessSVGExtensions().unpauseAnimations();
        }
    } else {
        m_activityState &= ~(WebCore::ActivityState::IsVisible | WebCore::ActivityState::IsVisibleOrOccluded);
        m_activityState |= WebCore::ActivityState::IsVisuallyIdle;
        // Stop SVG animations.
        for (WebCore::Frame* frame = &core()->mainFrame(); frame; frame = frame->tree().traverseNext()) {
            if (frame->document() && frame->document()->svgExtensions())
                frame->document()->accessSVGExtensions().pauseAnimations();
        }
    }
    core()->setActivityState(m_activityState);
    core()->settings().setHiddenPageDOMTimerThrottlingEnabled(!isVisible);
    if (&core()->chrome())
        core()->chrome().setChromeVisible(isVisible);
}

void
WKCWebViewPrivate::notifyFocusIn()
{
    m_activityState |= WebCore::ActivityState::WindowIsActive | WebCore::ActivityState::IsFocused;
    core()->setActivityState(m_activityState);
}

void
WKCWebViewPrivate::notifyFocusOut()
{
    m_activityState &= ~(WebCore::ActivityState::WindowIsActive | WebCore::ActivityState::IsFocused);
    core()->setActivityState(m_activityState);
}

// implementations
WKC_DEFINE_GLOBAL_CLASS_OBJ(bool, WKCWebView, m_clipsRepaints, true);

WKCWebView::WKCWebView()
     : m_private(0)
     , m_EPUB(0)
{
}

WKCWebView::~WKCWebView()
{
    if (m_private) {
        if (!m_private->m_forceTerminated) {
            stopLoading();
        }
        delete m_private;
        m_private = 0;
    }
    if (m_EPUB) {
        m_EPUB->~EPUB();
        WTF::fastFree(m_EPUB);
    }
}

WKCWebView*
WKCWebView::create(WKCClientBuilders& builders)
{
    WKCWebView* self = 0;

    self = (WKCWebView *)WTF::fastMalloc(sizeof(WKCWebView));
    ::memset(self, 0, sizeof(WKCWebView));
    self = new (self) WKCWebView();
    if (!self) return 0;
    if (!self->construct(builders)) {
        self->~WKCWebView();
        WTF::fastFree(self);
        return 0;
    }

    return self;
}

bool
WKCWebView::construct(WKCClientBuilders& builders)
{
    m_private = WKCWebViewPrivate::create(this, builders);
    if (!m_private) return false;
    m_EPUB = (EPUB *)WTF::fastMalloc(sizeof(EPUB));
    ::memset(m_EPUB, 0, sizeof(EPUB));
    m_EPUB = new (m_EPUB) EPUB(this);
    return true;
}

void
WKCWebView::deleteWKCWebView(WKCWebView *self)
{
    if (!self)
        return;
    self->~WKCWebView();
    WTF::fastFree(self);
}

void
WKCWebView::notifyForceTerminate()
{
    if (m_private) {
        m_private->notifyForceTerminate();
    }
}

// off-screen draw

bool
WKCWebView::setOffscreen(OffscreenFormat format, void* bitmap, int rowbytes, const WKCSize& offscreensize, const WKCSize& viewsize, bool fixedlayout, const WKCSize* const desktopsize, bool needslayout)
{
    WebCore::IntSize os(offscreensize.fWidth, offscreensize.fHeight);
    WebCore::IntSize vs(viewsize.fWidth, viewsize.fHeight);
    WebCore::IntSize ds;
    if (desktopsize) {
        ds.setWidth(desktopsize->fWidth);
        ds.setHeight(desktopsize->fHeight);
    }
    return m_private->setOffscreen(format, bitmap, rowbytes, os, vs, fixedlayout, desktopsize ? &ds : 0, needslayout);
}

void
WKCWebView::notifyResizeViewSize(const WKCSize& size)
{
    WebCore::IntSize s(size.fWidth, size.fHeight);
    m_private->notifyResizeViewSize(s);
}

void
WKCWebView::notifyResizeDesktopSize(const WKCSize& size, bool sendresizeevent)
{
    WebCore::IntSize s(size.fWidth, size.fHeight);
    m_private->notifyResizeDesktopSize(s, sendresizeevent);
}

void
WKCWebView::notifyRelayout(bool force)
{
    if (force) {
        WebCore::Frame* mainFrame = (WebCore::Frame *)&m_private->core()->mainFrame();
        for (WebCore::Frame* frame = mainFrame; frame; frame = frame->tree().traverseNext()) {
            WebCore::RenderView* renderView = frame->contentRenderer();
            if (!renderView)
                continue;
            renderView->setNeedsLayout();
        }
    }
    m_private->notifyRelayout();
}

void
WKCWebView::notifyPaintOffscreenFrom(const WKCRect& rect, const WKCPoint& p)
{
    WebCore::IntRect r(rect.fX, rect.fY, rect.fWidth, rect.fHeight);
    m_private->notifyPaintOffscreenFrom(r, p);
}

void
WKCWebView::notifyPaintOffscreen(const WKCRect& rect)
{
    WebCore::IntRect r(rect.fX, rect.fY, rect.fWidth, rect.fHeight);
    m_private->notifyPaintOffscreen(r);
}

#if USE(WKC_CAIRO)
void
WKCWebView::notifyPaintToContext(const WKCRect& rect, void* context)
{
    WebCore::IntRect r(rect.fX, rect.fY, rect.fWidth, rect.fHeight);
    m_private->notifyPaintToContext(r, context);
}
#endif

void
WKCWebView::notifyScrollOffscreen(const WKCRect& rect, const WKCSize& diff)
{
    WebCore::IntRect r(rect.fX, rect.fY, rect.fWidth, rect.fHeight);
    WebCore::IntSize d(diff.fWidth, diff.fHeight);
    m_private->notifyScrollOffscreen(r, d);
}

// events

bool
WKCWebView::notifyKeyPress(WKC::Key key, WKC::Modifier modifiers, bool in_autorepeat)
{
    WebCore::Frame& frame = m_private->core()->focusController().focusedOrMainFrame();
    if (!frame.view()) return false;

    WKC::WKCKeyEvent ev;
    ev.m_type = WKC::EKeyEventPressed;
    ev.m_key = key;
    ev.m_modifiers = modifiers;
    ev.m_char = 0;
    ev.m_autoRepeat = in_autorepeat;
    WebCore::PlatformKeyboardEvent keyboardEvent((void *)&ev);

    if (frame.eventHandler().keyEvent(keyboardEvent)) {
        return true;
    }
    return false;
}
bool
WKCWebView::notifyKeyRelease(WKC::Key key, WKC::Modifier modifiers)
{
    WebCore::Frame& frame = m_private->core()->focusController().focusedOrMainFrame();

    if (!frame.view()) return false;

    WKC::WKCKeyEvent ev;
    ev.m_type = WKC::EKeyEventReleased;
    ev.m_key = key;
    ev.m_modifiers = modifiers;
    ev.m_char = 0;
    ev.m_autoRepeat = false;
    WebCore::PlatformKeyboardEvent keyboardEvent((void *)&ev);

    if (frame.eventHandler().keyEvent(keyboardEvent)) {
        return true;
    }
    return false;
}
bool
WKCWebView::notifyKeyChar(unsigned int in_char)
{
    WebCore::Frame& frame = m_private->core()->focusController().focusedOrMainFrame();

    if (!frame.view()) return false;

    WKC::WKCKeyEvent ev;
    ev.m_type = WKC::EKeyEventChar;
    ev.m_char = in_char;
    ev.m_key = (WKC::Key)0;
    ev.m_autoRepeat = false;
    WebCore::PlatformKeyboardEvent keyboardEvent((void *)&ev);

    if (frame.eventHandler().keyEvent(keyboardEvent)) {
        return true;
    }
    return false;
}

bool
WKCWebView::notifyIMEComposition(const unsigned short* in_string, WKC::CompositionUnderline* in_underlines, unsigned int in_underlineNum, unsigned int in_cursorPosition, unsigned int in_selectionEnd, bool in_confirm)
{
    WebCore::Frame& frame = m_private->core()->focusController().focusedOrMainFrame();

    if (in_confirm) {
        frame.editor().confirmComposition(String(in_string));
    } else {
        WTF::Vector<WebCore::CompositionUnderline> underlines;
        if (in_underlineNum > 0) {
            underlines.resize(in_underlineNum);
            for (unsigned i = 0; i < in_underlineNum; i++) {
                underlines[i].startOffset = in_underlines[i].startOffset;
                underlines[i].endOffset = in_underlines[i].endOffset;
                underlines[i].thick = in_underlines[i].thick;
                underlines[i].color = in_underlines[i].color;
                underlines[i].compositionUnderlineColor = WebCore::CompositionUnderlineColor::GivenColor;
            }
        }
        frame.editor().setComposition(String(in_string), underlines, in_cursorPosition, in_selectionEnd);
    }
    return true;
}

bool
WKCWebView::notifyAccessKey(unsigned int in_char)
{
    WebCore::Frame& frame = m_private->core()->focusController().focusedOrMainFrame();
    if (!frame.view()) return false;

    WKC::WKCKeyEvent ev;
    ev.m_type = WKC::EKeyEventAccessKey;
    ev.m_key = WKC::EKeyUnknown;
    ev.m_modifiers = EModifierNone;

    WTF::OptionSet<WebCore::PlatformEvent::Modifier> mod = WebCore::EventHandler::accessKeyModifiers();
    if (mod.contains(WebCore::PlatformEvent::Modifier::AltKey))
        ev.m_modifiers |= WKC::Modifier::EModifierAlt;
    if (mod.contains(WebCore::PlatformEvent::Modifier::CtrlKey))
        ev.m_modifiers |= WKC::Modifier::EModifierCtrl;
    if (mod.contains(WebCore::PlatformEvent::Modifier::MetaKey))
        ev.m_modifiers |= WKC::Modifier::EModifierMod1;
    if (mod.contains(WebCore::PlatformEvent::Modifier::ShiftKey))
        ev.m_modifiers |= WKC::Modifier::EModifierShift;

    ev.m_char = in_char;
    ev.m_autoRepeat = false;
    WebCore::PlatformKeyboardEvent keyboardEvent((void *)&ev);

    if (frame.eventHandler().handleAccessKey(keyboardEvent)) {
        return true;
    }
    return false;
}

bool
WKCWebView::notifyMouseDown(const WKCPoint& pos, WKC::MouseButton button, WKC::Modifier modifiers)
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();
    WKC::WKCMouseEvent ev;

    ev.m_type = WKC::EMouseEventDown;
    ev.m_button = button;
    ev.m_x = pos.fX;
    ev.m_y = pos.fY;
    ev.m_modifiers = modifiers;
    ev.m_timestampinsec = wkcGetTickCountPeer() / 1000;
    WebCore::PlatformMouseEvent mouseEvent((void *)&ev);

    return frame->eventHandler().handleMousePressEvent(mouseEvent);
}
bool
WKCWebView::notifyMouseUp(const WKCPoint& pos, WKC::MouseButton button, Modifier modifiers)
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();
    WKC::WKCMouseEvent ev;

    ev.m_type = WKC::EMouseEventUp;
    ev.m_button = button;
    ev.m_x = pos.fX;
    ev.m_y = pos.fY;
    ev.m_modifiers = modifiers;
    ev.m_timestampinsec = wkcGetTickCountPeer() / 1000;
    WebCore::PlatformMouseEvent mouseEvent((void *)&ev);

    return frame->eventHandler().handleMouseReleaseEvent(mouseEvent);
}
bool
WKCWebView::notifyMouseMove(const WKCPoint& pos, WKC::MouseButton button, Modifier modifiers)
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();
    if (!frame->view()) return false;

    WKC::WKCMouseEvent ev;

    ev.m_type = WKC::EMouseEventMove;
    ev.m_button = button;
    ev.m_x = pos.fX;
    ev.m_y = pos.fY;
    ev.m_modifiers = modifiers;
    ev.m_timestampinsec = wkcGetTickCountPeer() / 1000;
    WebCore::PlatformMouseEvent mouseEvent((void *)&ev);

    return frame->eventHandler().mouseMoved(mouseEvent);
}
static int
getRendererCount(WebCore::RenderObject* renderer, WebCore::Node* linkNode)
{
    int count = 0;
    for (WebCore::RenderObject* r = renderer; r;) {
        if (r->node() == linkNode) {
            r = r->nextInPreOrderAfterChildren(renderer);
            continue;
        }
        count++;
        r = r->nextInPreOrder(renderer);
    }
    return count;
}
bool
WKCWebView::notifyMouseMoveTest(const WKCPoint& pos, WKC::MouseButton button, Modifier modifiers, bool& contentChanged)
{
    WKC::Element* element;
    WKC::Node* node;
    WebCore::Node* coreNode;
    WebCore::Node* linkNode;
    WebCore::Node* targetNode;
    WebCore::RenderObject* renderer;
    WebCore::RenderStyle* style;
    int* visibilityInfoList;
    int rendererCount = 0;
    int visibility = 0;
    int index = 0;
    bool result = false;

    contentChanged = false;

    element = getElementFromPoint(pos.fX, pos.fY);
    if (!element)
        return false;
    node = element->firstChild();
    if (!node)
        return false;
    coreNode = node ? node->priv().webcore() : 0;
    linkNode = coreNode ? coreNode->enclosingLinkEventParentOrSelf() : 0;

    if (!coreNode || !linkNode)
        return notifyMouseMove(pos, button, modifiers);

    renderer = linkNode->renderer();
    if (renderer && WTF::is<WebCore::RenderElement>(renderer)) {
        renderer = ((WebCore::RenderElement *)renderer)->hoverAncestor();
    } else {
        renderer = 0;
    }
    renderer = renderer ? renderer->containingBlock() : 0;
    while (renderer && renderer->isAnonymousBlock())
        renderer = renderer->parent();
    targetNode = renderer ? renderer->node() : 0;

    if (!renderer || !targetNode)
        return notifyMouseMove(pos, button, modifiers);

    for (WebCore::Node* n = targetNode; n; n = n->parentNode()) {
        if (n == linkNode)
            return notifyMouseMove(pos, button, modifiers);
    }

    rendererCount = getRendererCount(renderer, linkNode);
    if (rendererCount == 0)
        return notifyMouseMove(pos, button, modifiers);

    visibilityInfoList = (int*)WTF::fastMalloc(sizeof(int) * rendererCount);
    if (!visibilityInfoList)
        return notifyMouseMove(pos, button, modifiers);

    index = 0;
    for (WebCore::RenderObject* r = renderer; r;) {
        if (r->node() == linkNode) {
            r = r->nextInPreOrderAfterChildren(renderer);
            continue;
        }
        visibility = 0;
        style = const_cast<WebCore::RenderStyle*>(&r->style());
        if (WebCore::Visibility::Visible == style->visibility()) {
            visibility = 1;
        }
        visibilityInfoList[index] = visibility;
        index++;
        r = r->nextInPreOrder(renderer);
    }

    result = notifyMouseMove(pos, button, modifiers);

    renderer = targetNode->renderer();

    if (!renderer) {
        // Since the renderer has been deleted, there is a possibility that the appearance of the content has changed,
        // but in this case, we do not consider it to be a change for a mouse over menu.
        contentChanged = false;
        goto exit;
    }

    renderer->frame().document()->updateStyleIfNeeded();

    renderer = targetNode->renderer();

    if (getRendererCount(renderer, linkNode) != rendererCount) {
        contentChanged = true;
        goto exit;
    }

    index = 0;

    for (WebCore::RenderObject* r = renderer; r;) {
        if (r->node() == linkNode) {
            r = r->nextInPreOrderAfterChildren(renderer);
            continue;
        }
        style = const_cast<WebCore::RenderStyle*>(&r->style());
        visibility = 0;
        if (style) {
            if (WebCore::Visibility::Visible == style->visibility()) {
                visibility = 1;
            }
        }
        if (WTF::is<WebCore::RenderElement>(r)) {
            if (visibility != visibilityInfoList[index] && ((WebCore::RenderElement *)r)->firstChild()) {
                contentChanged = true;
                goto exit;
            }
        }
        index++;
        r = r->nextInPreOrder(renderer);
    }

exit:
    WTF::fastFree(visibilityInfoList);

    return result;
}
bool
WKCWebView::notifyMouseDoubleClick(const WKCPoint& pos, WKC::MouseButton button, WKC::Modifier modifiers)
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();
    WKC::WKCMouseEvent ev;

    ev.m_type = WKC::EMouseEventDoubleClick;
    ev.m_button = button;
    ev.m_x = pos.fX;
    ev.m_y = pos.fY;
    ev.m_modifiers = modifiers;
    ev.m_timestampinsec = wkcGetTickCountPeer() / 1000;
    WebCore::PlatformMouseEvent mouseEvent((void *)&ev);

    return frame->eventHandler().handleMousePressEvent(mouseEvent);
}
bool
WKCWebView::notifyMouseWheel(const WKCPoint& pos, const WKCSize& diff, WKC::Modifier modifiers)
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();
    WKC::WKCWheelEvent ev;

    ev.m_dx = diff.fWidth;
    ev.m_dy = diff.fHeight;
    ev.m_x = pos.fX;
    ev.m_y = pos.fY;
    ev.m_modifiers = modifiers;
    WebCore::PlatformWheelEvent wheelEvent((void *)&ev);
    return frame->eventHandler().handleWheelEvent(wheelEvent);
}

void
WKCWebView::notifySetMousePressed(bool pressed)
{
    WebCore::Frame* frame = &m_private->core()->focusController().focusedOrMainFrame();
    if (frame) {
        frame->eventHandler().setMousePressed(pressed);
    }
}

void
WKCWebView::notifyLostMouseCapture()
{
    WebCore::Frame* frame = &m_private->core()->focusController().focusedOrMainFrame();
    if (frame) {
        frame->eventHandler().lostMouseCapture();
    }
}

bool
WKCWebView::notifyTouchEvent(int type, const TouchPoint* points, int npoints, WKC::Modifier in_modifiers)
{
#if ENABLE(TOUCH_EVENTS)
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();
    WKC::WKCTouchEvent ev = {0};
    WTF::Vector<WKCTouchPoint> tp(npoints);

    for (int i=0; i<npoints; i++) {
        tp[i].m_id = points[i].fId;
        tp[i].m_state = points[i].fState;
        WKCPoint_SetPoint(&tp[i].m_pos, &points[i].fPoint);
    }

    ev.m_type = type;
    ev.m_points = tp.data();
    ev.m_npoints = npoints;
    ev.m_modifiers = in_modifiers;
    ev.m_timestampinsec = wkcGetTickCountPeer() / 1000;
    WebCore::PlatformTouchEvent touchEvent((void *)&ev);

    return frame->eventHandler().handleTouchEvent(touchEvent);
#else
    return false;
#endif
}

bool
WKCWebView::hasTouchEventHandlers()
{
#if ENABLE(TOUCH_EVENTS)
    return m_private->core()->mainFrame().document()->hasTouchEventHandlers();
#else
    return false;
#endif
}

bool
WKCWebView::notifyScroll(WKC::ScrollType type)
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();
    WebCore::ScrollDirection dir = WebCore::ScrollUp;
    WebCore::ScrollGranularity gra = WebCore::ScrollByLine;

    switch (type) {
    case EScrollUp:
        gra = WebCore::ScrollByLine;
        dir = WebCore::ScrollUp;
        break;
    case EScrollDown:
        gra = WebCore::ScrollByLine;
        dir = WebCore::ScrollDown;
        break;
    case EScrollLeft:
        gra = WebCore::ScrollByLine;
        dir = WebCore::ScrollLeft;
        break;
    case EScrollRight:
        gra = WebCore::ScrollByLine;
        dir = WebCore::ScrollRight;
        break;
    case EScrollPageUp:
        gra = WebCore::ScrollByPage;
        dir = WebCore::ScrollUp;
        break;
    case EScrollPageDown:
        gra = WebCore::ScrollByPage;
        dir = WebCore::ScrollDown;
        break;
    case EScrollPageLeft:
        gra = WebCore::ScrollByPage;
        dir = WebCore::ScrollLeft;
        break;
    case EScrollPageRight:
        gra = WebCore::ScrollByPage;
        dir = WebCore::ScrollRight;
        break;
    case EScrollTop:
        gra = WebCore::ScrollByDocument;
        dir = WebCore::ScrollUp;
        break;
    case EScrollBottom:
        gra = WebCore::ScrollByDocument;
        dir = WebCore::ScrollDown;
        break;
    default:
        return false;
    }

    if (!frame->eventHandler().scrollOverflow(dir, gra)) {
        frame->view()->scroll(dir, gra);
        return true;
    }
    return false;
}

bool
WKCWebView::notifyScroll(int dx, int dy)
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();

    frame->view()->scrollBy(WebCore::IntSize(dx, dy));

    m_private->updateOverlay(WebCore::IntRect(), true);

    return true;
}

bool
WKCWebView::notifyScrollTo(int x, int y)
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();

    if (!frame || !frame->view()) {
        return false;
    }

    frame->view()->setScrollPosition(WebCore::IntPoint(x, y));

    m_private->updateOverlay(WebCore::IntRect(), true);

    return true;
}

void
WKCWebView::scrollPosition(WKCPoint& pos)
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();

    if (!frame || !frame->view()) {
        pos.fX = 0;
        pos.fY = 0;
        return;
    }

    WebCore::IntPoint p = frame->view()->scrollPosition();
    pos.fX = p.x();
    pos.fY = p.y();
}

void
WKCWebView::minimumScrollPosition(WKCPoint& pos) const
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();

    if (!frame || !frame->view()) {
        pos.fX = 0;
        pos.fY = 0;
        return;
    }

    WebCore::IntPoint p = frame->view()->minimumScrollPosition();
    pos.fX = p.x();
    pos.fY = p.y();
}

void
WKCWebView::maximumScrollPosition(WKCPoint& pos) const
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();

    if (!frame || !frame->view()) {
        pos.fX = 0;
        pos.fY = 0;
        return;
    }

    WebCore::IntPoint p = frame->view()->maximumScrollPosition();
    pos.fX = p.x();
    pos.fY = p.y();
}

void
WKCWebView::contentsSize(WKCSize& size)
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();

    if (!frame || !frame->view()) {
        size.fWidth = 0;
        size.fHeight = 0;
        return;
    }

    WebCore::IntSize s = frame->view()->contentsSize();
    size.fWidth = s.width();
    size.fHeight = s.height();
}

void
WKCWebView::notifyFocusIn()
{
    m_private->notifyFocusIn();
}

void
WKCWebView::notifyFocusOut()
{
    m_private->notifyFocusOut();
}

void
WKCWebView::notifyScrollPositionChanged()
{
    m_private->notifyScrollPositionChanged();
}

bool
WKCWebView::setFocusedElement(WKC::Element* ielement)
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->focusController().focusedOrMainFrame();
    WebCore::Document* newDocument = 0;
    WebCore::Element* element = ielement ? static_cast<ElementPrivate&>(ielement->priv()).webcore() : 0;
    if (element && frame) {
        WebCore::Document* focusedDocument = frame->document();
        newDocument = &element->document();
        if (newDocument != focusedDocument) {
            focusedDocument->setFocusedElement(0);
        }
        if (newDocument)
            m_private->core()->focusController().setFocusedFrame(newDocument->frame());
    }
    if (newDocument) {
        return newDocument->setFocusedElement(element);
    }
    return m_private->core()->focusController().focusedOrMainFrame().document()->setFocusedElement(element);
}

void
WKCWebView::notifySuspend()
{
    // Ugh!: implement it!
    // 100106 ACCESS Co.,Ltd.
    return;
}

void
WKCWebView::notifyResume()
{
    // Ugh!: implement it!
    // 100106 ACCESS Co.,Ltd.
    return;
}

// APIs
const unsigned short*
WKCWebView::title()
{
    return m_private->m_mainFrame->title();
}
const char*
WKCWebView::uri()
{
    return m_private->m_mainFrame->uri();
}

bool
WKCWebView::canGoBack()
{
    NX_DP(("WKCWebView::canGoBack Enter\n"));

    if (!m_private->core()) {
        NX_DP(("WKCWebView::canGoBack Exit 1\n"));
        return false;
    }
    if (!m_private->core()->backForward().client()->backListCount()) {
        NX_DP(("WKCWebView::canGoBack Exit 2\n"));
        return false;
    }

    NX_DP(("WKCWebView::canGoBack Exit 3\n"));
    return true;
}
bool
WKCWebView::canGoBackOrForward(int steps)
{
    return m_private->core()->backForward().canGoBackOrForward(steps);
}
bool
WKCWebView::canGoForward()
{
    if (!m_private->core()) {
        return false;
    }
    if (!m_private->core()->backForward().client()->forwardListCount()) {
        return false;
    }
    return true;
}
bool
WKCWebView::goBack()
{
    return m_private->core()->backForward().goBack();
}
void
WKCWebView::goBackOrForward(int steps)
{
    m_private->core()->backForward().goBackOrForward(steps);
}
bool
WKCWebView::goForward()
{
    return m_private->core()->backForward().goForward();
}

void
WKCWebView::stopLoading()
{
    WebCore::Frame* frame = 0;
    WebCore::FrameLoader* loader = 0;
    frame = (WebCore::Frame *)&m_private->core()->mainFrame();
    if (!frame) return;
    loader = &frame->loader();
    if (!loader) return;
    loader->stopForUserCancel();
}
void
WKCWebView::reload()
{
    m_private->core()->mainFrame().loader().reload();
}
void
WKCWebView::reloadBypassCache()
{
    m_private->core()->mainFrame().loader().reload(WebCore::ReloadOption::FromOrigin);
}
bool
WKCWebView::loadURI(const char* uri, const char* referer)
{
    if (!uri) return false;
    if (!uri[0]) return false;
    if (!m_private) return false;

    WKCWebFrame* frame = m_private->m_mainFrame;
    return frame->loadURI(uri, referer);
}
void
WKCWebView::loadString(const char* content, const unsigned short* mimetype, const unsigned short* encoding, const char* base_uri)
{
    if (!content) return;
    if (!content[0]) return;
    if (!m_private) return;

    WKCWebFrame* frame = m_private->m_mainFrame;
    frame->loadString(content, mimetype, encoding, base_uri);
}
void
WKCWebView::loadHTMLString(const char* content, const char* base_uri)
{
    static const unsigned short cTextHtml[] = {'t','e','x','t','/','h','t','m','l',0};
    loadString(content, cTextHtml, 0, base_uri);
}

bool
WKCWebView::searchText(const unsigned short* text, bool case_sensitive, bool forward, bool wrap)
{
    WebCore::FindOptions opts;

    if (!case_sensitive)
        opts |= WebCore::CaseInsensitive;
    if (!forward)
        opts |= WebCore::Backwards;
    if (wrap)
        opts |= WebCore::WrapAround;

    return m_private->core()->findString(WTF::String(text), opts);
}
unsigned int
WKCWebView::markTextMatches(const unsigned short* string, bool case_sensitive, unsigned int limit)
{
    WebCore::FindOptions opts;

    if (!case_sensitive)
        opts |= WebCore::CaseInsensitive;

    return m_private->core()->markAllMatchesForText(WTF::String(string), opts, false, limit);
}
void
WKCWebView::setHighlightTextMatches(bool highlight)
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->mainFrame();

    do {
        frame->editor().setMarkedTextMatchesAreHighlighted(highlight);
        frame = frame->tree().traverseNext(WebCore::CanWrap::No);
    } while (frame);
}
void
WKCWebView::unmarkTextMatches()
{
    m_private->core()->unmarkAllTextMatches();
}

static WKCWebFrame*
kit(WebCore::Frame* coreFrame)
{
    if (!coreFrame)
      return 0;

    FrameLoaderClientWKC* client = static_cast<FrameLoaderClientWKC*>(&coreFrame->loader().client());
    return client ? client->webFrame() : 0;
}

WKCWebFrame*
WKCWebView::mainFrame()
{
    return m_private->m_mainFrame;
}
WKCWebFrame*
WKCWebView::focusedFrame()
{
    WebCore::Frame* focusedFrame = m_private->core()->focusController().focusedFrame();
    return kit(focusedFrame);
}

void
WKCWebView::executeScript(const char* script)
{
    m_private->core()->mainFrame().script().executeScript(WTF::String::fromUTF8(script), true);
}

bool
WKCWebView::isCaretAtBeginningOfTextField() const
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->focusController().focusedOrMainFrame();
    if (!frame || !frame->document())
        return false;

    WebCore::Element* element = frame->document()->focusedElement();
    if (!element)
        return false;

    WebCore::FrameSelection& frameSelection = frame->selection();
    if (!frameSelection.isCaret())
        return false;

    const WebCore::VisiblePosition& startPosition = frameSelection.selection().visibleStart();

    // cf. WebCore::isFirstVisiblePositionInNode

    if (startPosition.isNull())
        return false;

    if (!startPosition.deepEquivalent().containerNode()->isDescendantOrShadowDescendantOf(element))
        return false;

    WebCore::VisiblePosition previous = startPosition.previous();
    return previous.isNull() || !previous.deepEquivalent().deprecatedNode()->isDescendantOrShadowDescendantOf(element);
}

bool
WKCWebView::hasSelection()
{
    WebCore::Frame* frame = (WebCore::Frame *)&m_private->core()->focusController().focusedOrMainFrame();
    WTF::RefPtr<WebCore::Range> range = frame->selection().toNormalizedRange();
    if (!range)
        return false;
    return (range->startPosition() != range->endPosition());
}

void
WKCWebView::clearSelection()
{
    for (WebCore::Frame* frame = &m_private->core()->mainFrame(); frame; frame = frame->tree().traverseNext()) {
        frame->selection().clear();
    }
}

static bool
_selectionRects(WebCore::Page* page, WTF::Vector<WebCore::IntRect>& rects, bool textonly, bool useSelectionHeight)
{
    WebCore::Frame* frame = &page->focusController().focusedOrMainFrame();
    RefPtr<WebCore::Range> range = frame->selection().toNormalizedRange();
    if (!range) {
        return false;
    }
    WebCore::Node* startContainer = &range->startContainer();
    WebCore::Node* endContainer = &range->endContainer();

    if (!startContainer || !endContainer)
        return false;

    WebCore::Node* stopNode = range->pastLastNode();
    for (WebCore::Node* node = range->firstNode(); node && node != stopNode; node = node->nextSibling()) {
        WebCore::RenderObject* r = node->renderer();
        if (!r)
            continue;
        bool istext = r->isText();
        if (textonly && !istext)
            continue;
        if (istext) {
            WebCore::RenderText* renderText = WTF::downcast<WebCore::RenderText>(r);
            int startOffset = node == startContainer ? range->startOffset() : 0;
            int endOffset = node == endContainer ? range->endOffset() : std::numeric_limits<int>::max();
            rects = renderText->absoluteRectsForRange(startOffset, endOffset, useSelectionHeight);
        } else {
            const WebCore::FloatPoint absPos = r->localToAbsolute();
            const WebCore::LayoutPoint lp(absPos.x(), absPos.y());
            r->absoluteRects(rects, lp);
        }
    }

    /* adjust the rect with visible content rect */
    WebCore::IntRect visibleContentRect = frame->view()->visibleContentRect();
    for (size_t i = 0; i < rects.size(); ++i) {
        rects[i] = WebCore::intersection(rects[i], visibleContentRect);
        /* remove empty rect */
        if (rects[i].isEmpty()) {
            rects.remove(i);
            i--;
        }
    }

    return true;
}

WKCRect
WKCWebView::selectionBoundingBox(bool textonly, bool useSelectionHeight)
{
    WebCore::IntRect result;
    Vector<WebCore::IntRect> rects;

    _selectionRects(m_private->core(), rects, textonly, useSelectionHeight);
    const size_t n = rects.size();
    for (size_t i = 0; i < n; ++i)
        result.unite(rects[i]);

    WKCRect r = { result.x(), result.y(), result.width(), result.height() };
    return r;
}

const unsigned short*
WKCWebView::selectionText()
{
    return m_private->selectionText();
}

void
WKCWebView::selectAll()
{
    WebCore::Frame* frame = &m_private->core()->focusController().focusedOrMainFrame();
    frame->selection().selectAll();
}

WKC::Page*
WKCWebView::core()
{
    return m_private->wkcCore();
}

WKC::WKCSettings*
WKCWebView::settings()
{
    return m_private->settings();
}

bool
WKCWebView::canShowMimeType(const unsigned short* mime_type)
{
    WebCore::Frame& frame = m_private->core()->mainFrame();
    WebCore::FrameLoader& loader = frame.loader();
    return loader.client().canShowMIMEType(WTF::String(mime_type));
}

float
WKCWebView::zoomLevel()
{
    WebCore::Frame& frame = m_private->core()->mainFrame();
    return frame.pageZoomFactor();
}
float
WKCWebView::setZoomLevel(float zoom_level)
{
    WebCore::Frame& frame = m_private->core()->mainFrame();
    wkcOffscreenClearGlyphCachePeer();
    frame.setPageZoomFactor(zoom_level);
    return zoom_level;
}
void
WKCWebView::zoomIn(float ratio)
{
    float cur = 0;
    cur = zoomLevel();
    setZoomLevel(cur + ratio);
}
void
WKCWebView::zoomOut(float ratio)
{
    float cur = 0;
    cur = zoomLevel();
    setZoomLevel(cur - ratio);
}

float
WKCWebView::textOnlyZoomLevel()
{
    WebCore::Frame& frame = m_private->core()->mainFrame();
    return frame.textZoomFactor();
}
float
WKCWebView::setTextOnlyZoomLevel(float zoom_level)
{
    WebCore::Frame& frame = m_private->core()->mainFrame();
    wkcOffscreenClearGlyphCachePeer();
    frame.setTextZoomFactor(zoom_level);
    return zoom_level;
}
void
WKCWebView::textOnlyZoomIn(float ratio)
{
    float cur = 0;
    cur = textOnlyZoomLevel();
    setTextOnlyZoomLevel(cur + ratio);
}
void
WKCWebView::textOnlyZoomOut(float ratio)
{
    float cur = 0;
    cur = textOnlyZoomLevel();
    setTextOnlyZoomLevel(cur - ratio);
}

bool
WKCWebView::fullContentZoom()
{
    return m_private->m_isZoomFullContent;
}
void
WKCWebView::setFullContentZoom(bool full_content_zoom)
{
    float cur = 0;
    cur = zoomLevel();
    if (m_private->m_isZoomFullContent == full_content_zoom) {
        return;
    }
    m_private->m_isZoomFullContent = full_content_zoom;
    setZoomLevel(cur);
}

float
WKCWebView::opticalZoomLevel() const
{
    return m_private->opticalZoomLevel();
}
const WKCFloatPoint&
WKCWebView::opticalZoomOffset() const
{
    return m_private->opticalZoomOffset();
}

float
WKCWebView::setOpticalZoom(float zoom_level, const WKCFloatPoint& offset)
{
    m_private->setOpticalZoom(zoom_level, offset);
    return zoom_level;
}

void
WKCWebView::viewSize(WKCSize& size) const
{
    const WebCore::IntSize& s = m_private->viewSize();
    size.fWidth = s.width();
    size.fHeight = s.height();
}

const unsigned short*
WKCWebView::encoding()
{
    WTF::String encoding = m_private->core()->mainFrame().document()->charset();

    if (encoding.isEmpty()) {
        return 0;
    }
    if (m_private->m_encoding) {
        WTF::fastFree(m_private->m_encoding);
        m_private->m_encoding = 0;
    }
    m_private->m_encoding = wkc_wstrdup(encoding.charactersWithNullTermination().data());
    return m_private->m_encoding;
}
void
WKCWebView::setCustomEncoding(const unsigned short* encoding)
{
    m_private->core()->mainFrame().loader().reloadWithOverrideEncoding(WTF::String(encoding));
}
const unsigned short*
WKCWebView::customEncoding()
{
    WTF::String overrideEncoding = m_private->core()->mainFrame().loader().documentLoader()->overrideEncoding();

    if (overrideEncoding.isEmpty()) {
        return 0;
    }
    if (m_private->m_customEncoding) {
        WTF::fastFree(m_private->m_customEncoding);
        m_private->m_customEncoding = 0;
    }
    m_private->m_customEncoding = wkc_wstrdup(overrideEncoding.charactersWithNullTermination().data());
    return m_private->m_customEncoding;
}

WKC::LoadStatus
WKCWebView::loadStatus()
{
    return m_private->m_loadStatus;
}
double
WKCWebView::progress()
{
    return m_private->core()->progress().estimatedProgress();
}


bool
WKCWebView::hitTestResultForNode(const WKC::Node* node, WKC::HitTestResult& result)
{
    if (!node)
        return false;

    WebCore::Node* n = node->priv().webcore();
    WebCore::HitTestResult& hitTest = const_cast<WebCore::HitTestResult&>(result.priv()->webcore());

    hitTest.setLocalPoint(WebCore::IntPoint(0, 0));
    hitTest.setInnerNode(n);
    hitTest.setInnerNonSharedNode(n);

    if (n->hasTagName(WebCore::HTMLNames::areaTag)) {
        WebCore::HTMLImageElement* img = static_cast<WebCore::HTMLAreaElement*>(n)->imageElement();
        if (img)
            hitTest.setInnerNonSharedNode(img);
    }

    n = n->enclosingLinkEventParentOrSelf();
    if (n && n->isElementNode())
        hitTest.setURLElement(static_cast<WebCore::Element*>(n));

    return true;
}

void
WKCWebView::enterCompositingMode()
{
    m_private->enterCompositingMode();
}

void
WKCWebView::cachedSize(unsigned int& dead_resource, unsigned int& live_resource)
{
    live_resource = WebCore::MemoryCache::singleton().liveSize();
    dead_resource = WebCore::MemoryCache::singleton().deadSize();
}

void
WKCWebView::setPageCacheCapacity(int capacity)
{
    WebCore::PageCache::singleton().setMaxSize(capacity);
}

unsigned int
WKCWebView::getCachedPageCount()
{
    return WebCore::PageCache::singleton().pageCount();
}

void
WKCWebView::releaseMemory(ReleaseMemoryType type, bool is_synchronous)
{
    // Release Noncritical Memory

    if (type & EMemoryTypeNoncriticalInactiveFontData) {
        // Clear cache without the one used on a displayed page.
        WebCore::FontCache::singleton().purgeInactiveFontData();
    }

    if (type & EMemoryTypeNoncriticalFontWidthCache) {
        WebCore::clearWidthCaches();
    }

    if (type & EMemoryTypeNoncriticalSelectorQueryCache) {
        for (auto* document : WebCore::Document::allDocuments()) {
            document->clearSelectorQueryCache();
        }
    }

    if (type & EMemoryTypeNoncriticalMemoryCacheDeadResource) {
        WebCore::MemoryCache::singleton().pruneDeadResourcesToSize(0);
    }

#if ENABLE(WKC_HTTPCACHE)
    if (type & EMemoryTypeNoncriticalHTTPCache) {
        WebCore::ResourceHandleManager::sharedInstance()->resetHTTPCache();
    }
#endif

    if (type & EMemoryTypeNoncriticalXHRPreflightResultCache) {
        WebCore::CrossOriginPreflightResultCache::singleton().clear();
    }

    // Release Critical Memory

    if (type & EMemoryTypeCriticalPageCache) {
        WebCore::PageCache::singleton().pruneToSizeNow(0, WebCore::PruningReason::None);
    }

    if (type & EMemoryTypeCriticalMemoryCacheLiveResource) {
        WebCore::MemoryCache::singleton().pruneLiveResourcesToSize(0, /*shouldDestroyDecodedDataForAllLiveResources*/ is_synchronous);
    }

    if (type & EMemoryTypeCriticalCSSValuePool) {
        WebCore::CSSValuePool::singleton().drain();
    }

    if (type & EMemoryTypeCriticalStyleResolver) {
        WTF::Vector<WebCore::Document*> documents = WTF::copyToVector(WebCore::Document::allDocuments());
        for (auto& document : documents) {
            document->styleScope().clearResolver();
        }
    }

    if (type & EMemoryTypeCriticalJITCompiledCode) {
        WebCore::GCController::singleton().deleteAllCode(JSC::DeleteAllCodeEffort::PreventCollectionAndDeleteAllCode);
    }

    if (type & EMemoryTypeCriticalAllFontData) {
        // Clear cache completely, including even the one used on a displayed page.
        WebCore::FontCache::singleton().invalidate();
    }

    if (type & EMemoryTypeCriticalJSGarbage) {
        WKCWebKitRequestGarbageCollect(is_synchronous);
    }

    // FastMalloc has lock-free thread specific caches that can only be cleared from the thread itself.
    WebCore::WorkerThread::releaseFastMallocFreeMemoryInAllThreads();
    WTF::releaseFastMallocFreeMemory();
}

size_t
WKCWebView::fontCount()
{
    return WebCore::FontCache::singleton().fontCount();
}

size_t
WKCWebView::inactiveFontCount()
{
    return WebCore::FontCache::singleton().inactiveFontCount();
}

void
WKCWebView::clearFontCache(bool in_clearsAll)
{
    if (in_clearsAll) {
        // Clear cache completely, including even the one used on a displayed page.
        WebCore::FontCache::singleton().invalidate();
    } else {
        // Clear cache without the one used on a displayed page.
        WebCore::FontCache::singleton().purgeInactiveFontData();
    }
}

void
WKCWebView::clearEmojiCache(void)
{
    wkcDrawContextClearEmojiCachePeer();
}

void
WKCWebView::setPluginsFolder(const char* in_folder)
{
    wkcPluginSetPluginPathPeer(in_folder);
}

void
WKCWebView::setIsVisible(bool isVisible)
{
    m_private->setIsVisible(isVisible);
}

void
WKCWebKitSetPluginInstances(void* instance1, void* instance2)
{
    wkcPluginWindowSetInstancesPeer(instance1, instance2);
}

unsigned int
WKCWebView::getRSSLinkNum()
{
    WebCore::Document* doc = m_private->core()->mainFrame().document();
    RefPtr<WebCore::NodeList> list = doc->getElementsByTagName("link");
    unsigned len = list.get()->length();
    unsigned int ret = 0;
    
    if (len > 0) {
        for (unsigned i = 0; i < len; i++) {
            WebCore::HTMLLinkElement* link = static_cast<WebCore::HTMLLinkElement*>(list.get()->item(i));
            if (!link->getAttribute("href").isEmpty() && !link->getAttribute("href").isNull()) {
                if (equalIgnoringASCIICase(link->getAttribute("rel"), "alternate")) {
                    if (equalIgnoringASCIICase(link->getAttribute("type"), "application/rss+xml")
                        || equalIgnoringASCIICase(link->getAttribute("type"), "application/atom+xml")) {
                        ret ++;
                    }
                }
            }
        }
    }

    return ret;
}

unsigned int
WKCWebView::getRSSLinkInfo(WKCRSSLinkInfo* info, unsigned int info_len)
{
    WebCore::Document* doc = m_private->core()->mainFrame().document();
    RefPtr<WebCore::NodeList> list = doc->getElementsByTagName("link");
    unsigned int len = list.get()->length();
    unsigned int count = 0;

    if (info_len == 0)
        return 0;

    for (unsigned i = 0; i < len; i++) {
        WebCore::HTMLLinkElement* link = static_cast<WebCore::HTMLLinkElement*>(list.get()->item(i));
        if (!link->getAttribute("href").isEmpty() && !link->getAttribute("href").isNull()) {
            if (equalIgnoringASCIICase(link->getAttribute("rel"), "alternate")) {
                if (equalIgnoringASCIICase(link->getAttribute("type"), "application/rss+xml")
                    || equalIgnoringASCIICase(link->getAttribute("type"), "application/atom+xml")) {
                    /* get title and href attributes from DOM */   
                    (info + count)->m_flag = ERSSLinkFlagNone;
                    int title_len = link->getAttribute("title").length();
                    if (title_len > ERSSTitleLenMax) {
                        title_len = ERSSTitleLenMax;
                        (info + count)->m_flag |= ERSSLinkFlagTitleTruncate;
                    }
                    wkc_wstrncpy((info + count)->m_title, ERSSTitleLenMax, link->getAttribute("title").characters16(), title_len);

                    WebCore::URL url = doc->completeURL(link->getAttribute("href").string());
                    int url_len = url.string().utf8().length();
                    if (url_len > ERSSUrlLenMax) {
                        url_len = ERSSUrlLenMax;
                        (info + count)->m_flag |= ERSSLinkFlagUrlTruncate;
                    }
                    strncpy((info + count)->m_url, url.string().utf8().data(), url_len);
                    (info + count)->m_url[url_len] = 0;
                    
                    count ++;
                    if (count == info_len)
                        break;
                }
            }
        }
    }

    return count;
}

WKC::Element*
WKCWebView::getFocusedElement()
{
    return m_private->getFocusedElement();
}

WKC::Element*
WKCWebView::getElementFromPoint(int x, int y)
{
    return m_private->getElementFromPoint(x,y);
}

bool
WKCWebView::clickableFromPoint(int x, int y)
{
    WKC::Element* pelement = getElementFromPoint(x, y);
    return isClickableElement(pelement);
}

bool
WKCWebView::isClickableElement(WKC::Element* element)
{
    if (!element) {
        return false;
    }
    return m_private->core()->focusController().isClickableElement((WebCore::Element *)element->priv().webcore());
}

bool
WKCWebView::draggableFromPoint(int x, int y)
{
    WKC::Element* pelement = getElementFromPoint(x, y);
    if (!pelement)
        return false;
    WebCore::Element* element = (WebCore::Element *)pelement->priv().webcore();
    bool hasmousedown = false;
    while (element) {
        if (!hasmousedown)
            hasmousedown = element->hasEventListeners(WebCore::eventNames().mousedownEvent);
        if (element->hasEventListeners(WebCore::eventNames().dragEvent)
         || element->hasEventListeners(WebCore::eventNames().dragstartEvent)
         || element->hasEventListeners(WebCore::eventNames().dragendEvent)
         // In some contents there is mousemove event handler in parent node 
         // and mousedown event handler in child node. So we treat this node
         // as draggable.
         || (hasmousedown && element->hasEventListeners(WebCore::eventNames().mousemoveEvent))) {
            return true;
        }
        WebCore::RenderObject* renderer = element->renderer();
        if (renderer && renderer->isFrameSet()) {
            WebCore::RenderFrameSet* frameSetRenderer = WTF::downcast<WebCore::RenderFrameSet>(renderer);
            if (frameSetRenderer && (frameSetRenderer->isResizingRow() || frameSetRenderer->isResizingColumn()))
                return true;
        }
        if (element->isHTMLElement() && element->hasTagName(WebCore::HTMLNames::inputTag)) {
            WebCore::HTMLInputElement* ie = WTF::downcast<WebCore::HTMLInputElement>(element);
            if (ie && ie->isRangeControl())
                return true;
        }
        element = element->parentElement();
    }
    return false;
}

static bool
getScrollbarPartFromPoint(int x, int y, WebCore::Scrollbar& scrollbar, WebCore::FrameView* view, bool isFrameScrollbar, WKCWebView::ScrollbarPart& part, WebCore::IntRect& rect)
{
    WebCore::IntPoint mousePosition = scrollbar.convertFromContainingWindow(WebCore::IntPoint(x, y));
    mousePosition.move(scrollbar.x(), scrollbar.y());

    if (!scrollbar.frameRect().contains(mousePosition))
        return false;

    WebCore::IntRect track = WebCore::scrollbarTrackRect(scrollbar);
    if (track.contains(mousePosition)) {
        WebCore::IntRect beforeThumbRect;
        WebCore::IntRect thumbRect;
        WebCore::IntRect afterThumbRect;
        WebCore::scrollbarSplitTrack(scrollbar, track, beforeThumbRect, thumbRect, afterThumbRect);
        if (thumbRect.contains(mousePosition)) {
            part = WKCWebView::ThumbPart;
            rect = thumbRect;
        } else if (beforeThumbRect.contains(mousePosition)) {
            part = WKCWebView::BackTrackPart;
            rect = beforeThumbRect;
        } else if (afterThumbRect.contains(mousePosition)) {
            part = WKCWebView::ForwardTrackPart;
            rect = afterThumbRect;
        } else {
            part = WKCWebView::TrackBGPart;
            rect = track;
        }
    } else if (WebCore::scrollbarBackButtonRect(scrollbar, WebCore::BackButtonStartPart).contains(mousePosition)) {
        part = WKCWebView::BackButtonPart;
        rect = WebCore::scrollbarBackButtonRect(scrollbar, WebCore::BackButtonStartPart);
    } else if (WebCore::scrollbarBackButtonRect(scrollbar, WebCore::BackButtonEndPart).contains(mousePosition)) {
        part = WKCWebView::BackButtonPart;
        rect = WebCore::scrollbarBackButtonRect(scrollbar, WebCore::BackButtonEndPart);
    } else if (WebCore::scrollbarForwardButtonRect(scrollbar, WebCore::ForwardButtonStartPart).contains(mousePosition)) {
        part = WKCWebView::ForwardButtonPart;
        rect = WebCore::scrollbarForwardButtonRect(scrollbar, WebCore::ForwardButtonStartPart);
    } else if (WebCore::scrollbarForwardButtonRect(scrollbar, WebCore::ForwardButtonEndPart).contains(mousePosition)) {
        part = WKCWebView::ForwardButtonPart;
        rect = WebCore::scrollbarForwardButtonRect(scrollbar, WebCore::ForwardButtonEndPart);
    } else {
        part = WKCWebView::ScrollbarBGPart;
        rect = scrollbar.frameRect();
    }

    if (isFrameScrollbar)
        rect = view->convertToContainingWindow(rect);
    else
        rect = view->contentsToWindow(rect);

    return true;
}

static bool
isScrollbarOfFrameView(int x, int y, WebCore::FrameView* view, WKCWebView::ScrollbarPart& part, WKCRect& ir)
{
    WebCore::Scrollbar* scrollbar = view->verticalScrollbar();
    WebCore::IntRect rect(ir.fX, ir.fY, ir.fWidth, ir.fHeight);

    if (scrollbar) {
        if (getScrollbarPartFromPoint(x, y, *scrollbar, view, true, part, rect)) {
            ir = rect;
            return true;
        }
    }
    scrollbar = view->horizontalScrollbar();
    if (scrollbar) {
        if (getScrollbarPartFromPoint(x, y, *scrollbar, view, true, part, rect)) {
            ir = rect;
            return true;
        }
    }

    return false;
}

bool
WKCWebView::isScrollbarFromPoint(int x, int y, ScrollbarPart& part, WKCRect& ir)
{
    WebCore::Scrollbar* scrollbar = 0;
    WebCore::Frame* frame = &m_private->core()->mainFrame();
    WebCore::FrameView* view = 0;
    WebCore::IntRect rect;

    part = NoPart;

    while (frame) {
        WebCore::Document* doc = frame->document();
        view = frame->view();
        if (!view)
            return false;
        WebCore::IntPoint documentPoint = view->windowToContents(WebCore::IntPoint(x, y));
        WebCore::RenderView* renderView = doc->renderView();
        WebCore::HitTestRequest request(WebCore::HitTestRequest::ReadOnly | WebCore::HitTestRequest::Active);
        WebCore::HitTestResult result(documentPoint);

        renderView->layer()->hitTest(request, result);
        scrollbar = result.scrollbar();

        // Following is a part of MouseEventWithHitTestResults::targetNode() in order to get targetNode for subframe
        WebCore::Node* node = result.innerNode();
        if (!node)
            return false;
        if (!node->isConnected()) {
            WebCore::Element* element = node->parentElement();
            if (element && element->isConnected())
                node = element;
        }

        // Check if there is subframe
        frame = WebCore::EventHandler::subframeForTargetNode(node);
    }

    bool isScroll;
    if (scrollbar) {
        isScroll = getScrollbarPartFromPoint(x, y, *scrollbar, view, false, part, rect);
        ir = rect;
    } else {
        isScroll = isScrollbarOfFrameView(x, y , view, part, ir);
    }

    return isScroll;
}

// History
bool WKCWebView::addVisitedLink(const char* uri, const unsigned short* title, const struct tm* date)
{
    WebCore::Page* page = m_private->core();

    if (!page)
        return false;

    WebCore::URL url(WebCore::URL(), WTF::String::fromUTF8(uri));
    WebCore::SharedStringHash hash = WebCore::computeSharedStringHash(url);

    page->visitedLinkStore().addVisitedLink(*page, hash);

    return true;
}

bool WKCWebView::addVisitedLinkHash(SharedStringHash hash)
{
    WebCore::Page* page = m_private->core();

    if (!page)
        return false;

    page->visitedLinkStore().addVisitedLink(*page, hash);

    return true;
}

void WKCWebView::setInternalColorFormat(int fmt)
{
#if USE(WKC_CAIRO)
    switch (fmt) {
    case EInternalColorFormat8888:
        WebCore::ImageWKC::setInternalColorFormatARGB8888(false);
        break;
    case EInternalColorFormat8888or565:
        WebCore::ImageWKC::setInternalColorFormatARGB8888(true);
        break;
    }
#endif
}

void WKCWebView::setUseAntiAliasForDrawings(bool flag)
{
    m_private->setUseAntiAliasForDrawings(flag);
}

void WKCWebView::setUseAntiAliasForCanvas(bool flag)
{
    WKCWebViewPrivate::setUseAntiAliasForCanvas(flag);
}

void WKCWebView::setUseBilinearForScaledImages(bool flag)
{
    m_private->setUseBilinearForScaledImages(flag);
}

void WKCWebView::setUseBilinearForCanvasImages(bool flag)
{
    WKCWebViewPrivate::setUseBilinearForCanvasImages(flag);
}

void WKCWebView::setCookieEnabled(bool flag)
{
    m_private->core()->settings().setCookieEnabled(flag);
}

bool WKCWebView::cookieEnabled()
{
    return m_private->core()->settings().cookieEnabled();
}

void WKCWebKitClearCookies(void)
{
    WebCore::ResourceHandleManager::sharedInstance()->clearCookies();
}

int WKCWebKitCookieSerialize(char* buff, int bufflen)
{
    return WebCore::ResourceHandleManager::sharedInstance()->CookieSerialize(buff, bufflen);
}

void WKCWebKitCookieDeserialize(const char* buff, bool restart)
{
    WebCore::ResourceHandleManager::sharedInstance()->CookieDeserialize(buff, restart);
}

int WKCWebKitCookieGet(const char* uri, char* buf, unsigned int len)
{
    WebCore::URL ki(WebCore::URL(), WTF::String::fromUTF8(uri));
    WTF::String domain = ki.host().toString();
    WTF::String path = ki.path();
    bool secure = ki.protocolIs("https");

    WTF::Vector<WebCore::Cookie> rawCookiesList;
    bool rawCookiesImplemented = WebCore::ResourceHandleManager::sharedInstance()->getRawCookies(domain, path, secure, rawCookiesList);
    if (!rawCookiesImplemented) {
        return 0;
    }

    char* pbuf = buf;
    unsigned int remaining_buf_len = len;
    unsigned int cookies_len = 0;
    unsigned int cookies_size = rawCookiesList.size();

    for (int i = 0; i < cookies_size; i++) {
        unsigned int name_len = rawCookiesList[i].name.utf8().length();
        unsigned int value_len = rawCookiesList[i].value.utf8().length();
        unsigned int cookie_len = name_len + value_len + 2; // 2: length of '=' and ';'.

        if (name_len == 0)
            continue;

        cookies_len += cookie_len;

        if (!pbuf)
            continue;

        if (cookie_len > remaining_buf_len)
            break;

        strncpy(pbuf, rawCookiesList[i].name.utf8().data(), name_len);
        pbuf[name_len] = '=';
        pbuf += name_len + 1;
        remaining_buf_len -= name_len + 1;

        if (value_len > 0) {
            strncpy(pbuf, rawCookiesList[i].value.utf8().data(), value_len);
            pbuf += value_len;
            remaining_buf_len -= value_len;
        }

        *pbuf = ';';
        pbuf += 1;
        remaining_buf_len -= 1;
    }

    if (!pbuf)
        return cookies_len;

    if (pbuf - buf > 0) {
        pbuf -= 1;
        *pbuf = '\0'; // replace the last ';' with null terminator.
    }

    return pbuf - buf;
}

WKC_API void
WKCWebKitCookieSet(const char* uri, const char* cookie)
{
    WebCore::URL ki(WebCore::URL(), WTF::String::fromUTF8(uri));
    WTF::String domain = ki.host().toString();
    WTF::String path = ki.path();

    WebCore::ResourceHandleManager::sharedInstance()->setCookie(domain, path, WTF::String::fromUTF8(cookie));
}

int WKCWebKitCurrentWebSocketConnectionsNum(void)
{
	WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
	if (mgr)
		return mgr->getCurrentWebSocketConnectionsNum();
	else
		return -1;
}

void WKCWebKitSetDNSPrefetchProc(void(*requestprefetchproc)(const char*), void* resolverlocker)
{
    wkcNetSetPrefetchDNSCallbackPeer(requestprefetchproc, resolverlocker);
}

void WKCWebKitCachePrefetchedDNSEntry(const char* name, const unsigned char* ipaddr)
{
    wkcNetCachePrefetchedDNSEntryPeer(name, ipaddr);
}

int WKCWebKitGetNumberOfSockets(void)
{
    return wkcNetGetNumberOfSocketsPeer();
}

int WKCWebKitGetSocketStatistics(int in_numberOfArray, SocketStatistics* out_statistics)
{
    return wkcNetGetSocketStatisticsPeer(in_numberOfArray, (WKCSocketStatistics*)out_statistics);
}

void WKCWebKitSetIsNetworkAvailableProc(bool(*in_proc)(void))
{
    wkcNetSetIsNetworkAvailableCallbackPeer(in_proc);
}

void WKCWebView::permitSendRequest(void *handle, bool permit)
{
    WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
    if (mgr)
        mgr->permitRequest(handle, permit);
}

// global initialize / finalize

class WKCWebKitMemoryEventHandler {
public:
    WKCWebKitMemoryEventHandler(WKCMemoryEventHandler& handler)
         : m_memoryEventHandler(handler) {};
    ~WKCWebKitMemoryEventHandler() {};

    bool checkMemoryAvailability(unsigned int request_size, bool forimage) {
        return m_memoryEventHandler.checkMemoryAvailability(request_size, forimage);
    }
    bool checkMemoryAllocatable(unsigned int request_size, WKCMemoryEventHandler::AllocationReason reason) {
        return m_memoryEventHandler.checkMemoryAllocatable(request_size, reason);
    }
    void* notifyMemoryExhaust(unsigned int request_size, unsigned int& allocated_size) {
        return m_memoryEventHandler.notifyMemoryExhaust(request_size, allocated_size);
    }
    void notifyMemoryAllocationError(unsigned int request_size, WKCMemoryEventHandler::AllocationReason reason) {
        m_memoryEventHandler.notifyMemoryAllocationError(request_size, reason);
    }
    void notifyCrash(const char* file, int line, const char* function, const char* assertion) {
        m_memoryEventHandler.notifyCrash(file, line, function, assertion);
    }
    void notifyStackOverflow(bool need_restart, unsigned int stack_size, unsigned int consumption, unsigned int margin, void* stack_top, void* stack_base, void* current_stack_top, const char* file, int line, const char* function) {
        m_memoryEventHandler.notifyStackOverflow(need_restart, stack_size, consumption, margin, stack_top, stack_base, current_stack_top, file, line, function);
    }

private:
    WKCMemoryEventHandler& m_memoryEventHandler;
};

static void
WKCWebkitPrepareRestart()
{
    if (!wkcThreadCurrentIsMainThreadPeer()) {
        if (WebCore::ResourceHandleManager::isExistSharedInstance() &&
            WebCore::ResourceHandleManager::sharedInstance()->isNetworkThread()) {
            WebCore::ResourceHandleManager::sharedInstance()->notifyRequestRestartInNetworkThread();
        }
    }
}

static unsigned char gMemoryEventHandlerInstance[sizeof(WKCWebKitMemoryEventHandler)];
static WKCWebKitMemoryEventHandler* gMemoryEventHandler = (WKCWebKitMemoryEventHandler *)&gMemoryEventHandlerInstance;
static void*
WKCWebKitNotifyNoMemory(unsigned int request_size)
{
    unsigned int dummy = 0;

    // Kill Timer
    WebCore::MainThreadSharedTimer::singleton().stop();

    // Kill Network Thread
    if (wkcThreadCurrentIsMainThreadPeer()) {
        WebCore::ResourceHandleManager::forceTerminateInstance();
    }
    WKCWebkitPrepareRestart();

    return gMemoryEventHandler->notifyMemoryExhaust(request_size, dummy);
}

static FontNoMemoryProc gFontNoMemoryProc = 0;

static void
WKCWebKitNotifyFontNoMemory()
{
    if (gFontNoMemoryProc) {
        (gFontNoMemoryProc)();
    }
}

static WKCMemoryEventHandler::AllocationReason
WKCWebKitConvertAllocationReason(int in_reason)
{
    WKCMemoryEventHandler::AllocationReason result;

    switch (in_reason) {
    case WKC_MEMORYALLOC_TYPE_IMAGE:
        result = WKCMemoryEventHandler::Image;
        break;
    case WKC_MEMORYALLOC_TYPE_JAVASCRIPT:
        result = WKCMemoryEventHandler::JavaScript;
        break;
#if USE(WKC_CAIRO)
    case WKC_MEMORYALLOC_TYPE_PIXMAN:
        result = WKCMemoryEventHandler::Pixman;
        break;
#endif
    default:
        ASSERT_NOT_REACHED();
    }
    return result;
}

static void
WKCWebKitNotifyMemoryAllocationError(unsigned int request_size, int in_reason)
{
    gMemoryEventHandler->notifyMemoryAllocationError(request_size, WKCWebKitConvertAllocationReason(in_reason));
}

static void
WKCWebKitNotifyCrash(const char* file, int line, const char* function, const char* assertion)
{
    WKCWebkitPrepareRestart();
    gMemoryEventHandler->notifyCrash(file, line, function, assertion);
}

static void
WKCWebKitNotifyStackOverflow(bool need_restart, unsigned int stack_size, unsigned int consumption, unsigned int margin, void* stack_top, void* stack_base, void* current_stack_top, const char* file, int line, const char* function)
{
    if (need_restart) {
        WKCWebkitPrepareRestart();
    }
    gMemoryEventHandler->notifyStackOverflow(need_restart, stack_size, consumption, margin, stack_top, stack_base, current_stack_top, file, line, function);
}

static bool
WKCWebKitCheckMemoryAvailability(unsigned int size, bool forimage)
{
    return true;
}

static bool
WKCWebKitCheckMemoryAllocatable(unsigned int request_size, int in_reason)
{
    return gMemoryEventHandler->checkMemoryAllocatable(request_size, WKCWebKitConvertAllocationReason(in_reason));
}

static void*
peer_malloc_proc(unsigned int in_size, int in_crashonfailure)
{
    void* ptr = 0;

    if (in_crashonfailure) {
        ptr = WTF::fastMalloc(in_size);
    } else {
        WTF::TryMallocReturnValue rv = tryFastMalloc(in_size);
        if (!rv.getValue(ptr)) {
            return 0;
        }
    }
    return ptr;
}

static void*
peer_malloc_aligned_proc(unsigned int in_size, unsigned int in_align)
{
    void* ptr = 0;
    ptr = WTF::fastAlignedMalloc(in_align, in_size);
    return ptr;
}

static void*
font_peer_malloc_proc(int in_size)
{
    return WTF::fastMalloc(in_size);
}

static void
peer_free_proc(void* in_ptr)
{
    WTF::fastFree(in_ptr);
}

static void*
peer_realloc_proc(void* in_ptr, unsigned int in_size, int in_crashonfailure)
{
    void* ptr = 0;

    if (in_crashonfailure) {
        ptr = WTF::fastRealloc(in_ptr, in_size);
    } else {
        WTF::TryMallocReturnValue rv = WTF::tryFastRealloc(in_ptr, in_size);
        if (!rv.getValue(ptr)) {
            return 0;
        }
    }
    return ptr;
}

static void
peer_free_aligned_proc(void* in_ptr)
{
    WTF::fastAlignedFree(in_ptr);
}

class WKCWebKitTimerEventHandler {
public:
    WKCWebKitTimerEventHandler(WKCTimerEventHandler& handler)
        : m_timerEventHandler(handler) {};
    ~WKCWebKitTimerEventHandler() {};

    bool requestWakeUp(void* in_timer, bool(*in_proc)(void*), void* in_data) {
        return m_timerEventHandler.requestWakeUp(in_timer, in_proc, in_data);
    };
    void cancelWakeUp(void* in_timer) {
        return m_timerEventHandler.cancelWakeUp(in_timer);
    };
private:
    WKCTimerEventHandler& m_timerEventHandler;
};

static unsigned char gTimerEventHandlerInstance[sizeof(WKCWebKitTimerEventHandler)];
static WKCWebKitTimerEventHandler* gTimerEventHandler = (WKCWebKitTimerEventHandler *)&gTimerEventHandlerInstance;

static bool
WKCWebKitRequestWakeUp(void* in_timer, wkcTimeoutProc in_proc, void* in_data)
{
    return gTimerEventHandler->requestWakeUp(in_timer, in_proc, in_data);
}

static void
WKCWebKitCancelWakeUp(void* in_timer)
{
    gTimerEventHandler->cancelWakeUp(in_timer);
}

WKC_API void
WKCWebKitWakeUp(void* in_timer, void* in_data)
{
    wkcTimerWakeUpPeer(in_timer, in_data);
}

unsigned int
WKCWebKitGetTickCount()
{
    return wkcGetTickCountPeer();
}

void
WKCWebkitRegisterGetCurrentTimeCallback(WKC::GetCurrentTimeProc in_proc)
{
    wkcRegisterGetCurrentTimeProcPeer(in_proc);
}

bool
WKCWebKitIsMemoryCrashing()
{
    return wkcMemoryIsCrashingPeer();
}

static void WKCResetVariables();

bool
shouldInterruptJavaScript(JSC::ExecState*, void*, void*)
{
    if (!gValidViews || !gValidViews->size() || !gValidViews->first() || !gValidViews->first()->core())
        return true;
    return gValidViews->first()->core()->chrome().client().shouldInterruptJavaScript();
}

bool
WKCWebKitInitialize(void* memory, size_t physical_memory_size, size_t virtual_memory_size, void* font_memory, size_t font_memory_size, WKCMemoryEventHandler& memory_event_handler, WKCTimerEventHandler& timer_event_handler)
{
    NX_DP(("WKCWebKitInitialize Enter\n"));

    WKCResetVariables();

#if USE(MUTEX_DEBUG_LOG)
    InitializeCriticalSection(&gCriticalSection);
    gCriticalSectionFlag = true;
#endif

    if (!wkcSystemInitializePeer())
        return false;

    if (!wkcDebugPrintInitializePeer())
        return false;

    new (gMemoryEventHandler) WKCWebKitMemoryEventHandler(memory_event_handler);
    new (gTimerEventHandler) WKCWebKitTimerEventHandler(timer_event_handler);

    if (!wkcMemoryInitializePeer(peer_malloc_proc, peer_free_proc, peer_realloc_proc, peer_malloc_aligned_proc, peer_free_aligned_proc))
        return false;

    wkcMemorySetNotifyNoMemoryProcPeer(WKCWebKitNotifyNoMemory);
    wkcMemorySetNotifyMemoryAllocationErrorProcPeer(WKCWebKitNotifyMemoryAllocationError);
    wkcMemorySetNotifyCrashProcPeer(WKCWebKitNotifyCrash);
    wkcMemorySetNotifyStackOverflowProcPeer(WKCWebKitNotifyStackOverflow);
    wkcMemorySetCheckMemoryAllocatableProcPeer(WKCWebKitCheckMemoryAllocatable);

    if (!wkcThreadInitializePeer())
        return false;

#if ENABLE(WEBGL)
    if (!wkcGLInitializePeer())
        return false;
#endif

    wkcLayerInitializePeer();
    wkcHeapInitializePeer(memory, physical_memory_size, virtual_memory_size);

    if (!wkcHWOffscreenInitializePeer())
        return false;

    if (!wkcAudioInitializePeer())
        return false;
    if (!wkcMediaPlayerInitializePeer())
        return false;
    if (!wkcPluginInitializePeer())
        return false;

    if (!wkcTimerInitializePeer(WKCWebKitRequestWakeUp, WKCWebKitCancelWakeUp))
        return false;

    if (!wkcFontEngineInitializePeer(font_memory, font_memory_size, (fontPeerMalloc)font_peer_malloc_proc, (fontPeerFree)peer_free_proc, true))
        return false;

    wkcFontSetNotifyNoMemoryProcPeer(WKCWebKitNotifyFontNoMemory);

    wkcDrawContextInitializePeer();

    JSC::initializeThreading();
    RunLoop::initializeMainRunLoop();
    WorkQueue::initialize();
#if !LOG_DISABLED
    WebCore::initializeLogChannelsIfNecessary();
#endif
    JSC::Options::forceRAMSize() = wkcHeapGetPhysicalHeapTotalSizePeer();

    if (!wkcFileInitializePeer())
        return false;
    if (!wkcNetInitializePeer())
        return false;
    if (!wkcSSLInitializePeer())
        return false;

    wkcFileCallbackSetPeer(static_cast<const WKCFileProcs *>(WKCWebView::EPUB::EPUBFileProcsWrapper(wkcFileCallbackGetPeer())));

    WebCore::atomicCanonicalTextEncodingName("UTF-8");

    WebCore::SQLiteFileSystem::registerSQLiteVFS();

    WKC::WKCPrefs::initialize();

    if (!WKC::WKCWebView::EPUB::initialize())
        return false;

    if (!WKCGlobalSettings::isExistSharedInstance())
        if (!WKCGlobalSettings::createSharedInstance(true))
            return false;

    if (!WebCore::ResourceHandleManager::isExistSharedInstance())
        if (!WebCore::ResourceHandleManager::createSharedInstance())
            return false;

    // cache
    WebCore::MemoryCache::singleton().setCapacities(0, 0, 1*1024*1024);
    WTF::Seconds v(0);
    WebCore::MemoryCache::singleton().setDeadDecodedDataDeletionInterval(v);
    WKC::WKCPrefs::setMinDelayBeforeLiveDecodedPruneCaches(1);
    WebCore::PageCache::singleton().setMaxSize(0); /* dont use page cache now*/

    WKC::PlatformStrategiesWKC::initialize();

    WebCore::PlatformScreenInitialize();

#if ENABLE(JIT)
    JSC::Options::useLLInt() = false;
#endif

    {
        JSC::VM& vm = WebCore::commonVM();
        JSC::JSLockHolder locker(&vm);
        vm.ensureWatchdog();
        // We use default timer settings.
    }

    NX_DP(("WKCWebKitInitialize Exit\n"));
    return true;
}

void
WKCWebKitFinalize()
{
#if ENABLE(REMOTE_INSPECTOR)
    if (WebInspectorServer::sharedInstance())
        WebInspectorServer::deleteSharedInstance();
#endif

    if (WebCore::ResourceHandleManager::isExistSharedInstance())
        WebCore::ResourceHandleManager::deleteSharedInstance();

    WorkQueue::finalize();
    WTF::finalizeMainThreadPlatform();

    WKC::WKCWebView::EPUB::finalize();
    WKC::WKCPrefs::finalize();

    wkcHWOffscreenFinalizePeer();
    wkcPluginFinalizePeer();
    wkcMediaPlayerFinalizePeer();
    wkcAudioFinalizePeer();
    wkcLayerFinalizePeer();
#if ENABLE(WEBGL)
    wkcGLFinalizePeer();
#endif
    wkcSSLFinalizePeer();
    wkcFontEngineFinalizePeer();
    wkcSystemFinalizePeer();
    wkcNetFinalizePeer();
    wkcFileFinalizePeer();
    wkcTimerFinalizePeer();

    wkcDrawContextFinalizePeer();

    wkcHeapFinalizePeer();

    gTimerEventHandler->~WKCWebKitTimerEventHandler();
    memset(gTimerEventHandler, 0, sizeof(WKCWebKitTimerEventHandler));
    gMemoryEventHandler->~WKCWebKitMemoryEventHandler();
    memset(gMemoryEventHandler, 0, sizeof(WKCWebKitMemoryEventHandler));

    wkcThreadFinalizePeer();
    wkcMemoryFinalizePeer();

    wkcDebugPrintFinalizePeer();

#if USE(MUTEX_DEBUG_LOG)
    DeleteCriticalSection(&gCriticalSection);
    gCriticalSectionFlag = false;
#endif
}

bool
WKCWebKitSuspendFont()
{
    wkcFontEngineFinalizePeer();
    return true;
}

void
WKCWebKitResumeFont(void* font_memory, unsigned int font_memory_size)
{
    wkcFontEngineInitializePeer(font_memory, font_memory_size, (fontPeerMalloc)font_peer_malloc_proc, (fontPeerFree)peer_free_proc, true);
}

unsigned int WKCWebKitFontHeapSize()
{
    return wkcFontEngineHeapSizePeer();
}

int WKCWebKitRegisterFontOnMemory(const unsigned char* memPtr, unsigned int len)
{
    return wkcFontEngineRegisterSystemFontPeer(WKC_FONT_ENGINE_REGISTER_TYPE_MEMORY, memPtr, len);
}

int WKCWebKitRegisterFontInFile(const char* filePath)
{
    if (!filePath)
        return -1;

    return wkcFontEngineRegisterSystemFontPeer(WKC_FONT_ENGINE_REGISTER_TYPE_FILE, (const unsigned char *)filePath, ::strlen(filePath));
}

void WKCWebKitUnregisterFonts()
{
    wkcFontEngineUnregisterFontsPeer();
}

bool WKCWebKitSetFontScale(int id, float scale)
{
    return wkcFontEngineSetFontScalePeer(id, scale);
}

void
WKCWebKitSetFontNoMemoryCallback(FontNoMemoryProc proc)
{
    gFontNoMemoryProc = proc;
}

void
WKCWebKitSetHWOffscreenDeviceParams(const HWOffscreenDeviceParams* params, void* opaque)
{
    WKCHWOffscreenParams procs = {
        params->fLockProc,
        params->fUnlockProc,
        params->fEnable,
        params->fEnableForImagebuffer,
        params->fScreenWidth,
        params->fScreenHeight
    };
    wkcHWOffscreenSetParamsPeer(&procs, opaque);
#if ENABLE(WEBGL)
    wkcGLRegisterDeviceLockProcsPeer(params->fLockProc, params->fUnlockProc, opaque);
#endif
}

void
WKCWebKitSetLayerCallbacks(const LayerCallbacks* callbacks)
{
    wkcLayerInitializeCallbacksPeer(callbacks->fTextureMakeProc,
                                    callbacks->fTextureDeleteProc,
                                    callbacks->fTextureUpdateProc,
                                    callbacks->fTextureChangeProc,
                                    callbacks->fDidChangeParentProc,
                                    callbacks->fCanAllocateProc,
                                    callbacks->fAttachedGlTexturesProc);
}

void
WKCWebKitSetWebGLTextureCallbacks(const WebGLTextureCallbacks* callbacks)
{
#if ENABLE(WEBGL)
    wkcGLRegisterTextureCallbacksPeer(callbacks->fTextureMakeProc,
                                        callbacks->fTextureDeleteProc,
                                        callbacks->fTextureChangeProc);
#endif
}

void
WKCWebKitGetLayerProperties(void* layer, void** opaque_texture, int* width, int* height, bool* need_yflip, void** out_offscreen, int* offscreenwidth, int* offscreenheight, bool* is_3dcanvas)
{
    if (opaque_texture)
        *opaque_texture = wkcLayerGetOpaqueTexturePeer(layer);
    if (width && height)
        wkcLayerGetAllocatedSizePeer(layer, width, height);
    if (need_yflip)
        *need_yflip = wkcLayerIsNeededYFlipPeer(layer);
    if (out_offscreen)
        *out_offscreen = wkcLayerGetOffscreenPeer(layer);
    if (offscreenwidth && offscreenheight)
        wkcLayerGetOriginalSizePeer(layer, offscreenwidth, offscreenheight);
    if (is_3dcanvas) {
        *is_3dcanvas = (wkcLayerGetTypePeer(layer) == WKC_LAYER_TYPE_3DCANVAS) ? true : false;
    }
}

void*
WKCWebKitOffscreenNew(OffscreenFormat format, void* bitmap, int rowbytes, const WKCSize* size)
{
    int pformat = 0;

    switch (format) {
    case EOffscreenFormatPolygon:
        pformat = WKC_OFFSCREEN_TYPE_POLYGON;
        break;
    case EOffscreenFormatCairo16:
        pformat = WKC_OFFSCREEN_TYPE_CAIRO16;
        break;
    case EOffscreenFormatCairo32:
        pformat = WKC_OFFSCREEN_TYPE_CAIRO32;
        break;
    case EOffscreenFormatCairoSurface:
        pformat = WKC_OFFSCREEN_TYPE_CAIROSURFACE;
        break;
    default:
        return 0;
    }
    return wkcOffscreenNewPeer(pformat, bitmap, rowbytes, size);
}

void
WKCWebKitOffscreenDelete(void* offscreen)
{
    wkcOffscreenDeletePeer(offscreen);
}

bool
WKCWebKitOffscreenIsError(void* offscreen)
{
#if USE(WKC_CAIRO)
    return wkcOffscreenIsErrorPeer(offscreen);
#else
    return false;
#endif
}

void*
WKCWebKitDrawContextNew(void* offscreen)
{
    return wkcDrawContextNewPeer(offscreen);
}

void
WKCWebKitDrawContextDelete(void* context)
{
    wkcDrawContextDeletePeer(context);
}

bool
WKCWebKitDrawContextIsError(void* context)
{
#if USE(WKC_CAIRO)
    return wkcDrawContextIsErrorPeer(context);
#else
    return false;
#endif
}

// SSL
void* WKCWebKitSSLRegisterRootCA(const char* cert, int cert_len)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->SSLRegisterRootCA(cert, cert_len);
    }
    return NULL;
}

void* WKCWebKitSSLRegisterRootCAByDER(const char* cert, int cert_len)
{
    using WebCore::ResourceHandleManagerSSL;
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->SSLRegisterRootCAByDER(cert, cert_len);
    }
    return NULL;
}

int WKCWebKitSSLUnregisterRootCA(void* certid)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->SSLUnregisterRootCA(certid);
    }
    return -1;
}

void WKCWebKitSSLRootCADeleteAll(void)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        mgr->SSLRootCADeleteAll();
    }
}

void* WKCWebKitSSLRegisterCRL(const char* crl, int crl_len)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->SSLRegisterCRL(crl, crl_len);
    }
    return NULL;
}

int WKCWebKitSSLUnregisterCRL(void* crlid)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->SSLUnregisterCRL(crlid);
    }
    return -1;
}

void WKCWebKitSSLCRLDeleteAll(void)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        mgr->SSLCRLDeleteAll();
    }
}

void* WKCWebKitSSLRegisterClientCert(const unsigned char* pkcs12, int pkcs12_len, const unsigned char* pass, int pass_len)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->SSLRegisterClientCert(pkcs12, pkcs12_len, pass, pass_len);
    }
    return NULL;
}

void* WKCWebKitSSLRegisterClientCertByDER(const unsigned char* cert, int cert_len, const unsigned char* key, int key_len)
{
    using WebCore::ResourceHandleManagerSSL;
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->SSLRegisterClientCertByDER(cert, cert_len, key, key_len);
    }
    return NULL;
}

int WKCWebKitSSLUnregisterClientCert(void* certid)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->SSLUnregisterClientCert(certid);
    }
    return -1;
}

void WKCWebKitSSLClientCertDeleteAll(void)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        mgr->SSLClientCertDeleteAll();
    }
}

bool WKCWebKitSSLRegisterBlackCert(const char* issuerName, const char* SerialNumber)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->SSLRegisterBlackCert(issuerName, SerialNumber);
    }
    return false;
}

bool WKCWebKitSSLRegisterBlackCertByDER(const char* cert, int cert_len)
{
    using WebCore::ResourceHandleManagerSSL;
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->SSLRegisterBlackCertByDER(cert, cert_len);
    }
    return false;
}

void WKCWebKitSSLBlackCertDeleteAll(void)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        mgr->SSLBlackCertDeleteAll();
    }
}

bool WKCWebKitSSLRegisterUntrustedCertByDER(const char* cert, int cert_len)
{
    using WebCore::ResourceHandleManagerSSL;
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->SSLRegisterUntrustedCertByDER(cert, cert_len);
    }
    return false;
}

bool WKCWebKitSSLRegisterEVSSLOID(const char *issuerCommonName, const char *OID, const char *sha1FingerPrint, const char *SerialNumber)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->SSLRegisterEVSSLOID(issuerCommonName, OID, sha1FingerPrint, SerialNumber);
    }
    return false;
}

void WKCWebKitSSLEVSSLOIDDeleteAll(void)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        mgr->SSLEVSSLOIDDeleteAll();
    }
}

void WKCWebKitSSLSetAllowServerHost(const char *host_w_port)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        mgr->setAllowServerHost(host_w_port);
    }
}

const char** WKCWebKitSSLGetServerCertChain(const char* in_url, int& out_num)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        return mgr->getServerCertChain(in_url, out_num);
    }
    return (const char**)0;
}

void WKCWebKitSSLFreeServerCertChain(const char** chain, int num)
{
    if (WebCore::ResourceHandleManager::sharedInstance()) {
        WebCore::ResourceHandleManager* mgr = WebCore::ResourceHandleManager::sharedInstance();
        mgr->freeServerCertChain(chain, num);
    }
}

// File System
void WKCWebKitSetFileSystemProcs(const WKC::FileSystemProcs* procs)
{
    wkcFileCallbackSetPeer(static_cast<const WKCFileProcs *>(procs));
}

// Media Player
void WKCWebKitSetMediaPlayerProcs(const WKC::MediaPlayerProcs* procs)
{
    wkcMediaPlayerCallbackSetPeer(static_cast<const WKCMediaPlayerProcs *>(procs));
}

// Pasteboard
void WKCWebKitSetPasteboardProcs(const WKC::PasteboardProcs* procs)
{
    wkcPasteboardCallbackSetPeer(static_cast<const WKCPasteboardProcs *>(procs));
}

// Thread
void WKCWebKitSetThreadProcs(const WKC::ThreadProcs* procs)
{
    wkcThreadCallbackSetPeer(static_cast<const WKCThreadProcs *>(procs));
}

// glyph / image cache
bool WKCWebKitSetGlyphCache(int format, void* cache, const WKCSize* size)
{
    bool ret = false;
    if (cache) {
        ret = wkcOffscreenCreateGlyphCachePeer(format, cache, size);
        if (!ret) return false;
        ret = wkcHWOffscreenCreateGlyphCachePeer(format, cache, size);
        if (!ret) {
            wkcOffscreenDeleteGlyphCachePeer();
        }
        return ret;
    } else {
       wkcHWOffscreenDeleteGlyphCachePeer();
       wkcOffscreenDeleteGlyphCachePeer();
       return true;
    }
}
 
bool WKCWebKitSetImageCache(int format, void* cache, const WKCSize* size)
{
    bool ret = false;
    if (cache) {
        ret = wkcOffscreenCreateImageCachePeer(format, cache, size);
        if (!ret) return false;
        ret = wkcHWOffscreenCreateImageCachePeer(format, cache, size);
        if (!ret) {
            wkcOffscreenDeleteImageCachePeer();
        }
        return ret;
    } else {
       wkcHWOffscreenDeleteImageCachePeer();
       wkcOffscreenDeleteImageCachePeer();
       return true;
    }
}

void
WKCWebKitForceTerminate()
{
    NX_DP(("WKCWebKitForceTerminate Enter\n"));

    if (WebCore::ResourceHandleManager::isExistSharedInstance())
        WebCore::ResourceHandleManager::forceTerminateInstance();

    if (WKCGlobalSettings::isAutomatic())
        WKCGlobalSettings::deleteSharedInstance();

#if ENABLE(REMOTE_INSPECTOR)
    if (WebInspectorServer::sharedInstance())
        WebInspectorServer::forceTerminate();
#endif

    WorkQueue::forceTerminate();

    wkcMediaPlayerForceTerminatePeer();

    WKC::WKCPrefs::forceTerminate();
    WKC::WKCWebViewPrefs::forceTerminate();

    wkc_sqlite3_force_terminate();

    wkcFontEngineForceTerminatePeer();
    wkcPluginForceTerminatePeer();
    wkcAudioForceTerminatePeer();
    wkcHWOffscreenForceTerminatePeer();
    wkcLayerForceTerminatePeer();
#if ENABLE(WEBGL)
    wkcGLForceTerminatePeer();
#endif
    wkcTextBreakIteratorForceTerminatePeer();
    wkcOffscreenForceTerminatePeer();
    wkcDrawContextForceTerminatePeer();
    wkcSSLForceTerminatePeer();
    wkcNetForceTerminatePeer();

    wkcThreadForceTerminatePeer();

    WKC::WKCWebView::EPUB::forceTerminate();
    wkcFileForceTerminatePeer();
    wkcTimerForceTerminatePeer();
    wkcHeapForceTerminatePeer();
    wkcMemoryForceTerminatePeer();
    wkcDebugPrintForceTerminatePeer();
    memset(gTimerEventHandler, 0, sizeof(WKCWebKitTimerEventHandler));
    memset(gMemoryEventHandler, 0, sizeof(WKCWebKitMemoryEventHandler));

    NX_DP(("WKCWebKitForceTerminate Exit\n"));
}

void
WKCWebKitForceFinalize()
{
    WKCWebKitForceTerminate();
}

void
WKCWebKitResetMaxHeapUsage()
{
    wkcHeapResetMaxHeapUsagePeer();
}

static void
WKCResetVariables()
{
    wkc_libxml2_resetVariables();
#if ENABLE(XSLT)
    wkc_libxslt_resetVariables();
#endif
    wkc_pixman_resetVariables();
    wkc_cairo_resetVariables();
#if ENABLE(GRAPHICS_CONTEXT_3D)
    ShResetVariables();
#endif
    WebCore::resetCommonVM();
}

void
WKCWebKitResetGPU()
{
}

unsigned int
WKCWebKitAvailableMemory()
{
    NX_DP(("WKC::WKCWebKitAvailableMemory() is deprecated. Use WKC::Heap::GetAvailableSize() instead.\n"));
    return Heap::GetAvailableSize();
}

unsigned int
WKCWebKitMaxAvailableBlock()
{
    NX_DP(("WKC::WKCWebKitMaxAvailableBlock() is deprecated. Use WKC::Heap::GetMaxAvailableBlockSize() instead.\n"));
    return Heap::GetMaxAvailableBlockSize();
}
void
WKCWebKitRequestGarbageCollect(bool is_now)
{
    if (is_now) {
        WebCore::GCController::singleton().garbageCollectNow();
        WebCore::GCController::singleton().releaseFreeRegionsInHeap();
    } else { 
        WebCore::GCController::singleton().garbageCollectSoon();
    }
}

extern "C" void wkcMediaPlayerSetAudioResourcesPathPeer(const char* in_path);

void
WKCWebKitSetWebAudioResourcePath(const char* path)
{
    wkcMediaPlayerSetAudioResourcesPathPeer(path);
}

void
WKCWebViewPrivate::addOverlay(WKCOverlayIf* overlay, int zOrder, int fixedDirectionFlag)
{
    if (!m_overlayList)
        m_overlayList = WKCOverlayList::create(this);

    m_overlayList->add(overlay, zOrder, fixedDirectionFlag);
}

void
WKCWebViewPrivate::removeOverlay(WKCOverlayIf* overlay)
{
    if (m_overlayList && m_overlayList->remove(overlay) && m_overlayList->empty())
        m_overlayList.releaseNonNull();
}

void
WKCWebViewPrivate::updateOverlay(const WebCore::IntRect& rect, bool immediate)
{
    if (m_overlayList)
        m_overlayList->update(rect, immediate);
}

void
WKCWebViewPrivate::paintOverlay(WebCore::GraphicsContext& ctx)
{
    WebCore::Frame* frame = (WebCore::Frame *)&core()->mainFrame();
    if (!frame || !frame->contentRenderer() || !frame->view()) {
        return;
    }
    if (m_rootGraphicsLayer) {
        // AC layer exists. no need to paint here.
        return;
    }
    if (m_overlayList) {
        m_overlayList->paintOffscreen(ctx);
    }
}

#if ENABLE(REMOTE_INSPECTOR)
void
WKCWebKitSetWebInspectorResourcePath(const char* path)
{
    WebInspectorServer::setResourcePath(path);
}

bool
WKCWebKitStartWebInspector(const char* addr, int port, bool(*modalcycle)(void*), void* opaque)
{
    if (!addr || !modalcycle)
        return false;
    WebInspectorServer::createSharedInstance();

    bool success = WebInspectorServer::sharedInstance()->listen(addr, port);
    if (success) {
        WebCore::EventLoop_setCycleProc(modalcycle, opaque);
        NX_DP(("Inspector server started successfully. Listening at: http://%s:%d/", bindAddress, port));
    } else {
        NX_DP(("Couldn't start the inspector server."));
        WebInspectorServer::deleteSharedInstance();
    }
    return success;
}

void
WKCWebKitStopWebInspector()
{
    if (WebInspectorServer::sharedInstance()) {
        WebInspectorServer::deleteSharedInstance();
        WebCore::EventLoop_setCycleProc(0, 0);
    }
}

void
WKCWebViewPrivate::enableWebInspector(bool enable)
{
    if (!WebInspectorServer::sharedInstance())
        return;
    if (!m_inspectorServerClient)
        return;

    if (enable)
        m_inspectorServerClient->enableRemoteInspection();
    else
        m_inspectorServerClient->disableRemoteInspection();
    m_inspectorIsEnabled = enable;
    m_settings->setDeveloperExtrasEnabled(enable);
}

bool
WKCWebViewPrivate::isWebInspectorEnabled()
{
    return m_inspectorIsEnabled;
}
#else
void
WKCWebKitSetWebInspectorResourcePath(const char*)
{
}

bool
WKCWebKitStartWebInspector(const char*, int, bool(*)(void*), void*)
{
    return true;
}

void
WKCWebKitStopWebInspector()
{
}

void
WKCWebViewPrivate::enableWebInspector(bool)
{
}

bool
WKCWebViewPrivate::isWebInspectorEnabled()
{
    return false;
}
#endif

void
WKCWebView::enableWebInspector(bool enable)
{
    m_private->enableWebInspector(enable);
}

bool
WKCWebView::isWebInspectorEnabled()
{
    return m_private->isWebInspectorEnabled();
}

void
WKCWebView::setScrollPositionForOffscreen(const WKCPoint& scrollPosition)
{
    WebCore::IntPoint p(scrollPosition.fX, scrollPosition.fY);
    m_private->setScrollPositionForOffscreen(scrollPosition);
}

void
WKCWebView::scrollNodeByRecursively(WKC::Node* node, int dx, int dy)
{
    if (!node) {
        return;
    } 
    
    WebCore::Node* coreNode = node->priv().webcore();
    if (!coreNode->renderer() || !coreNode->renderer()->enclosingLayer()) {
        return;
    }
    coreNode->renderer()->enclosingLayer()->scrollByRecursively(WebCore::IntSize(dx, dy));
}

void
WKCWebView::scrollNodeBy(WKC::Node* node, int dx, int dy)
{
    if (!node) {
        return;
    } 
    
    WebCore::Node* coreNode = node->priv().webcore();
    WebCore::RenderObject* renderer = coreNode->renderer();
    WebCore::RenderLayer* layer = renderer ? renderer->enclosingLayer() : 0;

    if (!layer) {
        return;
    }

    bool isLineClampNoneByParent = true;
    if (renderer->parent()) {
        isLineClampNoneByParent = renderer->parent()->style().lineClamp().isNone();
    }
    if (renderer->hasOverflowClip() && isLineClampNoneByParent) {
        WebCore::ScrollOffset offset = layer->scrollOffset();
        int offsetX = offset.x() + dx;
        int offsetY = offset.y() + dy;
        layer->scrollToOffset(WebCore::ScrollOffset(offsetX, offsetY));

        WebCore::Frame* frame = &renderer->frame();
        if (frame) {
            WebCore::EventHandler* eventHandler = &frame->eventHandler();
            eventHandler->cancelFakeMouseMoveEvent(); // to avoid hover event when scrolling
            eventHandler->updateAutoscrollRenderer();
        }

    } else {
        renderer->view().frameView().scrollBy(WebCore::IntSize(dx, dy));
    }
}

// The logic of WKCWebView::isScrollableNode() is from SpatialNavigation.cpp's isScrollableNode().
bool
WKCWebView::isScrollableNode(const WKC::Node* node)
{
    if (!node) {
        return false;
    }
    WebCore::Node* coreNode = node->priv().webcore();

    if (WebCore::RenderObject* renderer = coreNode->renderer()) {
        if (!renderer->isBox() || !WTF::downcast<WebCore::RenderBox>(renderer)->canBeScrolledAndHasScrollableArea())
            return false;
        if (renderer->isTextArea()) {
            // Check shadow tree for the text control.
            WebCore::HTMLTextFormControlElement& form = WTF::downcast<WebCore::RenderTextControl>(renderer)->textFormControlElement();
            RefPtr<WebCore::TextControlInnerTextElement> innerTextElement = form.innerTextElement();
            return innerTextElement ? innerTextElement->hasChildNodes() : false;
        } else {
            return coreNode->hasChildNodes();
        }
    }
    return false;
}

bool
WKCWebView::canScrollNodeInDirection(const WKC::Node* node, WKC::FocusDirection direction)
{
    if (!node) {
        return false;
    }

    WebCore::Node *coreNode = node->priv().webcore();

    return WebCore::canScrollInDirection(coreNode, toWebCoreFocusDirection(direction));
}

WKCRect
WKCWebView::getScrollableContentRect(const WKC::Node* scrollableNode)
{
    if (!scrollableNode)
        return WKCRect();

    WebCore::Node* node = scrollableNode->priv().webcore();
    WebCore::RenderObject* renderer = node->renderer();

    if (WTF::is<WebCore::HTMLFrameOwnerElement>(node)) {
        WebCore::HTMLFrameOwnerElement* frameOwnerElement = downcast<WebCore::HTMLFrameOwnerElement>(node);
        if (frameOwnerElement->contentFrame()) {
            node = frameOwnerElement->contentFrame()->document();
            renderer = node->renderer();
        }
    }

    if (!renderer || !renderer->isBox() || renderer->style().visibility() != WebCore::Visibility::Visible)
        return WKCRect();

    WebCore::RenderBox* box = WTF::downcast<WebCore::RenderBox>(renderer);

    if (!box->canBeScrolledAndHasScrollableArea())
        return WKCRect();

    // frame (includes content area, but not includes scroll bar and padding and border)
    if (WTF::is<WebCore::Document>(node)) {
        WebCore::IntRect rect = box->absoluteContentBox();
        if (WebCore::HTMLFrameOwnerElement* owner = downcast<WebCore::Document>(node)->ownerElement()) {
            box = WTF::downcast<WebCore::RenderBox>(owner->renderer());
            rect.setLocation(box->absoluteContentBox().location());
        }
        return rect;
    }

    // overflow (includes content area and padding, but not scroll bar and border)
    if (box->scrollsOverflow()) {
        WebCore::IntRect rect = WebCore::snappedIntRect(box->clientBoxRect());
        rect.moveBy(renderer->absoluteBoundingBoxRect(true).location());
        return rect;
    }

    return WKCRect();
}

WKCRect
WKCWebView::transformedRect(const WKC::Node* node, const WKCRect& rect)
{
    if (!node) {
        return rect;
    }
    WebCore::Node* coreNode = node->priv().webcore();
    WebCore::RenderObject* renderer = coreNode->renderer();
    if (!renderer) {
        return rect;
    }

    WebCore::FloatPoint absPos = renderer->localToAbsolute();
    WebCore::IntRect r(rect.fX - absPos.x(), rect.fY - absPos.y(), rect.fWidth, rect.fHeight);
    WebCore::FloatQuad quad = renderer->localToAbsoluteQuad(WebCore::FloatQuad(r));
    WebCore::IntRect br = quad.enclosingBoundingBox();

    return br;
}

bool
WKCWebView::containsPoint(const WKC::Node* node, const WKCPoint& point)
{
    if (!node) {
        return false;
    }
    WebCore::Node* coreNode = node->priv().webcore();
    WebCore::RenderObject* renderer = coreNode->renderer();
    if (!renderer) {
        return false;
    }
    WebCore::FrameView* view = coreNode->document().view();
    if (!view) {
        return false;
    }

    WebCore::IntPoint pos = view->windowToContents(WebCore::IntPoint(point.fX, point.fY));
    WTF::Vector<WebCore::FloatQuad> quads;

    renderer->absoluteQuads(quads);

    for (size_t i = 0; i < quads.size(); ++i) {
        if (quads[i].containsPoint(pos)) {
            return true;
        }
    }
    return false;
}

bool
WKCWebView::editable()
{
    return m_private->editable();
}

void
WKCWebView::setEditable(bool enable)
{
    m_private->setEditable(enable);
}

void
WKCWebView::cancelFullScreen()
{
    WebCore::Document* childmost = 0;
    for (WebCore::Frame* frame = &m_private->core()->mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (frame->document()->webkitFullscreenElement())
            childmost = frame->document();
    }
    if (childmost)
        childmost->webkitCancelFullScreen();
}

void
WKCWebView::setSpatialNavigationEnabled(bool enable)
{
    m_private->setSpatialNavigationEnabled(enable);
}

void
WKCWebView::initializeGamepads()
{
#if ENABLE(GAMEPAD)
    WebCore::GamepadProvider::setSharedProvider(WebCore::GamepadProviderWKC::singleton());
#endif
}

int
WKCWebView::connectGamepad(const WKC::String& id, int naxes, int nbuttons)
{
#if ENABLE(GAMEPAD)
    return WebCore::GamepadProviderWKC::singleton().addGamepad(id, naxes, nbuttons);
#endif
}

void
WKCWebView::disconnectGamepad(int index)
{
#if ENABLE(GAMEPAD)
    WebCore::GamepadProviderWKC::singleton().removeGamepad(index);
#endif
}

bool
WKCWebView::updateGamepadValue(int index, long long timestamp, int naxes, const double* axes, int nbuttons, const double* buttons)
{
#if ENABLE(GAMEPAD)
    WebCore::GamepadProviderWKC::singleton().updateGamepadValue(index, WTF::MonotonicTime::fromRawSeconds(timestamp), naxes, axes, nbuttons, buttons);
    return true;
#else
    return false;
#endif
}

// session storage and local storage

static unsigned
storageMemoryConsumptionBytesForFrame(const WebCore::Storage* storage)
{
    unsigned bytes = 0;
    unsigned len = storage->area().length();
    for (int i=0; i<len; i++) {
        WTF::String v = storage->area().key(i);
        bytes += v.is8Bit() ? v.ascii().length() : v.length() * sizeof(UChar);

        v = storage->area().item(v);
        bytes += v.is8Bit() ? v.ascii().length() : v.length() * sizeof(UChar);
    }
    return bytes;
}

unsigned
WKCWebView::sessionStorageMemoryConsumptionBytes()
{
    WebCore::Page* page = m_private->core();
    if (!page)
        return 0;

    unsigned bytes = 0;
    const WebCore::Storage* storage = 0;
    if (page->mainFrame().document() && page->mainFrame().document()->domWindow() && page->mainFrame().document()->domWindow()->optionalSessionStorage()) {
        storage = page->mainFrame().document()->domWindow()->optionalSessionStorage();
        bytes += storageMemoryConsumptionBytesForFrame(storage);
    }
    for (WebCore::Frame* frame = page->mainFrame().tree().firstChild(); frame; frame = frame->tree().nextSibling()) {
        if (!(frame->document() && frame->document()->domWindow() && frame->document()->domWindow()->optionalSessionStorage()))
            continue;
        storage = frame->document()->domWindow()->optionalSessionStorage();
        bytes += storageMemoryConsumptionBytesForFrame(storage);
    }

    return bytes;
}

unsigned
WKCWebView::localStorageMemoryConsumptionBytes(const char* pagegroupname)
{
    WebCore::PageGroup* pageGroup = WebCore::PageGroup::pageGroup(pagegroupname);
    if (!pageGroup)
        return 0;

    unsigned bytes = 0;
    const WebCore::Storage* storage = 0;
    const WTF::HashSet<WebCore::Page*>& pages = pageGroup->pages();
    for (WTF::HashSet<WebCore::Page*>::const_iterator it = pages.begin(); it!=pages.end(); ++it) {
        const WebCore::Page* page = *it;
        if (!page)
            continue;

        if (page->mainFrame().document() && page->mainFrame().document()->domWindow() && page->mainFrame().document()->domWindow()->optionalLocalStorage()) {
            storage = page->mainFrame().document()->domWindow()->optionalLocalStorage();
            bytes += storageMemoryConsumptionBytesForFrame(storage);
        }
        for (WebCore::Frame* frame = page->mainFrame().tree().firstChild(); frame; frame = frame->tree().nextSibling()) {
            if (!(frame->document() && frame->document()->domWindow() && frame->document()->domWindow()->optionalLocalStorage()))
                continue;
            storage = frame->document()->domWindow()->optionalLocalStorage();
            bytes += storageMemoryConsumptionBytesForFrame(storage);
        }
    }

    return bytes;
}

void
WKCWebView::clearSessionStorage()
{
    WebCore::Page* page = m_private->core();
    if(!page)
        return;

    if (page->mainFrame().document() && page->mainFrame().document()->domWindow() && page->mainFrame().document()->domWindow()->optionalSessionStorage())
        page->mainFrame().document()->domWindow()->optionalSessionStorage()->area().clear(&page->mainFrame());
    for (WebCore::Frame* frame = page->mainFrame().tree().firstChild(); frame; frame = frame->tree().nextSibling()) {
        if (!(frame->document() && frame->document()->domWindow() && frame->document()->domWindow()->optionalSessionStorage()))
            continue;
        frame->document()->domWindow()->optionalSessionStorage()->area().clear(frame);
    }
}

void
WKCWebView::clearLocalStorage(const char* pagegroupname)
{
    WebCore::PageGroup* pageGroup = WebCore::PageGroup::pageGroup(pagegroupname);
    if (!pageGroup)
        return;

    unsigned bytes = 0;
    const WTF::HashSet<WebCore::Page*>& pages = pageGroup->pages();
    for (WTF::HashSet<WebCore::Page*>::const_iterator it = pages.begin(); it!=pages.end(); ++it) {
        const WebCore::Page* page = *it;
        if (!page)
            continue;

        if (page->mainFrame().document() && page->mainFrame().document()->domWindow() && page->mainFrame().document()->domWindow()->optionalLocalStorage())
            page->mainFrame().document()->domWindow()->optionalLocalStorage()->area().clear((WebCore::Frame *)&page->mainFrame());
        for (WebCore::Frame* frame = page->mainFrame().tree().firstChild(); frame; frame = frame->tree().nextSibling()) {
            if (!(frame->document() && frame->document()->domWindow() && frame->document()->domWindow()->optionalLocalStorage()))
                continue;
            frame->document()->domWindow()->optionalLocalStorage()->area().clear(frame);
        }
    }
}

#if ENABLE(CONTENT_EXTENSIONS)
class ContentExtensionClientWKC : public WebCore::ContentExtensions::ContentExtensionCompilationClient, public WebCore::ContentExtensions::CompiledContentExtension
{
public:
    ContentExtensionClientWKC()
    {
    }
    virtual ~ContentExtensionClientWKC()
    {
    }

    // ContentExtensionCompilationClient
    virtual void writeActions(WTF::Vector<WebCore::ContentExtensions::SerializedActionByte>&& code) override
    {
        m_actions.appendVector(code);
    }
    void writeFiltersWithoutDomainsBytecode(WTF::Vector<WebCore::ContentExtensions::DFABytecode>&& code) override
    {
        m_filtersWithoutDomainsBytecode.appendVector(code);
    }
    virtual void writeFiltersWithDomainsBytecode(WTF::Vector<WebCore::ContentExtensions::DFABytecode>&& code) override
    {
        m_filtersWithDomainsBytecode.appendVector(code);
    }
    virtual void writeDomainFiltersBytecode(WTF::Vector<WebCore::ContentExtensions::DFABytecode>&& code) override
    {
        m_domainFiltersBytecode.appendVector(code);
    }
    virtual void finalize() override
    {
    }

    // CompiledContentExtension
    virtual const WebCore::ContentExtensions::DFABytecode* filtersWithoutDomainsBytecode() const override
    {
        return m_filtersWithoutDomainsBytecode.begin();
    }
    virtual unsigned filtersWithoutDomainsBytecodeLength() const override
    {
        return m_filtersWithoutDomainsBytecode.size();
    }
    virtual const WebCore::ContentExtensions::DFABytecode* filtersWithDomainsBytecode() const override
    {
        return m_filtersWithDomainsBytecode.begin();
    }
    virtual unsigned filtersWithDomainsBytecodeLength() const override
    {
        return m_filtersWithDomainsBytecode.size();
    }
    virtual const WebCore::ContentExtensions::DFABytecode* domainFiltersBytecode() const override
    {
        return m_domainFiltersBytecode.begin();
    }
    virtual unsigned domainFiltersBytecodeLength() const override
    {
        return m_domainFiltersBytecode.size();
    }
    virtual const WebCore::ContentExtensions::SerializedActionByte* actions() const override
    {
        return m_actions.begin();
    }
    virtual unsigned actionsLength() const
    {
        return m_actions.size();
    }

private:
    WTF::Vector<WebCore::ContentExtensions::DFABytecode> m_filtersWithoutDomainsBytecode;
    WTF::Vector<WebCore::ContentExtensions::DFABytecode> m_filtersWithDomainsBytecode;
    WTF::Vector<WebCore::ContentExtensions::DFABytecode> m_domainFiltersBytecode;
    WTF::Vector<WebCore::ContentExtensions::SerializedActionByte> m_actions;
};
#endif

bool
WKCWebViewPrivate::addUserContentExtension(const char* name, const char* extension)
{
#if ENABLE(CONTENT_EXTENSIONS)
    WebCore::Page* page = core();
    if (!page)
        return false;
    if (!page->userContentController())
        return false;
    RefPtr<ContentExtensionClientWKC> contentExtensionClient = adoptRef(new ContentExtensionClientWKC());

    WTF::String json(extension);
    auto err = WebCore::ContentExtensions::compileRuleList(*contentExtensionClient, WTF::move(json));
    if (err)
        return false;

    page->userContentController()->addUserContentExtension(name, contentExtensionClient);
    return true;
#else
    UNUSED_PARAM(name);
    UNUSED_PARAM(extension);
    return true;
#endif
}

bool
WKCWebViewPrivate::compileUserContentExtension(const char* extension, unsigned char** out_filtersWithoutDomainsBytecode, unsigned int* out_filtersWithoutDomainsBytecodeLen, unsigned char** out_filtersWithDomainsBytecode, unsigned int* out_filtersWithDomainsBytecodeLen, unsigned char** out_domainFiltersBytecode, unsigned int* out_domainFiltersBytecodeLen, unsigned char** out_actions, unsigned int* out_actionsLen)
{
#if ENABLE(CONTENT_EXTENSIONS)
    WebCore::Page* page = core();
    if (!page)
        return false;
    if (!page->userContentController())
        return false;
    RefPtr<ContentExtensionClientWKC> contentExtensionClient = adoptRef(new ContentExtensionClientWKC());

    WTF::String json(extension);
    auto err = WebCore::ContentExtensions::compileRuleList(*contentExtensionClient, WTF::move(json));
    if (err)
        return false;

    unsigned int len = 0;
    unsigned char* p = nullptr;

    len = contentExtensionClient->filtersWithoutDomainsBytecodeLength();
    if (len && out_filtersWithoutDomainsBytecode) {
        p = (unsigned char *)wkc_malloc_crashonfailure(len);
        ::memcpy(p, contentExtensionClient->filtersWithoutDomainsBytecode(), len);
    } else
        p = nullptr;
    if (out_filtersWithoutDomainsBytecode)
        *out_filtersWithoutDomainsBytecode = p;
    if (out_filtersWithoutDomainsBytecodeLen)
        *out_filtersWithoutDomainsBytecodeLen = len;

    len = contentExtensionClient->filtersWithDomainsBytecodeLength();
    if (len && out_filtersWithDomainsBytecode) {
        p = (unsigned char *)wkc_malloc_crashonfailure(len);
        ::memcpy(p, contentExtensionClient->filtersWithDomainsBytecode(), len);
    } else
        p = nullptr;
    if (out_filtersWithDomainsBytecode)
        *out_filtersWithDomainsBytecode = p;
    if (out_filtersWithDomainsBytecodeLen)
        *out_filtersWithDomainsBytecodeLen = len;

    len = contentExtensionClient->domainFiltersBytecodeLength();
    if (len && out_domainFiltersBytecode) {
        p = (unsigned char *)wkc_malloc_crashonfailure(len);
        ::memcpy(p, contentExtensionClient->domainFiltersBytecode(), len);
    } else
        p = nullptr;
    if (out_domainFiltersBytecode)
        *out_domainFiltersBytecode = p;
    if (out_domainFiltersBytecodeLen)
        *out_domainFiltersBytecodeLen = len;

    len = contentExtensionClient->actionsLength();
    if (len && out_actions) {
        p = (unsigned char *)wkc_malloc_crashonfailure(len);
        ::memcpy(p, contentExtensionClient->actions(), len);
    } else
        p = nullptr;
    if (out_actions)
        *out_actions = p;
    if (out_actionsLen)
        *out_actionsLen = len;

    contentExtensionClient.leakRef();
    return true;
#else
    UNUSED_PARAM(extension);
    if (out_filtersWithoutDomainsBytecode)
        *out_filtersWithoutDomainsBytecode = nullptr;
    if (out_filtersWithoutDomainsBytecodeLen)
        *out_filtersWithoutDomainsBytecodeLen = 0;
    if (out_filtersWithDomainsBytecode)
        *out_filtersWithDomainsBytecode = nullptr;
    if (out_filtersWithDomainsBytecodeLen)
        *out_filtersWithDomainsBytecodeLen = 0;
    if (out_domainFiltersBytecode)
        *out_domainFiltersBytecode = nullptr;
    if (out_domainFiltersBytecodeLen)
        *out_domainFiltersBytecodeLen = 0;
    if (out_actions)
        *out_actions = nullptr;
    if (out_actionsLen)
        *out_actionsLen = 0;
    return true;
#endif
}

bool
WKCWebViewPrivate::addCompiledUserContentExtension(const char* name, const unsigned char* filtersWithoutDomainsBytecode, unsigned int filtersWithoutDomainsBytecodeLen, const unsigned char* filtersWithDomainsBytecode, unsigned int filtersWithDomainsBytecodeLen, const unsigned char* domainFiltersBytecode, unsigned int domainFiltersBytecodeLen, const unsigned char* actions, unsigned int actionsLen)
{
#if ENABLE(CONTENT_EXTENSIONS)
    WebCore::Page* page = core();
    if (!page)
        return false;
    if (!page->userContentController())
        return false;
    RefPtr<ContentExtensionClientWKC> contentExtensionClient = adoptRef(new ContentExtensionClientWKC());

    WTF::Vector<WebCore::ContentExtensions::DFABytecode> filtersWithoutDomains(filtersWithoutDomainsBytecodeLen);
    filtersWithoutDomains.append(filtersWithoutDomainsBytecode, filtersWithoutDomainsBytecodeLen);
    contentExtensionClient->writeFiltersWithoutDomainsBytecode(WTF::move(filtersWithoutDomains));

    WTF::Vector<WebCore::ContentExtensions::DFABytecode> filtersWithDomains(filtersWithDomainsBytecodeLen);
    filtersWithDomains.append(filtersWithDomainsBytecode, filtersWithDomainsBytecodeLen);
    contentExtensionClient->writeFiltersWithDomainsBytecode(WTF::move(filtersWithDomains));

    WTF::Vector<WebCore::ContentExtensions::DFABytecode> domainFilter(domainFiltersBytecodeLen);
    domainFilter.append(domainFiltersBytecode, domainFiltersBytecodeLen);
    contentExtensionClient->writeDomainFiltersBytecode(WTF::move(domainFilter));

    WTF::Vector<WebCore::ContentExtensions::SerializedActionByte> vaction(actionsLen);
    vaction.append(actions, actionsLen);
    contentExtensionClient->writeActions(WTF::move(vaction));

    page->userContentController()->addUserContentExtension(name, contentExtensionClient);

    return true;

#else
    UNUSED_PARAM(name);
    UNUSED_PARAM(filtersWithoutDomainsBytecode);
    UNUSED_PARAM(filtersWithoutDomainsBytecodeLen);
    UNUSED_PARAM(filtersWithDomainsBytecode);
    UNUSED_PARAM(filtersWithDomainsBytecodeLen);
    UNUSED_PARAM(domainFiltersBytecode);
    UNUSED_PARAM(domainFiltersBytecodeLen);
    UNUSED_PARAM(actions);
    UNUSED_PARAM(actionsLen);
    return true;
#endif
}

bool
WKCWebView::addUserContentExtension(const char* name, const char* extension)
{
    return m_private->addUserContentExtension(name, extension);
}

bool
WKCWebView::compileUserContentExtension(const char* extension, unsigned char** out_filtersWithoutDomainsBytecode, unsigned int* out_filtersWithoutDomainsBytecodeLen, unsigned char** out_filtersWithDomainsBytecode, unsigned int* out_filtersWithDomainsBytecodeLen, unsigned char** out_domainFiltersBytecode, unsigned int* out_domainFiltersBytecodeLen, unsigned char** out_actions, unsigned int* out_actionsLen)
{
    return m_private->compileUserContentExtension(extension, out_filtersWithoutDomainsBytecode, out_filtersWithoutDomainsBytecodeLen, out_filtersWithDomainsBytecode, out_filtersWithDomainsBytecodeLen, out_domainFiltersBytecode, out_domainFiltersBytecodeLen, out_actions, out_actionsLen);
}

bool
WKCWebView::addCompiledUserContentExtension(const char* name, const unsigned char* filtersWithoutDomainsBytecode, unsigned int filtersWithoutDomainsBytecodeLen, const unsigned char* filtersWithDomainsBytecode, unsigned int filtersWithDomainsBytecodeLen, const unsigned char* domainFiltersBytecode, unsigned int domainFiltersBytecodeLen, const unsigned char* actions, unsigned int actionsLen)
{
    return m_private->addCompiledUserContentExtension(name, filtersWithoutDomainsBytecode, filtersWithoutDomainsBytecodeLen, filtersWithDomainsBytecode, filtersWithDomainsBytecodeLen, domainFiltersBytecode, domainFiltersBytecodeLen, actions, actionsLen);
}

// Device Mode
void
WKCWebKitSetDeviceMode(int in_mode)
{
#if ENABLE(WKC_DEVICE_MODE_CSS_MEDIA)
    int mode = WebCore::DeviceModeEnum::console;
    if (in_mode == WKC::EDeviceModeHandheld) {
        mode = WebCore::DeviceModeEnum::handheld;
    }

    WebCore::setDeviceMode(mode);
#endif // ENABLE(WKC_DEVICE_MODE_CSS_MEDIA)
}

void
WKCWebKitSetPerformanceMode(int in_mode)
{
#if ENABLE(WKC_PERFORMANCE_MODE_CSS_MEDIA)
    int mode = WebCore::PerformanceModeEnum::performance0;

    switch (in_mode) {
    case WKC::EPerformanceMode0:
        mode = WebCore::PerformanceModeEnum::performance0;
        break;
    case WKC::EPerformanceMode1:
        mode = WebCore::PerformanceModeEnum::performance1;
        break;
    default:
        break;
    }

    WebCore::setPerformanceMode(mode);
#endif // ENABLE(WKC_PERFORMANCE_MODE_CSS_MEDIA)
}

void
WKCWebViewPrivate::recalcStyleSheet()
{
    WebCore::Frame* mainFrame = &core()->mainFrame();
    for (WebCore::Frame* frame = mainFrame; frame; frame = frame->tree().traverseNext()) {
        frame->document()->styleScope().didChangeStyleSheetEnvironment();
    }
}

void
WKCWebView::recalcStyleSheet()
{
    m_private->recalcStyleSheet();
}

void
WKCWebViewPrivate::enableAutoPlay(bool enable)
{
    m_autoPlayIsEnabled = enable;
    m_settings->setVideoPlaybackRequiresUserGesture(!enable);
}

bool
WKCWebViewPrivate::isAutoPlayEnabled()
{
    return m_autoPlayIsEnabled;
}

void
WKCWebView::enableAutoPlay(bool enable)
{
    m_private->enableAutoPlay(enable);
}

bool
WKCWebView::isAutoPlayEnabled()
{
    return m_private->isAutoPlayEnabled();
}

void
WKCWebView::dumpExternalRepresentation(Frame* frame)
{
    if (!frame) {
        return;
    }
    WTF::String output = WebCore::externalRepresentation(frame->priv().webcore(), WebCore::RenderAsTextShowAllLayers
                                                                                | WebCore::RenderAsTextShowLayerNesting
                                                                                | WebCore::RenderAsTextShowCompositedLayers
                                                                                | WebCore::RenderAsTextShowAddresses
                                                                                | WebCore::RenderAsTextShowIDAndClass
                                                                                | WebCore::RenderAsTextDontUpdateLayout
                                                                                | WebCore::RenderAsTextShowLayoutState
                                                                                | WebCore::RenderAsTextShowOverflow);
    fprintf(stderr, "%s\n", output.utf8().data());
}

void
WKCWebView::getCurrentFocusPoint(int& x, int& y)
{
    WebCore::LayoutPoint p = m_private->core()->focusController().getLastEntryPoint();
    x = p.x();
    y = p.y();
}

void
WKCWebView::clearCurrentFocusPoint()
{
    m_private->core()->focusController().clearLastEntryInfo();
}

// cancel range input dragging mode.

void
WKCWebKitCancelRangeInputDragging()
{
    WebCore::SliderThumbElement::cancelDragging();
}

void
WKCWebKitSetCanCacheToDiskCallback(CanCacheToDiskProc in_proc)
{
#if ENABLE(WKC_HTTPCACHE)
    WebCore::ResourceHandleManager::sharedInstance()->setCanCacheToDiskCallback(in_proc);
#endif
}

void
WKCWebKitSetConnectionFilteringCallback(ConnectionFilteringProc in_proc)
{
    WebCore::ResourceHandleManager::sharedInstance()->setConnectionFilteringCallback(in_proc);
}

void
WKCWebKitSetWillAcceptCookieCallback(WillAcceptCookieProc in_proc)
{
    WebCore::ResourceHandleManager::sharedInstance()->setWillAcceptCookieCallback(in_proc);
}

// dump HTTPCache list (for Debug)

void
WKCWebKitDumpHTTPCacheList()
{
    if (WebCore::ResourceHandleManager::isExistSharedInstance()) {
        WebCore::ResourceHandleManager::sharedInstance()->dumpHTTPCacheResourceList();
    }
}

// display link
void
WKCWebKitDisplayWasRefreshed()
{
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    WebCore::DisplayRefreshMonitorManager::sharedManager().displayWasUpdated();
#endif
}

// idn

namespace IDN {

int
toUnicode(const char* host, unsigned short* idn, int maxidn)
{
    return wkcI18NIDNtoUnicodePeer((const unsigned char *)host, -1, idn, maxidn);
}

int
fromUnicode(const unsigned short* idn, char* host, int maxhost)
{
    return wkcI18NIDNfromUnicodePeer(idn, -1, (unsigned char *)host, maxhost);
}

} // namespace

// network utility

namespace NetUtil {

/*
 * correctIPAddress()
 *
 * description:
 *   Check format of IP address.
 *   Whether the IP Address is not considered valid.
 *
 * argument:
 *   in_ipaddress: String of IP Address
 *
 * return value:
 *   0: in_ipaddress is not IP Address.
 *   4: in_ipaddress is IPv4 Address.
 *   6: in_ipaddress is IPv6 Address.
 */
int
correctIPAddress(const char *in_ipaddress)
{
    return wkcNetCheckCorrectIPAddressPeer(in_ipaddress);
}

} // namespace

namespace Base64 {
int
base64Encode(const char* in, char* buf, int buflen)
{
    WTF::Vector<char> encoded;

    WTF::base64Encode(in, ::strlen(in), encoded);
    if (buf && buflen > encoded.size())
        strncpy(buf, encoded.data(), encoded.size());

    return encoded.size();
}

} // namespace

} // namespace

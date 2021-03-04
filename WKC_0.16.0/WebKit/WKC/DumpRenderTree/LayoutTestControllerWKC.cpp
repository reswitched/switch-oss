/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Michael Alonzo <jmalonzo@gmail.com>
 * Copyright (C) 2009 Collabora Ltd.
 * Copyright (c) 2009-2015 ACCESS CO., LTD. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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

#if COMPILER(MSVC) && _MSC_VER >= 1700
extern bool __uncaught_exception();
#endif

#include "LayoutTestController.h"

#include "AnimationController.h"
#include "BackForwardList.h"
#include "CString.h"
#include "DumpRenderTree.h"
#include "DumpRenderTreeWKC.h"
#include "Frame.h"
#include "Element.h"
#include "HistoryItem.h"
#include "Page.h"
#include "RenderTreeAsText.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "WorkQueue.h"
#include "WorkQueueItem.h"
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRef.h>

#include "WKCWebFrame.h"
#include "WKCWebView.h"

#include "helpers/WKCDocument.h"
#include "helpers/privates/WKCDocumentPrivate.h"
#include "helpers/WKCFrame.h"

#include "SVGDocumentExtensions.h"
#include "SVGSMILElement.h"

static char* JSStringCopyUTF8CString(JSStringRef jsString)
{
    size_t dataSize = JSStringGetMaximumUTF8CStringSize(jsString);
    char* utf8 = (char*)malloc(dataSize);
    JSStringGetUTF8CString(jsString, utf8, dataSize);

    return utf8;
}

LayoutTestController::~LayoutTestController()
{
    // FIXME: implement
}

void LayoutTestController::addDisallowedURL(JSStringRef url)
{
    // FIXME: implement
}

void LayoutTestController::clearBackForwardList()
{
#if 0
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    WebCore::Page* page = (WebCore::Page *)view->core();
    WebCore::BackForwardListImpl* list = 0;
    WebCore::HistoryItem* cur = 0;
    int cap = 0;

    list = static_cast<WebCore::BackForwardListImpl *>(page->backForwardList());
    cap = list->capacity();
    cur = list->currentItem();
    list->setCapacity(0);
    list->setCapacity(cap);
    list->addItem(cur);
    list->goToItem(cur);
#endif
}

JSStringRef LayoutTestController::copyDecodedHostName(JSStringRef name)
{
    // FIXME: implement
    return 0;
}

JSStringRef LayoutTestController::copyEncodedHostName(JSStringRef name)
{
    // FIXME: implement
    return 0;
}

void LayoutTestController::dispatchPendingLoadRequests()
{
    // FIXME: Implement for testing fix for 6727495
}

void LayoutTestController::display()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    return;
#else
    displayWebView();
#endif
}

JSRetainPtr<JSStringRef> LayoutTestController::counterValueForElementById(JSStringRef id)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    char* idGChar = JSStringCopyUTF8CString(id);

    WebCore::Element* coreElement = ((WebCore::Frame *)mainFrame->core())->document()->getElementById(WTF::AtomicString(idGChar));
    free(idGChar);
    if (!coreElement) {
        return 0;
    }
    WTF::String counterValue = WebCore::counterValueForElement(coreElement);
    const char* counterValueGChar = counterValue.utf8().data();

    if (!counterValueGChar)
        return 0;
    JSRetainPtr<JSStringRef> v(Adopt, JSStringCreateWithUTF8CString(counterValueGChar));
    return v;
}

void LayoutTestController::keepWebHistory()
{
    // FIXME: implement
}

size_t LayoutTestController::webHistoryItemCount()
{
    // FIXME: implement
    return 0;
}

unsigned LayoutTestController::workerThreadCount() const
{
#if ENABLE(WORKER)
    return WebCore::WorkerThread::workerThreadCount();
#else
    return 0;
#endif
}

void LayoutTestController::notifyDone()
{
    if (m_waitToDump && !DumpRenderTreeWKC::topLoadingFrame())
        DumpRenderTreeWKC::dump();
    m_waitToDump = false;
    DumpRenderTreeWKC::setWaitForPolicy(false);
}

JSStringRef LayoutTestController::pathToLocalResource(JSContextRef context, JSStringRef url)
{
    // Function introduced in r28690. This may need special-casing on Windows.
    return JSStringRetain(url); // Do nothing on Unix.
}

void LayoutTestController::queueLoad(JSStringRef url, JSStringRef target)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();

    char* relativeURL = JSStringCopyUTF8CString(url);
    const char* baseURI = mainFrame->uri();

    WebCore::URL uri(WebCore::ParsedURLString, relativeURL);

    if (!uri.isValid()) {
        WebCore::URL base(WebCore::ParsedURLString, baseURI);
        uri = WebCore::URL(base, relativeURL);
    }
    free(relativeURL);

    const char* curi = uri.string().utf8().data();
    JSRetainPtr<JSStringRef> absoluteURL(Adopt, JSStringCreateWithUTF8CString(curi));

//    WorkQueue::shared()->queue(new LoadItem(absoluteURL.get(), target));
}

void LayoutTestController::setAcceptsEditing(bool acceptsEditing)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();

    view->setEditable(acceptsEditing);
}

void LayoutTestController::setAlwaysAcceptCookies(bool alwaysAcceptCookies)
{
    // FIXME: Implement this (and restore the default value before running each test in DumpRenderTree.cpp).
}

void LayoutTestController::setCustomPolicyDelegate(bool setDelegate, bool permissive)
{
    // FIXME: implement
}

void LayoutTestController::waitForPolicyDelegate()
{
    DumpRenderTreeWKC::setWaitForPolicy(true);
    setWaitToDump(true);
}

#if 0
void LayoutTestController::whiteListAccessFromOrigin(JSStringRef sourceOrigin, JSStringRef protocol, JSStringRef host, bool includeSubdomains)
{
    char* sourceOriginChar = JSStringCopyUTF8CString(sourceOrigin);
    char* protocolChar = JSStringCopyUTF8CString(protocol);
    char* hostChar = JSStringCopyUTF8CString(host);

    WebCore::SecurityOrigin::whiteListAccessFromOrigin(*WebCore::SecurityOrigin::createFromString(sourceOriginChar), protocolChar, hostChar, includeSubdomains);

    free(sourceOriginChar);
    free(protocolChar);
    free(hostChar);
}
#endif

void LayoutTestController::setMainFrameIsFirstResponder(bool flag)
{
    // FIXME: implement
}

void LayoutTestController::setTabKeyCyclesThroughElements(bool cycles)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    WebCore::Page* page = (WebCore::Page *)view->core();

    page->setTabKeyCyclesThroughElements(cycles);
}

void LayoutTestController::setUseDashboardCompatibilityMode(bool flag)
{
    // FIXME: implement
}

void LayoutTestController::setUserStyleSheetEnabled(bool flag)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    WebCore::Settings* settings = (WebCore::Settings *)view->settings();
    const char* userStyleSheet = DumpRenderTreeWKC::userStyleSheet();

    DumpRenderTreeWKC::setUserStyleSheetEnabled(flag);

    if (flag && userStyleSheet) {
        settings->setUserStyleSheetLocation(WebCore::URL(WebCore::URL(), userStyleSheet));
    } else {
        settings->setUserStyleSheetLocation(WebCore::URL());
    }
}

void LayoutTestController::setUserStyleSheetLocation(JSStringRef path)
{
    char* v = JSStringCopyUTF8CString(path);
    if (!v)
        return;
    DumpRenderTreeWKC::setUserStyleSheet(v);
    if (DumpRenderTreeWKC::userStyleSheetEnabled())
        setUserStyleSheetEnabled(true);
}

void LayoutTestController::setWindowIsKey(bool windowIsKey)
{
    // FIXME: implement
}

void LayoutTestController::setSmartInsertDeleteEnabled(bool flag)
{
    // FIXME: implement
}

void LayoutTestController::setWaitToDump(bool waitUntilDone)
{

    static const int timeoutSeconds = 15 * 1000;

    m_waitToDump = waitUntilDone;
    if (m_waitToDump) {
        DumpRenderTreeWKC::beginWatchdog(timeoutSeconds);
    }
}

int LayoutTestController::windowCount()
{
    return DumpRenderTreeWKC::webViewCount();
}

void LayoutTestController::setPrivateBrowsingEnabled(bool flag)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    WebCore::Page* page = (WebCore::Page *)view->core();
    page->setSessionID(flag ? WebCore::SessionID::legacyPrivateSessionID() : WebCore::SessionID::defaultSessionID());
}

void LayoutTestController::setXSSAuditorEnabled(bool flag)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    WebCore::Settings* settings = (WebCore::Settings *)view->settings();
    settings->setXSSAuditorEnabled(flag);
}

void LayoutTestController::setAllowUniversalAccessFromFileURLs(bool flag)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    WebCore::Settings* settings = (WebCore::Settings *)view->settings();
    settings->setAllowUniversalAccessFromFileURLs(flag);
}

void LayoutTestController::setAuthorAndUserStylesEnabled(bool flag)
{
    // FIXME: implement
}

void LayoutTestController::disableImageLoading()
{
    // FIXME: Implement for testing fix for https://bugs.webkit.org/show_bug.cgi?id=27896
    // Also need to make sure image loading is re-enabled for each new test.
}

void LayoutTestController::setMockGeolocationPosition(double latitude, double longitude, double accuracy)
{
    // FIXME: Implement for Geolocation layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=28264.
}

void LayoutTestController::setMockGeolocationError(int code, JSStringRef message)
{
    // FIXME: Implement for Geolocation layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=28264.
}

void LayoutTestController::setIconDatabaseEnabled(bool flag)
{
    // FIXME: implement
}

void LayoutTestController::setJavaScriptProfilingEnabled(bool flag)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    WebCore::Settings* settings = (WebCore::Settings *)view->settings();

    settings->setDeveloperExtrasEnabled(flag);

#if 0
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
    WebKitWebInspector* inspector = webkit_web_view_get_inspector(view);
    g_object_set(G_OBJECT(inspector), "javascript-profiling-enabled", flag, NULL);
#endif
}

void LayoutTestController::setSelectTrailingWhitespaceEnabled(bool flag)
{
    // FIXME: implement
}

void LayoutTestController::setPopupBlockingEnabled(bool flag)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    WebCore::Settings* settings = (WebCore::Settings *)view->settings();
    settings->setJavaScriptCanOpenWindowsAutomatically(flag);
}

bool LayoutTestController::elementDoesAutoCompleteForElementWithId(JSStringRef id)
{
    // FIXME: implement
    return false;
}

void LayoutTestController::execCommand(JSStringRef name, JSStringRef value)
{
    // FIXME: implement
}

void LayoutTestController::setCacheModel(int)
{
    // FIXME: implement
}

bool LayoutTestController::isCommandEnabled(JSStringRef /*name*/)
{
    // FIXME: implement
    return false;
}

void LayoutTestController::setPersistentUserStyleSheetLocation(JSStringRef jsURL)
{
    // FIXME: implement
}

void LayoutTestController::clearPersistentUserStyleSheet()
{
    // FIXME: implement
}

void LayoutTestController::clearAllDatabases()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
#else
    webkit_remove_all_web_databases();
#endif
}

void LayoutTestController::setDatabaseQuota(unsigned long long quota)
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
#else
    WebKitSecurityOrigin* origin = webkit_web_frame_get_security_origin(mainFrame);
    webkit_security_origin_set_web_database_quota(origin, quota);
#endif
}

void LayoutTestController::setAppCacheMaximumSize(unsigned long long size)
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
#else
    webkit_application_cache_set_maximum_size(size);
#endif
}

bool LayoutTestController::pauseAnimationAtTimeOnElementWithId(JSStringRef animationName, double time, JSStringRef elementId)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    char* name = JSStringCopyUTF8CString(animationName);
    char* element = JSStringCopyUTF8CString(elementId);
    bool returnValue = false;

    WebCore::Element* coreElement = ((WebCore::Frame *)(mainFrame->core()))->document()->getElementById(WTF::AtomicString(element));
    if (coreElement && coreElement->renderer()) {
        returnValue = ((WebCore::Frame *)(mainFrame->core()))->animation().pauseAnimationAtTime(coreElement->renderer(), WTF::AtomicString(name), time);
    }

    free(name);
    free(element);
    return returnValue;
}

bool LayoutTestController::pauseTransitionAtTimeOnElementWithId(JSStringRef propertyName, double time, JSStringRef elementId)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    char* name = JSStringCopyUTF8CString(propertyName);
    char* element = JSStringCopyUTF8CString(elementId);
    bool returnValue = false;

    WebCore::Element* coreElement = ((WebCore::Frame *)(mainFrame->core()))->document()->getElementById(WTF::AtomicString(element));
    if (coreElement && coreElement->renderer()) {
        returnValue = ((WebCore::Frame *)(mainFrame->core()))->animation().pauseTransitionAtTime(coreElement->renderer(), WTF::AtomicString(name), time);
    }
    free(name);
    free(element);
    return returnValue;
}

unsigned LayoutTestController::numberOfActiveAnimations() const
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();

    WebCore::AnimationController& controller = ((WebCore::Frame *)(mainFrame->core()))->animation();
    WebCore::Document* doc = ((WebCore::Frame *)mainFrame->core())->document();

    return controller.numberOfActiveAnimations(doc);
}

void LayoutTestController::overridePreference(JSStringRef key, JSStringRef value)
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
#else
    gchar* name = JSStringCopyUTF8CString(key);
    gchar* strValue = JSStringCopyUTF8CString(value);

    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    WebKitWebSettings* settings = webkit_web_view_get_settings(view);
    gchar* webSettingKey = copyWebSettingKey(name);

    if (webSettingKey) {
        GValue stringValue = { 0, { { 0 } } };
        g_value_init(&stringValue, G_TYPE_STRING);
        g_value_set_string(&stringValue, const_cast<gchar*>(strValue));

        WebKitWebSettingsClass* klass = WEBKIT_WEB_SETTINGS_GET_CLASS(settings);
        GParamSpec* pspec = g_object_class_find_property(G_OBJECT_CLASS(klass), webSettingKey);
        GValue propValue = { 0, { { 0 } } };
        g_value_init(&propValue, pspec->value_type);

        if (g_value_type_transformable(G_TYPE_STRING, pspec->value_type)) {
            g_value_transform(const_cast<GValue*>(&stringValue), &propValue);
            g_object_set_property(G_OBJECT(settings), webSettingKey, const_cast<GValue*>(&propValue));
        } else if (G_VALUE_HOLDS_BOOLEAN(&propValue)) {
            char* lowered = g_utf8_strdown(strValue, -1);
            g_object_set(G_OBJECT(settings), webSettingKey,
                         g_str_equal(lowered, "true")
                         || g_str_equal(strValue, "1"),
                         NULL);
            g_free(lowered);
        } else if (G_VALUE_HOLDS_INT(&propValue)) {
            std::string str(strValue);
            std::stringstream ss(str);
            int val = 0;
            if (!(ss >> val).fail())
                g_object_set(G_OBJECT(settings), webSettingKey, val, NULL);
        } else
            printf("LayoutTestController::overridePreference failed to override preference '%s'.\n", name);
    }

    g_free(webSettingKey);
    g_free(name);
    g_free(strValue);
#endif
}

void LayoutTestController::addUserScript(JSStringRef source, bool runAtStart, bool allFrames)
{
//    printf("LayoutTestController::addUserScript not implemented.\n");
}

void LayoutTestController::addUserStyleSheet(JSStringRef source, bool allFrames)
{
//    printf("LayoutTestController::addUserStyleSheet not implemented.\n");
}

void LayoutTestController::showWebInspector()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
#else
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    WebKitWebSettings* webSettings = webkit_web_view_get_settings(webView);
    WebKitWebInspector* inspector = webkit_web_view_get_inspector(webView);

    g_object_set(webSettings, "enable-developer-extras", TRUE, NULL);
    webkit_web_inspector_show(inspector);
#endif
}

void LayoutTestController::closeWebInspector()
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
#else
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    WebKitWebSettings* webSettings = webkit_web_view_get_settings(webView);
    WebKitWebInspector* inspector = webkit_web_view_get_inspector(webView);

    webkit_web_inspector_close(inspector);
    g_object_set(webSettings, "enable-developer-extras", FALSE, NULL);
#endif
}

void LayoutTestController::evaluateInWebInspector(long callId, JSStringRef script)
{
#if 1
    // Ugh!: implement it!
    // 100519 ACCESS Co.,Ltd.
#else
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    WebKitWebInspector* inspector = webkit_web_view_get_inspector(webView);
    char* scriptString = JSStringCopyUTF8CString(script);

    webkit_web_inspector_execute_script(inspector, callId, scriptString);
    g_free(scriptString);
#endif
}

void LayoutTestController::evaluateScriptInIsolatedWorld(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
    // FIXME: Implement this.
}

void LayoutTestController::evaluateScriptInIsolatedWorldAndReturnValue(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
    // FIXME: Implement this.
}

void LayoutTestController::removeAllVisitedLinks()
{
    // FIXME: Implement this.
}

void LayoutTestController::setAllowFileAccessFromFileURLs(bool)
{
    return;
}

void LayoutTestController::setApplicationCacheOriginQuota(unsigned long long quota)
{
    return;
}

void LayoutTestController::setAutofilled(JSContextRef, JSValueRef nodeObject, bool autofilled)
{
    return;
}

void LayoutTestController::setDomainRelaxationForbiddenForURLScheme(bool forbidden, JSStringRef scheme)
{
    return;
}

void LayoutTestController::setDefersLoading(bool)
{
    return;
}

void LayoutTestController::setJavaScriptCanAccessClipboard(bool flag)
{
    return;
}

void LayoutTestController::setMockDeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
    return;
}

void LayoutTestController::addMockSpeechInputResult(JSStringRef result, double confidence, JSStringRef language)
{
    return;
}

void LayoutTestController::setPluginsEnabled(bool flag)
{
    return;
}

void LayoutTestController::setValueForUser(JSContextRef, JSValueRef nodeObject, JSStringRef value)
{
    return;
}

void LayoutTestController::setViewModeMediaFeature(JSStringRef mode)
{
    return;
}

void LayoutTestController::setFrameFlatteningEnabled(bool enable)
{
    return;
}

void LayoutTestController::setSpatialNavigationEnabled(bool enable)
{
    return;
}

void LayoutTestController::setScrollbarPolicy(JSStringRef orientation, JSStringRef policy)
{
    return;
}

void LayoutTestController::setEditingBehavior(const char* editingBehavior)
{
    return;
}

void LayoutTestController::suspendAnimations() const
{
    return;
}

void LayoutTestController::resumeAnimations() const
{
    return;
}


void LayoutTestController::addOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef destinationProtocol, JSStringRef destinationHost, bool allowDestinationSubdomains)
{
    return;
}

void LayoutTestController::removeOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef destinationProtocol, JSStringRef destinationHost, bool allowDestinationSubdomains)
{
    return;
}



void LayoutTestController::setGeolocationPermission(bool allow)
{
    return;
}

void LayoutTestController::setDeveloperExtrasEnabled(bool)
{
    return;
}

void LayoutTestController::setAsynchronousSpellCheckingEnabled(bool)
{
    return;
}



void LayoutTestController::setWebViewEditable(bool)
{
    return;
}


void LayoutTestController::abortModal()
{
    return;
}


void LayoutTestController::dumpConfigurationForViewport(int deviceDPI, int deviceWidth, int deviceHeight, int availableWidth, int availableHeight)
{
    return;
}


void LayoutTestController::setSerializeHTTPLoads(bool serialize)
{
    return;
}




void LayoutTestController::apiTestNewWindowDataLoadBaseURL(JSStringRef utf8Data, JSStringRef baseURL)
{
    return;
}

void LayoutTestController::apiTestGoToCurrentBackForwardItem()
{
    return;
}



void LayoutTestController::authenticateSession(JSStringRef url, JSStringRef username, JSStringRef password)
{
    return;
}


JSRetainPtr<JSStringRef> LayoutTestController::layerTreeAsText() const
{
    return 0;
}


JSRetainPtr<JSStringRef> LayoutTestController::markerTextForListItem(JSContextRef context, JSValueRef nodeObject) const
{
    return 0;
}


JSValueRef LayoutTestController::originsWithLocalStorage(JSContextRef)
{
    return 0;
}

void LayoutTestController::deleteAllLocalStorage()
{
    return;
}

void LayoutTestController::deleteLocalStorageForOrigin(JSStringRef originIdentifier)
{
    return;
}

long long LayoutTestController::localStorageDiskUsageForOrigin(JSStringRef originIdentifier)
{
    return 0;
}

void LayoutTestController::observeStorageTrackerNotifications(unsigned number)
{
    return;
}

void LayoutTestController::syncLocalStorage()
{
    return;
}



void LayoutTestController::setMinimumTimerInterval(double)
{
    return;
}


bool LayoutTestController::callShouldCloseOnWebView()
{
    return false;
}

void LayoutTestController::clearAllApplicationCaches()
{
}


void LayoutTestController::clearApplicationCacheForOrigin(JSStringRef name)
{
}


long long LayoutTestController::applicationCacheDiskUsageForOrigin(JSStringRef name)
{
    return 0;
}


bool LayoutTestController::findString(JSContextRef, JSStringRef, JSObjectRef optionsArray)
{
    return false;
}

void LayoutTestController::goBack()
{
}

JSValueRef LayoutTestController::computedStyleIncludingVisitedInfo(JSContextRef, JSValueRef)
{
    return 0;
}

int LayoutTestController::pageNumberForElementById(JSStringRef id, float pageWidthInPixels, float pageHeightInPixels)
{
    return 0;
}

int LayoutTestController::numberOfPages(float pageWidthInPixels, float pageHeightInPixels)
{
    return 0;
}

int LayoutTestController::numberOfPendingGeolocationPermissionRequests()
{
    return 0;
}

JSRetainPtr<JSStringRef> LayoutTestController::pageProperty(const char* propertyName, int pageNumber) const
{
    return 0;
}

JSRetainPtr<JSStringRef> LayoutTestController::pageSizeAndMarginsInPixels(int pageNumber, int width, int height, int marginTop, int marginRight, int marginBottom, int marginLeft) const
{
    return 0;
}

bool LayoutTestController::isPageBoxVisible(int pageNumber) const
{
    return false;
}

JSValueRef LayoutTestController::originsWithApplicationCache(JSContextRef)
{
    return 0;
}

void LayoutTestController::setMockSpeechInputDumpRect(bool flag)
{
}

void LayoutTestController::allowRoundingHacks()
{
}

void LayoutTestController::addChromeInputField()
{
}

void LayoutTestController::removeChromeInputField()
{
}

void LayoutTestController::focusWebView()
{
}

void LayoutTestController::setBackingScaleFactor(double)
{
}

void LayoutTestController::simulateDesktopNotificationClick(JSStringRef title)
{
}

void LayoutTestController::setTextDirection(JSStringRef)
{
}

void LayoutTestController::setPageVisibility(const char*)
{
}

void LayoutTestController::resetPageVisibility()
{
}

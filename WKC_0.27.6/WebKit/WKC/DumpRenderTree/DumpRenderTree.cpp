/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Alp Toker <alp@nuanti.com>
 * Copyright (C) 2009 Jan Alonzo <jmalonzo@gmail.com>
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

#include "DumpRenderTreeWKC.h"

#include "CString.h"
#include "DOMWindow.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameView.h"
#include "Element.h"
#include "RenderTreeAsText.h"

#include "AccessibilityController.h"
#include "EventSender.h"
#include "GCController.h"
#include "LayoutTestController.h"
#include "WorkQueue.h"

#include "WKCWebView.h"
#include "WKCWebFrame.h"

#include "helpers/WKCFrame.h"
#include "helpers/privates/WKCFramePrivate.h"
#include "helpers/WKCSettings.h"

#include "Settings.h"

class GCController {
public:
    GCController();
    ~GCController();

    void makeWindowObject(JSContextRef in_context, JSObjectRef in_window_object, JSValueRef* in_exception);

    // Controller Methods - platfrom independant implementations
    void collect() const;
    void collectOnAlternateThread(bool in_wait_until_done) const;
    size_t getJSObjectCount() const;

private:
    static JSClassRef getJSClass();
};

static DumpRenderTreeWKC* gSelf = 0;

DumpRenderTreeWKC::DumpRenderTreeWKC(DumpRenderTreeEventReceiver& in_ui, WKC::WKCWebView* in_view)
    : fUI(in_ui)
    , fView(in_view)
{
    fTopLoadingFrame = 0;
    fController = 0;
    fGCController = 0;
    fAXController = 0;
    fWaitForPolicy = false;
    fDone = false;

    fDumpTree = true;
    fDumpPixels = false;
    fPrintSeparators = true;

    fMainFrame = fView->mainFrame();

    fUserStyleSheetEnabled = true;
    fUserStyleSheet = 0;

    fTimer = 0;
    fWatchdogTimer = 0;
    fWaitToDumpWatchdog = false;
}

DumpRenderTreeWKC::~DumpRenderTreeWKC()
{
    if (fUserStyleSheet)
        free(fUserStyleSheet);
    if (fTimer)
        wkcTimerDeletePeer(fTimer);
    if (fWatchdogTimer)
        wkcTimerDeletePeer(fWatchdogTimer);
    if (fController)
        delete fController;
    if (fAXController)
        delete fAXController;
    if (fGCController)
        delete fGCController;
    if (0) {
        // dummy calling for linking
        JSEvaluateScript(0, 0, 0, 0, 0, 0);
    }
}

DumpRenderTreeWKC*
DumpRenderTreeWKC::create(DumpRenderTreeEventReceiver& in_ui, WKC::WKCWebView* in_view)
{
    DumpRenderTreeWKC* self = 0;
    self = new DumpRenderTreeWKC(in_ui, in_view);
    if (!self)
        return 0;
    if (!self->construct()) {
        delete self;
        return 0;
    }
    gSelf = self;
    return self;
}

bool
DumpRenderTreeWKC::construct()
{
    fGCController = new GCController();
    if (!fGCController)
        return false;
    fAXController = new AccessibilityController();
    if (!fAXController)
        return false;

    fTimer = wkcTimerNewPeer();
    if (!fTimer)
        return false;
    fWatchdogTimer = wkcTimerNewPeer();
    if (!fWatchdogTimer)
        return false;
    return true;
}

WKC::WKCWebFrame*
DumpRenderTreeWKC::getMainFrame()
{
    return gSelf->fMainFrame;
}

WKC::WKCWebFrame*
DumpRenderTreeWKC::topLoadingFrame()
{
    return gSelf->fTopLoadingFrame;
}

void
DumpRenderTreeWKC::setWaitForPolicy(bool in_flag)
{
    gSelf->fWaitForPolicy = in_flag;
}

void
DumpRenderTreeWKC::setUserStyleSheetEnabled(bool in_flag)
{
    gSelf->fUserStyleSheetEnabled = in_flag;
}

bool
DumpRenderTreeWKC::userStyleSheetEnabled()
{
    return gSelf->fUserStyleSheetEnabled;
}

void
DumpRenderTreeWKC::setUserStyleSheet(const char* in_style_sheet)
{
    gSelf->fUserStyleSheet = strdup(in_style_sheet);
}

const char*
DumpRenderTreeWKC::userStyleSheet()
{
    return gSelf->fUserStyleSheet;
}


bool
DumpRenderTreeWKC::waitToDumpWatchdogFired()
{
    fWaitToDumpWatchdog = false;
    fController->waitToDumpWatchdogTimerFired();
    return false;
}

void
DumpRenderTreeWKC::beginWatchdog(int in_ms)
{
    if (gSelf->fWaitToDumpWatchdog)
        return;
    gSelf->fWaitToDumpWatchdog = true;
    wkcTimerStartOneShotPeer(gSelf->fWatchdogTimer, in_ms/1000.0, waitToDumpWatchdogFiredProc, gSelf);
}

int
DumpRenderTreeWKC::webViewCount()
{
    // Ugh!: support multiple window!
    // 100520 ACCESS Co.,Ltd.
    return 1;
}

void
DumpRenderTreeWKC::notifyEvent(const DumpRenderTreeEvent& in_ev)
{
    gSelf->fUI.dumpRenderTreePostEvent(in_ev);
}

void
DumpRenderTreeWKC::resetDefaultsToConsistentValues()
{
    WKC::WKCSettings* settings = 0;

    settings = fView->settings();
    settings->setPrivateBrowsingEnabled(false);
    settings->setDeveloperExtrasEnabled(false);
    settings->setXSSAuditorEnabled(false);
    settings->setJavaScriptCanOpenWindowsAutomatically(true);
    settings->setAllowUniversalAccessFromFileURLs(true);
    settings->setScriptEnabled(true);
    settings->setDOMPasteAllowed(true);
    settings->setStandardFontFamily("Times");
    settings->setFixedFontFamily("Courier");
    settings->setSerifFontFamily("Times");
    settings->setSansSerifFontFamily("Helvetica");
    settings->setDefaultFontSize(16);
    settings->setDefaultFixedFontSize(13);
    settings->setMinimumFontSize(1);
    settings->setCaretBrowsingEnabled(false);
    settings->setUsesPageCache(false);
}

static bool shouldLogFrameLoadDelegates(const char* in_path_or_url)
{
    return strstr(in_path_or_url, "loading/");
}

static bool shouldOpenWebInspector(const char* in_path_or_url)
{
    return strstr(in_path_or_url, "inspector/");
}

bool
DumpRenderTreeWKC::beginTest(const char* uri)
{
    std::string path_or_url(uri);
    std::string expected_pixel_hash;

    size_t separator_pos = path_or_url.find("'");
    if (separator_pos != std::string::npos) {
        path_or_url = std::string(uri, 0, separator_pos);
        expected_pixel_hash = std::string(uri, separator_pos + 1);
    }

    const std::string test_url(uri);

    resetDefaultsToConsistentValues();

    fController = LayoutTestController::create(test_url, expected_pixel_hash).leakRef();
    fTopLoadingFrame = 0;
    fDone = false;

    fController->setIconDatabaseEnabled(false);

    if (shouldLogFrameLoadDelegates(path_or_url.c_str()))
        fController->setDumpFrameLoadCallbacks(true);

    if (shouldOpenWebInspector(path_or_url.c_str()))
        fController->showWebInspector();

#if 0
    bool isSVGW3CTest = (fController->testPathOrURL().find("svg/W3C-SVG-1.1") != string::npos);
    GtkAllocation size;
    size.x = size.y = 0;
    size.width = isSVGW3CTest ? 480 : maxViewWidth;
    size.height = isSVGW3CTest ? 360 : maxViewHeight;
    gtk_window_resize(GTK_WINDOW(window), size.width, size.height);
    gtk_widget_size_allocate(container, &size);
#endif

#if 0
    if (prevTestBFItem)
        g_object_unref(prevTestBFItem);
    WebKitWebBackForwardList* bfList = webkit_web_view_get_back_forward_list(webView);
    prevTestBFItem = webkit_web_back_forward_list_get_current_item(bfList);
    if (prevTestBFItem)
        g_object_ref(prevTestBFItem);
#endif

    fView->loadURI(uri);

    return true;
}

void
DumpRenderTreeWKC::notifyLoadingStart(void* in_frame)
{
    WKC::WKCWebFrame* frame = (WKC::WKCWebFrame *)in_frame;

    if (!fTopLoadingFrame && !fDone)
        fTopLoadingFrame = frame;
}

static char* getFrameNameSuitableForTestResult(WKC::WKCWebView* in_view, WKC::WKCWebFrame* in_frame)
{
    WTF::String frame_name(in_frame->name());
    WTF::String tmp("main frame ");
    const char* u8 = "";

    if (in_frame == in_view->mainFrame()) {
        if (frame_name.length()) {
            tmp.append('"');
            tmp.append(frame_name);
            tmp.append('"');
            tmp.append('\0');
            u8 = tmp.utf8().data();
        } else {
            u8 = "main frame";
        }
    } else if (!frame_name.length()) {
        u8 = "frame (anonymous)";
    }

    return strdup(u8);
}

void
DumpRenderTreeWKC::notifyLoadingEnd(void* in_frame)
{
    WKC::WKCWebFrame* frame = (WKC::WKCWebFrame *)in_frame;
    WKC::Frame* wkcframe = frame->core();
    WebCore::Frame* coreframe= wkcframe->priv().webcore();

    if (!fDone && !fController->dumpFrameLoadCallbacks()) {
        int pending_frame_unload_events = coreframe->document()->domWindow()->pendingUnloadEventListeners();;
        if (pending_frame_unload_events) {
            char* frame_name = getFrameNameSuitableForTestResult(fView, frame);
            fUI.dumpRenderTreePrintf("%s - has %u onunload handler(s)\n", frame_name, pending_frame_unload_events);
            free(frame_name);
        }
    }

    if (frame != fTopLoadingFrame)
        return;

    fTopLoadingFrame = 0;
    if (fController->waitToDump())
        return;

    dump();
}

void
DumpRenderTreeWKC::notifyLoadingFailed(void* in_frame)
{
    WKC::WKCWebFrame* frame = (WKC::WKCWebFrame *)in_frame;

    if (frame != fTopLoadingFrame)
        return;

    fTopLoadingFrame = 0;
    if (fController->waitToDump())
        return;

    dump();
}

bool
DumpRenderTreeWKC::processWork()
{
    // if we finish all the commands, we're ready to dump state
    if (!fController->waitToDump())
        dump();

    return false;
}


void
DumpRenderTreeWKC::notifyWindowObjectCleared(void* in_context, void* in_window_object)
{
    JSGlobalContextRef context = (JSGlobalContextRef)in_context;
    JSObjectRef window_object = (JSObjectRef)in_window_object;
    JSValueRef exception = 0;

    ASSERT(fController);

    fController->makeWindowObject(context, window_object, &exception);
    ASSERT(!exception);

    fGCController->makeWindowObject(context, window_object, &exception);
    ASSERT(!exception);

    fAXController->makeWindowObject(context, window_object, &exception);
    ASSERT(!exception);

    JSStringRef event_sender_str = JSStringCreateWithUTF8CString("eventSender");
    JSValueRef event_sender = makeEventSender(context);
    JSObjectSetProperty(context, window_object, event_sender_str, event_sender, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, 0);
    JSStringRelease(event_sender_str);
}

static char*
_dumpFramesAsText(LayoutTestController* controller, WebCore::Frame* in_core_frame, bool is_main_frame)
{
    WTF::String result("");

    // Add header for all but the main frame.

    WebCore::FrameView* view = in_core_frame->view();

    if (view && view->layoutPending())
        view->layout();

    WebCore::Element* document_element = in_core_frame->document()->documentElement();
    WTF::String inner_text = document_element->innerText();

    if (!is_main_frame) {
        WTF::String frame_name(in_core_frame->tree().name());
        result.append("\n--------\nFrame: '");
        result.append(frame_name);
        result.append("'\n--------\n");
    }
    result.append(inner_text);
    result.append("\n");

    if (controller->dumpChildFramesAsText()) {
        for (WebCore::Frame* child = in_core_frame->tree().firstChild(); child; child = child->tree().nextSibling()) {
            char* tmp = _dumpFramesAsText(controller, child, false);
            result.append(tmp);
            free(tmp);
        }
    }

    return strdup(result.utf8().data());
}

char*
DumpRenderTreeWKC::dumpFramesAsText(WKC::WKCWebFrame* in_frame)
{
    WKC::Frame* wkcframe = in_frame->core();
    WebCore::Frame* core_frame = wkcframe->priv().webcore();

    bool is_main_frame = (fMainFrame == in_frame);
    if (!core_frame)
        return strdup("");
    return _dumpFramesAsText(fController, core_frame, is_main_frame);
}

char*
DumpRenderTreeWKC::dumpRenderTree(WKC::WKCWebFrame* frame)
{
    WKC::Frame* wkcframe = frame->core();
    WebCore::Frame* core_frame= wkcframe->priv().webcore();
    if (!core_frame)
        return strdup("");

    WebCore::FrameView* view = core_frame->view();

    if (view && view->layoutPending())
        view->layout();

    WTF::String string = WebCore::externalRepresentation(core_frame);
    return strdup(string.utf8().data());
}

void
DumpRenderTreeWKC::dump_()
{
    fWaitForPolicy = false;

    WKC::Frame* wkcframe = fMainFrame->core();
    WebCore::Frame* coreframe= wkcframe->priv().webcore();

    bool dump_as_text = fController->dumpAsText();
    if (fDumpTree) {
        char* result = 0;
        WebCore::DocumentLoader* doc_loader = coreframe->loader().documentLoader();
        WTF::String mime_type = doc_loader->responseMIMEType();
        if (mime_type.contains("text/plain", true)) {
            dump_as_text = true;
        }

        // Test can request controller to be dumped as text even
        // while test's response mime type is not text/plain.
        // Overriding this behavior with dumpAsText being false is a bad idea.
        if (dump_as_text)
            fController->setDumpAsText(dump_as_text);

        if (fController->dumpAsText())
            result = dumpFramesAsText(fMainFrame);
        else
            result = dumpRenderTree(fMainFrame);

        if (!result) {
            const char* error_message;
            if (fController->dumpAsText())
                error_message = "[documentElement innerText]";
            else if (fController->dumpDOMAsWebArchive())
                error_message = "[[mainFrame DOMDocument] webArchive]";
            else if (fController->dumpSourceAsWebArchive())
                error_message = "[[mainFrame dataSource] webArchive]";
            else
                error_message = "[mainFrame renderTreeAsExternalRepresentation]";
            fUI.dumpRenderTreePrintf("ERROR: nil result from %s", error_message);
        } else {
            fUI.dumpRenderTreePrintf("%s", result);
            free(result);
            if (!fController->dumpAsText() && !fController->dumpDOMAsWebArchive() && !fController->dumpSourceAsWebArchive()) {
//                dumpFrameScrollPosition(fMainFrame);
            }

            if (fController->dumpBackForwardList()) {
//                dumpBackForwardListForAllWebViews();
            }
        }

//        if (fPrintSeparators) {
//            fUI.dumpRenderTreePrintf("#EOF");
//        }
    }

    if (fDumpPixels) {
        if (!fController->dumpAsText() && !fController->dumpDOMAsWebArchive() && !fController->dumpSourceAsWebArchive()) {
            // FIXME: Add support for dumping pixels
        }
    }

    // FIXME: call displayWebView here when we support --paint

    fDone = true;

    fUI.dumpRenderTreeNotifyFinished();
}

void
DumpRenderTreeWKC::dump()
{
    gSelf->dump_();
}

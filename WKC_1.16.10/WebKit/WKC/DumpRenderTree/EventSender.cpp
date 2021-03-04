/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * Copyright (c) 2009,2010,2015 ACCESS CO., LTD. All rights reserved.
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
#include "EventSender.h"

#include "DumpRenderTree.h"
#include "DumpRenderTreeWKC.h"
#include "Frame.h"
#include "FrameView.h"

#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRef.h>
#include <wtf/ASCIICType.h>
#include <wtf/Platform.h>

#include "WKCWebView.h"
#include "WKCWebFrame.h"

// TODO: Currently drag and drop related code is left out and
// should be merged once we have drag and drop support in WebCore.

#if PLATFORM(WKC)
WKC_DEFINE_GLOBAL_BOOL(down, false);
WKC_DEFINE_GLOBAL_INT(currentEventButton, 1);
WKC_DEFINE_GLOBAL_BOOL(dragMode, true);
WKC_DEFINE_GLOBAL_BOOL(replayingSavedEvents,false);
WKC_DEFINE_GLOBAL_INT(lastMousePositionX, 0);
WKC_DEFINE_GLOBAL_INT(lastMousePositionY, 0);

WKC_DEFINE_GLOBAL_INT(lastClickPositionX, 0);
WKC_DEFINE_GLOBAL_INT(lastClickPositionY, 0);
WKC_DEFINE_GLOBAL_INT(clickCount, 0);
WKC_DEFINE_GLOBAL_INT(modifiers, 0);
#else
static bool down = false;
static int currentEventButton = 1;
static bool dragMode = true;
static bool replayingSavedEvents = false;
static int lastMousePositionX;
static int lastMousePositionY;

static int lastClickPositionX;
static int lastClickPositionY;
static int clickCount = 0;
static int modifiers = 0;
#endif

struct DelayedMessage {
    DumpRenderTreeEvent event;
    unsigned long delay;
    bool isDragEvent;
};

static DelayedMessage msgQueue[1024];

#if PLATFORM(WKC)
WKC_DEFINE_GLOBAL_UINT(endOfQueue, 0);
WKC_DEFINE_GLOBAL_UINT(startOfQueue, 0);
#else
static unsigned endOfQueue;
static unsigned startOfQueue;
#endif

static const float zoomMultiplierRatio = 1.2f;


DumpRenderTreeEvent::DumpRenderTreeEvent()
: fType(ENone)
, fX(0)
, fY(0)
, fButton(0)
, fModifiers(EModifierNone)
, fDir(EDirNone)
{
}

DumpRenderTreeEvent::DumpRenderTreeEvent(int type, int x, int y)
: fType(type)
, fX(x)
, fY(y)
, fButton(0)
, fModifiers(EModifierNone)
, fDir(EDirNone)
{
}

DumpRenderTreeEvent::~DumpRenderTreeEvent()
{
}

static JSValueRef getDragModeCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeBoolean(context, dragMode);
}

static bool setDragModeCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    dragMode = JSValueToBoolean(context, value);
    return true;
}

static JSValueRef leapForwardCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    // FIXME: Add proper support for forward leaps
    return JSValueMakeUndefined(context);
}

static JSValueRef contextClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WebCore::Frame* coreFrame = (WebCore::Frame *)mainFrame->core();
    WebCore::FrameView* frameView = coreFrame->view();

    frameView->layout();

    DumpRenderTreeEvent ev(DumpRenderTreeEvent::EEventMouseDown, lastMousePositionX, lastMousePositionY);
    down = true;
    DumpRenderTreeWKC::notifyEvent(ev);

    ev.fType = DumpRenderTreeEvent::EEventMouseUp;
    down = false;
    DumpRenderTreeWKC::notifyEvent(ev);

    return JSValueMakeUndefined(context);
}

static void updateClickCount(int button)
{
    // FIXME: take the last clicked button number and the time of last click into account.
    if (lastClickPositionX != lastMousePositionX || lastClickPositionY != lastMousePositionY || currentEventButton != button)
        clickCount = 1;
    else
        clickCount++;
}

static JSValueRef mouseDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WebCore::Frame* coreFrame = (WebCore::Frame *)mainFrame->core();
    WebCore::FrameView* frameView = coreFrame->view();

    down = true;

    DumpRenderTreeEvent ev(DumpRenderTreeEvent::EEventMouseDown, lastMousePositionX, lastMousePositionY);

    if (argumentCount == 1) {
        ev.fButton = (int)JSValueToNumber(context, arguments[0], exception) + 1;
        if (exception && *exception) {
            return JSValueMakeUndefined(context);
        }
    }

    currentEventButton = ev.fButton;

    updateClickCount(ev.fButton);

    if (!msgQueue[endOfQueue].delay) {
        frameView->layout();

        DumpRenderTreeWKC::notifyEvent(ev);
        if (clickCount == 2) {
            ev.fType = DumpRenderTreeEvent::EEventMouse2Down;
            DumpRenderTreeWKC::notifyEvent(ev);
        }
    } else {
        // replaySavedEvents should have the required logic to make leapForward delays work
        msgQueue[endOfQueue++].event = ev;
        replaySavedEvents();
    }

    return JSValueMakeUndefined(context);
}

static JSValueRef mouseUpCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WebCore::Frame* coreFrame = (WebCore::Frame *)mainFrame->core();
    WebCore::FrameView* frameView = coreFrame->view();

    DumpRenderTreeEvent ev(DumpRenderTreeEvent::EEventMouseUp, lastMousePositionX, lastMousePositionY);

    if (argumentCount == 1) {
        ev.fButton = (int)JSValueToNumber(context, arguments[0], exception) + 1;
        if (exception && *exception) {
            return JSValueMakeUndefined(context);
        }
    }

    currentEventButton = ev.fButton;

    down = false;

    if ((dragMode && !replayingSavedEvents) || msgQueue[endOfQueue].delay) {
        msgQueue[endOfQueue].event = ev;
        msgQueue[endOfQueue++].isDragEvent = true;
        replaySavedEvents();
    } else {
        frameView->layout();
        DumpRenderTreeWKC::notifyEvent(ev);
    }

    lastClickPositionX = lastMousePositionX;
    lastClickPositionY = lastMousePositionY;

    return JSValueMakeUndefined(context);
}

static JSValueRef mouseMoveToCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WebCore::Frame* coreFrame = (WebCore::Frame *)mainFrame->core();
    WebCore::FrameView* frameView = coreFrame->view();

    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    lastMousePositionX = (int)JSValueToNumber(context, arguments[0], exception);
    if (exception && *exception) {
        return JSValueMakeUndefined(context);
    }
    lastMousePositionY = (int)JSValueToNumber(context, arguments[1], exception);
    if (exception && *exception) {
        return JSValueMakeUndefined(context);
    }

    DumpRenderTreeEvent ev(DumpRenderTreeEvent::EEventMouseMove, lastMousePositionX, lastMousePositionY);

    ev.fModifiers = modifiers;

    if (dragMode && down && !replayingSavedEvents) {
        msgQueue[endOfQueue].event = ev;
        msgQueue[endOfQueue++].isDragEvent = true;
    } else {
        frameView->layout();
        DumpRenderTreeWKC::notifyEvent(ev);
    }

    return JSValueMakeUndefined(context);
}

static JSValueRef mouseWheelToCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WebCore::Frame* coreFrame = (WebCore::Frame *)mainFrame->core();
    WebCore::FrameView* frameView = coreFrame->view();

    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    int horizontal = (int)JSValueToNumber(context, arguments[0], exception);
    if (exception && *exception) {
        return JSValueMakeUndefined(context);
    }
    int vertical = (int)JSValueToNumber(context, arguments[1], exception);
    if (exception && *exception) {
        return JSValueMakeUndefined(context);
    }

    DumpRenderTreeEvent ev(DumpRenderTreeEvent::EEventMouseWheel, lastMousePositionX, lastMousePositionY);

    if (horizontal < 0)
        ev.fDir = DumpRenderTreeEvent::ELeft;
    else if (horizontal > 0)
        ev.fDir = DumpRenderTreeEvent::ERight;
    else if (vertical < 0)
        ev.fDir = DumpRenderTreeEvent::EUp;
    else if (vertical > 0)
        ev.fDir = DumpRenderTreeEvent::EDown;
    else {
        return JSValueMakeUndefined(context);
    }

    if (dragMode && down && !replayingSavedEvents) {
        msgQueue[endOfQueue].event = ev;
        msgQueue[endOfQueue++].isDragEvent = true;
    } else {
        frameView->layout();
        DumpRenderTreeWKC::notifyEvent(ev);
    }

    return JSValueMakeUndefined(context);
}

static JSValueRef beginDragWithFilesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    // FIXME: Implement this completely once WebCore has complete drag and drop support
    return JSValueMakeUndefined(context);
}

void replaySavedEvents()
{
    // FIXME: This doesn't deal with forward leaps, but it should.

    replayingSavedEvents = true;

    for (unsigned queuePos = 0; queuePos < endOfQueue; queuePos++) {
        DumpRenderTreeEvent ev = msgQueue[queuePos].event;

        switch (ev.fType) {
        case DumpRenderTreeEvent::EEventMouseUp:
        case DumpRenderTreeEvent::EEventMouseDown:
        case DumpRenderTreeEvent::EEventMouseMove:
            DumpRenderTreeWKC::notifyEvent(ev);
            break;
        default:
            break;
        }

        startOfQueue++;
    }

    int numQueuedMessages = endOfQueue - startOfQueue;
    if (!numQueuedMessages) {
        startOfQueue = 0;
        endOfQueue = 0;
        replayingSavedEvents = false;
        return;
    }

    startOfQueue = 0;
    endOfQueue = 0;

    replayingSavedEvents = false;
}

static JSValueRef keyDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WebCore::Frame* coreFrame = (WebCore::Frame *)mainFrame->core();
    WebCore::FrameView* frameView = coreFrame->view();

    if (argumentCount < 1)
        return JSValueMakeUndefined(context);

    static const JSStringRef lengthProperty = JSStringCreateWithUTF8CString("length");

    frameView->layout();

    // handle modifier keys.
    int state = 0;
    if (argumentCount > 1) {
        JSObjectRef modifiersArray = JSValueToObject(context, arguments[1], exception);
        if (modifiersArray) {
            for (int i = 0; i < JSValueToNumber(context, JSObjectGetProperty(context, modifiersArray, lengthProperty, 0), 0); ++i) {
                JSValueRef value = JSObjectGetPropertyAtIndex(context, modifiersArray, i, 0);
                JSStringRef string = JSValueToStringCopy(context, value, 0);
                if (JSStringIsEqualToUTF8CString(string, "ctrlKey"))
                    state |= DumpRenderTreeEvent::EControl;
                else if (JSStringIsEqualToUTF8CString(string, "shiftKey"))
                    state |= DumpRenderTreeEvent::EShift;
                else if (JSStringIsEqualToUTF8CString(string, "altKey"))
                    state |= DumpRenderTreeEvent::EAlt;

                JSStringRelease(string);
            }
        }
    }

    JSStringRef character = JSValueToStringCopy(context, arguments[0], exception);
    if (exception && *exception) {
        return JSValueMakeUndefined(context);
    }

    int key;
    if (JSStringIsEqualToUTF8CString(character, "leftArrow"))
        key = DumpRenderTreeEvent::EKeyLeft;
    else if (JSStringIsEqualToUTF8CString(character, "rightArrow"))
        key = DumpRenderTreeEvent::EKeyRight;
    else if (JSStringIsEqualToUTF8CString(character, "upArrow"))
        key = DumpRenderTreeEvent::EKeyUp;
    else if (JSStringIsEqualToUTF8CString(character, "downArrow"))
        key = DumpRenderTreeEvent::EKeyDown;
    else if (JSStringIsEqualToUTF8CString(character, "pageUp"))
        key = DumpRenderTreeEvent::EKeyPageUp;
    else if (JSStringIsEqualToUTF8CString(character, "pageDown"))
        key = DumpRenderTreeEvent::EKeyPageDown;
    else if (JSStringIsEqualToUTF8CString(character, "home"))
        key = DumpRenderTreeEvent::EKeyHome;
    else if (JSStringIsEqualToUTF8CString(character, "end"))
        key = DumpRenderTreeEvent::EKeyEnd;
    else if (JSStringIsEqualToUTF8CString(character, "delete"))
        key = DumpRenderTreeEvent::EKeyBackSpace;
    else if (JSStringIsEqualToUTF8CString(character, "F1"))
        key = DumpRenderTreeEvent::EKeyF1;
    else if (JSStringIsEqualToUTF8CString(character, "F2"))
        key = DumpRenderTreeEvent::EKeyF2;
    else if (JSStringIsEqualToUTF8CString(character, "F3"))
        key = DumpRenderTreeEvent::EKeyF3;
    else if (JSStringIsEqualToUTF8CString(character, "F4"))
        key = DumpRenderTreeEvent::EKeyF4;
    else if (JSStringIsEqualToUTF8CString(character, "F5"))
        key = DumpRenderTreeEvent::EKeyF5;
    else if (JSStringIsEqualToUTF8CString(character, "F6"))
        key = DumpRenderTreeEvent::EKeyF6;
    else if (JSStringIsEqualToUTF8CString(character, "F7"))
        key = DumpRenderTreeEvent::EKeyF7;
    else if (JSStringIsEqualToUTF8CString(character, "F8"))
        key = DumpRenderTreeEvent::EKeyF8;
    else if (JSStringIsEqualToUTF8CString(character, "F9"))
        key = DumpRenderTreeEvent::EKeyF9;
    else if (JSStringIsEqualToUTF8CString(character, "F10"))
        key = DumpRenderTreeEvent::EKeyF10;
    else if (JSStringIsEqualToUTF8CString(character, "F11"))
        key = DumpRenderTreeEvent::EKeyF11;
    else if (JSStringIsEqualToUTF8CString(character, "F12"))
        key = DumpRenderTreeEvent::EKeyF12;
    else {
        if (JSStringGetCharactersPtr(character)) {
            int charCode = JSStringGetCharactersPtr(character)[0];
            if (charCode == '\n' || charCode == '\r')
                key = DumpRenderTreeEvent::EKeyReturn;
            else if (charCode == '\t')
                key = DumpRenderTreeEvent::EKeyTab;
            else if (charCode == '\x8')
                key = DumpRenderTreeEvent::EKeyBackSpace;
            else {
                key = charCode;
                if (WTF::isASCIIUpper(charCode))
                    state |= DumpRenderTreeEvent::EShift;
            }
        }
    }

    JSStringRelease(character);

    // create and send the event
    DumpRenderTreeEvent ev(DumpRenderTreeEvent::EEventKeyDown, 0, 0);
    ev.fX = key;
    ev.fModifiers = state;
    modifiers = state;

    DumpRenderTreeWKC::notifyEvent(ev);

    ev.fType = DumpRenderTreeEvent::EEventKeyUp;
    DumpRenderTreeWKC::notifyEvent(ev);

    return JSValueMakeUndefined(context);
}

static JSValueRef textZoomInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    if (!view)
        return JSValueMakeUndefined(context);

    float currentZoom = view->zoomLevel();
    view->setZoomLevel(currentZoom * zoomMultiplierRatio);

    return JSValueMakeUndefined(context);
}

static JSValueRef textZoomOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    if (!view)
        return JSValueMakeUndefined(context);

    float currentZoom = view->zoomLevel();
    view->setZoomLevel(currentZoom / zoomMultiplierRatio);

    return JSValueMakeUndefined(context);
}

static JSValueRef zoomPageInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    view->zoomIn(10);
    return JSValueMakeUndefined(context);
}

static JSValueRef zoomPageOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    view->zoomOut(10);
    return JSValueMakeUndefined(context);
}

static JSStaticFunction staticFunctions[] = {
    { "mouseWheelTo", mouseWheelToCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "contextClick", contextClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseDown", mouseDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseUp", mouseUpCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseMoveTo", mouseMoveToCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "beginDragWithFiles", beginDragWithFilesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "leapForward", leapForwardCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "keyDown", keyDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "textZoomIn", textZoomInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "textZoomOut", textZoomOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "zoomPageIn", zoomPageInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "zoomPageOut", zoomPageOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { 0, 0, 0 }
};

static JSStaticValue staticValues[] = {
    { "dragMode", getDragModeCallback, setDragModeCallback, kJSPropertyAttributeNone },
    { 0, 0, 0, 0 }
};

static JSClassRef getClass(JSContextRef context)
{
    static JSClassRef eventSenderClass = 0;

    if (!eventSenderClass) {
        JSClassDefinition classDefinition = {
                0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        classDefinition.staticFunctions = staticFunctions;
        classDefinition.staticValues = staticValues;

        eventSenderClass = JSClassCreate(&classDefinition);
    }

    return eventSenderClass;
}

JSObjectRef makeEventSender(JSContextRef context)
{
    down = false;
    dragMode = true;
    lastMousePositionX = lastMousePositionY = 0;
    lastClickPositionX = lastClickPositionY = 0;

    if (!replayingSavedEvents) {
        // This function can be called in the middle of a test, even
        // while replaying saved events. Resetting these while doing that
        // can break things.
        endOfQueue = 0;
        startOfQueue = 0;
    }

    return JSObjectMake(context, getClass(context), 0);
}

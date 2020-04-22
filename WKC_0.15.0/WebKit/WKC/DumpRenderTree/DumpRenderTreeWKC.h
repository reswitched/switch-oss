/*
 *  DumpRenderTreeWKC.h
 *
 *  Copyright (c) 2010,2011 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#ifndef DUMPRENDERTREEWKC_H
#define DUMPRENDERTREEWKC_H

#include <wkc/wkcbase.h>

namespace WKC {
class WKCWebView;
class WKCWebFrame;
}

class LayoutTestController;
class GCController;
class AccessibilityController;

class WKC_API DumpRenderTreeEvent {
public:
    enum {
        ENone,
        EEventKeyDown,
        EEventKeyUp,
        EEventMouseDown,
        EEventMouse2Down,
        EEventMouseUp,
        EEventMouseMove,
        EEventMouseWheel,
    };
    enum {
        EModifierNone = 0,
        EShift = 1,
        EControl = 2,
        EAlt = 4,
    };
    enum {
        EDirNone,
        EUp,
        EDown,
        ELeft,
        ERight
    };
    enum {
        EKeyLeft = 0x10000,
        EKeyRight,
        EKeyUp,
        EKeyDown,
        EKeyReturn,
        EKeyTab,
        EKeyBackSpace,
        EKeyPageUp,
        EKeyPageDown,
        EKeyHome,
        EKeyEnd,
        EKeyF1, EKeyF2, EKeyF3, EKeyF4, EKeyF5, EKeyF6, EKeyF7, EKeyF8, EKeyF9, EKeyF10, EKeyF11, EKeyF12,
    };

    DumpRenderTreeEvent();
    DumpRenderTreeEvent(int in_type, int in_x, int in_y);
    ~DumpRenderTreeEvent();

public:
    int fType;
    int fX;
    int fY;
    int fButton;
    int fModifiers;
    int fDir;
};

class WKC_API DumpRenderTreeEventReceiver {
public:
    virtual void dumpRenderTreePostEvent(const DumpRenderTreeEvent& in_event) = 0;
    virtual void dumpRenderTreePrintf(const char* in_fmt, ...) = 0;
    virtual void dumpRenderTreeNotifyFinished() = 0;
};

class WKC_API DumpRenderTreeWKC {
public:
    static DumpRenderTreeWKC* create(DumpRenderTreeEventReceiver& in_ui, WKC::WKCWebView* in_view);
    ~DumpRenderTreeWKC();

    bool beginTest(const char* in_uri);

    void notifyLoadingStart(void* in_frame);
    void notifyWindowObjectCleared(void* in_context, void* in_window_object);
    void notifyLoadingEnd(void* in_frame);
    void notifyLoadingFailed(void* in_frame);

    static void setWaitForPolicy(bool in_flag);

    static WKC::WKCWebFrame* getMainFrame();
    static WKC::WKCWebFrame* topLoadingFrame();

    static void setUserStyleSheetEnabled(bool in_flag);
    static bool userStyleSheetEnabled();
    static void setUserStyleSheet(const char* in_style_sheet);
    static const char* userStyleSheet();

    static void beginWatchdog(int in_ms);

    static int webViewCount();

    static void notifyEvent(const DumpRenderTreeEvent& in_ev);

    static void dump();

private:
    DumpRenderTreeWKC(DumpRenderTreeEventReceiver& in_ui, WKC::WKCWebView* in_view);
    bool construct();

    void resetDefaultsToConsistentValues();

    static bool processWorkProc(void* in_obj) { return ((DumpRenderTreeWKC *)(in_obj))->processWork(); }
    bool processWork();

    static bool waitToDumpWatchdogFiredProc(void* in_obj) { return ((DumpRenderTreeWKC *)(in_obj))->waitToDumpWatchdogFired(); }
    bool waitToDumpWatchdogFired();

    void dump_();
    char* dumpFramesAsText(WKC::WKCWebFrame* in_frame);
    char* dumpRenderTree(WKC::WKCWebFrame* in_frame);

private:
    DumpRenderTreeEventReceiver& fUI;
    WKC::WKCWebView* fView;
    WKC::WKCWebFrame* fMainFrame;
    WKC::WKCWebFrame* fTopLoadingFrame;

    LayoutTestController* fController;
    GCController* fGCController;
    AccessibilityController* fAXController;

    bool fWaitForPolicy;
    bool fWaitToDumpWatchdog;
    bool fDone;

    bool fDumpTree;
    bool fDumpPixels;
    bool fPrintSeparators;

    bool fUserStyleSheetEnabled;
    char* fUserStyleSheet;

    void* fTimer;
    void* fWatchdogTimer;
};

#endif // DUMPRENDERTREEWKC_H

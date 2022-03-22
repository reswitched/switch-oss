/*
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (c) 2009,2010 ACCESS CO., LTD. All rights reserved.
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

#if COMPILER(MSVC) && _MSC_VER >= 1700
extern bool __uncaught_exception();
#endif

#include "DumpRenderTree.h"
#include "DumpRenderTreeWKC.h"
#include "Frame.h"
#include "WTFString.h"
#include "WorkQueueItem.h"

#include "FrameLoaderClientWKC.h"
#include "WKCWebFrame.h"
#include "WKCWebView.h"

#include <JavaScriptCore/JSStringRef.h>

static char* JSStringCopyUTF8CString(JSStringRef jsString)
{
    size_t dataSize = JSStringGetMaximumUTF8CStringSize(jsString);
    char* utf8 = (char*)malloc(dataSize);
    JSStringGetUTF8CString(jsString, utf8, dataSize);

    return utf8;
}

bool LoadItem::invoke() const
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebFrame* targetFrame = 0;

    JSStringRef targetString(m_target.get());

    if (!JSStringGetLength(targetString)) {
        targetFrame = mainFrame;
    } else {
        const JSChar* data = JSStringGetCharactersPtr(targetString);
        targetFrame = mainFrame->findFrame(data);
    }

    if (!targetFrame)
        return false;

    char* uri = JSStringCopyUTF8CString(m_url.get());
    targetFrame->loadURI(uri);
    free(uri);

    return true;
}

bool LoadHTMLStringItem::invoke() const
{
    return false;
}

bool ReloadItem::invoke() const
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    mainFrame->reload();
    return true;
}

bool ScriptItem::invoke() const
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* view = mainFrame->webView();
    char* scriptString = JSStringCopyUTF8CString(m_script.get());
    view->executeScript(scriptString);
    free(scriptString);
    return true;
}

bool BackForwardItem::invoke() const
{
    WKC::WKCWebFrame* mainFrame = DumpRenderTreeWKC::getMainFrame();
    WKC::WKCWebView* webView = mainFrame->webView();

    if (m_howFar == 1) {
        webView->goForward();
    } else if (m_howFar == -1) {
        webView->goBack();
    } else {
        webView->goBackOrForward(m_howFar);
    }
    return true;
}

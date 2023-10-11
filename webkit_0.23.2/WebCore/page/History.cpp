/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "History.h"

#include "BackForwardController.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "HistoryController.h"
#include "HistoryItem.h"
#include "MainFrame.h"
#include "Page.h"
#include "ScriptController.h"
#include "SecurityOrigin.h"
#include "SerializedScriptValue.h"
#include <wtf/CheckedArithmetic.h>
#include <wtf/MainThread.h>

namespace WebCore {

History::History(Frame* frame)
    : DOMWindowProperty(frame)
    , m_lastStateObjectRequested(nullptr)
{
}

unsigned History::length() const
{
    if (!m_frame)
        return 0;
    if (!m_frame->page())
        return 0;
    return m_frame->page()->backForward().count();
}

PassRefPtr<SerializedScriptValue> History::state()
{
    m_lastStateObjectRequested = stateInternal();
    return m_lastStateObjectRequested;
}

PassRefPtr<SerializedScriptValue> History::stateInternal() const
{
    if (!m_frame)
        return 0;

    if (HistoryItem* historyItem = m_frame->loader().history().currentItem())
        return historyItem->stateObject();

    return 0;
}

bool History::stateChanged() const
{
    return m_lastStateObjectRequested != stateInternal();
}

bool History::isSameAsCurrentState(SerializedScriptValue* state) const
{
    return state == stateInternal().get();
}

void History::back()
{
    go(-1);
}

void History::back(ScriptExecutionContext* context)
{
    go(context, -1);
}

void History::forward()
{
    go(1);
}

void History::forward(ScriptExecutionContext* context)
{
    go(context, 1);
}

void History::go(int distance)
{
    if (!m_frame)
        return;

    m_frame->navigationScheduler().scheduleHistoryNavigation(distance);
}

void History::go(ScriptExecutionContext* context, int distance)
{
    if (!m_frame)
        return;

    ASSERT(isMainThread());
    Document* activeDocument = downcast<Document>(context);
    if (!activeDocument)
        return;

    if (!activeDocument->canNavigate(m_frame))
        return;

    m_frame->navigationScheduler().scheduleHistoryNavigation(distance);
}

URL History::urlForState(const String& urlString)
{
    URL baseURL = m_frame->document()->baseURL();
    if (urlString.isEmpty())
        return baseURL;

    return URL(baseURL, urlString);
}

void History::stateObjectAdded(PassRefPtr<SerializedScriptValue> data, const String& title, const String& urlString, StateObjectType stateObjectType, ExceptionCode& ec)
{
    // Each unique main-frame document is only allowed to send 64mb of state object payload to the UI client/process.
    static uint32_t totalStateObjectPayloadLimit = 0x4000000;
    static double stateObjectTimeSpan = 30.0;
    static unsigned perStateObjectTimeSpanLimit = 100;

    if (!m_frame || !m_frame->page())
        return;

    URL fullURL = urlForState(urlString);
    if (!fullURL.isValid()) {
        ec = SECURITY_ERR;
        return;
    }

    const URL& documentURL = m_frame->document()->url();

    if (!protocolHostAndPortAreEqual(fullURL, documentURL) || fullURL.user() != documentURL.user() || fullURL.pass() != documentURL.pass()) {
        ec = SECURITY_ERR;
        return;
    }
    if (!m_frame->document()->securityOrigin().canRequest(fullURL) && (fullURL.path() != documentURL.path() || fullURL.query() != documentURL.query())) {
        ec = SECURITY_ERR;
        return;
    }

    Document* mainDocument = m_frame->page()->mainFrame().document();
    History* mainHistory = nullptr;
    if (mainDocument) {
        if (auto* mainDOMWindow = mainDocument->domWindow())
            mainHistory = mainDOMWindow->history();
    }

    if (!mainHistory)
        return;

    double currentTimestamp = currentTime();
    if (currentTimestamp - mainHistory->m_currentStateObjectTimeSpanStart > stateObjectTimeSpan) {
        mainHistory->m_currentStateObjectTimeSpanStart = currentTimestamp;
        mainHistory->m_currentStateObjectTimeSpanObjectsAdded = 0;
    }

    if (mainHistory->m_currentStateObjectTimeSpanObjectsAdded >= perStateObjectTimeSpanLimit) {
        ec = SECURITY_ERR;
        return;
    }

    Checked<unsigned> titleSize = title.length();
    titleSize *= 2;

    Checked<unsigned> urlSize = fullURL.string().length();
    urlSize *= 2;

    Checked<uint64_t> payloadSize = titleSize;
    payloadSize += urlSize;
    payloadSize += data ? data->data().size() : 0;

    Checked<uint64_t> newTotalUsage = mainHistory->m_totalStateObjectUsage;

    if (stateObjectType == StateObjectType::Replace)
        newTotalUsage -= m_mostRecentStateObjectUsage;
    newTotalUsage += payloadSize;

    if (newTotalUsage > totalStateObjectPayloadLimit) {
        ec = QUOTA_EXCEEDED_ERR;
        return;
    }

    m_mostRecentStateObjectUsage = payloadSize.unsafeGet();

    mainHistory->m_totalStateObjectUsage = newTotalUsage.unsafeGet();
    ++mainHistory->m_currentStateObjectTimeSpanObjectsAdded;

    if (!urlString.isEmpty())
        m_frame->document()->updateURLForPushOrReplaceState(fullURL);

    if (stateObjectType == StateObjectType::Push) {
        m_frame->loader().history().pushState(data, title, fullURL.string());
        m_frame->loader().client().dispatchDidPushStateWithinPage();
    } else if (stateObjectType == StateObjectType::Replace) {
        m_frame->loader().history().replaceState(data, title, fullURL.string());
        m_frame->loader().client().dispatchDidReplaceStateWithinPage();
    }
}

} // namespace WebCore

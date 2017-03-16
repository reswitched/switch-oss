/*
 * Copyright (C) 2007 Kevin Ollivier <kevino@theolliviers.com>. All rights reserved.
 * Copyright (c) 2010-2015 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "EventHandler.h"

#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "KeyboardEvent.h"
#include "MouseEventWithHitTestResults.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "RenderWidget.h"
#include "Scrollbar.h"

namespace WebCore {

#if ENABLE(DRAG_SUPPORT)
const double EventHandler::TextDragDelay = 0.0;
#endif

bool EventHandler::passMousePressEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe)
{
    (void)subframe->eventHandler().handleMousePressEvent(mev.event());
    return true;
}

bool EventHandler::passMouseMoveEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe, WebCore::HitTestResult* hittest)
{
#if ENABLE(DRAG_SUPPORT)
    if (m_mouseDownMayStartDrag && !m_mouseDownWasInSubframe)
        return false;
#endif
    (void)subframe->eventHandler().handleMouseMoveEvent(mev.event(), hittest);
    return true;
}

bool EventHandler::passMouseReleaseEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe)
{
    (void)subframe->eventHandler().handleMouseReleaseEvent(mev.event());
    return true;
}

bool EventHandler::passWidgetMouseDownEventToWidget(const MouseEventWithHitTestResults& event)
{
    // Figure out which view to send the event to.
    Node* targetNode = event.hitTestResult().innerNode();
    if (!targetNode || !targetNode->renderer() || !targetNode->renderer()->isWidget())
        return false;
    
    return passMouseDownEventToWidget(((RenderWidget *)targetNode->renderer())->widget());
}

bool EventHandler::passWidgetMouseDownEventToWidget(RenderWidget* renderWidget)
{
    return passMouseDownEventToWidget(renderWidget->widget());
}

bool EventHandler::passWheelEventToWidget(const PlatformWheelEvent& event, Widget& widget)
{
    if (!widget.isFrameView())
        return false;

    return static_cast<FrameView*>(&widget)->frame().eventHandler().handleWheelEvent(event);
}

bool EventHandler::passSubframeEventToSubframe(MouseEventWithHitTestResults&, Frame* subframe, HitTestResult*)
{
    notImplemented(); 
    return false; 
}

bool EventHandler::passMouseDownEventToWidget(Widget*)
{ 
    notImplemented();
    return false; 
}

void EventHandler::focusDocumentView()
{
    if (Page* page = m_frame.page())
        page->focusController().setFocusedFrame(PassRefPtr<Frame>(&m_frame));
}

bool EventHandler::eventActivatedView(const PlatformMouseEvent&) const
{
    // wx sends activate events separate from mouse events. 
    // We'll have to test the exact order of events, 
    // but for the moment just return false.
    return false;
}

unsigned EventHandler::accessKeyModifiers()
{
    return PlatformKeyboardEvent::AltKey;
}

bool EventHandler::tabsToAllFormControls(KeyboardEvent*) const
{
    // Ugh!: implement it!
    // 110622 ACCESS Co.,Ltd.
    notImplemented();
    return true;
}

}

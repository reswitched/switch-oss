/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2014-2015 Apple Inc. All rights reserved.
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
#include "EventHandler.h"

#include "AXObjectCache.h"
#include "BlockExceptions.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "DataTransfer.h"
#include "DictionaryLookup.h"
#include "DragController.h"
#include "Editor.h"
#include "EventNames.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "HTMLDocument.h"
#include "HTMLIFrameElement.h"
#include "KeyboardEvent.h"
#include "MainFrame.h"
#include "MouseEventWithHitTestResults.h"
#include "Page.h"
#include "Pasteboard.h"
#include "PlatformEventFactoryMac.h"
#include "Range.h"
#include "RenderLayer.h"
#include "RenderListBox.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "RuntimeApplicationChecks.h"
#include "ScrollAnimator.h"
#include "ScrollLatchingState.h"
#include "ScrollableArea.h"
#include "Scrollbar.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "WebCoreSystemInterface.h"
#include "WheelEventTestTrigger.h"
#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/ObjcRuntimeExtras.h>

#if ENABLE(MAC_GESTURE_EVENTS)
#import <WebKitAdditions/EventHandlerMacGesture.cpp>
#endif

namespace WebCore {

#if ENABLE(DRAG_SUPPORT)
const double EventHandler::TextDragDelay = 0.15;
#endif

static RetainPtr<NSEvent>& currentNSEventSlot()
{
    static NeverDestroyed<RetainPtr<NSEvent>> event;
    return event;
}

NSEvent *EventHandler::currentNSEvent()
{
    return currentNSEventSlot().get();
}

static RetainPtr<NSEvent>& correspondingPressureEventSlot()
{
    static NeverDestroyed<RetainPtr<NSEvent>> event;
    return event;
}

NSEvent *EventHandler::correspondingPressureEvent()
{
    return correspondingPressureEventSlot().get();
}

class CurrentEventScope {
     WTF_MAKE_NONCOPYABLE(CurrentEventScope);
public:
    CurrentEventScope(NSEvent *, NSEvent *correspondingPressureEvent);
    ~CurrentEventScope();

private:
    RetainPtr<NSEvent> m_savedCurrentEvent;
#ifndef NDEBUG
    RetainPtr<NSEvent> m_event;
#endif
    RetainPtr<NSEvent> m_savedPressureEvent;
    RetainPtr<NSEvent> m_correspondingPressureEvent;
};

inline CurrentEventScope::CurrentEventScope(NSEvent *event, NSEvent *correspondingPressureEvent)
    : m_savedCurrentEvent(currentNSEventSlot())
#ifndef NDEBUG
    , m_event(event)
#endif
    , m_savedPressureEvent(correspondingPressureEventSlot())
    , m_correspondingPressureEvent(correspondingPressureEvent)
{
    currentNSEventSlot() = event;
    correspondingPressureEventSlot() = correspondingPressureEvent;
}

inline CurrentEventScope::~CurrentEventScope()
{
    ASSERT(currentNSEventSlot() == m_event);
    currentNSEventSlot() = m_savedCurrentEvent;
    correspondingPressureEventSlot() = m_savedPressureEvent;
}

bool EventHandler::wheelEvent(NSEvent *event)
{
    Page* page = m_frame.page();
    if (!page)
        return false;

    CurrentEventScope scope(event, nil);
    return handleWheelEvent(PlatformEventFactory::createPlatformWheelEvent(event, page->chrome().platformPageClient()));
}

bool EventHandler::keyEvent(NSEvent *event)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    ASSERT([event type] == NSKeyDown || [event type] == NSKeyUp);

    CurrentEventScope scope(event, nil);
    return keyEvent(PlatformEventFactory::createPlatformKeyboardEvent(event));

    END_BLOCK_OBJC_EXCEPTIONS;

    return false;
}

void EventHandler::focusDocumentView()
{
    Page* page = m_frame.page();
    if (!page)
        return;

    if (FrameView* frameView = m_frame.view()) {
        if (NSView *documentView = frameView->documentView())
            page->chrome().focusNSView(documentView);
    }

    page->focusController().setFocusedFrame(&m_frame);
}

bool EventHandler::passWidgetMouseDownEventToWidget(const MouseEventWithHitTestResults& event)
{
    // Figure out which view to send the event to.
    auto* target = event.targetNode() ? event.targetNode()->renderer() : nullptr;
    if (!is<RenderWidget>(target))
        return false;

    // Double-click events don't exist in Cocoa. Since passWidgetMouseDownEventToWidget() will
    // just pass currentEvent down to the widget, we don't want to call it for events that
    // don't correspond to Cocoa events. The mousedown/ups will have already been passed on as
    // part of the pressed/released handling.
    return passMouseDownEventToWidget(downcast<RenderWidget>(*target).widget());
}

bool EventHandler::passWidgetMouseDownEventToWidget(RenderWidget* renderWidget)
{
    return passMouseDownEventToWidget(renderWidget->widget());
}

static bool lastEventIsMouseUp()
{
    // Many AppKit widgets run their own event loops and consume events while the mouse is down.
    // When they finish, currentEvent is the mouseUp that they exited on. We need to update
    // the WebCore state with this mouseUp, which we never saw. This method lets us detect
    // that state. Handling this was critical when we used AppKit widgets for form elements.
    // It's not clear in what cases this is helpful now -- it's possible it can be removed. 

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    NSEvent *currentEventAfterHandlingMouseDown = [NSApp currentEvent];
    return EventHandler::currentNSEvent() != currentEventAfterHandlingMouseDown
        && [currentEventAfterHandlingMouseDown type] == NSLeftMouseUp
        && [currentEventAfterHandlingMouseDown timestamp] >= [EventHandler::currentNSEvent() timestamp];
    END_BLOCK_OBJC_EXCEPTIONS;

    return false;
}

bool EventHandler::passMouseDownEventToWidget(Widget* pWidget)
{
    // FIXME: This function always returns true. It should be changed either to return
    // false in some cases or the return value should be removed.

    RefPtr<Widget> widget = pWidget;

    if (!widget) {
        LOG_ERROR("hit a RenderWidget without a corresponding Widget, means a frame is half-constructed");
        return true;
    }

    // In WebKit2 we will never have a native widget. Just return early and let the regular event handler machinery take care of
    // dispatching the event.
    if (!widget->platformWidget())
        return false;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSView *nodeView = widget->platformWidget();
    ASSERT([nodeView superview]);
    NSView *view = [nodeView hitTest:[[nodeView superview] convertPoint:[currentNSEvent() locationInWindow] fromView:nil]];
    if (!view) {
        // We probably hit the border of a RenderWidget
        return true;
    }

    Page* page = m_frame.page();
    if (!page)
        return true;

    if (page->chrome().client().firstResponder() != view) {
        // Normally [NSWindow sendEvent:] handles setting the first responder.
        // But in our case, the event was sent to the view representing the entire web page.
        if ([currentNSEvent() clickCount] <= 1 && [view acceptsFirstResponder] && [view needsPanelToBecomeKey])
            page->chrome().client().makeFirstResponder(view);
    }

    // We need to "defer loading" while tracking the mouse, because tearing down the
    // page while an AppKit control is tracking the mouse can cause a crash.

    // FIXME: In theory, WebCore now tolerates tear-down while tracking the
    // mouse. We should confirm that, and then remove the deferrsLoading
    // hack entirely.

    bool wasDeferringLoading = page->defersLoading();
    if (!wasDeferringLoading)
        page->setDefersLoading(true);

    ASSERT(!m_sendingEventToSubview);
    m_sendingEventToSubview = true;

    {
        WidgetHierarchyUpdatesSuspensionScope suspendWidgetHierarchyUpdates;
        [view mouseDown:currentNSEvent()];
    }

    m_sendingEventToSubview = false;
    
    if (!wasDeferringLoading)
        page->setDefersLoading(false);

    // Remember which view we sent the event to, so we can direct the release event properly.
    m_mouseDownView = view;
    m_mouseDownWasInSubframe = false;

    // Many AppKit widgets run their own event loops and consume events while the mouse is down.
    // When they finish, currentEvent is the mouseUp that they exited on.  We need to update
    // the EventHandler state with this mouseUp, which we never saw.
    // If this event isn't a mouseUp, we assume that the mouseUp will be coming later.  There
    // is a hole here if the widget consumes both the mouseUp and subsequent events.
    if (lastEventIsMouseUp())
        m_mousePressed = false;

    END_BLOCK_OBJC_EXCEPTIONS;

    return true;
}

// Note that this does the same kind of check as [target isDescendantOf:superview].
// There are two differences: This is a lot slower because it has to walk the whole
// tree, and this works in cases where the target has already been deallocated.
static bool findViewInSubviews(NSView *superview, NSView *target)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    NSEnumerator *e = [[superview subviews] objectEnumerator];
    NSView *subview;
    while ((subview = [e nextObject])) {
        if (subview == target || findViewInSubviews(subview, target)) {
            return true;
        }
    }
    END_BLOCK_OBJC_EXCEPTIONS;

    return false;
}

NSView *EventHandler::mouseDownViewIfStillGood()
{
    // Since we have no way of tracking the lifetime of m_mouseDownView, we have to assume that
    // it could be deallocated already. We search for it in our subview tree; if we don't find
    // it, we set it to nil.
    NSView *mouseDownView = m_mouseDownView;
    if (!mouseDownView) {
        return nil;
    }
    FrameView* topFrameView = m_frame.view();
    NSView *topView = topFrameView ? topFrameView->platformWidget() : nil;
    if (!topView || !findViewInSubviews(topView, mouseDownView)) {
        m_mouseDownView = nil;
        return nil;
    }
    return mouseDownView;
}

#if ENABLE(DRAG_SUPPORT)
bool EventHandler::eventLoopHandleMouseDragged(const MouseEventWithHitTestResults&)
{
    NSView *view = mouseDownViewIfStillGood();
    
    if (!view)
        return false;
    
    if (!m_mouseDownWasInSubframe) {
        ASSERT(!m_sendingEventToSubview);
        m_sendingEventToSubview = true;
        BEGIN_BLOCK_OBJC_EXCEPTIONS;
        [view mouseDragged:currentNSEvent()];
        END_BLOCK_OBJC_EXCEPTIONS;
        m_sendingEventToSubview = false;
    }
    
    return true;
}
#endif // ENABLE(DRAG_SUPPORT)

bool EventHandler::eventLoopHandleMouseUp(const MouseEventWithHitTestResults&)
{
    NSView *view = mouseDownViewIfStillGood();
    if (!view)
        return false;

    if (!m_mouseDownWasInSubframe) {
        ASSERT(!m_sendingEventToSubview);
        m_sendingEventToSubview = true;
        BEGIN_BLOCK_OBJC_EXCEPTIONS;
        [view mouseUp:currentNSEvent()];
        END_BLOCK_OBJC_EXCEPTIONS;
        m_sendingEventToSubview = false;
    }
 
    return true;
}
    
bool EventHandler::passSubframeEventToSubframe(MouseEventWithHitTestResults& event, Frame* subframe, HitTestResult* hoveredNode)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    switch ([currentNSEvent() type]) {
        case NSLeftMouseDragged:
        case NSOtherMouseDragged:
        case NSRightMouseDragged:
            // This check is bogus and results in <rdar://6813830>, but removing it breaks a number of
            // layout tests.
            if (!m_mouseDownWasInSubframe)
                return false;
#if ENABLE(DRAG_SUPPORT)
            if (subframe->page()->dragController().didInitiateDrag())
                return false;
#endif
        case NSMouseMoved:
            // Since we're passing in currentNSEvent() here, we can call
            // handleMouseMoveEvent() directly, since the save/restore of
            // currentNSEvent() that mouseMoved() does would have no effect.
            ASSERT(!m_sendingEventToSubview);
            m_sendingEventToSubview = true;
            subframe->eventHandler().handleMouseMoveEvent(currentPlatformMouseEvent(), hoveredNode);
            m_sendingEventToSubview = false;
            return true;
        
        case NSLeftMouseDown: {
            Node* node = event.targetNode();
            if (!node)
                return false;
            auto* renderer = node->renderer();
            if (!is<RenderWidget>(renderer))
                return false;
            Widget* widget = downcast<RenderWidget>(*renderer).widget();
            if (!widget || !widget->isFrameView())
                return false;
            if (!passWidgetMouseDownEventToWidget(downcast<RenderWidget>(renderer)))
                return false;
            m_mouseDownWasInSubframe = true;
            return true;
        }
        case NSLeftMouseUp: {
            if (!m_mouseDownWasInSubframe)
                return false;
            ASSERT(!m_sendingEventToSubview);
            m_sendingEventToSubview = true;
            subframe->eventHandler().handleMouseReleaseEvent(currentPlatformMouseEvent());
            m_sendingEventToSubview = false;
            return true;
        }
        default:
            return false;
    }
    END_BLOCK_OBJC_EXCEPTIONS;

    return false;
}

static IMP originalNSScrollViewScrollWheel;
static bool _nsScrollViewScrollWheelShouldRetainSelf;
static void selfRetainingNSScrollViewScrollWheel(NSScrollView *, SEL, NSEvent *);

static bool nsScrollViewScrollWheelShouldRetainSelf()
{
    ASSERT(isMainThread());

    return _nsScrollViewScrollWheelShouldRetainSelf;
}

static void setNSScrollViewScrollWheelShouldRetainSelf(bool shouldRetain)
{
    ASSERT(isMainThread());

    if (!originalNSScrollViewScrollWheel) {
        Method method = class_getInstanceMethod(objc_getRequiredClass("NSScrollView"), @selector(scrollWheel:));
        originalNSScrollViewScrollWheel = method_setImplementation(method, reinterpret_cast<IMP>(selfRetainingNSScrollViewScrollWheel));
    }

    _nsScrollViewScrollWheelShouldRetainSelf = shouldRetain;
}

static void selfRetainingNSScrollViewScrollWheel(NSScrollView *self, SEL selector, NSEvent *event)
{
    bool shouldRetainSelf = isMainThread() && nsScrollViewScrollWheelShouldRetainSelf();

    if (shouldRetainSelf)
        [self retain];
    wtfCallIMP<void>(originalNSScrollViewScrollWheel, self, selector, event);
    if (shouldRetainSelf)
        [self release];
}

bool EventHandler::passWheelEventToWidget(const PlatformWheelEvent& wheelEvent, Widget& widget)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSView* nodeView = widget.platformWidget();
    if (!nodeView) {
        // WebKit2 code path.
        if (!is<FrameView>(widget))
            return false;

        return downcast<FrameView>(widget).frame().eventHandler().handleWheelEvent(wheelEvent);
    }

    if ([currentNSEvent() type] != NSScrollWheel || m_sendingEventToSubview) 
        return false;

    ASSERT(nodeView);
    ASSERT([nodeView superview]);
    NSView *view = [nodeView hitTest:[[nodeView superview] convertPoint:[currentNSEvent() locationInWindow] fromView:nil]];
    if (!view) {
        // We probably hit the border of a RenderWidget
        return false;
    }

    ASSERT(!m_sendingEventToSubview);
    m_sendingEventToSubview = true;
    // Work around <rdar://problem/6806810> which can cause -[NSScrollView scrollWheel:] to
    // crash if the NSScrollView is released during timer or network callback dispatch
    // in the nested tracking runloop that -[NSScrollView scrollWheel:] runs.
    setNSScrollViewScrollWheelShouldRetainSelf(true);
    [view scrollWheel:currentNSEvent()];
    setNSScrollViewScrollWheelShouldRetainSelf(false);
    m_sendingEventToSubview = false;
    return true;

    END_BLOCK_OBJC_EXCEPTIONS;
    return false;
}

void EventHandler::mouseDown(NSEvent *event, NSEvent *correspondingPressureEvent)
{
    FrameView* v = m_frame.view();
    if (!v || m_sendingEventToSubview)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    
    m_mouseDownView = nil;
    
    CurrentEventScope scope(event, correspondingPressureEvent);

    handleMousePressEvent(currentPlatformMouseEvent());

    END_BLOCK_OBJC_EXCEPTIONS;
}

void EventHandler::mouseDragged(NSEvent *event, NSEvent *correspondingPressureEvent)
{
    FrameView* v = m_frame.view();
    if (!v || m_sendingEventToSubview)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    CurrentEventScope scope(event, correspondingPressureEvent);
    handleMouseMoveEvent(currentPlatformMouseEvent());

    END_BLOCK_OBJC_EXCEPTIONS;
}

void EventHandler::mouseUp(NSEvent *event, NSEvent *correspondingPressureEvent)
{
    FrameView* v = m_frame.view();
    if (!v || m_sendingEventToSubview)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    CurrentEventScope scope(event, correspondingPressureEvent);

    // Our behavior here is a little different that Qt. Qt always sends
    // a mouse release event, even for a double click. To correct problems
    // in khtml's DOM click event handling we do not send a release here
    // for a double click. Instead we send that event from FrameView's
    // handleMouseDoubleClickEvent. Note also that the third click of
    // a triple click is treated as a single click, but the fourth is then
    // treated as another double click. Hence the "% 2" below.
    int clickCount = [event clickCount];
    if (clickCount > 0 && clickCount % 2 == 0)
        handleMouseDoubleClickEvent(currentPlatformMouseEvent());
    else
        handleMouseReleaseEvent(currentPlatformMouseEvent());
    
    m_mouseDownView = nil;

    END_BLOCK_OBJC_EXCEPTIONS;
}

/*
 A hack for the benefit of AK's PopUpButton, which uses the Carbon menu manager, which thus
 eats all subsequent events after it is starts its modal tracking loop.  After the interaction
 is done, this routine is used to fix things up.  When a mouse down started us tracking in
 the widget, we post a fake mouse up to balance the mouse down we started with. When a 
 key down started us tracking in the widget, we post a fake key up to balance things out.
 In addition, we post a fake mouseMoved to get the cursor in sync with whatever we happen to 
 be over after the tracking is done.
 */
void EventHandler::sendFakeEventsAfterWidgetTracking(NSEvent *initiatingEvent)
{
    FrameView* view = m_frame.view();
    if (!view)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    m_sendingEventToSubview = false;
    int eventType = [initiatingEvent type];
    if (eventType == NSLeftMouseDown || eventType == NSKeyDown) {
        NSEvent *fakeEvent = nil;
        if (eventType == NSLeftMouseDown) {
            fakeEvent = [NSEvent mouseEventWithType:NSLeftMouseUp
                                           location:[initiatingEvent locationInWindow]
                                      modifierFlags:[initiatingEvent modifierFlags]
                                          timestamp:[initiatingEvent timestamp]
                                       windowNumber:[initiatingEvent windowNumber]
                                            context:[initiatingEvent context]
                                        eventNumber:[initiatingEvent eventNumber]
                                         clickCount:[initiatingEvent clickCount]
                                           pressure:[initiatingEvent pressure]];
        
            [NSApp postEvent:fakeEvent atStart:YES];
        } else { // eventType == NSKeyDown
            fakeEvent = [NSEvent keyEventWithType:NSKeyUp
                                         location:[initiatingEvent locationInWindow]
                                    modifierFlags:[initiatingEvent modifierFlags]
                                        timestamp:[initiatingEvent timestamp]
                                     windowNumber:[initiatingEvent windowNumber]
                                          context:[initiatingEvent context]
                                       characters:[initiatingEvent characters] 
                      charactersIgnoringModifiers:[initiatingEvent charactersIgnoringModifiers] 
                                        isARepeat:[initiatingEvent isARepeat] 
                                          keyCode:[initiatingEvent keyCode]];
            [NSApp postEvent:fakeEvent atStart:YES];
        }
        // FIXME: We should really get the current modifierFlags here, but there's no way to poll
        // them in Cocoa, and because the event stream was stolen by the Carbon menu code we have
        // no up-to-date cache of them anywhere.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        fakeEvent = [NSEvent mouseEventWithType:NSMouseMoved
                                       location:[[view->platformWidget() window] convertScreenToBase:[NSEvent mouseLocation]]
                                  modifierFlags:[initiatingEvent modifierFlags]
                                      timestamp:[initiatingEvent timestamp]
                                   windowNumber:[initiatingEvent windowNumber]
                                        context:[initiatingEvent context]
                                    eventNumber:0
                                     clickCount:0
                                       pressure:0];
#pragma clang diagnostic pop
        [NSApp postEvent:fakeEvent atStart:YES];
    }
    
    END_BLOCK_OBJC_EXCEPTIONS;
}

void EventHandler::mouseMoved(NSEvent *event, NSEvent* correspondingPressureEvent)
{
    // Reject a mouse moved if the button is down - screws up tracking during autoscroll
    // These happen because WebKit sometimes has to fake up moved events.
    if (!m_frame.view() || m_mousePressed || m_sendingEventToSubview)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    CurrentEventScope scope(event, correspondingPressureEvent);
    mouseMoved(currentPlatformMouseEvent());
    END_BLOCK_OBJC_EXCEPTIONS;
}

void EventHandler::pressureChange(NSEvent *event, NSEvent* correspondingPressureEvent)
{
    if (!m_frame.view() || m_sendingEventToSubview)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    CurrentEventScope scope(event, correspondingPressureEvent);
    handleMouseForceEvent(currentPlatformMouseEvent());
    END_BLOCK_OBJC_EXCEPTIONS;
}

void EventHandler::passMouseMovedEventToScrollbars(NSEvent *event, NSEvent* correspondingPressureEvent)
{
    // Reject a mouse moved if the button is down - screws up tracking during autoscroll
    // These happen because WebKit sometimes has to fake up moved events.
    if (!m_frame.view() || m_mousePressed || m_sendingEventToSubview)
        return;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    CurrentEventScope scope(event, correspondingPressureEvent);
    passMouseMovedEventToScrollbars(currentPlatformMouseEvent());
    END_BLOCK_OBJC_EXCEPTIONS;
}

static bool frameHasPlatformWidget(const Frame& frame)
{
    if (FrameView* frameView = frame.view()) {
        if (frameView->platformWidget())
            return true;
    }

    return false;
}

bool EventHandler::passMousePressEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe)
{
    // WebKit1 code path.
    if (frameHasPlatformWidget(m_frame))
        return passSubframeEventToSubframe(mev, subframe);

    // WebKit2 code path.
    subframe->eventHandler().handleMousePressEvent(mev.event());
    return true;
}

bool EventHandler::passMouseMoveEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe, HitTestResult* hoveredNode)
{
    // WebKit1 code path.
    if (frameHasPlatformWidget(m_frame))
        return passSubframeEventToSubframe(mev, subframe, hoveredNode);

#if ENABLE(DRAG_SUPPORT)
    // WebKit2 code path.
    if (m_mouseDownMayStartDrag && !m_mouseDownWasInSubframe)
        return false;
#endif

    subframe->eventHandler().handleMouseMoveEvent(mev.event(), hoveredNode);
    return true;
}

bool EventHandler::passMouseReleaseEventToSubframe(MouseEventWithHitTestResults& mev, Frame* subframe)
{
    // WebKit1 code path.
    if (frameHasPlatformWidget(m_frame))
        return passSubframeEventToSubframe(mev, subframe);

    // WebKit2 code path.
    subframe->eventHandler().handleMouseReleaseEvent(mev.event());
    return true;
}

PlatformMouseEvent EventHandler::currentPlatformMouseEvent() const
{
    NSView *windowView = nil;
    if (Page* page = m_frame.page())
        windowView = page->chrome().platformPageClient();
    return PlatformEventFactory::createPlatformMouseEvent(currentNSEvent(), correspondingPressureEvent(), windowView);
}

bool EventHandler::eventActivatedView(const PlatformMouseEvent& event) const
{
    return m_activationEventNumber == event.eventNumber();
}

#if ENABLE(DRAG_SUPPORT)

PassRefPtr<DataTransfer> EventHandler::createDraggingDataTransfer() const
{
    // Must be done before ondragstart adds types and data to the pboard,
    // also done for security, as it erases data from the last drag.
    auto pasteboard = std::make_unique<Pasteboard>(NSDragPboard);
    pasteboard->clear();
    return DataTransfer::createForDragAndDrop();
}

#endif

bool EventHandler::tabsToAllFormControls(KeyboardEvent* event) const
{
    Page* page = m_frame.page();
    if (!page)
        return false;

    KeyboardUIMode keyboardUIMode = page->chrome().client().keyboardUIMode();
    bool handlingOptionTab = isKeyboardOptionTab(event);

    // If tab-to-links is off, option-tab always highlights all controls
    if ((keyboardUIMode & KeyboardAccessTabsToLinks) == 0 && handlingOptionTab)
        return true;
    
    // If system preferences say to include all controls, we always include all controls
    if (keyboardUIMode & KeyboardAccessFull)
        return true;
    
    // Otherwise tab-to-links includes all controls, unless the sense is flipped via option-tab.
    if (keyboardUIMode & KeyboardAccessTabsToLinks)
        return !handlingOptionTab;
    
    return handlingOptionTab;
}

bool EventHandler::needsKeyboardEventDisambiguationQuirks() const
{
#if ENABLE(DASHBOARD_SUPPORT)
    if (m_frame.settings().usesDashboardBackwardCompatibilityMode())
        return true;
#endif
        
    if (m_frame.settings().needsKeyboardEventDisambiguationQuirks())
        return true;

    return false;
}

unsigned EventHandler::accessKeyModifiers()
{
    // Control+Option key combinations are usually unused on Mac OS X, but not when VoiceOver is enabled.
    // So, we use Control in this case, even though it conflicts with Emacs-style key bindings.
    // See <https://bugs.webkit.org/show_bug.cgi?id=21107> for more detail.
    if (AXObjectCache::accessibilityEnhancedUserInterfaceEnabled())
        return PlatformEvent::CtrlKey;

    return PlatformEvent::CtrlKey | PlatformEvent::AltKey;
}

static ScrollableArea* scrollableAreaForBox(RenderBox& box)
{
    if (is<RenderListBox>(box))
        return downcast<RenderListBox>(&box);

    return box.layer();
}
    
static ContainerNode* findEnclosingScrollableContainer(ContainerNode* node, float deltaX, float deltaY)
{
    // Find the first node with a valid scrollable area starting with the current
    // node and traversing its parents (or shadow hosts).
    for (ContainerNode* candidate = node; candidate; candidate = candidate->parentOrShadowHostNode()) {
        if (is<HTMLIFrameElement>(candidate))
            continue;

        if (is<HTMLHtmlElement>(candidate) || is<HTMLDocument>(candidate))
            return nullptr;

        RenderBox* box = candidate->renderBox();
        if (box && box->canBeScrolledAndHasScrollableArea()) {
            if (ScrollableArea* scrollableArea = scrollableAreaForBox(*box)) {
                if (((deltaY > 0) && !scrollableArea->scrolledToTop()) || ((deltaY < 0) && !scrollableArea->scrolledToBottom())
                    || ((deltaX > 0) && !scrollableArea->scrolledToRight()) || ((deltaX < 0) && !scrollableArea->scrolledToLeft())) {
                    return candidate;
                }
            }
        }
    }
    
    return nullptr;
}

static bool deltaIsPredominantlyVertical(float deltaX, float deltaY)
{
    return std::abs(deltaY) > std::abs(deltaX);
}
    
static bool scrolledToEdgeInDominantDirection(const ContainerNode& container, const ScrollableArea& area, float deltaX, float deltaY)
{
    if (!container.renderer())
        return true;

    const RenderStyle& style = container.renderer()->style();

    if (!deltaIsPredominantlyVertical(deltaX, deltaY) && deltaX) {
        if (style.overflowX() == OHIDDEN)
            return true;

        if (deltaX < 0)
            return area.scrolledToRight();
        
        return area.scrolledToLeft();
    }

    if (style.overflowY() == OHIDDEN)
        return true;

    if (deltaY < 0)
        return area.scrolledToBottom();
    
    return area.scrolledToTop();
}

static Widget* widgetForEventTarget(Element* eventTarget)
{
    if (!eventTarget)
        return nullptr;
    
    auto* target = eventTarget->renderer();
    if (!is<RenderWidget>(target))
        return nullptr;
    
    return downcast<RenderWidget>(*target).widget();
}

static ScrollView* scrollViewForEventTarget(Element* eventTarget)
{
    Widget* widget = widgetForEventTarget(eventTarget);
    if (!widget)
        return nullptr;
    
    if (!widget->isScrollView())
        return nullptr;
    
    return reinterpret_cast<ScrollView*>(widget);
}
    
static bool eventTargetIsPlatformWidget(Element* eventTarget)
{
    Widget* widget = widgetForEventTarget(eventTarget);
    if (!widget)
        return false;
    
    return widget->platformWidget();
}

static bool latchingIsLockedToPlatformFrame(const Frame& frame)
{
    ScrollLatchingState* latchedState = frame.mainFrame().latchingState();
    if (!latchedState)
        return false;

    if (frameHasPlatformWidget(frame) && &frame != latchedState->frame())
        return true;

    return false;
}

static bool latchingIsLockedToAncestorOfThisFrame(const Frame& frame)
{
    ScrollLatchingState* latchedState = frame.mainFrame().latchingState();
    if (!latchedState || !latchedState->frame())
        return false;

    if (&frame == latchedState->frame())
        return false;

    for (Frame* ancestor = frame.tree().parent(); ancestor; ancestor = ancestor->tree().parent()) {
        if (ancestor == latchedState->frame())
            return true;
    }
    
    return false;
}

static ScrollableArea* scrollableAreaForContainerNode(ContainerNode& container)
{
    ScrollableArea* scrollableArea = nullptr;

    if (RenderBox* box = container.renderBox())
        scrollableArea = scrollableAreaForBox(*box);

    return scrollableArea;
}

static bool latchedToFrameOrBody(ContainerNode& container)
{
    // FIXME(106133): We might need to add or switch to is<HTMLDocumentElement> when this bug is fixed.
    return is<HTMLFrameSetElement>(container) || is<HTMLBodyElement>(container);
}

void EventHandler::platformPrepareForWheelEvents(const PlatformWheelEvent& wheelEvent, const HitTestResult& result, RefPtr<Element>& wheelEventTarget, RefPtr<ContainerNode>& scrollableContainer, ScrollableArea*& scrollableArea, bool& isOverWidget)
{
    FrameView* view = m_frame.view();

    scrollableContainer = nullptr;
    scrollableArea = nullptr;
    if (!view)
        scrollableContainer = wheelEventTarget;
    else {
        if (eventTargetIsPlatformWidget(wheelEventTarget.get())) {
            scrollableContainer = wheelEventTarget;
            scrollableArea = scrollViewForEventTarget(wheelEventTarget.get());
        } else {
            scrollableContainer = findEnclosingScrollableContainer(wheelEventTarget.get(), wheelEvent.deltaX(), wheelEvent.deltaY());
            if (scrollableContainer)
                scrollableArea = scrollableAreaForContainerNode(*scrollableContainer);
            else {
                scrollableContainer = view->frame().document()->bodyOrFrameset();
                scrollableArea = view;
            }
        }
    }
    
    Page* page = m_frame.page();
    if (scrollableArea && page && page->expectsWheelEventTriggers())
        scrollableArea->scrollAnimator().setWheelEventTestTrigger(page->testTrigger());

    ScrollLatchingState* latchingState = m_frame.mainFrame().latchingState();
    if (wheelEvent.shouldConsiderLatching()) {
        if (scrollableContainer && scrollableArea) {
            bool startingAtScrollLimit = scrolledToEdgeInDominantDirection(*scrollableContainer, *scrollableArea, wheelEvent.deltaX(), wheelEvent.deltaY());
            if (!startingAtScrollLimit) {
                m_frame.mainFrame().pushNewLatchingState();
                latchingState = m_frame.mainFrame().latchingState();
                latchingState->setStartedGestureAtScrollLimit(false);
                latchingState->setWheelEventElement(wheelEventTarget);
                latchingState->setFrame(&m_frame);
                // FIXME: What prevents us from deleting this scrollable container while still holding a pointer to it?
                latchingState->setScrollableContainer(scrollableContainer);
                latchingState->setWidgetIsLatched(result.isOverWidget());
                isOverWidget = latchingState->widgetIsLatched();
                m_frame.mainFrame().wheelEventDeltaTracker()->beginTrackingDeltas();
            }
        }
    } else if (wheelEvent.shouldResetLatching())
        clearLatchedState();

    if (!wheelEvent.shouldResetLatching() && latchingState && latchingState->wheelEventElement()) {
        if (latchingIsLockedToPlatformFrame(m_frame))
            return;

        if (latchingIsLockedToAncestorOfThisFrame(m_frame))
            return;

        wheelEventTarget = latchingState->wheelEventElement();
        isOverWidget = latchingState->widgetIsLatched();
        scrollableContainer = latchingState->scrollableContainer();

        if (scrollableContainer) {
            if (!latchedToFrameOrBody(*scrollableContainer) && !latchingState->widgetIsLatched())
                scrollableArea = scrollableAreaForContainerNode(*scrollableContainer);
        }
    }
}

void EventHandler::platformRecordWheelEvent(const PlatformWheelEvent& wheelEvent)
{
    switch (wheelEvent.phase()) {
        case PlatformWheelEventPhaseBegan:
            m_frame.mainFrame().wheelEventDeltaTracker()->beginTrackingDeltas();
            break;
        case PlatformWheelEventPhaseEnded:
            m_frame.mainFrame().wheelEventDeltaTracker()->endTrackingDeltas();
            break;
        default:
            break;
    }

    m_frame.mainFrame().wheelEventDeltaTracker()->recordWheelEventDelta(wheelEvent);
}

static FrameView* frameViewForLatchingState(Frame& frame, ScrollLatchingState* latchingState)
{
    if (latchingIsLockedToPlatformFrame(frame))
        return frame.view();

    return latchingState->frame() ? latchingState->frame()->view() : frame.view();
}

bool EventHandler::platformCompleteWheelEvent(const PlatformWheelEvent& wheelEvent, ContainerNode* scrollableContainer, ScrollableArea* scrollableArea)
{
    // We do another check on the frame view because the event handler can run JS which results in the frame getting destroyed.
    ASSERT(m_frame.view());
    FrameView* view = m_frame.view();

    ScrollLatchingState* latchingState = m_frame.mainFrame().latchingState();
    if (wheelEvent.useLatchedEventElement() && !latchingIsLockedToAncestorOfThisFrame(m_frame) && latchingState && latchingState->scrollableContainer()) {

        m_isHandlingWheelEvent = false;

        // WebKit2 code path
        if (!frameHasPlatformWidget(m_frame) && !latchingState->startedGestureAtScrollLimit() && scrollableContainer == latchingState->scrollableContainer() && scrollableArea && view != scrollableArea) {
            // If we did not start at the scroll limit, do not pass the event on to be handled by enclosing scrollable regions.
            return true;
        }

        if (!latchingState->startedGestureAtScrollLimit())
            view = frameViewForLatchingState(m_frame, latchingState);

        ASSERT(view);

        bool didHandleWheelEvent = view->wheelEvent(wheelEvent);
        if (scrollableContainer == latchingState->scrollableContainer()) {
            // If we are just starting a scroll event, and have nowhere left to scroll, allow
            // the enclosing frame to handle the scroll.
            didHandleWheelEvent = !latchingState->startedGestureAtScrollLimit();
        }

        // If the platform widget is handling the event, we always want to return false.
        if (scrollableArea == view && view->platformWidget())
            didHandleWheelEvent = false;
        
        return didHandleWheelEvent;
    }
    
    bool didHandleEvent = view->wheelEvent(wheelEvent);
    m_isHandlingWheelEvent = false;
    return didHandleEvent;
}

bool EventHandler::platformCompletePlatformWidgetWheelEvent(const PlatformWheelEvent& wheelEvent, const Widget& widget, ContainerNode* scrollableContainer)
{
    // WebKit1: Prevent multiple copies of the scrollWheel event from being sent to the NSScrollView widget.
    if (frameHasPlatformWidget(m_frame) && widget.isFrameView())
        return true;

    ScrollLatchingState* latchingState = m_frame.mainFrame().latchingState();
    if (!latchingState)
        return false;

    if (wheelEvent.useLatchedEventElement() && latchingState->scrollableContainer() && scrollableContainer == latchingState->scrollableContainer())
        return !latchingState->startedGestureAtScrollLimit();

    return false;
}

void EventHandler::platformNotifyIfEndGesture(const PlatformWheelEvent& wheelEvent, ScrollableArea* scrollableArea)
{
    if (!scrollableArea)
        return;

    // Special case handling for ending wheel gesture to activate snap animation:
    if (wheelEvent.phase() != PlatformWheelEventPhaseEnded && wheelEvent.momentumPhase() != PlatformWheelEventPhaseEnded)
        return;

#if ENABLE(CSS_SCROLL_SNAP)
    if (ScrollAnimator* scrollAnimator = scrollableArea->existingScrollAnimator())
        scrollAnimator->processWheelEventForScrollSnap(wheelEvent);
#endif
}

VisibleSelection EventHandler::selectClosestWordFromHitTestResultBasedOnLookup(const HitTestResult& result)
{
    if (!m_frame.editor().behavior().shouldSelectBasedOnDictionaryLookup())
        return VisibleSelection();

    NSDictionary *options = nil;
    if (RefPtr<Range> range = rangeForDictionaryLookupAtHitTestResult(result, &options))
        return VisibleSelection(*range);

    return VisibleSelection();
}

}

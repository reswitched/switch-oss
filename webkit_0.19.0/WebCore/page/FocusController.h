/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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

#ifndef FocusController_h
#define FocusController_h

#include "FocusDirection.h"
#include "LayoutRect.h"
#include "Timer.h"
#include "ViewState.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>

namespace WebCore {

struct FocusCandidate;
class ContainerNode;
class Document;
class Element;
class Frame;
class HTMLFrameOwnerElement;
class IntRect;
class KeyboardEvent;
class Node;
class Page;
class TreeScope;

class FocusNavigationScope {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    ContainerNode* rootNode() const;
    Element* owner() const;
    WEBCORE_EXPORT static FocusNavigationScope focusNavigationScopeOf(Node*);
    static FocusNavigationScope focusNavigationScopeOwnedByShadowHost(Node*);
    static FocusNavigationScope focusNavigationScopeOwnedByIFrame(HTMLFrameOwnerElement*);

private:
    explicit FocusNavigationScope(TreeScope*);
    TreeScope* m_rootTreeScope;
};

class FocusController {
    WTF_MAKE_NONCOPYABLE(FocusController); WTF_MAKE_FAST_ALLOCATED;
public:
    explicit FocusController(Page&, ViewState::Flags);

    WEBCORE_EXPORT void setFocusedFrame(PassRefPtr<Frame>);
    Frame* focusedFrame() const { return m_focusedFrame.get(); }
    WEBCORE_EXPORT Frame& focusedOrMainFrame() const;

    WEBCORE_EXPORT bool setInitialFocus(FocusDirection, KeyboardEvent*);
    bool advanceFocus(FocusDirection, KeyboardEvent*, bool initialFocus = false);

    WEBCORE_EXPORT bool setFocusedElement(Element*, PassRefPtr<Frame>, FocusDirection = FocusDirectionNone);

    void setViewState(ViewState::Flags);

    WEBCORE_EXPORT void setActive(bool);
    bool isActive() const { return m_viewState & ViewState::WindowIsActive; }

    WEBCORE_EXPORT void setFocused(bool);
    bool isFocused() const { return m_viewState & ViewState::IsFocused; }

    bool contentIsVisible() const { return m_viewState & ViewState::IsVisible; }

    // These methods are used in WebCore/bindings/objc/DOM.mm.
    WEBCORE_EXPORT Element* nextFocusableElement(FocusNavigationScope, Node* start, KeyboardEvent*);
    WEBCORE_EXPORT Element* previousFocusableElement(FocusNavigationScope, Node* start, KeyboardEvent*);

    void setFocusedElementNeedsRepaint();
    double timeSinceFocusWasSet() const;

#if PLATFORM(WKC)
    bool isClickableElement(Element* element);
    bool setFocusedNode(Node *node);
    Element* findNextFocusableElement(const FocusDirection& direction, const IntRect* scope = 0, Element* base = 0);
    Element* findNearestFocusableElementFromPoint(const IntPoint& point, const IntRect* scope = 0);
    // get child element preferentially
    Element* findNearestClickableElementFromPoint(const IntPoint& point, const IntRect* scope = 0);
    LayoutPoint getLastEntryPoint() const { return m_lastEntryPoint; }
#endif

private:
#if PLATFORM(WKC)
    void findFocusableNodeInDirection(Node* container, Node* startingElement, const LayoutRect& startingRect, FocusDirection direction, KeyboardEvent* event, FocusCandidate& closest, const LayoutRect* scope);
    Element* findNearestFocusableElementFromPoint(Element* start, const LayoutPoint& point, const LayoutRect& scope);
    // get child element preferentially
    Element* findNearestClickableElementFromPoint(const Element* start, const LayoutPoint& point, const LayoutRect& scope);
#endif
    void setActiveInternal(bool);
    void setFocusedInternal(bool);
    void setIsVisibleAndActiveInternal(bool);

    bool advanceFocusDirectionally(FocusDirection, KeyboardEvent*);
    bool advanceFocusInDocumentOrder(FocusDirection, KeyboardEvent*, bool initialFocus);

    Element* findFocusableElementAcrossFocusScope(FocusDirection, FocusNavigationScope startScope, Node* start, KeyboardEvent*);
    Element* findFocusableElementRecursively(FocusDirection, FocusNavigationScope, Node* start, KeyboardEvent*);
    Element* findFocusableElementDescendingDownIntoFrameDocument(FocusDirection, Element*, KeyboardEvent*);

    // Searches through the given tree scope, starting from start node, for the next/previous selectable element that comes after/before start node.
    // The order followed is as specified in section 17.11.1 of the HTML4 spec, which is elements with tab indexes
    // first (from lowest to highest), and then elements without tab indexes (in document order).
    //
    // @param start The node from which to start searching. The node after this will be focused. May be null.
    //
    // @return The focus node that comes after/before start node.
    //
    // See http://www.w3.org/TR/html4/interact/forms.html#h-17.11.1
    Element* findFocusableElement(FocusDirection, FocusNavigationScope, Node* start, KeyboardEvent*);

    Element* findElementWithExactTabIndex(Node* start, int tabIndex, KeyboardEvent*, FocusDirection);

    bool advanceFocusDirectionallyInContainer(Node* container, const LayoutRect& startingRect, FocusDirection, KeyboardEvent*);
    void findFocusCandidateInContainer(Node& container, const LayoutRect& startingRect, FocusDirection, KeyboardEvent*, FocusCandidate& closest);

    void focusRepaintTimerFired();

    Page& m_page;
    RefPtr<Frame> m_focusedFrame;
    bool m_isChangingFocusedFrame;
    ViewState::Flags m_viewState;

    Timer m_focusRepaintTimer;
    double m_focusSetTime;

#if PLATFORM(WKC)
    LayoutRect m_lastExitRect;
    LayoutRect m_lastEntryRect;
    LayoutPoint m_lastEntryPoint;
    FocusDirection m_lastDirection;
#endif
};

} // namespace WebCore
    
#endif // FocusController_h
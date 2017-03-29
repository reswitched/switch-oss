/*
 * Copyright (C) 2006, 2007, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2012-2017 ACCESS CO., LTD. All rights reserved.
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
#include "FocusController.h"

#include "AXObjectCache.h"
#include "Chrome.h"
#include "Document.h"
#include "Editor.h"
#include "EditorClient.h"
#include "Element.h"
#include "ElementTraversal.h"
#include "Event.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FrameSelection.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLAreaElement.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLPlugInElement.h"
#include "HTMLTextAreaElement.h"
#include "HitTestResult.h"
#include "KeyboardEvent.h"
#include "MainFrame.h"
#include "NodeRenderingTraversal.h"
#include "Page.h"
#include "Range.h"
#include "RenderWidget.h"
#include "ScrollAnimator.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "SpatialNavigation.h"
#include "Widget.h"
#include "htmlediting.h" // For firstPositionInOrBeforeNode
#include <limits>
#include <wtf/CurrentTime.h>
#include <wtf/Ref.h>

#if PLATFORM(WKC)
#include "ElementIterator.h"
#include "HTMLLabelElement.h"
#endif

namespace WebCore {

using namespace HTMLNames;

FocusNavigationScope::FocusNavigationScope(TreeScope* treeScope)
    : m_rootTreeScope(treeScope)
{
    ASSERT(treeScope);
}

ContainerNode* FocusNavigationScope::rootNode() const
{
    return &m_rootTreeScope->rootNode();
}

Element* FocusNavigationScope::owner() const
{
    ContainerNode* root = rootNode();
    if (is<ShadowRoot>(*root))
        return downcast<ShadowRoot>(*root).hostElement();
    if (Frame* frame = root->document().frame())
        return frame->ownerElement();
    return nullptr;
}

FocusNavigationScope FocusNavigationScope::focusNavigationScopeOf(Node* node)
{
    ASSERT(node);
    Node* root = node;
    for (Node* n = node; n; n = NodeRenderingTraversal::parentInScope(n))
        root = n;
    // The result is not always a ShadowRoot nor a DocumentNode since
    // a starting node is in an orphaned tree in composed shadow tree.
    return FocusNavigationScope(&root->treeScope());
}

FocusNavigationScope FocusNavigationScope::focusNavigationScopeOwnedByShadowHost(Node* node)
{
    ASSERT(node);
    ASSERT(downcast<Element>(*node).shadowRoot());
    return FocusNavigationScope(downcast<Element>(*node).shadowRoot());
}

FocusNavigationScope FocusNavigationScope::focusNavigationScopeOwnedByIFrame(HTMLFrameOwnerElement* frame)
{
    ASSERT(frame);
    ASSERT(frame->contentFrame());
    return FocusNavigationScope(frame->contentFrame()->document());
}

static inline void dispatchEventsOnWindowAndFocusedElement(Document* document, bool focused)
{
    // If we have a focused node we should dispatch blur on it before we blur the window.
    // If we have a focused node we should dispatch focus on it after we focus the window.
    // https://bugs.webkit.org/show_bug.cgi?id=27105

    // Do not fire events while modal dialogs are up.  See https://bugs.webkit.org/show_bug.cgi?id=33962
    if (Page* page = document->page()) {
        if (page->defersLoading())
            return;
    }

    if (!focused && document->focusedElement())
        document->focusedElement()->dispatchBlurEvent(nullptr);
    document->dispatchWindowEvent(Event::create(focused ? eventNames().focusEvent : eventNames().blurEvent, false, false));
    if (focused && document->focusedElement())
        document->focusedElement()->dispatchFocusEvent(nullptr, FocusDirectionNone);
}

static inline bool hasCustomFocusLogic(Element& element)
{
    return is<HTMLElement>(element) && downcast<HTMLElement>(element).hasCustomFocusLogic();
}

static inline bool isNonFocusableShadowHost(Element& element, KeyboardEvent& event)
{
    return !element.isKeyboardFocusable(&event) && element.shadowRoot() && !hasCustomFocusLogic(element);
}

static inline bool isFocusableShadowHost(Node& node, KeyboardEvent& event)
{
    return is<Element>(node) && downcast<Element>(node).isKeyboardFocusable(&event) && downcast<Element>(node).shadowRoot() && !hasCustomFocusLogic(downcast<Element>(node));
}

static inline int adjustedTabIndex(Node& node, KeyboardEvent& event)
{
    if (!is<Element>(node))
        return 0;
    return isNonFocusableShadowHost(downcast<Element>(node), event) ? 0 : downcast<Element>(node).tabIndex();
}

static inline bool shouldVisit(Element& element, KeyboardEvent& event)
{
    return element.isKeyboardFocusable(&event) || isNonFocusableShadowHost(element, event);
}

FocusController::FocusController(Page& page, ViewState::Flags viewState)
    : m_page(page)
    , m_isChangingFocusedFrame(false)
    , m_viewState(viewState)
    , m_focusRepaintTimer(*this, &FocusController::focusRepaintTimerFired)
#if PLATFORM(WKC)
    , m_lastExitRect(0,0,0,0)
    , m_lastEntryRect(0,0,0,0)
    , m_lastEntryPoint(0,0)
    , m_lastDirection(FocusDirectionNone)
#endif
{
}

void FocusController::setFocusedFrame(PassRefPtr<Frame> frame)
{
    ASSERT(!frame || frame->page() == &m_page);
    if (m_focusedFrame == frame || m_isChangingFocusedFrame)
        return;

    m_isChangingFocusedFrame = true;

    RefPtr<Frame> oldFrame = m_focusedFrame;
    RefPtr<Frame> newFrame = frame;

    m_focusedFrame = newFrame;

    // Now that the frame is updated, fire events and update the selection focused states of both frames.
    if (oldFrame && oldFrame->view()) {
        oldFrame->selection().setFocused(false);
        oldFrame->document()->dispatchWindowEvent(Event::create(eventNames().blurEvent, false, false));
    }

    if (newFrame && newFrame->view() && isFocused()) {
        newFrame->selection().setFocused(true);
        newFrame->document()->dispatchWindowEvent(Event::create(eventNames().focusEvent, false, false));
    }

    m_page.chrome().focusedFrameChanged(newFrame.get());

    m_isChangingFocusedFrame = false;
}

Frame& FocusController::focusedOrMainFrame() const
{
    if (Frame* frame = focusedFrame())
        return *frame;
    return m_page.mainFrame();
}

void FocusController::setFocused(bool focused)
{
    m_page.setViewState(focused ? m_viewState | ViewState::IsFocused : m_viewState & ~ViewState::IsFocused);
}

void FocusController::setFocusedInternal(bool focused)
{
    if (!isFocused())
        focusedOrMainFrame().eventHandler().stopAutoscrollTimer();

    if (!m_focusedFrame)
        setFocusedFrame(&m_page.mainFrame());

    if (m_focusedFrame->view()) {
        m_focusedFrame->selection().setFocused(focused);
        dispatchEventsOnWindowAndFocusedElement(m_focusedFrame->document(), focused);
    }
}

Element* FocusController::findFocusableElementDescendingDownIntoFrameDocument(FocusDirection direction, Element* element, KeyboardEvent* event)
{
    // The node we found might be a HTMLFrameOwnerElement, so descend down the tree until we find either:
    // 1) a focusable node, or
    // 2) the deepest-nested HTMLFrameOwnerElement.
    while (is<HTMLFrameOwnerElement>(element)) {
        HTMLFrameOwnerElement& owner = downcast<HTMLFrameOwnerElement>(*element);
        if (!owner.contentFrame())
            break;
        Element* foundElement = findFocusableElement(direction, FocusNavigationScope::focusNavigationScopeOwnedByIFrame(&owner), 0, event);
        if (!foundElement)
            break;
        ASSERT(element != foundElement);
        element = foundElement;
    }
    return element;
}

bool FocusController::setInitialFocus(FocusDirection direction, KeyboardEvent* event)
{
    bool didAdvanceFocus = advanceFocus(direction, event, true);
    
    // If focus is being set initially, accessibility needs to be informed that system focus has moved 
    // into the web area again, even if focus did not change within WebCore. PostNotification is called instead
    // of handleFocusedUIElementChanged, because this will send the notification even if the element is the same.
    if (AXObjectCache* cache = focusedOrMainFrame().document()->existingAXObjectCache())
        cache->postNotification(focusedOrMainFrame().document(), AXObjectCache::AXFocusedUIElementChanged);

    return didAdvanceFocus;
}

bool FocusController::advanceFocus(FocusDirection direction, KeyboardEvent* event, bool initialFocus)
{
#if !PLATFORM(WKC)
    switch (direction) {
    case FocusDirectionForward:
    case FocusDirectionBackward:
        return advanceFocusInDocumentOrder(direction, event, initialFocus);
    case FocusDirectionLeft:
    case FocusDirectionRight:
    case FocusDirectionUp:
    case FocusDirectionDown:
        return advanceFocusDirectionally(direction, event);
    default:
        ASSERT_NOT_REACHED();
    }

    return false;
#else
    return false;
#endif
}

bool FocusController::advanceFocusInDocumentOrder(FocusDirection direction, KeyboardEvent* event, bool initialFocus)
{
    Frame& frame = focusedOrMainFrame();
    Document* document = frame.document();

    Node* currentNode = document->focusedElement();
    // FIXME: Not quite correct when it comes to focus transitions leaving/entering the WebView itself
    bool caretBrowsing = frame.settings().caretBrowsingEnabled();

    if (caretBrowsing && !currentNode)
        currentNode = frame.selection().selection().start().deprecatedNode();

    document->updateLayoutIgnorePendingStylesheets();

    RefPtr<Element> element = findFocusableElementAcrossFocusScope(direction, FocusNavigationScope::focusNavigationScopeOf(currentNode ? currentNode : document), currentNode, event);

    if (!element) {
        // We didn't find a node to focus, so we should try to pass focus to Chrome.
        if (!initialFocus && m_page.chrome().canTakeFocus(direction)) {
            document->setFocusedElement(nullptr);
            setFocusedFrame(nullptr);
            m_page.chrome().takeFocus(direction);
            return true;
        }

        // Chrome doesn't want focus, so we should wrap focus.
        element = findFocusableElementRecursively(direction, FocusNavigationScope::focusNavigationScopeOf(m_page.mainFrame().document()), 0, event);
        element = findFocusableElementDescendingDownIntoFrameDocument(direction, element.get(), event);

        if (!element)
            return false;
    }

    ASSERT(element);

    if (element == document->focusedElement()) {
        // Focus wrapped around to the same element.
        return true;
    }

    if (is<HTMLFrameOwnerElement>(*element) && (!is<HTMLPlugInElement>(*element) || !element->isKeyboardFocusable(event))) {
        // We focus frames rather than frame owners.
        // FIXME: We should not focus frames that have no scrollbars, as focusing them isn't useful to the user.
        HTMLFrameOwnerElement& owner = downcast<HTMLFrameOwnerElement>(*element);
        if (!owner.contentFrame())
            return false;

        document->setFocusedElement(nullptr);
        setFocusedFrame(owner.contentFrame());
        return true;
    }
    
    // FIXME: It would be nice to just be able to call setFocusedElement(node) here, but we can't do
    // that because some elements (e.g. HTMLInputElement and HTMLTextAreaElement) do extra work in
    // their focus() methods.

    Document& newDocument = element->document();

    if (&newDocument != document) {
        // Focus is going away from this document, so clear the focused node.
        document->setFocusedElement(nullptr);
    }

    setFocusedFrame(newDocument.frame());

    if (caretBrowsing) {
        Position position = firstPositionInOrBeforeNode(element.get());
        VisibleSelection newSelection(position, position, DOWNSTREAM);
        if (frame.selection().shouldChangeSelection(newSelection)) {
            AXTextStateChangeIntent intent(AXTextStateChangeTypeSelectionMove, AXTextSelection { AXTextSelectionDirectionDiscontiguous, AXTextSelectionGranularityUnknown, true });
            frame.selection().setSelection(newSelection, FrameSelection::defaultSetSelectionOptions(UserTriggered), intent);
        }
    }

    element->focus(false, direction);
    return true;
}

Element* FocusController::findFocusableElementAcrossFocusScope(FocusDirection direction, FocusNavigationScope scope, Node* currentNode, KeyboardEvent* event)
{
    ASSERT(!is<Element>(currentNode) || !isNonFocusableShadowHost(*downcast<Element>(currentNode), *event));
    Element* found;
    if (currentNode && direction == FocusDirectionForward && isFocusableShadowHost(*currentNode, *event)) {
        Element* foundInInnerFocusScope = findFocusableElementRecursively(direction, FocusNavigationScope::focusNavigationScopeOwnedByShadowHost(currentNode), 0, event);
        found = foundInInnerFocusScope ? foundInInnerFocusScope : findFocusableElementRecursively(direction, scope, currentNode, event);
    } else
        found = findFocusableElementRecursively(direction, scope, currentNode, event);

    // If there's no focusable node to advance to, move up the focus scopes until we find one.
    while (!found) {
        Element* owner = scope.owner();
        if (!owner)
            break;
        scope = FocusNavigationScope::focusNavigationScopeOf(owner);
        if (direction == FocusDirectionBackward && isFocusableShadowHost(*owner, *event)) {
            found = owner;
            break;
        }
        found = findFocusableElementRecursively(direction, scope, owner, event);
    }
    found = findFocusableElementDescendingDownIntoFrameDocument(direction, found, event);
    return found;
}

Element* FocusController::findFocusableElementRecursively(FocusDirection direction, FocusNavigationScope scope, Node* start, KeyboardEvent* event)
{
    // Starting node is exclusive.
    Element* found = findFocusableElement(direction, scope, start, event);
    if (!found)
        return nullptr;
    if (direction == FocusDirectionForward) {
        if (!isNonFocusableShadowHost(*found, *event))
            return found;
        Element* foundInInnerFocusScope = findFocusableElementRecursively(direction, FocusNavigationScope::focusNavigationScopeOwnedByShadowHost(found), 0, event);
        return foundInInnerFocusScope ? foundInInnerFocusScope : findFocusableElementRecursively(direction, scope, found, event);
    }
    ASSERT(direction == FocusDirectionBackward);
    if (isFocusableShadowHost(*found, *event)) {
        Element* foundInInnerFocusScope = findFocusableElementRecursively(direction, FocusNavigationScope::focusNavigationScopeOwnedByShadowHost(found), 0, event);
        return foundInInnerFocusScope ? foundInInnerFocusScope : found;
    }
    if (isNonFocusableShadowHost(*found, *event)) {
        Element* foundInInnerFocusScope = findFocusableElementRecursively(direction, FocusNavigationScope::focusNavigationScopeOwnedByShadowHost(found), 0, event);
        return foundInInnerFocusScope ? foundInInnerFocusScope :findFocusableElementRecursively(direction, scope, found, event);
    }
    return found;
}

Element* FocusController::findFocusableElement(FocusDirection direction, FocusNavigationScope scope, Node* node, KeyboardEvent* event)
{
    return (direction == FocusDirectionForward)
        ? nextFocusableElement(scope, node, event)
        : previousFocusableElement(scope, node, event);
}

Element* FocusController::findElementWithExactTabIndex(Node* start, int tabIndex, KeyboardEvent* event, FocusDirection direction)
{
    // Search is inclusive of start
    using namespace NodeRenderingTraversal;
    for (Node* node = start; node; node = direction == FocusDirectionForward ? nextInScope(node) : previousInScope(node)) {
        if (!is<Element>(*node))
            continue;
        Element& element = downcast<Element>(*node);
        if (shouldVisit(element, *event) && adjustedTabIndex(element, *event) == tabIndex)
            return &element;
    }
    return nullptr;
}

static Element* nextElementWithGreaterTabIndex(Node* start, int tabIndex, KeyboardEvent& event)
{
    // Search is inclusive of start
    int winningTabIndex = std::numeric_limits<short>::max() + 1;
    Element* winner = nullptr;
    for (Node* node = start; node; node = NodeRenderingTraversal::nextInScope(node)) {
        if (!is<Element>(*node))
            continue;
        Element& element = downcast<Element>(*node);
        if (shouldVisit(element, event) && element.tabIndex() > tabIndex && element.tabIndex() < winningTabIndex) {
            winner = &element;
            winningTabIndex = element.tabIndex();
        }
    }

    return winner;
}

static Element* previousElementWithLowerTabIndex(Node* start, int tabIndex, KeyboardEvent& event)
{
    // Search is inclusive of start
    int winningTabIndex = 0;
    Element* winner = nullptr;
    for (Node* node = start; node; node = NodeRenderingTraversal::previousInScope(node)) {
        if (!is<Element>(*node))
            continue;
        Element& element = downcast<Element>(*node);
        int currentTabIndex = adjustedTabIndex(element, event);
        if ((shouldVisit(element, event) || isNonFocusableShadowHost(element, event)) && currentTabIndex < tabIndex && currentTabIndex > winningTabIndex) {
            winner = &element;
            winningTabIndex = currentTabIndex;
        }
    }
    return winner;
}

Element* FocusController::nextFocusableElement(FocusNavigationScope scope, Node* start, KeyboardEvent* event)
{
    using namespace NodeRenderingTraversal;

    if (start) {
        int tabIndex = adjustedTabIndex(*start, *event);
        // If a node is excluded from the normal tabbing cycle, the next focusable node is determined by tree order
        if (tabIndex < 0) {
            for (Node* node = nextInScope(start); node; node = nextInScope(node)) {
                if (!is<Element>(*node))
                    continue;
                Element& element = downcast<Element>(*node);
                if (shouldVisit(element, *event) && adjustedTabIndex(element, *event) >= 0)
                    return &element;
            }
        }

        // First try to find a node with the same tabindex as start that comes after start in the scope.
        if (Element* winner = findElementWithExactTabIndex(nextInScope(start), tabIndex, event, FocusDirectionForward))
            return winner;

        if (!tabIndex)
            // We've reached the last node in the document with a tabindex of 0. This is the end of the tabbing order.
            return 0;
    }

    // Look for the first Element in the scope that:
    // 1) has the lowest tabindex that is higher than start's tabindex (or 0, if start is null), and
    // 2) comes first in the scope, if there's a tie.
    if (Element* winner = nextElementWithGreaterTabIndex(scope.rootNode(), start ? adjustedTabIndex(*start, *event) : 0, *event))
        return winner;

    // There are no nodes with a tabindex greater than start's tabindex,
    // so find the first node with a tabindex of 0.
    return findElementWithExactTabIndex(scope.rootNode(), 0, event, FocusDirectionForward);
}

Element* FocusController::previousFocusableElement(FocusNavigationScope scope, Node* start, KeyboardEvent* event)
{
    using namespace NodeRenderingTraversal;

    Node* last = nullptr;
    for (Node* node = scope.rootNode(); node; node = lastChildInScope(node))
        last = node;
    ASSERT(last);

    // First try to find the last node in the scope that comes before start and has the same tabindex as start.
    // If start is null, find the last node in the scope with a tabindex of 0.
    Node* startingNode;
    int startingTabIndex;
    if (start) {
        startingNode = previousInScope(start);
        startingTabIndex = adjustedTabIndex(*start, *event);
    } else {
        startingNode = last;
        startingTabIndex = 0;
    }

    // However, if a node is excluded from the normal tabbing cycle, the previous focusable node is determined by tree order
    if (startingTabIndex < 0) {
        for (Node* node = startingNode; node; node = previousInScope(node)) {
            if (!is<Element>(*node))
                continue;
            Element& element = downcast<Element>(*node);
            if (shouldVisit(element, *event) && adjustedTabIndex(element, *event) >= 0)
                return &element;
        }
    }

    if (Element* winner = findElementWithExactTabIndex(startingNode, startingTabIndex, event, FocusDirectionBackward))
        return winner;

    // There are no nodes before start with the same tabindex as start, so look for a node that:
    // 1) has the highest non-zero tabindex (that is less than start's tabindex), and
    // 2) comes last in the scope, if there's a tie.
    startingTabIndex = (start && startingTabIndex) ? startingTabIndex : std::numeric_limits<short>::max();
    return previousElementWithLowerTabIndex(last, startingTabIndex, *event);
}

static bool relinquishesEditingFocus(Node *node)
{
    ASSERT(node);
    ASSERT(node->hasEditableStyle());

    Node* root = node->rootEditableElement();
    Frame* frame = node->document().frame();
    if (!frame || !root)
        return false;

    return frame->editor().shouldEndEditing(rangeOfContents(*root).ptr());
}

static void clearSelectionIfNeeded(Frame* oldFocusedFrame, Frame* newFocusedFrame, Node* newFocusedNode)
{
    if (!oldFocusedFrame || !newFocusedFrame)
        return;
        
    if (oldFocusedFrame->document() != newFocusedFrame->document())
        return;

    const VisibleSelection& selection = oldFocusedFrame->selection().selection();
    if (selection.isNone())
        return;

    bool caretBrowsing = oldFocusedFrame->settings().caretBrowsingEnabled();
    if (caretBrowsing)
        return;

    Node* selectionStartNode = selection.start().deprecatedNode();
    if (selectionStartNode == newFocusedNode || selectionStartNode->isDescendantOf(newFocusedNode) || selectionStartNode->deprecatedShadowAncestorNode() == newFocusedNode)
        return;
        
    if (Node* mousePressNode = newFocusedFrame->eventHandler().mousePressNode()) {
        if (mousePressNode->renderer() && !mousePressNode->canStartSelection()) {
            // Don't clear the selection for contentEditable elements, but do clear it for input and textarea. See bug 38696.
            Node * root = selection.rootEditableElement();
            if (!root)
                return;

            if (Node* shadowAncestorNode = root->deprecatedShadowAncestorNode()) {
                if (!is<HTMLInputElement>(*shadowAncestorNode) && !is<HTMLTextAreaElement>(*shadowAncestorNode))
                    return;
            }
        }
    }

    oldFocusedFrame->selection().clear();
}

bool FocusController::setFocusedElement(Element* element, PassRefPtr<Frame> newFocusedFrame, FocusDirection direction)
{
    RefPtr<Frame> oldFocusedFrame = focusedFrame();
    RefPtr<Document> oldDocument = oldFocusedFrame ? oldFocusedFrame->document() : nullptr;
    
    Element* oldFocusedElement = oldDocument ? oldDocument->focusedElement() : nullptr;
    if (oldFocusedElement == element)
        return true;

    // FIXME: Might want to disable this check for caretBrowsing
    if (oldFocusedElement && oldFocusedElement->isRootEditableElement() && !relinquishesEditingFocus(oldFocusedElement))
        return false;

    m_page.editorClient().willSetInputMethodState();

    clearSelectionIfNeeded(oldFocusedFrame.get(), newFocusedFrame.get(), element);

    if (!element) {
        if (oldDocument)
            oldDocument->setFocusedElement(nullptr);
        m_page.editorClient().setInputMethodState(false);
        return true;
    }

    Ref<Document> newDocument(element->document());

    if (newDocument->focusedElement() == element) {
        m_page.editorClient().setInputMethodState(element->shouldUseInputMethod());
        return true;
    }
    
    if (oldDocument && oldDocument != newDocument.ptr())
        oldDocument->setFocusedElement(nullptr);

    if (newFocusedFrame && !newFocusedFrame->page()) {
        setFocusedFrame(nullptr);
        return false;
    }
    setFocusedFrame(newFocusedFrame);

    Ref<Element> protect(*element);

    bool successfullyFocused = newDocument->setFocusedElement(element, direction);
    if (!successfullyFocused)
        return false;

    if (newDocument->focusedElement() == element)
        m_page.editorClient().setInputMethodState(element->shouldUseInputMethod());

    m_focusSetTime = monotonicallyIncreasingTime();
    m_focusRepaintTimer.stop();

    return true;
}

void FocusController::setViewState(ViewState::Flags viewState)
{
    ViewState::Flags changed = m_viewState ^ viewState;
    m_viewState = viewState;

    if (changed & ViewState::IsFocused)
        setFocusedInternal(viewState & ViewState::IsFocused);
    if (changed & ViewState::WindowIsActive) {
        setActiveInternal(viewState & ViewState::WindowIsActive);
        if (changed & ViewState::IsVisible)
            setIsVisibleAndActiveInternal(viewState & ViewState::WindowIsActive);
    }
}

void FocusController::setActive(bool active)
{
    m_page.setViewState(active ? m_viewState | ViewState::WindowIsActive : m_viewState & ~ViewState::WindowIsActive);
}

void FocusController::setActiveInternal(bool active)
{
    if (FrameView* view = m_page.mainFrame().view()) {
        if (!view->platformWidget()) {
            view->updateLayoutAndStyleIfNeededRecursive();
            view->updateControlTints();
        }
    }

    focusedOrMainFrame().selection().pageActivationChanged();
    
    if (m_focusedFrame && isFocused())
        dispatchEventsOnWindowAndFocusedElement(m_focusedFrame->document(), active);
}

static void contentAreaDidShowOrHide(ScrollableArea* scrollableArea, bool didShow)
{
    if (didShow)
        scrollableArea->contentAreaDidShow();
    else
        scrollableArea->contentAreaDidHide();
}

void FocusController::setIsVisibleAndActiveInternal(bool contentIsVisible)
{
    FrameView* view = m_page.mainFrame().view();
    if (!view)
        return;

    contentAreaDidShowOrHide(view, contentIsVisible);

    for (Frame* frame = &m_page.mainFrame(); frame; frame = frame->tree().traverseNext()) {
        FrameView* frameView = frame->view();
        if (!frameView)
            continue;

        const HashSet<ScrollableArea*>* scrollableAreas = frameView->scrollableAreas();
        if (!scrollableAreas)
            continue;

        for (auto& scrollableArea : *scrollableAreas) {
            ASSERT(scrollableArea->scrollbarsCanBeActive() || m_page.shouldSuppressScrollbarAnimations());

            contentAreaDidShowOrHide(scrollableArea, contentIsVisible);
        }
    }
}

#if PLATFORM(WKC)
static bool isFocusControlBannedElement(const FocusCandidate& candidate)
{
    Document& document = candidate.visibleNode->document();
    FrameView* view = document.view();
    MainFrame& mainFrame = document.page()->mainFrame();
    FrameView* mainView = mainFrame.view();

    if (!candidate.visibleNode->isElementNode()) {
        ASSERT_NOT_REACHED();
        return false;
    }
    Element& candidateElement = downcast<Element>(*candidate.visibleNode);

    IntRect frameRect = view->frameRect();
    if (view->parent())
        frameRect = view->parent()->contentsToWindow(frameRect);

    Vector<FloatQuad> quads;
    candidateElement.renderer()->absoluteQuads(quads);
    size_t n = quads.size();
    LayoutRect candidateRect = candidate.rect;

    for (size_t i = 0; i < n; ++i) {
        if (n != 1)
            candidateRect = rectToAbsoluteCoordinates(candidateElement.document().frame(), quads[i].enclosingBoundingBox());

        if (candidateRect.isEmpty())
            continue;

        IntRect rect(candidateRect.x(), candidateRect.y(), candidateRect.width(), candidateRect.height());
        rect = mainView->contentsToWindow(rect);
        rect.intersect(frameRect);
        if (rect.isEmpty())
            continue;

        candidateRect = mainView->windowToContents(rect);
        HitTestResult result = mainFrame.eventHandler().hitTestResultAtPoint(candidateRect.center(), HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::IgnoreClipping | HitTestRequest::DisallowShadowContent);

        Node* hitNode = result.innerNode();

        if (hitNode && hitNode->isTextNode())
            hitNode = hitNode->parentElement();

        if (!hitNode)
            continue;

        // hit test result node is in other window. e.g. iframe
        if (document != hitNode->document())
            continue;

        // If the hit test result node is a label and belongs to the same form as the candidate element, then allows to be focused.
        if (is<HTMLLabelElement>(*hitNode) && candidateElement.isFormControlElement())
            if (downcast<HTMLLabelElement>(*hitNode).form() == downcast<HTMLFormControlElement>(candidateElement).form())
                return false;

        // hit test result node is not in other node.
        if (candidateElement.contains(hitNode))
            return false;
    }

    return true;
}
#endif

static void updateFocusCandidateIfNeeded(FocusDirection direction, const FocusCandidate& current, FocusCandidate& candidate, FocusCandidate& closest)
{
    ASSERT(candidate.visibleNode->isElementNode());
    ASSERT(candidate.visibleNode->renderer());

#if PLATFORM(WKC)
    if (candidate.visibleNode->hasTagName(aTag) && candidate.rect.isEmpty())
        return;

    // Ugh! Ignore any frames... May be inappropriate to scroll frames by focus.
    if (frameOwnerElement(candidate))
        return;

    RefPtr<KeyboardEvent> ev = KeyboardEvent::create();
    if (adjustedTabIndex(*candidate.focusableNode, *ev.get()) < 0)
        return;
#endif

    // Ignore iframes that don't have a src attribute
    if (frameOwnerElement(candidate) && (!frameOwnerElement(candidate)->contentFrame() || candidate.rect.isEmpty()))
        return;

#if PLATFORM(WKC)
    // Ignore off screen child nodes.
    if (candidate.isOffscreen)
        return;
#else
    // Ignore off screen child nodes of containers that do not scroll (overflow:hidden)
    if (candidate.isOffscreen && !canBeScrolledIntoView(direction, candidate))
        return;
#endif

    distanceDataForNode(direction, current, candidate);
    if (candidate.distance == maxDistance())
        return;

    if (candidate.isOffscreenAfterScrolling && candidate.alignment < Full)
        return;

#if PLATFORM(WKC)
    if (isFocusControlBannedElement(candidate))
        return;

    // Heuristic adjustment
    // The rectangles of the same size placed next to one another like a drop down list are more preferable to move focus to.
    if (candidate.alignment == Full || candidate.alignment == OverlapFull) {
        if ((candidate.rect.x() == current.rect.x() && candidate.rect.width() == current.rect.width())
         || (candidate.rect.y() == current.rect.y() && candidate.rect.height() == current.rect.height())) {
            switch (direction) {
            case FocusDirectionUp:
                if (current.rect.y() - candidate.rect.maxY() < current.rect.height() / 3)
                    candidate.alignment = SpecialAlignment;
                break;
            case FocusDirectionDown:
                if (candidate.rect.y() - current.rect.maxY() < current.rect.height() / 3)
                    candidate.alignment = SpecialAlignment;
                break;
            case FocusDirectionLeft:
                if (current.rect.x() - candidate.rect.maxX() < current.rect.width() / 3)
                    candidate.alignment = SpecialAlignment;
                break;
            case FocusDirectionRight:
                if (candidate.rect.x() - current.rect.maxX() < current.rect.width() / 3)
                    candidate.alignment = SpecialAlignment;
                break;
            default:
                break;
            }
        }
    }

    // Heuristic adjustment
    if (candidate.alignment == Partial)
        candidate.alignment = Full;
    if (candidate.alignment == OverlapPartial)
        candidate.alignment = OverlapFull;

    if (closest.isNull()) {
        closest = candidate;
        return;
    }

    if (candidate.alignment < closest.alignment)
        return;

    if (candidate.alignment > closest.alignment) {
        closest = candidate;
        return;
    }

    ASSERT(candidate.alignment == closest.alignment);

    LayoutSize viewSize = candidate.visibleNode->document().page()->mainFrame().view()->visibleContentRect().size();

    if (candidate.distance > closest.distance) {
        if (isRectInDirection(direction, candidate.rect, closest.rect) &&
            alignmentForRects(direction, candidate.rect, closest.rect, viewSize) == Full)
            closest = candidate;
        return;
    }

    if (candidate.distance < closest.distance) {
        if (isRectInDirection(direction, closest.rect, candidate.rect) &&
            alignmentForRects(direction, closest.rect, candidate.rect, viewSize) == Full)
            return;
        closest = candidate;
        return;
    }

    ASSERT(candidate.distance == closest.distance);

    LayoutRect intersectionRect = intersection(candidate.rect, closest.rect);
    if (!intersectionRect.isEmpty()) {
        // If 2 nodes are intersecting, do hit test to find which node in on top.
        LayoutUnit x = intersectionRect.x() + intersectionRect.width() / 2;
        LayoutUnit y = intersectionRect.y() + intersectionRect.height() / 2;
        HitTestResult result = candidate.visibleNode->document().page()->mainFrame().eventHandler().hitTestResultAtPoint(IntPoint(x, y), HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::IgnoreClipping | HitTestRequest::DisallowShadowContent);
        if (candidate.visibleNode->contains(result.innerNode()))
            closest = candidate;
    }
#else
    if (closest.isNull()) {
        closest = candidate;
        return;
    }

    LayoutRect intersectionRect = intersection(candidate.rect, closest.rect);
    if (!intersectionRect.isEmpty() && !areElementsOnSameLine(closest, candidate)) {
        // If 2 nodes are intersecting, do hit test to find which node in on top.
        LayoutUnit x = intersectionRect.x() + intersectionRect.width() / 2;
        LayoutUnit y = intersectionRect.y() + intersectionRect.height() / 2;
        HitTestResult result = candidate.visibleNode->document().page()->mainFrame().eventHandler().hitTestResultAtPoint(IntPoint(x, y), HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::IgnoreClipping | HitTestRequest::DisallowShadowContent);
        if (candidate.visibleNode->contains(result.innerNode())) {
            closest = candidate;
            return;
        }
        if (closest.visibleNode->contains(result.innerNode()))
            return;
    }

    if (candidate.alignment == closest.alignment) {
        if (candidate.distance < closest.distance)
            closest = candidate;
        return;
    }

    if (candidate.alignment > closest.alignment)
        closest = candidate;
#endif
}

void FocusController::findFocusCandidateInContainer(Node& container, const LayoutRect& startingRect, FocusDirection direction, KeyboardEvent* event, FocusCandidate& closest)
{
    Node* focusedNode = (focusedFrame() && focusedFrame()->document()) ? focusedFrame()->document()->focusedElement() : 0;

#if PLATFORM(WKC)
    if (!focusedNode)
        focusedNode = container.document().focusedElement();
#endif

    Element* element = ElementTraversal::firstWithin(container);
    FocusCandidate current;
    current.rect = startingRect;
    current.focusableNode = focusedNode;
    current.visibleNode = focusedNode;

    unsigned candidateCount = 0;
    for (; element; element = (element->isFrameOwnerElement() || canScrollInDirection(element, direction))
        ? ElementTraversal::nextSkippingChildren(*element, &container)
        : ElementTraversal::next(*element, &container)) {
        if (element == focusedNode)
            continue;

        if (!element->isKeyboardFocusable(event) && !element->isFrameOwnerElement() && !canScrollInDirection(element, direction))
            continue;

        FocusCandidate candidate = FocusCandidate(element, direction);
        if (candidate.isNull())
            continue;

        if (!isValidCandidate(direction, current, candidate))
            continue;

        candidateCount++;
        candidate.enclosingScrollableBox = &container;
        updateFocusCandidateIfNeeded(direction, current, candidate, closest);
    }

    // The variable 'candidateCount' keeps track of the number of nodes traversed in a given container.
    // If we have more than one container in a page then the total number of nodes traversed is equal to the sum of nodes traversed in each container.
    if (focusedFrame() && focusedFrame()->document()) {
        candidateCount += focusedFrame()->document()->page()->lastSpatialNavigationCandidateCount();
        focusedFrame()->document()->page()->setLastSpatialNavigationCandidateCount(candidateCount);
    }
}

bool FocusController::advanceFocusDirectionallyInContainer(Node* container, const LayoutRect& startingRect, FocusDirection direction, KeyboardEvent* event)
{
#if PLATFORM(WKC)
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);
#endif
    if (!container)
        return false;

    LayoutRect newStartingRect = startingRect;

    if (startingRect.isEmpty())
        newStartingRect = virtualRectForDirection(direction, nodeRectInAbsoluteCoordinates(container));

    // Find the closest node within current container in the direction of the navigation.
    FocusCandidate focusCandidate;
    findFocusCandidateInContainer(*container, newStartingRect, direction, event, focusCandidate);

    if (focusCandidate.isNull()) {
        // Nothing to focus, scroll if possible.
        // NOTE: If no scrolling is performed (i.e. scrollInDirection returns false), the
        // spatial navigation algorithm will skip this container.
        return scrollInDirection(container, direction);
    }

#if PLATFORM(WKC)
    Element* visibleElement = downcast<Element>(focusCandidate.visibleNode);
    if (!visibleElement->supportsFocus())
#endif
    if (HTMLFrameOwnerElement* frameElement = frameOwnerElement(focusCandidate)) {
        // If we have an iframe without the src attribute, it will not have a contentFrame().
        // We ASSERT here to make sure that
        // updateFocusCandidateIfNeeded() will never consider such an iframe as a candidate.
        ASSERT(frameElement->contentFrame());

        if (focusCandidate.isOffscreenAfterScrolling) {
            scrollInDirection(&focusCandidate.visibleNode->document(), direction);
            return true;
        }
        // Navigate into a new frame.
        LayoutRect rect;
        Element* focusedElement = focusedOrMainFrame().document()->focusedElement();
        if (focusedElement && !hasOffscreenRect(focusedElement))
            rect = nodeRectInAbsoluteCoordinates(focusedElement, true /* ignore border */);
        frameElement->contentFrame()->document()->updateLayoutIgnorePendingStylesheets();
        if (!advanceFocusDirectionallyInContainer(frameElement->contentFrame()->document(), rect, direction, event)) {
            // The new frame had nothing interesting, need to find another candidate.
            return advanceFocusDirectionallyInContainer(container, nodeRectInAbsoluteCoordinates(focusCandidate.visibleNode, true), direction, event);
        }
        return true;
    }

#if PLATFORM(WKC)
    // If focusCandidate is block element and has OverFlow settings(except for VISIBLE setting) or Select element, this processing is skipped. 
    if (!focusCandidate.visibleNode->isScrollableOverFlowBlockNode() && !focusCandidate.visibleNode->hasTagName(selectTag) && canScrollInDirection(focusCandidate.visibleNode, direction)) {
#else
    if (canScrollInDirection(focusCandidate.visibleNode, direction)) {
#endif
        if (focusCandidate.isOffscreenAfterScrolling) {
            scrollInDirection(focusCandidate.visibleNode, direction);
            return true;
        }
        // Navigate into a new scrollable container.
        LayoutRect startingRect;
        Element* focusedElement = focusedOrMainFrame().document()->focusedElement();
        if (focusedElement && !hasOffscreenRect(focusedElement))
            startingRect = nodeRectInAbsoluteCoordinates(focusedElement, true);
        return advanceFocusDirectionallyInContainer(focusCandidate.visibleNode, startingRect, direction, event);
    }
#if PLATFORM(WKC)
    if (focusCandidate.isOffscreenAfterScrolling || (focusCandidate.isOrgRectEmpty && focusCandidate.isOffscreen)) {
#else
    if (focusCandidate.isOffscreenAfterScrolling) {
#endif
        Node* container = focusCandidate.enclosingScrollableBox;
        scrollInDirection(container, direction);
        return true;
    }

    // We found a new focus node, navigate to it.
    Element* element = downcast<Element>(focusCandidate.focusableNode);
    ASSERT(element);

    element->focus(false, direction);
    return true;
}

bool FocusController::advanceFocusDirectionally(FocusDirection direction, KeyboardEvent* event)
{
    Document* focusedDocument = focusedOrMainFrame().document();
    if (!focusedDocument)
        return false;

    Element* focusedElement = focusedDocument->focusedElement();
    Node* container = focusedDocument;

    if (is<Document>(*container))
        downcast<Document>(*container).updateLayoutIgnorePendingStylesheets();

    // Figure out the starting rect.
    LayoutRect startingRect;
    if (focusedElement) {
#if PLATFORM(WKC)
    //If focsedNode or ParentFrame is block element and has OverFlow settings(except for VISIBLE setting), container is them.
        Node* scrollableNode = scrollableEnclosingBoxOrParentFrameForNodeInDirection(direction, focusedElement);
        if (focusedElement->isScrollableOverFlowBlockNode()) {
            container = focusedElement;
            startingRect = nodeRectInAbsoluteCoordinates(container, true /* ignore border */);
            startingRect = virtualRectForDirection(direction, startingRect, 1);
        } else if (scrollableNode && scrollableNode->isScrollableOverFlowBlockNode()) {
            container = scrollableNode;
            startingRect = nodeRectInAbsoluteCoordinates(focusedElement, true /* ignore border */);
        } else if (!hasOffscreenRect(focusedElement)) {
            container = scrollableNode;
            startingRect = nodeRectInAbsoluteCoordinates(focusedElement, true /* ignore border */);
        } else if (focusedElement->hasTagName(areaTag)) {
            HTMLAreaElement& area = downcast<HTMLAreaElement>(*focusedElement);
            container = scrollableEnclosingBoxOrParentFrameForNodeInDirection(direction, area.imageElement());
            startingRect = virtualRectForAreaElementAndDirection(&area, direction);
        }
        // Pieris:0295616
        // scrollableNode may be 0 if focusedElement is a node under the shadow root.
        // In this case we use focusedDocument as container.
        if (!container) {
            container = focusedDocument;
        }
#else
        if (!hasOffscreenRect(focusedElement)) {
            container = scrollableEnclosingBoxOrParentFrameForNodeInDirection(direction, focusedElement);
            startingRect = nodeRectInAbsoluteCoordinates(focusedElement, true /* ignore border */);
        } else if (is<HTMLAreaElement>(*focusedElement)) {
            HTMLAreaElement& area = downcast<HTMLAreaElement>(*focusedElement);
            container = scrollableEnclosingBoxOrParentFrameForNodeInDirection(direction, area.imageElement());
            startingRect = virtualRectForAreaElementAndDirection(&area, direction);
        }
#endif
    }

    if (focusedFrame() && focusedFrame()->document())
        focusedDocument->page()->setLastSpatialNavigationCandidateCount(0);

    bool consumed = false;
    do {
        consumed = advanceFocusDirectionallyInContainer(container, startingRect, direction, event);
        startingRect = nodeRectInAbsoluteCoordinates(container, true /* ignore border */);
        container = scrollableEnclosingBoxOrParentFrameForNodeInDirection(direction, container);
        if (is<Document>(container))
            downcast<Document>(*container).updateLayoutIgnorePendingStylesheets();
    } while (!consumed && container);

    return consumed;
}

void FocusController::setFocusedElementNeedsRepaint()
{
    m_focusRepaintTimer.startOneShot(0.033);
}

void FocusController::focusRepaintTimerFired()
{
    Document* focusedDocument = focusedOrMainFrame().document();
    if (!focusedDocument)
        return;

    Element* focusedElement = focusedDocument->focusedElement();
    if (!focusedElement)
        return;

    if (focusedElement->renderer())
        focusedElement->renderer()->repaint();
}

double FocusController::timeSinceFocusWasSet() const
{
    return monotonicallyIncreasingTime() - m_focusSetTime;
}

#if PLATFORM(WKC)
static bool isNodeInSpecificRect(Node* node, const LayoutRect* specificRect)
{
    if (!specificRect || specificRect->isEmpty())
        return !hasOffscreenRect(node);

    FrameView* frameView = node->document().view();
    if (!frameView)
        return false;

    LayoutRect rect;
    if (node->hasTagName(HTMLNames::areaTag)) {
        HTMLAreaElement* area = static_cast<HTMLAreaElement*>(node);
        HTMLImageElement* image = area->imageElement();
        if (!image || !image->renderer())
            return false;
        rect = rectToAbsoluteCoordinates(area->document().frame(), area->computeRect(image->renderer()));
    } else {
        RenderObject* render = node->renderer();
        if (!render)
            return false;
        rect = nodeRectInAbsoluteCoordinates(node, true /* ignore border */);
    }
    if (rect.isEmpty())
        return false;

    return specificRect->intersects(rect);
}

static bool isScrollableContainerNode(Node* node)
{
    if (!node)
        return false;

    if (RenderObject* renderer = node->renderer()) {
        return (renderer->isBox() && downcast<RenderBox>(renderer)->canBeScrolledAndHasScrollableArea()
             && node->hasChildNodes() && !node->isDocumentNode() && node->firstChild()->isElementNode());
    }

    return false;
}

void FocusController::findFocusableNodeInDirection(Node* container, Node* startingElement, const LayoutRect& startingRect, FocusDirection direction, KeyboardEvent* event, FocusCandidate& closest, const LayoutRect* scope)
{
    ASSERT(container);
    ASSERT(startingElement);

    Element* element = ElementTraversal::firstWithin(*container);
    FocusCandidate current;
    current.rect = startingRect;
    current.focusableNode = startingElement;
    current.visibleNode = startingElement;
    current.direction = m_lastDirection;
    current.exitRect = m_lastExitRect;
    if (!m_lastEntryRect.isEmpty() && current.rect.contains(m_lastEntryRect))
        current.entryRect = m_lastEntryRect;
    else
        current.entryRect = current.rect;

    LayoutRect r(current.rect);
    r.inflate(1); // to contain the points on the edge.
    if (m_lastEntryPoint != IntPoint::zero() && r.contains(m_lastEntryPoint))
        current.entryPoint = m_lastEntryPoint;
    else
        current.entryPoint = current.rect.center();

    for (; element; element = (element->isFrameOwnerElement() || canScrollInDirection(element, direction) || isScrollableContainerNode(element))
        ? ElementTraversal::nextSkippingChildren(*element, container)
        : ElementTraversal::next(*element, container)) {

        if (scope && !isNodeInSpecificRect(element, scope))
            continue;

        if (isScrollableContainerNode(element) && !element->renderer()->isTextArea()) {
            findFocusableNodeInDirection(element, startingElement, current.rect, direction, event, closest, scope);
            continue;
        }

        if (element == startingElement)
            continue;

        if (!element->isKeyboardFocusable(event) && !element->isFrameOwnerElement() && !canScrollInDirection(element, direction))
            continue;

        FocusCandidate candidate = FocusCandidate(element, direction);
        if (candidate.isNull())
            continue;

        candidate.enclosingScrollableBox = container;
        updateFocusCandidateIfNeeded(direction, current, candidate, closest);

        if (HTMLFrameOwnerElement* frameElement = frameOwnerElement(candidate)) {
            // Navigate into a new frame.
            if (!frameElement->contentFrame())
                continue;
            frameElement->contentFrame()->document()->updateLayoutIgnorePendingStylesheets();
            findFocusableNodeInDirection(frameElement->contentFrame()->document(), startingElement, current.rect, direction, event, closest, scope);
        }
    }
    if (closest.focusableNode && closest.focusableNode->isElementNode() && closest.distance != maxDistance()) {
        m_lastExitRect = current.entryRect;
        m_lastEntryRect = closest.entryRect;
        m_lastEntryPoint = closest.entryPoint;
        m_lastDirection = direction;
    }
}

static Element*
findFirstFocusableElement(Frame& frame, const LayoutRect* scope)
{
    ContainerNode* container = frame.document();
    Element* element = ElementTraversal::firstWithin(*container);

    for (; element; element = ElementTraversal::next(*element, container)) {
        RefPtr<KeyboardEvent> ev = KeyboardEvent::create();
        if (!isNodeInSpecificRect(element, scope) || adjustedTabIndex(*element, *ev.get()) < 0)
            continue;
        if (element->isFrameOwnerElement()) {
            HTMLFrameOwnerElement* owner = downcast<HTMLFrameOwnerElement>(element);
            if (owner->contentFrame()) {
                element = findFirstFocusableElement(*owner->contentFrame(), scope);
                if (element)
                    break;
                element = owner;
            }
        } else if (isScrollableContainerNode(element) && !element->renderer()->isTextArea()) {
            Element* child = ElementTraversal::firstChild(*element);
            if (child)
                element = child;
        }
        if (element->isFocusable() && !element->isFrameOwnerElement())
            break;
    }

    if (element && element->isFocusable())
        return element;

    return 0;
}

static Element*
findLastFocusableElement(Frame& frame, const LayoutRect* scope)
{
    ContainerNode* container = frame.document();
    Element* element = ElementTraversal::lastWithin(*container);

    for (; element; element = ElementTraversal::previous(*element, container)) {
        RefPtr<KeyboardEvent> ev = KeyboardEvent::create();
        if (!isNodeInSpecificRect(element, scope) || adjustedTabIndex(*element, *ev.get()) < 0)
            continue;
        if (element->isFrameOwnerElement()) {
            HTMLFrameOwnerElement* owner = downcast<HTMLFrameOwnerElement>(element);
            if (owner->contentFrame()) {
                element = findLastFocusableElement(*owner->contentFrame(), scope);
                if (element)
                    break;
                element = owner;
            }
        } else if (isScrollableContainerNode(element) && !element->renderer()->isTextArea()) {
            Element* child = ElementTraversal::lastChild(*element);
            if (child)
                element = child;
        }
        if (element->isFocusable() && !element->isFrameOwnerElement())
            break;
    }

    if (element && element->isFocusable())
        return element;

    return 0;
}

Element* FocusController::findNextFocusableElement(const FocusDirection& direction, const IntRect* scope, Element* base)
{
    Frame& frame = focusedOrMainFrame();
    Document* focusedDocument = frame.document();
    if (!focusedDocument)
        return 0;

    focusedDocument->updateLayoutIgnorePendingStylesheets();

    LayoutRect scopeRect(0,0,0,0);

    if (!base) {
        base = focusedDocument->focusedElement();
    }
    if (!base) {
        if (scope && !scope->isEmpty()) {
            LayoutRect lr = m_page.mainFrame().view()->windowToContents(*scope);
            scopeRect = lr;
        }
        if (direction == FocusDirectionUp
         || direction == FocusDirectionUpLeft
         || direction == FocusDirectionUpRight) {
            return findLastFocusableElement(m_page.mainFrame(), &scopeRect);
        } else {
            return findFirstFocusableElement(m_page.mainFrame(), &scopeRect);
        }
    }

    LayoutRect startingRect;
    if (!hasOffscreenRect(static_cast<Node*>(base))) {
        startingRect = nodeRectInAbsoluteCoordinates(base, true /* ignore border */);
    } else if (base->hasTagName(areaTag)) {
        startingRect = virtualRectForAreaElementAndDirection(downcast<HTMLAreaElement>(base), direction);
    }

    FocusCandidate focusCandidate;
    Frame& nframe = frame.tree().top();
    if (scope && !scope->isEmpty()) {
        LayoutRect lr = nframe.view()->windowToContents(*scope);
        scopeRect = lr;
    }
    findFocusableNodeInDirection(nframe.document(), base, startingRect, direction, 0, focusCandidate, &scopeRect);
    ASSERT(!frameOwnerElement(focusCandidate));

    Node* node = focusCandidate.focusableNode;
    if (!node || !node->isElementNode()) {
        return 0;
    }

    if (focusCandidate.isOffscreen) {
        return 0;
    }

    return downcast<Element>(node);
}

static Element*
getClosestElement(Element* element, FocusDirection direction, bool sibling = false)
{
    ASSERT(element);
    ASSERT(direction == FocusDirectionUp       ||
           direction == FocusDirectionDown     ||
           direction == FocusDirectionLeft     ||
           direction == FocusDirectionRight    ||
           direction == FocusDirectionUpLeft   ||
           direction == FocusDirectionUpRight  ||
           direction == FocusDirectionDownLeft ||
           direction == FocusDirectionDownRight);

    Element* closestElement;
    switch (direction) {
    case FocusDirectionUp:
    case FocusDirectionUpLeft:
    case FocusDirectionUpRight:
        if (sibling)
            closestElement = ElementTraversal::previousSibling(*element);
        else
            closestElement = ElementTraversal::previous(*element);
        break;
    case FocusDirectionDown:
    case FocusDirectionDownLeft:
    case FocusDirectionDownRight:
    case FocusDirectionLeft:
    case FocusDirectionRight:
        if (sibling)
            closestElement = ElementTraversal::nextSibling(*element);
        else
            closestElement = ElementTraversal::next(*element);
        break;
    default:
        ASSERT(false);
        closestElement = 0; // avoid warning
        break;
    }
    return closestElement;
}

static Element* findVerticallyFocusableElement(FocusDirection direction, Element* start, KeyboardEvent* event, const LayoutRect* scope)
{
    CRASH_IF_STACK_OVERFLOW(WKC_STACK_MARGIN_DEFAULT);

    ASSERT(direction == FocusDirectionUp || direction == FocusDirectionDown);

    if (!start)
        return 0;

    Element* element = 0;
    Document* document;

    for (element = start; element; element = getClosestElement(element, direction)) {

        if (!element->isFocusable()) {
            continue;
        }

        RefPtr<KeyboardEvent> ev = KeyboardEvent::create();
        if (adjustedTabIndex(*element, *ev.get()) < 0) {
            continue;
        }

        if (!isNodeInSpecificRect(element, scope)) {
            continue;
        }

        FocusCandidate candidate = FocusCandidate(element, direction);
        if (candidate.isNull())
            continue;

        if (element->isFrameOwnerElement() || (isScrollableContainerNode(element) && !element->renderer()->isTextArea())) {
            Element* childElement = 0;
            if (element->isFrameOwnerElement()) {
                HTMLFrameOwnerElement* owner = downcast<HTMLFrameOwnerElement>(element);
                if (!owner->contentFrame()) {
                    element = 0;
                    break;
                }
                document = owner->contentFrame()->document();
                if (direction == FocusDirectionUp) {
                    childElement = ElementTraversal::lastWithin(*document);
                } else {
                    childElement = ElementTraversal::firstWithin(*document);
                }
            } else {
                if (direction == FocusDirectionUp) {
                    childElement = ElementTraversal::lastChild(*element);
                } else {
                    childElement = ElementTraversal::firstChild(*element);
                }
            }

            FrameView* frameView = element->document().view();
            if (!frameView) {
                return 0;
            }
            LayoutRect scopeRect = element->renderer()->absoluteBoundingBoxRect(true);
            if (scope && !scope->isEmpty())
                scopeRect.intersect(*scope);

            childElement = findVerticallyFocusableElement(direction, childElement, event, &scopeRect);

            if (childElement) {
                element = childElement;
                break;
            }
            continue;
        }
        break;
    }
    return element;
}

static int
distanceBetweenRectAndPoint(const LayoutRect& rect,  const LayoutPoint& point)
{
    if (rect.contains(point)) {
        return 0;
    }
    int hdistance = 0;
    int vdistance = 0;
    if (point.x() < rect.x()) {
        hdistance = rect.x() - point.x();
    } else if (point.x() > rect.maxX()) {
        hdistance = point.x() - rect.maxX();
    }
    if (point.y() < rect.y()) {
        vdistance = rect.y() - point.y();
    } else if (point.y() > rect.maxY()) {
        vdistance = point.y() - rect.maxY();
    }
    return (hdistance * hdistance) + (vdistance * vdistance);
}

static int
distanceBetweenElementAndPoint(const Element* element,  const LayoutPoint& contentsPoint)
{
    int nearDist = INT_MAX;

    if (element->hasTagName(areaTag)) {
        const HTMLAreaElement* area = downcast<HTMLAreaElement>(element);
        const HTMLImageElement* image = area->imageElement();
        if (!image || !image->renderer())
            return nearDist;
        LayoutRect rect = rectToAbsoluteCoordinates(area->document().frame(), area->computeRect(image->renderer()));
        return distanceBetweenRectAndPoint(rect, contentsPoint);
    }

    if (!element->renderer())
        return nearDist;

    Vector<FloatQuad> quads;
    element->renderer()->absoluteQuads(quads);
    size_t n = quads.size();

    for (size_t i = 0; i < n; ++i) {
        LayoutRect rect = rectToAbsoluteCoordinates(element->document().frame(), quads[i].enclosingBoundingBox());
        int dist = distanceBetweenRectAndPoint(rect, contentsPoint);
        ASSERT(dist >= 0);
        if (dist == 0)
            return 0;
        if (dist < nearDist)
            nearDist = dist;
    }
    return nearDist;
}

Element*
FocusController::findNearestFocusableElementFromPoint(const IntPoint& point, const IntRect* scope)
{
    Element* nearest = 0;
    int nearDist = INT_MAX;
    LayoutRect scopeRect(0,0,0,0);

    Frame& fr = m_page.mainFrame();
    Document* doc = m_page.mainFrame().document();
    FrameView* frameView = m_page.mainFrame().view();
    if (!frameView)
        return 0;

    RefPtr<FrameView> protector(frameView);

    LayoutPoint contentsPoint = frameView->windowToContents(point);
    if (scope && !scope->isEmpty()) {
        LayoutRect lr = frameView->windowToContents(*scope);
        scopeRect = lr;
    }

    Element* element = findFirstFocusableElement(fr, &scopeRect);

    for (; element; element = findVerticallyFocusableElement(FocusDirectionDown, ElementTraversal::next(*element), 0, &scopeRect)) {
        if (!element->isKeyboardFocusable(0))
            continue;
        FocusCandidate candidate = FocusCandidate(element, FocusDirectionDown);
        if (candidate.isNull())
            continue;
        if (candidate.isOffscreen)
            continue;
        if (isFocusControlBannedElement(candidate))
            continue;
        int dist = distanceBetweenElementAndPoint(element, contentsPoint);
        ASSERT(dist >= 0);
        if (dist == 0) {
            nearest = element;
            break;
        }
        if (dist < nearDist) {
            nearest = element;
            nearDist = dist;
        }
    }

    return nearest;
}

#endif // PLATFORM(WKC)
} // namespace WebCore

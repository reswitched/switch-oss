/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2010, 2011, 2012, 2013 Google Inc. All rights reserved.
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
#include "EventDispatcher.h"

#include "EventContext.h"
#include "FocusEvent.h"
#include "FrameView.h"
#include "HTMLInputElement.h"
#include "HTMLMediaElement.h"
#include "InsertionPoint.h"
#include "InspectorInstrumentation.h"
#include "MouseEvent.h"
#include "NoEventDispatchAssertion.h"
#include "PseudoElement.h"
#include "ScopedEventQueue.h"
#include "ShadowRoot.h"
#include "SVGNames.h"
#include "SVGUseElement.h"
#include "TouchEvent.h"
#if ENABLE(TOUCH_EVENTS) && !PLATFORM(IOS)
#include "Touch.h"
#endif

namespace WebCore {

class WindowEventContext {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    WindowEventContext(PassRefPtr<Node>, const EventContext*);

    DOMWindow* window() const { return m_window.get(); }
    EventTarget* target() const { return m_target.get(); }
    bool handleLocalEvents(Event&);

private:
    RefPtr<DOMWindow> m_window;
    RefPtr<EventTarget> m_target;
};

WindowEventContext::WindowEventContext(PassRefPtr<Node> node, const EventContext* topEventContext)
{
    Node* topLevelContainer = topEventContext ? topEventContext->node() : node.get();
    if (!is<Document>(*topLevelContainer))
        return;

    m_window = downcast<Document>(*topLevelContainer).domWindow();
    m_target = topEventContext ? topEventContext->target() : node.get();
}

bool WindowEventContext::handleLocalEvents(Event& event)
{
    if (!m_window)
        return false;

    event.setTarget(m_target.get());
    event.setCurrentTarget(m_window.get());
    m_window->fireEventListeners(&event);
    return true;
}

class EventPath {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    EventPath(Node& origin, Event&);

    bool isEmpty() const { return m_path.isEmpty(); }
    size_t size() const { return m_path.size(); }
    const EventContext& contextAt(size_t i) const { return *m_path[i]; }
    EventContext& contextAt(size_t i) { return *m_path[i]; }

#if ENABLE(TOUCH_EVENTS)
    bool updateTouchLists(const TouchEvent&);
#endif
    void setRelatedTarget(Node& origin, EventTarget&);

    bool hasEventListeners(const AtomicString& eventType) const;

    EventContext* lastContextIfExists() { return m_path.isEmpty() ? 0 : m_path.last().get(); }

private:
#if ENABLE(TOUCH_EVENTS) && !PLATFORM(IOS)
    void updateTouchListsInEventPath(const TouchList*, TouchEventContext::TouchListType);
#endif

    Vector<std::unique_ptr<EventContext>, 32> m_path;
};

class EventRelatedNodeResolver {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    EventRelatedNodeResolver(Node& relatedNode)
        : m_relatedNode(relatedNode)
        , m_relatedNodeTreeScope(relatedNode.treeScope())
        , m_relatedNodeInCurrentTreeScope(nullptr)
        , m_currentTreeScope(nullptr)
#if ENABLE(TOUCH_EVENTS) && !PLATFORM(IOS)
        , m_touch(0)
        , m_touchListType(TouchEventContext::NotTouchList)
#endif
    {
    }

#if ENABLE(TOUCH_EVENTS) && !PLATFORM(IOS)
    EventRelatedNodeResolver(Touch& touch, TouchEventContext::TouchListType touchListType)
        : m_relatedNode(*touch.target()->toNode())
        , m_relatedNodeTreeScope(m_relatedNode.treeScope())
        , m_relatedNodeInCurrentTreeScope(nullptr)
        , m_currentTreeScope(nullptr)
        , m_touch(&touch)
        , m_touchListType(touchListType)
    {
        ASSERT(touch.target()->toNode());
    }
#endif

#if ENABLE(TOUCH_EVENTS) && !PLATFORM(IOS)
    Touch* touch() const { return m_touch; }
    TouchEventContext::TouchListType touchListType() const { return m_touchListType; }
#endif

    Node* moveToParentOrShadowHost(Node& newTarget)
    {
        TreeScope& newTreeScope = newTarget.treeScope();
        if (&newTreeScope == m_currentTreeScope)
            return m_relatedNodeInCurrentTreeScope;

        if (m_currentTreeScope) {
            ASSERT(is<ShadowRoot>(m_currentTreeScope->rootNode()));
            ASSERT(&newTarget == downcast<ShadowRoot>(m_currentTreeScope->rootNode()).hostElement());
            ASSERT(m_currentTreeScope->parentTreeScope() == &newTreeScope);
        }

        if (&newTreeScope == &m_relatedNodeTreeScope)
            m_relatedNodeInCurrentTreeScope = &m_relatedNode;
        else if (m_relatedNodeInCurrentTreeScope) {
            ASSERT(m_currentTreeScope);
            m_relatedNodeInCurrentTreeScope = &newTarget;
        } else {
            if (!m_currentTreeScope) {
                TreeScope* newTreeScopeAncestor = &newTreeScope;
                do {
                    m_relatedNodeInCurrentTreeScope = findHostOfTreeScopeInTargetTreeScope(m_relatedNodeTreeScope, *newTreeScopeAncestor);
                    newTreeScopeAncestor = newTreeScopeAncestor->parentTreeScope();
                    if (newTreeScopeAncestor == &m_relatedNodeTreeScope) {
                        m_relatedNodeInCurrentTreeScope = &m_relatedNode;
                        break;
                    }
                } while (newTreeScopeAncestor && !m_relatedNodeInCurrentTreeScope);
            }
            ASSERT(m_relatedNodeInCurrentTreeScope || findHostOfTreeScopeInTargetTreeScope(newTreeScope, m_relatedNodeTreeScope)
                || &newTreeScope.documentScope() != &m_relatedNodeTreeScope.documentScope());
        }

        m_currentTreeScope = &newTreeScope;

        return m_relatedNodeInCurrentTreeScope;
    }

    static Node* findHostOfTreeScopeInTargetTreeScope(const TreeScope& startingTreeScope, const TreeScope& targetScope)
    {
        ASSERT(&targetScope != &startingTreeScope);
        Node* previousHost = nullptr;
        for (const TreeScope* scope = &startingTreeScope; scope; scope = scope->parentTreeScope()) {
            if (scope == &targetScope) {
                ASSERT(previousHost);
                ASSERT_WITH_SECURITY_IMPLICATION(&previousHost->treeScope() == &targetScope);
                return previousHost;
            }
            if (is<ShadowRoot>(scope->rootNode()))
                previousHost = downcast<ShadowRoot>(scope->rootNode()).hostElement();
            else
                ASSERT_WITH_SECURITY_IMPLICATION(!scope->parentTreeScope());
        }
        return nullptr;
    }

private:
    Node& m_relatedNode;
    const TreeScope& m_relatedNodeTreeScope;
    Node* m_relatedNodeInCurrentTreeScope;
    TreeScope* m_currentTreeScope;
#if ENABLE(TOUCH_EVENTS) && !PLATFORM(IOS)
    Touch* m_touch;
    TouchEventContext::TouchListType m_touchListType;
#endif
};

inline EventTarget* eventTargetRespectingTargetRules(Node& referenceNode)
{
    if (is<PseudoElement>(referenceNode))
        return downcast<PseudoElement>(referenceNode).hostElement();

    // Events sent to elements inside an SVG use element's shadow tree go to the use element.
    if (is<SVGElement>(referenceNode)) {
        if (auto* useElement = downcast<SVGElement>(referenceNode).correspondingUseElement())
            return useElement;
    }

    return &referenceNode;
}

void EventDispatcher::dispatchScopedEvent(Node& node, PassRefPtr<Event> event)
{
    // We need to set the target here because it can go away by the time we actually fire the event.
    event->setTarget(eventTargetRespectingTargetRules(node));
    ScopedEventQueue::singleton().enqueueEvent(event);
}

void EventDispatcher::dispatchSimulatedClick(Element* element, Event* underlyingEvent, SimulatedClickMouseEventOptions mouseEventOptions, SimulatedClickVisualOptions visualOptions)
{
    if (element->isDisabledFormControl())
        return;

    DEPRECATED_DEFINE_STATIC_LOCAL(HashSet<Element*>, elementsDispatchingSimulatedClicks, ());
    if (!elementsDispatchingSimulatedClicks.add(element).isNewEntry)
        return;

    if (mouseEventOptions == SendMouseOverUpDownEvents)
        dispatchEvent(element, SimulatedMouseEvent::create(eventNames().mouseoverEvent, element->document().defaultView(), underlyingEvent, element));

    if (mouseEventOptions != SendNoEvents)
        dispatchEvent(element, SimulatedMouseEvent::create(eventNames().mousedownEvent, element->document().defaultView(), underlyingEvent, element));
    element->setActive(true, visualOptions == ShowPressedLook);
    if (mouseEventOptions != SendNoEvents)
        dispatchEvent(element, SimulatedMouseEvent::create(eventNames().mouseupEvent, element->document().defaultView(), underlyingEvent, element));
    element->setActive(false);

    // always send click
    dispatchEvent(element, SimulatedMouseEvent::create(eventNames().clickEvent, element->document().defaultView(), underlyingEvent, element));

    elementsDispatchingSimulatedClicks.remove(element);
}

static void callDefaultEventHandlersInTheBubblingOrder(Event& event, const EventPath& path)
{
    if (path.isEmpty())
        return;

    // Non-bubbling events call only one default event handler, the one for the target.
    path.contextAt(0).node()->defaultEventHandler(&event);
    ASSERT(!event.defaultPrevented());

    if (event.defaultHandled() || !event.bubbles())
        return;

    size_t size = path.size();
    for (size_t i = 1; i < size; ++i) {
        path.contextAt(i).node()->defaultEventHandler(&event);
        ASSERT(!event.defaultPrevented());
        if (event.defaultHandled())
            return;
    }
}

static void dispatchEventInDOM(Event& event, const EventPath& path, WindowEventContext& windowEventContext)
{
    // Trigger capturing event handlers, starting at the top and working our way down.
    event.setEventPhase(Event::CAPTURING_PHASE);

    // We don't dispatch load events to the window. This quirk was originally
    // added because Mozilla doesn't propagate load events to the window object.
    bool shouldFireEventAtWindow = event.type() != eventNames().loadEvent;
    if (shouldFireEventAtWindow && windowEventContext.handleLocalEvents(event) && event.propagationStopped())
        return;

    for (size_t i = path.size() - 1; i > 0; --i) {
        const EventContext& eventContext = path.contextAt(i);
        if (eventContext.currentTargetSameAsTarget())
            continue;
        eventContext.handleLocalEvents(event);
        if (event.propagationStopped())
            return;
    }

    event.setEventPhase(Event::AT_TARGET);
    path.contextAt(0).handleLocalEvents(event);
    if (event.propagationStopped())
        return;

    // Trigger bubbling event handlers, starting at the bottom and working our way up.
    size_t size = path.size();
    for (size_t i = 1; i < size; ++i) {
        const EventContext& eventContext = path.contextAt(i);
        if (eventContext.currentTargetSameAsTarget())
            event.setEventPhase(Event::AT_TARGET);
        else if (event.bubbles() && !event.cancelBubble())
            event.setEventPhase(Event::BUBBLING_PHASE);
        else
            continue;
        eventContext.handleLocalEvents(event);
        if (event.propagationStopped())
            return;
    }
    if (event.bubbles() && !event.cancelBubble()) {
        event.setEventPhase(Event::BUBBLING_PHASE);
        if (shouldFireEventAtWindow)
            windowEventContext.handleLocalEvents(event);
    }
}

bool EventDispatcher::dispatchEvent(Node* origin, PassRefPtr<Event> prpEvent)
{
    ASSERT_WITH_SECURITY_IMPLICATION(NoEventDispatchAssertion::isEventAllowedInMainThread());
    if (!prpEvent)
        return true;

    ASSERT(origin);
    RefPtr<Node> node(origin);
    RefPtr<Event> event(prpEvent);
    RefPtr<FrameView> view = node->document().view();
    EventPath eventPath(*node, *event);

    if (EventTarget* relatedTarget = event->relatedTarget())
        eventPath.setRelatedTarget(*node, *relatedTarget);
#if ENABLE(TOUCH_EVENTS) && !PLATFORM(IOS)
    if (is<TouchEvent>(*event)) {
        if (!eventPath.updateTouchLists(downcast<TouchEvent>(*event)))
            return true;
    }
#endif

    ChildNodesLazySnapshot::takeChildNodesLazySnapshot();

    EventTarget* target = eventTargetRespectingTargetRules(*node);
    event->setTarget(target);
    if (!event->target())
        return true;

    ASSERT_WITH_SECURITY_IMPLICATION(NoEventDispatchAssertion::isEventAllowedInMainThread());

    WindowEventContext windowEventContext(node.get(), eventPath.lastContextIfExists());

    InputElementClickState clickHandlingState;
    if (is<HTMLInputElement>(*node))
        downcast<HTMLInputElement>(*node).willDispatchEvent(*event, clickHandlingState);

    if (!event->propagationStopped() && !eventPath.isEmpty())
        dispatchEventInDOM(*event, eventPath, windowEventContext);

    event->setTarget(eventTargetRespectingTargetRules(*node));
    event->setCurrentTarget(nullptr);
    event->setEventPhase(0);

    if (clickHandlingState.stateful)
        downcast<HTMLInputElement>(*node).didDispatchClickEvent(*event, clickHandlingState);

    // Call default event handlers. While the DOM does have a concept of preventing
    // default handling, the detail of which handlers are called is an internal
    // implementation detail and not part of the DOM.
    if (!event->defaultPrevented() && !event->defaultHandled())
        callDefaultEventHandlersInTheBubblingOrder(*event, eventPath);

    // Ensure that after event dispatch, the event's target object is the
    // outermost shadow DOM boundary.
    event->setTarget(windowEventContext.target());
    event->setCurrentTarget(0);

    return !event->defaultPrevented();
}

static inline bool shouldEventCrossShadowBoundary(Event& event, ShadowRoot& shadowRoot, EventTarget& target)
{
    Node* targetNode = target.toNode();
#if ENABLE(FULLSCREEN_API) && ENABLE(VIDEO)
    // Video-only full screen is a mode where we use the shadow DOM as an implementation
    // detail that should not be detectable by the web content.
    if (targetNode) {
        if (Element* element = targetNode->document().webkitCurrentFullScreenElement()) {
            // FIXME: We assume that if the full screen element is a media element that it's
            // the video-only full screen. Both here and elsewhere. But that is probably wrong.
            if (element->isMediaElement() && shadowRoot.hostElement() == element)
                return false;
        }
    }
#endif

    // WebKit never allowed selectstart event to cross the the shadow DOM boundary.
    // Changing this breaks existing sites.
    // See https://bugs.webkit.org/show_bug.cgi?id=52195 for details.
    const AtomicString& eventType = event.type();
    bool targetIsInShadowRoot = targetNode && &targetNode->treeScope().rootNode() == &shadowRoot;
    return !targetIsInShadowRoot
        || !(eventType == eventNames().abortEvent
            || eventType == eventNames().changeEvent
            || eventType == eventNames().errorEvent
            || eventType == eventNames().loadEvent
            || eventType == eventNames().resetEvent
            || eventType == eventNames().resizeEvent
            || eventType == eventNames().scrollEvent
            || eventType == eventNames().selectEvent
            || eventType == eventNames().selectstartEvent);
}

static Node* nodeOrHostIfPseudoElement(Node* node)
{
    return is<PseudoElement>(*node) ? downcast<PseudoElement>(*node).hostElement() : node;
}

EventPath::EventPath(Node& targetNode, Event& event)
{
    bool inDocument = targetNode.inDocument();
    bool isSVGElement = targetNode.isSVGElement();
    bool isMouseOrFocusEvent = event.isMouseEvent() || event.isWheelEvent() || event.isFocusEvent();
#if ENABLE(TOUCH_EVENTS) && !PLATFORM(IOS)
    bool isTouchEvent = event.isTouchEvent();
#endif
    EventTarget* target = nullptr;

    Node* node = nodeOrHostIfPseudoElement(&targetNode);
    while (node) {
        if (!target || !isSVGElement) // FIXME: This code doesn't make sense once we've climbed out of the SVG subtree in a HTML document.
            target = eventTargetRespectingTargetRules(*node);
        for (; node; node = node->parentNode()) {
            EventTarget* currentTarget = eventTargetRespectingTargetRules(*node);
            if (isMouseOrFocusEvent)
                m_path.append(std::make_unique<MouseOrFocusEventContext>(node, currentTarget, target));
#if ENABLE(TOUCH_EVENTS) && !PLATFORM(IOS)
            else if (isTouchEvent)
                m_path.append(std::make_unique<TouchEventContext>(node, currentTarget, target));
#endif
            else
                m_path.append(std::make_unique<EventContext>(node, currentTarget, target));
            if (!inDocument)
                return;
            if (is<ShadowRoot>(*node))
                break;
        }
        if (!node || !shouldEventCrossShadowBoundary(event, downcast<ShadowRoot>(*node), *target))
            return;
        node = downcast<ShadowRoot>(*node).hostElement();
    }
}

#if ENABLE(TOUCH_EVENTS) && !PLATFORM(IOS)
static void addRelatedNodeResolversForTouchList(Vector<EventRelatedNodeResolver, 16>& touchTargetResolvers, TouchList* touchList, TouchEventContext::TouchListType type)
{
    const size_t touchListSize = touchList->length();
    for (size_t i = 0; i < touchListSize; ++i)
        touchTargetResolvers.append(EventRelatedNodeResolver(*touchList->item(i), type));
}

bool EventPath::updateTouchLists(const TouchEvent& touchEvent)
{
    if (!touchEvent.touches() || !touchEvent.targetTouches() || !touchEvent.changedTouches())
        return false;
    
    Vector<EventRelatedNodeResolver, 16> touchTargetResolvers;
    const size_t touchNodeCount = touchEvent.touches()->length() + touchEvent.targetTouches()->length() + touchEvent.changedTouches()->length();
    touchTargetResolvers.reserveInitialCapacity(touchNodeCount);

    addRelatedNodeResolversForTouchList(touchTargetResolvers, touchEvent.touches(), TouchEventContext::Touches);
    addRelatedNodeResolversForTouchList(touchTargetResolvers, touchEvent.targetTouches(), TouchEventContext::TargetTouches);
    addRelatedNodeResolversForTouchList(touchTargetResolvers, touchEvent.changedTouches(), TouchEventContext::ChangedTouches);

    ASSERT(touchTargetResolvers.size() == touchNodeCount);
    size_t eventPathSize = m_path.size();
    for (size_t i = 0; i < eventPathSize; ++i) {
        TouchEventContext& context = toTouchEventContext(*m_path[i]);
        Node& nodeToMoveTo = *context.node();
        for (size_t resolverIndex = 0; resolverIndex < touchNodeCount; ++resolverIndex) {
            EventRelatedNodeResolver& currentResolver = touchTargetResolvers[resolverIndex];
            Node* nodeInCurrentTreeScope = currentResolver.moveToParentOrShadowHost(nodeToMoveTo);
            ASSERT(currentResolver.touch());
            context.touchList(currentResolver.touchListType())->append(currentResolver.touch()->cloneWithNewTarget(nodeInCurrentTreeScope));
        }
    }
    return true;
}
#endif

void EventPath::setRelatedTarget(Node& origin, EventTarget& relatedTarget)
{
    Node* relatedNode = relatedTarget.toNode();
    if (!relatedNode)
        return;

    EventRelatedNodeResolver resolver(*relatedNode);

    bool originIsRelatedTarget = &origin == relatedNode;
    Node& rootNodeInOriginTreeScope = origin.treeScope().rootNode();

    size_t eventPathSize = m_path.size();
    size_t i = 0;
    while (i < eventPathSize) {
        Node* contextNode = m_path[i]->node();
        Node* currentRelatedNode = resolver.moveToParentOrShadowHost(*contextNode);
        if (!originIsRelatedTarget && m_path[i]->target() == currentRelatedNode)
            break;
        toMouseOrFocusEventContext(*m_path[i]).setRelatedTarget(currentRelatedNode);
        i++;
        if (originIsRelatedTarget && &rootNodeInOriginTreeScope == contextNode)
            break;
    }
    m_path.shrink(i);
}

bool EventPath::hasEventListeners(const AtomicString& eventType) const
{
    for (size_t i = 0; i < m_path.size(); i++) {
        if (m_path[i]->node()->hasEventListeners(eventType))
            return true;
    }

    return false;
}

}

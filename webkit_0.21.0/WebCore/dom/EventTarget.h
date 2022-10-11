/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *           (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef EventTarget_h
#define EventTarget_h

#include "EventListenerMap.h"
#include "EventNames.h"
#include "EventTargetInterfaces.h"
#include <memory>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>

namespace WTF {
class AtomicString;
}

namespace WebCore {

class AudioNode;
class AudioContext;
class AudioTrackList;
class DedicatedWorkerGlobalScope;
class DOMApplicationCache;
class DOMWindow;
class Event;
class EventListener;
class EventSource;
class FileReader;
class FileWriter;
class IDBDatabase;
class IDBRequest;
class IDBTransaction;
class ScriptProcessorNode;
class MediaController;
class MediaStream;
class MessagePort;
class Node;
class Notification;
class ScriptExecutionContext;
class TextTrack;
class TextTrackCue;
class VideoTrackList;
class WebSocket;
class WebKitNamedFlow;
class Worker;
class XMLHttpRequest;
class XMLHttpRequestUpload;

typedef int ExceptionCode;

struct FiringEventIterator {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
public:
#endif
    FiringEventIterator(const AtomicString& eventType, size_t& iterator, size_t& size)
        : eventType(eventType)
        , iterator(iterator)
        , size(size)
    {
    }

    const AtomicString& eventType;
    size_t& iterator;
    size_t& size;
};
typedef Vector<FiringEventIterator, 1> FiringEventIteratorVector;

struct EventTargetData {
    WTF_MAKE_NONCOPYABLE(EventTargetData); WTF_MAKE_FAST_ALLOCATED;
public:
    EventTargetData();
    ~EventTargetData();

    EventListenerMap eventListenerMap;
    std::unique_ptr<FiringEventIteratorVector> firingEventIterators;
};

enum EventTargetInterface {

#define DOM_EVENT_INTERFACE_DECLARE(name) name##EventTargetInterfaceType,
DOM_EVENT_TARGET_INTERFACES_FOR_EACH(DOM_EVENT_INTERFACE_DECLARE)
#undef DOM_EVENT_INTERFACE_DECLARE

};

class EventTarget {
public:
    void ref() { refEventTarget(); }
    void deref() { derefEventTarget(); }

    virtual EventTargetInterface eventTargetInterface() const = 0;
    virtual ScriptExecutionContext* scriptExecutionContext() const = 0;

    virtual Node* toNode();
    virtual DOMWindow* toDOMWindow();
    virtual bool isMessagePort() const;

    virtual bool addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture);
    virtual bool removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture);
    virtual void removeAllEventListeners();
    virtual bool dispatchEvent(PassRefPtr<Event>);
    bool dispatchEvent(PassRefPtr<Event>, ExceptionCode&); // DOM API
    virtual void uncaughtExceptionInEventHandler();

    // Used for legacy "onEvent" attribute APIs.
    bool setAttributeEventListener(const AtomicString& eventType, PassRefPtr<EventListener>);
    bool clearAttributeEventListener(const AtomicString& eventType);
    EventListener* getAttributeEventListener(const AtomicString& eventType);

    bool hasEventListeners() const;
    bool hasEventListeners(const AtomicString& eventType);
    bool hasCapturingEventListeners(const AtomicString& eventType);
    const EventListenerVector& getEventListeners(const AtomicString& eventType);

    bool fireEventListeners(Event*);
    bool isFiringEventListeners();

    void visitJSEventListeners(JSC::SlotVisitor&);
    void invalidateJSEventListeners(JSC::JSObject*);

protected:
    virtual ~EventTarget();
    
    virtual EventTargetData* eventTargetData() = 0;
    virtual EventTargetData& ensureEventTargetData() = 0;

private:
    virtual void refEventTarget() = 0;
    virtual void derefEventTarget() = 0;
    
    void fireEventListeners(Event*, EventTargetData*, EventListenerVector&);

    friend class EventListenerIterator;
};

class EventTargetWithInlineData : public EventTarget {
protected:
    virtual EventTargetData* eventTargetData() override final { return &m_eventTargetData; }
    virtual EventTargetData& ensureEventTargetData() override final { return m_eventTargetData; }
private:
    EventTargetData m_eventTargetData;
};

inline void EventTarget::visitJSEventListeners(JSC::SlotVisitor& visitor)
{
    EventListenerIterator iterator(this);
    while (EventListener* listener = iterator.nextListener())
        listener->visitJSFunction(visitor);
}

inline bool EventTarget::isFiringEventListeners()
{
    EventTargetData* d = eventTargetData();
    if (!d)
        return false;
    return d->firingEventIterators && !d->firingEventIterators->isEmpty();
}

inline bool EventTarget::hasEventListeners() const
{
    EventTargetData* d = const_cast<EventTarget*>(this)->eventTargetData();
    if (!d)
        return false;
    return !d->eventListenerMap.isEmpty();
}

inline bool EventTarget::hasEventListeners(const AtomicString& eventType)
{
    EventTargetData* d = eventTargetData();
    if (!d)
        return false;
    return d->eventListenerMap.contains(eventType);
}

inline bool EventTarget::hasCapturingEventListeners(const AtomicString& eventType)
{
    EventTargetData* d = eventTargetData();
    if (!d)
        return false;
    return d->eventListenerMap.containsCapturing(eventType);
}

} // namespace WebCore

#endif // EventTarget_h

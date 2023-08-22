/*
 * Copyright (C) 2010 Google Inc. All Rights Reserved.
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
 *
 */

#ifndef WorkerEventQueue_h
#define WorkerEventQueue_h

#include "EventQueue.h"
#include <wtf/HashMap.h>

namespace WebCore {

class ScriptExecutionContext;

class WorkerEventQueue final : public EventQueue {
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    explicit WorkerEventQueue(ScriptExecutionContext&);
    virtual ~WorkerEventQueue();

    virtual bool enqueueEvent(PassRefPtr<Event>) override;
    virtual bool cancelEvent(Event&) override;
    virtual void close() override;

#if PLATFORM(WKC)
    class EventDispatcher;
#endif
private:
    ScriptExecutionContext& m_scriptExecutionContext;
    bool m_isClosed;

#if !PLATFORM(WKC)
    class EventDispatcher;
#endif
    typedef HashMap<RefPtr<Event>, EventDispatcher*> EventDispatcherMap;
    EventDispatcherMap m_eventDispatcherMap;
};

}

#endif // WorkerEventQueue_h

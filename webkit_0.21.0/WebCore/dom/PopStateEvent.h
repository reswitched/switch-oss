/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE, INC. ``AS IS'' AND ANY
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

#ifndef PopStateEvent_h
#define PopStateEvent_h

#include "Event.h"
#include "SerializedScriptValue.h"
#include <bindings/ScriptValue.h>

namespace WebCore {

struct PopStateEventInit : public EventInit {
    PopStateEventInit();

    Deprecated::ScriptValue state;
};

class History;
class SerializedScriptValue;

class PopStateEvent final : public Event {
public:
    virtual ~PopStateEvent();
    static Ref<PopStateEvent> create();
    static Ref<PopStateEvent> create(PassRefPtr<SerializedScriptValue>, PassRefPtr<History>);
    static Ref<PopStateEvent> create(const AtomicString&, const PopStateEventInit&);

    PassRefPtr<SerializedScriptValue> serializedState() const { ASSERT(m_serializedState); return m_serializedState; }
    
    RefPtr<SerializedScriptValue> trySerializeState(JSC::ExecState*);
    
    const Deprecated::ScriptValue& state() const { return m_state; }
    History* history() const { return m_history.get(); }

    virtual EventInterface eventInterface() const override;

private:
    PopStateEvent();
    PopStateEvent(const AtomicString&, const PopStateEventInit&);
    explicit PopStateEvent(PassRefPtr<SerializedScriptValue>, PassRefPtr<History>);

    Deprecated::ScriptValue m_state;
    RefPtr<SerializedScriptValue> m_serializedState;
    bool m_triedToSerialize { false };
    RefPtr<History> m_history;
};

} // namespace WebCore

#endif // PopStateEvent_h

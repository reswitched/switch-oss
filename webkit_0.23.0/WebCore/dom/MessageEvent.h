/*
 * Copyright (C) 2007 Henry Mason (hmason@mac.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef MessageEvent_h
#define MessageEvent_h

#include "Blob.h"
#include "DOMWindow.h"
#include "Event.h"
#include "MessagePort.h"
#include "SerializedScriptValue.h"
#include <bindings/ScriptValue.h>
#include <memory>
#include <runtime/ArrayBuffer.h>

namespace WebCore {

class EventTarget;

struct MessageEventInit : public EventInit {
    MessageEventInit();

    Deprecated::ScriptValue data;
    String origin;
    String lastEventId;
    RefPtr<EventTarget> source;
    MessagePortArray ports;
};

class MessageEvent final : public Event {
public:
    static Ref<MessageEvent> create()
    {
        return adoptRef(*new MessageEvent);
    }
    static Ref<MessageEvent> create(std::unique_ptr<MessagePortArray> ports, const Deprecated::ScriptValue& data = Deprecated::ScriptValue(), const String& origin = String(), const String& lastEventId = String(), PassRefPtr<EventTarget> source = 0)
    {
        return adoptRef(*new MessageEvent(data, origin, lastEventId, source, WTF::move(ports)));
    }
    static Ref<MessageEvent> create(std::unique_ptr<MessagePortArray> ports, PassRefPtr<SerializedScriptValue> data, const String& origin = String(), const String& lastEventId = String(), PassRefPtr<EventTarget> source = 0)
    {
        return adoptRef(*new MessageEvent(data, origin, lastEventId, source, WTF::move(ports)));
    }
    static Ref<MessageEvent> create(const String& data, const String& origin = String())
    {
        return adoptRef(*new MessageEvent(data, origin));
    }
    static Ref<MessageEvent> create(PassRefPtr<Blob> data, const String& origin = String())
    {
        return adoptRef(*new MessageEvent(data, origin));
    }
    static Ref<MessageEvent> create(PassRefPtr<ArrayBuffer> data, const String& origin = String())
    {
        return adoptRef(*new MessageEvent(data, origin));
    }
    static Ref<MessageEvent> create(const AtomicString& type, const MessageEventInit& initializer)
    {
        return adoptRef(*new MessageEvent(type, initializer));
    }
    virtual ~MessageEvent();

    void initMessageEvent(const AtomicString& type, bool canBubble, bool cancelable, const Deprecated::ScriptValue& data, const String& origin, const String& lastEventId, DOMWindow* source, std::unique_ptr<MessagePortArray>);
    void initMessageEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<SerializedScriptValue> data, const String& origin, const String& lastEventId, DOMWindow* source, std::unique_ptr<MessagePortArray>);

    const String& origin() const { return m_origin; }
    const String& lastEventId() const { return m_lastEventId; }
    EventTarget* source() const { return m_source.get(); }
    MessagePortArray ports() const { return m_ports ? *m_ports : MessagePortArray(); }

    // FIXME: Remove this when we have custom ObjC binding support.
    SerializedScriptValue* data() const;

    // Needed for Objective-C bindings (see bug 28774).
    MessagePort* messagePort();
    void initMessageEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<SerializedScriptValue> data, const String& origin, const String& lastEventId, DOMWindow* source, MessagePort*);

    virtual EventInterface eventInterface() const override;

    enum DataType {
        DataTypeScriptValue,
        DataTypeSerializedScriptValue,
        DataTypeString,
        DataTypeBlob,
        DataTypeArrayBuffer
    };
    DataType dataType() const { return m_dataType; }
    const Deprecated::ScriptValue& dataAsScriptValue() const { ASSERT(m_dataType == DataTypeScriptValue); return m_dataAsScriptValue; }
    PassRefPtr<SerializedScriptValue> dataAsSerializedScriptValue() const { ASSERT(m_dataType == DataTypeSerializedScriptValue); return m_dataAsSerializedScriptValue; }
    String dataAsString() const { ASSERT(m_dataType == DataTypeString); return m_dataAsString; }
    Blob* dataAsBlob() const { ASSERT(m_dataType == DataTypeBlob); return m_dataAsBlob.get(); }
    ArrayBuffer* dataAsArrayBuffer() const { ASSERT(m_dataType == DataTypeArrayBuffer); return m_dataAsArrayBuffer.get(); }

    RefPtr<SerializedScriptValue> trySerializeData(JSC::ExecState*);
    
private:
    MessageEvent();
    MessageEvent(const AtomicString&, const MessageEventInit&);
    MessageEvent(const Deprecated::ScriptValue& data, const String& origin, const String& lastEventId, PassRefPtr<EventTarget> source, std::unique_ptr<MessagePortArray>);
    MessageEvent(PassRefPtr<SerializedScriptValue> data, const String& origin, const String& lastEventId, PassRefPtr<EventTarget> source, std::unique_ptr<MessagePortArray>);

    explicit MessageEvent(const String& data, const String& origin);
    explicit MessageEvent(PassRefPtr<Blob> data, const String& origin);
    explicit MessageEvent(PassRefPtr<ArrayBuffer> data, const String& origin);

    DataType m_dataType;
    Deprecated::ScriptValue m_dataAsScriptValue;
    RefPtr<SerializedScriptValue> m_dataAsSerializedScriptValue;
    bool m_triedToSerialize { false };
    String m_dataAsString;
    RefPtr<Blob> m_dataAsBlob;
    RefPtr<ArrayBuffer> m_dataAsArrayBuffer;
    String m_origin;
    String m_lastEventId;
    RefPtr<EventTarget> m_source;
    std::unique_ptr<MessagePortArray> m_ports;
};

} // namespace WebCore

#endif // MessageEvent_h

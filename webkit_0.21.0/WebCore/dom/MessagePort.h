/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef MessagePort_h
#define MessagePort_h

#include "EventListener.h"
#include "EventTarget.h"
#include "MessagePortChannel.h"
#include <memory>
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

    class Event;
    class Frame;
    class MessagePort;
    class ScriptExecutionContext;

    // The overwhelmingly common case is sending a single port, so handle that efficiently with an inline buffer of size 1.
    typedef Vector<RefPtr<MessagePort>, 1> MessagePortArray;

    class MessagePort final : public RefCounted<MessagePort>, public EventTargetWithInlineData {
    public:
        static Ref<MessagePort> create(ScriptExecutionContext& scriptExecutionContext) { return adoptRef(*new MessagePort(scriptExecutionContext)); }
        virtual ~MessagePort();

        void postMessage(PassRefPtr<SerializedScriptValue> message, const MessagePortArray*, ExceptionCode&);
        // Needed for Objective-C bindings (see bug 28774).
        void postMessage(PassRefPtr<SerializedScriptValue> message, MessagePort*, ExceptionCode&);

        void start();
        void close();

        void entangle(std::unique_ptr<MessagePortChannel>);
        std::unique_ptr<MessagePortChannel> disentangle();

        // Returns 0 if there is an exception, or if the passed-in array is 0/empty.
        static std::unique_ptr<MessagePortChannelArray> disentanglePorts(const MessagePortArray*, ExceptionCode&);

        // Returns 0 if the passed array is 0/empty.
        static std::unique_ptr<MessagePortArray> entanglePorts(ScriptExecutionContext&, std::unique_ptr<MessagePortChannelArray>);

        void messageAvailable();
        bool started() const { return m_started; }

        void contextDestroyed();

        virtual EventTargetInterface eventTargetInterface() const override { return MessagePortEventTargetInterfaceType; }
        virtual ScriptExecutionContext* scriptExecutionContext() const override { return m_scriptExecutionContext; }

        void dispatchMessages();

        using RefCounted<MessagePort>::ref;
        using RefCounted<MessagePort>::deref;

        bool hasPendingActivity();

        // Returns null if there is no entangled port, or if the entangled port is run by a different thread.
        // This is used solely to enable a GC optimization. Some platforms may not be able to determine ownership
        // of the remote port (since it may live cross-process) - those platforms may always return null.
        MessagePort* locallyEntangledPort();

        // A port starts out its life entangled, and remains entangled until it is closed or is cloned.
        bool isEntangled() { return !m_closed && !isNeutered(); }

        // A port gets neutered when it is transferred to a new owner via postMessage().
        bool isNeutered() { return !m_entangledChannel; }

        bool addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture) override;

    private:
        explicit MessagePort(ScriptExecutionContext&);

        virtual void refEventTarget() override { ref(); }
        virtual void derefEventTarget() override { deref(); }
        virtual bool isMessagePort() const override { return true; }

        std::unique_ptr<MessagePortChannel> m_entangledChannel;

        bool m_started;
        bool m_closed;

        ScriptExecutionContext* m_scriptExecutionContext;
    };

} // namespace WebCore

#endif // MessagePort_h

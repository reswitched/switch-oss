/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef WorkerGlobalScope_h
#define WorkerGlobalScope_h

#include "ContentSecurityPolicy.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "ScriptExecutionContext.h"
#include "WorkerEventQueue.h"
#include "WorkerScriptController.h"
#include <memory>
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/TypeCasts.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

    class Blob;
    class ScheduledAction;
    class WorkerInspectorController;
    class WorkerLocation;
    class WorkerNavigator;
    class WorkerThread;

    class WorkerGlobalScope : public RefCounted<WorkerGlobalScope>, public ScriptExecutionContext, public EventTargetWithInlineData {
    public:
        virtual ~WorkerGlobalScope();

        virtual bool isWorkerGlobalScope() const override { return true; }

        virtual ScriptExecutionContext* scriptExecutionContext() const override final { return const_cast<WorkerGlobalScope*>(this); }

        virtual bool isDedicatedWorkerGlobalScope() const { return false; }

        virtual const URL& url() const override final { return m_url; }
        virtual URL completeURL(const String&) const override final;

        virtual String userAgent(const URL&) const override;

        virtual void disableEval(const String& errorMessage) override;

        WorkerScriptController* script() { return m_script.get(); }
        void clearScript() { m_script = nullptr; }

        WorkerThread& thread() const { return m_thread; }

        using ScriptExecutionContext::hasPendingActivity;

#if !PLATFORM(WKC) || COMPILER(CLANG)
        virtual void postTask(Task) override; // Executes the task on context's thread asynchronously.
#else
        virtual void postTask(Task&&) override; // Executes the task on context's thread asynchronously.
#endif

        // WorkerGlobalScope
        WorkerGlobalScope* self() { return this; }
        WorkerLocation* location() const;
        void close();

        // WorkerUtils
        virtual void importScripts(const Vector<String>& urls, ExceptionCode&);
        WorkerNavigator* navigator() const;

        // Timers
        int setTimeout(std::unique_ptr<ScheduledAction>, int timeout);
        void clearTimeout(int timeoutId);
        int setInterval(std::unique_ptr<ScheduledAction>, int timeout);
        void clearInterval(int timeoutId);

        virtual bool isContextThread() const override;
        virtual bool isJSExecutionForbidden() const override;

        WorkerInspectorController& workerInspectorController() { return *m_workerInspectorController; }

        // These methods are used for GC marking. See JSWorkerGlobalScope::visitChildrenVirtual(SlotVisitor&) in
        // JSWorkerGlobalScopeCustom.cpp.
        WorkerNavigator* optionalNavigator() const { return m_navigator.get(); }
        WorkerLocation* optionalLocation() const { return m_location.get(); }

        using RefCounted<WorkerGlobalScope>::ref;
        using RefCounted<WorkerGlobalScope>::deref;

        bool isClosing() { return m_closing; }

        // An observer interface to be notified when the worker thread is getting stopped.
        class Observer {
            WTF_MAKE_NONCOPYABLE(Observer);
#if PLATFORM(WKC)
            WTF_MAKE_FAST_ALLOCATED;
#endif
        public:
            Observer(WorkerGlobalScope*);
            virtual ~Observer();
            virtual void notifyStop() = 0;
            void stopObserving();
        private:
            WorkerGlobalScope* m_context;
        };
        friend class Observer;
        void registerObserver(Observer*);
        void unregisterObserver(Observer*);
        void notifyObserversOfStop();

        virtual SecurityOrigin* topOrigin() const override { return m_topOrigin.get(); }

        virtual void addConsoleMessage(MessageSource, MessageLevel, const String& message, unsigned long requestIdentifier = 0) override;

#if ENABLE(SUBTLE_CRYPTO)
        virtual bool wrapCryptoKey(const Vector<uint8_t>& key, Vector<uint8_t>& wrappedKey) override;
        virtual bool unwrapCryptoKey(const Vector<uint8_t>& wrappedKey, Vector<uint8_t>& key) override;
#endif

    protected:
        WorkerGlobalScope(const URL&, const String& userAgent, WorkerThread&, PassRefPtr<SecurityOrigin> topOrigin);
        void applyContentSecurityPolicyFromString(const String& contentSecurityPolicy, ContentSecurityPolicy::HeaderType);

        virtual void logExceptionToConsole(const String& errorMessage, const String& sourceURL, int lineNumber, int columnNumber, RefPtr<Inspector::ScriptCallStack>&&) override;
        void addMessageToWorkerConsole(MessageSource, MessageLevel, const String& message, const String& sourceURL, unsigned lineNumber, unsigned columnNumber, RefPtr<Inspector::ScriptCallStack>&&, JSC::ExecState* = 0, unsigned long requestIdentifier = 0);

    private:
        virtual void refScriptExecutionContext() override { ref(); }
        virtual void derefScriptExecutionContext() override { deref(); }

        virtual void refEventTarget() override final { ref(); }
        virtual void derefEventTarget() override final { deref(); }

        virtual void addMessage(MessageSource, MessageLevel, const String& message, const String& sourceURL, unsigned lineNumber, unsigned columnNumber, RefPtr<Inspector::ScriptCallStack>&&, JSC::ExecState* = 0, unsigned long requestIdentifier = 0) override;

        virtual EventTarget* errorEventTarget() override;

        virtual WorkerEventQueue& eventQueue() const override final;

        URL m_url;
        String m_userAgent;

        mutable RefPtr<WorkerLocation> m_location;
        mutable RefPtr<WorkerNavigator> m_navigator;

        std::unique_ptr<WorkerScriptController> m_script;
        WorkerThread& m_thread;

        const std::unique_ptr<WorkerInspectorController> m_workerInspectorController;
        bool m_closing;

        HashSet<Observer*> m_workerObservers;

        mutable WorkerEventQueue m_eventQueue;

        RefPtr<SecurityOrigin> m_topOrigin;
    };

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::WorkerGlobalScope)
    static bool isType(const WebCore::ScriptExecutionContext& context) { return context.isWorkerGlobalScope(); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // WorkerGlobalScope_h

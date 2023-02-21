/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSMainThreadExecState.h"
#include "MutationObserver.h"

#if ENABLE(INDEXED_DATABASE)
#include "IDBPendingTransactionMonitor.h"
#endif

namespace WebCore {

#if !PLATFORM(WKC)
JSC::ExecState* JSMainThreadExecState::s_mainThreadState = 0;
#else
WKC_DEFINE_GLOBAL_CLASS_OBJ(JSC::ExecState*, JSMainThreadExecState, s_mainThreadState, 0);
#endif

void JSMainThreadExecState::didLeaveScriptContext()
{
#if ENABLE(INDEXED_DATABASE)
    // Indexed DB requires that transactions are created with an internal |active| flag
    // set to true, but the flag becomes false when control returns to the event loop.
    IDBPendingTransactionMonitor::deactivateNewTransactions();
#endif

    MutationObserver::deliverAllMutations();
}

JSC::JSValue functionCallHandlerFromAnyThread(JSC::ExecState* exec, JSC::JSValue functionObject, JSC::CallType callType, const JSC::CallData& callData, JSC::JSValue thisValue, const JSC::ArgList& args, NakedPtr<JSC::Exception>& returnedException)
{
    if (isMainThread())
        return JSMainThreadExecState::call(exec, functionObject, callType, callData, thisValue, args, returnedException);
    return JSC::call(exec, functionObject, callType, callData, thisValue, args, returnedException);
}

JSC::JSValue evaluateHandlerFromAnyThread(JSC::ExecState* exec, const JSC::SourceCode& source, JSC::JSValue thisValue, NakedPtr<JSC::Exception>& returnedException)
{
    if (isMainThread())
        return JSMainThreadExecState::evaluate(exec, source, thisValue, returnedException);
    return JSC::evaluate(exec, source, thisValue, returnedException);
}

} // namespace WebCore

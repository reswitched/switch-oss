/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
#include "ExceptionFuzz.h"

#include "Error.h"
#include "JSCInlines.h"
#include "TestRunnerUtils.h"

namespace JSC {

#if !PLATFORM(WKC)
static unsigned s_numberOfExceptionFuzzChecks;
#else
WKC_DEFINE_GLOBAL_UINT(s_numberOfExceptionFuzzChecks, 0);
#endif
unsigned numberOfExceptionFuzzChecks() { return s_numberOfExceptionFuzzChecks; }

// Call this only if you know that exception fuzzing is enabled.
void doExceptionFuzzing(ExecState* exec, const char* where, void* returnPC)
{
    ASSERT(Options::useExceptionFuzz());

    DeferGCForAWhile deferGC(exec->vm().heap);
    
    s_numberOfExceptionFuzzChecks++;
    
    unsigned fireTarget = Options::fireExceptionFuzzAt();
    if (fireTarget == s_numberOfExceptionFuzzChecks) {
        printf("JSC EXCEPTION FUZZ: Throwing fuzz exception with call frame %p, seen in %s and return address %p.\n", exec, where, returnPC);
        exec->vm().throwException(
            exec, createError(exec, ASCIILiteral("Exception Fuzz")));
    }
}

} // namespace JSC



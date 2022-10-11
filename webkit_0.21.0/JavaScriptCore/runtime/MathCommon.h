/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#ifndef MathCommon_h
#define MathCommon_h

#include "JITOperations.h"

#ifndef JIT_OPERATION
#define JIT_OPERATION
#endif

namespace JSC {
double JIT_OPERATION operationMathPow(double x, double y) WTF_INTERNAL;

inline int clz32(uint32_t number)
{
#if COMPILER(GCC) || COMPILER(CLANG)
    int zeroCount = 32;
    if (number)
        zeroCount = __builtin_clz(number);
    return zeroCount;
#else
    int zeroCount = 0;
    for (int i = 31; i >= 0; i--) {
        if (!(number >> i))
            zeroCount++;
        else
            break;
    }
    return zeroCount;
#endif
}

extern "C" {
double JIT_OPERATION jsRound(double value) REFERENCED_FROM_ASM WTF_INTERNAL;
}

}

#endif // MathCommon_h

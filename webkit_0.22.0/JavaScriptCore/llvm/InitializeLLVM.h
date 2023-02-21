/*
 * Copyright (C) 2013-2015 Apple Inc. All rights reserved.
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

#ifndef InitializeLLVM_h
#define InitializeLLVM_h

#if HAVE(LLVM)

#include "LLVMAPI.h"
#include <string>
#include <wtf/text/CString.h>

namespace JSC {

typedef void (*LoggerFunction)(const char*, ...) WTF_ATTRIBUTE_PRINTF(1, 2);
typedef LLVMAPI* (*LLVMInitializerFunction)(LoggerFunction, bool* enableFastISel);

LLVMInitializerFunction getLLVMInitializerFunction(bool verbose);

extern const CString* llvmBitcodeLibraryForInliningPath;

// You must call this before using JSC::llvm. It's safe to call this multiple times.
// Returns true if we successfully loaded LLVM. Returns false if we didn't.
bool initializeLLVM();

} // namespace JSC

#endif // HAVE(LLVM)

#endif // InitializeLLVM_h


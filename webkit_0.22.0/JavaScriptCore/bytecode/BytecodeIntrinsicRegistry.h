/*
 * Copyright (C) 2015 Yusuke Suzuki <utatane.tea@gmail.com>.
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

#ifndef BytecodeIntrinsicRegistry_h
#define BytecodeIntrinsicRegistry_h

#include "Identifier.h"
#include <wtf/HashTable.h>
#include <wtf/Noncopyable.h>

namespace JSC {

class CommonIdentifiers;
class BytecodeGenerator;
class BytecodeIntrinsicNode;
class RegisterID;
class Identifier;

#define JSC_COMMON_BYTECODE_INTRINSIC_FUNCTIONS_EACH_NAME(macro) \
    macro(isJSArray) \
    macro(tailCallForwardArguments) \
    macro(putByValDirect) \
    macro(toString)

#define JSC_COMMON_BYTECODE_INTRINSIC_CONSTANTS_EACH_NAME(macro) \
    macro(undefined) \
    macro(arrayIterationKindKey) \
    macro(arrayIterationKindValue) \
    macro(arrayIterationKindKeyValue) \
    macro(MAX_SAFE_INTEGER) \
    macro(symbolIterator)


class BytecodeIntrinsicRegistry {
    WTF_MAKE_NONCOPYABLE(BytecodeIntrinsicRegistry);
#if PLATFORM(WKC)
    WTF_MAKE_FAST_ALLOCATED;
#endif
public:
    explicit BytecodeIntrinsicRegistry(VM&);

#if !COMPILER(MSVC)
    typedef RegisterID* (BytecodeIntrinsicNode::* EmitterType)(BytecodeGenerator&, RegisterID*);
#else
    typedef RegisterID* (*EmitterType)(BytecodeIntrinsicNode*, BytecodeGenerator&, RegisterID*);
#endif

    EmitterType lookup(const Identifier&) const;

#define JSC_DECLARE_BYTECODE_INTRINSIC_CONSTANT_GENERATORS(name) JSValue name##Value(BytecodeGenerator&);
    JSC_COMMON_BYTECODE_INTRINSIC_CONSTANTS_EACH_NAME(JSC_DECLARE_BYTECODE_INTRINSIC_CONSTANT_GENERATORS)
#undef JSC_DECLARE_BYTECODE_INTRINSIC_CONSTANT_GENERATORS

private:
    VM& m_vm;
    HashMap<RefPtr<UniquedStringImpl>, EmitterType, IdentifierRepHash> m_bytecodeIntrinsicMap;

#define JSC_DECLARE_BYTECODE_INTRINSIC_CONSTANT_GENERATORS(name) Strong<Unknown> m_##name;
    JSC_COMMON_BYTECODE_INTRINSIC_CONSTANTS_EACH_NAME(JSC_DECLARE_BYTECODE_INTRINSIC_CONSTANT_GENERATORS)
#undef JSC_DECLARE_BYTECODE_INTRINSIC_CONSTANT_GENERATORS
};

} // namespace JSC

#endif // BytecodeIntrinsicRegistry_h

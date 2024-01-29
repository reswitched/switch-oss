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

#include "config.h"
#include "BytecodeIntrinsicRegistry.h"
#include "BytecodeGenerator.h"
#include "JSArrayIterator.h"
#include "JSCJSValueInlines.h"
#include "JSPromise.h"
#include "Nodes.h"
#include "StrongInlines.h"

namespace JSC {

#if !COMPILER(MSVC)
#define INITIALIZE_BYTECODE_INTRINSIC_NAMES_TO_SET(name) m_bytecodeIntrinsicMap.add(vm.propertyNames->name##PrivateName.impl(), &BytecodeIntrinsicNode::emit_intrinsic_##name);
#else
#define INITIALIZE_BYTECODE_INTRINSIC_NAMES_TO_SET(name) m_bytecodeIntrinsicMap.add(vm.propertyNames->name##PrivateName.impl(), &BytecodeIntrinsicNode::emit_intrinsic_##name##Proc);
#endif

BytecodeIntrinsicRegistry::BytecodeIntrinsicRegistry(VM& vm)
    : m_vm(vm)
    , m_bytecodeIntrinsicMap()
{
    JSC_COMMON_BYTECODE_INTRINSIC_FUNCTIONS_EACH_NAME(INITIALIZE_BYTECODE_INTRINSIC_NAMES_TO_SET)
    JSC_COMMON_BYTECODE_INTRINSIC_CONSTANTS_EACH_NAME(INITIALIZE_BYTECODE_INTRINSIC_NAMES_TO_SET)

    m_undefined.set(m_vm, jsUndefined());
    m_arrayIterationKindKey.set(m_vm, jsNumber(ArrayIterateKey));
    m_arrayIterationKindValue.set(m_vm, jsNumber(ArrayIterateValue));
    m_arrayIterationKindKeyValue.set(m_vm, jsNumber(ArrayIterateKeyValue));
    m_MAX_SAFE_INTEGER.set(m_vm, jsDoubleNumber(9007199254740991.0)); // 2 ^ 53 - 1
    m_symbolIterator.set(m_vm, Symbol::create(m_vm, static_cast<SymbolImpl&>(*m_vm.propertyNames->iteratorSymbol.impl())));
}

BytecodeIntrinsicNode::EmitterType BytecodeIntrinsicRegistry::lookup(const Identifier& ident) const
{
    if (!m_vm.propertyNames->isPrivateName(ident))
        return nullptr;
    auto iterator = m_bytecodeIntrinsicMap.find(ident.impl());
    if (iterator == m_bytecodeIntrinsicMap.end())
        return nullptr;
    return iterator->value;
}

#define JSC_DECLARE_BYTECODE_INTRINSIC_CONSTANT_GENERATORS(name) \
    JSValue BytecodeIntrinsicRegistry::name##Value(BytecodeGenerator&) \
    { \
        return m_##name.get(); \
    }
    JSC_COMMON_BYTECODE_INTRINSIC_CONSTANTS_EACH_NAME(JSC_DECLARE_BYTECODE_INTRINSIC_CONSTANT_GENERATORS)
#undef JSC_DECLARE_BYTECODE_INTRINSIC_CONSTANT_GENERATORS

} // namespace JSC


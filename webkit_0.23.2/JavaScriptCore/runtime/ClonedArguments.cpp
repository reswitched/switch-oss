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

#include "config.h"
#include "ClonedArguments.h"

#include "GetterSetter.h"
#include "InlineCallFrame.h"
#include "JSCInlines.h"

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(ClonedArguments);

const ClassInfo ClonedArguments::s_info = { "Arguments", &Base::s_info, 0, CREATE_METHOD_TABLE(ClonedArguments) };

ClonedArguments::ClonedArguments(VM& vm, Structure* structure)
    : Base(vm, structure, nullptr)
{
}

ClonedArguments* ClonedArguments::createEmpty(
    VM& vm, Structure* structure, JSFunction* callee)
{
    ClonedArguments* result =
        new (NotNull, allocateCell<ClonedArguments>(vm.heap))
        ClonedArguments(vm, structure);
    result->finishCreation(vm);
    result->m_callee.set(vm, result, callee);
    return result;
}

ClonedArguments* ClonedArguments::createEmpty(ExecState* exec, JSFunction* callee)
{
    // NB. Some clients might expect that the global object of of this object is the global object
    // of the callee. We don't do this for now, but maybe we should.
    return createEmpty(
        exec->vm(), exec->lexicalGlobalObject()->outOfBandArgumentsStructure(), callee);
}

ClonedArguments* ClonedArguments::createWithInlineFrame(ExecState* myFrame, ExecState* targetFrame, InlineCallFrame* inlineCallFrame, ArgumentsMode mode)
{
    VM& vm = myFrame->vm();
    
    JSFunction* callee;
    
    if (inlineCallFrame)
        callee = jsCast<JSFunction*>(inlineCallFrame->calleeRecovery.recover(targetFrame));
    else
        callee = jsCast<JSFunction*>(targetFrame->callee());

    ClonedArguments* result = createEmpty(myFrame, callee);
    
    unsigned length = 0; // Initialize because VC needs it.
    switch (mode) {
    case ArgumentsMode::Cloned: {
        if (inlineCallFrame) {
            if (inlineCallFrame->argumentCountRegister.isValid())
                length = targetFrame->r(inlineCallFrame->argumentCountRegister).unboxedInt32();
            else
                length = inlineCallFrame->arguments.size();
            length--;
            
            for (unsigned i = length; i--;)
                result->putDirectIndex(myFrame, i, inlineCallFrame->arguments[i + 1].recover(targetFrame));
        } else {
            length = targetFrame->argumentCount();
            
            for (unsigned i = length; i--;)
                result->putDirectIndex(myFrame, i, targetFrame->uncheckedArgument(i));
        }
        break;
    }
        
    case ArgumentsMode::FakeValues: {
        length = 0;
        break;
    } }
    
    result->putDirect(vm, vm.propertyNames->length, jsNumber(length));
    
    return result;
}

ClonedArguments* ClonedArguments::createWithMachineFrame(ExecState* myFrame, ExecState* targetFrame, ArgumentsMode mode)
{
    return createWithInlineFrame(myFrame, targetFrame, nullptr, mode);
}

ClonedArguments* ClonedArguments::createByCopyingFrom(
    ExecState* exec, Structure* structure, Register* argumentStart, unsigned length,
    JSFunction* callee)
{
    VM& vm = exec->vm();
    ClonedArguments* result = createEmpty(vm, structure, callee);
    
    for (unsigned i = length; i--;)
        result->putDirectIndex(exec, i, argumentStart[i].jsValue());
    
    result->putDirect(vm, vm.propertyNames->length, jsNumber(length));
    return result;
}

Structure* ClonedArguments::createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
{
    return Structure::create(vm, globalObject, prototype, TypeInfo(ClonedArgumentsType, StructureFlags), info());
}

bool ClonedArguments::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName ident, PropertySlot& slot)
{
    ClonedArguments* thisObject = jsCast<ClonedArguments*>(object);
    VM& vm = exec->vm();
    
    if (ident == vm.propertyNames->callee
        || ident == vm.propertyNames->caller
        || ident == vm.propertyNames->iteratorSymbol)
        thisObject->materializeSpecialsIfNecessary(exec);
    
    if (Base::getOwnPropertySlot(thisObject, exec, ident, slot))
        return true;
    
    return false;
}

void ClonedArguments::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& array, EnumerationMode mode)
{
    ClonedArguments* thisObject = jsCast<ClonedArguments*>(object);
    thisObject->materializeSpecialsIfNecessary(exec);
    Base::getOwnPropertyNames(thisObject, exec, array, mode);
}

void ClonedArguments::put(JSCell* cell, ExecState* exec, PropertyName ident, JSValue value, PutPropertySlot& slot)
{
    ClonedArguments* thisObject = jsCast<ClonedArguments*>(cell);
    VM& vm = exec->vm();
    
    if (ident == vm.propertyNames->callee
        || ident == vm.propertyNames->caller
        || ident == vm.propertyNames->iteratorSymbol) {
        thisObject->materializeSpecialsIfNecessary(exec);
        PutPropertySlot dummy = slot; // Shadow the given PutPropertySlot to prevent caching.
        Base::put(thisObject, exec, ident, value, dummy);
        return;
    }
    
    Base::put(thisObject, exec, ident, value, slot);
}

bool ClonedArguments::deleteProperty(JSCell* cell, ExecState* exec, PropertyName ident)
{
    ClonedArguments* thisObject = jsCast<ClonedArguments*>(cell);
    VM& vm = exec->vm();
    
    if (ident == vm.propertyNames->callee
        || ident == vm.propertyNames->caller
        || ident == vm.propertyNames->iteratorSymbol)
        thisObject->materializeSpecialsIfNecessary(exec);
    
    return Base::deleteProperty(thisObject, exec, ident);
}

bool ClonedArguments::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName ident, const PropertyDescriptor& descriptor, bool shouldThrow)
{
    ClonedArguments* thisObject = jsCast<ClonedArguments*>(object);
    VM& vm = exec->vm();
    
    if (ident == vm.propertyNames->callee
        || ident == vm.propertyNames->caller
        || ident == vm.propertyNames->iteratorSymbol)
        thisObject->materializeSpecialsIfNecessary(exec);
    
    return Base::defineOwnProperty(object, exec, ident, descriptor, shouldThrow);
}

void ClonedArguments::materializeSpecials(ExecState* exec)
{
    RELEASE_ASSERT(!specialsMaterialized());
    VM& vm = exec->vm();
    
    FunctionExecutable* executable = jsCast<FunctionExecutable*>(m_callee->executable());
    bool isStrictMode = executable->isStrictMode();
    
    if (isStrictMode) {
        putDirectAccessor(exec, vm.propertyNames->callee, globalObject()->throwTypeErrorGetterSetter(vm), DontDelete | DontEnum | Accessor);
        putDirectAccessor(exec, vm.propertyNames->caller, globalObject()->throwTypeErrorGetterSetter(vm), DontDelete | DontEnum | Accessor);
    } else
        putDirect(vm, vm.propertyNames->callee, JSValue(m_callee.get()));

    putDirect(vm, vm.propertyNames->iteratorSymbol, globalObject()->arrayProtoValuesFunction(), DontEnum);
    
    m_callee.clear();
}

void ClonedArguments::materializeSpecialsIfNecessary(ExecState* exec)
{
    if (!specialsMaterialized())
        materializeSpecials(exec);
}

void ClonedArguments::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    ClonedArguments* thisObject = jsCast<ClonedArguments*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    Base::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_callee);
}

} // namespace JSC


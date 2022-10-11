/*
 * Copyright (C) 2013 Apple, Inc. All rights reserved.
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
#include "MapIteratorPrototype.h"

#include "IteratorOperations.h"
#include "JSCJSValueInlines.h"
#include "JSCellInlines.h"
#include "JSMapIterator.h"
#include "StructureInlines.h"

namespace JSC {

const ClassInfo MapIteratorPrototype::s_info = { "Map Iterator", &Base::s_info, 0, CREATE_METHOD_TABLE(MapIteratorPrototype) };

static EncodedJSValue JSC_HOST_CALL MapIteratorPrototypeFuncNext(ExecState*);

void MapIteratorPrototype::finishCreation(VM& vm, JSGlobalObject* globalObject)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    vm.prototypeMap.addPrototype(this);

    JSC_NATIVE_FUNCTION(vm.propertyNames->next, MapIteratorPrototypeFuncNext, DontEnum, 0);
    putDirectWithoutTransition(vm, vm.propertyNames->toStringTagSymbol, jsString(&vm, "Map Iterator"), DontEnum | ReadOnly);
}

EncodedJSValue JSC_HOST_CALL MapIteratorPrototypeFuncNext(CallFrame* callFrame)
{
    JSMapIterator* iterator = jsDynamicCast<JSMapIterator*>(callFrame->thisValue());
    if (!iterator)
        return JSValue::encode(throwTypeError(callFrame, ASCIILiteral("Cannot call MapIterator.next() on a non-MapIterator object")));

    JSValue result;
    if (iterator->next(callFrame, result))
        return JSValue::encode(createIteratorResultObject(callFrame, result, false));
    iterator->finish();
    return JSValue::encode(createIteratorResultObject(callFrame, jsUndefined(), true));
}


}

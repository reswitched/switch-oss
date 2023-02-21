/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008, 2011 Apple Inc. All rights reserved.
 *  Copyright (C) 2003 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "ArrayConstructor.h"

#include "ArrayPrototype.h"
#include "ButterflyInlines.h"
#include "CopiedSpaceInlines.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "GetterSetter.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "Lookup.h"
#include "JSCInlines.h"

namespace JSC {

static EncodedJSValue JSC_HOST_CALL arrayConstructorIsArray(ExecState*);

}

#include "ArrayConstructor.lut.h"

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(ArrayConstructor);

const ClassInfo ArrayConstructor::s_info = { "Function", &InternalFunction::s_info, &arrayConstructorTable, CREATE_METHOD_TABLE(ArrayConstructor) };

/* Source for ArrayConstructor.lut.h
@begin arrayConstructorTable
  of        arrayConstructorOf          DontEnum|Function 0
  from      arrayConstructorFrom        DontEnum|Function 0
@end
*/

ArrayConstructor::ArrayConstructor(VM& vm, Structure* structure)
    : InternalFunction(vm, structure)
{
#if PLATFORM(WKC)
    WKC_DEFINE_STATIC_BOOL(inited, false);
    if (!inited) {
        inited = true;
        arrayConstructorTable.keys = 0;
    }
#endif
}

void ArrayConstructor::finishCreation(VM& vm, JSGlobalObject* globalObject, ArrayPrototype* arrayPrototype, GetterSetter* speciesSymbol)
{
    Base::finishCreation(vm, arrayPrototype->classInfo()->className);
    putDirectWithoutTransition(vm, vm.propertyNames->prototype, arrayPrototype, DontEnum | DontDelete | ReadOnly);
    putDirectWithoutTransition(vm, vm.propertyNames->length, jsNumber(1), ReadOnly | DontEnum | DontDelete);
    putDirectNonIndexAccessor(vm, vm.propertyNames->speciesSymbol, speciesSymbol, Accessor | ReadOnly | DontEnum);
    JSC_NATIVE_FUNCTION(vm.propertyNames->isArray, arrayConstructorIsArray, DontEnum, 1);
}

bool ArrayConstructor::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<InternalFunction>(exec, arrayConstructorTable, jsCast<ArrayConstructor*>(object), propertyName, slot);
}

// ------------------------------ Functions ---------------------------

JSObject* constructArrayWithSizeQuirk(ExecState* exec, ArrayAllocationProfile* profile, JSGlobalObject* globalObject, JSValue length, JSValue prototype)
{
    if (!length.isNumber())
        return constructArrayNegativeIndexed(exec, profile, globalObject, &length, 1, prototype);
    
    uint32_t n = length.toUInt32(exec);
    if (n != length.toNumber(exec))
        return exec->vm().throwException(exec, createRangeError(exec, ASCIILiteral("Array size is not a small enough positive integer.")));
    return constructEmptyArray(exec, profile, globalObject, n, prototype);
}

static inline JSObject* constructArrayWithSizeQuirk(ExecState* exec, const ArgList& args, JSValue prototype)
{
    JSGlobalObject* globalObject = asInternalFunction(exec->callee())->globalObject();

    // a single numeric argument denotes the array size (!)
    if (args.size() == 1)
        return constructArrayWithSizeQuirk(exec, nullptr, globalObject, args.at(0), prototype);

    // otherwise the array is constructed with the arguments in it
    return constructArray(exec, nullptr, globalObject, args, prototype);
}

static EncodedJSValue JSC_HOST_CALL constructWithArrayConstructor(ExecState* exec)
{
    ArgList args(exec);

    JSValue prototype = JSValue();
    JSObject* newTarget = exec->newTarget().getObject();
    if (newTarget->classInfo() != ArrayConstructor::info())
        prototype = newTarget->get(exec, exec->propertyNames().prototype);

    return JSValue::encode(constructArrayWithSizeQuirk(exec, args, prototype));
}

ConstructType ArrayConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWithArrayConstructor;
    return ConstructTypeHost;
}

static EncodedJSValue JSC_HOST_CALL callArrayConstructor(ExecState* exec)
{
    ArgList args(exec);
    return JSValue::encode(constructArrayWithSizeQuirk(exec, args, JSValue()));
}

CallType ArrayConstructor::getCallData(JSCell*, CallData& callData)
{
    // equivalent to 'new Array(....)'
    callData.native.function = callArrayConstructor;
    return CallTypeHost;
}

EncodedJSValue JSC_HOST_CALL arrayConstructorIsArray(ExecState* exec)
{
    return JSValue::encode(jsBoolean(exec->argument(0).inherits(JSArray::info())));
}

EncodedJSValue JSC_HOST_CALL arrayConstructorPrivateFuncIsArrayConstructor(ExecState* exec)
{
    return JSValue::encode(jsBoolean(jsDynamicCast<ArrayConstructor*>(exec->uncheckedArgument(0))));
}

} // namespace JSC

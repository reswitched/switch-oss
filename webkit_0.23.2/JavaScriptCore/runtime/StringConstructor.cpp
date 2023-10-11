/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "StringConstructor.h"

#include "Error.h"
#include "Executable.h"
#include "JITCode.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSCInlines.h"
#include "StringPrototype.h"

namespace JSC {

static EncodedJSValue JSC_HOST_CALL stringFromCharCode(ExecState*);
static EncodedJSValue JSC_HOST_CALL stringFromCodePoint(ExecState*);

}

#include "StringConstructor.lut.h"

namespace JSC {

const ClassInfo StringConstructor::s_info = { "Function", &InternalFunction::s_info, &stringConstructorTable, CREATE_METHOD_TABLE(StringConstructor) };

/* Source for StringConstructor.lut.h
@begin stringConstructorTable
  fromCharCode          stringFromCharCode         DontEnum|Function 1
  fromCodePoint         stringFromCodePoint        DontEnum|Function 1
  raw                   stringRaw                  DontEnum|Function 1
@end
*/

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(StringConstructor);

StringConstructor::StringConstructor(VM& vm, Structure* structure)
    : InternalFunction(vm, structure)
{
#if PLATFORM(WKC)
    WKC_DEFINE_STATIC_BOOL(inited, false);
    if (!inited) {
        inited = true;
        stringConstructorTable.keys = 0;
    }
#endif
}

void StringConstructor::finishCreation(VM& vm, StringPrototype* stringPrototype)
{
    Base::finishCreation(vm, stringPrototype->classInfo()->className);
    putDirectWithoutTransition(vm, vm.propertyNames->prototype, stringPrototype, ReadOnly | DontEnum | DontDelete);
    putDirectWithoutTransition(vm, vm.propertyNames->length, jsNumber(1), ReadOnly | DontEnum | DontDelete);
}

bool StringConstructor::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<InternalFunction>(exec, stringConstructorTable, jsCast<StringConstructor*>(object), propertyName, slot);
}

// ------------------------------ Functions --------------------------------

static EncodedJSValue JSC_HOST_CALL stringFromCharCode(ExecState* exec)
{
    VM& vm = exec->vm();

    unsigned length = exec->argumentCount();
    if (LIKELY(length == 1)) {
        unsigned code = exec->uncheckedArgument(0).toUInt32(exec);
        if (vm.exception()) {
            return JSValue::encode(JSValue());
        }
        return JSValue::encode(jsSingleCharacterString(exec, code));
    }

    UChar* buf;
    auto impl = StringImpl::createUninitialized(length, buf);
    for (unsigned i = 0; i < length; ++i) {
        buf[i] = static_cast<UChar>(exec->uncheckedArgument(i).toUInt32(exec));
        if (vm.exception()) {
            return JSValue::encode(JSValue());
        }
    }
    return JSValue::encode(jsString(exec, WTF::move(impl)));
}

JSCell* JSC_HOST_CALL stringFromCharCode(ExecState* exec, int32_t arg)
{
    return jsSingleCharacterString(exec, arg);
}

static EncodedJSValue JSC_HOST_CALL stringFromCodePoint(ExecState* exec)
{
    unsigned length = exec->argumentCount();
    StringBuilder builder;
    builder.reserveCapacity(length);

    for (unsigned i = 0; i < length; ++i) {
        double codePointAsDouble = exec->uncheckedArgument(i).toNumber(exec);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());

        uint32_t codePoint = static_cast<uint32_t>(codePointAsDouble);

        if (codePoint != codePointAsDouble || codePoint > UCHAR_MAX_VALUE)
            return throwVMError(exec, createRangeError(exec, ASCIILiteral("Arguments contain a value that is out of range of code points")));

        if (U_IS_BMP(codePoint))
            builder.append(static_cast<UChar>(codePoint));
        else {
            builder.append(U16_LEAD(codePoint));
            builder.append(U16_TRAIL(codePoint));
        }
    }

    return JSValue::encode(jsString(exec, builder.toString()));
}

static EncodedJSValue JSC_HOST_CALL constructWithStringConstructor(ExecState* exec)
{
    JSGlobalObject* globalObject = asInternalFunction(exec->callee())->globalObject();
    VM& vm = exec->vm();

    if (!exec->argumentCount())
        return JSValue::encode(StringObject::create(vm, globalObject->stringObjectStructure()));

    return JSValue::encode(StringObject::create(vm, globalObject->stringObjectStructure(), exec->uncheckedArgument(0).toString(exec)));
}

ConstructType StringConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWithStringConstructor;
    return ConstructTypeHost;
}

JSCell* stringConstructor(ExecState* exec, JSValue argument)
{
    if (argument.isSymbol())
        return jsNontrivialString(exec, asSymbol(argument)->descriptiveString());
    return argument.toString(exec);
}

static EncodedJSValue JSC_HOST_CALL callStringConstructor(ExecState* exec)
{
    if (!exec->argumentCount())
        return JSValue::encode(jsEmptyString(exec));
    return JSValue::encode(stringConstructor(exec, exec->uncheckedArgument(0)));
}

CallType StringConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callStringConstructor;
    return CallTypeHost;
}

} // namespace JSC

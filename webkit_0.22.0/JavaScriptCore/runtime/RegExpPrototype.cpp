/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All Rights Reserved.
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
#include "RegExpPrototype.h"

#include "ArrayPrototype.h"
#include "Error.h"
#include "JSArray.h"
#include "JSCJSValue.h"
#include "JSFunction.h"
#include "JSObject.h"
#include "JSString.h"
#include "JSStringBuilder.h"
#include "Lexer.h"
#include "ObjectPrototype.h"
#include "JSCInlines.h"
#include "RegExpObject.h"
#include "RegExp.h"
#include "RegExpCache.h"
#include "StringRecursionChecker.h"

namespace JSC {

static EncodedJSValue JSC_HOST_CALL regExpProtoFuncTest(ExecState*);
static EncodedJSValue JSC_HOST_CALL regExpProtoFuncExec(ExecState*);
static EncodedJSValue JSC_HOST_CALL regExpProtoFuncCompile(ExecState*);
static EncodedJSValue JSC_HOST_CALL regExpProtoFuncToString(ExecState*);
static EncodedJSValue JSC_HOST_CALL regExpProtoGetterGlobal(ExecState*);
static EncodedJSValue JSC_HOST_CALL regExpProtoGetterIgnoreCase(ExecState*);
static EncodedJSValue JSC_HOST_CALL regExpProtoGetterMultiline(ExecState*);
static EncodedJSValue JSC_HOST_CALL regExpProtoGetterSource(ExecState*);
static EncodedJSValue JSC_HOST_CALL regExpProtoGetterFlags(ExecState*);

}

#include "RegExpPrototype.lut.h"

namespace JSC {

const ClassInfo RegExpPrototype::s_info = { "RegExp", &RegExpObject::s_info, &regExpPrototypeTable, CREATE_METHOD_TABLE(RegExpPrototype) };

/* Source for RegExpPrototype.lut.h
@begin regExpPrototypeTable
  compile       regExpProtoFuncCompile      DontEnum|Function 2
  exec          regExpProtoFuncExec         DontEnum|Function 1
  test          regExpProtoFuncTest         DontEnum|Function 1
  toString      regExpProtoFuncToString     DontEnum|Function 0
  global        regExpProtoGetterGlobal     DontEnum|Accessor
  ignoreCase    regExpProtoGetterIgnoreCase DontEnum|Accessor
  multiline     regExpProtoGetterMultiline  DontEnum|Accessor
  source        regExpProtoGetterSource     DontEnum|Accessor
  flags         regExpProtoGetterFlags      DontEnum|Accessor
@end
*/

RegExpPrototype::RegExpPrototype(VM& vm, Structure* structure, RegExp* regExp)
    : RegExpObject(vm, structure, regExp)
{
#if PLATFORM(WKC)
    WKC_DEFINE_STATIC_BOOL(inited, false);
    if (!inited) {
        inited = true;
        regExpPrototypeTable.keys = 0;
    }
#endif
}

bool RegExpPrototype::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<Base>(exec, regExpPrototypeTable, jsCast<RegExpPrototype*>(object), propertyName, slot);
}

// ------------------------------ Functions ---------------------------

EncodedJSValue JSC_HOST_CALL regExpProtoFuncTest(ExecState* exec)
{
    JSValue thisValue = exec->thisValue();
    if (!thisValue.inherits(RegExpObject::info()))
        return throwVMTypeError(exec);
    return JSValue::encode(jsBoolean(asRegExpObject(thisValue)->test(exec, exec->argument(0).toString(exec))));
}

EncodedJSValue JSC_HOST_CALL regExpProtoFuncExec(ExecState* exec)
{
    JSValue thisValue = exec->thisValue();
    if (!thisValue.inherits(RegExpObject::info()))
        return throwVMTypeError(exec);
    return JSValue::encode(asRegExpObject(thisValue)->exec(exec, exec->argument(0).toString(exec)));
}

EncodedJSValue JSC_HOST_CALL regExpProtoFuncCompile(ExecState* exec)
{
    JSValue thisValue = exec->thisValue();
    if (!thisValue.inherits(RegExpObject::info()))
        return throwVMTypeError(exec);

    RegExp* regExp;
    JSValue arg0 = exec->argument(0);
    JSValue arg1 = exec->argument(1);
    
    if (arg0.inherits(RegExpObject::info())) {
        if (!arg1.isUndefined())
            return throwVMError(exec, createTypeError(exec, ASCIILiteral("Cannot supply flags when constructing one RegExp from another.")));
        regExp = asRegExpObject(arg0)->regExp();
    } else {
        String pattern = !exec->argumentCount() ? emptyString() : arg0.toString(exec)->value(exec);
        if (exec->hadException())
            return JSValue::encode(jsUndefined());

        RegExpFlags flags = NoFlags;
        if (!arg1.isUndefined()) {
            flags = regExpFlags(arg1.toString(exec)->value(exec));
            if (exec->hadException())
                return JSValue::encode(jsUndefined());
            if (flags == InvalidFlags)
                return throwVMError(exec, createSyntaxError(exec, ASCIILiteral("Invalid flags supplied to RegExp constructor.")));
        }
        regExp = RegExp::create(exec->vm(), pattern, flags);
    }

    if (!regExp->isValid())
        return throwVMError(exec, createSyntaxError(exec, regExp->errorMessage()));

    asRegExpObject(thisValue)->setRegExp(exec->vm(), regExp);
    asRegExpObject(thisValue)->setLastIndex(exec, 0);
    return JSValue::encode(jsUndefined());
}

typedef std::array<char, 3 + 1> FlagsString; // 3 different flags and a null character terminator.

static inline FlagsString flagsString(ExecState* exec, JSObject* regexp)
{
    FlagsString string;

    JSValue globalValue = regexp->get(exec, exec->propertyNames().global);
    if (exec->hadException())
        return string;
    JSValue ignoreCaseValue = regexp->get(exec, exec->propertyNames().ignoreCase);
    if (exec->hadException())
        return string;
    JSValue multilineValue = regexp->get(exec, exec->propertyNames().multiline);

    unsigned index = 0;
    if (globalValue.toBoolean(exec))
        string[index++] = 'g';
    if (ignoreCaseValue.toBoolean(exec))
        string[index++] = 'i';
    if (multilineValue.toBoolean(exec))
        string[index++] = 'm';
    ASSERT(index < string.size());
    string[index] = 0;
    return string;
}

EncodedJSValue JSC_HOST_CALL regExpProtoFuncToString(ExecState* exec)
{
    JSValue thisValue = exec->thisValue();
    if (!thisValue.isObject())
        return throwVMTypeError(exec);

    JSObject* thisObject = asObject(thisValue);

    StringRecursionChecker checker(exec, thisObject);
    if (JSValue earlyReturnValue = checker.earlyReturnValue())
        return JSValue::encode(earlyReturnValue);

    JSValue sourceValue = thisObject->get(exec, exec->propertyNames().source);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());
    String source = sourceValue.toString(exec)->value(exec);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    auto flags = flagsString(exec, thisObject);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    return JSValue::encode(jsMakeNontrivialString(exec, '/', source, '/', flags.data()));
}

EncodedJSValue JSC_HOST_CALL regExpProtoGetterGlobal(ExecState* exec)
{
    JSValue thisValue = exec->thisValue();
    if (!thisValue.inherits(RegExpObject::info()))
        return throwVMTypeError(exec);

    return JSValue::encode(jsBoolean(asRegExpObject(thisValue)->regExp()->global()));
}

EncodedJSValue JSC_HOST_CALL regExpProtoGetterIgnoreCase(ExecState* exec)
{
    JSValue thisValue = exec->thisValue();
    if (!thisValue.inherits(RegExpObject::info()))
        return throwVMTypeError(exec);

    return JSValue::encode(jsBoolean(asRegExpObject(thisValue)->regExp()->ignoreCase()));
}

EncodedJSValue JSC_HOST_CALL regExpProtoGetterMultiline(ExecState* exec)
{
    JSValue thisValue = exec->thisValue();
    if (!thisValue.inherits(RegExpObject::info()))
        return throwVMTypeError(exec);

    return JSValue::encode(jsBoolean(asRegExpObject(thisValue)->regExp()->multiline()));
}

EncodedJSValue JSC_HOST_CALL regExpProtoGetterFlags(ExecState* exec)
{
    JSValue thisValue = exec->thisValue();
    if (!thisValue.isObject())
        return throwVMTypeError(exec);

    auto flags = flagsString(exec, asObject(thisValue));
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    return JSValue::encode(jsString(exec, flags.data()));
}

template <typename CharacterType>
static inline void appendLineTerminatorEscape(StringBuilder&, CharacterType);

template <>
inline void appendLineTerminatorEscape<LChar>(StringBuilder& builder, LChar lineTerminator)
{
    if (lineTerminator == '\n')
        builder.append('n');
    else
        builder.append('r');
}

template <>
inline void appendLineTerminatorEscape<UChar>(StringBuilder& builder, UChar lineTerminator)
{
    if (lineTerminator == '\n')
        builder.append('n');
    else if (lineTerminator == '\r')
        builder.append('r');
    else if (lineTerminator == 0x2028)
        builder.appendLiteral("u2028");
    else
        builder.appendLiteral("u2029");
}

template <typename CharacterType>
static inline JSValue regExpProtoGetterSourceInternal(ExecState* exec, const String& pattern, const CharacterType* characters, unsigned length)
{
    bool previousCharacterWasBackslash = false;
    bool inBrackets = false;
    bool shouldEscape = false;

    // 15.10.6.4 specifies that RegExp.prototype.toString must return '/' + source + '/',
    // and also states that the result must be a valid RegularExpressionLiteral. '//' is
    // not a valid RegularExpressionLiteral (since it is a single line comment), and hence
    // source cannot ever validly be "". If the source is empty, return a different Pattern
    // that would match the same thing.
    if (!length)
        return jsNontrivialString(exec, ASCIILiteral("(?:)"));

    // early return for strings that don't contain a forwards slash and LineTerminator
    for (unsigned i = 0; i < length; ++i) {
        CharacterType ch = characters[i];
        if (!previousCharacterWasBackslash) {
            if (inBrackets) {
                if (ch == ']')
                    inBrackets = false;
            } else {
                if (ch == '/') {
                    shouldEscape = true;
                    break;
                }
                if (ch == '[')
                    inBrackets = true;
            }
        }

        if (Lexer<CharacterType>::isLineTerminator(ch)) {
            shouldEscape = true;
            break;
        }

        if (previousCharacterWasBackslash)
            previousCharacterWasBackslash = false;
        else
            previousCharacterWasBackslash = ch == '\\';
    }

    if (!shouldEscape)
        return jsString(exec, pattern);

    previousCharacterWasBackslash = false;
    inBrackets = false;
    StringBuilder result;
    for (unsigned i = 0; i < length; ++i) {
        CharacterType ch = characters[i];
        if (!previousCharacterWasBackslash) {
            if (inBrackets) {
                if (ch == ']')
                    inBrackets = false;
            } else {
                if (ch == '/')
                    result.append('\\');
                else if (ch == '[')
                    inBrackets = true;
            }
        }

        // escape LineTerminator
        if (Lexer<CharacterType>::isLineTerminator(ch)) {
            if (!previousCharacterWasBackslash)
                result.append('\\');

            appendLineTerminatorEscape<CharacterType>(result, ch);
        } else
            result.append(ch);

        if (previousCharacterWasBackslash)
            previousCharacterWasBackslash = false;
        else
            previousCharacterWasBackslash = ch == '\\';
    }

    return jsString(exec, result.toString());
}

EncodedJSValue JSC_HOST_CALL regExpProtoGetterSource(ExecState* exec)
{
    JSValue thisValue = exec->thisValue();
    if (!thisValue.inherits(RegExpObject::info()))
        return throwVMTypeError(exec);

    String pattern = asRegExpObject(thisValue)->regExp()->pattern();
    if (pattern.is8Bit())
        return JSValue::encode(regExpProtoGetterSourceInternal(exec, pattern, pattern.characters8(), pattern.length()));
    return JSValue::encode(regExpProtoGetterSourceInternal(exec, pattern, pattern.characters16(), pattern.length()));
}

} // namespace JSC

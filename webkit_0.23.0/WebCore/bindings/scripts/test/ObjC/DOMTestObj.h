/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
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

#import <WebCore/DOMObject.h>

@class DOMDictionary;
@class DOMDocument;
@class DOMNode;
@class DOMPromise;
@class DOMSVGDocument;
@class DOMSVGPoint;
@class DOMTestEnumType;
@class DOMTestNode;
@class DOMTestObj;
@class DOMTestObjectAConstructor;
@class DOMTestObjectBConstructor;
@class DOMTestObjectCConstructor;
@class DOMTestSubObjConstructor;
@class DOMany;
@class DOMbool;
@class NSString;
@protocol DOMEventListener;

enum {
    DOM_CONST_VALUE_0 = 0,
    DOM_CONST_VALUE_1 = 1,
    DOM_CONST_VALUE_2 = 2,
    DOM_CONST_VALUE_4 = 4,
    DOM_CONST_VALUE_8 = 8,
    DOM_CONST_VALUE_9 = -1,
    DOM_CONST_VALUE_11 = 0xffffffff,
    DOM_CONST_VALUE_12 = 0x01,
    DOM_CONST_VALUE_13 = 0X20,
    DOM_CONST_VALUE_14 = 0x1abc,
    DOM_CONST_JAVASCRIPT = 15,
    DOM_readonly = 0
} WEBKIT_ENUM_AVAILABLE_MAC(9876_5);

WEBKIT_CLASS_AVAILABLE_MAC(9876_5)
WEBCORE_EXPORT @interface DOMTestObj : DOMObject
@property (readonly) int readOnlyLongAttr;
@property (readonly, copy) NSString *readOnlyStringAttr;
@property (readonly, strong) DOMTestObj *readOnlyTestObjAttr;
@property (strong) DOMTestSubObjConstructor *TestSubObjEnabledBySetting;
@property char byteAttr;
@property unsigned char octetAttr;
@property short shortAttr;
@property unsigned short unsignedShortAttr;
@property int longAttr;
@property long long longLongAttr;
@property unsigned long long unsignedLongLongAttr;
@property (copy) NSString *stringAttr;
@property (strong) DOMTestObj *testObjAttr;
@property (strong) DOMTestObj *XMLObjAttr;
@property BOOL create;
@property (copy) NSString *reflectedStringAttr;
@property int reflectedIntegralAttr;
@property unsigned reflectedUnsignedIntegralAttr;
@property BOOL reflectedBooleanAttr;
@property (copy) NSString *reflectedURLAttr;
@property (copy) NSString *reflectedStringAttr;
@property int reflectedCustomIntegralAttr;
@property BOOL reflectedCustomBooleanAttr;
@property (copy) NSString *reflectedCustomURLAttr;
@property int attrWithGetterException;
@property int attrWithSetterException;
@property (copy) NSString *stringAttrWithGetterException;
@property (copy) NSString *stringAttrWithSetterException;
@property int customAttr;
@property int withScriptStateAttribute;
@property (strong) DOMTestObj *withScriptExecutionContextAttribute;
@property (strong) DOMTestObj *withScriptStateAttributeRaises;
@property (strong) DOMTestObj *withScriptExecutionContextAttributeRaises;
@property (strong) DOMTestObj *withScriptExecutionContextAndScriptStateAttribute;
@property (strong) DOMTestObj *withScriptExecutionContextAndScriptStateAttributeRaises;
@property (strong) DOMTestObj *withScriptExecutionContextAndScriptStateWithSpacesAttribute;
@property (strong) DOMTestObj *withScriptArgumentsAndCallStackAttribute;
@property int conditionalAttr1;
@property int conditionalAttr2;
@property int conditionalAttr3;
@property (strong) DOMTestObjectAConstructor *conditionalAttr4;
@property (strong) DOMTestObjectBConstructor *conditionalAttr5;
@property (strong) DOMTestObjectCConstructor *conditionalAttr6;
@property (strong) DOMany *anyAttribute;
@property (readonly, strong) DOMDocument *contentDocument;
@property (strong) DOMSVGPoint *mutablePoint;
@property (strong) DOMSVGPoint *immutablePoint;
@property int strawberry;
@property float strictFloat;
@property (readonly) int descriptionName;
@property int idName;
@property (readonly, copy) NSString *hashName;
@property (readonly) int replaceableAttribute;
@property (readonly) double nullableDoubleAttribute;
@property (readonly) int nullableLongAttribute;
@property (readonly) BOOL nullableBooleanAttribute;
@property (readonly, copy) NSString *nullableStringAttribute;
@property int nullableLongSettableAttribute;
@property int nullableStringValue;
@property (readonly, copy) NSString *attribute;
@property (readonly, strong) DOMPromise *testPromiseAttr;

- (void)voidMethod;
- (void)voidMethodWithArgs:(int)longArg strArg:(NSString *)strArg objArg:(DOMTestObj *)objArg;
- (char)byteMethod;
- (char)byteMethodWithArgs:(char)byteArg strArg:(NSString *)strArg objArg:(DOMTestObj *)objArg;
- (unsigned char)octetMethod;
- (unsigned char)octetMethodWithArgs:(unsigned char)octetArg strArg:(NSString *)strArg objArg:(DOMTestObj *)objArg;
- (int)longMethod;
- (int)longMethodWithArgs:(int)longArg strArg:(NSString *)strArg objArg:(DOMTestObj *)objArg;
- (DOMTestObj *)objMethod;
- (DOMTestObj *)objMethodWithArgs:(int)longArg strArg:(NSString *)strArg objArg:(DOMTestObj *)objArg;
- (void)methodWithEnumArg:(DOMTestEnumType *)enumArg;
- (DOMTestObj *)methodThatRequiresAllArgsAndThrows:(NSString *)strArg objArg:(DOMTestObj *)objArg;
- (void)serializedValue:(NSString *)serializedArg;
- (void)optionsObject:(DOMDictionary *)oo ooo:(DOMDictionary *)ooo;
- (void)methodWithException;
- (void)customMethod;
- (void)customMethodWithArgs:(int)longArg strArg:(NSString *)strArg objArg:(DOMTestObj *)objArg;
- (void)addEventListener:(NSString *)type listener:(id <DOMEventListener>)listener useCapture:(BOOL)useCapture;
- (void)removeEventListener:(NSString *)type listener:(id <DOMEventListener>)listener useCapture:(BOOL)useCapture;
- (void)withScriptStateVoid;
- (DOMTestObj *)withScriptStateObj;
- (void)withScriptStateVoidException;
- (DOMTestObj *)withScriptStateObjException;
- (void)withScriptExecutionContext;
- (void)withScriptExecutionContextAndScriptState;
- (DOMTestObj *)withScriptExecutionContextAndScriptStateObjException;
- (DOMTestObj *)withScriptExecutionContextAndScriptStateWithSpaces;
- (void)withScriptArgumentsAndCallStack;
- (void)methodWithOptionalArg:(int)opt;
- (void)methodWithNonOptionalArgAndOptionalArg:(int)nonOpt opt:(int)opt;
- (void)methodWithNonOptionalArgAndTwoOptionalArgs:(int)nonOpt opt1:(int)opt1 opt2:(int)opt2;
- (void)methodWithOptionalString:(NSString *)str;
- (void)methodWithOptionalStringIsUndefined:(NSString *)str;
- (void)methodWithOptionalStringIsNullString:(NSString *)str;
- (void)classMethod;
- (int)classMethodWithOptional:(int)arg;
- (void)classMethod2:(int)arg;
- (DOMSVGDocument *)getSVGDocument;
- (void)convert1:(DOMTestNode *)value;
- (void)convert2:(DOMTestNode *)value;
- (void)convert4:(DOMTestNode *)value;
- (void)convert5:(DOMTestNode *)value;
- (DOMSVGPoint *)mutablePointFunction;
- (DOMSVGPoint *)immutablePointFunction;
- (void)orange;
- (DOMbool *)strictFunction:(NSString *)str a:(float)a b:(int)b;
- (void)variadicStringMethod:(NSString *)head tail:(NSString *)tail;
- (void)variadicDoubleMethod:(double)head tail:(double)tail;
- (void)variadicNodeMethod:(DOMNode *)head tail:(DOMNode *)tail;
- (void)any:(float)a b:(int)b;
@end

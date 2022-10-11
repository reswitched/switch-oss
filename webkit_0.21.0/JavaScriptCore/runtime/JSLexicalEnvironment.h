/*
 * Copyright (C) 2008, 2009, 2013-2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#ifndef JSLexicalEnvironment_h
#define JSLexicalEnvironment_h

#include "CodeBlock.h"
#include "CopiedSpaceInlines.h"
#include "JSEnvironmentRecord.h"
#include "SymbolTable.h"

namespace JSC {

class Register;
    
class JSLexicalEnvironment : public JSEnvironmentRecord {
private:
    JSLexicalEnvironment(VM&, Structure*, JSScope*, SymbolTable*);
    
public:
    typedef JSEnvironmentRecord Base;
    static const unsigned StructureFlags = Base::StructureFlags | OverridesGetOwnPropertySlot | OverridesGetPropertyNames;

    static JSLexicalEnvironment* create(
        VM& vm, Structure* structure, JSScope* currentScope, SymbolTable* symbolTable)
    {
        JSLexicalEnvironment* result = 
            new (
                NotNull,
                allocateCell<JSLexicalEnvironment>(vm.heap, allocationSize(symbolTable)))
            JSLexicalEnvironment(vm, structure, currentScope, symbolTable);
        result->finishCreation(vm);
        return result;
    }
    
    static JSLexicalEnvironment* create(VM& vm, CallFrame* callFrame, JSScope* currentScope, CodeBlock* codeBlock)
    {
        JSGlobalObject* globalObject = callFrame->lexicalGlobalObject();
        Structure* structure = globalObject->activationStructure();
        SymbolTable* symbolTable = codeBlock->symbolTable();
        return create(vm, structure, currentScope, symbolTable);
    }
        
    static bool getOwnPropertySlot(JSObject*, ExecState*, PropertyName, PropertySlot&);
    static void getOwnNonIndexPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);

    static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);

    static bool deleteProperty(JSCell*, ExecState*, PropertyName);

    static JSValue toThis(JSCell*, ExecState*, ECMAMode);

    DECLARE_INFO;

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject) { return Structure::create(vm, globalObject, jsNull(), TypeInfo(ActivationObjectType, StructureFlags), info()); }

private:
    bool symbolTableGet(PropertyName, PropertySlot&);
    bool symbolTableGet(PropertyName, PropertyDescriptor&);
    bool symbolTableGet(PropertyName, PropertySlot&, bool& slotIsWriteable);
    bool symbolTablePut(ExecState*, PropertyName, JSValue, bool shouldThrow);
    bool symbolTablePutWithAttributes(VM&, PropertyName, JSValue, unsigned attributes);
};

inline JSLexicalEnvironment::JSLexicalEnvironment(VM& vm, Structure* structure, JSScope* currentScope, SymbolTable* symbolTable)
    : Base(vm, structure, currentScope, symbolTable)
{
}

inline JSLexicalEnvironment* asActivation(JSValue value)
{
    ASSERT(asObject(value)->inherits(JSLexicalEnvironment::info()));
    return jsCast<JSLexicalEnvironment*>(asObject(value));
}
    
ALWAYS_INLINE JSLexicalEnvironment* Register::lexicalEnvironment() const
{
    return asActivation(jsValue());
}

} // namespace JSC

#endif // JSLexicalEnvironment_h

/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "JSTestNode.h"

#include "ActiveDOMObject.h"
#include "JSDOMAttribute.h"
#include "JSDOMBinding.h"
#include "JSDOMConstructor.h"
#include "JSDOMConvertBoolean.h"
#include "JSDOMConvertInterface.h"
#include "JSDOMConvertStrings.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMIterator.h"
#include "JSDOMOperation.h"
#include "JSDOMOperationReturningPromise.h"
#include "JSDOMWrapperCache.h"
#include "JSTestNode.h"
#include "RuntimeEnabledFeatures.h"
#include "ScriptExecutionContext.h"
#include <JavaScriptCore/BuiltinNames.h>
#include <JavaScriptCore/HeapAnalyzer.h>
#include <JavaScriptCore/JSCInlines.h>
#include <JavaScriptCore/ObjectConstructor.h>
#include <wtf/GetPtr.h>
#include <wtf/PointerPreparations.h>
#include <wtf/URL.h>


namespace WebCore {
using namespace JSC;

// Functions

JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionTestWorkerPromise(JSC::JSGlobalObject*, JSC::CallFrame*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionCalculateSecretResult(JSC::JSGlobalObject*, JSC::CallFrame*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionGetSecretBoolean(JSC::JSGlobalObject*, JSC::CallFrame*);
#if ENABLE(TEST_FEATURE)
JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionTestFeatureGetSecretBoolean(JSC::JSGlobalObject*, JSC::CallFrame*);
#endif
JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionSymbolIterator(JSC::JSGlobalObject*, JSC::CallFrame*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionEntries(JSC::JSGlobalObject*, JSC::CallFrame*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionKeys(JSC::JSGlobalObject*, JSC::CallFrame*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionValues(JSC::JSGlobalObject*, JSC::CallFrame*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionForEach(JSC::JSGlobalObject*, JSC::CallFrame*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionToJSON(JSC::JSGlobalObject*, JSC::CallFrame*);

// Attributes

JSC::EncodedJSValue jsTestNodeConstructor(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestNodeConstructor(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::EncodedJSValue);
JSC::EncodedJSValue jsTestNodeName(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestNodeName(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::EncodedJSValue);

class JSTestNodePrototype : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestNodePrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestNodePrototype* ptr = new (NotNull, JSC::allocateCell<JSTestNodePrototype>(vm.heap)) JSTestNodePrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestNodePrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};
STATIC_ASSERT_ISO_SUBSPACE_SHARABLE(JSTestNodePrototype, JSTestNodePrototype::Base);

using JSTestNodeConstructor = JSDOMConstructor<JSTestNode>;

template<> EncodedJSValue JSC_HOST_CALL JSTestNodeConstructor::construct(JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame)
{
    VM& vm = lexicalGlobalObject->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    auto* castedThis = jsCast<JSTestNodeConstructor*>(callFrame->jsCallee());
    ASSERT(castedThis);
    auto object = TestNode::create();
    return JSValue::encode(toJSNewlyCreated<IDLInterface<TestNode>>(*lexicalGlobalObject, *castedThis->globalObject(), WTFMove(object)));
}

template<> JSValue JSTestNodeConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    return JSNode::getConstructor(vm, &globalObject);
}

template<> void JSTestNodeConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, JSTestNode::prototype(vm, globalObject), JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(vm, String("TestNode"_s)), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
}

template<> const ClassInfo JSTestNodeConstructor::s_info = { "TestNode", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNodeConstructor) };

/* Hash table for prototype */

static const HashTableValue JSTestNodePrototypeTableValues[] =
{
    { "constructor", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestNodeConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestNodeConstructor) } },
    { "name", static_cast<unsigned>(JSC::PropertyAttribute::CustomAccessor | JSC::PropertyAttribute::DOMAttribute), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestNodeName), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestNodeName) } },
    { "testWorkerPromise", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestNodePrototypeFunctionTestWorkerPromise), (intptr_t) (0) } },
    { "calculateSecretResult", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestNodePrototypeFunctionCalculateSecretResult), (intptr_t) (0) } },
    { "getSecretBoolean", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestNodePrototypeFunctionGetSecretBoolean), (intptr_t) (0) } },
#if ENABLE(TEST_FEATURE)
    { "testFeatureGetSecretBoolean", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestNodePrototypeFunctionTestFeatureGetSecretBoolean), (intptr_t) (0) } },
#else
    { 0, 0, NoIntrinsic, { 0, 0 } },
#endif
    { "entries", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestNodePrototypeFunctionEntries), (intptr_t) (0) } },
    { "keys", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestNodePrototypeFunctionKeys), (intptr_t) (0) } },
    { "values", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestNodePrototypeFunctionValues), (intptr_t) (0) } },
    { "forEach", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestNodePrototypeFunctionForEach), (intptr_t) (1) } },
    { "toJSON", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestNodePrototypeFunctionToJSON), (intptr_t) (0) } },
};

const ClassInfo JSTestNodePrototype::s_info = { "TestNodePrototype", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNodePrototype) };

void JSTestNodePrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestNode::info(), JSTestNodePrototypeTableValues, *this);
    bool hasDisabledRuntimeProperties = false;
    if (!jsCast<JSDOMGlobalObject*>(globalObject())->scriptExecutionContext()->isSecureContext()) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, reinterpret_cast<const LChar*>("calculateSecretResult"), strlen("calculateSecretResult"));
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        JSObject::deleteProperty(this, globalObject(), propertyName);
    }
    if (!jsCast<JSDOMGlobalObject*>(globalObject())->scriptExecutionContext()->isSecureContext()) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, reinterpret_cast<const LChar*>("getSecretBoolean"), strlen("getSecretBoolean"));
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        JSObject::deleteProperty(this, globalObject(), propertyName);
    }
#if ENABLE(TEST_FEATURE)
    if (!(jsCast<JSDOMGlobalObject*>(globalObject())->scriptExecutionContext()->isSecureContext() && RuntimeEnabledFeatures::sharedFeatures().testFeatureEnabled())) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, reinterpret_cast<const LChar*>("testFeatureGetSecretBoolean"), strlen("testFeatureGetSecretBoolean"));
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        JSObject::deleteProperty(this, globalObject(), propertyName);
    }
#endif
    if (!RuntimeEnabledFeatures::sharedFeatures().domIteratorEnabled()) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, reinterpret_cast<const LChar*>("entries"), strlen("entries"));
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        JSObject::deleteProperty(this, globalObject(), propertyName);
    }
    if (!RuntimeEnabledFeatures::sharedFeatures().domIteratorEnabled()) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, reinterpret_cast<const LChar*>("keys"), strlen("keys"));
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        JSObject::deleteProperty(this, globalObject(), propertyName);
    }
    if (!RuntimeEnabledFeatures::sharedFeatures().domIteratorEnabled()) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, reinterpret_cast<const LChar*>("values"), strlen("values"));
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        JSObject::deleteProperty(this, globalObject(), propertyName);
    }
    if (!RuntimeEnabledFeatures::sharedFeatures().domIteratorEnabled()) {
        hasDisabledRuntimeProperties = true;
        auto propertyName = Identifier::fromString(vm, reinterpret_cast<const LChar*>("forEach"), strlen("forEach"));
        VM::DeletePropertyModeScope scope(vm, VM::DeletePropertyMode::IgnoreConfigurable);
        JSObject::deleteProperty(this, globalObject(), propertyName);
    }
    if (hasDisabledRuntimeProperties && structure()->isDictionary())
        flattenDictionaryObject(vm);
    putDirect(vm, vm.propertyNames->iteratorSymbol, getDirect(vm, vm.propertyNames->builtinNames().entriesPublicName()), static_cast<unsigned>(JSC::PropertyAttribute::DontEnum));
}

const ClassInfo JSTestNode::s_info = { "TestNode", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNode) };

JSTestNode::JSTestNode(Structure* structure, JSDOMGlobalObject& globalObject, Ref<TestNode>&& impl)
    : JSNode(structure, globalObject, WTFMove(impl))
{
}

void JSTestNode::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

    static_assert(!std::is_base_of<ActiveDOMObject, TestNode>::value, "Interface is not marked as [ActiveDOMObject] even though implementation class subclasses ActiveDOMObject.");

}

JSObject* JSTestNode::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return JSTestNodePrototype::create(vm, &globalObject, JSTestNodePrototype::createStructure(vm, &globalObject, JSNode::prototype(vm, globalObject)));
}

JSObject* JSTestNode::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestNode>(vm, globalObject);
}

JSValue JSTestNode::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestNodeConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

template<> inline JSTestNode* IDLAttribute<JSTestNode>::cast(JSGlobalObject& lexicalGlobalObject, EncodedJSValue thisValue)
{
    return jsDynamicCast<JSTestNode*>(JSC::getVM(&lexicalGlobalObject), JSValue::decode(thisValue));
}

template<> inline JSTestNode* IDLOperation<JSTestNode>::cast(JSGlobalObject& lexicalGlobalObject, CallFrame& callFrame)
{
    return jsDynamicCast<JSTestNode*>(JSC::getVM(&lexicalGlobalObject), callFrame.thisValue());
}

EncodedJSValue jsTestNodeConstructor(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName)
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestNodePrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(lexicalGlobalObject, throwScope);
    return JSValue::encode(JSTestNode::getConstructor(JSC::getVM(lexicalGlobalObject), prototype->globalObject()));
}

bool setJSTestNodeConstructor(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestNodePrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype)) {
        throwVMTypeError(lexicalGlobalObject, throwScope);
        return false;
    }
    // Shadowing a built-in constructor
    return prototype->putDirect(vm, vm.propertyNames->constructor, JSValue::decode(encodedValue));
}

static inline JSValue jsTestNodeNameGetter(JSGlobalObject& lexicalGlobalObject, JSTestNode& thisObject, ThrowScope& throwScope)
{
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(lexicalGlobalObject);
    auto& impl = thisObject.wrapped();
    JSValue result = toJS<IDLDOMString>(lexicalGlobalObject, throwScope, impl.name());
    return result;
}

EncodedJSValue jsTestNodeName(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName)
{
    return IDLAttribute<JSTestNode>::get<jsTestNodeNameGetter, CastedThisErrorBehavior::Assert>(*lexicalGlobalObject, thisValue, "name");
}

static inline bool setJSTestNodeNameSetter(JSGlobalObject& lexicalGlobalObject, JSTestNode& thisObject, JSValue value, ThrowScope& throwScope)
{
    UNUSED_PARAM(lexicalGlobalObject);
    UNUSED_PARAM(throwScope);
    auto& impl = thisObject.wrapped();
    auto nativeValue = convert<IDLDOMString>(lexicalGlobalObject, value);
    RETURN_IF_EXCEPTION(throwScope, false);
    AttributeSetter::call(lexicalGlobalObject, throwScope, [&] {
        return impl.setName(WTFMove(nativeValue));
    });
    return true;
}

bool setJSTestNodeName(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    return IDLAttribute<JSTestNode>::set<setJSTestNodeNameSetter>(*lexicalGlobalObject, thisValue, encodedValue, "name");
}

static inline JSC::EncodedJSValue jsTestNodePrototypeFunctionTestWorkerPromiseBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperationReturningPromise<JSTestNode>::ClassParameter castedThis, Ref<DeferredPromise>&& promise, JSC::ThrowScope& throwScope)
{
    UNUSED_PARAM(lexicalGlobalObject);
    UNUSED_PARAM(callFrame);
    UNUSED_PARAM(throwScope);
    auto& impl = castedThis->wrapped();
    impl.testWorkerPromise(WTFMove(promise));
    return JSValue::encode(jsUndefined());
}

EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionTestWorkerPromise(JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame)
{
    return IDLOperationReturningPromise<JSTestNode>::call<jsTestNodePrototypeFunctionTestWorkerPromiseBody>(*lexicalGlobalObject, *callFrame, "testWorkerPromise");
}

static inline JSC::EncodedJSValue jsTestNodePrototypeFunctionCalculateSecretResultBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperationReturningPromise<JSTestNode>::ClassParameter castedThis, Ref<DeferredPromise>&& promise, JSC::ThrowScope& throwScope)
{
    UNUSED_PARAM(lexicalGlobalObject);
    UNUSED_PARAM(callFrame);
    UNUSED_PARAM(throwScope);
    auto& impl = castedThis->wrapped();
    impl.calculateSecretResult(WTFMove(promise));
    return JSValue::encode(jsUndefined());
}

EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionCalculateSecretResult(JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame)
{
    return IDLOperationReturningPromise<JSTestNode>::call<jsTestNodePrototypeFunctionCalculateSecretResultBody>(*lexicalGlobalObject, *callFrame, "calculateSecretResult");
}

static inline JSC::EncodedJSValue jsTestNodePrototypeFunctionGetSecretBooleanBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestNode>::ClassParameter castedThis, JSC::ThrowScope& throwScope)
{
    UNUSED_PARAM(lexicalGlobalObject);
    UNUSED_PARAM(callFrame);
    UNUSED_PARAM(throwScope);
    auto& impl = castedThis->wrapped();
    return JSValue::encode(toJS<IDLBoolean>(impl.getSecretBoolean()));
}

EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionGetSecretBoolean(JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame)
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunctionGetSecretBooleanBody>(*lexicalGlobalObject, *callFrame, "getSecretBoolean");
}

#if ENABLE(TEST_FEATURE)
static inline JSC::EncodedJSValue jsTestNodePrototypeFunctionTestFeatureGetSecretBooleanBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestNode>::ClassParameter castedThis, JSC::ThrowScope& throwScope)
{
    UNUSED_PARAM(lexicalGlobalObject);
    UNUSED_PARAM(callFrame);
    UNUSED_PARAM(throwScope);
    auto& impl = castedThis->wrapped();
    return JSValue::encode(toJS<IDLBoolean>(impl.testFeatureGetSecretBoolean()));
}

EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionTestFeatureGetSecretBoolean(JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame)
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunctionTestFeatureGetSecretBooleanBody>(*lexicalGlobalObject, *callFrame, "testFeatureGetSecretBoolean");
}

#endif

struct TestNodeIteratorTraits {
    static constexpr JSDOMIteratorType type = JSDOMIteratorType::Set;
    using KeyType = void;
    using ValueType = IDLInterface<TestNode>;
};

using TestNodeIterator = JSDOMIterator<JSTestNode, TestNodeIteratorTraits>;
using TestNodeIteratorPrototype = JSDOMIteratorPrototype<JSTestNode, TestNodeIteratorTraits>;

template<>
const JSC::ClassInfo TestNodeIterator::s_info = { "TestNode Iterator", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(TestNodeIterator) };

template<>
const JSC::ClassInfo TestNodeIteratorPrototype::s_info = { "TestNode Iterator", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(TestNodeIteratorPrototype) };

static inline EncodedJSValue jsTestNodePrototypeFunctionEntriesCaller(JSGlobalObject*, CallFrame*, JSTestNode* thisObject, JSC::ThrowScope&)
{
    return JSValue::encode(iteratorCreate<TestNodeIterator>(*thisObject, IterationKind::Value));
}

JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionEntries(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame)
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunctionEntriesCaller>(*lexicalGlobalObject, *callFrame, "entries");
}

static inline EncodedJSValue jsTestNodePrototypeFunctionKeysCaller(JSGlobalObject*, CallFrame*, JSTestNode* thisObject, JSC::ThrowScope&)
{
    return JSValue::encode(iteratorCreate<TestNodeIterator>(*thisObject, IterationKind::Key));
}

JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionKeys(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame)
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunctionKeysCaller>(*lexicalGlobalObject, *callFrame, "keys");
}

static inline EncodedJSValue jsTestNodePrototypeFunctionValuesCaller(JSGlobalObject*, CallFrame*, JSTestNode* thisObject, JSC::ThrowScope&)
{
    return JSValue::encode(iteratorCreate<TestNodeIterator>(*thisObject, IterationKind::Value));
}

JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionValues(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame)
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunctionValuesCaller>(*lexicalGlobalObject, *callFrame, "values");
}

static inline EncodedJSValue jsTestNodePrototypeFunctionForEachCaller(JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame, JSTestNode* thisObject, JSC::ThrowScope& throwScope)
{
    return JSValue::encode(iteratorForEach<TestNodeIterator>(*lexicalGlobalObject, *callFrame, *thisObject, throwScope));
}

JSC::EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionForEach(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame)
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunctionForEachCaller>(*lexicalGlobalObject, *callFrame, "forEach");
}

JSC::JSObject* JSTestNode::serialize(JSGlobalObject& lexicalGlobalObject, JSTestNode& thisObject, JSDOMGlobalObject& globalObject, ThrowScope& throwScope)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    auto* result = constructEmptyObject(&lexicalGlobalObject, globalObject.objectPrototype());

    auto nameValue = jsTestNodeNameGetter(lexicalGlobalObject, thisObject, throwScope);
    throwScope.assertNoException();
    result->putDirect(vm, Identifier::fromString(vm, "name"), nameValue);

    return result;
}

static inline EncodedJSValue jsTestNodePrototypeFunctionToJSONBody(JSGlobalObject* lexicalGlobalObject, CallFrame*, JSTestNode* thisObject, JSC::ThrowScope& throwScope)
{
    return JSValue::encode(JSTestNode::serialize(*lexicalGlobalObject, *thisObject, *thisObject->globalObject(), throwScope));
}

EncodedJSValue JSC_HOST_CALL jsTestNodePrototypeFunctionToJSON(JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame)
{
    return IDLOperation<JSTestNode>::call<jsTestNodePrototypeFunctionToJSONBody>(*lexicalGlobalObject, *callFrame, "toJSON");
}

void JSTestNode::analyzeHeap(JSCell* cell, HeapAnalyzer& analyzer)
{
    auto* thisObject = jsCast<JSTestNode*>(cell);
    analyzer.setWrappedObjectForCell(cell, &thisObject->wrapped());
    if (thisObject->scriptExecutionContext())
        analyzer.setLabelForCell(cell, "url " + thisObject->scriptExecutionContext()->url().string());
    Base::analyzeHeap(cell, analyzer);
}

#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestNode@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore8TestNodeE[]; }
#endif
#endif

JSC::JSValue toJSNewlyCreated(JSC::JSGlobalObject*, JSDOMGlobalObject* globalObject, Ref<TestNode>&& impl)
{

#if ENABLE(BINDING_INTEGRITY)
    void* actualVTablePointer = *(reinterpret_cast<void**>(impl.ptr()));
#if PLATFORM(WIN)
    void* expectedVTablePointer = WTF_PREPARE_VTBL_POINTER_FOR_INSPECTION(__identifier("??_7TestNode@WebCore@@6B@"));
#else
    void* expectedVTablePointer = WTF_PREPARE_VTBL_POINTER_FOR_INSPECTION(&_ZTVN7WebCore8TestNodeE[2]);
#endif

    // If this fails TestNode does not have a vtable, so you need to add the
    // ImplementationLacksVTable attribute to the interface definition
    static_assert(std::is_polymorphic<TestNode>::value, "TestNode is not polymorphic");

    // If you hit this assertion you either have a use after free bug, or
    // TestNode has subclasses. If TestNode has subclasses that get passed
    // to toJS() we currently require TestNode you to opt out of binding hardening
    // by adding the SkipVTableValidation attribute to the interface IDL definition
    RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
#endif
    return createWrapper<TestNode>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::JSGlobalObject* lexicalGlobalObject, JSDOMGlobalObject* globalObject, TestNode& impl)
{
    return wrap(lexicalGlobalObject, globalObject, impl);
}


}
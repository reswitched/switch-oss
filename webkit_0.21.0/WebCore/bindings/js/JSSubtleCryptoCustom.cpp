/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 * Copyright (c) 2017 ACCESS CO., LTD. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSSubtleCrypto.h"

#if ENABLE(SUBTLE_CRYPTO)

#include "CryptoAlgorithm.h"
#include "CryptoAlgorithmParameters.h"
#include "CryptoAlgorithmRegistry.h"
#include "CryptoKeyData.h"
#include "CryptoKeySerializationRaw.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "JSCryptoAlgorithmDictionary.h"
#include "JSCryptoKey.h"
#include "JSCryptoKeyPair.h"
#include "JSCryptoKeySerializationJWK.h"
#include "JSCryptoOperationData.h"
#include "JSDOMPromise.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

enum class CryptoKeyFormat {
    // An unformatted sequence of bytes. Intended for secret keys.
    Raw,

    // The DER encoding of the PrivateKeyInfo structure from RFC 5208.
    PKCS8,

    // The DER encoding of the SubjectPublicKeyInfo structure from RFC 5280.
    SPKI,

    // The key is represented as JSON according to the JSON Web Key format.
    JWK
};

static std::unique_ptr<CryptoAlgorithm> createAlgorithmFromJSValue(ExecState* exec, JSValue value)
{
    CryptoAlgorithmIdentifier algorithmIdentifier;
    if (!JSCryptoAlgorithmDictionary::getAlgorithmIdentifier(exec, value, algorithmIdentifier)) {
        ASSERT(exec->hadException());
        return nullptr;
    }

    auto result = CryptoAlgorithmRegistry::singleton().create(algorithmIdentifier);
    if (!result)
        setDOMException(exec, NOT_SUPPORTED_ERR);
    return result;
}

static bool cryptoKeyFormatFromJSValue(ExecState* exec, JSValue value, CryptoKeyFormat& result)
{
    String keyFormatString = value.toString(exec)->value(exec);
    if (exec->hadException())
        return false;
    if (keyFormatString == "raw")
        result = CryptoKeyFormat::Raw;
    else if (keyFormatString == "pkcs8")
        result = CryptoKeyFormat::PKCS8;
    else if (keyFormatString == "spki")
        result = CryptoKeyFormat::SPKI;
    else if (keyFormatString == "jwk")
        result = CryptoKeyFormat::JWK;
    else {
        throwTypeError(exec, "Unknown key format");
        return false;
    }
    return true;
}

static bool cryptoKeyUsagesFromJSValue(ExecState* exec, JSValue value, CryptoKeyUsage& result)
{
    if (!isJSArray(value)) {
        throwTypeError(exec);
        return false;
    }

    result = 0;

    JSArray* array = asArray(value);
    for (size_t i = 0; i < array->length(); ++i) {
        JSValue element = array->getIndex(exec, i);
        String usageString = element.toString(exec)->value(exec);
        if (exec->hadException())
            return false;
        if (usageString == "encrypt")
            result |= CryptoKeyUsageEncrypt;
        else if (usageString == "decrypt")
            result |= CryptoKeyUsageDecrypt;
        else if (usageString == "sign")
            result |= CryptoKeyUsageSign;
        else if (usageString == "verify")
            result |= CryptoKeyUsageVerify;
        else if (usageString == "deriveKey")
            result |= CryptoKeyUsageDeriveKey;
        else if (usageString == "deriveBits")
            result |= CryptoKeyUsageDeriveBits;
        else if (usageString == "wrapKey")
            result |= CryptoKeyUsageWrapKey;
        else if (usageString == "unwrapKey")
            result |= CryptoKeyUsageUnwrapKey;
    }
    return true;
}

JSValue JSSubtleCrypto::encrypt(ExecState* exec)
{
    if (exec->argumentCount() < 3)
        return exec->vm().throwException(exec, createNotEnoughArgumentsError(exec));

    auto algorithm = createAlgorithmFromJSValue(exec, exec->uncheckedArgument(0));
    if (!algorithm) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    auto parameters = JSCryptoAlgorithmDictionary::createParametersForEncrypt(exec, algorithm->identifier(), exec->uncheckedArgument(0));
    if (!parameters) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    RefPtr<CryptoKey> key = JSCryptoKey::toWrapped(exec->uncheckedArgument(1));
    if (!key)
        return throwTypeError(exec);

    if (!key->allows(CryptoKeyUsageEncrypt)) {
        m_impl->document()->addConsoleMessage(MessageSource::JS, MessageLevel::Error, ASCIILiteral("Key usages do not include 'encrypt'"));
        setDOMException(exec, NOT_SUPPORTED_ERR);
        return jsUndefined();
    }

    CryptoOperationData data;
    if (!cryptoOperationDataFromJSValue(exec, exec->uncheckedArgument(2), data)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    
    JSPromiseDeferred* promiseDeferred = JSPromiseDeferred::create(exec, globalObject());
    DeferredWrapper wrapper(exec, globalObject(), promiseDeferred);
#if !PLATFORM(WKC)
    auto successCallback = [wrapper](const Vector<uint8_t>& result) mutable {
        wrapper.resolve(result);
    };
    auto failureCallback = [wrapper]() mutable {
        wrapper.reject(nullptr);
    };
#else
    std::function<void(const Vector<uint8_t>&)> successCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper](const Vector<uint8_t>& result) mutable {
        wrapper.resolve(result);
    });
    std::function<void()> failureCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper]() mutable {
        wrapper.reject(nullptr);
    });
#endif

    ExceptionCode ec = 0;
    algorithm->encrypt(*parameters, *key, data, WTF::move(successCallback), WTF::move(failureCallback), ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }

    return promiseDeferred->promise();
}

JSValue JSSubtleCrypto::decrypt(ExecState* exec)
{
    if (exec->argumentCount() < 3)
        return exec->vm().throwException(exec, createNotEnoughArgumentsError(exec));

    auto algorithm = createAlgorithmFromJSValue(exec, exec->uncheckedArgument(0));
    if (!algorithm) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    auto parameters = JSCryptoAlgorithmDictionary::createParametersForDecrypt(exec, algorithm->identifier(), exec->uncheckedArgument(0));
    if (!parameters) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    RefPtr<CryptoKey> key = JSCryptoKey::toWrapped(exec->uncheckedArgument(1));
    if (!key)
        return throwTypeError(exec);

    if (!key->allows(CryptoKeyUsageDecrypt)) {
        m_impl->document()->addConsoleMessage(MessageSource::JS, MessageLevel::Error, ASCIILiteral("Key usages do not include 'decrypt'"));
        setDOMException(exec, NOT_SUPPORTED_ERR);
        return jsUndefined();
    }

    CryptoOperationData data;
    if (!cryptoOperationDataFromJSValue(exec, exec->uncheckedArgument(2), data)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    JSPromiseDeferred* promiseDeferred = JSPromiseDeferred::create(exec, globalObject());
    DeferredWrapper wrapper(exec, globalObject(), promiseDeferred);
#if !PLATFORM(WKC)
    auto successCallback = [wrapper](const Vector<uint8_t>& result) mutable {
        wrapper.resolve(result);
    };
    auto failureCallback = [wrapper]() mutable {
        wrapper.reject(nullptr);
    };
#else
    std::function<void(const Vector<uint8_t>&)> successCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper](const Vector<uint8_t>& result) mutable {
        wrapper.resolve(result);
    });
    std::function<void()> failureCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper]() mutable {
        wrapper.reject(nullptr);
    });
#endif

    ExceptionCode ec = 0;
    algorithm->decrypt(*parameters, *key, data, WTF::move(successCallback), WTF::move(failureCallback), ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }

    return promiseDeferred->promise();
}

JSValue JSSubtleCrypto::sign(ExecState* exec)
{
    if (exec->argumentCount() < 3)
        return exec->vm().throwException(exec, createNotEnoughArgumentsError(exec));

    auto algorithm = createAlgorithmFromJSValue(exec, exec->uncheckedArgument(0));
    if (!algorithm) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    auto parameters = JSCryptoAlgorithmDictionary::createParametersForSign(exec, algorithm->identifier(), exec->uncheckedArgument(0));
    if (!parameters) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    RefPtr<CryptoKey> key = JSCryptoKey::toWrapped(exec->uncheckedArgument(1));
    if (!key)
        return throwTypeError(exec);

    if (!key->allows(CryptoKeyUsageSign)) {
        m_impl->document()->addConsoleMessage(MessageSource::JS, MessageLevel::Error, ASCIILiteral("Key usages do not include 'sign'"));
        setDOMException(exec, NOT_SUPPORTED_ERR);
        return jsUndefined();
    }

    CryptoOperationData data;
    if (!cryptoOperationDataFromJSValue(exec, exec->uncheckedArgument(2), data)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    JSPromiseDeferred* promiseDeferred = JSPromiseDeferred::create(exec, globalObject());
    DeferredWrapper wrapper(exec, globalObject(), promiseDeferred);
#if !PLATFORM(WKC)
    auto successCallback = [wrapper](const Vector<uint8_t>& result) mutable {
        wrapper.resolve(result);
    };
    auto failureCallback = [wrapper]() mutable {
        wrapper.reject(nullptr);
    };
#else
    std::function<void(const Vector<uint8_t>&)> successCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper](const Vector<uint8_t>& result) mutable {
        wrapper.resolve(result);
    });
    std::function<void()> failureCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper]() mutable {
        wrapper.reject(nullptr);
    });
#endif

    ExceptionCode ec = 0;
    algorithm->sign(*parameters, *key, data, WTF::move(successCallback), WTF::move(failureCallback), ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }

    return promiseDeferred->promise();
}

JSValue JSSubtleCrypto::verify(ExecState* exec)
{
    if (exec->argumentCount() < 4)
        return exec->vm().throwException(exec, createNotEnoughArgumentsError(exec));

    auto algorithm = createAlgorithmFromJSValue(exec, exec->uncheckedArgument(0));
    if (!algorithm) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    auto parameters = JSCryptoAlgorithmDictionary::createParametersForVerify(exec, algorithm->identifier(), exec->uncheckedArgument(0));
    if (!parameters) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    RefPtr<CryptoKey> key = JSCryptoKey::toWrapped(exec->uncheckedArgument(1));
    if (!key)
        return throwTypeError(exec);

    if (!key->allows(CryptoKeyUsageVerify)) {
        m_impl->document()->addConsoleMessage(MessageSource::JS, MessageLevel::Error, ASCIILiteral("Key usages do not include 'verify'"));
        setDOMException(exec, NOT_SUPPORTED_ERR);
        return jsUndefined();
    }

    CryptoOperationData signature;
    if (!cryptoOperationDataFromJSValue(exec, exec->uncheckedArgument(2), signature)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    CryptoOperationData data;
    if (!cryptoOperationDataFromJSValue(exec, exec->uncheckedArgument(3), data)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    JSPromiseDeferred* promiseDeferred = JSPromiseDeferred::create(exec, globalObject());
    DeferredWrapper wrapper(exec, globalObject(), promiseDeferred);
#if !PLATFORM(WKC)
    auto successCallback = [wrapper](bool result) mutable {
        wrapper.resolve(result);
    };
    auto failureCallback = [wrapper]() mutable {
        wrapper.reject(nullptr);
    };
#else
    std::function<void(bool)> successCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper](bool result) mutable {
        wrapper.resolve(result);
    });
    std::function<void()> failureCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper]() mutable {
        wrapper.reject(nullptr);
    });
#endif

    ExceptionCode ec = 0;
    algorithm->verify(*parameters, *key, signature, data, WTF::move(successCallback), WTF::move(failureCallback), ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }

    return promiseDeferred->promise();
}

JSValue JSSubtleCrypto::digest(ExecState* exec)
{
    if (exec->argumentCount() < 2)
        return exec->vm().throwException(exec, createNotEnoughArgumentsError(exec));

    auto algorithm = createAlgorithmFromJSValue(exec, exec->uncheckedArgument(0));
    if (!algorithm) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    auto parameters = JSCryptoAlgorithmDictionary::createParametersForDigest(exec, algorithm->identifier(), exec->uncheckedArgument(0));
    if (!parameters) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    CryptoOperationData data;
    if (!cryptoOperationDataFromJSValue(exec, exec->uncheckedArgument(1), data)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    JSPromiseDeferred* promiseDeferred = JSPromiseDeferred::create(exec, globalObject());
    DeferredWrapper wrapper(exec, globalObject(), promiseDeferred);
#if !PLATFORM(WKC)
    auto successCallback = [wrapper](const Vector<uint8_t>& result) mutable {
        wrapper.resolve(result);
    };
    auto failureCallback = [wrapper]() mutable {
        wrapper.reject(nullptr);
    };
#else
    std::function<void(const Vector<uint8_t>&)> successCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper](const Vector<uint8_t>& result) mutable {
        wrapper.resolve(result);
    });
    std::function<void()> failureCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper]() mutable {
        wrapper.reject(nullptr);
    });
#endif

    ExceptionCode ec = 0;
    algorithm->digest(*parameters, data, WTF::move(successCallback), WTF::move(failureCallback), ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }

    return promiseDeferred->promise();
}

JSValue JSSubtleCrypto::generateKey(ExecState* exec)
{
    if (exec->argumentCount() < 1)
        return exec->vm().throwException(exec, createNotEnoughArgumentsError(exec));

    auto algorithm = createAlgorithmFromJSValue(exec, exec->uncheckedArgument(0));
    if (!algorithm) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    auto parameters = JSCryptoAlgorithmDictionary::createParametersForGenerateKey(exec, algorithm->identifier(), exec->uncheckedArgument(0));
    if (!parameters) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    bool extractable = false;
    if (exec->argumentCount() >= 2) {
        extractable = exec->uncheckedArgument(1).toBoolean(exec);
        if (exec->hadException())
            return jsUndefined();
    }

    CryptoKeyUsage keyUsages = 0;
    if (exec->argumentCount() >= 3) {
        if (!cryptoKeyUsagesFromJSValue(exec, exec->argument(2), keyUsages)) {
            ASSERT(exec->hadException());
            return jsUndefined();
        }
    }

    JSPromiseDeferred* promiseDeferred = JSPromiseDeferred::create(exec, globalObject());
    DeferredWrapper wrapper(exec, globalObject(), promiseDeferred);
#if !PLATFORM(WKC)
    auto successCallback = [wrapper](CryptoKey* key, CryptoKeyPair* keyPair) mutable {
        ASSERT(key || keyPair);
        ASSERT(!key || !keyPair);
        if (key)
            wrapper.resolve(key);
        else
            wrapper.resolve(keyPair);
    };
    auto failureCallback = [wrapper]() mutable {
        wrapper.reject(nullptr);
    };
#else
    std::function<void(CryptoKey*, CryptoKeyPair*)> successCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper](CryptoKey* key, CryptoKeyPair* keyPair) mutable {
        ASSERT(key || keyPair);
        ASSERT(!key || !keyPair);
        if (key)
            wrapper.resolve(key);
        else
            wrapper.resolve(keyPair);
    });
    std::function<void()> failureCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper]() mutable {
        wrapper.reject(nullptr);
    });
#endif

    ExceptionCode ec = 0;
    algorithm->generateKey(*parameters, extractable, keyUsages, WTF::move(successCallback), WTF::move(failureCallback), ec);
    if (ec) {
        setDOMException(exec, ec);
        return jsUndefined();
    }

    return promiseDeferred->promise();
}

static void importKey(ExecState* exec, CryptoKeyFormat keyFormat, CryptoOperationData data, std::unique_ptr<CryptoAlgorithm> algorithm, std::unique_ptr<CryptoAlgorithmParameters> parameters, bool extractable, CryptoKeyUsage keyUsages, CryptoAlgorithm::KeyCallback callback, CryptoAlgorithm::VoidCallback failureCallback)
{
    std::unique_ptr<CryptoKeySerialization> keySerialization;
    switch (keyFormat) {
    case CryptoKeyFormat::Raw:
        keySerialization = CryptoKeySerializationRaw::create(data);
        break;
    case CryptoKeyFormat::JWK: {
        String jwkString = String::fromUTF8(data.first, data.second);
        if (jwkString.isNull()) {
            throwTypeError(exec, "JWK JSON serialization is not valid UTF-8");
            return;
        }
        keySerialization = std::make_unique<JSCryptoKeySerializationJWK>(exec, jwkString);
        if (exec->hadException())
            return;
        break;
    }
    default:
        throwTypeError(exec, "Unsupported key format for import");
        return;
    }

    ASSERT(keySerialization);

    if (!keySerialization->reconcileAlgorithm(algorithm, parameters)) {
        if (!exec->hadException())
            throwTypeError(exec, "Algorithm specified in key is not compatible with one passed to importKey as argument");
        return;
    }
    if (exec->hadException())
        return;

    if (!algorithm) {
        throwTypeError(exec, "Neither key nor function argument has crypto algorithm specified");
        return;
    }
    ASSERT(parameters);

    keySerialization->reconcileExtractable(extractable);
    if (exec->hadException())
        return;

    keySerialization->reconcileUsages(keyUsages);
    if (exec->hadException())
        return;

    auto keyData = keySerialization->keyData();
    if (exec->hadException())
        return;

    ExceptionCode ec = 0;
    algorithm->importKey(*parameters, *keyData, extractable, keyUsages, WTF::move(callback), WTF::move(failureCallback), ec);
    if (ec)
        setDOMException(exec, ec);
}

JSValue JSSubtleCrypto::importKey(ExecState* exec)
{
    if (exec->argumentCount() < 3)
        return exec->vm().throwException(exec, createNotEnoughArgumentsError(exec));

    CryptoKeyFormat keyFormat;
    if (!cryptoKeyFormatFromJSValue(exec, exec->argument(0), keyFormat)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    CryptoOperationData data;
    if (!cryptoOperationDataFromJSValue(exec, exec->uncheckedArgument(1), data)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    std::unique_ptr<CryptoAlgorithm> algorithm;
    std::unique_ptr<CryptoAlgorithmParameters> parameters;
    if (!exec->uncheckedArgument(2).isNull()) {
        algorithm = createAlgorithmFromJSValue(exec, exec->uncheckedArgument(2));
        if (!algorithm) {
            ASSERT(exec->hadException());
            return jsUndefined();
        }
        parameters = JSCryptoAlgorithmDictionary::createParametersForImportKey(exec, algorithm->identifier(), exec->uncheckedArgument(2));
        if (!parameters) {
            ASSERT(exec->hadException());
            return jsUndefined();
        }
    }

    bool extractable = false;
    if (exec->argumentCount() >= 4) {
        extractable = exec->uncheckedArgument(3).toBoolean(exec);
        if (exec->hadException())
            return jsUndefined();
    }

    CryptoKeyUsage keyUsages = 0;
    if (exec->argumentCount() >= 5) {
        if (!cryptoKeyUsagesFromJSValue(exec, exec->argument(4), keyUsages)) {
            ASSERT(exec->hadException());
            return jsUndefined();
        }
    }

#if PLATFORM(WKC)
    // If data has been already freed, return undefined.
    // This is a workaround instead of https://trac.webkit.org/changeset/216992/webkit.
    if (!cryptoOperationDataFromJSValue(exec, exec->uncheckedArgument(1), data)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }
    if (!data.first)
        return jsUndefined();
#endif

    JSPromiseDeferred* promiseDeferred = JSPromiseDeferred::create(exec, globalObject());
    DeferredWrapper wrapper(exec, globalObject(), promiseDeferred);
#if !PLATFORM(WKC)
    auto successCallback = [wrapper](CryptoKey& result) mutable {
        wrapper.resolve(&result);
    };
    auto failureCallback = [wrapper]() mutable {
        wrapper.reject(nullptr);
    };
#else
    std::function<void(CryptoKey&)> successCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper](CryptoKey& result) mutable {
        wrapper.resolve(&result);
    });
    std::function<void()> failureCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper]() mutable {
        wrapper.reject(nullptr);
    });
#endif

    WebCore::importKey(exec, keyFormat, data, WTF::move(algorithm), WTF::move(parameters), extractable, keyUsages, WTF::move(successCallback), WTF::move(failureCallback));
    if (exec->hadException())
        return jsUndefined();

    return promiseDeferred->promise();
}

static void exportKey(ExecState* exec, CryptoKeyFormat keyFormat, const CryptoKey& key, CryptoAlgorithm::VectorCallback callback, CryptoAlgorithm::VoidCallback failureCallback)
{
    if (!key.extractable()) {
        throwTypeError(exec, "Key is not extractable");
        return;
    }

    switch (keyFormat) {
    case CryptoKeyFormat::Raw: {
        Vector<uint8_t> result;
        if (CryptoKeySerializationRaw::serialize(key, result))
            callback(result);
        else
            failureCallback();
        break;
    }
    case CryptoKeyFormat::JWK: {
        String result = JSCryptoKeySerializationJWK::serialize(exec, key);
        if (exec->hadException())
            return;
        CString utf8String = result.utf8(StrictConversion);
        Vector<uint8_t> resultBuffer;
        resultBuffer.append(utf8String.data(), utf8String.length());
        callback(resultBuffer);
        break;
    }
    default:
        throwTypeError(exec, "Unsupported key format for export");
        break;
    }
}

JSValue JSSubtleCrypto::exportKey(ExecState* exec)
{
    if (exec->argumentCount() < 2)
        return exec->vm().throwException(exec, createNotEnoughArgumentsError(exec));

    CryptoKeyFormat keyFormat;
    if (!cryptoKeyFormatFromJSValue(exec, exec->argument(0), keyFormat)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    RefPtr<CryptoKey> key = JSCryptoKey::toWrapped(exec->uncheckedArgument(1));
    if (!key)
        return throwTypeError(exec);

    JSPromiseDeferred* promiseDeferred = JSPromiseDeferred::create(exec, globalObject());
    DeferredWrapper wrapper(exec, globalObject(), promiseDeferred);
#if !PLATFORM(WKC)
    auto successCallback = [wrapper](const Vector<uint8_t>& result) mutable {
        wrapper.resolve(result);
    };
    auto failureCallback = [wrapper]() mutable {
        wrapper.reject(nullptr);
    };
#else
    std::function<void(const Vector<uint8_t>&)> successCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper](const Vector<uint8_t>& result) mutable {
        wrapper.resolve(result);
    });
    std::function<void()> failureCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper]() mutable {
        wrapper.reject(nullptr);
    });
#endif

    WebCore::exportKey(exec, keyFormat, *key, WTF::move(successCallback), WTF::move(failureCallback));
    if (exec->hadException())
        return jsUndefined();

    return promiseDeferred->promise();
}

JSValue JSSubtleCrypto::wrapKey(ExecState* exec)
{
    if (exec->argumentCount() < 4)
        return exec->vm().throwException(exec, createNotEnoughArgumentsError(exec));

    CryptoKeyFormat keyFormat;
    if (!cryptoKeyFormatFromJSValue(exec, exec->argument(0), keyFormat)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    RefPtr<CryptoKey> key = JSCryptoKey::toWrapped(exec->uncheckedArgument(1));
    if (!key)
        return throwTypeError(exec);

    RefPtr<CryptoKey> wrappingKey = JSCryptoKey::toWrapped(exec->uncheckedArgument(2));
    if (!wrappingKey)
        return throwTypeError(exec);

    if (!wrappingKey->allows(CryptoKeyUsageWrapKey)) {
        m_impl->document()->addConsoleMessage(MessageSource::JS, MessageLevel::Error, ASCIILiteral("Key usages do not include 'wrapKey'"));
        setDOMException(exec, NOT_SUPPORTED_ERR);
        return jsUndefined();
    }

    auto algorithm = createAlgorithmFromJSValue(exec, exec->uncheckedArgument(3));
    if (!algorithm) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    auto parameters = JSCryptoAlgorithmDictionary::createParametersForEncrypt(exec, algorithm->identifier(), exec->uncheckedArgument(3));
    if (!parameters) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    JSPromiseDeferred* promiseDeferred = JSPromiseDeferred::create(exec, globalObject());
    DeferredWrapper wrapper(exec, globalObject(), promiseDeferred);

    CryptoAlgorithm* algorithmPtr = algorithm.release();
    CryptoAlgorithmParameters* parametersPtr = parameters.release();

#if !PLATFORM(WKC)
    auto exportSuccessCallback = [keyFormat, algorithmPtr, parametersPtr, wrappingKey, wrapper](const Vector<uint8_t>& exportedKeyData) mutable {
        auto encryptSuccessCallback = [wrapper, algorithmPtr, parametersPtr](const Vector<uint8_t>& encryptedData) mutable {
            delete algorithmPtr;
            delete parametersPtr;
            wrapper.resolve(encryptedData);
        };
        auto encryptFailureCallback = [wrapper, algorithmPtr, parametersPtr]() mutable {
            delete algorithmPtr;
            delete parametersPtr;
            wrapper.reject(nullptr);
        };
        ExceptionCode ec = 0;
        algorithmPtr->encryptForWrapKey(*parametersPtr, *wrappingKey, std::make_pair(exportedKeyData.data(), exportedKeyData.size()), WTF::move(encryptSuccessCallback), WTF::move(encryptFailureCallback), ec);
        if (ec) {
            // FIXME: Report failure details to console, and possibly to calling script once there is a standardized way to pass errors to WebCrypto promise reject functions.
            encryptFailureCallback();
        }
    };

    auto exportFailureCallback = [wrapper, algorithmPtr, parametersPtr]() mutable {
        delete algorithmPtr;
        delete parametersPtr;
        wrapper.reject(nullptr);
    };
#else
    std::function<void(const Vector<uint8_t>&)> exportSuccessCallback(std::allocator_arg, WTF::voidFuncAllocator(), [keyFormat, algorithmPtr, parametersPtr, wrappingKey, wrapper](const Vector<uint8_t>& exportedKeyData) mutable {
        std::function<void(const Vector<uint8_t>&)> encryptSuccessCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper, algorithmPtr, parametersPtr](const Vector<uint8_t>& encryptedData) mutable {
            delete algorithmPtr;
            delete parametersPtr;
            wrapper.resolve(encryptedData);
        });
        std::function<void()> encryptFailureCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper, algorithmPtr, parametersPtr]() mutable {
            delete algorithmPtr;
            delete parametersPtr;
            wrapper.reject(nullptr);
        });
        ExceptionCode ec = 0;
        algorithmPtr->encryptForWrapKey(*parametersPtr, *wrappingKey, std::make_pair(exportedKeyData.data(), exportedKeyData.size()), WTF::move(encryptSuccessCallback), WTF::move(encryptFailureCallback), ec);
        if (ec) {
            // FIXME: Report failure details to console, and possibly to calling script once there is a standardized way to pass errors to WebCrypto promise reject functions.
            delete algorithmPtr;
            delete parametersPtr;
            wrapper.reject(nullptr);
        }
    });
    std::function<void()> exportFailureCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper, algorithmPtr, parametersPtr]() mutable {
        delete algorithmPtr;
        delete parametersPtr;
        algorithmPtr = nullptr;
        parametersPtr = nullptr;
        wrapper.reject(nullptr);
    });
#endif

    ExceptionCode ec = 0;
    WebCore::exportKey(exec, keyFormat, *key, WTF::move(exportSuccessCallback), WTF::move(exportFailureCallback));
    if (ec) {
        delete algorithmPtr;
        delete parametersPtr;
        setDOMException(exec, ec);
        return jsUndefined();
    }

    return promiseDeferred->promise();
}

JSValue JSSubtleCrypto::unwrapKey(ExecState* exec)
{
    if (exec->argumentCount() < 5)
        return exec->vm().throwException(exec, createNotEnoughArgumentsError(exec));

    CryptoKeyFormat keyFormat;
    if (!cryptoKeyFormatFromJSValue(exec, exec->argument(0), keyFormat)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    CryptoOperationData wrappedKeyData;
    if (!cryptoOperationDataFromJSValue(exec, exec->uncheckedArgument(1), wrappedKeyData)) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    RefPtr<CryptoKey> unwrappingKey = JSCryptoKey::toWrapped(exec->uncheckedArgument(2));
    if (!unwrappingKey)
        return throwTypeError(exec);

    if (!unwrappingKey->allows(CryptoKeyUsageUnwrapKey)) {
        m_impl->document()->addConsoleMessage(MessageSource::JS, MessageLevel::Error, ASCIILiteral("Key usages do not include 'unwrapKey'"));
        setDOMException(exec, NOT_SUPPORTED_ERR);
        return jsUndefined();
    }

    std::unique_ptr<CryptoAlgorithm> unwrapAlgorithm;
    std::unique_ptr<CryptoAlgorithmParameters> unwrapAlgorithmParameters;
    unwrapAlgorithm = createAlgorithmFromJSValue(exec, exec->uncheckedArgument(3));
    if (!unwrapAlgorithm) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }
    unwrapAlgorithmParameters = JSCryptoAlgorithmDictionary::createParametersForDecrypt(exec, unwrapAlgorithm->identifier(), exec->uncheckedArgument(3));
    if (!unwrapAlgorithmParameters) {
        ASSERT(exec->hadException());
        return jsUndefined();
    }

    std::unique_ptr<CryptoAlgorithm> unwrappedKeyAlgorithm;
    std::unique_ptr<CryptoAlgorithmParameters> unwrappedKeyAlgorithmParameters;
    if (!exec->uncheckedArgument(4).isNull()) {
        unwrappedKeyAlgorithm = createAlgorithmFromJSValue(exec, exec->uncheckedArgument(4));
        if (!unwrappedKeyAlgorithm) {
            ASSERT(exec->hadException());
            return jsUndefined();
        }
        unwrappedKeyAlgorithmParameters = JSCryptoAlgorithmDictionary::createParametersForImportKey(exec, unwrappedKeyAlgorithm->identifier(), exec->uncheckedArgument(4));
        if (!unwrappedKeyAlgorithmParameters) {
            ASSERT(exec->hadException());
            return jsUndefined();
        }
    }

    bool extractable = false;
    if (exec->argumentCount() >= 6) {
        extractable = exec->uncheckedArgument(5).toBoolean(exec);
        if (exec->hadException())
            return jsUndefined();
    }

    CryptoKeyUsage keyUsages = 0;
    if (exec->argumentCount() >= 7) {
        if (!cryptoKeyUsagesFromJSValue(exec, exec->argument(6), keyUsages)) {
            ASSERT(exec->hadException());
            return jsUndefined();
        }
    }

    JSPromiseDeferred* promiseDeferred = JSPromiseDeferred::create(exec, globalObject());
    DeferredWrapper wrapper(exec, globalObject(), promiseDeferred);
    Strong<JSDOMGlobalObject> domGlobalObject(exec->vm(), globalObject());

    CryptoAlgorithm* unwrappedKeyAlgorithmPtr = unwrappedKeyAlgorithm.release();
    CryptoAlgorithmParameters* unwrappedKeyAlgorithmParametersPtr = unwrappedKeyAlgorithmParameters.release();

#if !PLATFORM(WKC)
    auto decryptSuccessCallback = [domGlobalObject, keyFormat, unwrappedKeyAlgorithmPtr, unwrappedKeyAlgorithmParametersPtr, extractable, keyUsages, wrapper](const Vector<uint8_t>& result) mutable {
        auto importSuccessCallback = [wrapper](CryptoKey& key) mutable {
            wrapper.resolve(&key);
        };
        auto importFailureCallback = [wrapper]() mutable {
            wrapper.reject(nullptr);
        };
        ExecState* exec = domGlobalObject->globalExec();
        WebCore::importKey(exec, keyFormat, std::make_pair(result.data(), result.size()), std::unique_ptr<CryptoAlgorithm>(unwrappedKeyAlgorithmPtr), std::unique_ptr<CryptoAlgorithmParameters>(unwrappedKeyAlgorithmParametersPtr), extractable, keyUsages, WTF::move(importSuccessCallback), WTF::move(importFailureCallback));
        if (exec->hadException()) {
            // FIXME: Report exception details to console, and possibly to calling script once there is a standardized way to pass errors to WebCrypto promise reject functions.
            exec->clearException();
            importFailureCallback();
        }
    };

    auto decryptFailureCallback = [wrapper, unwrappedKeyAlgorithmPtr, unwrappedKeyAlgorithmParametersPtr]() mutable {
        delete unwrappedKeyAlgorithmPtr;
        delete unwrappedKeyAlgorithmParametersPtr;
        wrapper.reject(nullptr);
    };
#else
    std::function<void(const Vector<uint8_t>&)> decryptSuccessCallback(std::allocator_arg, WTF::voidFuncAllocator(), [domGlobalObject, keyFormat, unwrappedKeyAlgorithmPtr, unwrappedKeyAlgorithmParametersPtr, extractable, keyUsages, wrapper](const Vector<uint8_t>& result) mutable {
        std::function<void(CryptoKey&)> importSuccessCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper](CryptoKey& key) mutable {
            wrapper.resolve(&key);
        });
        std::function<void()> importFailureCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper]() mutable {
            wrapper.reject(nullptr);
        });
        ExecState* exec = domGlobalObject->globalExec();
        WebCore::importKey(exec, keyFormat, std::make_pair(result.data(), result.size()), std::unique_ptr<CryptoAlgorithm>(unwrappedKeyAlgorithmPtr), std::unique_ptr<CryptoAlgorithmParameters>(unwrappedKeyAlgorithmParametersPtr), extractable, keyUsages, WTF::move(importSuccessCallback), WTF::move(importFailureCallback));
        if (exec->hadException()) {
            // FIXME: Report exception details to console, and possibly to calling script once there is a standardized way to pass errors to WebCrypto promise reject functions.
            exec->clearException();
            wrapper.reject(nullptr);
        }
    });
    std::function<void()> decryptFailureCallback(std::allocator_arg, WTF::voidFuncAllocator(), [wrapper, &unwrappedKeyAlgorithmPtr, &unwrappedKeyAlgorithmParametersPtr]() mutable {
        delete unwrappedKeyAlgorithmPtr;
        delete unwrappedKeyAlgorithmParametersPtr;
        unwrappedKeyAlgorithmPtr = nullptr;
        unwrappedKeyAlgorithmParametersPtr = nullptr;
        wrapper.reject(nullptr);
    });
#endif

    ExceptionCode ec = 0;
    unwrapAlgorithm->decryptForUnwrapKey(*unwrapAlgorithmParameters, *unwrappingKey, wrappedKeyData, WTF::move(decryptSuccessCallback), WTF::move(decryptFailureCallback), ec);
    if (ec) {
        delete unwrappedKeyAlgorithmPtr;
        delete unwrappedKeyAlgorithmParametersPtr;
        setDOMException(exec, ec);
        return jsUndefined();
    }

    return promiseDeferred->promise();
}

} // namespace WebCore

#endif

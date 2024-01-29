/*
 * Copyright (C) 2013, 2015 Apple Inc. All Rights Reserved.
 * Copyright (C) 2011 The Chromium Authors. All rights reserved.
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
#include "InspectorBackendDispatcher.h"

#include "InspectorFrontendChannel.h"
#include "InspectorFrontendRouter.h"
#include "InspectorValues.h"
#include <wtf/TemporaryChange.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace Inspector {

SupplementalBackendDispatcher::SupplementalBackendDispatcher(BackendDispatcher& backendDispatcher)
    : m_backendDispatcher(backendDispatcher)
{
}

SupplementalBackendDispatcher::~SupplementalBackendDispatcher()
{
}

BackendDispatcher::CallbackBase::CallbackBase(Ref<BackendDispatcher>&& backendDispatcher, long requestId)
    : m_backendDispatcher(WTF::move(backendDispatcher))
    , m_requestId(requestId)
{
}

bool BackendDispatcher::CallbackBase::isActive() const
{
    return !m_alreadySent && m_backendDispatcher->isActive();
}

void BackendDispatcher::CallbackBase::sendFailure(const ErrorString& error)
{
    ASSERT(error.length());

    if (m_alreadySent)
        return;

    m_alreadySent = true;

    // Immediately send an error message since this is an async response with a single error.
    m_backendDispatcher->reportProtocolError(m_requestId, ServerError, error);
    m_backendDispatcher->sendPendingErrors();
}

void BackendDispatcher::CallbackBase::sendSuccess(RefPtr<InspectorObject>&& partialMessage)
{
    if (m_alreadySent)
        return;

    m_alreadySent = true;
    m_backendDispatcher->sendResponse(m_requestId, WTF::move(partialMessage));
}

BackendDispatcher::BackendDispatcher(Ref<FrontendRouter>&& router)
    : m_frontendRouter(WTF::move(router))
{
}

Ref<BackendDispatcher> BackendDispatcher::create(Ref<FrontendRouter>&& router)
{
    return adoptRef(*new BackendDispatcher(WTF::move(router)));
}

bool BackendDispatcher::isActive() const
{
    return m_frontendRouter->hasFrontends();
}

void BackendDispatcher::registerDispatcherForDomain(const String& domain, SupplementalBackendDispatcher* dispatcher)
{
    ASSERT_ARG(dispatcher, dispatcher);

    // FIXME: <https://webkit.org/b/148492> Agents should only register with the backend once,
    // and we should re-add the assertion that only one dispatcher is registered per domain.
    m_dispatchers.set(domain, dispatcher);
}

void BackendDispatcher::dispatch(const String& message)
{
    Ref<BackendDispatcher> protect(*this);

    ASSERT(!m_protocolErrors.size());

    long requestId = 0;
    RefPtr<InspectorObject> messageObject;

    {
        // In case this is a re-entrant call from a nested run loop, we don't want to lose
        // the outer request's id just because the inner request is bogus.
        TemporaryChange<Optional<long>> scopedRequestId(m_currentRequestId, Nullopt);

        RefPtr<InspectorValue> parsedMessage;
        if (!InspectorValue::parseJSON(message, parsedMessage)) {
            reportProtocolError(ParseError, ASCIILiteral("Message must be in JSON format"));
            sendPendingErrors();
            return;
        }

        if (!parsedMessage->asObject(messageObject)) {
            reportProtocolError(InvalidRequest, ASCIILiteral("Message must be a JSONified object"));
            sendPendingErrors();
            return;
        }

        RefPtr<InspectorValue> requestIdValue;
        if (!messageObject->getValue(ASCIILiteral("id"), requestIdValue)) {
            reportProtocolError(InvalidRequest, ASCIILiteral("'id' property was not found"));
            sendPendingErrors();
            return;
        }

        if (!requestIdValue->asInteger(requestId)) {
            reportProtocolError(InvalidRequest, ASCIILiteral("The type of 'id' property must be integer"));
            sendPendingErrors();
            return;
        }
    }

    {
        // We could be called re-entrantly from a nested run loop, so restore the previous id.
        TemporaryChange<Optional<long>> scopedRequestId(m_currentRequestId, requestId);

        RefPtr<InspectorValue> methodValue;
        if (!messageObject->getValue(ASCIILiteral("method"), methodValue)) {
            reportProtocolError(InvalidRequest, ASCIILiteral("'method' property wasn't found"));
            sendPendingErrors();
            return;
        }

        String methodString;
        if (!methodValue->asString(methodString)) {
            reportProtocolError(InvalidRequest, ASCIILiteral("The type of 'method' property must be string"));
            sendPendingErrors();
            return;
        }

        Vector<String> domainAndMethod;
        methodString.split('.', true, domainAndMethod);
        if (domainAndMethod.size() != 2 || !domainAndMethod[0].length() || !domainAndMethod[1].length()) {
            reportProtocolError(InvalidRequest, ASCIILiteral("The 'method' property was formatted incorrectly. It should be 'Domain.method'"));
            sendPendingErrors();
            return;
        }

        String domain = domainAndMethod[0];
        SupplementalBackendDispatcher* domainDispatcher = m_dispatchers.get(domain);
        if (!domainDispatcher) {
            reportProtocolError(MethodNotFound, "'" + domain + "' domain was not found");
            sendPendingErrors();
            return;
        }

        String method = domainAndMethod[1];
        domainDispatcher->dispatch(requestId, method, messageObject.releaseNonNull());

        if (m_protocolErrors.size())
            sendPendingErrors();
    }
}

void BackendDispatcher::sendResponse(long requestId, RefPtr<InspectorObject>&& result)
{
    ASSERT(!m_protocolErrors.size());

    // The JSON-RPC 2.0 specification requires that the "error" member have the value 'null'
    // if no error occurred during an invocation, but we do not include it at all.
    Ref<InspectorObject> responseMessage = InspectorObject::create();
    responseMessage->setObject(ASCIILiteral("result"), result);
    responseMessage->setInteger(ASCIILiteral("id"), requestId);
    m_frontendRouter->sendResponse(responseMessage->toJSONString());
}

void BackendDispatcher::sendPendingErrors()
{
    // These error codes are specified in JSON-RPC 2.0, Section 5.1.
    static const int errorCodes[] = {
        -32700, // ParseError
        -32600, // InvalidRequest
        -32601, // MethodNotFound
        -32602, // InvalidParams
        -32603, // InternalError
        -32000, // ServerError
    };

    // To construct the error object, only use the last error's code and message.
    // Per JSON-RPC 2.0, Section 5.1, the 'data' member may contain nested errors,
    // but only one top-level Error object should be sent per request.
    CommonErrorCode errorCode;
    String errorMessage;
    Ref<InspectorArray> payload = InspectorArray::create();
    
    for (auto& data : m_protocolErrors) {
        errorCode = std::get<0>(data);
        errorMessage = std::get<1>(data);

        ASSERT_ARG(errorCode, (unsigned)errorCode < WTF_ARRAY_LENGTH(errorCodes));
        ASSERT_ARG(errorCode, errorCodes[errorCode]);

        Ref<InspectorObject> error = InspectorObject::create();
        error->setInteger(ASCIILiteral("code"), errorCodes[errorCode]);
        error->setString(ASCIILiteral("message"), errorMessage);
        payload->pushObject(WTF::move(error));
    }

    Ref<InspectorObject> topLevelError = InspectorObject::create();
    topLevelError->setInteger(ASCIILiteral("code"), errorCodes[errorCode]);
    topLevelError->setString(ASCIILiteral("message"), errorMessage);
    topLevelError->setArray(ASCIILiteral("data"), WTF::move(payload));

    Ref<InspectorObject> message = InspectorObject::create();
    message->setObject(ASCIILiteral("error"), WTF::move(topLevelError));
    if (m_currentRequestId)
        message->setInteger(ASCIILiteral("id"), m_currentRequestId.value());
    else {
        // The 'null' value for an unknown id is specified in JSON-RPC 2.0, Section 5.
        message->setValue(ASCIILiteral("id"), InspectorValue::null());
    }

    m_frontendRouter->sendResponse(message->toJSONString());

    m_protocolErrors.clear();
    m_currentRequestId = Nullopt;
}
    
void BackendDispatcher::reportProtocolError(CommonErrorCode errorCode, const String& errorMessage)
{
    reportProtocolError(m_currentRequestId, errorCode, errorMessage);
}

void BackendDispatcher::reportProtocolError(Optional<long> relatedRequestId, CommonErrorCode errorCode, const String& errorMessage)
{
    ASSERT_ARG(errorCode, errorCode >= 0);

    // If the error was reported from an async callback, then no request id will be registered yet.
    if (!m_currentRequestId)
        m_currentRequestId = relatedRequestId;

    m_protocolErrors.append(std::tuple<CommonErrorCode, String>(errorCode, errorMessage));
}

template<typename T>
T BackendDispatcher::getPropertyValue(InspectorObject* object, const String& name, bool* out_optionalValueFound, T defaultValue, std::function<bool(InspectorValue&, T&)> asMethod, const char* typeName)
{
    T result(defaultValue);
    // out_optionalValueFound signals to the caller whether an optional property was found.
    // if out_optionalValueFound == nullptr, then this is a required property.
    if (out_optionalValueFound)
        *out_optionalValueFound = false;

    if (!object) {
        if (!out_optionalValueFound)
            reportProtocolError(BackendDispatcher::InvalidParams, String::format("'params' object must contain required parameter '%s' with type '%s'.", name.utf8().data(), typeName));
        return result;
    }

    auto findResult = object->find(name);
    if (findResult == object->end()) {
        if (!out_optionalValueFound)
            reportProtocolError(BackendDispatcher::InvalidParams, String::format("Parameter '%s' with type '%s' was not found.", name.utf8().data(), typeName));
        return result;
    }

    if (!asMethod(*findResult->value, result)) {
        reportProtocolError(BackendDispatcher::InvalidParams, String::format("Parameter '%s' has wrong type. It must be '%s'.", name.utf8().data(), typeName));
        return result;
    }

    if (out_optionalValueFound)
        *out_optionalValueFound = true;

    return result;
}

static bool castToInteger(InspectorValue& value, int& result) { return value.asInteger(result); }
static bool castToNumber(InspectorValue& value, double& result) { return value.asDouble(result); }
#if PLATFORM(WKC)
static bool castToString(InspectorValue& value, String& result) { return value.asString(result); }
static bool castToBoolean(InspectorValue& value, bool& result) { return value.asBoolean(result); }
static bool castToObject(InspectorValue& value, RefPtr<InspectorObject>& result) { return value.asObject(result); }
static bool castToArray(InspectorValue& value, RefPtr<InspectorArray>& output) { return value.asArray(output); }
static bool castToValue(InspectorValue& value, RefPtr<InspectorValue>& output) { return value.asValue(output); }
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1800
static bool asString(InspectorValue& value, String& output) { return value.asString(output); }
static bool asBoolean(InspectorValue& value, bool& output) { return value.asBoolean(output); }
static bool asObject(InspectorValue& value, RefPtr<InspectorObject>& output) { return value.asObject(output); }
static bool asArray(InspectorValue& value, RefPtr<InspectorArray>& output) { return value.asArray(output); }
static bool asValue(InspectorValue& value, RefPtr<InspectorValue>& output) { return value.asValue(output); }
#endif

int BackendDispatcher::getInteger(InspectorObject* object, const String& name, bool* valueFound)
{
    return getPropertyValue<int>(object, name, valueFound, 0, &castToInteger, "Integer");
}

double BackendDispatcher::getDouble(InspectorObject* object, const String& name, bool* valueFound)
{
    return getPropertyValue<double>(object, name, valueFound, 0, &castToNumber, "Number");
}

String BackendDispatcher::getString(InspectorObject* object, const String& name, bool* valueFound)
{
#if !PLATFORM(WKC)
#if !defined(_MSC_VER) || _MSC_VER > 1800
    return getPropertyValue<String>(object, name, valueFound, "", &InspectorValue::asString, "String");
#else
    return getPropertyValue<String>(object, name, valueFound, "", &asString, "String");
#endif
#else
    return getPropertyValue<String>(object, name, valueFound, "", &castToString, "String");
#endif
}

bool BackendDispatcher::getBoolean(InspectorObject* object, const String& name, bool* valueFound)
{
#if !PLATFORM(WKC)
#if !defined(_MSC_VER) || _MSC_VER > 1800
    return getPropertyValue<bool>(object, name, valueFound, false, &InspectorValue::asBoolean, "Boolean");
#else
    return getPropertyValue<bool>(object, name, valueFound, false, &asBoolean, "Boolean");
#endif
#else
    return getPropertyValue<bool>(object, name, valueFound, false, &castToBoolean, "Boolean");
#endif
}

RefPtr<InspectorObject> BackendDispatcher::getObject(InspectorObject* object, const String& name, bool* valueFound)
{
#if !PLATFORM(WKC)
#if !defined(_MSC_VER) || _MSC_VER > 1800
    return getPropertyValue<RefPtr<InspectorObject>>(object, name, valueFound, nullptr, &InspectorValue::asObject, "Object");
#else
    return getPropertyValue<RefPtr<InspectorObject>>(object, name, valueFound, nullptr, &asObject, "Object");
#endif
#else
    return getPropertyValue<RefPtr<InspectorObject>>(object, name, valueFound, nullptr, &castToObject, "Object");
#endif
}

RefPtr<InspectorArray> BackendDispatcher::getArray(InspectorObject* object, const String& name, bool* valueFound)
{
#if !PLATFORM(WKC)
#if !defined(_MSC_VER) || _MSC_VER > 1800
    return getPropertyValue<RefPtr<InspectorArray>>(object, name, valueFound, nullptr, &InspectorValue::asArray, "Array");
#else
    return getPropertyValue<RefPtr<InspectorArray>>(object, name, valueFound, nullptr, &asArray, "Array");
#endif
#else
    return getPropertyValue<RefPtr<InspectorArray>>(object, name, valueFound, nullptr, &castToArray, "Array");
#endif
}

RefPtr<InspectorValue> BackendDispatcher::getValue(InspectorObject* object, const String& name, bool* valueFound)
{
#if !PLATFORM(WKC)
#if !defined(_MSC_VER) || _MSC_VER > 1800
    return getPropertyValue<RefPtr<InspectorValue>>(object, name, valueFound, nullptr, &InspectorValue::asValue, "Value");
#else
    return getPropertyValue<RefPtr<InspectorValue>>(object, name, valueFound, nullptr, &asValue, "Value");
#endif
#else
    return getPropertyValue<RefPtr<InspectorValue>>(object, name, valueFound, nullptr, &castToValue, "Value");
#endif
}

} // namespace Inspector

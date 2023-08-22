/*
 *  JSWebNfcAdapterCustom.cpp
 *
 *  Copyright(c) 2015 ACCESS CO., LTD. All rights reserved.
 *
 */

#include "config.h"

#include "JSWebNfcAdapter.h"

#if ENABLE(WKC_WEB_NFC)

#include "Dictionary.h"
#include "FileReaderSync.h"
#include "WebNfcMessage.h"
#include "JSDOMPromise.h"

#include "JSCJSValue.h"
#include "JSWebNfcMessage.h"
#include "JSBlob.h"
#include "JSDOMError.h"
#include "JSONObject.h"

using namespace JSC;

namespace WebCore {

WebNfcMessageDataType dataType(String& data)
{
    WebNfcMessageDataType type;

    ExceptionCode ec = 0;
    RefPtr<DOMURL> tmpUrl = DOMURL::create(data, ec);
    if (ec == 0) {
        type = WebNfcMessageDataTypeURL;
    } else if (data == "[object Blob]") {
        type = WebNfcMessageDataTypeBlob;
    } else if (data == "[object Object]") {
        type = WebNfcMessageDataTypeJSON;
    } else {
        type = WebNfcMessageDataTypeString;
    }

    return type;
}

const String validateOptions(const Dictionary& options)
{
    String target;
    options.get("target", target);

    if (target.isEmpty() || target.isNull()) {
        return String("any");
    } else if(target == "tag" || target == "peer" || target == "any") {
        return target;
    }

    return String();
}

PassRefPtr<WebNfcMessage> validateMessage(ExecState* exec, Document* document, const Dictionary& message)
{
    String scope;
    message.get("scope", scope);
    if (scope.isEmpty() || scope.isNull()) {
        scope = document->baseURL().string();
    }

    Vector<String> datas;
    message.get("data", datas);

    WebNfcMessageDataType type;
    if (datas.size() > 0) {
        type = dataType(datas.at(0));
    } else {
        return 0;
    }

    for (int i = 1; i < datas.size(); ++i) {
        if (dataType(datas.at(i)) != type) {
            return 0;
        }
    }

    RefPtr<WebNfcMessage> webNfcMessage;
    switch (type) {
    case WebNfcMessageDataTypeString:
        webNfcMessage = WebNfcMessage::create(scope, datas, WebNfcMessageDataTypeString);
        break;
    case WebNfcMessageDataTypeURL:
        {
            Vector<RefPtr<DOMURL> > urls;
            for (Vector<String>::iterator it = datas.begin(); it != datas.end(); ++it) {
                ExceptionCode ec = 0;
                RefPtr<DOMURL> url = DOMURL::create((*it), ec);
                if (ec) {
                    return 0;
                }
                urls.append(url);
            }

            webNfcMessage = WebNfcMessage::create(scope, urls);
            break;
        }
    case WebNfcMessageDataTypeBlob:
        {
            Vector<Vector<char> > blobDatas;
            Vector<String> contentTypes;

            Deprecated::ScriptValue value;
            message.get("data", value);

            JSArray* array = asArray(value.jsValue());

            RefPtr<FileReaderSync> rader = FileReaderSync::create();

            for (int i = 0; i < array->getArrayLength(); ++i) {
                JSValue arrayData = array->getIndexQuickly(i);
                Blob* blob =JSBlob::toWrapped(arrayData);

                ExceptionCode ec = 0;
                RefPtr<ArrayBuffer> buffer = rader->readAsArrayBuffer(document, blob, ec);
                if (ec) {
                    return 0;
                }

                int bufferLen = buffer->byteLength();
                char* data = (char*)buffer->data();

                Vector<char> vector;
                for (int j = 0; j < bufferLen; ++j) {
                    vector.append(data[j]);
                }

                blobDatas.append(vector);
                contentTypes.append(blob->type());
            }

            webNfcMessage = WebNfcMessage::create(scope, blobDatas, contentTypes);
            break;
        }
    case WebNfcMessageDataTypeJSON:
        {
            Vector<String> datas;

            Deprecated::ScriptValue value;
            message.get("data", value);

            JSArray* array = asArray(value.jsValue());

            for (int i = 0; i < array->getArrayLength(); ++i) {
                JSValue arrayData = array->getIndexQuickly(i);
                String json = JSONStringify(exec, value.jsValue(), 0);
                datas.append(json);
            }

            webNfcMessage = WebNfcMessage::create(scope, datas, WebNfcMessageDataTypeJSON);
            break;
        }
    default:
        return 0;
    }

    return webNfcMessage;
}

void rejectSyntaxError(DeferredWrapper* deferredWrapper) {
    RefPtr<DOMError> error = DOMError::create("Syntax Error");
    deferredWrapper->reject(error.get());
}

JSValue JSWebNfcAdapter::send(ExecState* exec)
{
    JSPromiseDeferred* promiseDeferred = JSPromiseDeferred::create(exec, globalObject());
    DeferredWrapper* deferredWrapper = new DeferredWrapper(exec, globalObject(), promiseDeferred);

    if (exec->argumentCount() < 1) {
        rejectSyntaxError(deferredWrapper);
        return promiseDeferred->promise();
    }

    Dictionary message(exec, exec->argument(0));
    if (message.isUndefinedOrNull()) {
        rejectSyntaxError(deferredWrapper);
        return promiseDeferred->promise();
    }

    Dictionary options;
    if (exec->argumentCount() == 2) {
        options = Dictionary(exec, exec->argument(1));
    } else {
        options = Dictionary(exec, jsNull());
    }

    String target = validateOptions(options);
    if (target.isEmpty()) {
        rejectSyntaxError(deferredWrapper);
        return promiseDeferred->promise();
    }
    RefPtr<WebNfcMessage> webNfcMessage = validateMessage(exec, impl().document(), message);
    if (!webNfcMessage) {
        rejectSyntaxError(deferredWrapper);
        return promiseDeferred->promise();
    }

    impl().send(deferredWrapper, webNfcMessage, target);

    return promiseDeferred->promise();
}

}; // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)

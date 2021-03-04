/*
 *  JSWebNfcMessageCustom.cpp
 *
 *  Copyright(c) 2015 ACCESS CO., LTD. All rights reserved.
 *
 */

#include "config.h"

#include "JSWebNfcMessage.h"

#if ENABLE(WKC_WEB_NFC)

#include "WebNfcMessage.h"
#include "Blob.h"

#include "JSDOMBinding.h"
#include "ScriptExecutionContext.h"
#include <runtime/JSArray.h>
#include <wtf/GetPtr.h>
#include "JSDOMURL.h"
#include "JSBlob.h"
#include "JSONObject.h"

namespace WebCore {
using namespace JSC;

JSValue JSWebNfcMessage::data(ExecState* exec) const
{
    WebNfcMessage& imp = impl();
    switch (imp.dataType()) {
    case WebNfcMessageDataTypeString:
        return jsArray(exec, globalObject(), imp.stringDatas());
    case WebNfcMessageDataTypeURL:
        return jsArray(exec, globalObject(), imp.urlDatas());
    case WebNfcMessageDataTypeBlob:
        {
            WTF::Vector<RefPtr<Blob> > vector;
            for (int i = 0; i < imp.blobDatas().size(); ++i) {
                RefPtr<Blob> blob = Blob::create(imp.blobDatas().at(i), imp.blobContentTypes().at(i));
                vector.append(blob);
            }
            return jsArray(exec, globalObject(), vector);
        }
    case WebNfcMessageDataTypeJSON:
        {
            WTF::Vector<JSValue> vector;
            for (int i = 0; i < imp.stringDatas().size(); ++i) {
                vector.append(JSONParse(exec, imp.stringDatas().at(i)));
            }

            JSC::MarkedArgumentBuffer list;
            for (auto& element : vector)
                list.append(element);
            return JSC::constructArray(exec, 0, globalObject(), list);
        }
    default:
        break;
    }
    return jsNull();
}

} // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)

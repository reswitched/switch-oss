/*
 *  JSWebNfcCustom.cpp
 *
 *  Copyright(c) 2015 ACCESS CO., LTD. All rights reserved.
 *
 */

#include "config.h"

#include "JSWebNfc.h"

#if ENABLE(WKC_WEB_NFC)

#include "JSDOMPromise.h"

namespace WebCore {
using namespace JSC;

JSValue JSWebNfc::requestAdapter(ExecState* exec)
{
    JSPromiseDeferred* promiseDeferred = JSPromiseDeferred::create(exec, globalObject());
    DeferredWrapper* deferredWrapper = new DeferredWrapper(exec, globalObject(), promiseDeferred);
    impl().requestAdapter(deferredWrapper);
    return promiseDeferred->promise();
}

}; // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)

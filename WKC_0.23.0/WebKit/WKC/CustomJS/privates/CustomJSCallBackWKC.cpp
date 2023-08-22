/*
 * CustomJSCallBackWKC.cpp
 *
 * Copyright (c) 2011-2013 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"

#ifdef WKC_ENABLE_CUSTOMJS
#include "CustomJS/privates/CustomJSCallBackWKC.h"
#include <wkc/wkccustomjs.h>
#include <wkc/wkccustomjspeer.h>

#include <JSStringRef.h>
#include <APICast.h>
#include <JSDOMWindow.h>
#include <Frame.h>

#include "FrameLoaderClientWKC.h"
#include "WKCWebFrame.h"

//local function proto type
static void CustomJSArgsListAdds(JSContextRef ctx, size_t argLen, const JSValueRef args[], WKCCustomJSAPIArgs *apiArgs);
static void CustomJSArgsListAddNull(JSContextRef ctx, const JSValueRef args, WKCCustomJSAPIArgs *apiArgs);
static void CustomJSArgsListAddUndefined(JSContextRef ctx, const JSValueRef args, WKCCustomJSAPIArgs *apiArgs);
static void CustomJSArgsListAddBool(JSContextRef ctx, const JSValueRef args, WKCCustomJSAPIArgs *apiArgs);
static void CustomJSArgsListAddNum(JSContextRef ctx, const JSValueRef args, WKCCustomJSAPIArgs *apiArgs);
static void CustomJSArgsListAddStr(JSContextRef ctx, const JSValueRef args, WKCCustomJSAPIArgs *apiArgs);
static void CustomJSFuncArgsFree(size_t argLen, WKCCustomJSAPIArgs *apiArgs);

namespace WKC {

JSValueRef
CustomJSCallBackWKC(JSContextRef ctx, JSObjectRef jobj, JSObjectRef jobjThis, size_t argLen, const JSValueRef args[], JSValueRef* jobjExp)
{
    JSC::JSObject* jsObj = toJS(jobj);
    JSC::ExecState* exec = toJS(ctx);
    JSC::JSObject* jsWindowObj = toJS(jobjThis);
    const WTF::String& ustr = jsObj->toString(exec)->value(exec);

    int apiRet = 0;
    char* funcName = 0;
    WKCCustomJSAPIArgs *apiArgs = 0;
    WKCCustomJSAPIPtr apiPtr = 0;
    WKCCustomJSAPIList* api = 0;
    void* context = 0;

    WebCore::JSDOMWindow* jsDOMWindow = 0;
    WebCore::Frame* coreFrame = 0;
    FrameLoaderClientWKC* client = 0;
    WKCWebFrame* frame = 0;
    size_t funcNameSt, funcNameEd, funcNameLen;
    bool isExternalApi = false;

    jsDOMWindow = WebCore::toJSDOMWindow(jsWindowObj);
    if (!jsDOMWindow)
        goto end;
    coreFrame = jsDOMWindow->impl().frame();
    if (!coreFrame)
        goto end;
    client = static_cast<FrameLoaderClientWKC*>(&coreFrame->loader().client());
    if (!client)
        goto end;
    frame = client->webFrame();
    if (!frame)
        goto end;

    //pick up function name
    funcNameSt = ustr.find(" ") + 1;
    funcNameEd = ustr.find('(');
    funcNameLen = funcNameEd - funcNameSt;
    funcName = (char*)fastMalloc(funcNameLen + 1);
    memset(funcName, '\0', funcNameLen + 1);
    memcpy(funcName, (ustr.utf8().data() + funcNameSt), funcNameLen);

    //copy args
    if (argLen) {
        apiArgs = (WKCCustomJSAPIArgs *)fastMalloc( argLen * sizeof(WKCCustomJSAPIArgs) );
        CustomJSArgsListAdds(ctx, argLen, args, apiArgs);
    }

    //registered Search
    api = frame->getCustomJSAPIInternal(funcName);
    if (!api || !api->CustomJSAPI) {
        api = frame->getCustomJSAPI(funcName);
        if (!api || !api->CustomJSAPI)
            goto end;
        isExternalApi = true;
    }
    apiPtr = api->CustomJSAPI;

    //execute callback
    if (client->dispatchWillCallCustomJS(api, &context)) {
        if (isExternalApi) {
            apiRet = wkcCustomJSCallExtLibPeer(context, apiPtr, argLen, apiArgs);
        } else {
            apiRet = (*apiPtr)(context, argLen, apiArgs);
        }
    }

end:
    //memory free
    if (apiArgs)
        CustomJSFuncArgsFree(argLen, apiArgs);
    if (funcName)
        fastFree(funcName);

    return JSValueMakeNumber(ctx, (double)apiRet);
}

JSValueRef
CustomJSStringCallBackWKC(JSContextRef ctx, JSObjectRef jobj, JSObjectRef jobjThis, size_t argLen, const JSValueRef args[], JSValueRef* jobjExp)
{
    JSC::JSObject* jsObj = toJS(jobj);
    JSC::ExecState* exec = toJS(ctx);
    JSC::JSObject* jsWindowObj = toJS(jobjThis);
    JSC::JSString* jstr = jsObj->toString(exec);
    WTF::String ustr = jstr->tryGetValue();

    char* stringApiRet = 0;
    char* funcName = 0;
    WKCCustomJSAPIArgs *apiArgs = 0;
    WKCCustomJSStringAPIPtr stringApiPtr = 0;
    WKCCustomJSStringFreePtr stringFreePtr = 0;
    WKCCustomJSAPIList* api = 0;
    void* context = 0;

    WebCore::JSDOMWindow* jsDOMWindow = 0;
    WebCore::Frame* coreFrame = 0;
    FrameLoaderClientWKC* client = 0;
    WKCWebFrame* frame = 0;
    size_t funcNameSt, funcNameEd, funcNameLen;
    bool isExternalApi = false;

    jsDOMWindow = WebCore::toJSDOMWindow(jsWindowObj);
    if (!jsDOMWindow)
        goto end;
    coreFrame = jsDOMWindow->impl().frame();
    if (!coreFrame)
        goto end;
    client = static_cast<FrameLoaderClientWKC*>(&coreFrame->loader().client());
    if (!client)
        goto end;
    frame = client->webFrame();
    if (!frame)
        goto end;

    //pick up function name
    funcNameSt = ustr.find(" ") + 1;
    funcNameEd = ustr.find('(');
    funcNameLen = funcNameEd - funcNameSt;
    funcName = (char*)fastMalloc(funcNameLen + 1);
    memset(funcName, '\0', funcNameLen + 1);
    memcpy(funcName, (ustr.utf8().data() + funcNameSt), funcNameLen);

    //copy args
    if (argLen) {
        apiArgs = (WKCCustomJSAPIArgs *)fastMalloc( argLen * sizeof(WKCCustomJSAPIArgs) );
        CustomJSArgsListAdds(ctx, argLen, args, apiArgs);
    }

    //registered Search
    api = frame->getCustomJSStringAPIInternal(funcName);
    if (!api || !api->CustomJSStringAPI || !api->CustomJSStringFree) {
        api = frame->getCustomJSStringAPI(funcName);
        if (!api || !api->CustomJSStringAPI || !api->CustomJSStringFree)
            goto end;
        isExternalApi = true;
    }
    stringApiPtr = api->CustomJSStringAPI;
    stringFreePtr = api->CustomJSStringFree;

    //execute callback
    if (client->dispatchWillCallCustomJS(api, &context)) {
        if (isExternalApi) {
            stringApiRet = wkcCustomJSStringCallExtLibPeer(context, stringApiPtr, argLen, apiArgs);
        } else {
            stringApiRet = (*stringApiPtr)(context, argLen, apiArgs);
        }
    }

end:
    //make js string
    JSStringRef strBuf= JSStringCreateWithUTF8CString(stringApiRet);
    JSValueRef result = JSValueMakeString(ctx, strBuf);
    JSStringRelease(strBuf);

    //memory free
    if (stringApiRet)
        (*stringFreePtr)(context, stringApiRet);

    if (apiArgs)
        CustomJSFuncArgsFree(argLen, apiArgs);
    if (funcName)
        fastFree(funcName);

    return result;
}

} // namespace

static void
CustomJSArgsListAdds(JSContextRef ctx, size_t argLen, const JSValueRef args[], WKCCustomJSAPIArgs *apiArgs)
{
        JSType argType;
        for( int i = 0; i < argLen; i++ ) {
            argType = JSValueGetType(ctx, args[i]);
            switch (argType) {
            case kJSTypeNull:
                CustomJSArgsListAddNull(ctx, args[i], &apiArgs[i]);
                break;
            case kJSTypeUndefined:
                CustomJSArgsListAddUndefined(ctx, args[i], &apiArgs[i]);
                break;
            case kJSTypeBoolean:
                CustomJSArgsListAddBool(ctx, args[i], &apiArgs[i]);
                break;
            case kJSTypeNumber:
                CustomJSArgsListAddNum(ctx, args[i], &apiArgs[i]);
                break;
            case kJSTypeString:
                CustomJSArgsListAddStr( ctx, args[i], &apiArgs[i]);
                break;
            case kJSTypeObject:
                // fall through
            default:
                //unsupport
                apiArgs[i].typeID = WKC_CUSTOMJS_ARG_TYPE_UNSUPPORT;
                break;
            }
        }
}

static void
CustomJSArgsListAddNull(JSContextRef ctx, const JSValueRef args, WKCCustomJSAPIArgs *apiArgs)
{
    apiArgs->typeID = WKC_CUSTOMJS_ARG_TYPE_NULL;
}

static void
CustomJSArgsListAddUndefined(JSContextRef ctx, const JSValueRef args, WKCCustomJSAPIArgs *apiArgs)
{
    apiArgs->typeID = WKC_CUSTOMJS_ARG_TYPE_UNDEFINED;
}

static void
CustomJSArgsListAddBool(JSContextRef ctx, const JSValueRef args, WKCCustomJSAPIArgs *apiArgs)
{
    bool flg = JSValueToBoolean(ctx, args);

    apiArgs->typeID = WKC_CUSTOMJS_ARG_TYPE_BOOLEAN;
    apiArgs->arg.booleanData = flg;
}

static void
CustomJSArgsListAddNum(JSContextRef ctx, const JSValueRef args, WKCCustomJSAPIArgs *apiArgs)
{
    JSValueRef exception;

    double num = JSValueToNumber(ctx, args, &exception);

    apiArgs->typeID = WKC_CUSTOMJS_ARG_TYPE_DOUBLE;
    apiArgs->arg.doubleData = num;
}

static void
CustomJSArgsListAddStr(JSContextRef ctx, const JSValueRef args, WKCCustomJSAPIArgs *apiArgs)
{
    JSValueRef exception;

    JSStringRef jstrArg = JSValueToStringCopy(ctx, args, &exception);
    size_t len = JSStringGetMaximumUTF8CStringSize(jstrArg);

    void *apiArg = fastMalloc(len);
    memset(apiArg, 0x00, len);
    JSStringGetUTF8CString(jstrArg, (char *)apiArg, len);

    apiArgs->typeID = WKC_CUSTOMJS_ARG_TYPE_CHAR;
    apiArgs->arg.charPtr = (char *)apiArg;

    JSStringRelease(jstrArg);
}

static void
CustomJSFuncArgsFree(size_t argLen, WKCCustomJSAPIArgs *apiArgs)
{
    for( int i = 0; i < argLen; i++ ) {
        if (WKC_CUSTOMJS_ARG_TYPE_CHAR == apiArgs[i].typeID)
            fastFree((void *)apiArgs[i].arg.charPtr);
    }
    fastFree(apiArgs);
}
#endif // WKC_ENABLE_CUSTOMJS

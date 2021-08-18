/*
 *  wkccustomjs.h
 *
 *  Copyright(c) 2011, 2012 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKC_CUSTOMJS_H_
#define _WKC_CUSTOMJS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define WKC_CUSTOMJS_FUNCTION_NAME_LENGTH_MAX 64

enum {
    WKC_CUSTOMJS_ARG_TYPE_NULL = 0,
    WKC_CUSTOMJS_ARG_TYPE_UNDEFINED,
    WKC_CUSTOMJS_ARG_TYPE_BOOLEAN,
    WKC_CUSTOMJS_ARG_TYPE_DOUBLE,
    WKC_CUSTOMJS_ARG_TYPE_CHAR,
    WKC_CUSTOMJS_ARG_TYPE_UNSUPPORT,
    WKC_CUSTOMJS_ARG_TYPES
};

struct WKCCustomJSAPIArgs_ {

    int typeID; // WKC_CUSTOMJS_ARG_TYPE_XXX
    union {
        double     doubleData;
        bool       booleanData;
        char       *charPtr;
    } arg;

};
typedef struct WKCCustomJSAPIArgs_ WKCCustomJSAPIArgs;

// for integer
typedef int (*WKCCustomJSAPIPtr) (void* context, int argn, WKCCustomJSAPIArgs *args);
// for string
typedef char* (*WKCCustomJSStringAPIPtr) (void* context, int argn, WKCCustomJSAPIArgs *args);
typedef void (*WKCCustomJSStringFreePtr) (void* context, char* str);

struct WKCCustomJSAPIList_ {

    char CustomJSName[WKC_CUSTOMJS_FUNCTION_NAME_LENGTH_MAX];

    WKCCustomJSAPIPtr CustomJSAPI;

    WKCCustomJSStringAPIPtr CustomJSStringAPI;
    WKCCustomJSStringFreePtr CustomJSStringFree;

};
typedef struct WKCCustomJSAPIList_ WKCCustomJSAPIList;

#ifdef __cplusplus
}
#endif

#endif /* _WKC_CUSTOMJS_H_ */


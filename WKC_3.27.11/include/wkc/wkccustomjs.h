/*
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
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


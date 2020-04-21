/*
 *  wkccustomjspeer.h
 *
 *  Copyright(c) 2011, 2012 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKC_CUSTOMJSPEER_H_
#define _WKC_CUSTOMJSPEER_H_

#include <wkc/wkcbase.h>

#ifdef WKC_ENABLE_CUSTOMJS
#include <wkc/wkccustomjs.h>

/**
   @file
   @brief CustomJS related peers.
*/
/*@{*/

WKC_BEGIN_C_LINKAGE

/**
@brief Calls extend library
@param context Context
@param apiPtr Pointer to extend API
@param argLen Length of arguments
@param args Arguments
@return Return value from extend API
*/
WKC_PEER_API int wkcCustomJSCallExtLibPeer(void* context, WKCCustomJSAPIPtr apiPtr, int argLen, WKCCustomJSAPIArgs *args);

/**
@brief Calls extend library
@param context Context
@param apiPtr Pointer to extend API
@param argLen Length of arguments
@param args Arguments
@return Return value from extend API (string). This value must be freed by wkcCustomJSStringFreeExtLibPeer().
*/
WKC_PEER_API char* wkcCustomJSStringCallExtLibPeer(void* context, WKCCustomJSStringAPIPtr stringApiPtr, int argLen, WKCCustomJSAPIArgs *args);

/**
@brief Frees string obtained by wkcCustomJSStringCallExtLibPeer().
@param context Context
@param str String to free
@return None
*/
WKC_PEER_API void wkcCustomJSStringFreeExtLibPeer(void* context, WKCCustomJSStringFreePtr stringFreePtr, char* str);

WKC_END_C_LINKAGE

/*@}*/

#endif // WKC_ENABLE_CUSTOMJS
#endif /* _WKC_CUSTOMJSPEER_H_ */

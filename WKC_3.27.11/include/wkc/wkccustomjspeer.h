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

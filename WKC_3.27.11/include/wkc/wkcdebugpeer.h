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

#ifndef _WKC_DEBUGPEER_H_
#define _WKC_DEBUGPEER_H_

/**
   @file
   @brief Debug related peers.
*/

/* @{ */

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
@brief Set SocketErrorCallback in Network Peer layer
@param "callback" ErrorCallback
*/
WKC_PEER_API void wkcNetSetSocketErrorCallbackPeer(void(*callback)(int));

WKC_PEER_API void wkcLoggingPeer(const char* in_format, va_list in_args);
WKC_PEER_API void wkcSetLoggingCallbackPeer(void(*callback)(const char* in_format, va_list in_args));

WKC_PEER_API void wkcSetVirtualAddressCallbackEnabledPeer(bool enabled);

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _WKC_DEBUGPEER_H_ */

/*
 *  wkcdebugpeer.h
 *
 *  Copyright(c) 2016-2020 ACCESS CO., LTD. All rights reserved.
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

WKC_PEER_API void wkcReportAllocFailurePeer(unsigned int size, unsigned int count, unsigned int metaDataUsageSize);
WKC_PEER_API void wkcSetReportAllocFailureCallbackPeer(void(*callback)(unsigned int, unsigned int, unsigned int));

WKC_PEER_API void wkcLoggingPeer(const char* in_format, va_list in_args);
WKC_PEER_API void wkcSetLoggingCallbackPeer(void(*callback)(const char* in_format, va_list in_args));

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _WKC_DEBUGPEER_H_ */

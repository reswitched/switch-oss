/*
 *  wkcdebugpeer.h
 *
 *  Copyright(c) 2016-2017 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKC_DEBUGPEER_H_
#define _WKC_DEBUGPEER_H_

/**
   @file
   @brief Debug related peers.
*/

/* @{ */

#ifdef __cplusplus
extern "C" {
#endif

/**
@brief Set SocketErrorCallback in Network Peer layer
@param "callback" ErrorCallback
*/
WKC_PEER_API void wkcNetSetSocketErrorCallbackPeer(void(*callback)(int));
/**
@brief Report when an arrow function is used.
*/
WKC_PEER_API void wkcReportArrowFunctionPeer();
WKC_PEER_API void wkcSetReportArrowFunctionCallbackPeer(void(*callback)());
/**
@brief Report when 'this' is used in an arrow function.
*/
WKC_PEER_API void wkcReportArrowFunctionThisPeer();
WKC_PEER_API void wkcSetReportArrowFunctionThisCallbackPeer(void(*callback)());

WKC_PEER_API void wkcReportAllocFailurePeer(unsigned int size, unsigned int count, unsigned int metaDataUsageSize);
WKC_PEER_API void wkcSetReportAllocFailureCallbackPeer(void(*callback)(unsigned int, unsigned int, unsigned int));

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _WKC_DEBUGPEER_H_ */

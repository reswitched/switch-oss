/*
 *  wkcdebugpeer.h
 *
 *  Copyright(c) 2016 ACCESS CO., LTD. All rights reserved.
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

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _WKC_DEBUGPEER_H_ */

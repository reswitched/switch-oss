/*
 *  wkcsocket.h
 *
 *  Copyright(c) 2010-2016 ACCESS CO., LTD. All rights reserved.
 */

#ifndef _WKC_SOCKET_H_
#define _WKC_SOCKET_H_

/**
   @file
   @brief Socket related peers.
*/

/* @{ */

#include <wkc/wkcclib.h>
#include <peer_socket.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WKCSocketStatistics_ {
    int fFd;
    unsigned int fRecvBytes;
    unsigned int fSendBytes;
} WKCSocketStatistics;

/**
@brief Initializes Network Peer layer
@retval "!= false" Succeeded
@retval "== false" Failed
@details
Performs initialization of the network system on a system, or required initialization on the Network Peer layer.
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API bool wkcNetInitializePeer(void);
/**
@brief Finalizes Network Peer layer
@details
Performs finalization of the network system on a system, or required finalization on the Network Peer layer.
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API void wkcNetFinalizePeer(void);
/**
@brief Forcibly terminates Network Peer layer
@details
Performs the forced termination of the Network Peer layer.
@attention
Do not release the memory allocated within this function.
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API void wkcNetForceTerminatePeer(void);

/**
@brief Berkeley Socket socket() function
@param "in_domain" Communication domain, conforms to Berkeley Socket socket()
@param "in_type" Communication method, conforms to Berkeley Socket socket()
@param "in_protocol" Specific protocol to be used, conforms to Berkeley Socket socket()
@retval "Conforms to Berkeley Socket socket()"
@attention
- The behavior must be the same as the Berkeley Socket socket() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int  wkcNetSocketPeer(int in_domain, int in_type, int in_protocol);
/**
@brief Berkeley Socket close() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket close()
@retval "Conforms to Berkeley Socket close()"
@attention
- The behavior must be the same as the Berkeley Socket close() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int  wkcNetClosePeer(int in_sockfd);
/**
@brief Berkeley Socket listen() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket listen()
@param "in_backlog" Maximum length of queue for the pending connections, conforms to Berkeley Socket listen()
@retval "Conforms to Berkeley Socket listen()"
@attention
- The behavior must be the same as the Berkeley Socket listen() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int  wkcNetListenPeer(int in_sockfd, int in_backlog);
/**
@brief Berkeley Socket accept() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket accept()
@param "out_addr" accept destination address, conforms to Berkeley Socket accept()
@param "inout_addrlen" Length of out_addr, conforms to Berkeley Socket accept()
@retval "Conforms to Berkeley Socket accept()"
@attention
- The behavior must be the same as the Berkeley Socket accept() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int  wkcNetAcceptPeer(int in_sockfd, struct sockaddr *out_addr, socklen_t *inout_addrlen);
/**
@brief Berkeley Socket bind() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket bind()
@param "in_addr" bind origin address, conforms to Berkeley Socket bind()
@param "in_addrlen" Length of in_addr, conforms to Berkeley Socket bind()
@retval "Conforms to Berkeley Socket bind()"
@attention
- The behavior must be the same as the Berkeley Socket bind() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int  wkcNetBindPeer(int in_sockfd, const struct sockaddr *in_addr, socklen_t in_addrlen);
/**
@brief Berkeley Socket connect() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket connect()
@param "in_addr" Connect destination address, conforms to Berkeley Socket connect()
@param "in_addrlen" Length of in_addr, conforms to Berkeley Socket connect()
@retval "Conforms to Berkeley Socket connect()"
@attention
- The behavior must be the same as the Berkeley Socket connect() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int  wkcNetConnectPeer(int in_sockfd, const struct sockaddr *in_addr, socklen_t in_addrlen);
/**
@brief Berkeley Socket recvfrom() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket recvfrom()
@param "out_buf" Reception data buffer, conforms to Berkeley Socket recvfrom()
@param "in_len" Length of out_buf, conforms to Berkeley Socket recvfrom()
@param "in_flags" Flag, conforms to Berkeley Socket recvfrom()
@param "out_src_addr" Reception address, conforms to Berkeley Socket recvfrom()
@param "out_addrlen" Length of out_src_addr, conforms to Berkeley Socket recvfrom()
@retval "Conforms to Berkeley Socket recvfrom()"
@attention
- The behavior must be the same as the Berkeley Socket recvfrom() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API ssize_t wkcNetRecvFromPeer(int in_sockfd, void *out_buf, size_t in_len, int in_flags, struct sockaddr *out_src_addr, socklen_t *out_addrlen);
/**
@brief Berkeley Socket recv() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket recvfrom()
@param "out_buf" Reception data buffer, conforms to Berkeley Socket recvfrom()
@param "in_len" Length of out_buf, conforms to Berkeley Socket recvfrom()
@param "in_flags" Flag, conforms to Berkeley Socket recvfrom()
@retval "Conforms to Berkeley Socket recv()"
@attention
- The behavior must be the same as the Berkeley Socket recv() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API ssize_t wkcNetRecvPeer(int in_sockfd, void *out_buf, size_t in_len, int in_flags);
/**
@brief Berkeley Socket sendto() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket send()
@param "in_buf" Transmission data buffer, conforms to Berkeley Socket send()
@param "in_len" Length of in_buf, conforms to Berkeley Socket send()
@param "in_flags" Flag, conforms to Berkeley Socket send()
@param "in_dest_addr" Destination address, conforms to Berkeley Socket sendto()
@param "in_addrlen" Length of in_dest_addr, conforms to Berkeley Socket sendto()
@retval "Conforms to Berkeley Socket sendto()"
@attention
- The behavior must be the same as the Berkeley Socket sendto() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API ssize_t wkcNetSendToPeer(int in_sockfd, const void *in_buf, size_t in_len, int in_flags, const struct sockaddr *in_dest_addr, socklen_t in_addrlen);
/**
@brief Berkeley Socket send() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket send()
@param "in_buf" Transmission data buffer, conforms to Berkeley Socket send()
@param "in_len" Length of in_buf, conforms to Berkeley Socket send()
@param "in_flags" Flag, conforms to Berkeley Socket send()
@retval "Conforms to Berkeley Socket send()"
@attention
- The behavior must be the same as the Berkeley Socket send() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API ssize_t wkcNetSendPeer(int in_sockfd, const void *in_buf, size_t in_len, int in_flags);
/**
@brief Berkeley Socket shutdown() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket shutdown()
@param "in_how" Reason, conforms to Berkeley Socket shutdown()
@retval "Conforms to Berkeley Socket shutdown()"
@attention
- The behavior must be the same as the Berkeley Socket shutdown() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int wkcNetShutdownPeer(int in_sockfd, int in_how);
/**
@brief Berkeley Socket getsockopt() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket setsockopt()
@param "in_level" Protocol layer, conforms to Berkeley Socket setsockopt()
@param "in_optname" Option, conforms to Berkeley Socket setsockopt()
@param "out_optval" Pointer to area that stores result, conforms to Berkeley Socket setsockopt()
@param "out_optlen" Length of out_optval, conforms to Berkeley Socket setsockopt()
@retval "Conforms to Berkeley Socket getsockopt()"
@attention
- The behavior must be the same as the Berkeley Socket getsockopt() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int wkcNetGetSockOptPeer(int in_sockfd, int in_level, int in_optname, void *out_optval, socklen_t *out_optlen);
/**
@brief Berkeley Socket setsockopt() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket setsockopt()
@param "in_level" Protocol layer, conforms to Berkeley Socket setsockopt()
@param "in_optname" Option, conforms to Berkeley Socket setsockopt()
@param "in_optval" Pointer to area that stores option value, conforms to Berkeley Socket setsockopt()
@param "in_optlen" Length of in_optval, conforms to Berkeley Socket setsockopt()
@retval "Conforms to Berkeley Socket setsockopt()"
@attention
- The behavior must be the same as the Berkeley Socket setsockopt() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int wkcNetSetSockOptPeer(int in_sockfd, int in_level, int in_optname, const void *in_optval, socklen_t in_optlen);
/**
@brief Berkeley Socket fcntl() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket ioctl()
@param "in_cmd" Command for socket descriptor, conforms to Berkeley Socket ioctl()
@param "in_val"  Value for command, conforms to Berkeley Socket ioctl()
@retval "Conforms to Berkeley Socket fcntl()"
@attention
It is only necessary to implement either wkcNetFcntlPeer() or wkcNetIoctlSocketPeer().
- The behavior must be the same as the Berkeley Socket fcntl() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int wkcNetFcntlPeer(int in_sockfd, int in_cmd, long in_val, ...);
/**
@brief Berkeley Socket ioctl() function
@param "in_sockfd" Opened socket descriptor, conforms to Berkeley Socket ioctl()
@param "in_cmd" Command for socket descriptor, conforms to Berkeley Socket ioctl()
@param "in_val"  Value for command, conforms to Berkeley Socket ioctl()
@retval "Conforms to Berkeley Socket ioctl()"
@attention
It is only necessary to implement either wkcNetFcntlPeer() or wkcNetIoctlSocketPeer().
- The behavior must be the same as the Berkeley Socket ioctl() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int wkcNetIoctlSocketPeer(int in_sockfd, int in_cmd, long *in_val);
/**
@brief Berkeley Socket poll() function
@param "inout_fds" Group of socket descriptors to monitor, conforms to Berkeley Socket poll()
@param "in_nfds"  Number of socket descriptors to monitor, conforms to Berkeley Socket poll()
@param "in_timeout" Limit of elapsed time until returning, conforms to Berkeley Socket poll()
@retval "Conforms to Berkeley Socket poll()"
@attention
It is only necessary to implement either wkcNetPollPeer() or wkcNetSelectPeer().
- The behavior must be the same as the Berkeley Socket poll() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int wkcNetPollPeer(struct pollfd *inout_fds, nfds_t in_nfds, int in_timeout);
/**
@brief Berkeley Socket select() function
@param "in_nfds" Sum of maximum value of socket descriptor included in 3 groups plus 1, conforms to Berkeley Socket select()
@param "out_readfds" Group of socket descriptors that monitor whether reading is possible, conforms to Berkeley Socket select()
@param "out_writefds" Group of socket descriptors that monitor whether writing is possible, conforms to Berkeley Socket select()
@param "out_exceptfds" Group of socket descriptors to monitor exceptions, conforms to Berkeley Socket select()
@param "in_timeout" Limit of elapsed time until returning, conforms to Berkeley Socket select()
@retval "Conforms to Berkeley Socket select()"
@attention
It is only necessary to implement either wkcNetPollPeer() or wkcNetSelectPeer().
- The behavior must be the same as the Berkeley Socket select() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int wkcNetSelectPeer(int in_nfds, fd_set *out_readfds, fd_set *out_writefds, fd_set *out_exceptfds, struct timeval *in_timeout);
/**
@brief Berkeley Socket getsockname() function
@param "in_sockfd" Socket descriptor, conforms to Berkeley Socket getsockname()
@param "out_addr" Buffer that stores address, conforms to Berkeley Socket getsockname()
@param "out_addrlen" Size of area pointed by in_addr, conforms to Berkeley Socket getsockname()
@retval "Conforms to Berkeley Socket getsockname()"
@attention
- The behavior must be the same as the Berkeley Socket getsockname() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int wkcNetGetSockNamePeer(int in_sockfd, struct sockaddr *out_addr, socklen_t *out_addrlen);
/**
@brief Berkeley Socket gethostbyname() function
@param "in_name" Host name, conforms to Berkeley Socket gethostbyname()
@retval "Conforms to Berkeley Socket gethostbyname()"
@attention
- The behavior must be the same as the Berkeley Socket gethostbyname() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API struct hostent *wkcNetGetHostByNamePeer(const char *in_name);
/**
@brief Berkeley Socket gethostbyaddr() function
@param "in_addr" Pointer to host address, conforms to Berkeley Socket gethostbyaddr()
@param "in_len"  Host address length, conforms to Berkeley Socket gethostbyaddr()
@param "in_type" Host address type, conforms to Berkeley Socket gethostbyaddr()
@retval "Conforms to Berkeley Socket gethostbyaddr()"
@attention
- The behavior must be the same as the Berkeley Socket gethostbyaddr() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API struct hostent *wkcNetGetHostByAddrPeer(const void *in_addr, socklen_t in_len, int in_type);
/**
@brief Berkeley Socket getpeername() function
@param "sockfd" Socket descriptor, conforms to Berkeley Socket getpeername()
@param "addr"  Buffer that stores address, conforms to Berkeley Socket getpeername()
@param "addrlen" Size of area pointed by addr, conforms to Berkeley Socket getpeername()
@retval "Conforms to Berkeley Socket getpeername()"
@attention
- The behavior must be the same as the Berkeley Socket getpeername() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int wkcNetGetPeerNamePeer(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
/**
@brief Berkeley Socket getaddrinfo() function
@param "node" Node name, conforms to Berkeley Socket getaddrinfo()
@param "service" Service name, conforms to Berkeley Socket getaddrinfo() 
@param "hints" Hint of socket type, conforms to Berkeley Socket getaddrinfo()
@param "res" List of link that includes response information related to nodes, conforms to Berkeley Socket getaddrinfo()
@retval "Conforms to Berkeley Socket getaddrinfo()"
@attention
- The behavior must be the same as the Berkeley Socket getaddrinfo() function.
- Errors that occurred in the function must be able to be handled by Berkeley Socket and set for wkcNetSetLastErrorPeer() so that they can be obtained by wkcNetGetLastErrorPeer().
@remarks
Implementing this function is necessary on each system.
*/
WKC_PEER_API int wkcNetGetAddrInfoPeer(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
/**
@brief Berkeley Socket freeaddrinfo() function
@param "res" List of link obtained by wkcNetGetAddrInfoPeer() to release, conforms to Berkeley Socket wkcNetGetAddrInfoPeer()
@attention
- The behavior must be the same as the Berkeley Socket freeaddrinfo() function.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void wkcNetFreeAddrInfoPeer(struct addrinfo *res);
/**
@brief Berkeley Socket inet_addr() function
@param cp Internet host address
@retval Address info in network byte order
@attention
- The behavior must be the same as the Berkeley Socket inet_addr() function.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API wkc_addr_t wkcNetInetAddrPeer(const char *cp);
/**
@brief Prefetch given hostname
@param in_hostname hostname
@param in_hostnamelen length of hostname
*/
WKC_PEER_API void wkcNetPrefetchDNSPeer(const char* in_hostname, unsigned int in_hostnamelen);

/**
@brief Set callback for DNS Prefetch
@param in_requestprefetchproc callback
@param in_resolverlocker resolver locker
*/
WKC_PEER_API void wkcNetSetPrefetchDNSCallbackPeer(void(*in_requestprefetchproc)(const char*), void* in_resolverlocker);

/**
@brief Cache resolved DNS entry
@param in_hostname hostname
@param in_ipaddr resolved IP address
*/
WKC_PEER_API void wkcNetCachePrefetchedDNSEntryPeer(const char* in_hostname, const unsigned char* in_ipaddr);

/**
@brief Get the number of sockets
@retval Number of sockets
*/
WKC_PEER_API int wkcNetGetNumberOfSocketsPeer(void);

/**
@brief Get the statistics of sockets
@param in_numberOfArray Number of array
@param out_statistics Statistics of sockets
@retval Number of statistics
*/
WKC_PEER_API int wkcNetGetSocketStatisticsPeer(int in_numberOfArray, WKCSocketStatistics* out_statistics);

/**
@brief Gets errors that occurred in Network Peer layer
@details
Berkeley Sockets get errors using the errno variable, however, Network Peer gets errors using this function.
@retval Error that occurred in Network Peer layer
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API int  wkcNetGetLastErrorPeer(void);
/**
@brief Saves errors that occurred in Network Peer layer
@param "in_err" Error that occurred in Network Peer layer
@details
Berkeley Sockets store errors in the errno variable, however, Network Peer stores errors within this function.
@remarks
The existing implementation can be used for this function without change.
*/
WKC_PEER_API void wkcNetSetLastErrorPeer(int in_err);

/**
@brief Set IsNetworkAvailable callback in Network Peer Layer
@param in_callback callback
*/
WKC_PEER_API void wkcNetSetIsNetworkAvailableCallbackPeer(bool(*in_callback)(void));

#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _WKC_PEER_NET_H_ */

/*
 * "Default" SSLSocket methods, used by sockets that do neither SSL nor socks.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "cert.h"
#include "ssl.h"
#include "sslimpl.h"

#if defined(WIN32)
#define MAP_ERROR(from, to) \
    if (err == from) {      \
        PORT_SetError(to);  \
    }
#define DEFINE_ERROR PRErrorCode err = PR_GetError();
#else
#define MAP_ERROR(from, to)
#define DEFINE_ERROR
#endif

int
ssl_DefConnect(sslSocket *ss, const PRNetAddr *sa)
{
    PRFileDesc *lower = ss->fd->lower;
    int rv;

    rv = lower->methods->connect(lower, sa, ss->cTimeout);
    return rv;
}

int
ssl_DefBind(sslSocket *ss, const PRNetAddr *addr)
{
    PRFileDesc *lower = ss->fd->lower;
    int rv;

    rv = lower->methods->bind(lower, addr);
    return rv;
}

int
ssl_DefListen(sslSocket *ss, int backlog)
{
    PRFileDesc *lower = ss->fd->lower;
    int rv;

    rv = lower->methods->listen(lower, backlog);
    return rv;
}

int
ssl_DefShutdown(sslSocket *ss, int how)
{
    PRFileDesc *lower = ss->fd->lower;
    int rv;

    rv = lower->methods->shutdown(lower, how);
    return rv;
}

#if defined(NN_NINTENDO_SDK) && defined(NN_ENABLE_SSL_PRIVATE)
static int
IsFromPeer(sslSocket *ss, PRNetAddr* pRemoteAddr)
{
    if (pRemoteAddr->raw.family == AF_INET)
    {
        return (pRemoteAddr->inet.ip   == ss->peerAddr.inet.ip) && 
               (pRemoteAddr->inet.port == ss->peerAddr.inet.port);
    }
#ifdef NN_ENABLE_IPV6
    else if (pRemoteAddr->raw.family == PR_AF_INET6)
    {
        return (memcmp(&pRemoteAddr->ipv6.ip, &ss->peerAddr.ipv6.ip, sizeof(ss->peerAddr.ipv6.ip)) == 0) && 
               (pRemoteAddr->ipv6.port == ss->peerAddr.ipv6.port);
    }
#endif // NN_ENABLE_IPV6

    return 0;
}
#endif // NN_NINTENDO_SDK && NN_ENABLE_SSL_PRIVATE

int
ssl_DefRecv(sslSocket *ss, unsigned char *buf, int len, int flags)
{
    PRFileDesc *lower = ss->fd->lower;
    int rv;

    PORT_Assert(buf && len > 0);

#if defined(NN_NINTENDO_SDK) && defined(NN_ENABLE_SSL_PRIVATE)
    if(IS_DTLS(ss))
    {
        PRNetAddr remoteAddr;
        memset(&remoteAddr, 0, sizeof(remoteAddr));
        remoteAddr.raw.family = ss->peerAddr.raw.family;
        rv = lower->methods->recvfrom(lower, (void *)buf, len, flags, &remoteAddr, ss->rTimeout);

        if( rv > 0 && !IsFromPeer(ss, &remoteAddr) )
        {
            printf("\nWARNING: Got packet from unexpected peer!\n\n");
            PORT_SetError(PR_WOULD_BLOCK_ERROR);
            return SECFailure;
        }
    }
    else
    {
#endif // NN_NINTENDO_SDK && NN_ENABLE_SSL_PRIVATE
    rv = lower->methods->recv(lower, (void *)buf, len, flags, ss->rTimeout);
#if defined(NN_NINTENDO_SDK) && defined(NN_ENABLE_SSL_PRIVATE)
    }
#endif // NN_NINTENDO_SDK && NN_ENABLE_SSL_PRIVATE

    if (rv < 0) {
        DEFINE_ERROR
        MAP_ERROR(PR_SOCKET_SHUTDOWN_ERROR, PR_CONNECT_RESET_ERROR)
    } else if (rv > len) {
        PORT_Assert(rv <= len);
        PORT_SetError(PR_BUFFER_OVERFLOW_ERROR);
        rv = SECFailure;
    }
    return rv;
}

/* Default (unencrypted) send.
 * For blocking sockets, always returns len or SECFailure, no short writes.
 * For non-blocking sockets:
 *   Returns positive count if any data was written, else returns SECFailure.
 *   Short writes may occur.
 */
int
ssl_DefSend(sslSocket *ss, const unsigned char *buf, int len, int flags)
{
    PRFileDesc *lower = ss->fd->lower;
    int sent = 0;

#if NSS_DISABLE_NAGLE_DELAYS
    /* Although this is overkill, we disable Nagle delays completely for
    ** SSL sockets.
    */
    if (ss->opt.useSecurity && !ss->delayDisabled) {
        ssl_EnableNagleDelay(ss, PR_FALSE); /* ignore error */
        ss->delayDisabled = 1;
    }
#endif
    do {
        int rv = -1;
#if defined(NN_NINTENDO_SDK) && defined(NN_ENABLE_SSL_PRIVATE)
        if(IS_DTLS(ss))
        {
            rv = lower->methods->sendto(lower, (const void *)(buf + sent),
                                      len - sent, flags, &ss->peerAddr, ss->wTimeout);
        }
        else
        {
#endif // NN_NINTENDO_SDK && NN_ENABLE_SSL_PRIVATE
        rv = lower->methods->send(lower, (const void *)(buf + sent),
                                      len - sent, flags, ss->wTimeout);
#if defined(NN_NINTENDO_SDK) && defined(NN_ENABLE_SSL_PRIVATE)
        }
#endif // NN_NINTENDO_SDK && NN_ENABLE_SSL_PRIVATE
        if (rv < 0) {
            PRErrorCode err = PR_GetError();
            if (err == PR_WOULD_BLOCK_ERROR) {
                ss->lastWriteBlocked = 1;
                return sent ? sent : SECFailure;
            }
            ss->lastWriteBlocked = 0;
            MAP_ERROR(PR_CONNECT_ABORTED_ERROR, PR_CONNECT_RESET_ERROR)
            /* Loser */
            return rv;
        }
        sent += rv;

        if (IS_DTLS(ss) && (len > sent)) {
            /* We got a partial write so just return it */
            return sent;
        }
    } while (len > sent);
    ss->lastWriteBlocked = 0;
    return sent;
}

int
ssl_DefRead(sslSocket *ss, unsigned char *buf, int len)
{
    PRFileDesc *lower = ss->fd->lower;
    int rv;

    rv = lower->methods->read(lower, (void *)buf, len);
    if (rv < 0) {
        DEFINE_ERROR
        MAP_ERROR(PR_SOCKET_SHUTDOWN_ERROR, PR_CONNECT_RESET_ERROR)
    }
    return rv;
}

int
ssl_DefWrite(sslSocket *ss, const unsigned char *buf, int len)
{
    PRFileDesc *lower = ss->fd->lower;
    int sent = 0;

    do {
        int rv = lower->methods->write(lower, (const void *)(buf + sent),
                                       len - sent);
        if (rv < 0) {
            PRErrorCode err = PR_GetError();
            if (err == PR_WOULD_BLOCK_ERROR) {
                ss->lastWriteBlocked = 1;
                return sent ? sent : SECFailure;
            }
            ss->lastWriteBlocked = 0;
            MAP_ERROR(PR_CONNECT_ABORTED_ERROR, PR_CONNECT_RESET_ERROR)
            /* Loser */
            return rv;
        }
        sent += rv;
    } while (len > sent);
    ss->lastWriteBlocked = 0;
    return sent;
}

int
ssl_DefGetpeername(sslSocket *ss, PRNetAddr *name)
{
#if defined(NN_NINTENDO_SDK) && defined(NN_ENABLE_SSL_PRIVATE)
    if(IS_DTLS(ss))
    {
        memcpy(name, &ss->peerAddr, sizeof(ss->peerAddr));
        return 0;
    }
#endif // NN_NINTENDO_SDK && NN_ENABLE_SSL_PRIVATE
    PRFileDesc *lower = ss->fd->lower;
    int rv;

    rv = lower->methods->getpeername(lower, name);
    return rv;
}

int
ssl_DefGetsockname(sslSocket *ss, PRNetAddr *name)
{
    PRFileDesc *lower = ss->fd->lower;
    int rv;

    rv = lower->methods->getsockname(lower, name);
    return rv;
}

int
ssl_DefClose(sslSocket *ss)
{
    PRFileDesc *fd;
    PRFileDesc *popped;
    int rv;

    fd = ss->fd;

    /* First, remove the SSL layer PRFileDesc from the socket's stack,
    ** then invoke the SSL layer's PRFileDesc destructor.
    ** This must happen before the next layer down is closed.
    */
    PORT_Assert(fd->higher == NULL);
    if (fd->higher) {
        PORT_SetError(PR_BAD_DESCRIPTOR_ERROR);
        return SECFailure;
    }
    ss->fd = NULL;

    /* PR_PopIOLayer will swap the contents of the top two PRFileDescs on
    ** the stack, and then remove the second one.  This way, the address
    ** of the PRFileDesc on the top of the stack doesn't change.
    */
    popped = PR_PopIOLayer(fd, PR_TOP_IO_LAYER);
    popped->dtor(popped);

    /* fd is now the PRFileDesc for the next layer down.
    ** Now close the underlying socket.
    */
    rv = fd->methods->close(fd);

    ssl_FreeSocket(ss);

    SSL_TRC(5, ("%d: SSL[%d]: closing, rv=%d errno=%d",
                SSL_GETPID(), fd, rv, PORT_GetError()));
    return rv;
}

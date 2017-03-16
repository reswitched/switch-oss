/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 * Copyright (c) 2012, 2015 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SocketStreamHandle_h
#define SocketStreamHandle_h

#include "SocketStreamHandleBase.h"
#include "Timer.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

    class AuthenticationChallenge;
    class Credential;
    class NetworkingContext;
    class ProtectionSpace;
    class SocketStreamHandleClient;

    class SocketStreamHandle : public RefCounted<SocketStreamHandle>, public SocketStreamHandleBase {
    public:
        static PassRefPtr<SocketStreamHandle> create(const URL& url, SocketStreamHandleClient* client, NetworkingContext& networkingContext) { return adoptRef(new SocketStreamHandle(url, client, networkingContext)); }

        virtual ~SocketStreamHandle();

        int  OpenSocketCallback(void* addr);

        bool isClosingSocketStreamChannel();

        const URL& url() const { return m_url; }
        enum {
            None,
            Initialized,
            Connected,
            CanReceive
        };
        int socketState() const { return m_socketState; }

        bool isConstructed() const { return m_constructed; }

    protected:
        virtual int platformSend(const char* data, int length);
        virtual void platformClose();

    private:
        SocketStreamHandle(const URL&, SocketStreamHandleClient*, NetworkingContext&);

        bool getStoredCONNECTProxyCredentials(const ProtectionSpace&, String& login, String& password);

        // No authentication for streams per se, but proxy may ask for credentials.
        void didReceiveAuthenticationChallenge(const AuthenticationChallenge&);
        void receivedCredential(const AuthenticationChallenge&, const Credential&);
        void receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&);
        void receivedCancellation(const AuthenticationChallenge&);

    private:
        void construct();
        //void finalizeSocket();
        void progressTimerFired();

    private:
        bool m_constructed;

        int m_socketState;

        int m_socket;

        Timer m_progressTimer;
        void nextProgress(bool refresh);
        float m_interval;
        unsigned int m_lastUpdate;

        bool m_isProxy;
        const static int m_recvDataLen = 32768;
        void* m_recvData;

        void* m_handle;      // CURL handle
        void* m_multiHandle; // CURLM handle

        Ref<NetworkingContext> m_networkingContext;
    };

}  // namespace WebCore

#endif  // SocketStreamHandle_h

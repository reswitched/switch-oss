/*
 * Copyright (C) 2009 Brent Fulgham.  All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
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

#include "config.h"
#include "SocketStreamHandle.h"

#if USE(CURL)

#include "Logging.h"
#include "NotImplemented.h"
#include "SocketStreamHandleClient.h"
#include "URL.h"
#include <wtf/MainThread.h>
#include <wtf/text/CString.h>

namespace WebCore {

SocketStreamHandle::SocketStreamHandle(const URL& url, SocketStreamHandleClient* client)
    : SocketStreamHandleBase(url, client)
{
    LOG(Network, "SocketStreamHandle %p new client %p", this, m_client);
    ASSERT(isMainThread());
    startThread();
}

SocketStreamHandle::~SocketStreamHandle()
{
    LOG(Network, "SocketStreamHandle %p delete", this);
    ASSERT(!m_workerThread);
}

int SocketStreamHandle::platformSend(const char* data, int length)
{
    LOG(Network, "SocketStreamHandle %p platformSend", this);

    ASSERT(isMainThread());

    startThread();

    auto copy = createCopy(data, length);

    std::lock_guard<std::mutex> lock(m_mutexSend);
    m_sendData.append(SocketData { WTF::move(copy), length });

    return length;
}

void SocketStreamHandle::platformClose()
{
    LOG(Network, "SocketStreamHandle %p platformClose", this);

    ASSERT(isMainThread());

    stopThread();

    if (m_client)
        m_client->didCloseSocketStream(this);
}

bool SocketStreamHandle::readData(CURL* curlHandle)
{
    ASSERT(!isMainThread());

    const int bufferSize = 1024;
    std::unique_ptr<char[]> data(new char[bufferSize]);
    size_t bytesRead = 0;

    CURLcode ret = curl_easy_recv(curlHandle, data.get(), bufferSize, &bytesRead);

    if (ret == CURLE_OK && bytesRead >= 0) {
        m_mutexReceive.lock();
        m_receiveData.append(SocketData { WTF::move(data), static_cast<int>(bytesRead) });
        m_mutexReceive.unlock();

        ref();

        callOnMainThread([this] {
            didReceiveData();
            deref();
        });

        return true;
    }

    if (ret == CURLE_AGAIN)
        return true;

    return false;
}

bool SocketStreamHandle::sendData(CURL* curlHandle)
{
    ASSERT(!isMainThread());

    while (true) {

        m_mutexSend.lock();
        if (!m_sendData.size()) {
            m_mutexSend.unlock();
            break;
        }
        auto sendData = m_sendData.takeFirst();
        m_mutexSend.unlock();

        int totalBytesSent = 0;
        while (totalBytesSent < sendData.size) {
            size_t bytesSent = 0;
            CURLcode ret = curl_easy_send(curlHandle, sendData.data.get() + totalBytesSent, sendData.size - totalBytesSent, &bytesSent);
            if (ret == CURLE_OK)
                totalBytesSent += bytesSent;
            else
                break;
        }

        // Insert remaining data into send queue.

        if (totalBytesSent < sendData.size) {
            const int restLength = sendData.size - totalBytesSent;
            auto copy = createCopy(sendData.data.get() + totalBytesSent, restLength);

            std::lock_guard<std::mutex> lock(m_mutexSend);
            m_sendData.prepend(SocketData { WTF::move(copy), restLength });

            return false;
        }
    }

    return true;
}

bool SocketStreamHandle::waitForAvailableData(CURL* curlHandle, std::chrono::milliseconds selectTimeout)
{
    ASSERT(!isMainThread());

    std::chrono::microseconds usec = std::chrono::duration_cast<std::chrono::microseconds>(selectTimeout);

    struct timeval timeout;
    if (usec <= std::chrono::microseconds(0)) {
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
    } else {
        timeout.tv_sec = usec.count() / 1000000;
        timeout.tv_usec = usec.count() % 1000000;
    }

    long socket;
    if (curl_easy_getinfo(curlHandle, CURLINFO_LASTSOCKET, &socket) != CURLE_OK)
        return false;

    fd_set fdread;
    FD_ZERO(&fdread);
    FD_SET(socket, &fdread);
    int rc = ::select(0, &fdread, nullptr, nullptr, &timeout);
    return rc == 1;
}

void SocketStreamHandle::startThread()
{
    ASSERT(isMainThread());

    if (m_workerThread)
        return;

    ref(); // stopThread() will call deref().

    m_workerThread = createThread("WebSocket thread", [this] {

        ASSERT(!isMainThread());

        CURL* curlHandle = curl_easy_init();

        if (!curlHandle)
            return;

        curl_easy_setopt(curlHandle, CURLOPT_URL, m_url.host().utf8().data());
        curl_easy_setopt(curlHandle, CURLOPT_PORT, m_url.port());
        curl_easy_setopt(curlHandle, CURLOPT_CONNECT_ONLY);

        // Connect to host
        if (curl_easy_perform(curlHandle) != CURLE_OK)
            return;

        ref();

        callOnMainThread([this] {
            // Check reference count to fix a crash.
            // When the call is invoked on the main thread after all other references are released, the SocketStreamClient
            // is already deleted. Accessing the SocketStreamClient in didOpenSocket() will then cause a crash.
            if (refCount() > 1)
                didOpenSocket();
            deref();
        });

        while (!m_stopThread) {
            // Send queued data
            sendData(curlHandle);

            // Wait until socket has available data
            if (waitForAvailableData(curlHandle, std::chrono::milliseconds(20)))
                readData(curlHandle);
        }

        curl_easy_cleanup(curlHandle);
    });
}

void SocketStreamHandle::stopThread()
{
    ASSERT(isMainThread());

    if (!m_workerThread)
        return;

    m_stopThread = true;
    waitForThreadCompletion(m_workerThread);
    m_workerThread = 0;
    deref();
}

void SocketStreamHandle::didReceiveData()
{
    ASSERT(isMainThread());

    m_mutexReceive.lock();

    auto receiveData = WTF::move(m_receiveData);

    m_mutexReceive.unlock();

    for (auto& socketData : receiveData) {
        if (socketData.size > 0) {
            if (m_client && state() == Open)
                m_client->didReceiveSocketStreamData(this, socketData.data.get(), socketData.size);
        } else
            platformClose();
    }
}

void SocketStreamHandle::didOpenSocket()
{
    ASSERT(isMainThread());

    m_state = Open;

    if (m_client)
        m_client->didOpenSocketStream(this);
}

std::unique_ptr<char[]> SocketStreamHandle::createCopy(const char* data, int length)
{
    std::unique_ptr<char[]> copy(new char[length]);
    memcpy(copy.get(), data, length);

    return WTF::move(copy);
}

void SocketStreamHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge&)
{
    notImplemented();
}

void SocketStreamHandle::receivedCredential(const AuthenticationChallenge&, const Credential&)
{
    notImplemented();
}

void SocketStreamHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&)
{
    notImplemented();
}

void SocketStreamHandle::receivedCancellation(const AuthenticationChallenge&)
{
    notImplemented();
}

void SocketStreamHandle::receivedRequestToPerformDefaultHandling(const AuthenticationChallenge&)
{
    notImplemented();
}

void SocketStreamHandle::receivedChallengeRejection(const AuthenticationChallenge&)
{
    notImplemented();
}

} // namespace WebCore

#endif

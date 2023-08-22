/*
 * Copyright (C) 2011, 2012 Google Inc.  All rights reserved.
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

#if ENABLE(WEB_SOCKETS)

#include "WorkerThreadableWebSocketChannel.h"

#include "Blob.h"
#include "Document.h"
#include "ScriptExecutionContext.h"
#include "ThreadableWebSocketChannelClientWrapper.h"
#include "WebSocketChannel.h"
#include "WebSocketChannelClient.h"
#include "WorkerGlobalScope.h"
#include "WorkerLoaderProxy.h"
#include "WorkerRunLoop.h"
#include "WorkerThread.h"
#include <runtime/ArrayBuffer.h>
#include <wtf/MainThread.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

WorkerThreadableWebSocketChannel::WorkerThreadableWebSocketChannel(WorkerGlobalScope* context, WebSocketChannelClient* client, const String& taskMode)
    : m_workerGlobalScope(context)
    , m_workerClientWrapper(ThreadableWebSocketChannelClientWrapper::create(context, client))
    , m_bridge(Bridge::create(m_workerClientWrapper, m_workerGlobalScope, taskMode))
{
    m_bridge->initialize();
}

WorkerThreadableWebSocketChannel::~WorkerThreadableWebSocketChannel()
{
    if (m_bridge)
        m_bridge->disconnect();
}

void WorkerThreadableWebSocketChannel::connect(const URL& url, const String& protocol)
{
    if (m_bridge)
        m_bridge->connect(url, protocol);
}

String WorkerThreadableWebSocketChannel::subprotocol()
{
    ASSERT(m_workerClientWrapper);
    return m_workerClientWrapper->subprotocol();
}

String WorkerThreadableWebSocketChannel::extensions()
{
    ASSERT(m_workerClientWrapper);
    return m_workerClientWrapper->extensions();
}

ThreadableWebSocketChannel::SendResult WorkerThreadableWebSocketChannel::send(const String& message)
{
    if (!m_bridge)
        return ThreadableWebSocketChannel::SendFail;
    return m_bridge->send(message);
}

ThreadableWebSocketChannel::SendResult WorkerThreadableWebSocketChannel::send(const ArrayBuffer& binaryData, unsigned byteOffset, unsigned byteLength)
{
    if (!m_bridge)
        return ThreadableWebSocketChannel::SendFail;
    return m_bridge->send(binaryData, byteOffset, byteLength);
}

ThreadableWebSocketChannel::SendResult WorkerThreadableWebSocketChannel::send(Blob& binaryData)
{
    if (!m_bridge)
        return ThreadableWebSocketChannel::SendFail;
    return m_bridge->send(binaryData);
}

unsigned long WorkerThreadableWebSocketChannel::bufferedAmount() const
{
    if (!m_bridge)
        return 0;
    return m_bridge->bufferedAmount();
}

void WorkerThreadableWebSocketChannel::close(int code, const String& reason)
{
    if (m_bridge)
        m_bridge->close(code, reason);
}

void WorkerThreadableWebSocketChannel::fail(const String& reason)
{
    if (m_bridge)
        m_bridge->fail(reason);
}

void WorkerThreadableWebSocketChannel::disconnect()
{
    m_bridge->disconnect();
    m_bridge = nullptr;
}

void WorkerThreadableWebSocketChannel::suspend()
{
    m_workerClientWrapper->suspend();
    if (m_bridge)
        m_bridge->suspend();
}

void WorkerThreadableWebSocketChannel::resume()
{
    m_workerClientWrapper->resume();
    if (m_bridge)
        m_bridge->resume();
}

WorkerThreadableWebSocketChannel::Peer::Peer(PassRefPtr<ThreadableWebSocketChannelClientWrapper> clientWrapper, WorkerLoaderProxy& loaderProxy, ScriptExecutionContext* context, const String& taskMode)
    : m_workerClientWrapper(clientWrapper)
    , m_loaderProxy(loaderProxy)
    , m_mainWebSocketChannel(WebSocketChannel::create(downcast<Document>(context), this))
    , m_taskMode(taskMode)
{
    ASSERT(isMainThread());
}

WorkerThreadableWebSocketChannel::Peer::~Peer()
{
    ASSERT(isMainThread());
    if (m_mainWebSocketChannel)
        m_mainWebSocketChannel->disconnect();
}

void WorkerThreadableWebSocketChannel::Peer::connect(const URL& url, const String& protocol)
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel)
        return;
    m_mainWebSocketChannel->connect(url, protocol);
}

void WorkerThreadableWebSocketChannel::Peer::send(const String& message)
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel || !m_workerClientWrapper)
        return;

    ThreadableWebSocketChannel::SendResult sendRequestResult = m_mainWebSocketChannel->send(message);
    RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper = m_workerClientWrapper;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, sendRequestResult] (ScriptExecutionContext&) {
        workerClientWrapper->setSendRequestResult(sendRequestResult);
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, sendRequestResult] (ScriptExecutionContext&) {
        workerClientWrapper->setSendRequestResult(sendRequestResult);
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

void WorkerThreadableWebSocketChannel::Peer::send(const ArrayBuffer& binaryData)
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel || !m_workerClientWrapper)
        return;

    ThreadableWebSocketChannel::SendResult sendRequestResult = m_mainWebSocketChannel->send(binaryData, 0, binaryData.byteLength());
    RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper = m_workerClientWrapper;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, sendRequestResult] (ScriptExecutionContext&) {
        workerClientWrapper->setSendRequestResult(sendRequestResult);
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, sendRequestResult](ScriptExecutionContext&) {
        workerClientWrapper->setSendRequestResult(sendRequestResult);
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

void WorkerThreadableWebSocketChannel::Peer::send(Blob& binaryData)
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel || !m_workerClientWrapper)
        return;

    ThreadableWebSocketChannel::SendResult sendRequestResult = m_mainWebSocketChannel->send(binaryData);
    RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper = m_workerClientWrapper;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, sendRequestResult] (ScriptExecutionContext&) {
        workerClientWrapper->setSendRequestResult(sendRequestResult);
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, sendRequestResult](ScriptExecutionContext&) {
        workerClientWrapper->setSendRequestResult(sendRequestResult);
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

void WorkerThreadableWebSocketChannel::Peer::bufferedAmount()
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel || !m_workerClientWrapper)
        return;

    RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper = m_workerClientWrapper;
    unsigned long bufferedAmount = m_mainWebSocketChannel->bufferedAmount();
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, bufferedAmount] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->setBufferedAmount(bufferedAmount);
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, bufferedAmount](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->setBufferedAmount(bufferedAmount);
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

void WorkerThreadableWebSocketChannel::Peer::close(int code, const String& reason)
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel)
        return;
    m_mainWebSocketChannel->close(code, reason);
}

void WorkerThreadableWebSocketChannel::Peer::fail(const String& reason)
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel)
        return;
    m_mainWebSocketChannel->fail(reason);
}

void WorkerThreadableWebSocketChannel::Peer::disconnect()
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel)
        return;
    m_mainWebSocketChannel->disconnect();
    m_mainWebSocketChannel = nullptr;
}

void WorkerThreadableWebSocketChannel::Peer::suspend()
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel)
        return;
    m_mainWebSocketChannel->suspend();
}

void WorkerThreadableWebSocketChannel::Peer::resume()
{
    ASSERT(isMainThread());
    if (!m_mainWebSocketChannel)
        return;
    m_mainWebSocketChannel->resume();
}

void WorkerThreadableWebSocketChannel::Peer::didConnect()
{
    ASSERT(isMainThread());

    RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper = m_workerClientWrapper;
    StringCapture capturedSubprotocol(m_mainWebSocketChannel->subprotocol());
    StringCapture capturedExtensions(m_mainWebSocketChannel->extensions());
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, capturedSubprotocol, capturedExtensions] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->setSubprotocol(capturedSubprotocol.string());
        workerClientWrapper->setExtensions(capturedExtensions.string());
        workerClientWrapper->didConnect();
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, capturedSubprotocol, capturedExtensions](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->setSubprotocol(capturedSubprotocol.string());
        workerClientWrapper->setExtensions(capturedExtensions.string());
        workerClientWrapper->didConnect();
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

void WorkerThreadableWebSocketChannel::Peer::didReceiveMessage(const String& message)
{
    ASSERT(isMainThread());

    RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper = m_workerClientWrapper;
    StringCapture capturedMessage(message);
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, capturedMessage] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didReceiveMessage(capturedMessage.string());
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, capturedMessage](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didReceiveMessage(capturedMessage.string());
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

void WorkerThreadableWebSocketChannel::Peer::didReceiveBinaryData(Vector<char>&& binaryData)
{
    ASSERT(isMainThread());

    RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper = m_workerClientWrapper;
    Vector<char>* capturedData = new Vector<char>(WTF::move(binaryData));
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, capturedData] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didReceiveBinaryData(WTF::move(*capturedData));
        delete capturedData;
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, capturedData](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didReceiveBinaryData(WTF::move(*capturedData));
        delete capturedData;
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

void WorkerThreadableWebSocketChannel::Peer::didUpdateBufferedAmount(unsigned long bufferedAmount)
{
    ASSERT(isMainThread());

    RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper = m_workerClientWrapper;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, bufferedAmount] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didUpdateBufferedAmount(bufferedAmount);
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, bufferedAmount](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didUpdateBufferedAmount(bufferedAmount);
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

void WorkerThreadableWebSocketChannel::Peer::didStartClosingHandshake()
{
    ASSERT(isMainThread());

    RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper = m_workerClientWrapper;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didStartClosingHandshake();
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didStartClosingHandshake();
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

void WorkerThreadableWebSocketChannel::Peer::didClose(unsigned long unhandledBufferedAmount, ClosingHandshakeCompletionStatus closingHandshakeCompletion, unsigned short code, const String& reason)
{
    ASSERT(isMainThread());
    m_mainWebSocketChannel = nullptr;

    RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper = m_workerClientWrapper;
    StringCapture capturedReason(reason);
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(
        [workerClientWrapper, unhandledBufferedAmount, closingHandshakeCompletion, code, capturedReason] (ScriptExecutionContext& context) {
            ASSERT_UNUSED(context, context.isWorkerGlobalScope());
            workerClientWrapper->didClose(unhandledBufferedAmount, closingHandshakeCompletion, code, capturedReason.string());
        }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(),
        [workerClientWrapper, unhandledBufferedAmount, closingHandshakeCompletion, code, capturedReason](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didClose(unhandledBufferedAmount, closingHandshakeCompletion, code, capturedReason.string());
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

void WorkerThreadableWebSocketChannel::Peer::didReceiveMessageError()
{
    ASSERT(isMainThread());

    RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper = m_workerClientWrapper;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(Z[workerClientWrapper] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didReceiveMessageError();
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didReceiveMessageError();
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

WorkerThreadableWebSocketChannel::Bridge::Bridge(PassRefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper, PassRefPtr<WorkerGlobalScope> workerGlobalScope, const String& taskMode)
    : m_workerClientWrapper(workerClientWrapper)
    , m_workerGlobalScope(workerGlobalScope)
    , m_loaderProxy(m_workerGlobalScope->thread().workerLoaderProxy())
    , m_taskMode(taskMode)
    , m_peer(nullptr)
{
    ASSERT(m_workerClientWrapper.get());
}

WorkerThreadableWebSocketChannel::Bridge::~Bridge()
{
    disconnect();
}

void WorkerThreadableWebSocketChannel::Bridge::mainThreadInitialize(ScriptExecutionContext& context, WorkerLoaderProxy* loaderProxy, PassRefPtr<ThreadableWebSocketChannelClientWrapper> prpClientWrapper, const String& taskMode)
{
    ASSERT(isMainThread());
    ASSERT_UNUSED(context, context.isDocument());

    RefPtr<ThreadableWebSocketChannelClientWrapper> clientWrapper = prpClientWrapper;

    Peer* peerPtr = Peer::create(clientWrapper, *loaderProxy, &context, taskMode);
#if !PLATFORM(WKC)
    bool sent = loaderProxy->postTaskForModeToWorkerGlobalScope({
        ScriptExecutionContext::Task::CleanupTask,
        [clientWrapper, loaderProxy, peerPtr] (ScriptExecutionContext& context) {
            ASSERT_UNUSED(context, context.isWorkerGlobalScope());
            if (clientWrapper->failedWebSocketChannelCreation()) {
                // If Bridge::initialize() quitted earlier, we need to kick mainThreadDestroy() to delete the peer.
                loaderProxy->postTaskToLoader([peerPtr] (ScriptExecutionContext& context) {
                    ASSERT(isMainThread());
                    ASSERT_UNUSED(context, context.isDocument());
                    delete peerPtr;
                });
            } else
                clientWrapper->didCreateWebSocketChannel(peerPtr);
        }
    }, taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p1(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [clientWrapper, loaderProxy, peerPtr] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        if (clientWrapper->failedWebSocketChannelCreation()) {
            // If Bridge::initialize() quitted earlier, we need to kick mainThreadDestroy() to delete the peer.
            std::function<void(ScriptExecutionContext&)> p2(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [peerPtr](ScriptExecutionContext& context) {
                ASSERT(isMainThread());
                ASSERT_UNUSED(context, context.isDocument());
                delete peerPtr;
            });
            loaderProxy->postTaskToLoader(p2);
        } else
            clientWrapper->didCreateWebSocketChannel(peerPtr);
    });
    bool sent = loaderProxy->postTaskForModeToWorkerGlobalScope(ScriptExecutionContext::Task(ScriptExecutionContext::Task::CleanupTask, p1), taskMode);
#endif

    if (!sent) {
        clientWrapper->clearPeer();
        delete peerPtr;
    }
}

void WorkerThreadableWebSocketChannel::Bridge::initialize()
{
    ASSERT(!m_peer);
    setMethodNotCompleted();
    Ref<Bridge> protect(*this);

    WorkerLoaderProxy* loaderProxy = &m_loaderProxy;
    RefPtr<ThreadableWebSocketChannelClientWrapper> workerClientWrapper = m_workerClientWrapper;
    StringCapture capturedTaskMode(m_taskMode);
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([loaderProxy, workerClientWrapper, capturedTaskMode] (ScriptExecutionContext& context) {
        mainThreadInitialize(context, loaderProxy, workerClientWrapper, capturedTaskMode.string());
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [loaderProxy, workerClientWrapper, capturedTaskMode](ScriptExecutionContext& context) {
        mainThreadInitialize(context, loaderProxy, workerClientWrapper, capturedTaskMode.string());
    });
    m_loaderProxy.postTaskToLoader(p);
#endif
    waitForMethodCompletion();

    // m_peer may be null when the nested runloop exited before a peer has created.
    m_peer = m_workerClientWrapper->peer();
    if (!m_peer)
        m_workerClientWrapper->setFailedWebSocketChannelCreation();
}

void WorkerThreadableWebSocketChannel::Bridge::connect(const URL& url, const String& protocol)
{
    ASSERT(m_workerClientWrapper);
    if (!m_peer)
        return;

    Peer* peer = m_peer;
    URLCapture capturedURL(url);
    StringCapture capturedProtocol(protocol);
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([peer, capturedURL, capturedProtocol] (ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->connect(capturedURL.url(), capturedProtocol.string());
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [peer, capturedURL, capturedProtocol](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->connect(capturedURL.url(), capturedProtocol.string());
    });
    m_loaderProxy.postTaskToLoader(p);
#endif
}

ThreadableWebSocketChannel::SendResult WorkerThreadableWebSocketChannel::Bridge::send(const String& message)
{
    if (!m_workerClientWrapper || !m_peer)
        return ThreadableWebSocketChannel::SendFail;
    setMethodNotCompleted();

    Peer* peer = m_peer;
    StringCapture capturedMessage(message);
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([peer, capturedMessage] (ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->send(capturedMessage.string());
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [peer, capturedMessage](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->send(capturedMessage.string());
    });
    m_loaderProxy.postTaskToLoader(p);
#endif

    Ref<Bridge> protect(*this);
    waitForMethodCompletion();
    ThreadableWebSocketChannelClientWrapper* clientWrapper = m_workerClientWrapper.get();
    if (!clientWrapper)
        return ThreadableWebSocketChannel::SendFail;
    return clientWrapper->sendRequestResult();
}

ThreadableWebSocketChannel::SendResult WorkerThreadableWebSocketChannel::Bridge::send(const ArrayBuffer& binaryData, unsigned byteOffset, unsigned byteLength)
{
    if (!m_workerClientWrapper || !m_peer)
        return ThreadableWebSocketChannel::SendFail;

    // ArrayBuffer isn't thread-safe, hence the content of ArrayBuffer is copied into Vector<char>.
    Vector<char>* dataPtr = std::make_unique<Vector<char>>(byteLength).release();
    if (binaryData.byteLength())
        memcpy(dataPtr->data(), static_cast<const char*>(binaryData.data()) + byteOffset, byteLength);
    setMethodNotCompleted();

    Peer* peer = m_peer;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([peer, dataPtr] (ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        std::unique_ptr<Vector<char>> data(dataPtr);
        RefPtr<ArrayBuffer> arrayBuffer = ArrayBuffer::create(data->data(), data->size());
        peer->send(*arrayBuffer);
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [peer, dataPtr](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        std::unique_ptr<Vector<char>> data(dataPtr);
        RefPtr<ArrayBuffer> arrayBuffer = ArrayBuffer::create(data->data(), data->size());
        peer->send(*arrayBuffer);
    });
    m_loaderProxy.postTaskToLoader(p);
#endif

    Ref<Bridge> protect(*this);
    waitForMethodCompletion();
    ThreadableWebSocketChannelClientWrapper* clientWrapper = m_workerClientWrapper.get();
    if (!clientWrapper)
        return ThreadableWebSocketChannel::SendFail;
    return clientWrapper->sendRequestResult();
}

ThreadableWebSocketChannel::SendResult WorkerThreadableWebSocketChannel::Bridge::send(Blob& binaryData)
{
    if (!m_workerClientWrapper || !m_peer)
        return ThreadableWebSocketChannel::SendFail;
    setMethodNotCompleted();

    Peer* peer = m_peer;
    URLCapture capturedURL(binaryData.url());
    StringCapture capturedType(binaryData.type());
    long long size = binaryData.size();
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([peer, capturedURL, capturedType, size] (ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->send(Blob::deserialize(capturedURL.url(), capturedType.string(), size));
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [peer, capturedURL, capturedType, size](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->send(Blob::deserialize(capturedURL.url(), capturedType.string(), size));
    });
    m_loaderProxy.postTaskToLoader(p);
#endif

    Ref<Bridge> protect(*this);
    waitForMethodCompletion();
    ThreadableWebSocketChannelClientWrapper* clientWrapper = m_workerClientWrapper.get();
    if (!clientWrapper)
        return ThreadableWebSocketChannel::SendFail;
    return clientWrapper->sendRequestResult();
}

unsigned long WorkerThreadableWebSocketChannel::Bridge::bufferedAmount()
{
    if (!m_workerClientWrapper || !m_peer)
        return 0;
    setMethodNotCompleted();

    Peer* peer = m_peer;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([peer] (ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->bufferedAmount();
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [peer](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->bufferedAmount();
    });
    m_loaderProxy.postTaskToLoader(p);
#endif

    Ref<Bridge> protect(*this);
    waitForMethodCompletion();
    ThreadableWebSocketChannelClientWrapper* clientWrapper = m_workerClientWrapper.get();
    if (clientWrapper)
        return clientWrapper->bufferedAmount();
    return 0;
}

void WorkerThreadableWebSocketChannel::Bridge::close(int code, const String& reason)
{
    if (!m_peer)
        return;

    Peer* peer = m_peer;
    StringCapture capturedReason(reason);
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([peer, code, capturedReason] (ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->close(code, capturedReason.string());
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [peer, code, capturedReason](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->close(code, capturedReason.string());
    });
    m_loaderProxy.postTaskToLoader(p);
#endif
}

void WorkerThreadableWebSocketChannel::Bridge::fail(const String& reason)
{
    if (!m_peer)
        return;

    Peer* peer = m_peer;
    StringCapture capturedReason(reason);
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([peer, capturedReason] (ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->fail(capturedReason.string());
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [peer, capturedReason](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->fail(capturedReason.string());
    });
    m_loaderProxy.postTaskToLoader(p);
#endif
}

void WorkerThreadableWebSocketChannel::Bridge::disconnect()
{
    clearClientWrapper();
    if (m_peer) {
        Peer* peer = m_peer;
#if !PLATFORM(WKC)
        m_loaderProxy.postTaskToLoader([peer] (ScriptExecutionContext& context) {
            ASSERT(isMainThread());
            ASSERT_UNUSED(context, context.isDocument());
            delete peer;
        });
#else
        std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [peer] (ScriptExecutionContext& context) {
            ASSERT(isMainThread());
            ASSERT_UNUSED(context, context.isDocument());
            delete peer;
        });
        m_loaderProxy.postTaskToLoader(p);
#endif
        m_peer = nullptr;
    }
    m_workerGlobalScope = nullptr;
}

void WorkerThreadableWebSocketChannel::Bridge::suspend()
{
    if (!m_peer)
        return;

    Peer* peer = m_peer;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([peer] (ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->suspend();
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [peer](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->suspend();
    });
    m_loaderProxy.postTaskToLoader(p);
#endif
}

void WorkerThreadableWebSocketChannel::Bridge::resume()
{
    if (!m_peer)
        return;

    Peer* peer = m_peer;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([peer] (ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->resume();
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [peer](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        ASSERT(peer);

        peer->resume();
    });
    m_loaderProxy.postTaskToLoader(p);
#endif
}

void WorkerThreadableWebSocketChannel::Bridge::clearClientWrapper()
{
    m_workerClientWrapper->clearClient();
}

void WorkerThreadableWebSocketChannel::Bridge::setMethodNotCompleted()
{
    ASSERT(m_workerClientWrapper);
    m_workerClientWrapper->clearSyncMethodDone();
}

// Caller of this function should hold a reference to the bridge, because this function may call WebSocket::didClose() in the end,
// which causes the bridge to get disconnected from the WebSocket and deleted if there is no other reference.
void WorkerThreadableWebSocketChannel::Bridge::waitForMethodCompletion()
{
    if (!m_workerGlobalScope)
        return;
    WorkerRunLoop& runLoop = m_workerGlobalScope->thread().runLoop();
    MessageQueueWaitResult result = MessageQueueMessageReceived;
    ThreadableWebSocketChannelClientWrapper* clientWrapper = m_workerClientWrapper.get();
    while (m_workerGlobalScope && clientWrapper && !clientWrapper->syncMethodDone() && result != MessageQueueTerminated) {
        result = runLoop.runInMode(m_workerGlobalScope.get(), m_taskMode); // May cause this bridge to get disconnected, which makes m_workerGlobalScope become null.
        clientWrapper = m_workerClientWrapper.get();
    }
}

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)

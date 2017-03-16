/*
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
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
#include "WorkerThreadableLoader.h"

#include "Document.h"
#include "DocumentThreadableLoader.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "SecurityOrigin.h"
#include "ThreadableLoader.h"
#include "WorkerGlobalScope.h"
#include "WorkerLoaderProxy.h"
#include "WorkerThread.h"
#include <wtf/MainThread.h>
#include <wtf/Vector.h>

namespace WebCore {

static const char loadResourceSynchronouslyMode[] = "loadResourceSynchronouslyMode";

WorkerThreadableLoader::WorkerThreadableLoader(WorkerGlobalScope* workerGlobalScope, ThreadableLoaderClient* client, const String& taskMode, const ResourceRequest& request, const ThreadableLoaderOptions& options)
    : m_workerGlobalScope(workerGlobalScope)
    , m_workerClientWrapper(ThreadableLoaderClientWrapper::create(client))
    , m_bridge(*(new MainThreadBridge(m_workerClientWrapper, m_workerGlobalScope->thread().workerLoaderProxy(), taskMode, request, options, workerGlobalScope->url().strippedForUseAsReferrer())))
{
}

WorkerThreadableLoader::~WorkerThreadableLoader()
{
    m_bridge.destroy();
}

void WorkerThreadableLoader::loadResourceSynchronously(WorkerGlobalScope* workerGlobalScope, const ResourceRequest& request, ThreadableLoaderClient& client, const ThreadableLoaderOptions& options)
{
    WorkerRunLoop& runLoop = workerGlobalScope->thread().runLoop();

    // Create a unique mode just for this synchronous resource load.
    String mode = loadResourceSynchronouslyMode;
    mode.append(String::number(runLoop.createUniqueId()));

    RefPtr<WorkerThreadableLoader> loader = WorkerThreadableLoader::create(workerGlobalScope, &client, mode, request, options);
    MessageQueueWaitResult result = MessageQueueMessageReceived;
    while (!loader->done() && result != MessageQueueTerminated)
        result = runLoop.runInMode(workerGlobalScope, mode);

    if (!loader->done() && result == MessageQueueTerminated)
        loader->cancel();
}

void WorkerThreadableLoader::cancel()
{
    m_bridge.cancel();
}

WorkerThreadableLoader::MainThreadBridge::MainThreadBridge(PassRefPtr<ThreadableLoaderClientWrapper> workerClientWrapper, WorkerLoaderProxy& loaderProxy, const String& taskMode,
                                                           const ResourceRequest& request, const ThreadableLoaderOptions& options, const String& outgoingReferrer)
    : m_workerClientWrapper(workerClientWrapper)
    , m_loaderProxy(loaderProxy)
    , m_taskMode(taskMode.isolatedCopy())
{
    ASSERT(m_workerClientWrapper.get());

    auto* requestData = request.copyData().release();
    auto* optionsCopy = options.isolatedCopy().release();
    StringCapture capturedOutgoingReferrer(outgoingReferrer);
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([this, requestData, optionsCopy, capturedOutgoingReferrer](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        Document& document = downcast<Document>(context);

        auto request = ResourceRequest::adopt(std::unique_ptr<CrossThreadResourceRequestData>(requestData));
        request->setHTTPReferrer(capturedOutgoingReferrer.string());

        auto options = std::unique_ptr<ThreadableLoaderOptions>(optionsCopy);

        // FIXME: If the a site requests a local resource, then this will return a non-zero value but the sync path
        // will return a 0 value. Either this should return 0 or the other code path should do a callback with
        // a failure.
        m_mainThreadLoader = DocumentThreadableLoader::create(document, *this, *request, *options);
        ASSERT(m_mainThreadLoader);
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [this, requestData, optionsCopy, capturedOutgoingReferrer](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        Document& document = downcast<Document>(context);

        auto request = ResourceRequest::adopt(std::unique_ptr<CrossThreadResourceRequestData>(requestData));
        request->setHTTPReferrer(capturedOutgoingReferrer.string());

        auto options = std::unique_ptr<ThreadableLoaderOptions>(optionsCopy);

        // FIXME: If the a site requests a local resource, then this will return a non-zero value but the sync path
        // will return a 0 value. Either this should return 0 or the other code path should do a callback with
        // a failure.
        m_mainThreadLoader = DocumentThreadableLoader::create(document, *this, *request, *options);
#if !PLATFORM(WKC)
        ASSERT(m_mainThreadLoader);
#endif
    });
    m_loaderProxy.postTaskToLoader(p);
#endif
}

void WorkerThreadableLoader::MainThreadBridge::destroy()
{
    // Ensure that no more client callbacks are done in the worker context's thread.
    clearClientWrapper();

    // "delete this" and m_mainThreadLoader::deref() on the worker object's thread.
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([this] (ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        delete this;
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [this](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());
        delete this;
    });
    m_loaderProxy.postTaskToLoader(p);
#endif
}

void WorkerThreadableLoader::MainThreadBridge::cancel()
{
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskToLoader([this] (ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());

        if (!m_mainThreadLoader)
            return;
        m_mainThreadLoader->cancel();
        m_mainThreadLoader = nullptr;
    });
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [this](ScriptExecutionContext& context) {
        ASSERT(isMainThread());
        ASSERT_UNUSED(context, context.isDocument());

        if (!m_mainThreadLoader)
            return;
        m_mainThreadLoader->cancel();
        m_mainThreadLoader = nullptr;
    });
    m_loaderProxy.postTaskToLoader(p);
#endif

    ThreadableLoaderClientWrapper* clientWrapper = m_workerClientWrapper.get();
    if (!clientWrapper->done()) {
        // If the client hasn't reached a termination state, then transition it by sending a cancellation error.
        // Note: no more client callbacks will be done after this method -- the clearClientWrapper() call ensures that.
        ResourceError error(String(), 0, String(), String());
        error.setIsCancellation(true);
        clientWrapper->didFail(error);
    }
    clearClientWrapper();
}

void WorkerThreadableLoader::MainThreadBridge::clearClientWrapper()
{
    m_workerClientWrapper->clearClient();
}

void WorkerThreadableLoader::MainThreadBridge::didSendData(unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper = m_workerClientWrapper;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, bytesSent, totalBytesToBeSent] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didSendData(bytesSent, totalBytesToBeSent);
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, bytesSent, totalBytesToBeSent](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didSendData(bytesSent, totalBytesToBeSent);
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

void WorkerThreadableLoader::MainThreadBridge::didReceiveResponse(unsigned long identifier, const ResourceResponse& response)
{
    RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper = m_workerClientWrapper;
    auto* responseData = response.copyData().release();
#if !PLATFORM(WKC)
    if (!m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, identifier, responseData] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        auto response(ResourceResponse::adopt(std::unique_ptr<CrossThreadResourceResponseData>(responseData)));
        workerClientWrapper->didReceiveResponse(identifier, *response);
    }, m_taskMode))
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, identifier, responseData](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        auto response(ResourceResponse::adopt(std::unique_ptr<CrossThreadResourceResponseData>(responseData)));
        workerClientWrapper->didReceiveResponse(identifier, *response);
    });
    if (!m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode))
#endif
        delete responseData;
}

void WorkerThreadableLoader::MainThreadBridge::didReceiveData(const char* data, int dataLength)
{
    RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper = m_workerClientWrapper;
    Vector<char>* vectorPtr = new Vector<char>(dataLength);
    memcpy(vectorPtr->data(), data, dataLength);
#if !PLATFORM(WKC)
    if (!m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, vectorPtr] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didReceiveData(vectorPtr->data(), vectorPtr->size());
        delete vectorPtr;
    }, m_taskMode))
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, vectorPtr](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didReceiveData(vectorPtr->data(), vectorPtr->size());
        delete vectorPtr;
    });
    if (!m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode))
#endif
        delete vectorPtr;
}

void WorkerThreadableLoader::MainThreadBridge::didFinishLoading(unsigned long identifier, double finishTime)
{
    RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper = m_workerClientWrapper;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, identifier, finishTime] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didFinishLoading(identifier, finishTime);
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, identifier, finishTime](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didFinishLoading(identifier, finishTime);
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

void WorkerThreadableLoader::MainThreadBridge::didFail(const ResourceError& error)
{
    RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper = m_workerClientWrapper;
    ResourceError* capturedError = new ResourceError(error.copy());
#if !PLATFORM(WKC)
    if (!m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, capturedError] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didFail(*capturedError);
        delete capturedError;
    }, m_taskMode))
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, capturedError](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didFail(*capturedError);
        delete capturedError;
    });
    if (!m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode))
#endif
        delete capturedError;
}

void WorkerThreadableLoader::MainThreadBridge::didFailAccessControlCheck(const ResourceError& error)
{
    RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper = m_workerClientWrapper;
    ResourceError* capturedError = new ResourceError(error.copy());
#if !PLATFORM(WKC)
    if (!m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper, capturedError] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didFailAccessControlCheck(*capturedError);
        delete capturedError;
    }, m_taskMode))
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper, capturedError](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didFailAccessControlCheck(*capturedError);
        delete capturedError;
    });
    if (!m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode))
#endif
        delete capturedError;
}

void WorkerThreadableLoader::MainThreadBridge::didFailRedirectCheck()
{
    RefPtr<ThreadableLoaderClientWrapper> workerClientWrapper = m_workerClientWrapper;
#if !PLATFORM(WKC)
    m_loaderProxy.postTaskForModeToWorkerGlobalScope([workerClientWrapper] (ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didFailRedirectCheck();
    }, m_taskMode);
#else
    std::function<void(ScriptExecutionContext&)> p(std::allocator_arg, WTF::voidScriptExecutionContextFuncAllocator(), [workerClientWrapper](ScriptExecutionContext& context) {
        ASSERT_UNUSED(context, context.isWorkerGlobalScope());
        workerClientWrapper->didFailRedirectCheck();
    });
    m_loaderProxy.postTaskForModeToWorkerGlobalScope(p, m_taskMode);
#endif
}

} // namespace WebCore

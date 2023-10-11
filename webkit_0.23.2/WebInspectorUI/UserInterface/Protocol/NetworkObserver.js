/*
 * Copyright (C) 2013, 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.NetworkObserver = class NetworkObserver
{
    // Events defined by the "Network" domain.

    requestWillBeSent(requestId, frameId, loaderId, documentURL, request, timestamp, initiator, redirectResponse, type)
    {
        WebInspector.frameResourceManager.resourceRequestWillBeSent(requestId, frameId, loaderId, request, type, redirectResponse, timestamp, initiator);
    }

    requestServedFromCache(requestId)
    {
        WebInspector.frameResourceManager.markResourceRequestAsServedFromMemoryCache(requestId);
    }

    responseReceived(requestId, frameId, loaderId, timestamp, type, response)
    {
        WebInspector.frameResourceManager.resourceRequestDidReceiveResponse(requestId, frameId, loaderId, type, response, timestamp);
    }

    dataReceived(requestId, timestamp, dataLength, encodedDataLength)
    {
        WebInspector.frameResourceManager.resourceRequestDidReceiveData(requestId, dataLength, encodedDataLength, timestamp);
    }

    loadingFinished(requestId, timestamp, sourceMapURL)
    {
        WebInspector.frameResourceManager.resourceRequestDidFinishLoading(requestId, timestamp, sourceMapURL);
    }

    loadingFailed(requestId, timestamp, errorText, canceled)
    {
        WebInspector.frameResourceManager.resourceRequestDidFailLoading(requestId, canceled, timestamp);
    }

    requestServedFromMemoryCache(requestId, frameId, loaderId, documentURL, timestamp, initiator, resource)
    {
        WebInspector.frameResourceManager.resourceRequestWasServedFromMemoryCache(requestId, frameId, loaderId, resource, timestamp, initiator);
    }

    webSocketWillSendHandshakeRequest(requestId, timestamp, request)
    {
        // FIXME: Not implemented.
    }

    webSocketHandshakeResponseReceived(requestId, timestamp, response)
    {
        // FIXME: Not implemented.
    }

    webSocketCreated(requestId, url)
    {
        // FIXME: Not implemented.
    }

    webSocketClosed(requestId, timestamp)
    {
        // FIXME: Not implemented.
    }

    webSocketFrameReceived(requestId, timestamp, response)
    {
        // FIXME: Not implemented.
    }

    webSocketFrameError(requestId, timestamp, errorMessage)
    {
        // FIXME: Not implemented.
    }

    webSocketFrameSent(requestId, timestamp, response)
    {
        // FIXME: Not implemented.
    }
};

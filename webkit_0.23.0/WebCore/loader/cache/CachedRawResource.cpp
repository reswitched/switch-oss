/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "CachedRawResource.h"

#include "CachedRawResourceClient.h"
#include "CachedResourceClientWalker.h"
#include "CachedResourceLoader.h"
#include "HTTPHeaderNames.h"
#include "SharedBuffer.h"
#include "SubresourceLoader.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/StringView.h>

namespace WebCore {

CachedRawResource::CachedRawResource(ResourceRequest& resourceRequest, Type type, SessionID sessionID)
    : CachedResource(resourceRequest, type, sessionID)
    , m_identifier(0)
    , m_allowEncodedDataReplacement(true)
{
    ASSERT(isMainOrRawResource());
}

const char* CachedRawResource::calculateIncrementalDataChunk(SharedBuffer* data, unsigned& incrementalDataLength)
{
    incrementalDataLength = 0;
    if (!data)
        return 0;

    unsigned previousDataLength = encodedSize();
    ASSERT(data->size() >= previousDataLength);
    incrementalDataLength = data->size() - previousDataLength;
    return data->data() + previousDataLength;
}

void CachedRawResource::addDataBuffer(SharedBuffer& data)
{
    CachedResourceHandle<CachedRawResource> protect(this);
    ASSERT(dataBufferingPolicy() == BufferData);
    m_data = &data;

    unsigned incrementalDataLength;
    const char* incrementalData = calculateIncrementalDataChunk(&data, incrementalDataLength);
    setEncodedSize(data.size());
    notifyClientsDataWasReceived(incrementalData, incrementalDataLength);
    if (dataBufferingPolicy() == DoNotBufferData) {
        if (m_loader)
            m_loader->setDataBufferingPolicy(DoNotBufferData);
        clear();
        return;
    }

    CachedResource::addDataBuffer(data);
}

void CachedRawResource::addData(const char* data, unsigned length)
{
    ASSERT(dataBufferingPolicy() == DoNotBufferData);
    notifyClientsDataWasReceived(data, length);
    CachedResource::addData(data, length);
}

void CachedRawResource::finishLoading(SharedBuffer* data)
{
    CachedResourceHandle<CachedRawResource> protect(this);
    DataBufferingPolicy dataBufferingPolicy = this->dataBufferingPolicy();
    if (dataBufferingPolicy == BufferData) {
        m_data = data;

        unsigned incrementalDataLength;
        const char* incrementalData = calculateIncrementalDataChunk(data, incrementalDataLength);
        if (data)
            setEncodedSize(data->size());
        notifyClientsDataWasReceived(incrementalData, incrementalDataLength);
    }

    m_allowEncodedDataReplacement = !m_loader->isQuickLookResource();

    CachedResource::finishLoading(data);
    if (dataBufferingPolicy == BufferData && this->dataBufferingPolicy() == DoNotBufferData) {
        if (m_loader)
            m_loader->setDataBufferingPolicy(DoNotBufferData);
        clear();
    }
}

void CachedRawResource::notifyClientsDataWasReceived(const char* data, unsigned length)
{
    if (!length)
        return;

    CachedResourceHandle<CachedRawResource> protect(this);
    CachedResourceClientWalker<CachedRawResourceClient> w(m_clients);
    while (CachedRawResourceClient* c = w.next())
        c->dataReceived(this, data, length);
}

void CachedRawResource::didAddClient(CachedResourceClient* c)
{
    if (!hasClient(c))
        return;
    // The calls to the client can result in events running, potentially causing
    // this resource to be evicted from the cache and all clients to be removed,
    // so a protector is necessary.
    CachedResourceHandle<CachedRawResource> protect(this);
    CachedRawResourceClient* client = static_cast<CachedRawResourceClient*>(c);
    size_t redirectCount = m_redirectChain.size();
    for (size_t i = 0; i < redirectCount; i++) {
        RedirectPair redirect = m_redirectChain[i];
        ResourceRequest request(redirect.m_request);
        client->redirectReceived(this, request, redirect.m_redirectResponse);
        if (!hasClient(c))
            return;
    }
    ASSERT(redirectCount == m_redirectChain.size());

    if (!m_response.isNull())
        client->responseReceived(this, m_response);
    if (!hasClient(c))
        return;
    if (m_data)
        client->dataReceived(this, m_data->data(), m_data->size());
    if (!hasClient(c))
       return;
    CachedResource::didAddClient(client);
}

void CachedRawResource::allClientsRemoved()
{
    if (m_loader)
        m_loader->cancelIfNotFinishing();
}

void CachedRawResource::redirectReceived(ResourceRequest& request, const ResourceResponse& response)
{
    CachedResourceHandle<CachedRawResource> protect(this);
    if (!response.isNull()) {
        CachedResourceClientWalker<CachedRawResourceClient> w(m_clients);
        while (CachedRawResourceClient* c = w.next())
            c->redirectReceived(this, request, response);
        m_redirectChain.append(RedirectPair(request, response));
    }
    CachedResource::redirectReceived(request, response);
}

void CachedRawResource::responseReceived(const ResourceResponse& response)
{
    CachedResourceHandle<CachedRawResource> protect(this);
    if (!m_identifier)
        m_identifier = m_loader->identifier();
    CachedResource::responseReceived(response);
    CachedResourceClientWalker<CachedRawResourceClient> w(m_clients);
    while (CachedRawResourceClient* c = w.next())
        c->responseReceived(this, m_response);
}

void CachedRawResource::didSendData(unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    CachedResourceClientWalker<CachedRawResourceClient> w(m_clients);
    while (CachedRawResourceClient* c = w.next())
        c->dataSent(this, bytesSent, totalBytesToBeSent);
}

void CachedRawResource::switchClientsToRevalidatedResource()
{
    ASSERT(m_loader);
    // If we're in the middle of a successful revalidation, responseReceived() hasn't been called, so we haven't set m_identifier.
    ASSERT(!m_identifier);
    downcast<CachedRawResource>(*resourceToRevalidate()).m_identifier = m_loader->identifier();
    CachedResource::switchClientsToRevalidatedResource();
}

void CachedRawResource::setDefersLoading(bool defers)
{
    if (m_loader)
        m_loader->setDefersLoading(defers);
}

void CachedRawResource::setDataBufferingPolicy(DataBufferingPolicy dataBufferingPolicy)
{
    m_options.setDataBufferingPolicy(dataBufferingPolicy);
}

static bool shouldIgnoreHeaderForCacheReuse(HTTPHeaderName name)
{
    switch (name) {
    // FIXME: This list of headers that don't affect cache policy almost certainly isn't complete.
    case HTTPHeaderName::Accept:
    case HTTPHeaderName::CacheControl:
    case HTTPHeaderName::Pragma:
    case HTTPHeaderName::Purpose:
    case HTTPHeaderName::Referer:
    case HTTPHeaderName::UserAgent:
        return true;

    default:
        return false;
    }
}

bool CachedRawResource::canReuse(const ResourceRequest& newRequest) const
{
    if (dataBufferingPolicy() == DoNotBufferData)
        return false;

    if (m_resourceRequest.httpMethod() != newRequest.httpMethod())
        return false;

    if (m_resourceRequest.httpBody() != newRequest.httpBody())
        return false;

    if (m_resourceRequest.allowCookies() != newRequest.allowCookies())
        return false;

    // Ensure most headers match the existing headers before continuing.
    // Note that the list of ignored headers includes some headers explicitly related to caching.
    // A more detailed check of caching policy will be performed later, this is simply a list of
    // headers that we might permit to be different and still reuse the existing CachedResource.
    const HTTPHeaderMap& newHeaders = newRequest.httpHeaderFields();
    const HTTPHeaderMap& oldHeaders = m_resourceRequest.httpHeaderFields();

    for (const auto& header : newHeaders) {
        if (header.keyAsHTTPHeaderName) {
            if (!shouldIgnoreHeaderForCacheReuse(header.keyAsHTTPHeaderName.value())
                && header.value != oldHeaders.commonHeaders().get(header.keyAsHTTPHeaderName.value()))
                return false;
        } else if (header.value != oldHeaders.uncommonHeaders().get(header.key))
            return false;
    }

    // For this second loop, we don't actually need to compare values, checking that the
    // key is contained in newHeaders is sufficient due to the previous loop.
    for (const auto& header : oldHeaders) {
        if (header.keyAsHTTPHeaderName) {
            if (!shouldIgnoreHeaderForCacheReuse(header.keyAsHTTPHeaderName.value())
                && !newHeaders.commonHeaders().contains(header.keyAsHTTPHeaderName.value()))
                return false;
        } else if (!newHeaders.uncommonHeaders().contains(header.key))
            return false;
    }

    return true;
}

void CachedRawResource::clear()
{
    m_data = nullptr;
    setEncodedSize(0);
    if (m_loader)
        m_loader->clearResourceData();
}

} // namespace WebCore

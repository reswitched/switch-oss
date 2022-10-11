/*
 * Copyright (C) 2014 Igalia S.L
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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

#ifndef PlatformMediaResourceLoader_h
#define PlatformMediaResourceLoader_h

#if ENABLE(VIDEO)

#include <wtf/Noncopyable.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class ResourceError;
class ResourceRequest;
class ResourceResponse;

class PlatformMediaResourceLoaderClient {
public:
    virtual ~PlatformMediaResourceLoaderClient() { }

    virtual void responseReceived(const ResourceResponse&) { }
    virtual void dataReceived(const char*, int) { }
    virtual void accessControlCheckFailed(const ResourceError&) { }
    virtual void loadFailed(const ResourceError&) { }
    virtual void loadFinished() { }
#if USE(SOUP)
    virtual char* getOrCreateReadBuffer(size_t /*requestedSize*/, size_t& /*actualSize*/) { return nullptr; };
#endif
};

class PlatformMediaResourceLoader : public RefCounted<PlatformMediaResourceLoader> {
    WTF_MAKE_NONCOPYABLE(PlatformMediaResourceLoader); WTF_MAKE_FAST_ALLOCATED;
public:
    enum LoadOption {
        BufferData = 1 << 0,
    };
    typedef unsigned LoadOptions;

    virtual ~PlatformMediaResourceLoader() { }

    virtual bool start(const ResourceRequest&, LoadOptions) = 0;
    virtual void stop() { }
    virtual void setDefersLoading(bool) { }
    virtual bool didPassAccessControlCheck() const { return false; }

protected:
    explicit PlatformMediaResourceLoader(std::unique_ptr<PlatformMediaResourceLoaderClient> client)
        : m_client(WTF::move(client))
    {
    }

    std::unique_ptr<PlatformMediaResourceLoaderClient> m_client;
};

} // namespace WebCore

#endif
#endif

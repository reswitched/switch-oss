/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#ifndef PlatformContentFilter_h
#define PlatformContentFilter_h

#include <wtf/Ref.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ContentFilterUnblockHandler;
class ResourceRequest;
class ResourceResponse;
class SharedBuffer;

class PlatformContentFilter {
    WTF_MAKE_FAST_ALLOCATED;
    WTF_MAKE_NONCOPYABLE(PlatformContentFilter);

protected:
    PlatformContentFilter() = default;

public:
    virtual ~PlatformContentFilter() { }
    virtual void willSendRequest(ResourceRequest&, const ResourceResponse&) = 0;
    virtual void responseReceived(const ResourceResponse&) = 0;
    virtual void addData(const char* data, int length) = 0;
    virtual void finishedAddingData() = 0;
    virtual bool needsMoreData() const = 0;
    virtual bool didBlockData() const = 0;
    virtual Ref<SharedBuffer> replacementData() const = 0;
    virtual ContentFilterUnblockHandler unblockHandler() const = 0;
    virtual String unblockRequestDeniedScript() const { return emptyString(); }
};

} // namespace WebCore

#endif // PlatformContentFilter_h

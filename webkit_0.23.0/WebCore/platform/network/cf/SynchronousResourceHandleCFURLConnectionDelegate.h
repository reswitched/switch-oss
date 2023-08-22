/*
 * Copyright (C) 2004-2013 Apple Inc.  All rights reserved.
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

#ifndef SynchronousResourceHandleCFURLConnectionDelegate_h
#define SynchronousResourceHandleCFURLConnectionDelegate_h

#if USE(CFNETWORK)

#include "ResourceHandleCFURLConnectionDelegate.h"

namespace WebCore {

class SynchronousResourceHandleCFURLConnectionDelegate final : public ResourceHandleCFURLConnectionDelegate {
public:
    SynchronousResourceHandleCFURLConnectionDelegate(ResourceHandle*);

    virtual void didReceiveData(CFDataRef, CFIndex originalLength) override;
    virtual void didFinishLoading() override;
    virtual void didFail(CFErrorRef) override;
#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
    virtual void didReceiveDataArray(CFArrayRef dataArray) override;
#endif // USE(NETWORK_CFDATA_ARRAY_CALLBACK)

private:
    virtual void setupRequest(CFMutableURLRequestRef) override;
    virtual void setupConnectionScheduling(CFURLConnectionRef) override;

    virtual CFURLRequestRef willSendRequest(CFURLRequestRef, CFURLResponseRef) override;
    virtual void didReceiveResponse(CFURLConnectionRef, CFURLResponseRef) override;
    virtual CFCachedURLResponseRef willCacheResponse(CFCachedURLResponseRef) override;
    virtual void didReceiveChallenge(CFURLAuthChallengeRef) override;
    virtual void didSendBodyData(CFIndex totalBytesWritten, CFIndex totalBytesExpectedToWrite) override;
    virtual Boolean shouldUseCredentialStorage() override;
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    virtual Boolean canRespondToProtectionSpace(CFURLProtectionSpaceRef) override;
#endif // USE(PROTECTION_SPACE_AUTH_CALLBACK)

    virtual void continueWillSendRequest(CFURLRequestRef) override;
    virtual void continueDidReceiveResponse() override;
    virtual void continueWillCacheResponse(CFCachedURLResponseRef) override;
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    virtual void continueCanAuthenticateAgainstProtectionSpace(bool) override;
#endif // USE(PROTECTION_SPACE_AUTH_CALLBACK)
};

} // namespace WebCore.

#endif // USE(CFNETWORK)

#endif // ResourceHandleCFURLConnectionDelegate_h

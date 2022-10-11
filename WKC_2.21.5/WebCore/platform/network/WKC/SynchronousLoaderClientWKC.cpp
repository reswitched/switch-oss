/*
* Copyright (C) 2015-2022 ACCESS CO., LTD. All rights reserved.
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

#include "SynchronousLoaderClient.h"

#include "AuthenticationChallenge.h"
#include "FrameLoaderClientWKC.h"
#include "FrameLoader.h"
#include "ResourceHandleInternalWKC.h"

#include <curl/curl.h>

namespace WebCore {

void SynchronousLoaderClient::didReceiveAuthenticationChallenge(ResourceHandle* handle, const AuthenticationChallenge& challenge)
{
    ASSERT(handle->hasAuthenticationChallenge());

    if (handle->shouldUseCredentialStorage()) {
        NetworkingContext* ctx = handle->context();
        if (ctx) {
            Frame* frame = reinterpret_cast<WKC::FrameNetworkingContextWKC *>(ctx)->coreFrame();
            if (frame) {
                FrameLoaderClient* flc = ctx->frameLoaderClient();
                if (flc && flc->byWKC()) {
                    flc->dispatchDidReceiveAuthenticationChallenge(frame->loader().documentLoader(), 0 /* FIXME */, challenge);
                    return;
                }
            }
        }
    }

    handle->receivedRequestToContinueWithoutCredential(challenge);

    ASSERT(!handle->hasAuthenticationChallenge());
}

ResourceError SynchronousLoaderClient::platformBadResponseError()
{
    int errorCode = 0;
    URL failingURL;
    String localizedDescription("Bad Server Response");

    return ResourceError("CURL"_s, errorCode, failingURL, localizedDescription);
}

} // namespace WebCore

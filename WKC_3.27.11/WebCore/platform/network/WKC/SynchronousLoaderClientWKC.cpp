/*
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
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
                    flc->dispatchDidReceiveAuthenticationChallenge(frame->loader().documentLoader(), ResourceLoaderIdentifier::generate(), challenge);
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

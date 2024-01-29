/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "PingLoader.h"

#include "Document.h"
#include "FormData.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "HTTPHeaderNames.h"
#include "InspectorInstrumentation.h"
#include "LoaderStrategy.h"
#include "Page.h"
#include "PlatformStrategies.h"
#include "ProgressTracker.h"
#include "ResourceHandle.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"
#include <wtf/text/CString.h>

namespace WebCore {

void PingLoader::loadImage(Frame& frame, const URL& url)
{
    if (!frame.document()->securityOrigin().canDisplay(url)) {
        FrameLoader::reportLocalLoadFailed(&frame, url);
        return;
    }

    ResourceRequest request(url);
    request.setHTTPHeaderField(HTTPHeaderName::CacheControl, "max-age=0");
    String referrer = SecurityPolicy::generateReferrerHeader(frame.document()->referrerPolicy(), request.url(), frame.loader().outgoingReferrer());
    if (!referrer.isEmpty())
        request.setHTTPReferrer(referrer);
    frame.loader().addExtraFieldsToSubresourceRequest(request);

    startPingLoad(frame, request);
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/links.html#hyperlink-auditing
void PingLoader::sendPing(Frame& frame, const URL& pingURL, const URL& destinationURL)
{
    ResourceRequest request(pingURL);
    request.setHTTPMethod("POST");
    request.setHTTPContentType("text/ping");
    request.setHTTPBody(FormData::create("PING"));
    request.setHTTPHeaderField(HTTPHeaderName::CacheControl, "max-age=0");
    frame.loader().addExtraFieldsToSubresourceRequest(request);

    SecurityOrigin* sourceOrigin = &frame.document()->securityOrigin();
    Ref<SecurityOrigin> pingOrigin(SecurityOrigin::create(pingURL));
    FrameLoader::addHTTPOriginIfNeeded(request, sourceOrigin->toString());
    request.setHTTPHeaderField(HTTPHeaderName::PingTo, destinationURL);
    if (!SecurityPolicy::shouldHideReferrer(pingURL, frame.loader().outgoingReferrer())) {
        request.setHTTPHeaderField(HTTPHeaderName::PingFrom, frame.document()->url());
        if (!sourceOrigin->isSameSchemeHostPort(pingOrigin.get())) {
            String referrer = SecurityPolicy::generateReferrerHeader(frame.document()->referrerPolicy(), pingURL, frame.loader().outgoingReferrer());
            if (!referrer.isEmpty())
                request.setHTTPReferrer(referrer);
        }
    }

    startPingLoad(frame, request);
}

void PingLoader::sendViolationReport(Frame& frame, const URL& reportURL, PassRefPtr<FormData> report)
{
    ResourceRequest request(reportURL);
    request.setHTTPMethod("POST");
    request.setHTTPContentType("application/json");
    request.setHTTPBody(report);
    request.setAllowCookies(frame.document()->securityOrigin().isSameSchemeHostPort(SecurityOrigin::create(reportURL).get()));
    frame.loader().addExtraFieldsToSubresourceRequest(request);

    String referrer = SecurityPolicy::generateReferrerHeader(frame.document()->referrerPolicy(), reportURL, frame.loader().outgoingReferrer());
    if (!referrer.isEmpty())
        request.setHTTPReferrer(referrer);

    startPingLoad(frame, request);
}

void PingLoader::startPingLoad(Frame& frame, ResourceRequest& request)
{
    unsigned long identifier = frame.page()->progress().createUniqueIdentifier();
    // FIXME: Why activeDocumentLoader? I would have expected documentLoader().
    // Itseems like the PingLoader should be associated with the current
    // Document in the Frame, but the activeDocumentLoader will be associated
    // with the provisional DocumentLoader if there is a provisional
    // DocumentLoader.
    bool shouldUseCredentialStorage = frame.loader().client().shouldUseCredentialStorage(frame.loader().activeDocumentLoader(), identifier);

    InspectorInstrumentation::continueAfterPingLoader(frame, identifier, frame.loader().activeDocumentLoader(), request, ResourceResponse());

    platformStrategies()->loaderStrategy()->createPingHandle(frame.loader().networkingContext(), request, shouldUseCredentialStorage);
}

}

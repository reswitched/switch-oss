/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2012 Intel Inc. All rights reserved.
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
#include "PerformanceResourceTiming.h"

#if ENABLE(RESOURCE_TIMING)

#include "Document.h"
#include "DocumentLoadTiming.h"
#include "DocumentLoader.h"
#include "HTTPHeaderNames.h"
#include "URL.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "SecurityOrigin.h"
#include <wtf/Vector.h>

namespace WebCore {

static double monotonicTimeToDocumentMilliseconds(Document* document, double seconds)
{
    ASSERT(seconds >= 0.0);
    return Performance::reduceTimeResolution(document->loader()->timing().monotonicTimeToZeroBasedDocumentTime(seconds)) * 1000.0;
}

static bool passesTimingAllowCheck(const ResourceResponse& response, Document* requestingDocument)
{
    Ref<SecurityOrigin> resourceOrigin = SecurityOrigin::create(response.url());
    if (resourceOrigin->isSameSchemeHostPort(requestingDocument->securityOrigin()))
        return true;

    const String& timingAllowOriginString = response.httpHeaderField(HTTPHeaderName::TimingAllowOrigin);
    if (timingAllowOriginString.isEmpty() || equalIgnoringCase(timingAllowOriginString, "null"))
        return false;

    if (timingAllowOriginString == "*")
        return true;

    const String& securityOrigin = requestingDocument->securityOrigin().toString();
    Vector<String> timingAllowOrigins;
    timingAllowOriginString.split(' ', timingAllowOrigins);
    for (auto& origin : timingAllowOrigins) {
        if (origin == securityOrigin)
            return true;
    }

    return false;
}

PerformanceResourceTiming::PerformanceResourceTiming(const AtomicString& initiatorType, const ResourceRequest& request, const ResourceResponse& response, double initiationTime, double finishTime, Document* requestingDocument)
    : PerformanceEntry(request.url().string(), "resource", monotonicTimeToDocumentMilliseconds(requestingDocument, initiationTime), monotonicTimeToDocumentMilliseconds(requestingDocument, finishTime))
    , m_initiatorType(initiatorType)
    , m_timing(response.resourceLoadTiming())
    , m_finishTime(finishTime)
    , m_shouldReportDetails(passesTimingAllowCheck(response, requestingDocument))
    , m_requestingDocument(requestingDocument)
{
}

PerformanceResourceTiming::~PerformanceResourceTiming()
{
}

AtomicString PerformanceResourceTiming::initiatorType() const
{
    return m_initiatorType;
}

double PerformanceResourceTiming::redirectStart() const
{
    // FIXME: Need to track and report redirects for resources.
    if (!m_shouldReportDetails)
        return 0.0;
    return 0;
}

double PerformanceResourceTiming::redirectEnd() const
{
    if (!m_shouldReportDetails)
        return 0.0;
    return 0;
}

double PerformanceResourceTiming::fetchStart() const
{
    // FIXME: This should be different depending on redirects.
    return (startTime());
}

double PerformanceResourceTiming::domainLookupStart() const
{
    if (!m_shouldReportDetails)
        return 0.0;

    if (m_timing.domainLookupStart < 0)
        return fetchStart();

    return resourceTimeToDocumentMilliseconds(m_timing.domainLookupStart);
}

double PerformanceResourceTiming::domainLookupEnd() const
{
    if (!m_shouldReportDetails)
        return 0.0;

    if (m_timing.domainLookupEnd < 0)
        return domainLookupStart();

    return resourceTimeToDocumentMilliseconds(m_timing.domainLookupEnd);
}

double PerformanceResourceTiming::connectStart() const
{
    if (!m_shouldReportDetails)
        return 0.0;

    // connectStart will be -1 when a network request is not made.
    if (m_timing.connectStart < 0)
        return domainLookupEnd();

    // connectStart includes any DNS time, so we may need to trim that off.
    double connectStart = m_timing.connectStart;
    if (m_timing.domainLookupEnd >= 0)
        connectStart = m_timing.domainLookupEnd;

    return resourceTimeToDocumentMilliseconds(connectStart);
}

double PerformanceResourceTiming::connectEnd() const
{
    if (!m_shouldReportDetails)
        return 0.0;

    // connectStart will be -1 when a network request is not made.
    if (m_timing.connectEnd < 0)
        return connectStart();

    return resourceTimeToDocumentMilliseconds(m_timing.connectEnd);
}

double PerformanceResourceTiming::secureConnectionStart() const
{
    if (!m_shouldReportDetails)
        return 0.0;

    if (m_timing.secureConnectionStart < 0) // Secure connection not negotiated.
        return 0.0;

    return resourceTimeToDocumentMilliseconds(m_timing.secureConnectionStart);
}

double PerformanceResourceTiming::requestStart() const
{
    if (!m_shouldReportDetails)
        return 0.0;

    return resourceTimeToDocumentMilliseconds(m_timing.requestStart);
}

double PerformanceResourceTiming::responseEnd() const
{
#if !PLATFORM(WKC)
    return monotonicTimeToDocumentMilliseconds(m_requestingDocument.get(), m_finishTime);
#else
    return monotonicTimeToDocumentMilliseconds(m_requestingDocument, m_finishTime);
#endif
}

double PerformanceResourceTiming::resourceTimeToDocumentMilliseconds(double deltaMilliseconds) const
{
    if (!deltaMilliseconds)
        return 0.0;
    double documentStartTime = m_requestingDocument->loader()->timing().monotonicTimeToZeroBasedDocumentTime(m_requestingDocument->loader()->timing().navigationStart()) * 1000.0;
    double resourceTimeSeconds = (documentStartTime + deltaMilliseconds) / 1000.0;
    return 1000.0 * Performance::reduceTimeResolution(resourceTimeSeconds);
}

} // namespace WebCore

#endif // ENABLE(RESOURCE_TIMING)

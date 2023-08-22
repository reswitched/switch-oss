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

#import "config.h"
#import "ParentalControlsContentFilter.h"

#if HAVE(PARENTAL_CONTROLS)

#import "ContentFilterUnblockHandler.h"
#import "Logging.h"
#import "ResourceResponse.h"
#import "SharedBuffer.h"
#import "SoftLinking.h"
#import "WebFilterEvaluatorSPI.h"
#import <objc/runtime.h>

SOFT_LINK_PRIVATE_FRAMEWORK(WebContentAnalysis);
SOFT_LINK_CLASS(WebContentAnalysis, WebFilterEvaluator);

namespace WebCore {

bool ParentalControlsContentFilter::enabled()
{
    bool enabled = [getWebFilterEvaluatorClass() isManagedSession];
    LOG(ContentFiltering, "ParentalControlsContentFilter is %s.\n", enabled ? "enabled" : "not enabled");
    return enabled;
}

std::unique_ptr<ParentalControlsContentFilter> ParentalControlsContentFilter::create()
{
    return std::make_unique<ParentalControlsContentFilter>();
}

ParentalControlsContentFilter::ParentalControlsContentFilter()
    : m_filterState { kWFEStateBuffering }
{
    ASSERT([getWebFilterEvaluatorClass() isManagedSession]);
}

static inline bool canHandleResponse(const ResourceResponse& response)
{
#if PLATFORM(MAC)
    return response.url().protocolIs("https");
#else
    return response.url().protocolIsInHTTPFamily();
#endif
}

void ParentalControlsContentFilter::responseReceived(const ResourceResponse& response)
{
    ASSERT(!m_webFilterEvaluator);

    if (!canHandleResponse(response)) {
        m_filterState = kWFEStateAllowed;
        return;
    }

    m_webFilterEvaluator = adoptNS([allocWebFilterEvaluatorInstance() initWithResponse:response.nsURLResponse()]);
    updateFilterState();
}

void ParentalControlsContentFilter::addData(const char* data, int length)
{
    ASSERT(![m_replacementData.get() length]);
    m_replacementData = [m_webFilterEvaluator addData:[NSData dataWithBytesNoCopy:(void*)data length:length freeWhenDone:NO]];
    updateFilterState();
    ASSERT(needsMoreData() || [m_replacementData.get() length]);
}

void ParentalControlsContentFilter::finishedAddingData()
{
    ASSERT(![m_replacementData.get() length]);
    m_replacementData = [m_webFilterEvaluator dataComplete];
    updateFilterState();
}

bool ParentalControlsContentFilter::needsMoreData() const
{
    return m_filterState == kWFEStateBuffering;
}

bool ParentalControlsContentFilter::didBlockData() const
{
    return m_filterState == kWFEStateBlocked;
}

Ref<SharedBuffer> ParentalControlsContentFilter::replacementData() const
{
    ASSERT(didBlockData());
    return adoptRef(*SharedBuffer::wrapNSData(m_replacementData.get()).leakRef());
}

ContentFilterUnblockHandler ParentalControlsContentFilter::unblockHandler() const
{
#if PLATFORM(IOS)
    return ContentFilterUnblockHandler { ASCIILiteral("unblock"), m_webFilterEvaluator };
#else
    return { };
#endif
}

void ParentalControlsContentFilter::updateFilterState()
{
    m_filterState = [m_webFilterEvaluator filterState];
#if !LOG_DISABLED
    if (!needsMoreData())
        LOG(ContentFiltering, "ParentalControlsContentFilter stopped buffering with state %d and replacement data length %zu.\n", m_filterState, [m_replacementData length]);
#endif
}

} // namespace WebCore

#endif // HAVE(PARENTAL_CONTROLS)

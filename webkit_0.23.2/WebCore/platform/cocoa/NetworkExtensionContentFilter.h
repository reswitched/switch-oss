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

#ifndef NetworkExtensionContentFilter_h
#define NetworkExtensionContentFilter_h

#include "PlatformContentFilter.h"
#include <objc/NSObjCRuntime.h>
#include <wtf/Compiler.h>
#include <wtf/OSObjectPtr.h>
#include <wtf/RetainPtr.h>

#define HAVE_NETWORK_EXTENSION PLATFORM(IOS) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000 && CPU(X86_64))

enum NEFilterSourceStatus : NSInteger;

OBJC_CLASS NEFilterSource;
OBJC_CLASS NSData;

namespace WebCore {

class NetworkExtensionContentFilter final : public PlatformContentFilter {
    friend std::unique_ptr<NetworkExtensionContentFilter> std::make_unique<NetworkExtensionContentFilter>();

public:
    static bool enabled();
    static std::unique_ptr<NetworkExtensionContentFilter> create();

    void willSendRequest(ResourceRequest&, const ResourceResponse&) override;
    void responseReceived(const ResourceResponse&) override;
    void addData(const char* data, int length) override;
    void finishedAddingData() override;
    bool needsMoreData() const override;
    bool didBlockData() const override;
    Ref<SharedBuffer> replacementData() const override;
    ContentFilterUnblockHandler unblockHandler() const override;

private:
    NetworkExtensionContentFilter();
    void handleDecision(NEFilterSourceStatus, NSData *replacementData);

    NEFilterSourceStatus m_status;
    OSObjectPtr<dispatch_queue_t> m_queue;
    OSObjectPtr<dispatch_semaphore_t> m_semaphore;
    RetainPtr<NSData> m_replacementData;
    RetainPtr<NEFilterSource> m_neFilterSource;
};

} // namespace WebCore

#endif // NetworkExtensionContentFilter_h

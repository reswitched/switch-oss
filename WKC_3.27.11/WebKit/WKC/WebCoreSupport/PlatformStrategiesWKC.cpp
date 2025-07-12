/*
 * Copyright (C) 2010, 2011, 2016 Apple Inc. All rights reserved.
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

#include "config.h"

#include "PlatformStrategiesWKC.h"

#include "WebKitLegacy/WebCoreSupport/WebResourceLoadScheduler.h"
#include "AudioDestination.h"
#include "MediaStrategy.h"

#include "BlobRegistryWKC.h"

using namespace WebCore;

namespace WKC {

class MediaStrategyWKC : public WebCore::MediaStrategy
{
private:
#if ENABLE(WEB_AUDIO)
    Ref<WebCore::AudioDestination> createAudioDestination(WebCore::AudioIOCallback& callback, const WTF::String& inputDeviceId,
        unsigned numberOfInputChannels, unsigned numberOfOutputChannels, float sampleRate) override final
    {
        return AudioDestination::create(callback, inputDeviceId, numberOfInputChannels, numberOfOutputChannels, sampleRate);
    }
#endif
};
void PlatformStrategiesWKC::initialize()
{
    if (!hasPlatformStrategies()) {
        PlatformStrategiesWKC* platformStrategies = new PlatformStrategiesWKC();
        setPlatformStrategies(platformStrategies);
    }
}

PlatformStrategiesWKC::PlatformStrategiesWKC()
{
}

LoaderStrategy* PlatformStrategiesWKC::createLoaderStrategy()
{
    return new WebResourceLoadScheduler;
}

PasteboardStrategy* PlatformStrategiesWKC::createPasteboardStrategy()
{
    return nullptr;
}

WebCore::MediaStrategy* PlatformStrategiesWKC::createMediaStrategy()
{
    return new MediaStrategyWKC;
}

BlobRegistry* PlatformStrategiesWKC::createBlobRegistry()
{
    return new BlobRegistryWKC;
}

} // namespace

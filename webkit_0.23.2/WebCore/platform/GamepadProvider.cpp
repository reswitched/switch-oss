/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
#include "GamepadProvider.h"

#if ENABLE(GAMEPAD)

#include "GamepadProviderClient.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

#if !PLATFORM(WKC)
static GamepadProvider* sharedProvider = nullptr;
#else
WKC_DEFINE_GLOBAL_PTR(GamepadProvider*, sharedProvider, nullptr);
#endif

GamepadProvider& GamepadProvider::singleton()
{
    if (!sharedProvider) {
#if !PLATFORM(WKC)
        static NeverDestroyed<GamepadProvider> defaultProvider;
        sharedProvider = &defaultProvider.get();
#else
        sharedProvider = new GamepadProvider();
#endif
    }

    return *sharedProvider;
}

void GamepadProvider::setSharedProvider(GamepadProvider& newProvider)
{
    sharedProvider = &newProvider;
}

void GamepadProvider::startMonitoringGamepads(GamepadProviderClient*)
{
}

void GamepadProvider::stopMonitoringGamepads(GamepadProviderClient*)
{
}

const Vector<PlatformGamepad*>& GamepadProvider::platformGamepads()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<Vector<PlatformGamepad*>> defaultGamepads;
    return defaultGamepads;
#else
    WKC_DEFINE_STATIC_PTR(Vector<PlatformGamepad*>*, defaultGamepads, 0);
    if (!defaultGamepads)
        defaultGamepads = new Vector<PlatformGamepad*>();
    return *defaultGamepads;
#endif
}

void GamepadProvider::dispatchPlatformGamepadInputActivity()
{
    for (auto& client : m_clients)
        client->platformGamepadInputActivity(m_shouldMakeGamepadsVisible);

    m_shouldMakeGamepadsVisible = false;
}

} // namespace WebCore

#endif // ENABLE(GAMEPAD)

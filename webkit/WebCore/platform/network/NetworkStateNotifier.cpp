/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#include "config.h"
#include "NetworkStateNotifier.h"

#if PLATFORM(IOS)
#include "Settings.h"
#endif

#if !PLATFORM(WKC)
#include <mutex>
#else
#include <wkcmutex.h>
#endif
#include <wtf/Assertions.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

NetworkStateNotifier& networkStateNotifier()
{
#if !PLATFORM(WKC)
    static std::once_flag onceFlag;
    static LazyNeverDestroyed<NetworkStateNotifier> networkStateNotifier;

    std::call_once(onceFlag, []{
        networkStateNotifier.construct();
    });

    return networkStateNotifier;
#else
    WKC_DEFINE_STATIC_PTR(NetworkStateNotifier*, networkStateNotifier, 0);
    if (!networkStateNotifier)
        networkStateNotifier = new NetworkStateNotifier();
    return *networkStateNotifier;
#endif
}

void NetworkStateNotifier::addNetworkStateChangeListener(std::function<void (bool)> listener)
{
    ASSERT(listener);
#if PLATFORM(IOS)
    if (Settings::shouldOptOutOfNetworkStateObservation())
        return;
    registerObserverIfNecessary();
#endif

    m_listeners.append(WTF::move(listener));
}

void NetworkStateNotifier::notifyNetworkStateChange() const
{
    for (const auto& listener : m_listeners)
        listener(m_isOnLine);
}

#if PLATFORM(WKC)
void NetworkStateNotifier::setOnLine(bool onLine)
{
    if (m_isOnLine == onLine)
        return;

    m_isOnLine = onLine;

    notifyNetworkStateChange();
}
#endif

} // namespace WebCore

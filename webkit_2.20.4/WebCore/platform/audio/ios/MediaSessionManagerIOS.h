/*
 * Copyright (C) 2014-2017 Apple Inc. All rights reserved.
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

#pragma once

#if PLATFORM(IOS_FAMILY)

#include "MediaSessionManagerCocoa.h"
#include <wtf/RetainPtr.h>

OBJC_CLASS WebMediaSessionHelper;

#if defined(__OBJC__) && __OBJC__
extern NSString* WebUIApplicationWillResignActiveNotification;
extern NSString* WebUIApplicationWillEnterForegroundNotification;
extern NSString* WebUIApplicationDidBecomeActiveNotification;
extern NSString* WebUIApplicationDidEnterBackgroundNotification;
#endif

namespace WebCore {

class MediaSessionManageriOS : public MediaSessionManagerCocoa {
public:
    virtual ~MediaSessionManageriOS();

    void externalOutputDeviceAvailableDidChange();
    bool hasWirelessTargetsAvailable() override;
#if HAVE(CELESTIAL)
    void carPlayServerDied();
    void updateCarPlayIsConnected(Optional<bool>&&);
    void activeAudioRouteDidChange(Optional<bool>&&);
#endif

private:
    friend class PlatformMediaSessionManager;

    MediaSessionManageriOS();

    void resetRestrictions() override;

    void configureWireLessTargetMonitoring() override;
    void providePresentingApplicationPIDIfNecessary() final;
    void sessionWillEndPlayback(PlatformMediaSession&, DelayCallingUpdateNowPlaying) final;

#if !RELEASE_LOG_DISABLED
    const char* logClassName() const final { return "MediaSessionManageriOS"; }
#endif

    RetainPtr<WebMediaSessionHelper> m_objcObserver;
#if HAVE(CELESTIAL)
    bool m_havePresentedApplicationPID { false };
#endif
};

} // namespace WebCore

#endif // PLATFORM(IOS_FAMILY)

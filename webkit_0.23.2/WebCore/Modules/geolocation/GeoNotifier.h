/*
 * Copyright (C) 2008-2011, 2015 Apple Inc. All Rights Reserved.
 * Copyright 2010, The Android Open Source Project
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

#ifndef GeoNotifier_h
#define GeoNotifier_h

#if ENABLE(GEOLOCATION)

#include "Timer.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Geoposition;
class Geolocation;
class PositionCallback;
class PositionError;
class PositionErrorCallback;
class PositionOptions;

class GeoNotifier : public RefCounted<GeoNotifier> {
public:
    static Ref<GeoNotifier> create(Geolocation& geolocation, PassRefPtr<PositionCallback> positionCallback, PassRefPtr<PositionErrorCallback> positionErrorCallback, PassRefPtr<PositionOptions> options)
    {
        return adoptRef(*new GeoNotifier(geolocation, positionCallback, positionErrorCallback, options));
    }

    PositionOptions* options() const { return m_options.get(); }
    void setFatalError(PassRefPtr<PositionError>);

    bool useCachedPosition() const { return m_useCachedPosition; }
    void setUseCachedPosition();

    void runSuccessCallback(Geoposition*);
    void runErrorCallback(PositionError*);

    void startTimerIfNeeded();
    void stopTimer();
    void timerFired();
    bool hasZeroTimeout() const;

private:
    GeoNotifier(Geolocation&, PassRefPtr<PositionCallback>, PassRefPtr<PositionErrorCallback>, PassRefPtr<PositionOptions>);

    Ref<Geolocation> m_geolocation;
    RefPtr<PositionCallback> m_successCallback;
    RefPtr<PositionErrorCallback> m_errorCallback;
    RefPtr<PositionOptions> m_options;
    Timer m_timer;
    RefPtr<PositionError> m_fatalError;
    bool m_useCachedPosition;
};

} // namespace WebCore

#endif // ENABLE(GEOLOCATION)

#endif // GeoNotifier_h

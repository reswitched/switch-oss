/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
#include "GeolocationController.h"

#if ENABLE(GEOLOCATION)

#include "GeolocationClient.h"
#include "GeolocationError.h"
#include "GeolocationPosition.h"

namespace WebCore {

GeolocationController::GeolocationController(Page& page, GeolocationClient& client)
    : m_page(page)
    , m_client(client)
{
    m_page.addViewStateChangeObserver(*this);
}

GeolocationController::~GeolocationController()
{
    ASSERT(m_observers.isEmpty());

    // NOTE: We don't have to remove ourselves from page's ViewStateChangeObserver set, since
    // we are supplement of the Page, and our destructor getting called means the page is being
    // torn down.

    m_client.geolocationDestroyed();
}

void GeolocationController::addObserver(Geolocation* observer, bool enableHighAccuracy)
{
    // This may be called multiple times with the same observer, though removeObserver()
    // is called only once with each.
    bool wasEmpty = m_observers.isEmpty();
    m_observers.add(observer);
    if (enableHighAccuracy)
        m_highAccuracyObservers.add(observer);

    if (enableHighAccuracy)
        m_client.setEnableHighAccuracy(true);
    if (wasEmpty)
        m_client.startUpdating();
}

void GeolocationController::removeObserver(Geolocation* observer)
{
    if (!m_observers.contains(observer))
        return;

    m_observers.remove(observer);
    m_highAccuracyObservers.remove(observer);

    if (m_observers.isEmpty())
        m_client.stopUpdating();
    else if (m_highAccuracyObservers.isEmpty())
        m_client.setEnableHighAccuracy(false);
}

void GeolocationController::requestPermission(Geolocation* geolocation)
{
    if (!m_page.isVisible()) {
        m_pendedPermissionRequest.add(geolocation);
        return;
    }

    m_client.requestPermission(geolocation);
}

void GeolocationController::cancelPermissionRequest(Geolocation* geolocation)
{
    if (m_pendedPermissionRequest.remove(geolocation))
        return;

    m_client.cancelPermissionRequest(geolocation);
}

void GeolocationController::positionChanged(GeolocationPosition* position)
{
    m_lastPosition = position;
    Vector<RefPtr<Geolocation>> observersVector;
    copyToVector(m_observers, observersVector);
    for (auto& observer : observersVector)
        observer->positionChanged();
}

void GeolocationController::errorOccurred(GeolocationError* error)
{
    Vector<RefPtr<Geolocation>> observersVector;
    copyToVector(m_observers, observersVector);
    for (auto& observer : observersVector)
        observer->setError(error);
}

GeolocationPosition* GeolocationController::lastPosition()
{
    if (m_lastPosition.get())
        return m_lastPosition.get();

    return m_client.lastPosition();
}

void GeolocationController::viewStateDidChange(ViewState::Flags, ViewState::Flags)
{
    if (!m_page.isVisible())
        return;

    HashSet<RefPtr<Geolocation>> pendedPermissionRequests = WTF::move(m_pendedPermissionRequest);
    for (auto& permissionRequest : pendedPermissionRequests)
        m_client.requestPermission(permissionRequest.get());
}

const char* GeolocationController::supplementName()
{
    return "GeolocationController";
}

void provideGeolocationTo(Page* page, GeolocationClient* client)
{
    ASSERT(page);
    ASSERT(client);
    Supplement<Page>::provideTo(page, GeolocationController::supplementName(), std::make_unique<GeolocationController>(*page, *client));
}
    
} // namespace WebCore

#endif // ENABLE(GEOLOCATION)

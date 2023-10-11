/*
 * Copyright 2010, The Android Open Source Project
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DeviceOrientationController.h"

#include "DeviceOrientationClient.h"
#include "DeviceOrientationData.h"
#include "DeviceOrientationEvent.h"
#include "Page.h"

namespace WebCore {

DeviceOrientationController::DeviceOrientationController(DeviceOrientationClient* client)
    : DeviceController(client)
{
#if PLATFORM(IOS)
    // FIXME: Temporarily avoid asserting while OpenSource is using a different design.
    // We should reconcile the differences between OpenSource and iOS so that we can
    // remove this code path.
    if (m_client)
        deviceOrientationClient()->setController(this);
#else
    ASSERT(m_client);
    deviceOrientationClient()->setController(this);
#endif
}

void DeviceOrientationController::didChangeDeviceOrientation(DeviceOrientationData* orientation)
{
    dispatchDeviceEvent(DeviceOrientationEvent::create(eventNames().deviceorientationEvent, orientation));
}

DeviceOrientationClient* DeviceOrientationController::deviceOrientationClient()
{
    return static_cast<DeviceOrientationClient*>(m_client);
}

#if PLATFORM(IOS)
// FIXME: We should look to reconcile the iOS and OpenSource differences with this class
// so that we can either remove these methods or remove the PLATFORM(IOS)-guard.
void DeviceOrientationController::suspendUpdates()
{
    if (m_client)
        m_client->stopUpdating();
}

void DeviceOrientationController::resumeUpdates()
{
    if (m_client && !m_listeners.isEmpty())
        m_client->startUpdating();
}
#else
bool DeviceOrientationController::hasLastData()
{
    return deviceOrientationClient()->lastOrientation();
}

PassRefPtr<Event> DeviceOrientationController::getLastEvent()
{
    return DeviceOrientationEvent::create(eventNames().deviceorientationEvent, deviceOrientationClient()->lastOrientation());
}
#endif // PLATFORM(IOS)

const char* DeviceOrientationController::supplementName()
{
    return "DeviceOrientationController";
}

DeviceOrientationController* DeviceOrientationController::from(Page* page)
{
    return static_cast<DeviceOrientationController*>(Supplement<Page>::from(page, supplementName()));
}

bool DeviceOrientationController::isActiveAt(Page* page)
{
    if (DeviceOrientationController* self = DeviceOrientationController::from(page))
        return self->isActive();
    return false;
}

void provideDeviceOrientationTo(Page* page, DeviceOrientationClient* client)
{
    DeviceOrientationController::provideTo(page, DeviceOrientationController::supplementName(), std::make_unique<DeviceOrientationController>(client));
}

} // namespace WebCore

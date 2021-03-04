/*
 * Copyright 2010, The Android Open Source Project
 * Copyright (c) 2011, 2012 ACCESS CO., LTD. All rights reserved.
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

#ifndef WKCDeviceOrientationClient_h
#define WKCDeviceOrientationClient_h

#include <wkc/wkcbase.h>

namespace WKC {

class DeviceOrientation;
class DeviceOrientationController;

/*@{*/

/** @brief Class that notifies of events of device orientation. */
class WKC_API DeviceOrientationClientIf {
public:
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Sets controller.
       @endcond
    */
    virtual void setController(DeviceOrientationController*) = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies to start updating.
       @endcond
    */
    virtual void startUpdating() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies to stop updating.
       @endcond
    */
    virtual void stopUpdating() = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Returns last device orientation data.
       @return Device orientation data
       @endcond
    */
    virtual DeviceOrientation* lastOrientation() const = 0;
    /**
       @cond WKC_PRIVATE_DOCUMENT
       @brief Notifies controller is destroyed.
       @endcond
    */
    virtual void deviceOrientationControllerDestroyed() = 0;
};

/*@}*/

} // namespace

#endif // WKCDeviceOrientationClient_h

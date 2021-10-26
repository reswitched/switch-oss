/*
 *  Copyright (c) 2011, 2014 ACCESS CO., LTD. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 * 
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 * 
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#ifndef DeviceMotionClientWKC_h
#define DeviceMotionClientWKC_h

#include "DeviceMotionClient.h"

namespace WebCore {
class DeviceMotionController;
class DeviceMotionData;
}

namespace WKC {
class DeviceMotionClientIf;
class WKCWebViewPrivate;
class DeviceMotionControllerPrivate;
class DeviceMotionData;

class DeviceMotionClientWKC : public WebCore::DeviceMotionClient {
public:
    static DeviceMotionClientWKC* create(WKCWebViewPrivate*);
    virtual ~DeviceMotionClientWKC();

    virtual void setController(WebCore::DeviceMotionController*);
    virtual void startUpdating();
    virtual void stopUpdating();
    virtual WebCore::DeviceMotionData* lastMotion() const;
    virtual void deviceMotionControllerDestroyed();

private:
    DeviceMotionClientWKC(WKCWebViewPrivate* webView);
    bool construct();

private:
    WKCWebViewPrivate* m_view;
    DeviceMotionClientIf* m_appClient;
    DeviceMotionControllerPrivate* m_controller;
    mutable DeviceMotionData* m_motion;
};

} // namespece

#endif // DeviceMotionClientWKC_h

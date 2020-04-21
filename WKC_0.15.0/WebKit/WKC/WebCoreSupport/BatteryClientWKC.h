/*
 *  Copyright (c) 2014 ACCESS CO., LTD. All rights reserved.
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

#ifndef BatteryClientWKC_h
#define BatteryClientWKC_h

#include "BatteryClient.h"

#if ENABLE(BATTERY_STATUS)

namespace WebCore {
class BatteryController;
} // namespace

namespace WKC {
class BatteryClientIf;
class WKCWebViewPrivate;
class BatteryControllerPrivate;

class BatteryClientWKC : public WebCore::BatteryClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static BatteryClientWKC* create(WKCWebViewPrivate*);
    virtual ~BatteryClientWKC();

    virtual void setController(WebCore::BatteryController*);
    virtual void startUpdating();
    virtual void stopUpdating();
    virtual void batteryControllerDestroyed();

private:
    BatteryClientWKC(WKCWebViewPrivate*);
    bool construct();

private:
    WKCWebViewPrivate* m_view;
    BatteryClientIf* m_appClient;
    BatteryControllerPrivate* m_controller;
};

} // namespace

#endif

#endif // BatteryClientWKC_h

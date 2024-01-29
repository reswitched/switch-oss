/*
 *  Copyright (c) 2016-2019 ACCESS CO., LTD. All rights reserved.
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

#ifndef VibrationClientWKC_h
#define VibrationClientWKC_h

#include "VibrationClient.h"

namespace WKC {

class WKCWebViewPrivate;
class VibrationClientIf;

class VibrationClientWKC : public WebCore::VibrationClient {
public:
    static VibrationClientWKC* create(WKCWebViewPrivate* view);
    virtual ~VibrationClientWKC() override;

    virtual void vibrate(const unsigned& time) override;

    virtual void cancelVibration() override;

    virtual void vibrationEnd() override;

    virtual void vibrationDestroyed() override;

private:
    VibrationClientWKC(WKCWebViewPrivate* view);
    bool construct();

private:
    WKCWebViewPrivate* m_view;
    WKC::VibrationClientIf* m_appClient;
};
} // namespace

#endif // VibrationClientWKC_h

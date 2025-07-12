/*
 *  NavigatorWebNfc.cpp
 *
 *  Copyright(c) 2015 ACCESS CO., LTD. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"
#include "NavigatorWebNfc.h"

#if ENABLE(WKC_WEB_NFC)

#include "Navigator.h"
#include "WebNfc.h"

namespace WebCore {

NavigatorWebNfc::NavigatorWebNfc()
{
}

NavigatorWebNfc::~NavigatorWebNfc()
{
}

NavigatorWebNfc* NavigatorWebNfc::from(Navigator* navigator)
{
    NavigatorWebNfc* supplement = static_cast<NavigatorWebNfc*>(Supplement<Navigator>::from(navigator, supplementName()));
    if (!supplement) {
        auto newSupplement = std::make_unique<NavigatorWebNfc>();
        supplement = newSupplement.get();
        provideTo(navigator, supplementName(), WTF::move(newSupplement));
    }
    return supplement;
}

WebNfc* NavigatorWebNfc::nfc(Navigator* navigator)
{
    if (!navigator->frame()) {
        return 0;
    }

    NavigatorWebNfc* navigatorWebNfc = from(navigator);
    if (!navigatorWebNfc->m_nfc) {
        navigatorWebNfc->m_nfc = WebNfc::create(navigator);
    }
    return navigatorWebNfc->m_nfc.get();
}

const char* NavigatorWebNfc::supplementName()
{
    return "NavigatorWebNfc";
}

} // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)

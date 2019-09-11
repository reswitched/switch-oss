/*
 *  WebNfc.cpp
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
#include "WebNfc.h"

#if ENABLE(WKC_WEB_NFC)

#include "Navigator.h"
#include "Frame.h"
#include "WebNfcController.h"
#include "JSDOMPromise.h"

namespace WebCore {

WebNfc::WebNfc(Navigator* navigator)
{
    ASSERT(navigator);
    ASSERT(navigator->frame());
    ASSERT(navigator->frame()->page());
    ASSERT(navigator->frame()->document());

    m_webNfcController = WebNfcController::from(navigator->frame()->page());
    m_webNfcController->setDocument(navigator->frame()->document());
}

WebNfc::~WebNfc()
{
}

Ref<WebNfc> WebNfc::create(Navigator* navigator)
{
    return adoptRef(*new WebNfc(navigator));
}

void WebNfc::requestAdapter(DeferredWrapper* deferredWrapper)
{
    m_webNfcController->requestAdapter(deferredWrapper);
}

} // namespace WebCore

#endif // ENABLE(WKC_WEB_NFC)

/*
 * Copyright (c) 2015-2016 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(WKC_WEB_NFC)

#include "WKCEnums.h"

#include "helpers/WKCWebNfcController.h"
#include "helpers/privates/WKCWebNfcControllerPrivate.h"

#include "WebNfcController.h"

#include "helpers/privates/WKCWebNfcMessagePrivate.h"

namespace WKC {

// Private Implementation

WebNfcControllerPrivate::WebNfcControllerPrivate(WebCore::WebNfcController* parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_message(0)
{
}

WebNfcControllerPrivate::~WebNfcControllerPrivate()
{
    WebNfcMessage::destroy(m_message);
}

void
WebNfcControllerPrivate::updateAdapter(int id, int result)
{
    m_webcore->updateAdapter(id, result);
}

void
WebNfcControllerPrivate::updateMessage(WebNfcMessage* message)
{
    if (!message) {
        return;
    }
    WebNfcMessage::destroy(m_message);
    m_message = message;
    m_webcore->updateMessage(message->priv()->webcore().get());
}

void
WebNfcControllerPrivate::notifySendResult(int id, int result)
{
    m_webcore->notifySendResult(id, result);
}


// Implementation

WebNfcController::WebNfcController(WebNfcControllerPrivate& parent)
    : m_private(parent)
{
}

WebNfcController::~WebNfcController()
{
}

void
WebNfcController::updateAdapter(int id, int result)
{
    m_private.updateAdapter(id, result);
}

void
WebNfcController::updateMessage(WebNfcMessage* message)
{
    m_private.updateMessage(message);
}

void
WebNfcController::notifySendResult(int id, int result)
{
    m_private.notifySendResult(id, result);
}

} // namespace WKC

#endif // ENABLE(WKC_WEB_NFC)

/*
 *  Copyright (c) 2018-2019 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"

#include "NetworkStorageSession.h"

#include "FileStream.h"

namespace WebCore {

using namespace PAL;

NetworkStorageSession::NetworkStorageSession(SessionID sessionID)
    : m_sessionID(sessionID)
{
}

NetworkStorageSession::~NetworkStorageSession()
{
}

void NetworkStorageSession::ensureSession(SessionID, const String&)
{
    ASSERT_NOT_REACHED();
}

static std::unique_ptr<NetworkStorageSession>& defaultSession()
{
    static NeverDestroyed<std::unique_ptr<NetworkStorageSession>> session;
    if (session.isNull())
        session.construct();
    return session;
}

NetworkStorageSession& NetworkStorageSession::defaultStorageSession()
{
    if (!defaultSession())
        defaultSession() = std::make_unique<NetworkStorageSession>(SessionID::defaultSessionID());
    return *defaultSession();
}

void NetworkStorageSession::switchToNewTestingSession()
{
}

}

/*
 * Copyright (c) 2011 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCAuthenticationChallenge.h"
#include "helpers/privates/WKCAuthenticationChallengePrivate.h"
#include "helpers/WKCProtectionSpace.h"

#include "AuthenticationChallenge.h"

namespace WKC {

AuthenticationChallengePrivate::AuthenticationChallengePrivate(const WebCore::AuthenticationChallenge& parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_protectionSpace(parent.protectionSpace())
    , m_sourceHandle(parent.sourceHandle())
{
}

AuthenticationChallengePrivate::~AuthenticationChallengePrivate()
{
}

////////////////////////////////////////////////////////////////////////////////

AuthenticationChallenge::AuthenticationChallenge(AuthenticationChallengePrivate& parent)
    : m_private(parent)
{
}

AuthenticationChallenge::~AuthenticationChallenge()
{
}

const ProtectionSpace&
AuthenticationChallenge::protectionSpace() const
{
    return m_private.protectionSpace().wkc();
}

ResourceHandle*
AuthenticationChallenge::sourceHandle() const
{
    return &m_private.sourceHandle().wkc();
}

} // namespace


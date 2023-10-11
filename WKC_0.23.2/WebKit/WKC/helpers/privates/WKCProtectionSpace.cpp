/*
 * Copyright (c) 2011-2016 ACCESS CO., LTD. All rights reserved.
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

#include "helpers/WKCProtectionSpace.h"
#include "helpers/privates/WKCProtectionSpacePrivate.h"

#include "ProtectionSpace.h"

#include "WTFString.h"
#include "helpers/WKCString.h"

#include "helpers/privates/WKCHelpersEnumsPrivate.h"

namespace WKC {

ProtectionSpacePrivate::ProtectionSpacePrivate(const WebCore::ProtectionSpace& parent)
    : m_webcore(parent)
    , m_wkc(*this)
    , m_host(m_webcore.host())
    , m_realm(m_webcore.realm())
{
}

ProtectionSpacePrivate::~ProtectionSpacePrivate()
{
}

bool
ProtectionSpacePrivate::isProxy() const
{
    return m_webcore.isProxy();
}

ProtectionSpaceAuthenticationScheme
ProtectionSpacePrivate::authenticationScheme() const
{
    return toWKCProtectionSpaceAuthenticationScheme(m_webcore.authenticationScheme());
}

ProtectionSpaceServerType
ProtectionSpacePrivate::serverType() const
{
    return toWKCProtectionSpaceServerType(m_webcore.serverType());
}

////////////////////////////////////////////////////////////////////////////////

ProtectionSpace::ProtectionSpace(ProtectionSpacePrivate& parent)
    : m_private(parent)
{
}

ProtectionSpace::~ProtectionSpace()
{
}


const String&
ProtectionSpace::host() const
{
    return m_private.host();
}

const String&
ProtectionSpace::realm() const
{
    return m_private.realm();
}

bool
ProtectionSpace::isProxy() const
{
    return m_private.isProxy();
}

ProtectionSpaceAuthenticationScheme
ProtectionSpace::authenticationScheme() const
{
    return m_private.authenticationScheme();
}

ProtectionSpaceServerType
ProtectionSpace::serverType() const
{
    return m_private.serverType();
}

} // namespace

/*
 * Copyright (c) 2011-2020 ACCESS CO., LTD. All rights reserved.
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


#ifndef _WKC_HELPERS_PRIVATE_PROTECTIONSPACEPRIVATE_H_
#define _WKC_HELPERS_PRIVATE_PROTECTIONSPACEPRIVATE_H_

#include "helpers/WKCProtectionSpace.h"
#include "helpers/WKCString.h"

namespace WebCore { class ProtectionSpace; }

namespace WKC {

class ProtectinoSpace;

class ProtectionSpaceWrap : public ProtectionSpace {
WTF_MAKE_FAST_ALLOCATED;
public:
    ProtectionSpaceWrap(ProtectionSpacePrivate& parent) : ProtectionSpace(parent) {}
    ~ProtectionSpaceWrap() {}
};

class ProtectionSpacePrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    ProtectionSpacePrivate(const WebCore::ProtectionSpace&);
    ~ProtectionSpacePrivate();

    const WebCore::ProtectionSpace& webcore() const { return m_webcore; }
    const ProtectionSpace& wkc() const { return m_wkc; }

    const String& host() const { return m_host; }
    int port() const;
    const String& realm() const { return m_realm; }
    bool isProxy() const;

    ProtectionSpaceAuthenticationScheme authenticationScheme() const;
    ProtectionSpaceServerType serverType() const;

private:
    const WebCore::ProtectionSpace& m_webcore;
    ProtectionSpaceWrap m_wkc;

    String m_host;
    String m_realm;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_PROTECTIONSPACEPRIVATE_H_

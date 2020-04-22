/*
 * Copyright (c) 2011-2015 ACCESS CO., LTD. All rights reserved.
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


#ifndef _WKC_HELPERS_PRIVATE_AUTHENTICATIONCHALLENGE_H_
#define _WKC_HELPERS_PRIVATE_AUTHENTICATIONCHALLENGE_H_

#include "helpers/WKCAuthenticationChallenge.h"

#include "WKCProtectionSpacePrivate.h"
#include "WKCResourceHandlePrivate.h"

namespace WebCore {
class AuthenticationChallenge;
}

namespace WKC {
class AuthenticationChallenge;
class ResourceHandlePrivate;

class AuthenticationChallengeWrap : public AuthenticationChallenge {
WTF_MAKE_FAST_ALLOCATED;
public:
    AuthenticationChallengeWrap(AuthenticationChallengePrivate& parent) : AuthenticationChallenge(parent) {}
    ~AuthenticationChallengeWrap() {}
};

class AuthenticationChallengePrivate
{
WTF_MAKE_FAST_ALLOCATED;
public:
    AuthenticationChallengePrivate(const WebCore::AuthenticationChallenge& parent);
    ~AuthenticationChallengePrivate();

    const WebCore::AuthenticationChallenge& webcore() const { return m_webcore; }
    const AuthenticationChallenge& wkc() const { return m_wkc; }

    const ProtectionSpacePrivate& protectionSpace() const { return m_protectionSpace; }
    ResourceHandlePrivate& sourceHandle() { return m_sourceHandle; }

private:
    const WebCore::AuthenticationChallenge& m_webcore;
    AuthenticationChallengeWrap m_wkc;

    ProtectionSpacePrivate m_protectionSpace;
    ResourceHandlePrivate m_sourceHandle;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_AUTHENTICATIONCHALLENGE_H_

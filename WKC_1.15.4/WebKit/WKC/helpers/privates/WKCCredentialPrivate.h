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


#ifndef _WKC_PRIVATE_PRIVATE_CREDENTIAL_H_
#define _WKC_PRIVATE_PRIVATE_CREDENTIAL_H_

#include "Credential.h"
#include "helpers/WKCCredential.h"

namespace WebCore {
class Credential;
} // namespace

namespace WKC {
class Credential;

class CredentialPrivate {
WTF_MAKE_FAST_ALLOCATED;
public:
    CredentialPrivate(const Credential&);
    CredentialPrivate(const Credential&, const String& user, const String& password, CredentialPersistence pers);
    ~CredentialPrivate();

    CredentialPrivate(const CredentialPrivate&);

    const WebCore::Credential& webcore() const { return m_webcore; }
    const Credential& wkc() const { return m_wkc; }

private:
    const Credential& m_wkc;
    WebCore::Credential m_webcore;
};
} // namespace

#endif // _WKC_PRIVATE_PRIVATE_CREDENTIAL_H_


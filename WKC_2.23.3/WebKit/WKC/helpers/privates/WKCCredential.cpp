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

#include "helpers/WKCCredential.h"
#include "helpers/privates/WKCCredentialPrivate.h"

#include "Credential.h"
#include "WTFString.h"

#include "helpers/WKCString.h"

#include "helpers/privates/WKCHelpersEnumsPrivate.h"

namespace WKC {
CredentialPrivate::CredentialPrivate(const Credential& cred)
    : m_wkc(cred)
    , m_webcore()
{
}

CredentialPrivate::CredentialPrivate(const Credential& cred, const String& user, const String& password, CredentialPersistence pers)
    : m_wkc(cred)
    , m_webcore(user, password, toWebCoreCredentialPersistence(pers))
{
}

CredentialPrivate::~CredentialPrivate()
{
}

CredentialPrivate::CredentialPrivate(const CredentialPrivate& other)
    : m_webcore(other.m_webcore, other.m_webcore.persistence())
    , m_wkc(other.m_wkc)
{
}

////////////////////////////////////////////////////////////////////////////////

Credential::Credential()
    : m_private(new CredentialPrivate(*this))
{
}

Credential::Credential(const String& user, const String& password, CredentialPersistence pers)
    : m_private(new CredentialPrivate(*this, user, password, pers))
{
}

Credential::~Credential()
{
    delete m_private;
}

Credential::Credential(const Credential& other)
    : m_private(new CredentialPrivate(*(other.m_private)))
{
}

Credential&
Credential::operator=(const Credential& other)
{
    if (this != &other) {
        delete m_private;
        m_private = new CredentialPrivate(*(other.m_private));
    }
    return *this;
}

} // namespace

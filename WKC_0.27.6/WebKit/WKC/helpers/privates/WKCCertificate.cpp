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

#include "helpers/WKCCertificate.h"
#include "helpers/privates/WKCCertificatePrivate.h"

#include "CertificateWKC.h"
#include "WTFString.h"

#include "helpers/WKCString.h"

namespace WKC {

ClientCertificatePrivate::ClientCertificatePrivate(WebCore::ClientCertificate** parent)
    : m_webcore(parent)
    , m_wkc(*this)
{
}

ClientCertificatePrivate::~ClientCertificatePrivate()
{
}

const WKC::String
ClientCertificatePrivate::issuer(int i)
{
    return WKC::String(m_webcore[i]->issuer());
}

const WKC::String
ClientCertificatePrivate::subject(int i)
{
    return WKC::String(m_webcore[i]->subject());
}

const WKC::String
ClientCertificatePrivate::notbefore(int i)
{
    return WKC::String(m_webcore[i]->notbefore());
}

const WKC::String
ClientCertificatePrivate::notafter(int i)
{
    return WKC::String(m_webcore[i]->notafter());
}

const WKC::String
ClientCertificatePrivate::serialnumber(int i)
{
    return WKC::String(m_webcore[i]->serialnumber());
}

////////////////////////////////////////////////////////////////////////////////

ClientCertificate::ClientCertificate(ClientCertificatePrivate& parent)
    : m_private(parent)
{
}

ClientCertificate::~ClientCertificate()
{
}

const WKC::String
ClientCertificate::issuer(int i)
{
    return m_private.issuer(i);
}

const WKC::String
ClientCertificate::subject(int i)
{
    return m_private.subject(i);
}

const WKC::String
ClientCertificate::notbefore(int i)
{
    return m_private.notbefore(i);
}

const WKC::String
ClientCertificate::notafter(int i)
{
    return m_private.notafter(i);
}

const WKC::String
ClientCertificate::serialnumber(int i)
{
    return m_private.serialnumber(i);
}

} // namespace

/*
 *  Copyright (c) 2011-2014 ACCESS CO., LTD. All rights reserved.
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

#ifndef _WKC_HELPERS_PRIVATE_CERTIFICATE_H_
#define _WKC_HELPERS_PRIVATE_CERTIFICATE_H_

#include "helpers/WKCCertificate.h"
#include "helpers/WKCString.h"

namespace WebCore {
class ClientCertificate;
} // namespace

namespace WKC {

class String;

class ClientCertificateWrap : public ClientCertificate {
public:
    ClientCertificateWrap(ClientCertificatePrivate& parent) : ClientCertificate(parent) {}
    ~ClientCertificateWrap() {}
};

class ClientCertificatePrivate {
public:
    ClientCertificatePrivate(WebCore::ClientCertificate**);
    ~ClientCertificatePrivate();

    WebCore::ClientCertificate** webcore() const { return m_webcore; }
    ClientCertificate& wkc() { return m_wkc; }

    const WKC::String issuer(int i);
    const WKC::String subject(int i);
    const WKC::String notbefore(int i);
    const WKC::String notafter(int i);
    const WKC::String serialnumber(int i);

private:
    WebCore::ClientCertificate** m_webcore;
    ClientCertificateWrap m_wkc;
};

} // namespace

#endif // _WKC_HELPERS_PRIVATE_CERTIFICATE_H_

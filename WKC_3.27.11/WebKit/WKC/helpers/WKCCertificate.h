/*
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef _WKC_HELPERS_WKC_CERTIFICATE_H_
#define _WKC_HELPERS_WKC_CERTIFICATE_H_

#include <wkc/wkcbase.h>

namespace WKC {
class String;
class ClientCertificatePrivate;

class WKC_API ClientCertificate {
public:
    ClientCertificatePrivate& priv() const { return m_private; }

    const String issuer(int i);
    const String subject(int i);
    const String notbefore(int i);
    const String notafter(int i);
    const String serialnumber(int i);

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    ClientCertificate(ClientCertificatePrivate&);
    ~ClientCertificate();

private:
    ClientCertificate(const ClientCertificate&);
    ClientCertificate& operator=(const ClientCertificate&);

    ClientCertificatePrivate& m_private;
};

} // namespace

#endif // _WKC_HELPERS_WKC_CERTIFICATE_H_

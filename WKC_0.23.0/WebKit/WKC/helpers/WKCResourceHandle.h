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

#ifndef _WKC_HELPERS_WKC_RESOURCEHANDLE_H_
#define _WKC_HELPERS_WKC_RESOURCEHANDLE_H_

#include <wkc/wkcbase.h>

namespace WKC {
class Credential;
class AuthenticationChallenge;
class ResourceHandlePrivate;
class ResourceLoader;

class WKC_API ResourceHandle {
public:
    void receivedCredential(const AuthenticationChallenge&, const Credential&);
    void receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&);

    long SSLVerifyOpenSSLResult() const;
    long SSLVerifycURLResult() const;
    const char* url() const;
    const char* urlhost() const;
    int  contentComposition() const;
    void* httphandle();

    bool isSSL() const;
    bool isEVSSL() const;
    int  secureState() const;
    int  secureLevel() const;

    long long expectedContentLength() const;

    ResourceHandlePrivate& priv() const { return m_private; }

protected:
    // Applications must not create/destroy WKC helper instances by new/delete.
    // Or, it causes memory leaks or crashes.
    ResourceHandle(ResourceHandlePrivate&);
    ~ResourceHandle();

private:
    ResourceHandle(const ResourceHandle&);
    ResourceHandle& operator=(const ResourceHandle&);

    ResourceHandlePrivate& m_private;
};
} // namespace

#endif // _WKC_HELPERS_WKC_RESOURCEHANDLE_H_

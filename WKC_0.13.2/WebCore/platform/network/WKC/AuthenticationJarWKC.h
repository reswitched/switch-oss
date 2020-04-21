/*
 * Copyright (c) 2012,2013 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ACCESS CO.,LTD. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ACCESS CO.,LTD. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
#ifndef AuthenticationJar_h
#define AuthenticationJar_h

#include "CString.h"
#include "URL.h"
#include <wtf/RefPtr.h>

#include "ResourceHandle.h"
#include "ResourceHandleInternalWKC.h"

namespace WebCore {

class AuthenticationJar {
WTF_MAKE_FAST_ALLOCATED;
public:
    AuthenticationJar();
    ~AuthenticationJar();

    AuthenticationJar* get() { return this; }

    bool Challenge(ResourceHandle *job, long httpCode);

    bool getWebUserPassword(String url, ProtectionSpaceServerType& servertype, ProtectionSpaceAuthenticationScheme& authscheme, String realm, String& user, String& passwd);
    void setWebUserPassword(String url, ProtectionSpaceServerType servertype, ProtectionSpaceAuthenticationScheme authscheme, String realm, String user, String passwd, String location = "", bool confirmed = false);
    void deleteWebUserPassword(String url, String realm);

    bool getProxyUserPassword(String url, int port, ProtectionSpaceServerType& servertype, ProtectionSpaceAuthenticationScheme& authscheme, String& user, String& passwd);
    void setProxyUserPassword(String url, int port, ProtectionSpaceServerType servertype, ProtectionSpaceAuthenticationScheme authscheme, String user, String passwd);
    void deleteProxyUserPassword(String url, int port);

private:
    class AuthInfo {
    WTF_MAKE_FAST_ALLOCATED;
    public:
        AuthInfo()
            : m_serverType(ProtectionSpaceServerHTTP)
            , m_authScheme(ProtectionSpaceAuthenticationSchemeDefault)
            , m_host("")
            , m_port(0)
            , m_basePath("")
            , m_fullPath("")
            , m_realm("")
            , m_confirmed(false)
            , m_user("")
            , m_passwd("")
            , m_tmpUser("")
            , m_tmpPasswd("")
        {};
        ~AuthInfo() {};

        ProtectionSpaceServerType           m_serverType;
        ProtectionSpaceAuthenticationScheme m_authScheme;
        String m_host;
        unsigned short m_port;
        String m_basePath;
        String m_fullPath;
        String m_realm;

        bool   m_confirmed;
        String m_user;
        String m_passwd;
        String m_tmpUser;
        String m_tmpPasswd;
    };

    Vector<AuthInfo*> m_WebAuthInfoList;
    Vector<AuthInfo*> m_ProxyAuthInfoList;

    AuthInfo* longer(AuthInfo* current, AuthInfo* newinfo, bool compFull);
};

}

#endif // AuthenticationJar_h

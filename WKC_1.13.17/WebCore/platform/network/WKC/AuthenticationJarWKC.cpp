/*
 * Copyright (c) 2012-2019 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"

#include "AuthenticationJarWKC.h"
#include "ResourceHandleManagerWKC.h"

#include <wkc/wkcpeer.h>

// debug print
#define NO_NXLOG
#include "utils/nxDebugPrint.h"

namespace WebCore {

AuthenticationJar::AuthenticationJar()
    : m_WebAuthInfoList(0)
    , m_ProxyAuthInfoList(0)
{
}

AuthenticationJar::~AuthenticationJar()
{
    AuthInfo *info;
    int size;

    size = m_WebAuthInfoList.size();
    while (0 < size) {
        info = m_WebAuthInfoList[size - 1];
        m_WebAuthInfoList.remove(size - 1);
        delete info;
        size--;
    }

    size = m_ProxyAuthInfoList.size();
    while (0 < size) {
        info = m_ProxyAuthInfoList[size - 1];
        m_ProxyAuthInfoList.remove(size - 1);
        delete info;
        size--;
    }
}

AuthenticationJar::AuthInfo* AuthenticationJar::longer(AuthInfo* current, AuthInfo* newinfo, bool compFull)
{
    if (!current)
        return newinfo;

    if (compFull) {
        if (current->m_fullPath.length() < newinfo->m_fullPath.length())
            return newinfo;
    }
    else {
        if (current->m_basePath.length() < newinfo->m_basePath.length())
            return newinfo;
    }
    return current;
}

static void sanitizeURL(URL &url, unsigned short &port)
{
    url.setUser("");
    url.setPass("");
    url.setQuery("");
    url.removeFragmentIdentifier();
    port = (url.port()) ? url.port().value() : (url.protocolIs("http") ? 80 : 443);
    url.removePort();
}

static String basePath(String path)
{
    return path.left(path.reverseFind('/') + 1);
}

bool AuthenticationJar::getWebUserPassword(String url, ProtectionSpaceServerType& servertype, ProtectionSpaceAuthenticationScheme& authscheme, String realm, String& user, String& passwd)
{
    int size = m_WebAuthInfoList.size();
    if (0 == size)
        return false;

    unsigned short url_port;
    URL kurl(ParsedURLString, url);
    sanitizeURL(kurl, url_port);

    String url_host      = kurl.host().toString();
    String url_full_path = kurl.path().length() ? kurl.path() : "/";
    String url_base_path = basePath(url_full_path);
    if (!url_full_path.endsWith("/")) {
        url_full_path = url_full_path + "/";
    }

    AuthInfo* info;
    AuthInfo* longest = NULL;
    AuthInfo* fullpath_longest = NULL;
    AuthInfo* basepath_longest = NULL;
    for (int i = 0; i < size; i++) {
        info = m_WebAuthInfoList[i];

        // host compare
        if (info->m_host != url_host) continue;

        // realm compare
        if (!realm.isEmpty() && info->m_realm != realm) continue;

        // path compare
        // full path match
        if (url_full_path == info->m_fullPath) {
            servertype = info->m_serverType;
            authscheme = info->m_authScheme;
            user       = (info->m_confirmed) ? info->m_user   : info->m_tmpUser;
            passwd     = (info->m_confirmed) ? info->m_passwd : info->m_tmpPasswd; 
            return true;
        }
        // url_full_path include info->m_fullPath or info->m_basePath
        if (url_full_path.startsWith(info->m_fullPath)) {
            fullpath_longest = longer(fullpath_longest, info, true);
        }
        else if (url_full_path.startsWith(info->m_basePath)) {
            basepath_longest = longer(basepath_longest, info, false);
        }
    }

    longest = fullpath_longest ? fullpath_longest : basepath_longest;
    if (longest) {
        // found
        servertype = longest->m_serverType;
        authscheme = longest->m_authScheme;
        user       = (longest->m_confirmed) ? longest->m_user   : longest->m_tmpUser;
        passwd     = (longest->m_confirmed) ? longest->m_passwd : longest->m_tmpPasswd;
        if (fullpath_longest) {
            // hear means longest->m_fullPath is folder
            if (fullpath_longest->m_fullPath != fullpath_longest->m_basePath)
                fullpath_longest->m_basePath = fullpath_longest->m_fullPath;
        }
        return true;
    }

    return false;
}

void AuthenticationJar::setWebUserPassword(String url, ProtectionSpaceServerType servertype, ProtectionSpaceAuthenticationScheme authscheme, String realm, String user, String passwd, String location, bool confirmed)
{
    unsigned short url_port;
    URL kurl = URL(ParsedURLString, url);
    sanitizeURL(kurl, url_port);

    String url_host      = kurl.host().toString();
    String url_full_path = kurl.path().length() ? kurl.path() : "/";
    String url_base_path = basePath(url_full_path);
    if (!url_full_path.endsWith("/")) {
        url_full_path = url_full_path + "/";
    }

    URL location_url;
    unsigned short location_port;
    bool FullBaseSame = false;
    if (!location.isEmpty()) {
        URL location_url =  URL(kurl, location);
        sanitizeURL(location_url, location_port);

        if (location_url.path().startsWith(url_full_path)) {
            // location_url starts with url_full_path which end with '/'.
            // this means url_full_path without end '/' was FOLDER.
            // for example url_path is /a/b, it add info fullpath is /a/b/ and basepath is /a/ for this path case.
            // but the url redirect to /a/b/xxx. This means /a/b/ is FOLDER.
            // so it will change basepath to /a/b/
            FullBaseSame = true;
        }
        confirmed = true;
    }

    AuthInfo *info;
    int size = m_WebAuthInfoList.size();
    if (ProtectionSpaceAuthenticationSchemeNTLM == authscheme) {
        // NTLM match
        for (int i = 0; i < size; i++) {
            info = m_WebAuthInfoList[i];
            if (info->m_authScheme != authscheme)  continue;
            if (info->m_serverType != servertype)  continue;
            if (info->m_host       != url_host)    continue;
            // found & update
            if (confirmed) {
                info->m_user   = user;
                info->m_passwd = passwd;
                info->m_confirmed = true;
                info->m_tmpUser   = "";
                info->m_tmpPasswd = "";
            }
            else {
                info->m_confirmed = false;
                info->m_tmpUser   = user;
                info->m_tmpPasswd = passwd;
            }
            return;
        }
    }
    else {
        // Basic or Digest match
        for (int i = 0; i < size; i++) {
            info = m_WebAuthInfoList[i];
            if (info->m_authScheme != authscheme) continue;
            if (info->m_serverType != servertype) continue;
            if (info->m_realm      != realm)      continue;
            if (info->m_host       != url_host)   continue;
            // path check
            if (url_full_path == info->m_fullPath) {
                if (FullBaseSame) {
                    info->m_basePath = info->m_fullPath;
                }
                if (confirmed) {
                    info->m_user   = user;
                    info->m_passwd = passwd;
                    info->m_confirmed = true;
                    info->m_tmpUser   = "";
                    info->m_tmpPasswd = "";
                }
                else {
                    info->m_confirmed = false;
                    info->m_tmpUser   = user;
                    info->m_tmpPasswd = passwd;
                }
                return;
            }
            if (url_base_path == info->m_basePath) {
                // in one folder, the folder has AN auth info like .htaccess of Apatch.
                // In one folder, files more than two do not assume that it has separate certification information.
                if (confirmed) {
                    info->m_user   = user;
                    info->m_passwd = passwd;
                    info->m_confirmed = true;
                    info->m_tmpUser   = "";
                    info->m_tmpPasswd = "";
                }
                else {
                    info->m_confirmed = false;
                    info->m_tmpUser   = user;
                    info->m_tmpPasswd = passwd;
                }
                return;
            }
        }
    }

    if (!location.isEmpty()) {
        // do not add new one when redirect
        return;
    }

    // not found then add

    info = new AuthInfo;
    if (!info)
        return;

    if (ProtectionSpaceAuthenticationSchemeNTLM == authscheme) {
        url_base_path = "/";
        url_full_path = "/";
    }
    info->m_serverType = servertype;
    info->m_authScheme = authscheme;
    info->m_host       = url_host;
    info->m_port       = url_port;
    info->m_basePath   = url_base_path;
    info->m_fullPath   = url_full_path;
    info->m_realm      = realm;
    if (confirmed) {
        info->m_user      = user;
        info->m_passwd    = passwd;
        info->m_confirmed = true;
        info->m_tmpUser   = "";
        info->m_tmpPasswd = "";
    }
    else {
        info->m_user      = "";
        info->m_passwd    = "";
        info->m_confirmed = false;
        info->m_tmpUser   = user;
        info->m_tmpPasswd = passwd;
    }

    m_WebAuthInfoList.append(info);
}

void AuthenticationJar::deleteWebUserPassword(String url, String realm)
{
    int size = m_WebAuthInfoList.size();
    if (0 == size)
        return;

    unsigned short url_port;
    URL kurl(ParsedURLString, url);
    sanitizeURL(kurl, url_port);

    String url_host      = kurl.host().toString();
    String url_full_path = kurl.path().length() ? kurl.path() : "/";
    String url_base_path = basePath(url_full_path);
    if (!url_full_path.endsWith("/")) {
        url_full_path = url_full_path + "/";
    }

    AuthInfo* info;
    for (int i = 0; i < size; i++) {
        info = m_WebAuthInfoList[i];

        // host compare
        if (url_host != info->m_host) continue;

        // realm compare
        if (!realm.isEmpty() && realm != info->m_realm) continue;

        // path compare
        // full path match
        if (url_full_path == info->m_fullPath && url_base_path == info->m_basePath) {
            m_WebAuthInfoList.remove(i);
            delete info;
            return;
        }
    }
}

bool AuthenticationJar::getProxyUserPassword(String url, int port, ProtectionSpaceServerType& servertype, ProtectionSpaceAuthenticationScheme& authscheme, String& user, String& passwd)
{
    int size = m_ProxyAuthInfoList.size();
    if (0 == size)
        return false;

    unsigned short url_port;
    URL kurl(ParsedURLString, url);
    sanitizeURL(kurl, url_port);
    String url_host = kurl.host().toString();

    AuthInfo *info;
    for (int i = 0; i < size; i++) {
        info = m_ProxyAuthInfoList[i];
        if (info->m_serverType < ProtectionSpaceProxyHTTP) continue;
        if (info->m_host != url_host) continue;
        if (info->m_port != port) continue;
        // found
        servertype = info->m_serverType;
        authscheme = info->m_authScheme;
        user       = info->m_user;
        passwd     = info->m_passwd;
        return true;
    }

    return false;
}

void AuthenticationJar::setProxyUserPassword(String url, int port, ProtectionSpaceServerType servertype, ProtectionSpaceAuthenticationScheme authscheme, String user, String passwd)
{
    unsigned short url_port;
    URL kurl(ParsedURLString, url);
    sanitizeURL(kurl, url_port);
    String url_path = kurl.path();
    String url_host = kurl.host().toString();

    AuthInfo *info;
    int size = m_ProxyAuthInfoList.size();
    for (int i = 0; i < size; i++) {
        info = m_ProxyAuthInfoList[i];
        if (info->m_serverType != servertype) continue;
        if (info->m_host != url_host) continue;
        if (info->m_port != port) continue;
        // found & update
        info->m_authScheme = authscheme;
        info->m_user   = user;
        info->m_passwd = passwd;
        return;
    }

    info = new AuthInfo;
    if (!info)
        return;

    info->m_serverType = servertype;
    info->m_authScheme = authscheme;
    info->m_host       = url_host;
    info->m_port       = port;
    info->m_basePath   = "";
    info->m_fullPath   = "";
    info->m_user       = user;
    info->m_passwd     = passwd;
    info->m_realm      = "";

    m_ProxyAuthInfoList.append(info);
}

void AuthenticationJar::deleteProxyUserPassword(String url, int port)
{
    int size = m_ProxyAuthInfoList.size();
    if (0 == size)
        return;

    unsigned short url_port;
    URL kurl(ParsedURLString, url);
    sanitizeURL(kurl, url_port);
    String url_host = kurl.host().toString();

    AuthInfo *info;
    for (int i = 0; i < size; i++) {
        info = m_ProxyAuthInfoList[i];
        if (info->m_serverType < ProtectionSpaceProxyHTTP) continue;
        if (info->m_host != url_host) continue;
        if (info->m_port != port) continue;
        // found

        m_ProxyAuthInfoList.remove(i);
        delete info;
        return;
    }
}





} // namespace

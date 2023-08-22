/*
 * Copyright (C) 2006, 2007, 2008, 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "PlatformCookieJar.h"

#if USE(CFNETWORK)

#include "CFNetworkSPI.h"
#include "Cookie.h"
#include "URL.h"
#include "NetworkStorageSession.h"
#include "SoftLinking.h"
#include <CFNetwork/CFHTTPCookiesPriv.h>
#include <CoreFoundation/CoreFoundation.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(WIN)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#include <windows.h>
#endif

#if PLATFORM(WIN) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED <= 1090) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED < 80000)
enum {
    CFHTTPCookieStorageAcceptPolicyExclusivelyFromMainDocumentDomain = 3
};
#endif

namespace WebCore {

static const CFStringRef s_setCookieKeyCF = CFSTR("Set-Cookie");
static const CFStringRef s_cookieCF = CFSTR("Cookie");

static inline RetainPtr<CFStringRef> cookieDomain(CFHTTPCookieRef cookie)
{
    return adoptCF(CFHTTPCookieCopyDomain(cookie));
}

static inline CFAbsoluteTime cookieExpirationTime(CFHTTPCookieRef cookie)
{
    return CFHTTPCookieGetExpirationTime(cookie);
}

static inline RetainPtr<CFStringRef> cookieName(CFHTTPCookieRef cookie)
{
    return adoptCF(CFHTTPCookieCopyName(cookie));
}

static inline RetainPtr<CFStringRef> cookiePath(CFHTTPCookieRef cookie)
{
    return adoptCF(CFHTTPCookieCopyPath(cookie));
}

static inline RetainPtr<CFStringRef> cookieValue(CFHTTPCookieRef cookie)
{
    return adoptCF(CFHTTPCookieCopyValue(cookie));
}

static RetainPtr<CFArrayRef> filterCookies(CFArrayRef unfilteredCookies)
{
    ASSERT(unfilteredCookies);
    CFIndex count = CFArrayGetCount(unfilteredCookies);
    RetainPtr<CFMutableArrayRef> filteredCookies = adoptCF(CFArrayCreateMutable(0, count, &kCFTypeArrayCallBacks));
    for (CFIndex i = 0; i < count; ++i) {
        CFHTTPCookieRef cookie = (CFHTTPCookieRef)CFArrayGetValueAtIndex(unfilteredCookies, i);

        // <rdar://problem/5632883> CFHTTPCookieStorage would store an empty cookie,
        // which would be sent as "Cookie: =". We have a workaround in setCookies() to prevent
        // that, but we also need to avoid sending cookies that were previously stored, and
        // there's no harm to doing this check because such a cookie is never valid.
        if (!CFStringGetLength(cookieName(cookie).get()))
            continue;

        if (CFHTTPCookieIsHTTPOnly(cookie))
            continue;

        CFArrayAppendValue(filteredCookies.get(), cookie);
    }
    return filteredCookies;
}

static RetainPtr<CFArrayRef> copyCookiesForURLWithFirstPartyURL(const NetworkStorageSession& session, const URL& firstParty, const URL& url)
{
    bool secure = url.protocolIs("https");

#if (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 80000) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000)
    return adoptCF(_CFHTTPCookieStorageCopyCookiesForURLWithMainDocumentURL(session.cookieStorage().get(), url.createCFURL().get(), firstParty.createCFURL().get(), secure));
#else
    // _CFHTTPCookieStorageCopyCookiesForURLWithMainDocumentURL is not available on other platforms.
    UNUSED_PARAM(firstParty);
    return adoptCF(CFHTTPCookieStorageCopyCookiesForURL(session.cookieStorage().get(), url.createCFURL().get(), secure));
#endif
}

static CFArrayRef createCookies(CFDictionaryRef headerFields, CFURLRef url)
{
#if (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 90000)
    CFArrayRef parsedCookies = _CFHTTPParsedCookiesWithResponseHeaderFields(kCFAllocatorDefault, headerFields, url);
#else
    CFArrayRef parsedCookies = CFHTTPCookieCreateWithResponseHeaderFields(kCFAllocatorDefault, headerFields, url);
#endif
    if (!parsedCookies)
        parsedCookies = CFArrayCreate(kCFAllocatorDefault, 0, 0, &kCFTypeArrayCallBacks);

    return parsedCookies;
}

void setCookiesFromDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url, const String& value)
{
    // <rdar://problem/5632883> CFHTTPCookieStorage stores an empty cookie, which would be sent as "Cookie: =".
    if (value.isEmpty())
        return;

    RetainPtr<CFURLRef> urlCF = url.createCFURL();
    RetainPtr<CFURLRef> firstPartyForCookiesCF = firstParty.createCFURL();

    // <http://bugs.webkit.org/show_bug.cgi?id=6531>, <rdar://4409034>
    // cookiesWithResponseHeaderFields doesn't parse cookies without a value
    String cookieString = value.contains('=') ? value : value + "=";

    RetainPtr<CFStringRef> cookieStringCF = cookieString.createCFString();
    RetainPtr<CFDictionaryRef> headerFieldsCF = adoptCF(CFDictionaryCreate(kCFAllocatorDefault,
        (const void**)&s_setCookieKeyCF, (const void**)&cookieStringCF, 1,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    RetainPtr<CFArrayRef> unfilteredCookies = adoptCF(createCookies(headerFieldsCF.get(), urlCF.get()));
    CFHTTPCookieStorageSetCookies(session.cookieStorage().get(), filterCookies(unfilteredCookies.get()).get(), urlCF.get(), firstPartyForCookiesCF.get());
}

String cookiesForDOM(const NetworkStorageSession& session, const URL& firstParty, const URL& url)
{
    RetainPtr<CFArrayRef> cookiesCF = copyCookiesForURLWithFirstPartyURL(session, firstParty, url);
    RetainPtr<CFDictionaryRef> headerCF = adoptCF(CFHTTPCookieCopyRequestHeaderFields(kCFAllocatorDefault, filterCookies(cookiesCF.get()).get()));
    return (CFStringRef)CFDictionaryGetValue(headerCF.get(), s_cookieCF);
}

String cookieRequestHeaderFieldValue(const NetworkStorageSession& session, const URL& firstParty, const URL& url)
{
    RetainPtr<CFArrayRef> cookiesCF = copyCookiesForURLWithFirstPartyURL(session, firstParty, url);
    RetainPtr<CFDictionaryRef> headerCF = adoptCF(CFHTTPCookieCopyRequestHeaderFields(kCFAllocatorDefault, cookiesCF.get()));
    return (CFStringRef)CFDictionaryGetValue(headerCF.get(), s_cookieCF);
}

bool cookiesEnabled(const NetworkStorageSession& session, const URL& /*firstParty*/, const URL& /*url*/)
{
    CFHTTPCookieStorageAcceptPolicy policy = CFHTTPCookieStorageGetCookieAcceptPolicy(session.cookieStorage().get());
    return policy == CFHTTPCookieStorageAcceptPolicyOnlyFromMainDocumentDomain || policy == CFHTTPCookieStorageAcceptPolicyExclusivelyFromMainDocumentDomain || policy == CFHTTPCookieStorageAcceptPolicyAlways;
}

bool getRawCookies(const NetworkStorageSession& session, const URL& firstParty, const URL& url, Vector<Cookie>& rawCookies)
{
    rawCookies.clear();

    RetainPtr<CFArrayRef> cookiesCF = copyCookiesForURLWithFirstPartyURL(session, firstParty, url);

    CFIndex count = CFArrayGetCount(cookiesCF.get());
    rawCookies.reserveCapacity(count);

    for (CFIndex i = 0; i < count; i++) {
       CFHTTPCookieRef cookie = (CFHTTPCookieRef)CFArrayGetValueAtIndex(cookiesCF.get(), i);
       String name = cookieName(cookie).get();
       String value = cookieValue(cookie).get();
       String domain = cookieDomain(cookie).get();
       String path = cookiePath(cookie).get();

       double expires = (cookieExpirationTime(cookie) + kCFAbsoluteTimeIntervalSince1970) * 1000;

       bool httpOnly = CFHTTPCookieIsHTTPOnly(cookie);
       bool secure = CFHTTPCookieIsSecure(cookie);
       bool session = false;    // FIXME: Need API for if a cookie is a session cookie.

       rawCookies.uncheckedAppend(Cookie(name, value, domain, path, expires, httpOnly, secure, session));
    }

    return true;
}

void deleteCookie(const NetworkStorageSession& session, const URL& url, const String& name)
{
    RetainPtr<CFHTTPCookieStorageRef> cookieStorage = session.cookieStorage();

    RetainPtr<CFURLRef> urlCF = url.createCFURL();

    bool sendSecureCookies = url.protocolIs("https");
    RetainPtr<CFArrayRef> cookiesCF = adoptCF(CFHTTPCookieStorageCopyCookiesForURL(cookieStorage.get(), urlCF.get(), sendSecureCookies));

    CFIndex count = CFArrayGetCount(cookiesCF.get());
    for (CFIndex i = 0; i < count; i++) {
        CFHTTPCookieRef cookie = (CFHTTPCookieRef)CFArrayGetValueAtIndex(cookiesCF.get(), i);
        if (String(cookieName(cookie).get()) == name) {
            CFHTTPCookieStorageDeleteCookie(cookieStorage.get(), cookie);
            break;
        }
    }
}

void getHostnamesWithCookies(const NetworkStorageSession& session, HashSet<String>& hostnames)
{
    RetainPtr<CFArrayRef> cookiesCF = adoptCF(CFHTTPCookieStorageCopyCookies(session.cookieStorage().get()));
    if (!cookiesCF)
        return;

    CFIndex count = CFArrayGetCount(cookiesCF.get());
    for (CFIndex i = 0; i < count; ++i) {
        CFHTTPCookieRef cookie = static_cast<CFHTTPCookieRef>(const_cast<void *>(CFArrayGetValueAtIndex(cookiesCF.get(), i)));
        RetainPtr<CFStringRef> domain = cookieDomain(cookie);
        hostnames.add(domain.get());
    }
}

void deleteAllCookies(const NetworkStorageSession& session)
{
    CFHTTPCookieStorageDeleteAllCookies(session.cookieStorage().get());
}

#if PLATFORM(WIN)
void deleteCookiesForHostnames(const NetworkStorageSession& session, const Vector<String>& hostnames)
{
}

void deleteAllCookiesModifiedSince(const NetworkStorageSession&, std::chrono::system_clock::time_point)
{
}
#endif

} // namespace WebCore

#endif // USE(CFNETWORK)

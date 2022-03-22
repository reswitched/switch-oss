/*
 * Copyright (C) 2003-2017 Apple Inc. All rights reserved.
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

#pragma once

#include <wtf/Forward.h>
#include <wtf/HashSet.h>
#include <wtf/URL.h>
#include <wtf/WallTime.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class NetworkStorageSession;

struct Cookie;
struct CookieRequestHeaderFieldProxy;
struct SameSiteInfo;

enum class IncludeSecureCookies : bool;

std::pair<String, bool> cookiesForDOM(const NetworkStorageSession&, const URL& firstParty, const SameSiteInfo&, const URL&, Optional<FrameIdentifier> frameID, Optional<PageIdentifier> pageID, IncludeSecureCookies);
void setCookiesFromDOM(const NetworkStorageSession&, const URL& firstParty, const SameSiteInfo&, const URL&, Optional<FrameIdentifier> frameID, Optional<PageIdentifier> pageID, const String&);
bool cookiesEnabled(const NetworkStorageSession&);
std::pair<String, bool> cookieRequestHeaderFieldValue(const NetworkStorageSession&, const URL& firstParty, const SameSiteInfo&, const URL&, Optional<FrameIdentifier> frameID, Optional<PageIdentifier> pageID, IncludeSecureCookies);
std::pair<String, bool> cookieRequestHeaderFieldValue(const NetworkStorageSession&, const CookieRequestHeaderFieldProxy&);
bool getRawCookies(const NetworkStorageSession&, const URL& firstParty, const SameSiteInfo&, const URL&, Optional<FrameIdentifier> frameID, Optional<PageIdentifier> pageID, Vector<Cookie>&);
void deleteCookie(const NetworkStorageSession&, const URL&, const String&);
void getHostnamesWithCookies(const NetworkStorageSession&, HashSet<String>& hostnames);
void deleteCookiesForHostnames(const NetworkStorageSession&, const Vector<String>& cookieHostNames);
void deleteAllCookies(const NetworkStorageSession&);
void deleteAllCookiesModifiedSince(const NetworkStorageSession&, WallTime);

}

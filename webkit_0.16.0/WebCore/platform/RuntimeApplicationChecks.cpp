/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RuntimeApplicationChecks.h"

#if USE(CF)
#include <CoreFoundation/CoreFoundation.h>
#include <wtf/RetainPtr.h>
#endif

#include <wtf/text/WTFString.h>

namespace WebCore {
    
static bool mainBundleIsEqualTo(const String& bundleIdentifierString)
{
    // FIXME: We should consider merging this file with RuntimeApplicationChecksIOS.mm.
    // Then we can remove the PLATFORM(IOS)-guard.
#if USE(CF) && !PLATFORM(IOS)
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (!mainBundle)
        return false;

    CFStringRef bundleIdentifier = CFBundleGetIdentifier(mainBundle);
    if (!bundleIdentifier)
        return false;

    return CFStringCompare(bundleIdentifier, bundleIdentifierString.createCFString().get(), 0) == kCFCompareEqualTo;
#else
    UNUSED_PARAM(bundleIdentifierString);
    return false;
#endif
}

bool applicationIsSafari()
{
#if !PLATFORM(WKC)
    static bool isSafari = mainBundleIsEqualTo("com.apple.Safari");
#else
    WKC_DEFINE_STATIC_BOOL(isSafari, mainBundleIsEqualTo("com.apple.Safari"));
#endif
    return isSafari;
}

bool applicationIsAppleMail()
{
#if !PLATFORM(WKC)
    static bool isAppleMail = mainBundleIsEqualTo("com.apple.mail");
#else
    WKC_DEFINE_STATIC_BOOL(isAppleMail, mainBundleIsEqualTo("com.apple.mail"));
#endif
    return isAppleMail;
}

bool applicationIsIBooks()
{
#if !PLATFORM(WKC)
    static bool isIBooks = mainBundleIsEqualTo("com.apple.iBooksX");
#else
    WKC_DEFINE_STATIC_BOOL(isIBooks, mainBundleIsEqualTo("com.apple.iBooksX"));
#endif
    return isIBooks;
}

bool applicationIsITunes()
{
#if !PLATFORM(WKC)
    static bool isITunes = mainBundleIsEqualTo("com.apple.iTunes");
#else
    WKC_DEFINE_STATIC_BOOL(isITunes, mainBundleIsEqualTo("com.apple.iTunes"));
#endif
    return isITunes;
}

bool applicationIsMicrosoftMessenger()
{
#if !PLATFORM(WKC)
    static bool isMicrosoftMessenger = mainBundleIsEqualTo("com.microsoft.Messenger");
#else
    WKC_DEFINE_STATIC_BOOL(isMicrosoftMessenger, mainBundleIsEqualTo("com.microsoft.Messenger"));
#endif
    return isMicrosoftMessenger;
}

bool applicationIsAdobeInstaller()
{
#if !PLATFORM(WKC)
    static bool isAdobeInstaller = mainBundleIsEqualTo("com.adobe.Installers.Setup");
#else
    WKC_DEFINE_STATIC_BOOL(isAdobeInstaller, mainBundleIsEqualTo("com.adobe.Installers.Setup"));
#endif
    return isAdobeInstaller;
}

bool applicationIsAOLInstantMessenger()
{
#if !PLATFORM(WKC)
    static bool isAOLInstantMessenger = mainBundleIsEqualTo("com.aol.aim.desktop");
#else
    WKC_DEFINE_STATIC_BOOL(isAOLInstantMessenger, mainBundleIsEqualTo("com.aol.aim.desktop"));
#endif
    return isAOLInstantMessenger;
}

bool applicationIsMicrosoftMyDay()
{
#if !PLATFORM(WKC)
    static bool isMicrosoftMyDay = mainBundleIsEqualTo("com.microsoft.myday");
#else
    WKC_DEFINE_STATIC_BOOL(isMicrosoftMyDay, mainBundleIsEqualTo("com.microsoft.myday"));
#endif
    return isMicrosoftMyDay;
}

bool applicationIsMicrosoftOutlook()
{
#if !PLATFORM(WKC)
    static bool isMicrosoftOutlook = mainBundleIsEqualTo("com.microsoft.Outlook");
#else
    WKC_DEFINE_STATIC_BOOL(isMicrosoftOutlook, mainBundleIsEqualTo("com.microsoft.Outlook"));
#endif
    return isMicrosoftOutlook;
}

bool applicationIsQuickenEssentials()
{
#if !PLATFORM(WKC)
    static bool isQuickenEssentials = mainBundleIsEqualTo("com.intuit.QuickenEssentials");
#else
    WKC_DEFINE_STATIC_BOOL(isQuickenEssentials, mainBundleIsEqualTo("com.intuit.QuickenEssentials"));
#endif
    return isQuickenEssentials;
}

bool applicationIsAperture()
{
#if !PLATFORM(WKC)
    static bool isAperture = mainBundleIsEqualTo("com.apple.Aperture");
#else
    WKC_DEFINE_STATIC_BOOL(isAperture, mainBundleIsEqualTo("com.apple.Aperture"));
#endif
    return isAperture;
}

bool applicationIsVersions()
{
#if !PLATFORM(WKC)
    static bool isVersions = mainBundleIsEqualTo("com.blackpixel.versions");
#else
    WKC_DEFINE_STATIC_BOOL(isVersions, mainBundleIsEqualTo("com.blackpixel.versions"));
#endif
    return isVersions;
}

bool applicationIsHRBlock()
{
#if !PLATFORM(WKC)
    static bool isHRBlock = mainBundleIsEqualTo("com.hrblock.tax.2010");
#else
    WKC_DEFINE_STATIC_BOOL(isHRBlock, mainBundleIsEqualTo("com.hrblock.tax.2010"));
#endif
    return isHRBlock;
}

bool applicationIsSolidStateNetworksDownloader()
{
#if !PLATFORM(WKC)
    static bool isSolidStateNetworksDownloader = mainBundleIsEqualTo("com.solidstatenetworks.awkhost");
#else
    WKC_DEFINE_STATIC_BOOL(isSolidStateNetworksDownloader, mainBundleIsEqualTo("com.solidstatenetworks.awkhost"));
#endif
    return isSolidStateNetworksDownloader;
}

} // namespace WebCore

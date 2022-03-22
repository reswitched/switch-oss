/*
 * Copyright (C) 2016-2018 Apple Inc. All rights reserved.
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

#import "config.h"
#import "AVAssetMIMETypeCache.h"

#if PLATFORM(COCOA)

#import "ContentType.h"
#import <pal/cf/CoreMediaSoftLink.h>
#import <pal/cocoa/AVFoundationSoftLink.h>
#import <wtf/HashSet.h>

#if !PLATFORM(MACCATALYST)
SOFT_LINK_FRAMEWORK_OPTIONAL_PREFLIGHT(AVFoundation)
#endif

namespace WebCore {

AVAssetMIMETypeCache& AVAssetMIMETypeCache::singleton()
{
    static NeverDestroyed<AVAssetMIMETypeCache> cache;
    return cache.get();
}

bool AVAssetMIMETypeCache::isAvailable() const
{
#if ENABLE(VIDEO) && USE(AVFOUNDATION)
#if PLATFORM(MACCATALYST)
    // FIXME: This should be using AVFoundationLibraryIsAvailable() instead, but doing so causes soft-linking
    // to subsequently fail on certain symbols. See <rdar://problem/42224780> for more details.
    return PAL::isAVFoundationFrameworkAvailable();
#else
    return AVFoundationLibraryIsAvailable();
#endif
#else
    return false;
#endif
}

bool AVAssetMIMETypeCache::canDecodeTypeInternal(const ContentType& type)
{
#if ENABLE(VIDEO) && USE(AVFOUNDATION)
    ASSERT(isAvailable());
    return [PAL::getAVURLAssetClass() isPlayableExtendedMIMEType:type.raw()];
#endif

    return false;
}

bool AVAssetMIMETypeCache::isUnsupportedContainerType(const String& type)
{
    if (type.isEmpty())
        return false;

    String lowerCaseType = type.convertToASCIILowercase();

    // AVFoundation will return non-video MIME types which it claims to support, but which we
    // do not support in the <video> element. Reject all non video/, audio/, and application/ types.
    if (!lowerCaseType.startsWith("video/") && !lowerCaseType.startsWith("audio/") && !lowerCaseType.startsWith("application/"))
        return true;

    // Reject types we know AVFoundation does not support that sites commonly ask about.
    if (lowerCaseType == "video/webm" || lowerCaseType == "audio/webm" || lowerCaseType == "video/x-webm")
        return true;

    if (lowerCaseType == "video/x-flv")
        return true;

    if (lowerCaseType == "audio/ogg" || lowerCaseType == "video/ogg" || lowerCaseType == "application/ogg")
        return true;

    if (lowerCaseType == "video/h264")
        return true;

    return false;
}

const HashSet<String, ASCIICaseInsensitiveHash>& AVAssetMIMETypeCache::staticContainerTypeList()
{
    static const auto cache = makeNeverDestroyed(HashSet<String, ASCIICaseInsensitiveHash> {
        "application/vnd.apple.mpegurl",
        "application/x-mpegurl",
        "audio/3gpp",
        "audio/aac",
        "audio/aacp",
        "audio/aiff",
        "audio/basic",
        "audio/mp3",
        "audio/mp4",
        "audio/mpeg",
        "audio/mpeg3",
        "audio/mpegurl",
        "audio/mpg",
        "audio/vnd.wave",
        "audio/wav",
        "audio/wave",
        "audio/x-aac",
        "audio/x-aiff",
        "audio/x-m4a",
        "audio/x-mpegurl",
        "audio/x-wav",
        "video/3gpp",
        "video/3gpp2",
        "video/mp4",
        "video/mpeg",
        "video/mpeg2",
        "video/mpg",
        "video/quicktime",
        "video/x-m4v",
        "video/x-mpeg",
        "video/x-mpg",
    });
    return cache;
}

void AVAssetMIMETypeCache::initializeCache(HashSet<String, ASCIICaseInsensitiveHash>& cache)
{
#if ENABLE(VIDEO) && USE(AVFOUNDATION)
    if (!isAvailable())
        return;

    for (NSString* type in [PAL::getAVURLAssetClass() audiovisualMIMETypes])
        cache.add(type);

    if (m_cacheTypeCallback)
        m_cacheTypeCallback(copyToVector(cache));
#endif
}

}

#endif

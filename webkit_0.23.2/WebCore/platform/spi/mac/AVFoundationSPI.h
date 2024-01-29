/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#import "SoftLinking.h"
#import <objc/runtime.h>

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)

#if USE(APPLE_INTERNAL_SDK)

#import <AVFoundation/AVOutputContext.h>
#import <AVFoundation/AVPlayer_Private.h>

#else

#import <AVFoundation/AVPlayer.h>

@class AVOutputContext;
@interface AVOutputContext : NSObject <NSSecureCoding>
@property (nonatomic, readonly) NSString *deviceName;
+ (instancetype)outputContext;
@end

@interface AVPlayer (AVPlayerExternalPlaybackSupportPrivate)
@property (nonatomic, retain) AVOutputContext *outputContext;
@end

#endif

#endif // ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)

#if PLATFORM(IOS)

#if HAVE(AVKIT) && USE(APPLE_INTERNAL_SDK)

#import <AVFoundation/AVPlayerLayer_Private.h>
#import <AVKit/AVPlayerViewController_WebKitOnly.h>

#else

#import <AVFoundation/AVPlayerLayer.h>

#endif

#if !HAVE(AVKIT) || !USE(APPLE_INTERNAL_SDK) || __IPHONE_OS_VERSION_MIN_REQUIRED < 90000

@interface AVPlayerLayer (AVPlayerLayerPictureInPictureModeSupportPrivate)
- (void)setPIPModeEnabled:(BOOL)flag;
@end

#endif

#endif // PLATFORM(IOS)


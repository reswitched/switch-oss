/*
 * Copyright (C) 2011-2015 Apple Inc. All rights reserved.
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

#if ENABLE(VIDEO) && USE(AVFOUNDATION)
#import "MediaPlayerPrivateAVFoundationObjC.h"

#import "AVFoundationSPI.h"
#import "AVTrackPrivateAVFObjCImpl.h"
#import "AudioSourceProviderAVFObjC.h"
#import "AudioTrackPrivateAVFObjC.h"
#import "AuthenticationChallenge.h"
#import "BlockExceptions.h"
#import "CDMSessionAVFoundationObjC.h"
#import "Cookie.h"
#import "ExceptionCodePlaceholder.h"
#import "FloatConversion.h"
#import "FloatConversion.h"
#import "GraphicsContext.h"
#import "GraphicsContextCG.h"
#import "InbandMetadataTextTrackPrivateAVF.h"
#import "InbandTextTrackPrivateAVFObjC.h"
#import "InbandTextTrackPrivateLegacyAVFObjC.h"
#import "OutOfBandTextTrackPrivateAVF.h"
#import "URL.h"
#import "Logging.h"
#import "MediaPlaybackTargetMac.h"
#import "MediaSelectionGroupAVFObjC.h"
#import "MediaTimeAVFoundation.h"
#import "PlatformTimeRanges.h"
#import "QuartzCoreSPI.h"
#import "SecurityOrigin.h"
#import "SerializedPlatformRepresentationMac.h"
#import "TextEncoding.h"
#import "TextTrackRepresentation.h"
#import "UUID.h"
#import "VideoTrackPrivateAVFObjC.h"
#import "WebCoreAVFResourceLoader.h"
#import "WebCoreCALayerExtras.h"
#import "WebCoreSystemInterface.h"
#import <functional>
#import <objc/runtime.h>
#import <runtime/DataView.h>
#import <runtime/JSCInlines.h>
#import <runtime/TypedArrayInlines.h>
#import <runtime/Uint16Array.h>
#import <runtime/Uint32Array.h>
#import <runtime/Uint8Array.h>
#import <wtf/CurrentTime.h>
#import <wtf/ListHashSet.h>
#import <wtf/NeverDestroyed.h>
#import <wtf/text/CString.h>
#import <wtf/text/StringBuilder.h>

#if ENABLE(AVF_CAPTIONS)
#include "TextTrack.h"
#endif

#import <AVFoundation/AVFoundation.h>
#if PLATFORM(IOS)
#import "WAKAppKitStubs.h"
#import <CoreImage/CoreImage.h>
#import <mach/mach_port.h>
#else
#import <Foundation/NSGeometry.h>
#import <QuartzCore/CoreImage.h>
#endif

#if USE(VIDEOTOOLBOX)
#import <CoreVideo/CoreVideo.h>
#import <VideoToolbox/VideoToolbox.h>
#endif

#if USE(CFNETWORK)
#include "CFNSURLConnectionSPI.h"
#endif

namespace std {
template <> struct iterator_traits<HashSet<RefPtr<WebCore::MediaSelectionOptionAVFObjC>>::iterator> {
    typedef RefPtr<WebCore::MediaSelectionOptionAVFObjC> value_type;
};
}

@interface WebVideoContainerLayer : CALayer
@end

@implementation WebVideoContainerLayer

- (void)setBounds:(CGRect)bounds
{
    [super setBounds:bounds];
    for (CALayer* layer in self.sublayers)
        layer.frame = bounds;
}

- (void)setPosition:(CGPoint)position
{
    if (!CATransform3DIsIdentity(self.transform)) {
        // Pre-apply the transform added in the WebProcess to fix <rdar://problem/18316542> to the position.
        position = CGPointApplyAffineTransform(position, CATransform3DGetAffineTransform(self.transform));
    }
    [super setPosition:position];
}
@end

#if ENABLE(AVF_CAPTIONS)
// Note: This must be defined before our SOFT_LINK macros:
@class AVMediaSelectionOption;
@interface AVMediaSelectionOption (OutOfBandExtensions)
@property (nonatomic, readonly) NSString* outOfBandSource;
@property (nonatomic, readonly) NSString* outOfBandIdentifier;
@end
#endif

@interface AVURLAsset (WebKitExtensions)
@property (nonatomic, readonly) NSURL *resolvedURL;
@end

typedef AVPlayer AVPlayerType;
typedef AVPlayerItem AVPlayerItemType;
typedef AVPlayerItemLegibleOutput AVPlayerItemLegibleOutputType;
typedef AVPlayerItemVideoOutput AVPlayerItemVideoOutputType;
typedef AVMetadataItem AVMetadataItemType;
typedef AVMediaSelectionGroup AVMediaSelectionGroupType;
typedef AVMediaSelectionOption AVMediaSelectionOptionType;

#pragma mark - Soft Linking

// Soft-linking headers must be included last since they #define functions, constants, etc.
#import "CoreMediaSoftLink.h"

SOFT_LINK_FRAMEWORK_OPTIONAL(AVFoundation)

SOFT_LINK_FRAMEWORK_OPTIONAL(CoreImage)
SOFT_LINK_FRAMEWORK_OPTIONAL(CoreVideo)

#if USE(VIDEOTOOLBOX)
SOFT_LINK_FRAMEWORK_OPTIONAL(VideoToolbox)
#endif

SOFT_LINK(CoreVideo, CVPixelBufferGetWidth, size_t, (CVPixelBufferRef pixelBuffer), (pixelBuffer))
SOFT_LINK(CoreVideo, CVPixelBufferGetHeight, size_t, (CVPixelBufferRef pixelBuffer), (pixelBuffer))
SOFT_LINK(CoreVideo, CVPixelBufferGetBaseAddress, void*, (CVPixelBufferRef pixelBuffer), (pixelBuffer))
SOFT_LINK(CoreVideo, CVPixelBufferGetBytesPerRow, size_t, (CVPixelBufferRef pixelBuffer), (pixelBuffer))
SOFT_LINK(CoreVideo, CVPixelBufferGetDataSize, size_t, (CVPixelBufferRef pixelBuffer), (pixelBuffer))
SOFT_LINK(CoreVideo, CVPixelBufferGetPixelFormatType, OSType, (CVPixelBufferRef pixelBuffer), (pixelBuffer))
SOFT_LINK(CoreVideo, CVPixelBufferLockBaseAddress, CVReturn, (CVPixelBufferRef pixelBuffer, CVOptionFlags lockFlags), (pixelBuffer, lockFlags))
SOFT_LINK(CoreVideo, CVPixelBufferUnlockBaseAddress, CVReturn, (CVPixelBufferRef pixelBuffer, CVOptionFlags lockFlags), (pixelBuffer, lockFlags))

#if USE(VIDEOTOOLBOX)
SOFT_LINK(VideoToolbox, VTPixelTransferSessionCreate, OSStatus, (CFAllocatorRef allocator, VTPixelTransferSessionRef *pixelTransferSessionOut), (allocator, pixelTransferSessionOut))
SOFT_LINK(VideoToolbox, VTPixelTransferSessionTransferImage, OSStatus, (VTPixelTransferSessionRef session, CVPixelBufferRef sourceBuffer, CVPixelBufferRef destinationBuffer), (session, sourceBuffer, destinationBuffer))
#endif

SOFT_LINK_CLASS(AVFoundation, AVPlayer)
SOFT_LINK_CLASS(AVFoundation, AVPlayerItem)
SOFT_LINK_CLASS(AVFoundation, AVPlayerItemVideoOutput)
SOFT_LINK_CLASS(AVFoundation, AVPlayerLayer)
SOFT_LINK_CLASS(AVFoundation, AVURLAsset)
SOFT_LINK_CLASS(AVFoundation, AVAssetImageGenerator)
SOFT_LINK_CLASS(AVFoundation, AVMetadataItem)

SOFT_LINK_CLASS(CoreImage, CIContext)
SOFT_LINK_CLASS(CoreImage, CIImage)

SOFT_LINK_POINTER(AVFoundation, AVAudioTimePitchAlgorithmSpectral, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVAudioTimePitchAlgorithmVarispeed, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicVisual, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicAudible, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaTypeClosedCaption, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaTypeVideo, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaTypeAudio, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaTypeMetadata, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVPlayerItemDidPlayToEndTimeNotification, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVURLAssetInheritURIQueryComponentFromReferencingURIKey, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVAssetImageGeneratorApertureModeCleanAperture, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVURLAssetReferenceRestrictionsKey, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVLayerVideoGravityResizeAspect, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVLayerVideoGravityResizeAspectFill, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVLayerVideoGravityResize, NSString *)
SOFT_LINK_POINTER(CoreVideo, kCVPixelBufferPixelFormatTypeKey, NSString *)

SOFT_LINK_POINTER_OPTIONAL(AVFoundation, AVURLAssetClientBundleIdentifierKey, NSString *)

#define AVPlayer getAVPlayerClass()
#define AVPlayerItem getAVPlayerItemClass()
#define AVPlayerLayer getAVPlayerLayerClass()
#define AVURLAsset getAVURLAssetClass()
#define AVAssetImageGenerator getAVAssetImageGeneratorClass()
#define AVMetadataItem getAVMetadataItemClass()

#define AVAudioTimePitchAlgorithmSpectral getAVAudioTimePitchAlgorithmSpectral()
#define AVAudioTimePitchAlgorithmVarispeed getAVAudioTimePitchAlgorithmVarispeed()
#define AVMediaCharacteristicVisual getAVMediaCharacteristicVisual()
#define AVMediaCharacteristicAudible getAVMediaCharacteristicAudible()
#define AVMediaTypeClosedCaption getAVMediaTypeClosedCaption()
#define AVMediaTypeVideo getAVMediaTypeVideo()
#define AVMediaTypeAudio getAVMediaTypeAudio()
#define AVMediaTypeMetadata getAVMediaTypeMetadata()
#define AVPlayerItemDidPlayToEndTimeNotification getAVPlayerItemDidPlayToEndTimeNotification()
#define AVURLAssetInheritURIQueryComponentFromReferencingURIKey getAVURLAssetInheritURIQueryComponentFromReferencingURIKey()
#define AVURLAssetClientBundleIdentifierKey getAVURLAssetClientBundleIdentifierKey()
#define AVAssetImageGeneratorApertureModeCleanAperture getAVAssetImageGeneratorApertureModeCleanAperture()
#define AVURLAssetReferenceRestrictionsKey getAVURLAssetReferenceRestrictionsKey()
#define AVLayerVideoGravityResizeAspect getAVLayerVideoGravityResizeAspect()
#define AVLayerVideoGravityResizeAspectFill getAVLayerVideoGravityResizeAspectFill()
#define AVLayerVideoGravityResize getAVLayerVideoGravityResize()
#define kCVPixelBufferPixelFormatTypeKey getkCVPixelBufferPixelFormatTypeKey()

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
typedef AVMediaSelectionGroup AVMediaSelectionGroupType;
typedef AVMediaSelectionOption AVMediaSelectionOptionType;

SOFT_LINK_CLASS(AVFoundation, AVPlayerItemLegibleOutput)
SOFT_LINK_CLASS(AVFoundation, AVMediaSelectionGroup)
SOFT_LINK_CLASS(AVFoundation, AVMediaSelectionOption)

SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicLegible, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaTypeSubtitle, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicContainsOnlyForcedSubtitles, NSString *)
SOFT_LINK_POINTER(AVFoundation, AVPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly, NSString *)

#define AVPlayerItemLegibleOutput getAVPlayerItemLegibleOutputClass()
#define AVMediaSelectionGroup getAVMediaSelectionGroupClass()
#define AVMediaSelectionOption getAVMediaSelectionOptionClass()
#define AVMediaCharacteristicLegible getAVMediaCharacteristicLegible()
#define AVMediaTypeSubtitle getAVMediaTypeSubtitle()
#define AVMediaCharacteristicContainsOnlyForcedSubtitles getAVMediaCharacteristicContainsOnlyForcedSubtitles()
#define AVPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly getAVPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly()
#endif

#if ENABLE(AVF_CAPTIONS)
SOFT_LINK_POINTER(AVFoundation, AVURLAssetHTTPCookiesKey, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVURLAssetOutOfBandAlternateTracksKey, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVOutOfBandAlternateTrackDisplayNameKey, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVOutOfBandAlternateTrackExtendedLanguageTagKey, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVOutOfBandAlternateTrackIsDefaultKey, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVOutOfBandAlternateTrackMediaCharactersticsKey, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVOutOfBandAlternateTrackIdentifierKey, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVOutOfBandAlternateTrackSourceKey, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicDescribesMusicAndSoundForAccessibility, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicTranscribesSpokenDialogForAccessibility, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicIsAuxiliaryContent, NSString*)

#define AVURLAssetHTTPCookiesKey getAVURLAssetHTTPCookiesKey()
#define AVURLAssetOutOfBandAlternateTracksKey getAVURLAssetOutOfBandAlternateTracksKey()
#define AVOutOfBandAlternateTrackDisplayNameKey getAVOutOfBandAlternateTrackDisplayNameKey()
#define AVOutOfBandAlternateTrackExtendedLanguageTagKey getAVOutOfBandAlternateTrackExtendedLanguageTagKey()
#define AVOutOfBandAlternateTrackIsDefaultKey getAVOutOfBandAlternateTrackIsDefaultKey()
#define AVOutOfBandAlternateTrackMediaCharactersticsKey getAVOutOfBandAlternateTrackMediaCharactersticsKey()
#define AVOutOfBandAlternateTrackIdentifierKey getAVOutOfBandAlternateTrackIdentifierKey()
#define AVOutOfBandAlternateTrackSourceKey getAVOutOfBandAlternateTrackSourceKey()
#define AVMediaCharacteristicDescribesMusicAndSoundForAccessibility getAVMediaCharacteristicDescribesMusicAndSoundForAccessibility()
#define AVMediaCharacteristicTranscribesSpokenDialogForAccessibility getAVMediaCharacteristicTranscribesSpokenDialogForAccessibility()
#define AVMediaCharacteristicIsAuxiliaryContent getAVMediaCharacteristicIsAuxiliaryContent()
#endif

#if ENABLE(DATACUE_VALUE)
SOFT_LINK_POINTER(AVFoundation, AVMetadataKeySpaceQuickTimeUserData, NSString*)
SOFT_LINK_POINTER_OPTIONAL(AVFoundation, AVMetadataKeySpaceISOUserData, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVMetadataKeySpaceQuickTimeMetadata, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVMetadataKeySpaceiTunes, NSString*)
SOFT_LINK_POINTER(AVFoundation, AVMetadataKeySpaceID3, NSString*)

#define AVMetadataKeySpaceQuickTimeUserData getAVMetadataKeySpaceQuickTimeUserData()
#define AVMetadataKeySpaceISOUserData getAVMetadataKeySpaceISOUserData()
#define AVMetadataKeySpaceQuickTimeMetadata getAVMetadataKeySpaceQuickTimeMetadata()
#define AVMetadataKeySpaceiTunes getAVMetadataKeySpaceiTunes()
#define AVMetadataKeySpaceID3 getAVMetadataKeySpaceID3()
#endif

#if PLATFORM(IOS)
SOFT_LINK_POINTER(AVFoundation, AVURLAssetBoundNetworkInterfaceName, NSString *)
#define AVURLAssetBoundNetworkInterfaceName getAVURLAssetBoundNetworkInterfaceName()
#endif

#if ENABLE(AVF_CAPTIONS)
SOFT_LINK_FRAMEWORK(MediaToolbox)
SOFT_LINK_OPTIONAL(MediaToolbox, MTEnableCaption2015Behavior, Boolean, (), ())
#endif

using namespace WebCore;

enum MediaPlayerAVFoundationObservationContext {
    MediaPlayerAVFoundationObservationContextPlayerItem,
    MediaPlayerAVFoundationObservationContextPlayerItemTrack,
    MediaPlayerAVFoundationObservationContextPlayer,
    MediaPlayerAVFoundationObservationContextAVPlayerLayer,
};

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP) && HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
@interface WebCoreAVFMovieObserver : NSObject <AVPlayerItemLegibleOutputPushDelegate>
#else
@interface WebCoreAVFMovieObserver : NSObject
#endif
{
    MediaPlayerPrivateAVFoundationObjC* m_callback;
    int m_delayCallbacks;
}
-(id)initWithCallback:(MediaPlayerPrivateAVFoundationObjC*)callback;
-(void)disconnect;
-(void)metadataLoaded;
-(void)didEnd:(NSNotification *)notification;
-(void)observeValueForKeyPath:keyPath ofObject:(id)object change:(NSDictionary *)change context:(MediaPlayerAVFoundationObservationContext)context;
#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
- (void)legibleOutput:(id)output didOutputAttributedStrings:(NSArray *)strings nativeSampleBuffers:(NSArray *)nativeSamples forItemTime:(CMTime)itemTime;
- (void)outputSequenceWasFlushed:(id)output;
#endif
@end

#if HAVE(AVFOUNDATION_LOADER_DELEGATE)
@interface WebCoreAVFLoaderDelegate : NSObject<AVAssetResourceLoaderDelegate> {
    MediaPlayerPrivateAVFoundationObjC* m_callback;
}
- (id)initWithCallback:(MediaPlayerPrivateAVFoundationObjC*)callback;
- (BOOL)resourceLoader:(AVAssetResourceLoader *)resourceLoader shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest;
- (void)setCallback:(MediaPlayerPrivateAVFoundationObjC*)callback;
@end
#endif

#if HAVE(AVFOUNDATION_VIDEO_OUTPUT)
@interface WebCoreAVFPullDelegate : NSObject<AVPlayerItemOutputPullDelegate> {
    MediaPlayerPrivateAVFoundationObjC *m_callback;
    dispatch_semaphore_t m_semaphore;
}
- (id)initWithCallback:(MediaPlayerPrivateAVFoundationObjC *)callback;
- (void)setCallback:(MediaPlayerPrivateAVFoundationObjC*)callback;
- (void)outputMediaDataWillChange:(AVPlayerItemOutput *)sender;
- (void)outputSequenceWasFlushed:(AVPlayerItemOutput *)output;
@end
#endif

namespace WebCore {

static NSArray *assetMetadataKeyNames();
static NSArray *itemKVOProperties();
static NSArray *assetTrackMetadataKeyNames();
static NSArray *playerKVOProperties();
static AVAssetTrack* firstEnabledTrack(NSArray* tracks);

#if !LOG_DISABLED
static const char *boolString(bool val)
{
    return val ? "true" : "false";
}
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
typedef HashMap<MediaPlayer*, MediaPlayerPrivateAVFoundationObjC*> PlayerToPrivateMapType;
static PlayerToPrivateMapType& playerToPrivateMap()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(PlayerToPrivateMapType, map, ());
    return map;
};
#endif

#if HAVE(AVFOUNDATION_LOADER_DELEGATE)
static dispatch_queue_t globalLoaderDelegateQueue()
{
    static dispatch_queue_t globalQueue;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        globalQueue = dispatch_queue_create("WebCoreAVFLoaderDelegate queue", DISPATCH_QUEUE_SERIAL);
    });
    return globalQueue;
}
#endif

#if HAVE(AVFOUNDATION_VIDEO_OUTPUT)
static dispatch_queue_t globalPullDelegateQueue()
{
    static dispatch_queue_t globalQueue;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        globalQueue = dispatch_queue_create("WebCoreAVFPullDelegate queue", DISPATCH_QUEUE_SERIAL);
    });
    return globalQueue;
}
#endif

#if USE(CFNETWORK)
class WebCoreNSURLAuthenticationChallengeClient : public RefCounted<WebCoreNSURLAuthenticationChallengeClient>, public AuthenticationClient {
public:
    static RefPtr<WebCoreNSURLAuthenticationChallengeClient> create(NSURLAuthenticationChallenge *challenge)
    {
        return adoptRef(new WebCoreNSURLAuthenticationChallengeClient(challenge));
    }

    using RefCounted<WebCoreNSURLAuthenticationChallengeClient>::ref;
    using RefCounted<WebCoreNSURLAuthenticationChallengeClient>::deref;

private:
    WebCoreNSURLAuthenticationChallengeClient(NSURLAuthenticationChallenge *challenge)
        : m_challenge(challenge)
    {
        ASSERT(m_challenge);
    }

    virtual void refAuthenticationClient() override { ref(); }
    virtual void derefAuthenticationClient() override { deref(); }

    virtual void receivedCredential(const AuthenticationChallenge&, const Credential& credential) override
    {
        [[m_challenge sender] useCredential:credential.nsCredential() forAuthenticationChallenge:m_challenge.get()];
    }

    virtual void receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&) override
    {
        [[m_challenge sender] continueWithoutCredentialForAuthenticationChallenge:m_challenge.get()];
    }

    virtual void receivedCancellation(const AuthenticationChallenge&) override
    {
        [[m_challenge sender] cancelAuthenticationChallenge:m_challenge.get()];
    }

    virtual void receivedRequestToPerformDefaultHandling(const AuthenticationChallenge&) override
    {
        if ([[m_challenge sender] respondsToSelector:@selector(performDefaultHandlingForAuthenticationChallenge:)])
            [[m_challenge sender] performDefaultHandlingForAuthenticationChallenge:m_challenge.get()];
    }

    virtual void receivedChallengeRejection(const AuthenticationChallenge&) override
    {
        if ([[m_challenge sender] respondsToSelector:@selector(rejectProtectionSpaceAndContinueWithChallenge:)])
            [[m_challenge sender] rejectProtectionSpaceAndContinueWithChallenge:m_challenge.get()];
    }

    RetainPtr<NSURLAuthenticationChallenge> m_challenge;
};
#endif

void MediaPlayerPrivateAVFoundationObjC::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
        registrar([](MediaPlayer* player) { return std::make_unique<MediaPlayerPrivateAVFoundationObjC>(player); },
            getSupportedTypes, supportsType, 0, 0, 0, supportsKeySystem);
}

MediaPlayerPrivateAVFoundationObjC::MediaPlayerPrivateAVFoundationObjC(MediaPlayer* player)
    : MediaPlayerPrivateAVFoundation(player)
    , m_weakPtrFactory(this)
#if PLATFORM(IOS)
    , m_videoFullscreenGravity(MediaPlayer::VideoGravityResizeAspect)
#endif
    , m_objcObserver(adoptNS([[WebCoreAVFMovieObserver alloc] initWithCallback:this]))
    , m_videoFrameHasDrawn(false)
    , m_haveCheckedPlayability(false)
#if HAVE(AVFOUNDATION_VIDEO_OUTPUT)
    , m_videoOutputDelegate(adoptNS([[WebCoreAVFPullDelegate alloc] initWithCallback:this]))
    , m_videoOutputSemaphore(nullptr)
#endif
#if HAVE(AVFOUNDATION_LOADER_DELEGATE)
    , m_loaderDelegate(adoptNS([[WebCoreAVFLoaderDelegate alloc] initWithCallback:this]))
#endif
    , m_currentTextTrack(0)
    , m_cachedRate(0)
    , m_cachedTotalBytes(0)
    , m_pendingStatusChanges(0)
    , m_cachedItemStatus(MediaPlayerAVPlayerItemStatusDoesNotExist)
    , m_cachedLikelyToKeepUp(false)
    , m_cachedBufferEmpty(false)
    , m_cachedBufferFull(false)
    , m_cachedHasEnabledAudio(false)
    , m_shouldBufferData(true)
    , m_cachedIsReadyForDisplay(false)
    , m_haveBeenAskedToCreateLayer(false)
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    , m_allowsWirelessVideoPlayback(true)
#endif
{
#if ENABLE(ENCRYPTED_MEDIA_V2)
    playerToPrivateMap().set(player, this);
#endif
}

MediaPlayerPrivateAVFoundationObjC::~MediaPlayerPrivateAVFoundationObjC()
{
#if ENABLE(ENCRYPTED_MEDIA_V2)
    playerToPrivateMap().remove(player());
#endif
#if HAVE(AVFOUNDATION_LOADER_DELEGATE)
    [m_loaderDelegate.get() setCallback:0];
    [[m_avAsset.get() resourceLoader] setDelegate:nil queue:0];

    for (auto& pair : m_resourceLoaderMap)
        pair.value->invalidate();
#endif
#if HAVE(AVFOUNDATION_VIDEO_OUTPUT)
    [m_videoOutputDelegate setCallback:0];
    [m_videoOutput setDelegate:nil queue:0];
    if (m_videoOutputSemaphore)
        dispatch_release(m_videoOutputSemaphore);
#endif

    if (m_videoLayer)
        destroyVideoLayer();

    cancelLoad();
}

void MediaPlayerPrivateAVFoundationObjC::cancelLoad()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::cancelLoad(%p)", this);
    tearDownVideoRendering();

    [[NSNotificationCenter defaultCenter] removeObserver:m_objcObserver.get()];
    [m_objcObserver.get() disconnect];

    // Tell our observer to do nothing when our cancellation of pending loading calls its completion handler.
    setIgnoreLoadStateChanges(true);
    if (m_avAsset) {
        [m_avAsset.get() cancelLoading];
        m_avAsset = nil;
    }

    clearTextTracks();

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP) && HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
    if (m_legibleOutput) {
        if (m_avPlayerItem)
            [m_avPlayerItem.get() removeOutput:m_legibleOutput.get()];
        m_legibleOutput = nil;
    }
#endif

    if (m_avPlayerItem) {
        for (NSString *keyName in itemKVOProperties())
            [m_avPlayerItem.get() removeObserver:m_objcObserver.get() forKeyPath:keyName];
        
        m_avPlayerItem = nil;
    }
    if (m_avPlayer) {
        if (m_timeObserver)
            [m_avPlayer.get() removeTimeObserver:m_timeObserver.get()];
        m_timeObserver = nil;

        for (NSString *keyName in playerKVOProperties())
            [m_avPlayer.get() removeObserver:m_objcObserver.get() forKeyPath:keyName];
        m_avPlayer = nil;
    }

    // Reset cached properties
    m_pendingStatusChanges = 0;
    m_cachedItemStatus = MediaPlayerAVPlayerItemStatusDoesNotExist;
    m_cachedSeekableRanges = nullptr;
    m_cachedLoadedRanges = nullptr;
    m_cachedHasEnabledAudio = false;
    m_cachedPresentationSize = FloatSize();
    m_cachedDuration = MediaTime::zeroTime();

    for (AVPlayerItemTrack *track in m_cachedTracks.get())
        [track removeObserver:m_objcObserver.get() forKeyPath:@"enabled"];
    m_cachedTracks = nullptr;

#if ENABLE(WEB_AUDIO) && USE(MEDIATOOLBOX)
    if (m_provider) {
        m_provider->setPlayerItem(nullptr);
        m_provider->setAudioTrack(nullptr);
    }
#endif

    setIgnoreLoadStateChanges(false);
}

bool MediaPlayerPrivateAVFoundationObjC::hasLayerRenderer() const
{
    return m_haveBeenAskedToCreateLayer;
}

bool MediaPlayerPrivateAVFoundationObjC::hasContextRenderer() const
{
#if HAVE(AVFOUNDATION_VIDEO_OUTPUT)
    if (m_videoOutput)
        return true;
#endif
    return m_imageGenerator;
}

void MediaPlayerPrivateAVFoundationObjC::createContextVideoRenderer()
{
#if HAVE(AVFOUNDATION_VIDEO_OUTPUT)
    createVideoOutput();
#else
    createImageGenerator();
#endif
}

void MediaPlayerPrivateAVFoundationObjC::createImageGenerator()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createImageGenerator(%p)", this);

    if (!m_avAsset || m_imageGenerator)
        return;

    m_imageGenerator = [AVAssetImageGenerator assetImageGeneratorWithAsset:m_avAsset.get()];

    [m_imageGenerator.get() setApertureMode:AVAssetImageGeneratorApertureModeCleanAperture];
    [m_imageGenerator.get() setAppliesPreferredTrackTransform:YES];
    [m_imageGenerator.get() setRequestedTimeToleranceBefore:kCMTimeZero];
    [m_imageGenerator.get() setRequestedTimeToleranceAfter:kCMTimeZero];

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createImageGenerator(%p) - returning %p", this, m_imageGenerator.get());
}

void MediaPlayerPrivateAVFoundationObjC::destroyContextVideoRenderer()
{
#if HAVE(AVFOUNDATION_VIDEO_OUTPUT)
    destroyVideoOutput();
#endif
    destroyImageGenerator();
}

void MediaPlayerPrivateAVFoundationObjC::destroyImageGenerator()
{
    if (!m_imageGenerator)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::destroyImageGenerator(%p) - destroying  %p", this, m_imageGenerator.get());

    m_imageGenerator = 0;
}

void MediaPlayerPrivateAVFoundationObjC::createVideoLayer()
{
    if (!m_avPlayer || m_haveBeenAskedToCreateLayer)
        return;

    auto weakThis = createWeakPtr();
    callOnMainThread([this, weakThis] {
        if (!weakThis)
            return;

        if (!m_avPlayer || m_haveBeenAskedToCreateLayer)
            return;
        m_haveBeenAskedToCreateLayer = true;

        if (!m_videoLayer)
            createAVPlayerLayer();

#if USE(VIDEOTOOLBOX) && (!defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000)
        if (!m_videoOutput)
            createVideoOutput();
#endif

        player()->client().mediaPlayerRenderingModeChanged(player());
    });
}

void MediaPlayerPrivateAVFoundationObjC::createAVPlayerLayer()
{
    if (!m_avPlayer)
        return;

    m_videoLayer = adoptNS([allocAVPlayerLayerInstance() init]);
    [m_videoLayer setPlayer:m_avPlayer.get()];
    [m_videoLayer setBackgroundColor:cachedCGColor(Color::black, ColorSpaceDeviceRGB)];
#ifndef NDEBUG
    [m_videoLayer setName:@"MediaPlayerPrivate AVPlayerLayer"];
#endif
    [m_videoLayer addObserver:m_objcObserver.get() forKeyPath:@"readyForDisplay" options:NSKeyValueObservingOptionNew context:(void *)MediaPlayerAVFoundationObservationContextAVPlayerLayer];
    updateVideoLayerGravity();
    [m_videoLayer setContentsScale:player()->client().mediaPlayerContentsScale()];
    IntSize defaultSize = player()->client().mediaPlayerContentBoxRect().pixelSnappedSize();
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createVideoLayer(%p) - returning %p", this, m_videoLayer.get());

#if PLATFORM(IOS)
    [m_videoLayer web_disableAllActions];
    m_videoInlineLayer = adoptNS([[WebVideoContainerLayer alloc] init]);
#ifndef NDEBUG
    [m_videoInlineLayer setName:@"WebVideoContainerLayer"];
#endif
    [m_videoInlineLayer setFrame:CGRectMake(0, 0, defaultSize.width(), defaultSize.height())];
    if (m_videoFullscreenLayer) {
        [m_videoLayer setFrame:CGRectMake(0, 0, m_videoFullscreenFrame.width(), m_videoFullscreenFrame.height())];
        [m_videoFullscreenLayer insertSublayer:m_videoLayer.get() atIndex:0];
    } else {
        [m_videoInlineLayer insertSublayer:m_videoLayer.get() atIndex:0];
        [m_videoLayer setFrame:m_videoInlineLayer.get().bounds];
    }
    if ([m_videoLayer respondsToSelector:@selector(setPIPModeEnabled:)])
        [m_videoLayer setPIPModeEnabled:(player()->fullscreenMode() & MediaPlayer::VideoFullscreenModePictureInPicture)];
#else
    [m_videoLayer setFrame:CGRectMake(0, 0, defaultSize.width(), defaultSize.height())];
#endif
}

void MediaPlayerPrivateAVFoundationObjC::destroyVideoLayer()
{
    if (!m_videoLayer)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::destroyVideoLayer(%p) - destroying %p", this, m_videoLayer.get());

    [m_videoLayer.get() removeObserver:m_objcObserver.get() forKeyPath:@"readyForDisplay"];
    [m_videoLayer.get() setPlayer:nil];

#if PLATFORM(IOS)
    if (m_videoFullscreenLayer)
        [m_videoLayer removeFromSuperlayer];
    m_videoInlineLayer = nil;
#endif

    m_videoLayer = nil;
}

MediaTime MediaPlayerPrivateAVFoundationObjC::getStartDate() const
{
    // Date changes as the track's playback position changes. Must subtract currentTime (offset in seconds) from date offset to get date beginning
    double date = [[m_avPlayerItem currentDate] timeIntervalSince1970] * 1000;

    // No live streams were made during the epoch (1970). AVFoundation returns 0 if the media file doesn't have a start date
    if (!date)
        return MediaTime::invalidTime();

    double currentTime = CMTimeGetSeconds([m_avPlayerItem currentTime]) * 1000;

    // Rounding due to second offset error when subtracting.
    return MediaTime::createWithDouble(round(date - currentTime));
}

bool MediaPlayerPrivateAVFoundationObjC::hasAvailableVideoFrame() const
{
    if (currentRenderingMode() == MediaRenderingToLayer)
        return m_cachedIsReadyForDisplay;

    return m_videoFrameHasDrawn;
}

#if ENABLE(AVF_CAPTIONS)
static const NSArray* mediaDescriptionForKind(PlatformTextTrack::TrackKind kind)
{
    static bool manualSelectionMode = MTEnableCaption2015BehaviorPtr() && MTEnableCaption2015BehaviorPtr()();
    if (manualSelectionMode)
        return @[ AVMediaCharacteristicIsAuxiliaryContent ];

    // FIXME: Match these to correct types:
    if (kind == PlatformTextTrack::Caption)
        return [NSArray arrayWithObjects: AVMediaCharacteristicTranscribesSpokenDialogForAccessibility, nil];

    if (kind == PlatformTextTrack::Subtitle)
        return [NSArray arrayWithObjects: AVMediaCharacteristicTranscribesSpokenDialogForAccessibility, nil];

    if (kind == PlatformTextTrack::Description)
        return [NSArray arrayWithObjects: AVMediaCharacteristicTranscribesSpokenDialogForAccessibility, AVMediaCharacteristicDescribesMusicAndSoundForAccessibility, nil];

    if (kind == PlatformTextTrack::Forced)
        return [NSArray arrayWithObjects: AVMediaCharacteristicContainsOnlyForcedSubtitles, nil];

    return [NSArray arrayWithObjects: AVMediaCharacteristicTranscribesSpokenDialogForAccessibility, nil];
}
    
void MediaPlayerPrivateAVFoundationObjC::notifyTrackModeChanged()
{
    trackModeChanged();
}
    
void MediaPlayerPrivateAVFoundationObjC::synchronizeTextTrackState()
{
    const Vector<RefPtr<PlatformTextTrack>>& outOfBandTrackSources = player()->outOfBandTrackSources();
    
    for (auto& textTrack : m_textTracks) {
        if (textTrack->textTrackCategory() != InbandTextTrackPrivateAVF::OutOfBand)
            continue;
        
        RefPtr<OutOfBandTextTrackPrivateAVF> trackPrivate = static_cast<OutOfBandTextTrackPrivateAVF*>(textTrack.get());
        RetainPtr<AVMediaSelectionOptionType> currentOption = trackPrivate->mediaSelectionOption();
        
        for (auto& track : outOfBandTrackSources) {
            RetainPtr<CFStringRef> uniqueID = String::number(track->uniqueId()).createCFString();
            
            if (![[currentOption.get() outOfBandIdentifier] isEqual: reinterpret_cast<const NSString*>(uniqueID.get())])
                continue;
            
            InbandTextTrackPrivate::Mode mode = InbandTextTrackPrivate::Hidden;
            if (track->mode() == PlatformTextTrack::Hidden)
                mode = InbandTextTrackPrivate::Hidden;
            else if (track->mode() == PlatformTextTrack::Disabled)
                mode = InbandTextTrackPrivate::Disabled;
            else if (track->mode() == PlatformTextTrack::Showing)
                mode = InbandTextTrackPrivate::Showing;
            
            textTrack->setMode(mode);
            break;
        }
    }
}
#endif


static NSURL *canonicalURL(const String& url)
{
    NSURL *cocoaURL = URL(ParsedURLString, url);
    if (url.isEmpty())
        return cocoaURL;

    RetainPtr<NSURLRequest> request = adoptNS([[NSURLRequest alloc] initWithURL:cocoaURL]);
    if (!request)
        return cocoaURL;

    NSURLRequest *canonicalRequest = [NSURLProtocol canonicalRequestForRequest:request.get()];
    if (!canonicalRequest)
        return cocoaURL;

    return [canonicalRequest URL];
}

#if PLATFORM(IOS)
static NSHTTPCookie* toNSHTTPCookie(const Cookie& cookie)
{
    RetainPtr<NSMutableDictionary> properties = adoptNS([[NSMutableDictionary alloc] init]);
    [properties setDictionary:@{
        NSHTTPCookieName: cookie.name,
        NSHTTPCookieValue: cookie.value,
        NSHTTPCookieDomain: cookie.domain,
        NSHTTPCookiePath: cookie.path,
        NSHTTPCookieExpires: [NSDate dateWithTimeIntervalSince1970:(cookie.expires / 1000)],
    }];
    if (cookie.secure)
        [properties setObject:@YES forKey:NSHTTPCookieSecure];
    if (cookie.session)
        [properties setObject:@YES forKey:NSHTTPCookieDiscard];

    return [NSHTTPCookie cookieWithProperties:properties.get()];
}
#endif

void MediaPlayerPrivateAVFoundationObjC::createAVAssetForURL(const String& url)
{
    if (m_avAsset)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createAVAssetForURL(%p) - url = %s", this, url.utf8().data());

    setDelayCallbacks(true);

    RetainPtr<NSMutableDictionary> options = adoptNS([[NSMutableDictionary alloc] init]);    

    [options.get() setObject:[NSNumber numberWithInt:AVAssetReferenceRestrictionForbidRemoteReferenceToLocal | AVAssetReferenceRestrictionForbidLocalReferenceToRemote] forKey:AVURLAssetReferenceRestrictionsKey];

    RetainPtr<NSMutableDictionary> headerFields = adoptNS([[NSMutableDictionary alloc] init]);

    String referrer = player()->referrer();
    if (!referrer.isEmpty())
        [headerFields.get() setObject:referrer forKey:@"Referer"];

    String userAgent = player()->userAgent();
    if (!userAgent.isEmpty())
        [headerFields.get() setObject:userAgent forKey:@"User-Agent"];

    if ([headerFields.get() count])
        [options.get() setObject:headerFields.get() forKey:@"AVURLAssetHTTPHeaderFieldsKey"];

    if (player()->doesHaveAttribute("x-itunes-inherit-uri-query-component"))
        [options.get() setObject: [NSNumber numberWithBool: TRUE] forKey: AVURLAssetInheritURIQueryComponentFromReferencingURIKey];

#if PLATFORM(IOS)
    // FIXME: rdar://problem/20354688
    String identifier = player()->sourceApplicationIdentifier();
    if (!identifier.isEmpty() && AVURLAssetClientBundleIdentifierKey)
        [options setObject:identifier forKey:AVURLAssetClientBundleIdentifierKey];
#endif

#if ENABLE(AVF_CAPTIONS)
    const Vector<RefPtr<PlatformTextTrack>>& outOfBandTrackSources = player()->outOfBandTrackSources();
    if (!outOfBandTrackSources.isEmpty()) {
        RetainPtr<NSMutableArray> outOfBandTracks = adoptNS([[NSMutableArray alloc] init]);
        for (auto& trackSource : outOfBandTrackSources) {
            RetainPtr<CFStringRef> label = trackSource->label().createCFString();
            RetainPtr<CFStringRef> language = trackSource->language().createCFString();
            RetainPtr<CFStringRef> uniqueID = String::number(trackSource->uniqueId()).createCFString();
            RetainPtr<CFStringRef> url = trackSource->url().createCFString();
            [outOfBandTracks.get() addObject:@{
                AVOutOfBandAlternateTrackDisplayNameKey: reinterpret_cast<const NSString*>(label.get()),
                AVOutOfBandAlternateTrackExtendedLanguageTagKey: reinterpret_cast<const NSString*>(language.get()),
                AVOutOfBandAlternateTrackIsDefaultKey: trackSource->isDefault() ? @YES : @NO,
                AVOutOfBandAlternateTrackIdentifierKey: reinterpret_cast<const NSString*>(uniqueID.get()),
                AVOutOfBandAlternateTrackSourceKey: reinterpret_cast<const NSString*>(url.get()),
                AVOutOfBandAlternateTrackMediaCharactersticsKey: mediaDescriptionForKind(trackSource->kind()),
            }];
        }

        [options.get() setObject:outOfBandTracks.get() forKey:AVURLAssetOutOfBandAlternateTracksKey];
    }
#endif

#if PLATFORM(IOS)
    String networkInterfaceName = player()->mediaPlayerNetworkInterfaceName();
    if (!networkInterfaceName.isEmpty())
        [options setObject:networkInterfaceName forKey:AVURLAssetBoundNetworkInterfaceName];
#endif

#if PLATFORM(IOS)
    Vector<Cookie> cookies;
    if (player()->getRawCookies(URL(ParsedURLString, url), cookies)) {
        RetainPtr<NSMutableArray> nsCookies = adoptNS([[NSMutableArray alloc] initWithCapacity:cookies.size()]);
        for (auto& cookie : cookies)
            [nsCookies addObject:toNSHTTPCookie(cookie)];

        [options setObject:nsCookies.get() forKey:AVURLAssetHTTPCookiesKey];
    }
#endif

    NSURL *cocoaURL = canonicalURL(url);
    m_avAsset = adoptNS([allocAVURLAssetInstance() initWithURL:cocoaURL options:options.get()]);

#if HAVE(AVFOUNDATION_LOADER_DELEGATE)
    [[m_avAsset.get() resourceLoader] setDelegate:m_loaderDelegate.get() queue:globalLoaderDelegateQueue()];
#endif

    m_haveCheckedPlayability = false;

    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::setAVPlayerItem(AVPlayerItemType *item)
{
    if (!m_avPlayer)
        return;

    if (pthread_main_np()) {
        [m_avPlayer replaceCurrentItemWithPlayerItem:item];
        return;
    }

    RetainPtr<AVPlayerType> strongPlayer = m_avPlayer.get();
    RetainPtr<AVPlayerItemType> strongItem = item;
    dispatch_async(dispatch_get_main_queue(), [strongPlayer, strongItem] {
        [strongPlayer replaceCurrentItemWithPlayerItem:strongItem.get()];
    });
}

void MediaPlayerPrivateAVFoundationObjC::createAVPlayer()
{
    if (m_avPlayer)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createAVPlayer(%p)", this);

    setDelayCallbacks(true);

    m_avPlayer = adoptNS([allocAVPlayerInstance() init]);
    for (NSString *keyName in playerKVOProperties())
        [m_avPlayer.get() addObserver:m_objcObserver.get() forKeyPath:keyName options:NSKeyValueObservingOptionNew context:(void *)MediaPlayerAVFoundationObservationContextPlayer];

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP) && HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
    [m_avPlayer.get() setAppliesMediaSelectionCriteriaAutomatically:NO];
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    updateDisableExternalPlayback();
    [m_avPlayer.get() setAllowsExternalPlayback:m_allowsWirelessVideoPlayback];
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    if (m_shouldPlayToPlaybackTarget)
        setShouldPlayToPlaybackTarget(true);
#endif

    if (player()->client().mediaPlayerIsVideo())
        createAVPlayerLayer();

    if (m_avPlayerItem)
        setAVPlayerItem(m_avPlayerItem.get());

    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::createAVPlayerItem()
{
    if (m_avPlayerItem)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createAVPlayerItem(%p)", this);

    setDelayCallbacks(true);

    // Create the player item so we can load media data. 
    m_avPlayerItem = adoptNS([allocAVPlayerItemInstance() initWithAsset:m_avAsset.get()]);

    [[NSNotificationCenter defaultCenter] addObserver:m_objcObserver.get() selector:@selector(didEnd:) name:AVPlayerItemDidPlayToEndTimeNotification object:m_avPlayerItem.get()];

    NSKeyValueObservingOptions options = NSKeyValueObservingOptionNew | NSKeyValueObservingOptionPrior;
    for (NSString *keyName in itemKVOProperties())
        [m_avPlayerItem.get() addObserver:m_objcObserver.get() forKeyPath:keyName options:options context:(void *)MediaPlayerAVFoundationObservationContextPlayerItem];

    [m_avPlayerItem setAudioTimePitchAlgorithm:(player()->preservesPitch() ? AVAudioTimePitchAlgorithmSpectral : AVAudioTimePitchAlgorithmVarispeed)];

    if (m_avPlayer)
        setAVPlayerItem(m_avPlayerItem.get());

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP) && HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
    const NSTimeInterval legibleOutputAdvanceInterval = 2;

    RetainPtr<NSArray> subtypes = adoptNS([[NSArray alloc] initWithObjects:[NSNumber numberWithUnsignedInt:kCMSubtitleFormatType_WebVTT], nil]);
    m_legibleOutput = adoptNS([allocAVPlayerItemLegibleOutputInstance() initWithMediaSubtypesForNativeRepresentation:subtypes.get()]);
    [m_legibleOutput.get() setSuppressesPlayerRendering:YES];

    [m_legibleOutput.get() setDelegate:m_objcObserver.get() queue:dispatch_get_main_queue()];
    [m_legibleOutput.get() setAdvanceIntervalForDelegateInvocation:legibleOutputAdvanceInterval];
    [m_legibleOutput.get() setTextStylingResolution:AVPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly];
    [m_avPlayerItem.get() addOutput:m_legibleOutput.get()];
#endif

#if ENABLE(WEB_AUDIO) && USE(MEDIATOOLBOX)
    if (m_provider) {
        m_provider->setPlayerItem(m_avPlayerItem.get());
        m_provider->setAudioTrack(firstEnabledTrack(safeAVAssetTracksForAudibleMedia()));
    }
#endif

    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::checkPlayability()
{
    if (m_haveCheckedPlayability)
        return;
    m_haveCheckedPlayability = true;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::checkPlayability(%p)", this);
    auto weakThis = createWeakPtr();

    [m_avAsset.get() loadValuesAsynchronouslyForKeys:[NSArray arrayWithObject:@"playable"] completionHandler:^{
        callOnMainThread([weakThis] {
            if (weakThis)
                weakThis->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::AssetPlayabilityKnown);
        });
    }];
}

void MediaPlayerPrivateAVFoundationObjC::beginLoadingMetadata()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::beginLoadingMetadata(%p) - requesting metadata loading", this);

    dispatch_group_t metadataLoadingGroup = dispatch_group_create();
    dispatch_group_enter(metadataLoadingGroup);
    auto weakThis = createWeakPtr();
    [m_avAsset.get() loadValuesAsynchronouslyForKeys:assetMetadataKeyNames() completionHandler:^{

        callOnMainThread([weakThis, metadataLoadingGroup] {
            if (weakThis && [weakThis->m_avAsset.get() statusOfValueForKey:@"tracks" error:nil] == AVKeyValueStatusLoaded) {
                for (AVAssetTrack *track in [weakThis->m_avAsset.get() tracks]) {
                    dispatch_group_enter(metadataLoadingGroup);
                    [track loadValuesAsynchronouslyForKeys:assetTrackMetadataKeyNames() completionHandler:^{
                        dispatch_group_leave(metadataLoadingGroup);
                    }];
                }
            }
            dispatch_group_leave(metadataLoadingGroup);
        });
    }];

    dispatch_group_notify(metadataLoadingGroup, dispatch_get_main_queue(), ^{
        callOnMainThread([weakThis] {
            if (weakThis)
                [weakThis->m_objcObserver.get() metadataLoaded];
        });

        dispatch_release(metadataLoadingGroup);
    });
}

MediaPlayerPrivateAVFoundation::ItemStatus MediaPlayerPrivateAVFoundationObjC::playerItemStatus() const
{
    if (!m_avPlayerItem)
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusDoesNotExist;

    if (m_cachedItemStatus == AVPlayerItemStatusUnknown)
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusUnknown;
    if (m_cachedItemStatus == AVPlayerItemStatusFailed)
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusFailed;
    if (m_cachedLikelyToKeepUp)
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusPlaybackLikelyToKeepUp;
    if (m_cachedBufferFull)
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusPlaybackBufferFull;
    if (m_cachedBufferEmpty)
        return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusPlaybackBufferEmpty;

    return MediaPlayerPrivateAVFoundation::MediaPlayerAVPlayerItemStatusReadyToPlay;
}

PlatformMedia MediaPlayerPrivateAVFoundationObjC::platformMedia() const
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::platformMedia(%p)", this);
    PlatformMedia pm;
    pm.type = PlatformMedia::AVFoundationMediaPlayerType;
    pm.media.avfMediaPlayer = m_avPlayer.get();
    return pm;
}

PlatformLayer* MediaPlayerPrivateAVFoundationObjC::platformLayer() const
{
#if PLATFORM(IOS)
    return m_haveBeenAskedToCreateLayer ? m_videoInlineLayer.get() : nullptr;
#else
    return m_haveBeenAskedToCreateLayer ? m_videoLayer.get() : nullptr;
#endif
}

#if PLATFORM(IOS)
void MediaPlayerPrivateAVFoundationObjC::setVideoFullscreenLayer(PlatformLayer* videoFullscreenLayer)
{
    if (m_videoFullscreenLayer == videoFullscreenLayer)
        return;

    m_videoFullscreenLayer = videoFullscreenLayer;

    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    
    CAContext *oldContext = [m_videoLayer context];
    CAContext *newContext = nil;
    
    if (m_videoFullscreenLayer && m_videoLayer) {
        [m_videoFullscreenLayer insertSublayer:m_videoLayer.get() atIndex:0];
        [m_videoLayer setFrame:CGRectMake(0, 0, m_videoFullscreenFrame.width(), m_videoFullscreenFrame.height())];
        newContext = [m_videoFullscreenLayer context];
    } else if (m_videoInlineLayer && m_videoLayer) {
        [m_videoLayer setFrame:[m_videoInlineLayer bounds]];
        [m_videoLayer removeFromSuperlayer];
        [m_videoInlineLayer insertSublayer:m_videoLayer.get() atIndex:0];
        newContext = [m_videoInlineLayer context];
    } else if (m_videoLayer)
        [m_videoLayer removeFromSuperlayer];

    if (oldContext && newContext && oldContext != newContext) {
        mach_port_t fencePort = [oldContext createFencePort];
        [newContext setFencePort:fencePort];
        mach_port_deallocate(mach_task_self(), fencePort);
    }
    [CATransaction commit];

    if (m_videoFullscreenLayer && m_textTrackRepresentationLayer) {
        syncTextTrackBounds();
        [m_videoFullscreenLayer addSublayer:m_textTrackRepresentationLayer.get()];
    }

    updateDisableExternalPlayback();
}

void MediaPlayerPrivateAVFoundationObjC::setVideoFullscreenFrame(FloatRect frame)
{
    m_videoFullscreenFrame = frame;
    if (!m_videoFullscreenLayer)
        return;

    if (m_videoLayer) {
        [m_videoLayer setFrame:CGRectMake(0, 0, frame.width(), frame.height())];
    }
    syncTextTrackBounds();
}

void MediaPlayerPrivateAVFoundationObjC::setVideoFullscreenGravity(MediaPlayer::VideoGravity gravity)
{
    m_videoFullscreenGravity = gravity;
    if (!m_videoLayer)
        return;

    NSString *videoGravity = AVLayerVideoGravityResizeAspect;
    if (gravity == MediaPlayer::VideoGravityResize)
        videoGravity = AVLayerVideoGravityResize;
    else if (gravity == MediaPlayer::VideoGravityResizeAspect)
        videoGravity = AVLayerVideoGravityResizeAspect;
    else if (gravity == MediaPlayer::VideoGravityResizeAspectFill)
        videoGravity = AVLayerVideoGravityResizeAspectFill;
    else
        ASSERT_NOT_REACHED();
    
    if ([m_videoLayer videoGravity] == videoGravity)
        return;

    [m_videoLayer setVideoGravity:videoGravity];
    syncTextTrackBounds();
}

void MediaPlayerPrivateAVFoundationObjC::setVideoFullscreenMode(MediaPlayer::VideoFullscreenMode mode)
{
    if (m_videoLayer && [m_videoLayer respondsToSelector:@selector(setPIPModeEnabled:)])
        [m_videoLayer setPIPModeEnabled:(mode & MediaPlayer::VideoFullscreenModePictureInPicture)];
}

NSArray *MediaPlayerPrivateAVFoundationObjC::timedMetadata() const
{
    if (m_currentMetaData)
        return m_currentMetaData.get();
    return nil;
}

String MediaPlayerPrivateAVFoundationObjC::accessLog() const
{
    if (!m_avPlayerItem)
        return emptyString();
    
    AVPlayerItemAccessLog *log = [m_avPlayerItem.get() accessLog];
    RetainPtr<NSString> logString = adoptNS([[NSString alloc] initWithData:[log extendedLogData] encoding:[log extendedLogDataStringEncoding]]);

    return logString.get();
}

String MediaPlayerPrivateAVFoundationObjC::errorLog() const
{
    if (!m_avPlayerItem)
        return emptyString();

    AVPlayerItemErrorLog *log = [m_avPlayerItem.get() errorLog];
    RetainPtr<NSString> logString = adoptNS([[NSString alloc] initWithData:[log extendedLogData] encoding:[log extendedLogDataStringEncoding]]);

    return logString.get();
}
#endif

void MediaPlayerPrivateAVFoundationObjC::platformSetVisible(bool isVisible)
{
    [CATransaction begin];
    [CATransaction setDisableActions:YES];    
    if (m_videoLayer)
        [m_videoLayer.get() setHidden:!isVisible];
    [CATransaction commit];
}
    
void MediaPlayerPrivateAVFoundationObjC::platformPlay()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::platformPlay(%p)", this);
    if (!metaDataAvailable())
        return;

    setDelayCallbacks(true);
    m_cachedRate = requestedRate();
    [m_avPlayer.get() setRate:requestedRate()];
    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::platformPause()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::platformPause(%p)", this);
    if (!metaDataAvailable())
        return;

    setDelayCallbacks(true);
    m_cachedRate = 0;
    [m_avPlayer.get() setRate:0];
    setDelayCallbacks(false);
}

MediaTime MediaPlayerPrivateAVFoundationObjC::platformDuration() const
{
    // Do not ask the asset for duration before it has been loaded or it will fetch the
    // answer synchronously.
    if (!m_avAsset || assetStatus() < MediaPlayerAVAssetStatusLoaded)
        return MediaTime::invalidTime();
    
    CMTime cmDuration;
    
    // Check the AVItem if we have one and it has loaded duration, some assets never report duration.
    if (m_avPlayerItem && playerItemStatus() >= MediaPlayerAVPlayerItemStatusReadyToPlay)
        cmDuration = [m_avPlayerItem.get() duration];
    else
        cmDuration = [m_avAsset.get() duration];

    if (CMTIME_IS_NUMERIC(cmDuration))
        return toMediaTime(cmDuration);

    if (CMTIME_IS_INDEFINITE(cmDuration))
        return MediaTime::positiveInfiniteTime();

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::platformDuration(%p) - invalid duration, returning %s", this, toString(MediaTime::invalidTime()).utf8().data());
    return MediaTime::invalidTime();
}

MediaTime MediaPlayerPrivateAVFoundationObjC::currentMediaTime() const
{
    if (!metaDataAvailable() || !m_avPlayerItem)
        return MediaTime::zeroTime();

    CMTime itemTime = [m_avPlayerItem.get() currentTime];
    if (CMTIME_IS_NUMERIC(itemTime))
        return std::max(toMediaTime(itemTime), MediaTime::zeroTime());

    return MediaTime::zeroTime();
}

void MediaPlayerPrivateAVFoundationObjC::seekToTime(const MediaTime& time, const MediaTime& negativeTolerance, const MediaTime& positiveTolerance)
{
    // setCurrentTime generates several event callbacks, update afterwards.
    setDelayCallbacks(true);

    if (m_metadataTrack)
        m_metadataTrack->flushPartialCues();

    CMTime cmTime = toCMTime(time);
    CMTime cmBefore = toCMTime(negativeTolerance);
    CMTime cmAfter = toCMTime(positiveTolerance);

    auto weakThis = createWeakPtr();

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::seekToTime(%p) - calling seekToTime", this);

    [m_avPlayerItem.get() seekToTime:cmTime toleranceBefore:cmBefore toleranceAfter:cmAfter completionHandler:^(BOOL finished) {
        callOnMainThread([weakThis, finished] {
            auto _this = weakThis.get();
            if (!_this)
                return;

            _this->seekCompleted(finished);
        });
    }];

    setDelayCallbacks(false);
}

void MediaPlayerPrivateAVFoundationObjC::setVolume(float volume)
{
#if PLATFORM(IOS)
    UNUSED_PARAM(volume);
    return;
#else
    if (!metaDataAvailable())
        return;

    [m_avPlayer.get() setVolume:volume];
#endif
}

void MediaPlayerPrivateAVFoundationObjC::setClosedCaptionsVisible(bool closedCaptionsVisible)
{
    UNUSED_PARAM(closedCaptionsVisible);

    if (!metaDataAvailable())
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::setClosedCaptionsVisible(%p) - set to %s", this, boolString(closedCaptionsVisible));
}

void MediaPlayerPrivateAVFoundationObjC::setRateDouble(double rate)
{
    setDelayCallbacks(true);
    m_cachedRate = rate;
    [m_avPlayer.get() setRate:rate];
    setDelayCallbacks(false);
}

double MediaPlayerPrivateAVFoundationObjC::rate() const
{
    if (!metaDataAvailable())
        return 0;

    return m_cachedRate;
}

void MediaPlayerPrivateAVFoundationObjC::setPreservesPitch(bool preservesPitch)
{
    if (m_avPlayerItem)
        [m_avPlayerItem setAudioTimePitchAlgorithm:(preservesPitch ? AVAudioTimePitchAlgorithmSpectral : AVAudioTimePitchAlgorithmVarispeed)];
}

std::unique_ptr<PlatformTimeRanges> MediaPlayerPrivateAVFoundationObjC::platformBufferedTimeRanges() const
{
    auto timeRanges = std::make_unique<PlatformTimeRanges>();

    if (!m_avPlayerItem)
        return timeRanges;

    for (NSValue *thisRangeValue in m_cachedLoadedRanges.get()) {
        CMTimeRange timeRange = [thisRangeValue CMTimeRangeValue];
        if (CMTIMERANGE_IS_VALID(timeRange) && !CMTIMERANGE_IS_EMPTY(timeRange))
            timeRanges->add(toMediaTime(timeRange.start), toMediaTime(CMTimeRangeGetEnd(timeRange)));
    }
    return timeRanges;
}

MediaTime MediaPlayerPrivateAVFoundationObjC::platformMinTimeSeekable() const
{
    if (!m_cachedSeekableRanges || ![m_cachedSeekableRanges count])
        return MediaTime::zeroTime();

    MediaTime minTimeSeekable = MediaTime::positiveInfiniteTime();
    bool hasValidRange = false;
    for (NSValue *thisRangeValue in m_cachedSeekableRanges.get()) {
        CMTimeRange timeRange = [thisRangeValue CMTimeRangeValue];
        if (!CMTIMERANGE_IS_VALID(timeRange) || CMTIMERANGE_IS_EMPTY(timeRange))
            continue;

        hasValidRange = true;
        MediaTime startOfRange = toMediaTime(timeRange.start);
        if (minTimeSeekable > startOfRange)
            minTimeSeekable = startOfRange;
    }
    return hasValidRange ? minTimeSeekable : MediaTime::zeroTime();
}

MediaTime MediaPlayerPrivateAVFoundationObjC::platformMaxTimeSeekable() const
{
    if (!m_cachedSeekableRanges)
        m_cachedSeekableRanges = [m_avPlayerItem seekableTimeRanges];

    MediaTime maxTimeSeekable;
    for (NSValue *thisRangeValue in m_cachedSeekableRanges.get()) {
        CMTimeRange timeRange = [thisRangeValue CMTimeRangeValue];
        if (!CMTIMERANGE_IS_VALID(timeRange) || CMTIMERANGE_IS_EMPTY(timeRange))
            continue;
        
        MediaTime endOfRange = toMediaTime(CMTimeRangeGetEnd(timeRange));
        if (maxTimeSeekable < endOfRange)
            maxTimeSeekable = endOfRange;
    }
    return maxTimeSeekable;
}

MediaTime MediaPlayerPrivateAVFoundationObjC::platformMaxTimeLoaded() const
{
    if (!m_cachedLoadedRanges)
        return MediaTime::zeroTime();

    MediaTime maxTimeLoaded;
    for (NSValue *thisRangeValue in m_cachedLoadedRanges.get()) {
        CMTimeRange timeRange = [thisRangeValue CMTimeRangeValue];
        if (!CMTIMERANGE_IS_VALID(timeRange) || CMTIMERANGE_IS_EMPTY(timeRange))
            continue;
        
        MediaTime endOfRange = toMediaTime(CMTimeRangeGetEnd(timeRange));
        if (maxTimeLoaded < endOfRange)
            maxTimeLoaded = endOfRange;
    }

    return maxTimeLoaded;   
}

unsigned long long MediaPlayerPrivateAVFoundationObjC::totalBytes() const
{
    if (!metaDataAvailable())
        return 0;

    if (m_cachedTotalBytes)
        return m_cachedTotalBytes;

    for (AVPlayerItemTrack *thisTrack in m_cachedTracks.get())
        m_cachedTotalBytes += [[thisTrack assetTrack] totalSampleDataLength];

    return m_cachedTotalBytes;
}

void MediaPlayerPrivateAVFoundationObjC::setAsset(RetainPtr<id> asset)
{
    m_avAsset = asset;
}

MediaPlayerPrivateAVFoundation::AssetStatus MediaPlayerPrivateAVFoundationObjC::assetStatus() const
{
    if (!m_avAsset)
        return MediaPlayerAVAssetStatusDoesNotExist;

    for (NSString *keyName in assetMetadataKeyNames()) {
        NSError *error = nil;
        AVKeyValueStatus keyStatus = [m_avAsset.get() statusOfValueForKey:keyName error:&error];
#if !LOG_DISABLED
        if (error)
            LOG(Media, "MediaPlayerPrivateAVFoundation::assetStatus - statusOfValueForKey failed for %s, error = %s", [keyName UTF8String], [[error localizedDescription] UTF8String]);
#endif

        if (keyStatus < AVKeyValueStatusLoaded)
            return MediaPlayerAVAssetStatusLoading;// At least one key is not loaded yet.
        
        if (keyStatus == AVKeyValueStatusFailed)
            return MediaPlayerAVAssetStatusFailed; // At least one key could not be loaded.

        if (keyStatus == AVKeyValueStatusCancelled)
            return MediaPlayerAVAssetStatusCancelled; // Loading of at least one key was cancelled.
    }

    if ([[m_avAsset.get() valueForKey:@"playable"] boolValue])
        return MediaPlayerAVAssetStatusPlayable;

    return MediaPlayerAVAssetStatusLoaded;
}

long MediaPlayerPrivateAVFoundationObjC::assetErrorCode() const
{
    if (!m_avAsset)
        return 0;

    NSError *error = nil;
    [m_avAsset statusOfValueForKey:@"playable" error:&error];
    return [error code];
}

void MediaPlayerPrivateAVFoundationObjC::paintCurrentFrameInContext(GraphicsContext* context, const FloatRect& rect)
{
    if (!metaDataAvailable() || context->paintingDisabled())
        return;

    setDelayCallbacks(true);
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

#if HAVE(AVFOUNDATION_VIDEO_OUTPUT)
    if (videoOutputHasAvailableFrame())
        paintWithVideoOutput(context, rect);
    else
#endif
        paintWithImageGenerator(context, rect);

    END_BLOCK_OBJC_EXCEPTIONS;
    setDelayCallbacks(false);

    m_videoFrameHasDrawn = true;
}

void MediaPlayerPrivateAVFoundationObjC::paint(GraphicsContext* context, const FloatRect& rect)
{
    if (!metaDataAvailable() || context->paintingDisabled())
        return;

    // We can ignore the request if we are already rendering to a layer.
    if (currentRenderingMode() == MediaRenderingToLayer)
        return;

    // paint() is best effort, so only paint if we already have an image generator or video output available.
    if (!hasContextRenderer())
        return;

    paintCurrentFrameInContext(context, rect);
}

void MediaPlayerPrivateAVFoundationObjC::paintWithImageGenerator(GraphicsContext* context, const FloatRect& rect)
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::paintWithImageGenerator(%p)", this);

    RetainPtr<CGImageRef> image = createImageForTimeInRect(currentTime(), rect);
    if (image) {
        GraphicsContextStateSaver stateSaver(*context);
        context->translate(rect.x(), rect.y() + rect.height());
        context->scale(FloatSize(1.0f, -1.0f));
        context->setImageInterpolationQuality(InterpolationLow);
        IntRect paintRect(IntPoint(0, 0), IntSize(rect.width(), rect.height()));
        CGContextDrawImage(context->platformContext(), CGRectMake(0, 0, paintRect.width(), paintRect.height()), image.get());
        image = 0;
    }
}

static const HashSet<String>& avfMIMETypes()
{
    static NeverDestroyed<HashSet<String>> cache = [] () {
        HashSet<String> types;

        NSArray *nsTypes = [AVURLAsset audiovisualMIMETypes];
        for (NSString *mimeType in nsTypes)
            types.add([mimeType lowercaseString]);

        return types;
    }();

    
    return cache;
}

RetainPtr<CGImageRef> MediaPlayerPrivateAVFoundationObjC::createImageForTimeInRect(float time, const FloatRect& rect)
{
    if (!m_imageGenerator)
        createImageGenerator();
    ASSERT(m_imageGenerator);

#if !LOG_DISABLED
    double start = monotonicallyIncreasingTime();
#endif

    [m_imageGenerator.get() setMaximumSize:CGSize(rect.size())];
    RetainPtr<CGImageRef> rawImage = adoptCF([m_imageGenerator.get() copyCGImageAtTime:CMTimeMakeWithSeconds(time, 600) actualTime:nil error:nil]);
    RetainPtr<CGImageRef> image = adoptCF(CGImageCreateCopyWithColorSpace(rawImage.get(), deviceRGBColorSpaceRef()));

#if !LOG_DISABLED
    double duration = monotonicallyIncreasingTime() - start;
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createImageForTimeInRect(%p) - creating image took %.4f", this, narrowPrecisionToFloat(duration));
#endif

    return image;
}

void MediaPlayerPrivateAVFoundationObjC::getSupportedTypes(HashSet<String>& supportedTypes)
{
    supportedTypes = avfMIMETypes();
} 

#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
static bool keySystemIsSupported(const String& keySystem)
{
    if (equalIgnoringASCIICase(keySystem, "com.apple.fps") || equalIgnoringASCIICase(keySystem, "com.apple.fps.1_0") || equalIgnoringASCIICase(keySystem, "org.w3c.clearkey"))
        return true;
    return false;
}
#endif

MediaPlayer::SupportsType MediaPlayerPrivateAVFoundationObjC::supportsType(const MediaEngineSupportParameters& parameters)
{
#if ENABLE(ENCRYPTED_MEDIA)
    // From: <http://dvcs.w3.org/hg/html-media/raw-file/eme-v0.1b/encrypted-media/encrypted-media.html#dom-canplaytype>
    // In addition to the steps in the current specification, this method must run the following steps:

    // 1. Check whether the Key System is supported with the specified container and codec type(s) by following the steps for the first matching condition from the following list:
    //    If keySystem is null, continue to the next step.
    if (!parameters.keySystem.isNull() && !parameters.keySystem.isEmpty()) {
        // "Clear Key" is only supported with HLS:
        if (equalIgnoringASCIICase(parameters.keySystem, "org.w3c.clearkey") && !parameters.type.isEmpty() && !equalIgnoringASCIICase(parameters.type, "application/x-mpegurl"))
            return MediaPlayer::IsNotSupported;

        // If keySystem contains an unrecognized or unsupported Key System, return the empty string
        if (!keySystemIsSupported(parameters.keySystem))
            return MediaPlayer::IsNotSupported;

        // If the Key System specified by keySystem does not support decrypting the container and/or codec specified in the rest of the type string.
        // (AVFoundation does not provide an API which would allow us to determine this, so this is a no-op)
    }

    // 2. Return "maybe" or "probably" as appropriate per the existing specification of canPlayType().
#endif

#if ENABLE(MEDIA_SOURCE)
    if (parameters.isMediaSource)
        return MediaPlayer::IsNotSupported;
#endif
#if ENABLE(MEDIA_STREAM)
    if (parameters.isMediaStream)
        return MediaPlayer::IsNotSupported;
#endif
    if (isUnsupportedMIMEType(parameters.type))
        return MediaPlayer::IsNotSupported;

    if (!staticMIMETypeList().contains(parameters.type) && !avfMIMETypes().contains(parameters.type))
        return MediaPlayer::IsNotSupported;

    // The spec says:
    // "Implementors are encouraged to return "maybe" unless the type can be confidently established as being supported or not."
    if (parameters.codecs.isEmpty())
        return MediaPlayer::MayBeSupported;

    NSString *typeString = [NSString stringWithFormat:@"%@; codecs=\"%@\"", (NSString *)parameters.type, (NSString *)parameters.codecs];
    return [AVURLAsset isPlayableExtendedMIMEType:typeString] ? MediaPlayer::IsSupported : MediaPlayer::MayBeSupported;
}

bool MediaPlayerPrivateAVFoundationObjC::supportsKeySystem(const String& keySystem, const String& mimeType)
{
#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
    if (!keySystem.isEmpty()) {
        // "Clear Key" is only supported with HLS:
        if (equalIgnoringASCIICase(keySystem, "org.w3c.clearkey") && !mimeType.isEmpty() && !equalIgnoringASCIICase(mimeType, "application/x-mpegurl"))
            return MediaPlayer::IsNotSupported;

        if (!keySystemIsSupported(keySystem))
            return false;

        if (!mimeType.isEmpty() && isUnsupportedMIMEType(mimeType))
            return false;

        if (!mimeType.isEmpty() && !staticMIMETypeList().contains(mimeType) && !avfMIMETypes().contains(mimeType))
            return false;

        return true;
    }
#else
    UNUSED_PARAM(keySystem);
    UNUSED_PARAM(mimeType);
#endif
    return false;
}

#if HAVE(AVFOUNDATION_LOADER_DELEGATE)
#if ENABLE(ENCRYPTED_MEDIA_V2)
static void fulfillRequestWithKeyData(AVAssetResourceLoadingRequest *request, ArrayBuffer* keyData)
{
    if (AVAssetResourceLoadingContentInformationRequest *infoRequest = [request contentInformationRequest]) {
        [infoRequest setContentLength:keyData->byteLength()];
        [infoRequest setByteRangeAccessSupported:YES];
    }

    if (AVAssetResourceLoadingDataRequest *dataRequest = [request dataRequest]) {
        long long start = [dataRequest currentOffset];
        long long end = std::min<long long>(keyData->byteLength(), [dataRequest currentOffset] + [dataRequest requestedLength]);

        if (start < 0 || end < 0 || start >= static_cast<long long>(keyData->byteLength())) {
            [request finishLoadingWithError:nil];
            return;
        }

        ASSERT(start <= std::numeric_limits<int>::max());
        ASSERT(end <= std::numeric_limits<int>::max());
        RefPtr<ArrayBuffer> requestedKeyData = keyData->slice(static_cast<int>(start), static_cast<int>(end));
        RetainPtr<NSData> nsData = adoptNS([[NSData alloc] initWithBytes:requestedKeyData->data() length:requestedKeyData->byteLength()]);
        [dataRequest respondWithData:nsData.get()];
    }

    [request finishLoading];
}
#endif

bool MediaPlayerPrivateAVFoundationObjC::shouldWaitForLoadingOfResource(AVAssetResourceLoadingRequest* avRequest)
{
    String scheme = [[[avRequest request] URL] scheme];
    String keyURI = [[[avRequest request] URL] absoluteString];

#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)
    if (scheme == "skd") {
        // Create an initData with the following layout:
        // [4 bytes: keyURI size], [keyURI size bytes: keyURI]
        unsigned keyURISize = keyURI.length() * sizeof(UChar);
        RefPtr<ArrayBuffer> initDataBuffer = ArrayBuffer::create(4 + keyURISize, 1);
        RefPtr<JSC::DataView> initDataView = JSC::DataView::create(initDataBuffer, 0, initDataBuffer->byteLength());
        initDataView->set<uint32_t>(0, keyURISize, true);

        RefPtr<Uint16Array> keyURIArray = Uint16Array::create(initDataBuffer, 4, keyURI.length());
        keyURIArray->setRange(StringView(keyURI).upconvertedCharacters(), keyURI.length() / sizeof(unsigned char), 0);

#if ENABLE(ENCRYPTED_MEDIA)
        if (!player()->keyNeeded("com.apple.lskd", emptyString(), static_cast<const unsigned char*>(initDataBuffer->data()), initDataBuffer->byteLength()))
#elif ENABLE(ENCRYPTED_MEDIA_V2)
        RefPtr<Uint8Array> initData = Uint8Array::create(initDataBuffer, 0, initDataBuffer->byteLength());
        if (!player()->keyNeeded(initData.get()))
#endif
            return false;

        m_keyURIToRequestMap.set(keyURI, avRequest);
        return true;
#if ENABLE(ENCRYPTED_MEDIA_V2)
    } else if (scheme == "clearkey") {
        String keyID = [[[avRequest request] URL] resourceSpecifier];
        StringView keyIDView(keyID);
        CString utf8EncodedKeyId = UTF8Encoding().encode(keyIDView, URLEncodedEntitiesForUnencodables);

        RefPtr<Uint8Array> initData = Uint8Array::create(utf8EncodedKeyId.length());
        initData->setRange((JSC::Uint8Adaptor::Type*)utf8EncodedKeyId.data(), utf8EncodedKeyId.length(), 0);

        auto keyData = player()->cachedKeyForKeyId(keyID);
        if (keyData) {
            fulfillRequestWithKeyData(avRequest, keyData.get());
            return false;
        }

        if (!player()->keyNeeded(initData.get()))
            return false;

        m_keyURIToRequestMap.set(keyID, avRequest);
        return true;
#endif
    }
#endif

    RefPtr<WebCoreAVFResourceLoader> resourceLoader = WebCoreAVFResourceLoader::create(this, avRequest);
    m_resourceLoaderMap.add(avRequest, resourceLoader);
    resourceLoader->startLoading();
    return true;
}

bool MediaPlayerPrivateAVFoundationObjC::shouldWaitForResponseToAuthenticationChallenge(NSURLAuthenticationChallenge* nsChallenge)
{
#if USE(CFNETWORK)
    RefPtr<WebCoreNSURLAuthenticationChallengeClient> client = WebCoreNSURLAuthenticationChallengeClient::create(nsChallenge);
    RetainPtr<CFURLAuthChallengeRef> cfChallenge = adoptCF([nsChallenge _createCFAuthChallenge]);
    AuthenticationChallenge challenge(cfChallenge.get(), client.get());
#else
    AuthenticationChallenge challenge(nsChallenge);
#endif

    return player()->shouldWaitForResponseToAuthenticationChallenge(challenge);
}

void MediaPlayerPrivateAVFoundationObjC::didCancelLoadingRequest(AVAssetResourceLoadingRequest* avRequest)
{
    String scheme = [[[avRequest request] URL] scheme];

    WebCoreAVFResourceLoader* resourceLoader = m_resourceLoaderMap.get(avRequest);

    if (resourceLoader)
        resourceLoader->stopLoading();
}

void MediaPlayerPrivateAVFoundationObjC::didStopLoadingRequest(AVAssetResourceLoadingRequest *avRequest)
{
    m_resourceLoaderMap.remove(avRequest);
}
#endif

bool MediaPlayerPrivateAVFoundationObjC::isAvailable()
{
    return AVFoundationLibrary() && isCoreMediaFrameworkAvailable();
}

MediaTime MediaPlayerPrivateAVFoundationObjC::mediaTimeForTimeValue(const MediaTime& timeValue) const
{
    if (!metaDataAvailable())
        return timeValue;

    // FIXME - impossible to implement until rdar://8721510 is fixed.
    return timeValue;
}

double MediaPlayerPrivateAVFoundationObjC::maximumDurationToCacheMediaTime() const
{
#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1010
    return 0;
#else
    return 5;
#endif
}

void MediaPlayerPrivateAVFoundationObjC::updateVideoLayerGravity()
{
    if (!m_videoLayer)
        return;

#if PLATFORM(IOS)
    // Do not attempt to change the video gravity while in full screen mode.
    // See setVideoFullscreenGravity().
    if (m_videoFullscreenLayer)
        return;
#endif

    [CATransaction begin];
    [CATransaction setDisableActions:YES];    
    NSString* gravity = shouldMaintainAspectRatio() ? AVLayerVideoGravityResizeAspect : AVLayerVideoGravityResize;
    [m_videoLayer.get() setVideoGravity:gravity];
    [CATransaction commit];
}

static AVAssetTrack* firstEnabledTrack(NSArray* tracks)
{
    NSUInteger index = [tracks indexOfObjectPassingTest:^(id obj, NSUInteger, BOOL *) {
        return [static_cast<AVAssetTrack*>(obj) isEnabled];
    }];
    if (index == NSNotFound)
        return nil;
    return [tracks objectAtIndex:index];
}

void MediaPlayerPrivateAVFoundationObjC::tracksChanged()
{
    String primaryAudioTrackLanguage = m_languageOfPrimaryAudioTrack;
    m_languageOfPrimaryAudioTrack = String();

    if (!m_avAsset)
        return;

    setDelayCharacteristicsChangedNotification(true);

    bool haveCCTrack = false;
    bool hasCaptions = false;

    // This is called whenever the tracks collection changes so cache hasVideo and hasAudio since we are
    // asked about those fairly fequently.
    if (!m_avPlayerItem) {
        // We don't have a player item yet, so check with the asset because some assets support inspection
        // prior to becoming ready to play.
        AVAssetTrack* firstEnabledVideoTrack = firstEnabledTrack([m_avAsset.get() tracksWithMediaCharacteristic:AVMediaCharacteristicVisual]);
        setHasVideo(firstEnabledVideoTrack);
        setHasAudio(firstEnabledTrack([m_avAsset.get() tracksWithMediaCharacteristic:AVMediaCharacteristicAudible]));
#if !HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
        hasCaptions = [[m_avAsset.get() tracksWithMediaType:AVMediaTypeClosedCaption] count];
#endif

        presentationSizeDidChange(firstEnabledVideoTrack ? FloatSize(CGSizeApplyAffineTransform([firstEnabledVideoTrack naturalSize], [firstEnabledVideoTrack preferredTransform])) : FloatSize());
    } else {
        bool hasVideo = false;
        bool hasAudio = false;
        bool hasMetaData = false;
        for (AVPlayerItemTrack *track in m_cachedTracks.get()) {
            if ([track isEnabled]) {
                AVAssetTrack *assetTrack = [track assetTrack];
                NSString *mediaType = [assetTrack mediaType];
                if ([mediaType isEqualToString:AVMediaTypeVideo])
                    hasVideo = true;
                else if ([mediaType isEqualToString:AVMediaTypeAudio])
                    hasAudio = true;
                else if ([mediaType isEqualToString:AVMediaTypeClosedCaption]) {
#if !HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
                    hasCaptions = true;
#endif
                    haveCCTrack = true;
                } else if ([mediaType isEqualToString:AVMediaTypeMetadata]) {
                    hasMetaData = true;
                }
            }
        }

#if ENABLE(VIDEO_TRACK)
        updateAudioTracks();
        updateVideoTracks();

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
        hasAudio |= (m_audibleGroup && m_audibleGroup->selectedOption());
        hasVideo |= (m_visualGroup && m_visualGroup->selectedOption());
#endif
#endif

        // Always says we have video if the AVPlayerLayer is ready for diaplay to work around
        // an AVFoundation bug which causes it to sometimes claim a track is disabled even
        // when it is not.
        setHasVideo(hasVideo || m_cachedIsReadyForDisplay);

        setHasAudio(hasAudio);
#if ENABLE(DATACUE_VALUE)
        if (hasMetaData)
            processMetadataTrack();
#endif
    }

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
    AVMediaSelectionGroupType *legibleGroup = safeMediaSelectionGroupForLegibleMedia();
    if (legibleGroup && m_cachedTracks) {
        hasCaptions = [[AVMediaSelectionGroup playableMediaSelectionOptionsFromArray:[legibleGroup options]] count];
        if (hasCaptions)
            processMediaSelectionOptions();
    }
#endif

#if !HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT) && HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
    if (!hasCaptions && haveCCTrack)
        processLegacyClosedCaptionsTracks();
#elif !HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
    if (haveCCTrack)
        processLegacyClosedCaptionsTracks();
#endif

    setHasClosedCaptions(hasCaptions);

    LOG(Media, "MediaPlayerPrivateAVFoundation:tracksChanged(%p) - hasVideo = %s, hasAudio = %s, hasCaptions = %s",
        this, boolString(hasVideo()), boolString(hasAudio()), boolString(hasClosedCaptions()));

    sizeChanged();

    if (primaryAudioTrackLanguage != languageOfPrimaryAudioTrack())
        characteristicsChanged();

#if ENABLE(WEB_AUDIO) && USE(MEDIATOOLBOX)
    if (m_provider)
        m_provider->setAudioTrack(firstEnabledTrack(safeAVAssetTracksForAudibleMedia()));
#endif

    setDelayCharacteristicsChangedNotification(false);
}

#if ENABLE(VIDEO_TRACK)
template <typename RefT, typename PassRefT>
void determineChangedTracksFromNewTracksAndOldItems(NSArray* tracks, NSString* trackType, Vector<RefT>& oldItems, RefT (*itemFactory)(AVPlayerItemTrack*), MediaPlayer* player, void (MediaPlayer::*removedFunction)(PassRefT), void (MediaPlayer::*addedFunction)(PassRefT))
{
    RetainPtr<NSSet> newTracks = adoptNS([[NSSet alloc] initWithArray:[tracks objectsAtIndexes:[tracks indexesOfObjectsPassingTest:^(id track, NSUInteger, BOOL*){
        return [[[track assetTrack] mediaType] isEqualToString:trackType];
    }]]]);
    RetainPtr<NSMutableSet> oldTracks = adoptNS([[NSMutableSet alloc] initWithCapacity:oldItems.size()]);

    for (auto& oldItem : oldItems) {
        if (oldItem->playerItemTrack())
            [oldTracks addObject:oldItem->playerItemTrack()];
    }

    // Find the added & removed AVPlayerItemTracks:
    RetainPtr<NSMutableSet> removedTracks = adoptNS([oldTracks mutableCopy]);
    [removedTracks minusSet:newTracks.get()];

    RetainPtr<NSMutableSet> addedTracks = adoptNS([newTracks mutableCopy]);
    [addedTracks minusSet:oldTracks.get()];

    typedef Vector<RefT> ItemVector;
    ItemVector replacementItems;
    ItemVector addedItems;
    ItemVector removedItems;
    for (auto& oldItem : oldItems) {
        if (oldItem->playerItemTrack() && [removedTracks containsObject:oldItem->playerItemTrack()])
            removedItems.append(oldItem);
        else
            replacementItems.append(oldItem);
    }

    for (AVPlayerItemTrack* track in addedTracks.get())
        addedItems.append(itemFactory(track));

    replacementItems.appendVector(addedItems);
    oldItems.swap(replacementItems);

    for (auto& removedItem : removedItems)
        (player->*removedFunction)(removedItem);

    for (auto& addedItem : addedItems)
        (player->*addedFunction)(addedItem);
}

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
template <typename RefT, typename PassRefT>
void determineChangedTracksFromNewTracksAndOldItems(MediaSelectionGroupAVFObjC* group, Vector<RefT>& oldItems, const Vector<String>& characteristics, RefT (*itemFactory)(MediaSelectionOptionAVFObjC&), MediaPlayer* player, void (MediaPlayer::*removedFunction)(PassRefT), void (MediaPlayer::*addedFunction)(PassRefT))
{
    group->updateOptions(characteristics);

    // Only add selection options which do not have an associated persistant track.
    ListHashSet<RefPtr<MediaSelectionOptionAVFObjC>> newSelectionOptions;
    for (auto& option : group->options()) {
        if (!option)
            continue;
        AVMediaSelectionOptionType* avOption = option->avMediaSelectionOption();
        if (!avOption)
            continue;
        if (![avOption respondsToSelector:@selector(track)] || ![avOption performSelector:@selector(track)])
            newSelectionOptions.add(option);
    }

    ListHashSet<RefPtr<MediaSelectionOptionAVFObjC>> oldSelectionOptions;
    for (auto& oldItem : oldItems) {
        if (MediaSelectionOptionAVFObjC *option = oldItem->mediaSelectionOption())
            oldSelectionOptions.add(option);
    }

    // Find the added & removed AVMediaSelectionOptions:
    ListHashSet<RefPtr<MediaSelectionOptionAVFObjC>> removedSelectionOptions;
    for (auto& oldOption : oldSelectionOptions) {
        if (!newSelectionOptions.contains(oldOption))
            removedSelectionOptions.add(oldOption);
    }

    ListHashSet<RefPtr<MediaSelectionOptionAVFObjC>> addedSelectionOptions;
    for (auto& newOption : newSelectionOptions) {
        if (!oldSelectionOptions.contains(newOption))
            addedSelectionOptions.add(newOption);
    }

    typedef Vector<RefT> ItemVector;
    ItemVector replacementItems;
    ItemVector addedItems;
    ItemVector removedItems;
    for (auto& oldItem : oldItems) {
        if (oldItem->mediaSelectionOption() && removedSelectionOptions.contains(oldItem->mediaSelectionOption()))
            removedItems.append(oldItem);
        else
            replacementItems.append(oldItem);
    }

    for (auto& option : addedSelectionOptions)
        addedItems.append(itemFactory(*option.get()));

    replacementItems.appendVector(addedItems);
    oldItems.swap(replacementItems);
    
    for (auto& removedItem : removedItems)
        (player->*removedFunction)(removedItem);
    
    for (auto& addedItem : addedItems)
        (player->*addedFunction)(addedItem);
}
#endif

void MediaPlayerPrivateAVFoundationObjC::updateAudioTracks()
{
#if !LOG_DISABLED
    size_t count = m_audioTracks.size();
#endif

    determineChangedTracksFromNewTracksAndOldItems(m_cachedTracks.get(), AVMediaTypeAudio, m_audioTracks, &AudioTrackPrivateAVFObjC::create, player(), &MediaPlayer::removeAudioTrack, &MediaPlayer::addAudioTrack);

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
    Vector<String> characteristics = player()->preferredAudioCharacteristics();
    if (!m_audibleGroup) {
        if (AVMediaSelectionGroupType *group = safeMediaSelectionGroupForAudibleMedia())
            m_audibleGroup = MediaSelectionGroupAVFObjC::create(m_avPlayerItem.get(), group, characteristics);
    }

    if (m_audibleGroup)
        determineChangedTracksFromNewTracksAndOldItems(m_audibleGroup.get(), m_audioTracks, characteristics, &AudioTrackPrivateAVFObjC::create, player(), &MediaPlayer::removeAudioTrack, &MediaPlayer::addAudioTrack);
#endif

    for (auto& track : m_audioTracks)
        track->resetPropertiesFromTrack();

#if !LOG_DISABLED
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::updateAudioTracks(%p) - audio track count was %lu, is %lu", this, count, m_audioTracks.size());
#endif
}

void MediaPlayerPrivateAVFoundationObjC::updateVideoTracks()
{
#if !LOG_DISABLED
    size_t count = m_videoTracks.size();
#endif

    determineChangedTracksFromNewTracksAndOldItems(m_cachedTracks.get(), AVMediaTypeVideo, m_videoTracks, &VideoTrackPrivateAVFObjC::create, player(), &MediaPlayer::removeVideoTrack, &MediaPlayer::addVideoTrack);

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
    if (!m_visualGroup) {
        if (AVMediaSelectionGroupType *group = safeMediaSelectionGroupForVisualMedia())
            m_visualGroup = MediaSelectionGroupAVFObjC::create(m_avPlayerItem.get(), group, Vector<String>());
    }

    if (m_visualGroup)
        determineChangedTracksFromNewTracksAndOldItems(m_visualGroup.get(), m_videoTracks, Vector<String>(), &VideoTrackPrivateAVFObjC::create, player(), &MediaPlayer::removeVideoTrack, &MediaPlayer::addVideoTrack);
#endif

    for (auto& track : m_audioTracks)
        track->resetPropertiesFromTrack();

#if !LOG_DISABLED
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::updateVideoTracks(%p) - video track count was %lu, is %lu", this, count, m_videoTracks.size());
#endif
}

bool MediaPlayerPrivateAVFoundationObjC::requiresTextTrackRepresentation() const
{
#if PLATFORM(IOS)
    if (m_videoFullscreenLayer)
        return true;
#endif
    return false;
}

void MediaPlayerPrivateAVFoundationObjC::syncTextTrackBounds()
{
#if PLATFORM(IOS)
    if (!m_videoFullscreenLayer || !m_textTrackRepresentationLayer)
        return;
    
    CGRect textFrame = m_videoLayer ? [m_videoLayer videoRect] : CGRectMake(0, 0, m_videoFullscreenFrame.width(), m_videoFullscreenFrame.height());
    [m_textTrackRepresentationLayer setFrame:textFrame];
#endif
}

void MediaPlayerPrivateAVFoundationObjC::setTextTrackRepresentation(TextTrackRepresentation* representation)
{
#if PLATFORM(IOS)
    PlatformLayer* representationLayer = representation ? representation->platformLayer() : nil;
    if (representationLayer == m_textTrackRepresentationLayer) {
        syncTextTrackBounds();
        return;
    }

    if (m_textTrackRepresentationLayer)
        [m_textTrackRepresentationLayer removeFromSuperlayer];

    m_textTrackRepresentationLayer = representationLayer;

    if (m_videoFullscreenLayer && m_textTrackRepresentationLayer) {
        syncTextTrackBounds();
        [m_videoFullscreenLayer addSublayer:m_textTrackRepresentationLayer.get()];
    }

#else
    UNUSED_PARAM(representation);
#endif
}
#endif // ENABLE(VIDEO_TRACK)

#if ENABLE(WEB_AUDIO) && USE(MEDIATOOLBOX)
AudioSourceProvider* MediaPlayerPrivateAVFoundationObjC::audioSourceProvider()
{
    if (!m_provider) {
        m_provider = AudioSourceProviderAVFObjC::create(m_avPlayerItem.get());
        m_provider->setAudioTrack(firstEnabledTrack(safeAVAssetTracksForAudibleMedia()));
    }

    return m_provider.get();
}
#endif

void MediaPlayerPrivateAVFoundationObjC::sizeChanged()
{
    if (!m_avAsset)
        return;

    setNaturalSize(m_cachedPresentationSize);
}
    
bool MediaPlayerPrivateAVFoundationObjC::hasSingleSecurityOrigin() const 
{
    if (!m_avAsset || [m_avAsset statusOfValueForKey:@"resolvedURL" error:nullptr] != AVKeyValueStatusLoaded)
        return false;
    
    Ref<SecurityOrigin> resolvedOrigin(SecurityOrigin::create(resolvedURL()));
    Ref<SecurityOrigin> requestedOrigin(SecurityOrigin::createFromString(assetURL()));
    return resolvedOrigin.get().isSameSchemeHostPort(&requestedOrigin.get());
}

#if HAVE(AVFOUNDATION_VIDEO_OUTPUT)
void MediaPlayerPrivateAVFoundationObjC::createVideoOutput()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createVideoOutput(%p)", this);

    if (!m_avPlayerItem || m_videoOutput)
        return;

#if USE(VIDEOTOOLBOX) && (!defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000)
    NSDictionary* attributes = nil;
#else
    NSDictionary* attributes = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithUnsignedInt:kCVPixelFormatType_32BGRA], kCVPixelBufferPixelFormatTypeKey,
                                nil];
#endif
    m_videoOutput = adoptNS([allocAVPlayerItemVideoOutputInstance() initWithPixelBufferAttributes:attributes]);
    ASSERT(m_videoOutput);

    [m_videoOutput setDelegate:m_videoOutputDelegate.get() queue:globalPullDelegateQueue()];

    [m_avPlayerItem.get() addOutput:m_videoOutput.get()];

#if defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101000
    waitForVideoOutputMediaDataWillChange();
#endif

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createVideoOutput(%p) - returning %p", this, m_videoOutput.get());
}

void MediaPlayerPrivateAVFoundationObjC::destroyVideoOutput()
{
    if (!m_videoOutput)
        return;

    if (m_avPlayerItem)
        [m_avPlayerItem.get() removeOutput:m_videoOutput.get()];
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::destroyVideoOutput(%p) - destroying  %p", this, m_videoOutput.get());

    m_videoOutput = 0;
}

RetainPtr<CVPixelBufferRef> MediaPlayerPrivateAVFoundationObjC::createPixelBuffer()
{
    if (!m_videoOutput)
        createVideoOutput();
    ASSERT(m_videoOutput);

#if !LOG_DISABLED
    double start = monotonicallyIncreasingTime();
#endif

    CMTime currentTime = [m_avPlayerItem.get() currentTime];

    if (![m_videoOutput.get() hasNewPixelBufferForItemTime:currentTime])
        return 0;

    RetainPtr<CVPixelBufferRef> buffer = adoptCF([m_videoOutput.get() copyPixelBufferForItemTime:currentTime itemTimeForDisplay:nil]);
    if (!buffer)
        return 0;

#if USE(VIDEOTOOLBOX)
    // Create a VTPixelTransferSession, if necessary, as we cannot guarantee timely delivery of ARGB pixels.
    if (!m_pixelTransferSession) {
        VTPixelTransferSessionRef session = 0;
        VTPixelTransferSessionCreate(kCFAllocatorDefault, &session);
        m_pixelTransferSession = adoptCF(session);
    }

    CVPixelBufferRef outputBuffer;
    CVPixelBufferCreate(kCFAllocatorDefault, CVPixelBufferGetWidth(buffer.get()), CVPixelBufferGetHeight(buffer.get()), kCVPixelFormatType_32BGRA, 0, &outputBuffer);
    VTPixelTransferSessionTransferImage(m_pixelTransferSession.get(), buffer.get(), outputBuffer);
    buffer = adoptCF(outputBuffer);
#endif

#if !LOG_DISABLED
    double duration = monotonicallyIncreasingTime() - start;
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::createPixelBuffer(%p) - creating buffer took %.4f", this, narrowPrecisionToFloat(duration));
#endif

    return buffer;
}

bool MediaPlayerPrivateAVFoundationObjC::videoOutputHasAvailableFrame()
{
    if (!m_avPlayerItem)
        return false;

    if (m_lastImage)
        return true;

    if (!m_videoOutput)
        createVideoOutput();

    return [m_videoOutput hasNewPixelBufferForItemTime:[m_avPlayerItem currentTime]];
}

static const void* CVPixelBufferGetBytePointerCallback(void* info)
{
    CVPixelBufferRef pixelBuffer = static_cast<CVPixelBufferRef>(info);
    CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
    return CVPixelBufferGetBaseAddress(pixelBuffer);
}

static void CVPixelBufferReleaseBytePointerCallback(void* info, const void*)
{
    CVPixelBufferRef pixelBuffer = static_cast<CVPixelBufferRef>(info);
    CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
}

static void CVPixelBufferReleaseInfoCallback(void* info)
{
    CVPixelBufferRef pixelBuffer = static_cast<CVPixelBufferRef>(info);
    CFRelease(pixelBuffer);
}

static RetainPtr<CGImageRef> createImageFromPixelBuffer(CVPixelBufferRef pixelBuffer)
{
    // pixelBuffer will be of type kCVPixelFormatType_32BGRA.
    ASSERT(CVPixelBufferGetPixelFormatType(pixelBuffer) == kCVPixelFormatType_32BGRA);

    size_t width = CVPixelBufferGetWidth(pixelBuffer);
    size_t height = CVPixelBufferGetHeight(pixelBuffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);
    size_t byteLength = CVPixelBufferGetDataSize(pixelBuffer);
    CGBitmapInfo bitmapInfo = kCGBitmapByteOrder32Little | kCGImageAlphaFirst;

    CFRetain(pixelBuffer); // Balanced by CVPixelBufferReleaseInfoCallback in providerCallbacks.
    CGDataProviderDirectCallbacks providerCallbacks = { 0, CVPixelBufferGetBytePointerCallback, CVPixelBufferReleaseBytePointerCallback, 0, CVPixelBufferReleaseInfoCallback };
    RetainPtr<CGDataProviderRef> provider = adoptCF(CGDataProviderCreateDirect(pixelBuffer, byteLength, &providerCallbacks));

    return adoptCF(CGImageCreate(width, height, 8, 32, bytesPerRow, deviceRGBColorSpaceRef(), bitmapInfo, provider.get(), NULL, false, kCGRenderingIntentDefault));
}

void MediaPlayerPrivateAVFoundationObjC::updateLastImage()
{
    RetainPtr<CVPixelBufferRef> pixelBuffer = createPixelBuffer();

    // Calls to copyPixelBufferForItemTime:itemTimeForDisplay: may return nil if the pixel buffer
    // for the requested time has already been retrieved. In this case, the last valid image (if any)
    // should be displayed.
    if (pixelBuffer)
        m_lastImage = createImageFromPixelBuffer(pixelBuffer.get());
}

void MediaPlayerPrivateAVFoundationObjC::paintWithVideoOutput(GraphicsContext* context, const FloatRect& outputRect)
{
#if (!defined(__MAC_OS_X_VERSION_MIN_REQUIRED) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000)
    if (m_videoOutput && !m_lastImage && !videoOutputHasAvailableFrame())
        waitForVideoOutputMediaDataWillChange();
#endif

    updateLastImage();

    if (!m_lastImage)
        return;

    AVAssetTrack* firstEnabledVideoTrack = firstEnabledTrack([m_avAsset.get() tracksWithMediaCharacteristic:AVMediaCharacteristicVisual]);
    if (!firstEnabledVideoTrack)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::paintWithVideoOutput(%p)", this);

    GraphicsContextStateSaver stateSaver(*context);
    FloatRect imageRect(0, 0, CGImageGetWidth(m_lastImage.get()), CGImageGetHeight(m_lastImage.get()));
    AffineTransform videoTransform = [firstEnabledVideoTrack preferredTransform];
    FloatRect transformedOutputRect = videoTransform.inverse().mapRect(outputRect);

    context->concatCTM(videoTransform);
    context->drawNativeImage(m_lastImage.get(), imageRect.size(), ColorSpaceDeviceRGB, transformedOutputRect, imageRect);

    // If we have created an AVAssetImageGenerator in the past due to m_videoOutput not having an available
    // video frame, destroy it now that it is no longer needed.
    if (m_imageGenerator)
        destroyImageGenerator();

}

PassNativeImagePtr MediaPlayerPrivateAVFoundationObjC::nativeImageForCurrentTime()
{
    updateLastImage();
    return m_lastImage.get();
}

void MediaPlayerPrivateAVFoundationObjC::waitForVideoOutputMediaDataWillChange()
{
    if (!m_videoOutputSemaphore)
        m_videoOutputSemaphore = dispatch_semaphore_create(0);

    [m_videoOutput requestNotificationOfMediaDataChangeWithAdvanceInterval:0];

    // Wait for 1 second.
    long result = dispatch_semaphore_wait(m_videoOutputSemaphore, dispatch_time(0, 1 * NSEC_PER_SEC));

    if (result)
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::waitForVideoOutputMediaDataWillChange(%p) timed out", this);
}

void MediaPlayerPrivateAVFoundationObjC::outputMediaDataWillChange(AVPlayerItemVideoOutput*)
{
    dispatch_semaphore_signal(m_videoOutputSemaphore);
}
#endif

#if ENABLE(ENCRYPTED_MEDIA)
MediaPlayer::MediaKeyException MediaPlayerPrivateAVFoundationObjC::generateKeyRequest(const String& keySystem, const unsigned char* initDataPtr, unsigned initDataLength)
{
    if (!keySystemIsSupported(keySystem))
        return MediaPlayer::KeySystemNotSupported;

    RefPtr<Uint8Array> initData = Uint8Array::create(initDataPtr, initDataLength);
    String keyURI;
    String keyID;
    RefPtr<Uint8Array> certificate;
    if (!extractKeyURIKeyIDAndCertificateFromInitData(initData.get(), keyURI, keyID, certificate))
        return MediaPlayer::InvalidPlayerState;

    if (!m_keyURIToRequestMap.contains(keyURI))
        return MediaPlayer::InvalidPlayerState;

    String sessionID = createCanonicalUUIDString();

    RetainPtr<AVAssetResourceLoadingRequest> avRequest = m_keyURIToRequestMap.get(keyURI);

    RetainPtr<NSData> certificateData = adoptNS([[NSData alloc] initWithBytes:certificate->baseAddress() length:certificate->byteLength()]);
    NSString* assetStr = keyID;
    RetainPtr<NSData> assetID = [NSData dataWithBytes: [assetStr cStringUsingEncoding:NSUTF8StringEncoding] length:[assetStr lengthOfBytesUsingEncoding:NSUTF8StringEncoding]];
    NSError* error = 0;
    RetainPtr<NSData> keyRequest = [avRequest.get() streamingContentKeyRequestDataForApp:certificateData.get() contentIdentifier:assetID.get() options:nil error:&error];

    if (!keyRequest) {
        NSError* underlyingError = [[error userInfo] objectForKey:NSUnderlyingErrorKey];
        player()->keyError(keySystem, sessionID, MediaPlayerClient::DomainError, [underlyingError code]);
        return MediaPlayer::NoError;
    }

    RefPtr<ArrayBuffer> keyRequestBuffer = ArrayBuffer::create([keyRequest.get() bytes], [keyRequest.get() length]);
    RefPtr<Uint8Array> keyRequestArray = Uint8Array::create(keyRequestBuffer, 0, keyRequestBuffer->byteLength());
    player()->keyMessage(keySystem, sessionID, keyRequestArray->data(), keyRequestArray->byteLength(), URL());

    // Move ownership of the AVAssetResourceLoadingRequestfrom the keyIDToRequestMap to the sessionIDToRequestMap:
    m_sessionIDToRequestMap.set(sessionID, avRequest);
    m_keyURIToRequestMap.remove(keyURI);

    return MediaPlayer::NoError;
}

MediaPlayer::MediaKeyException MediaPlayerPrivateAVFoundationObjC::addKey(const String& keySystem, const unsigned char* keyPtr, unsigned keyLength, const unsigned char* initDataPtr, unsigned initDataLength, const String& sessionID)
{
    if (!keySystemIsSupported(keySystem))
        return MediaPlayer::KeySystemNotSupported;

    if (!m_sessionIDToRequestMap.contains(sessionID))
        return MediaPlayer::InvalidPlayerState;

    RetainPtr<AVAssetResourceLoadingRequest> avRequest = m_sessionIDToRequestMap.get(sessionID);
    RetainPtr<NSData> keyData = adoptNS([[NSData alloc] initWithBytes:keyPtr length:keyLength]);
    [[avRequest.get() dataRequest] respondWithData:keyData.get()];
    [avRequest.get() finishLoading];
    m_sessionIDToRequestMap.remove(sessionID);

    player()->keyAdded(keySystem, sessionID);

    UNUSED_PARAM(initDataPtr);
    UNUSED_PARAM(initDataLength);
    return MediaPlayer::NoError;
}

MediaPlayer::MediaKeyException MediaPlayerPrivateAVFoundationObjC::cancelKeyRequest(const String& keySystem, const String& sessionID)
{
    if (!keySystemIsSupported(keySystem))
        return MediaPlayer::KeySystemNotSupported;

    if (!m_sessionIDToRequestMap.contains(sessionID))
        return MediaPlayer::InvalidPlayerState;

    m_sessionIDToRequestMap.remove(sessionID);
    return MediaPlayer::NoError;
}
#endif

#if ENABLE(ENCRYPTED_MEDIA_V2)
RetainPtr<AVAssetResourceLoadingRequest> MediaPlayerPrivateAVFoundationObjC::takeRequestForKeyURI(const String& keyURI)
{
    return m_keyURIToRequestMap.take(keyURI);
}

void MediaPlayerPrivateAVFoundationObjC::keyAdded()
{
    Vector<String> fulfilledKeyIds;

    for (auto& pair : m_keyURIToRequestMap) {
        const String& keyId = pair.key;
        const RetainPtr<AVAssetResourceLoadingRequest>& request = pair.value;

        auto keyData = player()->cachedKeyForKeyId(keyId);
        if (!keyData)
            continue;

        fulfillRequestWithKeyData(request.get(), keyData.get());
        fulfilledKeyIds.append(keyId);
    }

    for (auto& keyId : fulfilledKeyIds)
        m_keyURIToRequestMap.remove(keyId);
}

std::unique_ptr<CDMSession> MediaPlayerPrivateAVFoundationObjC::createSession(const String& keySystem, CDMSessionClient* client)
{
    if (!keySystemIsSupported(keySystem))
        return nullptr;

    return std::make_unique<CDMSessionAVFoundationObjC>(this, client);
}
#endif

#if !HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT)
void MediaPlayerPrivateAVFoundationObjC::processLegacyClosedCaptionsTracks()
{
#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
    [m_avPlayerItem.get() selectMediaOption:nil inMediaSelectionGroup:safeMediaSelectionGroupForLegibleMedia()];
#endif

    Vector<RefPtr<InbandTextTrackPrivateAVF>> removedTextTracks = m_textTracks;
    for (AVPlayerItemTrack *playerItemTrack in m_cachedTracks.get()) {

        AVAssetTrack *assetTrack = [playerItemTrack assetTrack];
        if (![[assetTrack mediaType] isEqualToString:AVMediaTypeClosedCaption])
            continue;

        bool newCCTrack = true;
        for (unsigned i = removedTextTracks.size(); i > 0; --i) {
            if (removedTextTracks[i - 1]->textTrackCategory() != InbandTextTrackPrivateAVF::LegacyClosedCaption)
                continue;

            RefPtr<InbandTextTrackPrivateLegacyAVFObjC> track = static_cast<InbandTextTrackPrivateLegacyAVFObjC*>(m_textTracks[i - 1].get());
            if (track->avPlayerItemTrack() == playerItemTrack) {
                removedTextTracks.remove(i - 1);
                newCCTrack = false;
                break;
            }
        }

        if (!newCCTrack)
            continue;
        
        m_textTracks.append(InbandTextTrackPrivateLegacyAVFObjC::create(this, playerItemTrack));
    }

    processNewAndRemovedTextTracks(removedTextTracks);
}
#endif

NSArray* MediaPlayerPrivateAVFoundationObjC::safeAVAssetTracksForAudibleMedia()
{
    if (!m_avAsset)
        return nil;

    if ([m_avAsset.get() statusOfValueForKey:@"tracks" error:NULL] != AVKeyValueStatusLoaded)
        return nil;

    return [m_avAsset tracksWithMediaCharacteristic:AVMediaCharacteristicAudible];
}

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
bool MediaPlayerPrivateAVFoundationObjC::hasLoadedMediaSelectionGroups()
{
    if (!m_avAsset)
        return false;

    if ([m_avAsset.get() statusOfValueForKey:@"availableMediaCharacteristicsWithMediaSelectionOptions" error:NULL] != AVKeyValueStatusLoaded)
        return false;

    return true;
}

AVMediaSelectionGroupType* MediaPlayerPrivateAVFoundationObjC::safeMediaSelectionGroupForLegibleMedia()
{
    if (!hasLoadedMediaSelectionGroups())
        return nil;

    return [m_avAsset.get() mediaSelectionGroupForMediaCharacteristic:AVMediaCharacteristicLegible];
}

AVMediaSelectionGroupType* MediaPlayerPrivateAVFoundationObjC::safeMediaSelectionGroupForAudibleMedia()
{
    if (!hasLoadedMediaSelectionGroups())
        return nil;

    return [m_avAsset.get() mediaSelectionGroupForMediaCharacteristic:AVMediaCharacteristicAudible];
}

AVMediaSelectionGroupType* MediaPlayerPrivateAVFoundationObjC::safeMediaSelectionGroupForVisualMedia()
{
    if (!hasLoadedMediaSelectionGroups())
        return nil;

    return [m_avAsset.get() mediaSelectionGroupForMediaCharacteristic:AVMediaCharacteristicVisual];
}

void MediaPlayerPrivateAVFoundationObjC::processMediaSelectionOptions()
{
    AVMediaSelectionGroupType *legibleGroup = safeMediaSelectionGroupForLegibleMedia();
    if (!legibleGroup) {
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::processMediaSelectionOptions(%p) - nil mediaSelectionGroup", this);
        return;
    }

    // We enabled automatic media selection because we want alternate audio tracks to be enabled/disabled automatically,
    // but set the selected legible track to nil so text tracks will not be automatically configured.
    if (!m_textTracks.size())
        [m_avPlayerItem.get() selectMediaOption:nil inMediaSelectionGroup:safeMediaSelectionGroupForLegibleMedia()];

    Vector<RefPtr<InbandTextTrackPrivateAVF>> removedTextTracks = m_textTracks;
    NSArray *legibleOptions = [AVMediaSelectionGroup playableMediaSelectionOptionsFromArray:[legibleGroup options]];
    for (AVMediaSelectionOptionType *option in legibleOptions) {
        bool newTrack = true;
        for (unsigned i = removedTextTracks.size(); i > 0; --i) {
            if (removedTextTracks[i - 1]->textTrackCategory() == InbandTextTrackPrivateAVF::LegacyClosedCaption)
                continue;
            
            RetainPtr<AVMediaSelectionOptionType> currentOption;
#if ENABLE(AVF_CAPTIONS)
            if (removedTextTracks[i - 1]->textTrackCategory() == InbandTextTrackPrivateAVF::OutOfBand) {
                RefPtr<OutOfBandTextTrackPrivateAVF> track = static_cast<OutOfBandTextTrackPrivateAVF*>(removedTextTracks[i - 1].get());
                currentOption = track->mediaSelectionOption();
            } else
#endif
            {
                RefPtr<InbandTextTrackPrivateAVFObjC> track = static_cast<InbandTextTrackPrivateAVFObjC*>(removedTextTracks[i - 1].get());
                currentOption = track->mediaSelectionOption();
            }
            
            if ([currentOption.get() isEqual:option]) {
                removedTextTracks.remove(i - 1);
                newTrack = false;
                break;
            }
        }
        if (!newTrack)
            continue;

#if ENABLE(AVF_CAPTIONS)
        if ([option outOfBandSource]) {
            m_textTracks.append(OutOfBandTextTrackPrivateAVF::create(this, option));
            m_textTracks.last()->setHasBeenReported(true); // Ignore out-of-band tracks that we passed to AVFoundation so we do not double-count them
            continue;
        }
#endif

        m_textTracks.append(InbandTextTrackPrivateAVFObjC::create(this, option, InbandTextTrackPrivate::Generic));
    }

    processNewAndRemovedTextTracks(removedTextTracks);
}

void MediaPlayerPrivateAVFoundationObjC::processMetadataTrack()
{
    if (m_metadataTrack)
        return;

    m_metadataTrack = InbandMetadataTextTrackPrivateAVF::create(InbandTextTrackPrivate::Metadata, InbandTextTrackPrivate::Data);
    m_metadataTrack->setInBandMetadataTrackDispatchType("com.apple.streaming");
    player()->addTextTrack(m_metadataTrack);
}

void MediaPlayerPrivateAVFoundationObjC::processCue(NSArray *attributedStrings, NSArray *nativeSamples, const MediaTime& time)
{
    if (!m_currentTextTrack)
        return;

    m_currentTextTrack->processCue(reinterpret_cast<CFArrayRef>(attributedStrings), reinterpret_cast<CFArrayRef>(nativeSamples), time);
}

void MediaPlayerPrivateAVFoundationObjC::flushCues()
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::flushCues(%p)", this);

    if (!m_currentTextTrack)
        return;
    
    m_currentTextTrack->resetCueValues();
}
#endif // HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)

void MediaPlayerPrivateAVFoundationObjC::setCurrentTextTrack(InbandTextTrackPrivateAVF *track)
{
    if (m_currentTextTrack == track)
        return;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::setCurrentTextTrack(%p) - selecting track %p, language = %s", this, track, track ? track->language().string().utf8().data() : "");
        
    m_currentTextTrack = track;

    if (track) {
        if (track->textTrackCategory() == InbandTextTrackPrivateAVF::LegacyClosedCaption)
            [m_avPlayer.get() setClosedCaptionDisplayEnabled:YES];
#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
#if ENABLE(AVF_CAPTIONS)
        else if (track->textTrackCategory() == InbandTextTrackPrivateAVF::OutOfBand)
            [m_avPlayerItem.get() selectMediaOption:static_cast<OutOfBandTextTrackPrivateAVF*>(track)->mediaSelectionOption() inMediaSelectionGroup:safeMediaSelectionGroupForLegibleMedia()];
#endif
        else
            [m_avPlayerItem.get() selectMediaOption:static_cast<InbandTextTrackPrivateAVFObjC*>(track)->mediaSelectionOption() inMediaSelectionGroup:safeMediaSelectionGroupForLegibleMedia()];
#endif
    } else {
#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
        [m_avPlayerItem.get() selectMediaOption:0 inMediaSelectionGroup:safeMediaSelectionGroupForLegibleMedia()];
#endif
        [m_avPlayer.get() setClosedCaptionDisplayEnabled:NO];
    }

}

String MediaPlayerPrivateAVFoundationObjC::languageOfPrimaryAudioTrack() const
{
    if (!m_languageOfPrimaryAudioTrack.isNull())
        return m_languageOfPrimaryAudioTrack;

    if (!m_avPlayerItem.get())
        return emptyString();

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
    // If AVFoundation has an audible group, return the language of the currently selected audible option.
    AVMediaSelectionGroupType *audibleGroup = [m_avAsset.get() mediaSelectionGroupForMediaCharacteristic:AVMediaCharacteristicAudible];
    AVMediaSelectionOptionType *currentlySelectedAudibleOption = [m_avPlayerItem.get() selectedMediaOptionInMediaSelectionGroup:audibleGroup];
    if (currentlySelectedAudibleOption) {
        m_languageOfPrimaryAudioTrack = [[currentlySelectedAudibleOption locale] localeIdentifier];
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::languageOfPrimaryAudioTrack(%p) - returning language of selected audible option: %s", this, m_languageOfPrimaryAudioTrack.utf8().data());

        return m_languageOfPrimaryAudioTrack;
    }
#endif // HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)

    // AVFoundation synthesizes an audible group when there is only one ungrouped audio track if there is also a legible group (one or
    // more in-band text tracks). It doesn't know about out-of-band tracks, so if there is a single audio track return its language.
    NSArray *tracks = [m_avAsset.get() tracksWithMediaType:AVMediaTypeAudio];
    if (!tracks || [tracks count] != 1) {
        m_languageOfPrimaryAudioTrack = emptyString();
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::languageOfPrimaryAudioTrack(%p) - %lu audio tracks, returning emptyString()", this, static_cast<unsigned long>(tracks ? [tracks count] : 0));
        return m_languageOfPrimaryAudioTrack;
    }

    AVAssetTrack *track = [tracks objectAtIndex:0];
    m_languageOfPrimaryAudioTrack = AVTrackPrivateAVFObjCImpl::languageForAVAssetTrack(track);

#if !LOG_DISABLED
    if (m_languageOfPrimaryAudioTrack == emptyString())
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::languageOfPrimaryAudioTrack(%p) - single audio track has no language, returning emptyString()", this);
    else
        LOG(Media, "MediaPlayerPrivateAVFoundationObjC::languageOfPrimaryAudioTrack(%p) - returning language of single audio track: %s", this, m_languageOfPrimaryAudioTrack.utf8().data());
#endif

    return m_languageOfPrimaryAudioTrack;
}

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
bool MediaPlayerPrivateAVFoundationObjC::isCurrentPlaybackTargetWireless() const
{
    if (!m_avPlayer)
        return false;

    bool wirelessTarget = m_avPlayer.get().externalPlaybackActive;
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::isCurrentPlaybackTargetWireless(%p) - returning %s", this, boolString(wirelessTarget));

    return wirelessTarget;
}

MediaPlayer::WirelessPlaybackTargetType MediaPlayerPrivateAVFoundationObjC::wirelessPlaybackTargetType() const
{
    if (!m_avPlayer)
        return MediaPlayer::TargetTypeNone;

#if PLATFORM(IOS)
    switch (wkExernalDeviceTypeForPlayer(m_avPlayer.get())) {
    case wkExternalPlaybackTypeNone:
        return MediaPlayer::TargetTypeNone;
    case wkExternalPlaybackTypeAirPlay:
        return MediaPlayer::TargetTypeAirPlay;
    case wkExternalPlaybackTypeTVOut:
        return MediaPlayer::TargetTypeTVOut;
    }

    ASSERT_NOT_REACHED();
    return MediaPlayer::TargetTypeNone;

#else
    return MediaPlayer::TargetTypeAirPlay;
#endif
}

String MediaPlayerPrivateAVFoundationObjC::wirelessPlaybackTargetName() const
{
    if (!m_avPlayer)
        return emptyString();

    String wirelessTargetName;
#if !PLATFORM(IOS)
    if (m_outputContext)
        wirelessTargetName = m_outputContext.get().deviceName;
#else
    wirelessTargetName = wkExernalDeviceDisplayNameForPlayer(m_avPlayer.get());
#endif
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::wirelessPlaybackTargetName(%p) - returning %s", this, wirelessTargetName.utf8().data());

    return wirelessTargetName;
}

bool MediaPlayerPrivateAVFoundationObjC::wirelessVideoPlaybackDisabled() const
{
    if (!m_avPlayer)
        return !m_allowsWirelessVideoPlayback;

    m_allowsWirelessVideoPlayback = [m_avPlayer.get() allowsExternalPlayback];
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::wirelessVideoPlaybackDisabled(%p) - returning %s", this, boolString(!m_allowsWirelessVideoPlayback));

    return !m_allowsWirelessVideoPlayback;
}

void MediaPlayerPrivateAVFoundationObjC::setWirelessVideoPlaybackDisabled(bool disabled)
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::setWirelessVideoPlaybackDisabled(%p) - %s", this, boolString(disabled));
    m_allowsWirelessVideoPlayback = !disabled;
    if (!m_avPlayer)
        return;

    setDelayCallbacks(true);
    [m_avPlayer.get() setAllowsExternalPlayback:!disabled];
    setDelayCallbacks(false);
}

#if !PLATFORM(IOS)
void MediaPlayerPrivateAVFoundationObjC::setWirelessPlaybackTarget(Ref<MediaPlaybackTarget>&& target)
{
    MediaPlaybackTargetMac* macTarget = toMediaPlaybackTargetMac(&target.get());

    m_outputContext = macTarget->outputContext();
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::setWirelessPlaybackTarget(%p) - target = %p, device name = %s", this, m_outputContext.get(), [m_outputContext.get().deviceName UTF8String]);

    if (!m_outputContext || !m_outputContext.get().deviceName)
        setShouldPlayToPlaybackTarget(false);
}

void MediaPlayerPrivateAVFoundationObjC::setShouldPlayToPlaybackTarget(bool shouldPlay)
{
    m_shouldPlayToPlaybackTarget = shouldPlay;

    AVOutputContext *newContext = shouldPlay ? m_outputContext.get() : nil;
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::setShouldPlayToPlaybackTarget(%p) - target = %p, shouldPlay = %s", this, newContext, boolString(shouldPlay));

    if (!m_avPlayer)
        return;

    RetainPtr<AVOutputContext> currentContext = m_avPlayer.get().outputContext;
    if ((!newContext && !currentContext.get()) || [currentContext.get() isEqual:newContext])
        return;

    setDelayCallbacks(true);
    m_avPlayer.get().outputContext = newContext;
    setDelayCallbacks(false);
}
#endif // !PLATFORM(IOS)

void MediaPlayerPrivateAVFoundationObjC::updateDisableExternalPlayback()
{
    if (!m_avPlayer)
        return;

#if PLATFORM(IOS)
    [m_avPlayer setUsesExternalPlaybackWhileExternalScreenIsActive:m_videoFullscreenLayer != nil];
#endif
}
#endif

void MediaPlayerPrivateAVFoundationObjC::playerItemStatusDidChange(int status)
{
    m_cachedItemStatus = status;

    updateStates();
}

void MediaPlayerPrivateAVFoundationObjC::playbackLikelyToKeepUpWillChange()
{
    m_pendingStatusChanges++;
}

void MediaPlayerPrivateAVFoundationObjC::playbackLikelyToKeepUpDidChange(bool likelyToKeepUp)
{
    m_cachedLikelyToKeepUp = likelyToKeepUp;

    ASSERT(m_pendingStatusChanges);
    if (!--m_pendingStatusChanges)
        updateStates();
}

void MediaPlayerPrivateAVFoundationObjC::playbackBufferEmptyWillChange()
{
    m_pendingStatusChanges++;
}

void MediaPlayerPrivateAVFoundationObjC::playbackBufferEmptyDidChange(bool bufferEmpty)
{
    m_cachedBufferEmpty = bufferEmpty;

    ASSERT(m_pendingStatusChanges);
    if (!--m_pendingStatusChanges)
        updateStates();
}

void MediaPlayerPrivateAVFoundationObjC::playbackBufferFullWillChange()
{
    m_pendingStatusChanges++;
}

void MediaPlayerPrivateAVFoundationObjC::playbackBufferFullDidChange(bool bufferFull)
{
    m_cachedBufferFull = bufferFull;

    ASSERT(m_pendingStatusChanges);
    if (!--m_pendingStatusChanges)
        updateStates();
}

void MediaPlayerPrivateAVFoundationObjC::seekableTimeRangesDidChange(RetainPtr<NSArray> seekableRanges)
{
    m_cachedSeekableRanges = seekableRanges;

    seekableTimeRangesChanged();
    updateStates();
}

void MediaPlayerPrivateAVFoundationObjC::loadedTimeRangesDidChange(RetainPtr<NSArray> loadedRanges)
{
    m_cachedLoadedRanges = loadedRanges;

    loadedTimeRangesChanged();
    updateStates();
}

void MediaPlayerPrivateAVFoundationObjC::firstFrameAvailableDidChange(bool isReady)
{
    m_cachedIsReadyForDisplay = isReady;
    if (!hasVideo() && isReady)
        tracksChanged();
    updateStates();
}

void MediaPlayerPrivateAVFoundationObjC::trackEnabledDidChange(bool)
{
    tracksChanged();
    updateStates();
}

void MediaPlayerPrivateAVFoundationObjC::setShouldBufferData(bool shouldBuffer)
{
    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::shouldBufferData(%p) - %s", this, boolString(shouldBuffer));
    if (m_shouldBufferData == shouldBuffer)
        return;

    m_shouldBufferData = shouldBuffer;
    
    if (!m_avPlayer)
        return;

    setAVPlayerItem(shouldBuffer ? m_avPlayerItem.get() : nil);
}

#if ENABLE(DATACUE_VALUE)
static const AtomicString& metadataType(NSString *avMetadataKeySpace)
{
    static NeverDestroyed<const AtomicString> quickTimeUserData("com.apple.quicktime.udta", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<const AtomicString> isoUserData("org.mp4ra", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<const AtomicString> quickTimeMetadata("com.apple.quicktime.mdta", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<const AtomicString> iTunesMetadata("com.apple.itunes", AtomicString::ConstructFromLiteral);
    static NeverDestroyed<const AtomicString> id3Metadata("org.id3", AtomicString::ConstructFromLiteral);

    if ([avMetadataKeySpace isEqualToString:AVMetadataKeySpaceQuickTimeUserData])
        return quickTimeUserData;
    if (AVMetadataKeySpaceISOUserData && [avMetadataKeySpace isEqualToString:AVMetadataKeySpaceISOUserData])
        return isoUserData;
    if ([avMetadataKeySpace isEqualToString:AVMetadataKeySpaceQuickTimeMetadata])
        return quickTimeMetadata;
    if ([avMetadataKeySpace isEqualToString:AVMetadataKeySpaceiTunes])
        return iTunesMetadata;
    if ([avMetadataKeySpace isEqualToString:AVMetadataKeySpaceID3])
        return id3Metadata;

    return emptyAtom;
}
#endif

void MediaPlayerPrivateAVFoundationObjC::metadataDidArrive(RetainPtr<NSArray> metadata, const MediaTime& mediaTime)
{
    m_currentMetaData = metadata && ![metadata isKindOfClass:[NSNull class]] ? metadata : nil;

    LOG(Media, "MediaPlayerPrivateAVFoundationObjC::metadataDidArrive(%p) - adding %i cues at time %s", this, m_currentMetaData ? static_cast<int>([m_currentMetaData.get() count]) : 0, toString(mediaTime).utf8().data());

#if ENABLE(DATACUE_VALUE)
    if (seeking())
        return;

    if (!m_metadataTrack)
        processMetadataTrack();

    if (!metadata || [metadata isKindOfClass:[NSNull class]]) {
        m_metadataTrack->updatePendingCueEndTimes(mediaTime);
        return;
    }

    // Set the duration of all incomplete cues before adding new ones.
    MediaTime earliestStartTime = MediaTime::positiveInfiniteTime();
    for (AVMetadataItemType *item in m_currentMetaData.get()) {
        MediaTime start = toMediaTime(item.time);
        if (start < earliestStartTime)
            earliestStartTime = start;
    }
    m_metadataTrack->updatePendingCueEndTimes(earliestStartTime);

    for (AVMetadataItemType *item in m_currentMetaData.get()) {
        MediaTime start = toMediaTime(item.time);
        MediaTime end = MediaTime::positiveInfiniteTime();
        if (CMTIME_IS_VALID(item.duration))
            end = start + toMediaTime(item.duration);

        AtomicString type = nullAtom;
        if (item.keySpace)
            type = metadataType(item.keySpace);

        m_metadataTrack->addDataCue(start, end, SerializedPlatformRepresentationMac::create(item), type);
    }
#endif
}

void MediaPlayerPrivateAVFoundationObjC::tracksDidChange(RetainPtr<NSArray> tracks)
{
    for (AVPlayerItemTrack *track in m_cachedTracks.get())
        [track removeObserver:m_objcObserver.get() forKeyPath:@"enabled"];

    NSArray *assetTracks = [m_avAsset tracks];

    m_cachedTracks = [tracks objectsAtIndexes:[tracks indexesOfObjectsPassingTest:^(id obj, NSUInteger, BOOL*) {
        AVAssetTrack* assetTrack = [obj assetTrack];

        if ([assetTracks containsObject:assetTrack])
            return YES;

        // Track is a streaming track. Omit if it belongs to a valid AVMediaSelectionGroup.
        if (!hasLoadedMediaSelectionGroups())
            return NO;

        if ([assetTrack hasMediaCharacteristic:AVMediaCharacteristicAudible] && safeMediaSelectionGroupForAudibleMedia())
            return NO;

        if ([assetTrack hasMediaCharacteristic:AVMediaCharacteristicVisual] && safeMediaSelectionGroupForVisualMedia())
            return NO;

        if ([assetTrack hasMediaCharacteristic:AVMediaCharacteristicLegible] && safeMediaSelectionGroupForLegibleMedia())
            return NO;

        return YES;
    }]];

    for (AVPlayerItemTrack *track in m_cachedTracks.get())
        [track addObserver:m_objcObserver.get() forKeyPath:@"enabled" options:NSKeyValueObservingOptionNew context:(void *)MediaPlayerAVFoundationObservationContextPlayerItemTrack];

    m_cachedTotalBytes = 0;

    tracksChanged();
    updateStates();
}

void MediaPlayerPrivateAVFoundationObjC::hasEnabledAudioDidChange(bool hasEnabledAudio)
{
    m_cachedHasEnabledAudio = hasEnabledAudio;

    tracksChanged();
    updateStates();
}

void MediaPlayerPrivateAVFoundationObjC::presentationSizeDidChange(FloatSize size)
{
    m_cachedPresentationSize = size;

    sizeChanged();
    updateStates();
}

void MediaPlayerPrivateAVFoundationObjC::durationDidChange(const MediaTime& duration)
{
    m_cachedDuration = duration;

    invalidateCachedDuration();
}

void MediaPlayerPrivateAVFoundationObjC::rateDidChange(double rate)
{
    m_cachedRate = rate;

    updateStates();
    rateChanged();
}
    
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
void MediaPlayerPrivateAVFoundationObjC::playbackTargetIsWirelessDidChange()
{
    playbackTargetIsWirelessChanged();
}
#endif

void MediaPlayerPrivateAVFoundationObjC::canPlayFastForwardDidChange(bool newValue)
{
    m_cachedCanPlayFastForward = newValue;
}

void MediaPlayerPrivateAVFoundationObjC::canPlayFastReverseDidChange(bool newValue)
{
    m_cachedCanPlayFastReverse = newValue;
}

URL MediaPlayerPrivateAVFoundationObjC::resolvedURL() const
{
    if (!m_avAsset || [m_avAsset statusOfValueForKey:@"resolvedURL" error:nullptr] != AVKeyValueStatusLoaded)
        return MediaPlayerPrivateAVFoundation::resolvedURL();

    return URL([m_avAsset resolvedURL]);
}

NSArray* assetMetadataKeyNames()
{
    static NSArray* keys;
    if (!keys) {
        keys = [[NSArray alloc] initWithObjects:@"duration",
                    @"naturalSize",
                    @"preferredTransform",
                    @"preferredVolume",
                    @"preferredRate",
                    @"playable",
                    @"resolvedURL",
                    @"tracks",
                    @"availableMediaCharacteristicsWithMediaSelectionOptions",
                   nil];
    }
    return keys;
}

NSArray* itemKVOProperties()
{
    static NSArray* keys;
    if (!keys) {
        keys = [[NSArray alloc] initWithObjects:@"presentationSize",
                @"status",
                @"asset",
                @"tracks",
                @"seekableTimeRanges",
                @"loadedTimeRanges",
                @"playbackLikelyToKeepUp",
                @"playbackBufferFull",
                @"playbackBufferEmpty",
                @"duration",
                @"hasEnabledAudio",
                @"timedMetadata",
                @"canPlayFastForward",
                @"canPlayFastReverse",
                nil];
    }
    return keys;
}

NSArray* assetTrackMetadataKeyNames()
{
    static NSArray* keys = [[NSArray alloc] initWithObjects:@"totalSampleDataLength", @"mediaType", @"enabled", @"preferredTransform", @"naturalSize", nil];
    return keys;
}

NSArray* playerKVOProperties()
{
    static NSArray* keys = [[NSArray alloc] initWithObjects:@"rate",
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
                            @"externalPlaybackActive", @"allowsExternalPlayback",
#endif
                            nil];
    return keys;
}
} // namespace WebCore

@implementation WebCoreAVFMovieObserver

- (id)initWithCallback:(MediaPlayerPrivateAVFoundationObjC*)callback
{
    self = [super init];
    if (!self)
        return nil;
    m_callback = callback;
    return self;
}

- (void)disconnect
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    m_callback = 0;
}

- (void)metadataLoaded
{
    if (!m_callback)
        return;

    m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::AssetMetadataLoaded);
}

- (void)didEnd:(NSNotification *)unusedNotification
{
    UNUSED_PARAM(unusedNotification);
    if (!m_callback)
        return;
    m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification::ItemDidPlayToEndTime);
}

- (void)observeValueForKeyPath:keyPath ofObject:(id)object change:(NSDictionary *)change context:(MediaPlayerAVFoundationObservationContext)context
{
    UNUSED_PARAM(object);
    id newValue = [change valueForKey:NSKeyValueChangeNewKey];

    if (!m_callback)
        return;

    bool willChange = [[change valueForKey:NSKeyValueChangeNotificationIsPriorKey] boolValue];

#if !LOG_DISABLED
    if (willChange)
        LOG(Media, "WebCoreAVFMovieObserver::observeValueForKeyPath(%p) - will change, keyPath = %s", self, [keyPath UTF8String]);
    else {
        RetainPtr<NSString> valueString = adoptNS([[NSString alloc] initWithFormat:@"%@", newValue]);
        LOG(Media, "WebCoreAVFMovieObserver::observeValueForKeyPath(%p) - did change, keyPath = %s, value = %s", self, [keyPath UTF8String], [valueString.get() UTF8String]);
    }
#endif

    std::function<void ()> function;

    if (context == MediaPlayerAVFoundationObservationContextAVPlayerLayer) {
        if ([keyPath isEqualToString:@"readyForDisplay"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::firstFrameAvailableDidChange, m_callback, [newValue boolValue]);
    }

    if (context == MediaPlayerAVFoundationObservationContextPlayerItemTrack) {
        if ([keyPath isEqualToString:@"enabled"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::trackEnabledDidChange, m_callback, [newValue boolValue]);
    }

    if (context == MediaPlayerAVFoundationObservationContextPlayerItem && willChange) {
        if ([keyPath isEqualToString:@"playbackLikelyToKeepUp"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::playbackLikelyToKeepUpWillChange, m_callback);
        else if ([keyPath isEqualToString:@"playbackBufferEmpty"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::playbackBufferEmptyWillChange, m_callback);
        else if ([keyPath isEqualToString:@"playbackBufferFull"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::playbackBufferFullWillChange, m_callback);
    }

    if (context == MediaPlayerAVFoundationObservationContextPlayerItem && !willChange) {
        // A value changed for an AVPlayerItem
        if ([keyPath isEqualToString:@"status"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::playerItemStatusDidChange, m_callback, [newValue intValue]);
        else if ([keyPath isEqualToString:@"playbackLikelyToKeepUp"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::playbackLikelyToKeepUpDidChange, m_callback, [newValue boolValue]);
        else if ([keyPath isEqualToString:@"playbackBufferEmpty"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::playbackBufferEmptyDidChange, m_callback, [newValue boolValue]);
        else if ([keyPath isEqualToString:@"playbackBufferFull"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::playbackBufferFullDidChange, m_callback, [newValue boolValue]);
        else if ([keyPath isEqualToString:@"asset"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::setAsset, m_callback, RetainPtr<id>(newValue));
        else if ([keyPath isEqualToString:@"loadedTimeRanges"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::loadedTimeRangesDidChange, m_callback, RetainPtr<NSArray>(newValue));
        else if ([keyPath isEqualToString:@"seekableTimeRanges"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::seekableTimeRangesDidChange, m_callback, RetainPtr<NSArray>(newValue));
        else if ([keyPath isEqualToString:@"tracks"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::tracksDidChange, m_callback, RetainPtr<NSArray>(newValue));
        else if ([keyPath isEqualToString:@"hasEnabledAudio"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::hasEnabledAudioDidChange, m_callback, [newValue boolValue]);
        else if ([keyPath isEqualToString:@"presentationSize"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::presentationSizeDidChange, m_callback, FloatSize([newValue sizeValue]));
        else if ([keyPath isEqualToString:@"duration"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::durationDidChange, m_callback, toMediaTime([newValue CMTimeValue]));
        else if ([keyPath isEqualToString:@"timedMetadata"] && newValue) {
            MediaTime now;
            CMTime itemTime = [(AVPlayerItemType *)object currentTime];
            if (CMTIME_IS_NUMERIC(itemTime))
                now = std::max(toMediaTime(itemTime), MediaTime::zeroTime());
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::metadataDidArrive, m_callback, RetainPtr<NSArray>(newValue), now);
        } else if ([keyPath isEqualToString:@"canPlayFastReverse"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::canPlayFastReverseDidChange, m_callback, [newValue boolValue]);
        else if ([keyPath isEqualToString:@"canPlayFastForward"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::canPlayFastForwardDidChange, m_callback, [newValue boolValue]);
    }

    if (context == MediaPlayerAVFoundationObservationContextPlayer && !willChange) {
        // A value changed for an AVPlayer.
        if ([keyPath isEqualToString:@"rate"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::rateDidChange, m_callback, [newValue doubleValue]);
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
        else if ([keyPath isEqualToString:@"externalPlaybackActive"] || [keyPath isEqualToString:@"allowsExternalPlayback"])
            function = std::bind(&MediaPlayerPrivateAVFoundationObjC::playbackTargetIsWirelessDidChange, m_callback);
#endif
    }
    
    if (!function)
        return;

    auto weakThis = m_callback->createWeakPtr();
    m_callback->scheduleMainThreadNotification(MediaPlayerPrivateAVFoundation::Notification([weakThis, function]{
        // weakThis and function both refer to the same MediaPlayerPrivateAVFoundationObjC instance. If the WeakPtr has
        // been cleared, the underlying object has been destroyed, and it is unsafe to call function().
        if (!weakThis)
            return;
        function();
    }));
}

#if HAVE(AVFOUNDATION_MEDIA_SELECTION_GROUP)
- (void)legibleOutput:(id)output didOutputAttributedStrings:(NSArray *)strings nativeSampleBuffers:(NSArray *)nativeSamples forItemTime:(CMTime)itemTime
{
    UNUSED_PARAM(output);
    UNUSED_PARAM(nativeSamples);

    if (!m_callback)
        return;

    RetainPtr<WebCoreAVFMovieObserver> strongSelf = self;
    RetainPtr<NSArray> strongStrings = strings;
    RetainPtr<NSArray> strongSamples = nativeSamples;
    callOnMainThread([strongSelf, strongStrings, strongSamples, itemTime] {
        MediaPlayerPrivateAVFoundationObjC* callback = strongSelf->m_callback;
        if (!callback)
            return;
        callback->processCue(strongStrings.get(), strongSamples.get(), toMediaTime(itemTime));
    });
}

- (void)outputSequenceWasFlushed:(id)output
{
    UNUSED_PARAM(output);

    if (!m_callback)
        return;
    
    RetainPtr<WebCoreAVFMovieObserver> strongSelf = self;
    callOnMainThread([strongSelf] {
        if (MediaPlayerPrivateAVFoundationObjC* callback = strongSelf->m_callback)
            callback->flushCues();
    });
}
#endif

@end

#if HAVE(AVFOUNDATION_LOADER_DELEGATE)
@implementation WebCoreAVFLoaderDelegate

- (id)initWithCallback:(MediaPlayerPrivateAVFoundationObjC*)callback
{
    self = [super init];
    if (!self)
        return nil;
    m_callback = callback;
    return self;
}

- (BOOL)resourceLoader:(AVAssetResourceLoader *)resourceLoader shouldWaitForLoadingOfRequestedResource:(AVAssetResourceLoadingRequest *)loadingRequest
{
    UNUSED_PARAM(resourceLoader);
    if (!m_callback)
        return NO;

    RetainPtr<WebCoreAVFLoaderDelegate> strongSelf = self;
    RetainPtr<AVAssetResourceLoadingRequest> strongRequest = loadingRequest;
    callOnMainThread([strongSelf, strongRequest] {
        MediaPlayerPrivateAVFoundationObjC* callback = strongSelf->m_callback;
        if (!callback) {
            [strongRequest finishLoadingWithError:nil];
            return;
        }

        if (!callback->shouldWaitForLoadingOfResource(strongRequest.get()))
            [strongRequest finishLoadingWithError:nil];
    });

    return YES;
}

- (BOOL)resourceLoader:(AVAssetResourceLoader *)resourceLoader shouldWaitForResponseToAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    UNUSED_PARAM(resourceLoader);
    if (!m_callback)
        return NO;

    if ([[[challenge protectionSpace] authenticationMethod] isEqualToString:NSURLAuthenticationMethodServerTrust])
        return NO;

    RetainPtr<WebCoreAVFLoaderDelegate> strongSelf = self;
    RetainPtr<NSURLAuthenticationChallenge> strongChallenge = challenge;
    callOnMainThread([strongSelf, strongChallenge] {
        MediaPlayerPrivateAVFoundationObjC* callback = strongSelf->m_callback;
        if (!callback) {
            [[strongChallenge sender] cancelAuthenticationChallenge:strongChallenge.get()];
            return;
        }

        if (!callback->shouldWaitForResponseToAuthenticationChallenge(strongChallenge.get()))
            [[strongChallenge sender] cancelAuthenticationChallenge:strongChallenge.get()];
    });

    return YES;
}

- (void)resourceLoader:(AVAssetResourceLoader *)resourceLoader didCancelLoadingRequest:(AVAssetResourceLoadingRequest *)loadingRequest
{
    UNUSED_PARAM(resourceLoader);
    if (!m_callback)
        return;

    RetainPtr<WebCoreAVFLoaderDelegate> strongSelf = self;
    RetainPtr<AVAssetResourceLoadingRequest> strongRequest = loadingRequest;
    callOnMainThread([strongSelf, strongRequest] {
        MediaPlayerPrivateAVFoundationObjC* callback = strongSelf->m_callback;
        if (callback)
            callback->didCancelLoadingRequest(strongRequest.get());
    });
}

- (void)setCallback:(MediaPlayerPrivateAVFoundationObjC*)callback
{
    m_callback = callback;
}
@end
#endif

#if HAVE(AVFOUNDATION_VIDEO_OUTPUT)
@implementation WebCoreAVFPullDelegate
- (id)initWithCallback:(MediaPlayerPrivateAVFoundationObjC *)callback
{
    self = [super init];
    if (self)
        m_callback = callback;
    return self;
}

- (void)setCallback:(MediaPlayerPrivateAVFoundationObjC *)callback
{
    m_callback = callback;
}

- (void)outputMediaDataWillChange:(AVPlayerItemVideoOutput *)output
{
    if (m_callback)
        m_callback->outputMediaDataWillChange(output);
}

- (void)outputSequenceWasFlushed:(AVPlayerItemVideoOutput *)output
{
    UNUSED_PARAM(output);
    // No-op.
}
@end
#endif

#endif

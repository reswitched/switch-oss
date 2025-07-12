/*
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 */

#include "config.h"

#if ENABLE(VIDEO)

#include "MediaPlayerPrivateWKC.h"

#include "BitmapImage.h"
#include "Chrome.h"
#include "CString.h"
#include "FloatQuad.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameNetworkingContext.h"
#include "GraphicsContext.h"
#include "HTMLMediaElement.h"
#include "ImageWKC.h"
#include "InspectorInstrumentation.h"
#include "InspectorInstrumentationPublic.h"
#include "Page.h"
#include "MediaStream.h"
#include "WTFString.h"
#include "ResourceError.h"
#include "ResourceHandle.h"
#include "ResourceHandleManagerWKC.h"
#include "ResourceRequest.h"
#include "TimeRanges.h"

#include "NotImplemented.h"

#if ENABLE(MEDIA_SOURCE)
#include "MediaSourcePrivate.h"
#include "MediaSourcePrivateWKC.h"
#endif
#if ENABLE(MEDIA_STREAM)
#include "MediaStreamRegistry.h"
#endif

#include <wkc/wkcgpeer.h>

// debug print
#define NO_NXLOG
#include "utils/nxDebugPrint.h"

#ifdef WKC_API
# undef WKC_API
# define WKC_API
#endif
#include "helpers/privates/WKCHTMLMediaElementPrivate.h"
#include "helpers/privates/WKCResourceResponsePrivate.h"

namespace WebCore {

MediaPlayerPrivateWKC::MediaPlayerPrivateWKC(MediaPlayer* player)
    : m_player(player)
    , m_peer(0)
    , m_audiopeer(0)
    , m_volume(1)
    , m_muted(false)
    , m_resizeAtDrawing(false)
    , m_size()
    , m_httpConnections()
    , m_lastBuffered(0)
{
    const WKCMediaPlayerCallbacks cb = {
        notifyPlayerStateProc,
        notifyRequestInvalidateProc,
        notifyAudioDataAvailableProc,
        notifyKeyNeededProc,
        notifyKeyRequestProc,
        notifyKeyAddedProc,
        notifyKeyErrorProc,
        chromeVisibleProc,
        createHTMLMediaElementProc,
        destroyHTMLMediaElementProc,
        notifyPlayerStreamOpenURIProc,
        notifyPlayerStreamCloseProc,
        notifyPlayerStreamCancelProc,
        onCurlDebugProc,
    };

    m_peer = wkcMediaPlayerCreatePeer(&cb, (void *)this);
    if (!m_peer)
        CRASH_WITH_NO_MEMORY();
}

MediaPlayerPrivateWKC::~MediaPlayerPrivateWKC()
{
    while (m_httpConnections.size()) {
        httpConnection& c = m_httpConnections.at(0);
        auto h = c.m_connection;
        if (h) {
            h->cancel();
            h->clearClient();
        }
        m_httpConnections.remove(0);
    }
    m_httpConnections.clear();
    if (m_audiopeer) {
        wkcAudioClosePeer(m_audiopeer);
    }
    if (m_peer) {
        wkcMediaPlayerDeletePeer(m_peer);
    }
    m_networkInfo.clear();
}

class MediaPlayerFactoryWKC final : public MediaPlayerFactory {
private:
    MediaPlayerEnums::MediaEngineIdentifier identifier() const final { return MediaPlayerEnums::MediaEngineIdentifier::MediaFoundation; };

    std::unique_ptr<MediaPlayerPrivateInterface> createMediaEnginePlayer(MediaPlayer* player) const final
    {
        return makeUnique<MediaPlayerPrivateWKC>(player);
    }

    void getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types) const final
    {
        return MediaPlayerPrivateWKC::getSupportedTypes(types);
    }

    MediaPlayer::SupportsType supportsTypeAndCodecs(const MediaEngineSupportParameters& parameters) const final
    {
        return MediaPlayerPrivateWKC::supportsType(parameters);
    }
};

void
MediaPlayerPrivateWKC::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable()) {
        registrar(makeUnique<MediaPlayerFactoryWKC>());
    }
}

static const HashSet<String, ASCIICaseInsensitiveHash>& mimeTypeCache()
{
    static NeverDestroyed<HashSet<String, ASCIICaseInsensitiveHash>> cachedTypes;
    if (cachedTypes.isNull())
        cachedTypes.construct();

    if (cachedTypes.get().size() > 0)
        return cachedTypes;

    int items = wkcMediaPlayerSupportedMIMETypesPeer();
    for (int i=0; i<items; i++) {
        cachedTypes.get().add(String(wkcMediaPlayerSupportedMIMETypePeer(i)));
    }

    return cachedTypes;
}

void
MediaPlayerPrivateWKC::getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types)
{
    types = mimeTypeCache();
}

MediaPlayer::SupportsType
MediaPlayerPrivateWKC::supportsType(const MediaEngineSupportParameters& parameters)
{
    if (parameters.type.isEmpty())
        return MediaPlayer::SupportsType::IsNotSupported;

    if (!mimeTypeCache().contains(parameters.type.containerType()))
        return MediaPlayer::SupportsType::IsNotSupported;

    if (parameters.type.codecs().isEmpty())
        return MediaPlayer::SupportsType::MayBeSupported;

    bool mayBeSupported = false;
    for (const auto& codec : parameters.type.codecs()) {
        int ret = wkcMediaPlayerIsSupportedMIMETypePeer(parameters.type.containerType().utf8().data(), codec.utf8().data(), nullptr, parameters.url.string().utf8().data());
        if (ret == WKC_MEDIA_SUPPORT_NOTSUPPORTED) {
            return MediaPlayer::SupportsType::IsNotSupported;
        }
        if (ret == WKC_MEDIA_SUPPORT_MAYBESUPPORTED) {
            mayBeSupported = true;
        }
    }
    if (mayBeSupported)
        return MediaPlayer::SupportsType::MayBeSupported;

    return MediaPlayer::SupportsType::IsSupported;
}

#if ENABLE(ENCRYPTED_MEDIA)
void MediaPlayerPrivateWKC::notifyKeyNeeded(const char *in_keySystem, const char *in_sessionID, const unsigned char *in_initData, unsigned in_initDataLen)
{
    if (!m_player) {
        return;
    }

    // Conversion
    String strkeySystem = in_keySystem;
    String strSessionID = in_sessionID;

    m_player->keyNeeded(strkeySystem, strSessionID, in_initData, in_initDataLen);
}

void MediaPlayerPrivateWKC::notifyKeyRequest(const char *in_keySystem, const char *in_sessionID, const unsigned char *in_binaryBuff, unsigned in_len, const char *in_defaultURL)
{
    if (!m_player) {
        return;
    }

    // Conversion
    String strkeySystem = in_keySystem;
    String strSessionID = in_sessionID;
    if (in_defaultURL != NULL) {
        const URL& defaultURL = URL(URL(), in_defaultURL);
        m_player->keyMessage(strkeySystem, strSessionID, in_binaryBuff, in_len, defaultURL);
    } else {
        const URL& defaultURL = URL();
        m_player->keyMessage(strkeySystem, strSessionID, in_binaryBuff, in_len, defaultURL);
    }
}

void MediaPlayerPrivateWKC::notifyKeyAdded(const char *in_keySystem, const char *in_sessionID)
{
    if (!m_player) {
        return;
    }

    // Conversion
    String strkeySystem = in_keySystem;
    String strSessionID = in_sessionID;

    m_player->keyAdded(strkeySystem, strSessionID);
}

void MediaPlayerPrivateWKC::notifyKeyError(const char *in_keySystem, const char *in_sessionID, int in_keyErrorCode, unsigned short in_systemCode)
{
    if (!m_player) {
        return;
    }

    // Conversion
    String strkeySystem = in_keySystem;
    String strSessionID = in_sessionID;

    m_player->keyError(strkeySystem, strSessionID, (MediaPlayerClient::MediaKeyErrorCode)in_keyErrorCode, in_systemCode);
}
#endif

HashSet<RefPtr<SecurityOrigin>>
MediaPlayerPrivateWKC::originsInMediaCache(const String&)
{
    notImplemented();
    return { };
}

void
MediaPlayerPrivateWKC::clearMediaCache(const String&, WallTime)
{
    notImplemented();
}

void
MediaPlayerPrivateWKC::clearMediaCacheForOrigins(const String&, const HashSet<RefPtr<SecurityOrigin>>&)
{
    notImplemented();
}

bool
MediaPlayerPrivateWKC::supportsKeySystem(const String& keySystem, const String& mimeType)
{
#if ENABLE(ENCRYPTED_MEDIA_V2)
    return wkcMediaPlayerIsSupportedKeySystemPeer(keySystem.utf8().data(), mimeType.utf8().data());
#else
    return false;
#endif
}

bool MediaPlayerPrivateWKC::isAvailable()
{
    notImplemented();
    return true;
}

// interfaces
void
MediaPlayerPrivateWKC::load(const String& url)
{
    if (!canManipulatePlayer())
        return;

#if ENABLE(MEDIA_STREAM)
    MediaStream* desc = (MediaStream *)MediaStreamRegistry::registry().lookup(url);
    if (desc) {
        if (desc->getVideoTracks().size() > 0) {
            (void)wkcMediaPlayerLoadPeer(m_peer, desc->getVideoTracks().at(0)->source()->id().utf8().data(), WKC_MEDIA_TYPE_MEDIASTREAM);
            return;
        } else if (desc->getAudioTracks().size() > 0) {
            (void)wkcMediaPlayerLoadPeer(m_peer, desc->getAudioTracks().at(0)->source()->id().utf8().data(), WKC_MEDIA_TYPE_MEDIASTREAM);
            return;
        }
    }
#endif
    wkcMediaPlayerSetCurlVerbosePeer(m_peer, InspectorInstrumentationPublic::hasFrontends());
    (void)wkcMediaPlayerLoadPeer(m_peer, url.utf8().data(), WKC_MEDIA_TYPE_NORMAL);

    static const char* kSpliter = "://";
    size_t splitPos = url.find(kSpliter);
    if (splitPos != notFound) {
        m_protocol = (url.left(splitPos) + kSpliter);
    }
}
#if ENABLE(MEDIA_SOURCE)
void
MediaPlayerPrivateWKC::load(const URL& url, const ContentType& type, MediaSourcePrivateClient* client)
{
    if (!canManipulatePlayer())
        return;

    client->setPrivateAndOpen(adoptRef(*MediaSourcePrivateWKC::create(this, m_peer)));

    (void)wkcMediaPlayerLoadPeer(m_peer, url.string().utf8().data(), WKC_MEDIA_TYPE_MEDIASOURCE);
}
#endif
#if ENABLE(MEDIA_STREAM)
void
MediaPlayerPrivateWKC::load(WebCore::MediaStreamPrivate&)
{
    notImplemented();
}
#endif
void
MediaPlayerPrivateWKC::cancelLoad()
{
    if (!canManipulatePlayer())
        return;

    wkcMediaPlayerCancelLoadPeer(m_peer);
}

void
MediaPlayerPrivateWKC::prepareToPlay()
{
    if (!canManipulatePlayer())
        return;

    (void)wkcMediaPlayerPrepareToPlayPeer(m_peer);
}

bool
MediaPlayerPrivateWKC::didLoadingProgress() const
{
    int items = wkcMediaPlayerBufferedRangesPeer(m_peer);
    if (!items)
        return false;

    float buffered = 0.0;
    for (int i=0; i<items; i++) {
        float start=0, end=0;
        if (wkcMediaPlayerBufferedRangePeer(m_peer, i, &start, &end)==WKC_MEDIA_ERROR_OK) {
            buffered += end - start;
        }
    }
    bool ret = buffered != m_lastBuffered;
    m_lastBuffered = buffered;

    return ret;
}

void
MediaPlayerPrivateWKC::play()
{
    if (!canManipulatePlayer())
        return;

    (void)wkcMediaPlayerPlayPeer(m_peer);
}
void
MediaPlayerPrivateWKC::pause()
{
    if (!canManipulatePlayer())
        return;

    (void)wkcMediaPlayerPausePeer(m_peer);
}

bool
MediaPlayerPrivateWKC::supportsFullscreen() const
{
    return wkcMediaPlayerSupportsFullScreenPeer(m_peer);
}
bool
MediaPlayerPrivateWKC::canSaveMediaData() const
{
    return wkcMediaPlayerCanSaveMediaDataPeer(m_peer);
}

FloatSize
MediaPlayerPrivateWKC::naturalSize() const
{
    WKCSize size = {0};
    if (wkcMediaPlayerNaturalSizePeer(m_peer, &size)==WKC_MEDIA_ERROR_OK) {
        return IntSize(size.fWidth, size.fHeight);
    }
    return IntSize(0,0);
}

bool
MediaPlayerPrivateWKC::hasVideo() const
{
    return wkcMediaPlayerHasVideoPeer(m_peer);
}
bool
MediaPlayerPrivateWKC::hasAudio() const
{
    return wkcMediaPlayerHasAudioPeer(m_peer);
}

void
MediaPlayerPrivateWKC::setPageIsVisible(bool flag)
{
    (void)wkcMediaPlayerSetPageIsVisiblePeer(m_peer, flag);
}

float
MediaPlayerPrivateWKC::duration() const
{
    return wkcMediaPlayerDurationPeer(m_peer);
}

double
MediaPlayerPrivateWKC::durationDouble() const
{
    return wkcMediaPlayerDurationDoublePeer(m_peer);
}

float
MediaPlayerPrivateWKC::currentTime() const
{
    return wkcMediaPlayerCurrentTimePeer(m_peer);
}

double
MediaPlayerPrivateWKC::currentTimeDouble() const
{
    return wkcMediaPlayerCurrentTimeDoublePeer(m_peer);
}

void
MediaPlayerPrivateWKC::seek(float in_time)
{
    if (!canManipulatePlayer()) {
        m_player->seekEnd();
        return;
    }

    (void)wkcMediaPlayerSeekPeer(m_peer, in_time);
}
bool
MediaPlayerPrivateWKC::seeking() const
{
    return wkcMediaPlayerIsSeekingPeer(m_peer);
}

WTF::MediaTime
MediaPlayerPrivateWKC::startTime() const
{
    WTF::MediaTime ret = WTF::MediaTime::createWithFloat(wkcMediaPlayerStartTimePeer(m_peer));
    return ret;
}

double
MediaPlayerPrivateWKC::rate() const
{
    return wkcMediaPlayerRatePeer(m_peer);
}

void
MediaPlayerPrivateWKC::setRate(float rate)
{
    if (!canManipulatePlayer())
        return;

    (void)wkcMediaPlayerSetRatePeer(m_peer, rate);
}
void
MediaPlayerPrivateWKC::setPreservesPitch(bool flag)
{
    if (!canManipulatePlayer())
        return;

    (void)wkcMediaPlayerSetPreservesPitchPeer(m_peer, flag);
}

bool
MediaPlayerPrivateWKC::paused() const
{
    return wkcMediaPlayerIsPausedPeer(m_peer);
}

float
MediaPlayerPrivateWKC::volume() const
{
    if (m_muted)
        return 0.f;

    if (m_audiopeer) {
        return wkcAudioGetVolumePeer(m_audiopeer);
    } else {
        return wkcMediaPlayerVolumePeer(m_peer);
    }
}

void
MediaPlayerPrivateWKC::setVolume(float vol)
{
    if (!canManipulatePlayer())
        return;

    if (m_muted)
        return;

    if (vol<0) vol = 0;
    if (vol>1.f) vol = 1.f;

    m_volume = vol;

    if (m_audiopeer) {
        (void)wkcAudioSetVolumePeer(m_audiopeer, vol);
    } else {
        (void)wkcMediaPlayerSetVolumePeer(m_peer, vol);
    }
}

void
MediaPlayerPrivateWKC::setMuted(bool muted)
{
    if (!canManipulatePlayer())
        return;

    m_muted = muted;

    float vol = muted ? 0 : m_volume;
    if (m_audiopeer) {
        (void)wkcAudioSetVolumePeer(m_audiopeer, vol);
    } else {
        (void)wkcMediaPlayerSetVolumePeer(m_peer, vol);
    }
}

bool
MediaPlayerPrivateWKC::hasClosedCaptions() const
{
    return wkcMediaPlayerHasClosedCaptionsPeer(m_peer);
}
void
MediaPlayerPrivateWKC::setClosedCaptionsVisible(bool flag)
{
    (void)wkcMediaPlayerSetClosedCaptionsVisiblePeer(m_peer, flag);
}

MediaPlayer::NetworkState
MediaPlayerPrivateWKC::networkState() const
{
    const int ret = wkcMediaPlayerNetworkStatePeer(m_peer);
    switch (ret) {
    case WKC_MEDIA_NETWORKSTATE_EMPTY:
        return MediaPlayer::NetworkState::Empty;
    case WKC_MEDIA_NETWORKSTATE_IDLE:
        return MediaPlayer::NetworkState::Idle;
    case WKC_MEDIA_NETWORKSTATE_LOADING:
        return MediaPlayer::NetworkState::Loading;
    case WKC_MEDIA_NETWORKSTATE_LOADED:
        return MediaPlayer::NetworkState::Loaded;
    case WKC_MEDIA_NETWORKSTATE_FORMATERROR:
        return MediaPlayer::NetworkState::FormatError;
    case WKC_MEDIA_NETWORKSTATE_DECODEERROR:
        return MediaPlayer::NetworkState::DecodeError;
    case WKC_MEDIA_NETWORKSTATE_NETWORKERROR:
    default:
        return MediaPlayer::NetworkState::NetworkError;
    }
}

MediaPlayer::ReadyState
MediaPlayerPrivateWKC::readyState() const
{
    const int ret = wkcMediaPlayerReadyStatePeer(m_peer);
    switch (ret) {
    case WKC_MEDIA_READYSTATE_HAVE_METADATA:
        return MediaPlayer::ReadyState::HaveMetadata;
    case WKC_MEDIA_READYSTATE_HAVE_CURRENTDATA:
        return MediaPlayer::ReadyState::HaveCurrentData;
    case WKC_MEDIA_READYSTATE_HAVE_FUTUREDATA:
        return MediaPlayer::ReadyState::HaveFutureData;
    case WKC_MEDIA_READYSTATE_HAVE_ENOUGHDATA:
        return MediaPlayer::ReadyState::HaveEnoughData;
    case WKC_MEDIA_READYSTATE_HAVE_NOTHING:
    default:
        return MediaPlayer::ReadyState::HaveNothing;
    }
}

float
MediaPlayerPrivateWKC::maxTimeSeekable() const
{
    return wkcMediaPlayerMaxTimeSeekablePeer(m_peer);
}

std::unique_ptr<PlatformTimeRanges>
MediaPlayerPrivateWKC::buffered() const
{
    std::unique_ptr<PlatformTimeRanges> range = std::make_unique<PlatformTimeRanges>();
    int items = wkcMediaPlayerBufferedRangesPeer(m_peer);
    for (int i=0; i<items; i++) {
        float start=0, end=0;
        if (wkcMediaPlayerBufferedRangePeer(m_peer, i, &start, &end)==WKC_MEDIA_ERROR_OK) {
            MediaTime s = MediaTime::createWithFloat(start);
            MediaTime e = MediaTime::createWithFloat(end);
            range->add(s, e);
        }
    }
    return range;
}

double
MediaPlayerPrivateWKC::maximumDurationToCacheMediaTime() const
{
    return 0;
}

unsigned long long
MediaPlayerPrivateWKC::totalBytes() const
{
    return wkcMediaPlayerTotalBytesPeer(m_peer);
}

void
MediaPlayerPrivateWKC::setSize(const IntSize& isize)
{
    m_size = isize;

    WKCSize size = { isize.width(), isize.height() };

    int ret = wkcMediaPlayerSetSizePeer(m_peer, &size);
    if (ret==WKC_MEDIA_ERROR_NOTSUPPORTED) {
        m_resizeAtDrawing = true;
    } else {
        m_resizeAtDrawing = false;
    }
}

void
MediaPlayerPrivateWKC::paint(GraphicsContext& ctx, const FloatRect& r)
{
    nxLog_in("ctx:%p r:%dx%d.%dx%d", &ctx, r.x(), r.y(), r.width(), r.height());

    int type = wkcMediaPlayerVideoSinkTypePeer(m_peer);

    switch (type) {
    case WKC_MEDIA_VIDEOSINKTYPE_BITMAP:
    {
        int fmt = 0;
        int rowbytes = 0;
        void* mask = 0;
        int maskrowbytes = 0;
        WKCSize wsize = {0};
        void* bitmap = wkcMediaPlayerLockImagePeer(m_peer, &fmt, &rowbytes, &mask, &maskrowbytes, &wsize);

        if (!bitmap)
            break;

        if ((fmt&WKC_IMAGETYPE_TYPEMASK) == WKC_IMAGETYPE_ARGB8888) {
            const IntSize size(wsize.fWidth, wsize.fHeight);
            RefPtr<Image> img = PlatformImageHelper::createImage(reinterpret_cast<unsigned char*>(bitmap), wsize.fWidth, wsize.fHeight);
            if (!img)
                CRASH_WITH_NO_MEMORY();

            if (m_resizeAtDrawing) {
                const FloatRect dr(r.location(), m_size);
                const FloatRect sr(FloatPoint(), size);
                ctx.drawImage(*img, dr, sr);
            } else {
                ctx.drawImage(*img, r.location());
            }
        }

        wkcMediaPlayerUnlockImagePeer(m_peer, bitmap);
        break;
    }

    case WKC_MEDIA_VIDEOSINKTYPE_WINDOW:
    {
        const WKCRect wr = { r.x(), r.y(), r.width(), r.height() };
        (void)wkcMediaPlayerSetWindowPositionPeer(m_peer, &wr);
        break;
    }

    case WKC_MEDIA_VIDEOSINKTYPE_HOLEDWINDOW:
    {
        AffineTransform m = ctx.getCTM();
        const FloatQuad q = m.mapQuad(FloatRect(r));
        if (!q.isEmpty()) {
            const FloatRect mr = q.boundingBox();
            const WKCRect wr = { mr.x(), mr.y(), mr.width(), mr.height() };
            (void)wkcMediaPlayerSetWindowPositionPeer(m_peer, &wr);
            ctx.clearRect(r);
        }
        break;
    }

    case WKC_MEDIA_VIDEOSINKTYPE_LAYER:
    {
        break;
    }

    default:
        nxLog_w("unknown media video sink type");
        break;
    }

    nxLog_out("");
}

void
MediaPlayerPrivateWKC::paintCurrentFrameInContext(GraphicsContext& c, const FloatRect& r)
{
    nxLog_in("");

    int type = wkcMediaPlayerVideoSinkTypePeer(m_peer);

    if (type!=WKC_MEDIA_VIDEOSINKTYPE_BITMAP) {
        notImplemented();
        nxLog_noimp();
        return;
    }

    paint(c, r);

    nxLog_out("");
}

DestinationColorSpace
MediaPlayerPrivateWKC::colorSpace()
{
    return DestinationColorSpace::SRGB();
}

void
MediaPlayerPrivateWKC::setPreload(MediaPlayer::Preload type)
{
    int t;
    switch (type) {
    case MediaPlayer::Preload::None:
        t = WKC_MEDIA_PRELOAD_NONE;
        break;
    case MediaPlayer::Preload::MetaData:
        t = WKC_MEDIA_PRELOAD_METADATA;
        break;
    case MediaPlayer::Preload::Auto:
        t = WKC_MEDIA_PRELOAD_AUTO;
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }
    (void)wkcMediaPlayerSetPreloadPeer(m_peer, t);
}

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
void
MediaPlayerPrivateWKC::deliverNotification(MediaPlayerProxyNotificationType)
{
}
void
MediaPlayerPrivateWKC::setMediaPlayerProxy(WebMediaPlayerProxy*)
{
}
#endif

void
MediaPlayerPrivateWKC::enterFullscreen()
{
    wkcMediaPlayerEnterFullscreenPeer(m_peer);
}

void
MediaPlayerPrivateWKC::exitFullscreen()
{
    wkcMediaPlayerExitFullscreenPeer(m_peer);
}

bool
MediaPlayerPrivateWKC::canEnterFullscreen() const
{
    return wkcMediaPlayerCanEnterFullscreenPeer(m_peer);
}

bool
MediaPlayerPrivateWKC::supportsAcceleratedRendering() const
{
    if (wkcMediaPlayerVideoSinkTypePeer(m_peer)!=WKC_MEDIA_VIDEOSINKTYPE_LAYER)
        return false;
    return true;
}

void
MediaPlayerPrivateWKC::acceleratedRenderingStateChanged()
{
    wkcMediaPlayerNotifyVideoLayerRenderingStateChangedPeer(m_peer);
}

PlatformLayer*
MediaPlayerPrivateWKC::platformLayer() const
{
    return (PlatformLayer *)wkcMediaPlayerVideoLayerPeer(m_peer);
}

bool
MediaPlayerPrivateWKC::hasSingleSecurityOrigin() const
{
    return wkcMediaPlayerHasSingleSecurityOriginPeer(m_peer);
}

MediaPlayer::MovieLoadType
MediaPlayerPrivateWKC::movieLoadType() const
{
    const int type = wkcMediaPlayerMovieLoadTypePeer(m_peer);
    switch (type) {
    case WKC_MEDIA_MOVIELOADTYPE_DOWNLOAD:
        return MediaPlayer::MovieLoadType::Download;
    case WKC_MEDIA_MOVIELOADTYPE_STOREDSTREAM:
        return MediaPlayer::MovieLoadType::StoredStream;
    case WKC_MEDIA_MOVIELOADTYPE_LIVESTREAM:
        return MediaPlayer::MovieLoadType::LiveStream;
    case WKC_MEDIA_MOVIELOADTYPE_UNKNOWN:
    default:
        return MediaPlayer::MovieLoadType::Unknown;
    }
}

void
MediaPlayerPrivateWKC::setLoop(bool loop)
{
    wkcMediaPlayerSetLoopPeer(m_peer, loop);
}

void
MediaPlayerPrivateWKC::removedFromDocument()
{
    wkcMediaPlayerRemovedFromDocumentPeer(m_peer);
}

void
MediaPlayerPrivateWKC::willBecomeInactive()
{
    wkcMediaPlayerWillBecomeInactivePeer(m_peer);
}

void*
MediaPlayerPrivateWKC::platformPlayer() const
{
    return m_peer;
}

std::optional<VideoPlaybackQualityMetrics>
MediaPlayerPrivateWKC::videoPlaybackQualityMetrics()
{
    auto wkc = wkcMediaPlayerVideoPlaybackQualityMetricsPeer(m_peer);
    VideoPlaybackQualityMetrics metrics;
    metrics.totalVideoFrames = wkc.fTotalVideoFrames;
    metrics.droppedVideoFrames = wkc.fDroppedVideoFrames;
    return metrics;
}

// callbacks
void
MediaPlayerPrivateWKC::notifyPlayerState(int state)
{
    switch (state) {
    case WKC_MEDIA_PLAYERSTATE_NETWORKSTATECHANGED:
        m_player->networkStateChanged();
        // Request repaint to update media contols rendering immediately.
        m_player->repaint();
        break;
    case WKC_MEDIA_PLAYERSTATE_READYSTATECHANGED:
        m_player->readyStateChanged();
        break;
    case WKC_MEDIA_PLAYERSTATE_VOLUMECHANGED:
        m_player->volumeChanged(volume());
        break;
    case WKC_MEDIA_PLAYERSTATE_TIMECHANGED:
        m_player->timeChanged();
        break;
    case WKC_MEDIA_PLAYERSTATE_SIZECHANGED:
        m_player->sizeChanged();
        break;
    case WKC_MEDIA_PLAYERSTATE_RATECHANGED:
        m_player->rateChanged();
        break;
    case WKC_MEDIA_PLAYERSTATE_DURATIONCHANGED:
        m_player->durationChanged();
        break;
    case WKC_MEDIA_PLAYERSTATE_PLAYBACKSTATECHANGED:
        m_player->playbackStateChanged();
        // Request repaint to update media contols rendering immediately.
        m_player->repaint();
        break;
    case WKC_MEDIA_PLAYERSTATE_SEEKBEGIN:
        m_player->seekBegin();
        break;
    case WKC_MEDIA_PLAYERSTATE_SEEKEND:
        m_player->seekEnd();
        break;
    case WKC_MEDIA_PLAYERSTATE_RESETUSERGESTURERESTRICTION:
        m_player->resetUserGestureRestriction();
        break;
    default:
        break;
    }
}

void
MediaPlayerPrivateWKC::onCurlDebug(void* handle, int type, const char* data, unsigned int size)
{
    HTMLMediaElement* element = static_cast<HTMLMediaElement*>(&m_player->client());
    if (!element)
        return;

    Frame* frame = element->document().frame();
    if (!frame || !frame->loader().networkingContext())
        return;

    switch (type) {
    case CURLINFO_HEADER_OUT:
        didSendRequest(frame, handle, data, size);
        break;
    case CURLINFO_HEADER_IN:
        didReceiveResponse(frame, handle, data, size);
        break;
    case CURLINFO_DATA_IN:
        didReceiveData(frame, handle, data, size);
        break;
    default:
        break;
    }
}

void
MediaPlayerPrivateWKC::didSendRequest(Frame* frame, void* handle, const char* data, unsigned int size)
{
    auto it = m_networkInfo.find(handle);
    if (it != m_networkInfo.end()) {
        didFailLoading(frame, handle, ResourceError(String(), CURLE_RECV_ERROR, URL({}, String()), String("canceled")));
    }

    String message(data);
    Vector<String> lines = message.split("\r\n");
    if (lines.size() <= 1)
        return;

    String uri;
    String path;
    int requestBodySize = 0;
    ResourceRequest request;

    for (int i = 0; i < lines.size(); i++) {
        if (i == 0) {
            Vector<String> elements = lines[i].split(" ");
            if (elements.size() == 3) {
                request.setHTTPMethod(elements[0]);
                size_t splitPos = elements[1].find(m_protocol);
                if (splitPos == notFound)
                    path = elements[1];
                else
                    uri = elements[1];
                continue;
            }
            return;
        } else {
            size_t splitPos = lines[i].find(":");
            if (splitPos != notFound) {
                String name = lines[i].left(splitPos);
                String value = lines[i].substring(splitPos + 1).stripWhiteSpace();
                request.addHTTPHeaderField(name, value);
                if (uri.isEmpty() && equalIgnoringASCIICase(name, "Host")) {
                    uri.append(m_protocol);
                    uri.append(value);
                    uri.append(path);
                }
                if (equalIgnoringASCIICase(name, "Content-Length")) {
                    auto stoll = [](const char* str) {
                        errno = 0;
                        int64_t val = strtoll(str, nullptr, 10);
                        if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
                            || (errno != 0 && val == 0))
                            val = -1;
                        return val;
                    };
                    requestBodySize = stoll(value.utf8().data());
                    if (requestBodySize < 0)
                        return;
                }
            }
        }
    }
    if (uri.isEmpty())
        return;
    request.setURL(URL({ }, uri));

    auto identifier = ResourceLoaderIdentifier::generate();
    ResourceResponse res;
    InspectorInstrumentation::willSendRequest(frame, identifier, nullptr, request, res, nullptr);

    NetworkInfo info;
    info.m_identifier = identifier;
    info.m_requestHeaderSize = size;
    info.m_requestBodySize = requestBodySize;
    m_networkInfo.set(handle, info);
}

void
MediaPlayerPrivateWKC::didReceiveResponse(Frame* frame, void* handle, const char* data, unsigned int size)
{
    auto it = m_networkInfo.find(handle);
    if (it == m_networkInfo.end())
        return;

    String message(data);
    Vector<String> lines = message.split("\r\n");
    if (lines.size() <= 1)
        return;
    
    auto isHttp2 = [](const String& str) {
        return ((notFound != str.findIgnoringASCIICase("http/2")) ? true : false);
    };
    auto stoll = [](const char* str) {
        errno = 0;
        int64_t val = strtoll(str, nullptr, 10);
        if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
            || (errno != 0 && val == 0))
            val = -1;
        return val;
    };
    int statusCode = 0;
    bool found = false;
    auto& info = it->value;
    info.m_contentLength = 0;
    info.m_receivedLength = 0;
    ResourceResponse response;

    for (int i = 0; i < lines.size(); i++) {
        if (i == 0) {
            size_t splitPos = lines[i].find(" ");
            if (splitPos != notFound) {
                response.setHTTPVersion(lines[i].left(splitPos));
                String right = lines[i].substring(splitPos + 1).stripWhiteSpace();
                size_t splitPos = right.find(" ");
                if (splitPos != notFound) {
                    statusCode = stoll(right.left(splitPos).utf8().data());
                    if (statusCode > 0) {
                        response.setHTTPStatusCode(statusCode);
                        response.setHTTPStatusText(right.substring(splitPos + 1).stripWhiteSpace());
                        continue;
                    }
                } else if (isHttp2(response.httpVersion())) {
                    statusCode = stoll(right.utf8().data());
                    if (statusCode > 0) {
                        response.setHTTPStatusCode(statusCode);
                        continue;
                    }
                }
            } else {
                // I don't think this is the case, but RFC 7540 8.1.2.4 states
                // HTTP/2 does not define a way to carry the version or reason phrase
                // that is included in an HTTP/1.1 status line.
                statusCode = stoll(lines[i].utf8().data());
                if (statusCode > 0) {
                    response.setHTTPVersion("http/2");
                    response.setHTTPStatusCode(statusCode);
                    continue;
                }
            }
            return;
        } else {
            size_t splitPos = lines[i].find(":");
            if (splitPos != notFound) {
                String name = lines[i].left(splitPos);
                String value = lines[i].substring(splitPos + 1).stripWhiteSpace();
                response.addHTTPHeaderField(name, value);
                if (!found && equalIgnoringASCIICase(name, "Content-Length")) {
                    info.m_contentLength = stoll(value.utf8().data());
                    if (info.m_contentLength < 0)
                        return;
                    found = true;
                }
                if (equalIgnoringASCIICase(name, "Content-Range")) {
                    splitPos = value.find(" ");
                    if (splitPos != notFound)
                        value = value.substring(splitPos + 1).stripWhiteSpace();
                    splitPos = value.find("/");
                    if (splitPos != notFound) {
                        String range = value.left(splitPos);
                        splitPos = range.find("-");
                        if (splitPos != notFound) {
                            int64_t start = stoll(range.left(splitPos).utf8().data());
                            int64_t end = stoll(range.substring(splitPos + 1).utf8().data());
                            if (start >= 0 && end >= 0 && start < end) {
                                info.m_contentLength = (end - start + 1);
                                found = true;
                            }
                        }
                    }
                }
            }
        }
    }

    InspectorInstrumentation::didReceiveResourceResponse(*frame, info.m_identifier, nullptr, response, nullptr);
    info.m_httpVersion = response.httpVersion().convertToASCIILowercase();
    info.m_responseHeaderSize = size;
    if (info.m_contentLength == 0) {
        didFinishLoading(frame, handle);
    }
}

void
MediaPlayerPrivateWKC::didReceiveData(Frame* frame, void* handle, const char* data, unsigned int size)
{
    auto it = m_networkInfo.find(handle);
    if (it == m_networkInfo.end())
        return;
    auto& info = it->value;
    if (info.m_contentLength == 0 || size == 0)
        return;

    info.m_receivedLength += size;
    InspectorInstrumentation::didReceiveData(frame, info.m_identifier, nullptr, size);
    if (info.m_receivedLength >= info.m_contentLength) {
        didFinishLoading(frame, handle);
    }
}

void
MediaPlayerPrivateWKC::didFinishLoading(Frame* frame, void* handle)
{
    auto it = m_networkInfo.find(handle);
    if (it == m_networkInfo.end())
        return;
    auto& info = it->value;

    NetworkLoadMetrics metrics;
    // see ALPN Protocol ID(RFC7301)
    if (info.m_httpVersion.find("http/3") != notFound)
        metrics.protocol = "h3";
    else if (info.m_httpVersion.find("http/2") != notFound)
        metrics.protocol = "h2";
    else
        metrics.protocol = info.m_httpVersion;
    metrics.responseBodyBytesReceived = info.m_receivedLength;
    auto additionalMetrics = AdditionalNetworkLoadMetricsForWebInspector::create();
    additionalMetrics->requestHeaderBytesSent = info.m_requestHeaderSize;
    additionalMetrics->requestBodyBytesSent = info.m_requestBodySize;
    additionalMetrics->responseHeaderBytesReceived = info.m_responseHeaderSize;
    metrics.additionalNetworkLoadMetricsForWebInspector = WTFMove(additionalMetrics);
    InspectorInstrumentation::didFinishLoading(frame, nullptr, info.m_identifier, metrics, nullptr);

    m_networkInfo.remove(it);
}

void
MediaPlayerPrivateWKC::didFailLoading(Frame* frame, void* handle, const ResourceError& error)
{
    auto it = m_networkInfo.find(handle);
    if (it == m_networkInfo.end())
        return;
    auto& info = it->value;

    InspectorInstrumentation::didFailLoading(frame, nullptr, info.m_identifier, error);

    m_networkInfo.remove(it);
}

void
MediaPlayerPrivateWKC::notifyRequestInvalidate(int x, int y, int w, int h)
{
    if (w<0 &&h<0) {
        // full invalidate
        m_player->repaint();
    } else {
        m_player->repaint();
    }
}

void
MediaPlayerPrivateWKC::notifyAudioDataAvailable(float timing, int len)
{
    if (!m_audiopeer) {
        bool hasAudio = wkcMediaPlayerHasAudioPeer(m_peer);
        int atype = wkcMediaPlayerAudioSinkTypePeer(m_peer);

        if (!m_audiopeer && hasAudio && atype==WKC_MEDIA_AUDIOSINKTYPE_BINARYSTREAM) {
            int rate = wkcMediaPlayerAudioRatePeer(m_peer);
            int ch = wkcMediaPlayerAudioChannelsPeer(m_peer);
            int endian = 0;
            int bps = wkcMediaPlayerAudioBitsPerSamplePeer(m_peer, &endian);
            if (rate>0 && ch>0 && bps>0) {
                m_audiopeer = wkcAudioOpenPeer(rate, bps, ch, endian);
                if (!m_audiopeer)
                    return;
            }
        }
    }

    unsigned int olen = 0;
    unsigned int consumed = 0;
    void* buf = wkcMediaPlayerLockAudioPeer(m_peer, &olen);
    if (buf && olen) {
        consumed = wkcAudioWritePeer(m_audiopeer, buf, olen);
    }
    wkcMediaPlayerUnlockAudioPeer(m_peer, consumed);
}

#if ENABLE(ENCRYPTED_MEDIA)
MediaPlayer::MediaKeyException
MediaPlayerPrivateWKC::generateKeyRequest(const String& keySystem, const unsigned char* initData, unsigned initDataLength)
{
    /* return m_private->generateKeyRequest(keySystem.lower(), initData, initDataLength); */
    int ret = wkcMediaPlayerGenerateKeyRequestPeer(m_peer, keySystem.lower().utf8().data(), initData, initDataLength);
    switch (ret) {
    case WKC_MEDIA_KEYEXCEPTION_NOERROR:
        return MediaPlayer::NoError;
    case WKC_MEDIA_KEYEXCEPTION_INVALIDPLAYERSTATE:
        return MediaPlayer::InvalidPlayerState;
    case WKC_MEDIA_KEYEXCEPTION_NOTSUPPORTED:
        return MediaPlayer::KeySystemNotSupported;
    case WKC_MEDIA_KEYEXCEPTION_INVALIDACCESS:
        return MediaPlayer::InvalidAccess;
    default:
        return MediaPlayer::NoError;
    }
}

MediaPlayer::MediaKeyException
MediaPlayerPrivateWKC::addKey(const String& keySystem, const unsigned char* key, unsigned keyLength, const unsigned char* initData, unsigned initDataLength, const String& sessionId)
{
    int ret = wkcMediaPlayerAddKeyPeer(m_peer, keySystem.lower().utf8().data(), key, keyLength, initData, initDataLength, sessionId.utf8().data());
    switch (ret) {
    case WKC_MEDIA_KEYEXCEPTION_NOERROR:
        return MediaPlayer::NoError;
    case WKC_MEDIA_KEYEXCEPTION_INVALIDPLAYERSTATE:
        return MediaPlayer::InvalidPlayerState;
    case WKC_MEDIA_KEYEXCEPTION_NOTSUPPORTED:
        return MediaPlayer::KeySystemNotSupported;
    case WKC_MEDIA_KEYEXCEPTION_INVALIDACCESS:
        return MediaPlayer::InvalidAccess;
    default:
        return MediaPlayer::NoError;
    }
}

MediaPlayer::MediaKeyException
MediaPlayerPrivateWKC::cancelKeyRequest(const String& keySystem, const String& sessionId)
{
    int ret = wkcMediaPlayerCancelKeyRequestPeer(m_peer, keySystem.lower().utf8().data(), sessionId.utf8().data());
    switch (ret) {
    case WKC_MEDIA_KEYEXCEPTION_NOERROR:
        return MediaPlayer::NoError;
    case WKC_MEDIA_KEYEXCEPTION_INVALIDPLAYERSTATE:
        return MediaPlayer::InvalidPlayerState;
    case WKC_MEDIA_KEYEXCEPTION_NOTSUPPORTED:
        return MediaPlayer::KeySystemNotSupported;
    default:
        return MediaPlayer::NoError;
    }
}
#endif /* ENCRYPTED_MEDIA */

int
MediaPlayerPrivateWKC::notifyPlayerStreamOpenURI(const char* uri, const char* method, const WKCMediaPlayerStreamHeaderInfo* headers, int len, const WKCMediaPlayerStreamStateProcs* procs)
{
    HTMLMediaElement* element = static_cast<HTMLMediaElement*>(&m_player->client());
    if (!element)
        return -1;

    Frame* frame = element->document().frame();
    if (!frame || !frame->loader().networkingContext())
        return -1;

    ResourceRequest req(uri);
    req.setHTTPMethod(method);
    for (int i = 0; i < len; i++) {
        req.addHTTPHeaderField(headers[i].fName, headers[i].fValue);
    }
    auto handle = WebCore::ResourceHandle::create(frame->loader().networkingContext(), req, this, false, false, false, nullptr, false);
    if (!handle)
        return -1;

    httpConnection c = {0};
    c.m_connection = handle;
    c.m_procs = *procs;

    WKC_DEFINE_STATIC_INT(gPlayerStreamsCount, 0);
    c.m_id = ++gPlayerStreamsCount;

    m_httpConnections.append(c);

    return c.m_id;
}

MediaPlayerPrivateWKC::httpConnection*
MediaPlayerPrivateWKC::findConnection(int id)
{
    int count = m_httpConnections.size();
    for (int i=0; i<count; i++) {
        if (m_httpConnections.at(i).m_id==id) {
            return &m_httpConnections.at(i);
        }
    }
    return 0;
}

MediaPlayerPrivateWKC::httpConnection*
MediaPlayerPrivateWKC::findConnection(ResourceHandle* h)
{
    int count = m_httpConnections.size();
    for (int i=0; i<count; i++) {
        if (m_httpConnections.at(i).m_connection.get()==h) {
            return &m_httpConnections.at(i);
        }
    }
    return 0;
}

void
MediaPlayerPrivateWKC::removeConnection(ResourceHandle* h)
{
    int count = m_httpConnections.size();
    for (int i=0; i<count; i++) {
        if (m_httpConnections.at(i).m_connection.get()==h) {
            m_httpConnections.remove(i);
            return;
        }
    }
}

int
MediaPlayerPrivateWKC::notifyPlayerStreamClose(int id)
{
    httpConnection* c = findConnection(id);
    if (!c)
        return -1;
    auto h = c->m_connection;
    if (!h)
        return -1;
    h->cancel();
    removeConnection(h.get());
    return id;
}

int
MediaPlayerPrivateWKC::notifyPlayerStreamCancel(int id)
{
    httpConnection* c = findConnection(id);
    if (!c)
        return -1;
    auto h = c->m_connection;
    if (!h)
        return -1;
    h->cancel();
    removeConnection(h.get());
    return id;
}

void
MediaPlayerPrivateWKC::didReceiveData(ResourceHandle* h, const SharedBuffer& data, int /*encodedDataLength*/)
{
    httpConnection* c = findConnection(h);
    if (!c)
        return;
    if (c->m_procs.fReceiveDataProc)
        (*c->m_procs.fReceiveDataProc)(c->m_procs.fInstance, c->m_id, reinterpret_cast<const unsigned char *>(data.data()), data.size());
}

void
MediaPlayerPrivateWKC::didFinishLoading(ResourceHandle* h, const NetworkLoadMetrics&)
{
    httpConnection* c = findConnection(h);
    if (!c)
        return;
    if (c->m_procs.fFinishProc)
        (*c->m_procs.fFinishProc)(c->m_procs.fInstance, c->m_id);
}

void
MediaPlayerPrivateWKC::didFail(ResourceHandle* h, const ResourceError& error)
{
    httpConnection* c = findConnection(h);
    if (!c)
        return;
    if (c->m_procs.fErrorProc)
        (*c->m_procs.fErrorProc)(c->m_procs.fInstance, c->m_id, error.errorCode());
}

void
MediaPlayerPrivateWKC::willSendRequestAsync(ResourceHandle*, ResourceRequest&& request, ResourceResponse&&, CompletionHandler<void(ResourceRequest&&)>&& completionHandler)
{
    completionHandler(WTFMove(request));
}

void
MediaPlayerPrivateWKC::didReceiveResponseAsync(ResourceHandle* h, ResourceResponse&& rsp, CompletionHandler<void()>&& completion)
{
    httpConnection* c = findConnection(h);
    if (!c)
        return;

    if (!c->m_procs.fReceiveResponseProc)
        return;

    WKC::ResourceResponsePrivate priv(rsp);
    c->m_procs.fReceiveResponseProc(c->m_procs.fInstance, c->m_id, reinterpret_cast<const void* const>(&priv.wkc()));
    completion();
}

bool
MediaPlayerPrivateWKC::chromeVisible()
{
    HTMLMediaElement* element = static_cast<HTMLMediaElement*>(&m_player->client());
    if (!element)
        return false;

    Document& document = element->document();
    if (!&document)
        return false;

    Frame* frame = document.frame();
    if (!frame || !frame->page() || !&frame->page()->chrome() || !frame->page()->chrome().chromeVisible())
        return false;

    return true;
}

bool
MediaPlayerPrivateWKC::canManipulatePlayer()
{
    if (networkState() == MediaPlayer::NetworkState::FormatError)
        return false;

    return true;
}

void*
MediaPlayerPrivateWKC::createHTMLMediaElement()
{
    HTMLMediaElement* element = static_cast<HTMLMediaElement*>(&m_player->client());
    if (!element)
        return 0;

    WKC::HTMLMediaElementPrivate* wkc_private_element = new WKC::HTMLMediaElementPrivate(element);
    if (!wkc_private_element)
        return 0;

    element->ref();

    return static_cast<void*>(&wkc_private_element->wkc());
}

void
MediaPlayerPrivateWKC::destroyHTMLMediaElement(void* p)
{
    if (!p)
        return;

    WKC::HTMLMediaElement* wkc_element = static_cast<WKC::HTMLMediaElement*>(p);
    WKC::HTMLMediaElementPrivate* wkc_private_element = static_cast<WKC::HTMLMediaElementPrivate*>(&wkc_element->priv());

    static_cast<HTMLMediaElement*>(wkc_private_element->webcore())->deref();

    delete(wkc_private_element);
}

} // namespace WebCore

#endif // ENABLE(VIDEO)

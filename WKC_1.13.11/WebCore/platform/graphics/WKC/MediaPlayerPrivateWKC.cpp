/*
 *  Copyright (c) 2011-2019 ACCESS CO., LTD. All rights reserved.
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
#include "Page.h"
#include "MediaStream.h"
#include "WTFString.h"
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
#include <wkc/wkcmediapeer.h>

// debug print
#define NO_NXLOG
#include "utils/nxDebugPrint.h"

#ifdef WKC_API
# undef WKC_API
# define WKC_API
#endif
#include "helpers/privates/WKCHTMLMediaElementPrivate.h"

namespace WebCore {

MediaPlayerPrivate::MediaPlayerPrivate(MediaPlayer* player)
    : m_player(player)
    , m_peer(0)
    , m_audiopeer(0)
    , m_resizeAtDrawing(false)
    , m_size()
    , m_httpConnections()
    , m_lastBuffered(0)
{
}

MediaPlayerPrivate::~MediaPlayerPrivate()
{
    while (m_httpConnections.size()) {
        httpConnection& c = m_httpConnections.at(0);
        ResourceHandle* h = static_cast<ResourceHandle *>(c.m_connection);
        if (h) {
            h->cancel();
            h->deref();
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
}

std::unique_ptr<MediaPlayerPrivateInterface>
MediaPlayerPrivate::create(MediaPlayer* player)
{
    std::unique_ptr<MediaPlayerPrivateInterface> self = std::make_unique<MediaPlayerPrivate>(player);
    if (!self)
        return nullptr;
    if (!((MediaPlayerPrivate*)(self.get()))->construct()) {
        // should not return 0.
        wkcMemoryNotifyNoMemoryPeer(sizeof(MediaPlayerPrivate));
        return nullptr;
    }
    return self;
}

bool
MediaPlayerPrivate::construct()
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
    };
    const WKCMediaPlayerStreamCallbacks scb = {
        (WKCMPSGetProxyInfoProc)notifyPlayerStreamGetProxyProc,
        (WKCMPSGetCookieProc)notifyPlayerStreamGetCookieProc,
        (WKCMPSOpenURIProc)notifyPlayerStreamOpenURIProc,
        (WKCMPSCloseProc)notifyPlayerStreamCloseProc,
        (WKCMPSCancelProc)notifyPlayerStreamCancelProc,
    };

    m_peer = wkcMediaPlayerCreatePeer(&cb, &scb, (void *)this);
    if (!m_peer)
        return false;
    return true;
}

void
MediaPlayerPrivate::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable()) {
        registrar(create, getSupportedTypes, supportsType, originsInMediaCache, clearMediaCache, clearMediaCacheForOrigins, supportsKeySystem);
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
MediaPlayerPrivate::getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types)
{
    types = mimeTypeCache();
}

MediaPlayer::SupportsType
MediaPlayerPrivate::supportsType(const MediaEngineSupportParameters& parameters)
{
    if (parameters.type.isEmpty())
        return MediaPlayer::IsNotSupported;

    if (mimeTypeCache().contains(parameters.type.containerType()))
        return parameters.type.codecs().isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported;

    return MediaPlayer::IsNotSupported;
}

#if ENABLE(ENCRYPTED_MEDIA)
void MediaPlayerPrivate::notifyKeyNeeded(const char *in_keySystem, const char *in_sessionID, const unsigned char *in_initData, unsigned in_initDataLen)
{
    if (!m_player) {
        return;
    }

    // Conversion
    String strkeySystem = in_keySystem;
    String strSessionID = in_sessionID;

    m_player->keyNeeded(strkeySystem, strSessionID, in_initData, in_initDataLen);
}

void MediaPlayerPrivate::notifyKeyRequest(const char *in_keySystem, const char *in_sessionID, const unsigned char *in_binaryBuff, unsigned in_len, const char *in_defaultURL)
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

void MediaPlayerPrivate::notifyKeyAdded(const char *in_keySystem, const char *in_sessionID)
{
    if (!m_player) {
        return;
    }

    // Conversion
    String strkeySystem = in_keySystem;
    String strSessionID = in_sessionID;

    m_player->keyAdded(strkeySystem, strSessionID);
}

void MediaPlayerPrivate::notifyKeyError(const char *in_keySystem, const char *in_sessionID, int in_keyErrorCode, unsigned short in_systemCode)
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
MediaPlayerPrivate::originsInMediaCache(const String&)
{
    notImplemented();
    return { };
}

void
MediaPlayerPrivate::clearMediaCache(const String&, WallTime)
{
    notImplemented();
}

void
MediaPlayerPrivate::clearMediaCacheForOrigins(const String&, const HashSet<RefPtr<SecurityOrigin>>&)
{
    notImplemented();
}

bool
MediaPlayerPrivate::supportsKeySystem(const String& keySystem, const String& mimeType)
{
#if ENABLE(ENCRYPTED_MEDIA_V2)
    return wkcMediaPlayerIsSupportedKeySystemPeer(keySystem.utf8().data(), mimeType.utf8().data());
#else
    return false;
#endif
}

bool MediaPlayerPrivate::isAvailable() 
{
    notImplemented();
    return true;
}

// interfaces
void
MediaPlayerPrivate::load(const String& url)
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
    (void)wkcMediaPlayerLoadPeer(m_peer, url.utf8().data(), WKC_MEDIA_TYPE_NORMAL);
}
#if ENABLE(MEDIA_SOURCE)
void
MediaPlayerPrivate::load(const String& url, MediaSourcePrivateClient* client)
{
    if (!canManipulatePlayer())
        return;

    client->setPrivateAndOpen(adoptRef(*MediaSourcePrivateWKC::create(this, m_peer)));

    (void)wkcMediaPlayerLoadPeer(m_peer, url.utf8().data(), WKC_MEDIA_TYPE_MEDIASOURCE);
}
#endif
#if ENABLE(MEDIA_STREAM)
void
MediaPlayerPrivate::load(WebCore::MediaStreamPrivate&)
{
    notImplemented();
}
#endif
void
MediaPlayerPrivate::cancelLoad()
{
    if (!canManipulatePlayer())
        return;

    wkcMediaPlayerCancelLoadPeer(m_peer);
}
    
void
MediaPlayerPrivate::prepareToPlay()
{
    if (!canManipulatePlayer())
        return;

    (void)wkcMediaPlayerPrepareToPlayPeer(m_peer);
}

bool
MediaPlayerPrivate::didLoadingProgress() const
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
MediaPlayerPrivate::play()
{
    if (!canManipulatePlayer())
        return;

    (void)wkcMediaPlayerPlayPeer(m_peer);
}
void
MediaPlayerPrivate::pause()
{
    if (!canManipulatePlayer())
        return;

    (void)wkcMediaPlayerPausePeer(m_peer);
}

bool
MediaPlayerPrivate::supportsFullscreen() const
{
    return wkcMediaPlayerSupportsFullScreenPeer(m_peer);
}
bool
MediaPlayerPrivate::supportsSave() const
{
    return wkcMediaPlayerSupportsSavePeer(m_peer);
}

FloatSize
MediaPlayerPrivate::naturalSize() const
{
    WKCSize size = {0};
    if (wkcMediaPlayerNaturalSizePeer(m_peer, &size)==WKC_MEDIA_ERROR_OK) {
        return IntSize(size.fWidth, size.fHeight);
    }
    return IntSize(0,0);
}

bool
MediaPlayerPrivate::hasVideo() const
{
    return wkcMediaPlayerHasVideoPeer(m_peer);
}
bool
MediaPlayerPrivate::hasAudio() const
{
    return wkcMediaPlayerHasAudioPeer(m_peer);
}

void
MediaPlayerPrivate::setVisible(bool flag)
{
    (void)wkcMediaPlayerSetVisiblePeer(m_peer, flag);
}

float
MediaPlayerPrivate::duration() const
{
    return wkcMediaPlayerDurationPeer(m_peer);
}

double
MediaPlayerPrivate::durationDouble() const
{
    return wkcMediaPlayerDurationDoublePeer(m_peer);
}

float
MediaPlayerPrivate::currentTime() const
{
    return wkcMediaPlayerCurrentTimePeer(m_peer);
}

double
MediaPlayerPrivate::currentTimeDouble() const
{
    return wkcMediaPlayerCurrentTimeDoublePeer(m_peer);
}

void
MediaPlayerPrivate::seek(float in_time)
{
    if (!canManipulatePlayer()) {
        m_player->seekEnd();
        return;
    }

    (void)wkcMediaPlayerSeekPeer(m_peer, in_time);
}
bool
MediaPlayerPrivate::seeking() const
{
    return wkcMediaPlayerIsSeekingPeer(m_peer);
}

WTF::MediaTime
MediaPlayerPrivate::startTime() const
{
    WTF::MediaTime ret = WTF::MediaTime::createWithFloat(wkcMediaPlayerStartTimePeer(m_peer));
    return ret;
}

void
MediaPlayerPrivate::setEndTime(float in_time)
{
    (void)wkcMediaPlayerSetEndTimePeer(m_peer, in_time);
}

double
MediaPlayerPrivate::rate() const
{
    return wkcMediaPlayerRatePeer(m_peer);
}

void
MediaPlayerPrivate::setRate(float rate)
{
    if (!canManipulatePlayer())
        return;

    (void)wkcMediaPlayerSetRatePeer(m_peer, rate);
}
void
MediaPlayerPrivate::setPreservesPitch(bool flag)
{
    if (!canManipulatePlayer())
        return;

    (void)wkcMediaPlayerSetPreservesPitchPeer(m_peer, flag);
}

bool
MediaPlayerPrivate::paused() const
{
    return wkcMediaPlayerIsPausedPeer(m_peer);
}

void
MediaPlayerPrivate::setVolume(float vol)
{
    if (!canManipulatePlayer())
        return;

    if (vol<0) vol = 0;
    if (vol>1.f) vol = 1.f;
    if (m_audiopeer) {
        (void)wkcAudioSetVolumePeer(m_audiopeer, vol);
    } else {
        (void)wkcMediaPlayerSetVolumePeer(m_peer, vol);
    }
}

bool
MediaPlayerPrivate::hasClosedCaptions() const
{
    return wkcMediaPlayerHasClosedCaptionsPeer(m_peer);
}
void
MediaPlayerPrivate::setClosedCaptionsVisible(bool flag)
{
    (void)wkcMediaPlayerSetClosedCaptionsVisiblePeer(m_peer, flag);
}

MediaPlayer::NetworkState
MediaPlayerPrivate::networkState() const
{
    const int ret = wkcMediaPlayerNetworkStatePeer(m_peer);
    switch (ret) {
    case WKC_MEDIA_NETWORKSTATE_EMPTY:
        return MediaPlayer::Empty;
    case WKC_MEDIA_NETWORKSTATE_IDLE:
        return MediaPlayer::Idle;
    case WKC_MEDIA_NETWORKSTATE_LOADING:
        return MediaPlayer::Loading;
    case WKC_MEDIA_NETWORKSTATE_LOADED:
        return MediaPlayer::Loaded;
    case WKC_MEDIA_NETWORKSTATE_FORMATERROR:
        return MediaPlayer::FormatError;
    case WKC_MEDIA_NETWORKSTATE_DECODEERROR:
        return MediaPlayer::DecodeError;
    case WKC_MEDIA_NETWORKSTATE_NETWORKERROR:
    default:
        return MediaPlayer::NetworkError;
    }
}

MediaPlayer::ReadyState
MediaPlayerPrivate::readyState() const
{
    const int ret = wkcMediaPlayerReadyStatePeer(m_peer);
    switch (ret) {
    case WKC_MEDIA_READYSTATE_HAVE_METADATA:
        return MediaPlayer::HaveMetadata;
    case WKC_MEDIA_READYSTATE_HAVE_CURRENTDATA:
        return MediaPlayer::HaveCurrentData;
    case WKC_MEDIA_READYSTATE_HAVE_FUTUREDATA:
        return MediaPlayer::HaveFutureData;
    case WKC_MEDIA_READYSTATE_HAVE_ENOUGHDATA:
        return MediaPlayer::HaveEnoughData;
    case WKC_MEDIA_READYSTATE_HAVE_NOTHING:
    default:
        return MediaPlayer::HaveNothing;
    }
}

float
MediaPlayerPrivate::maxTimeSeekable() const
{
    return wkcMediaPlayerMaxTimeSeekablePeer(m_peer);
}

std::unique_ptr<PlatformTimeRanges>
MediaPlayerPrivate::buffered() const
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
MediaPlayerPrivate::maximumDurationToCacheMediaTime() const
{
    return 0;
}

int
MediaPlayerPrivate::dataRate() const
{
    return wkcMediaPlayerDataRatePeer(m_peer);
}

bool
MediaPlayerPrivate::totalBytesKnown() const
{
    return wkcMediaPlayerTotalBytesKnownPeer(m_peer);
}
unsigned long long
MediaPlayerPrivate::totalBytes() const
{
    return wkcMediaPlayerTotalBytesPeer(m_peer);
}
unsigned
MediaPlayerPrivate::bytesLoaded() const
{
    return wkcMediaPlayerBytesLoadedPeer(m_peer);
}

void
MediaPlayerPrivate::setSize(const IntSize& isize)
{
    m_size = isize;

    WKCSize size = { isize.width(), isize.height() };

    int ret = wkcMediaPlayerSetSizePeer(m_peer, &size);
    if (ret==WKC_MEDIA_ERROR_NOTSUPPORTED) {
        m_resizeAtDrawing = true;
    } else {
        m_resizeAtDrawing = false;
    }

    const WKCRect wr = { m_paintRect.x(), m_paintRect.y(), m_paintRect.width(), m_paintRect.height() };
    (void)wkcMediaPlayerSetWindowPositionPeer(m_peer, &wr);
}

void
MediaPlayerPrivate::paint(GraphicsContext& ctx, const FloatRect& r)
{
    nxLog_in("ctx:%p r:%dx%d.%dx%d", &ctx, r.x(), r.y(), r.width(), r.height());

    m_paintRect = r;

    {
        const WKCRect wr = { r.x(), r.y(), r.width(), r.height() };
        (void)wkcMediaPlayerSetWindowPositionPeer(m_peer, &wr);
    }

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
            Ref<ImageWKC> wimg = ImageWKC::create(ImageWKC::EColorARGB8888, bitmap, rowbytes, size, false);
            wimg->setHasAlpha(false);
            if (fmt&WKC_IMAGETYPE_FLAG_HASALPHA)
                wimg->setHasAlpha(true);
            Ref<BitmapImage> img = BitmapImage::create(WTFMove(wimg));
            if (m_resizeAtDrawing) {
                const FloatRect dr(r.location(), m_size);
                const FloatRect sr(FloatPoint(), size);
                ctx.drawImage(img.get(), dr, sr);
            } else {
                ctx.drawImage(img.get(), r.location());
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
        ctx.save();
        ctx.fillRect(r, Color::black);
        ctx.restore();
        break;
    }

    default:
        nxLog_w("unknown media video sink type");
        break;
    }

    nxLog_out("");
}

void
MediaPlayerPrivate::paintCurrentFrameInContext(GraphicsContext& c, const FloatRect& r)
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

void
MediaPlayerPrivate::setAutobuffer(bool flag)
{
    (void)wkcMediaPlayerSetAutoBufferPeer(m_peer, flag);
}

bool
MediaPlayerPrivate::canLoadPoster() const
{
    return wkcMediaPlayerCanLoadPosterPeer(m_peer);
}

void
MediaPlayerPrivate::setPoster(const String& str)
{
    const char* cstr = str.utf8().data();
    (void)wkcMediaPlayerSetPosterPeer(m_peer, cstr);
}

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
void
MediaPlayerPrivate::deliverNotification(MediaPlayerProxyNotificationType)
{
}
void
MediaPlayerPrivate::setMediaPlayerProxy(WebMediaPlayerProxy*)
{
}
#endif

void
MediaPlayerPrivate::enterFullscreen()
{
    wkcMediaPlayerEnterFullscreenPeer(m_peer);
}

void
MediaPlayerPrivate::exitFullscreen()
{
    wkcMediaPlayerExitFullscreenPeer(m_peer);
}

bool
MediaPlayerPrivate::canEnterFullscreen() const
{
    return wkcMediaPlayerCanEnterFullscreenPeer(m_peer);
}

bool
MediaPlayerPrivate::supportsAcceleratedRendering() const
{
    if (wkcMediaPlayerVideoSinkTypePeer(m_peer)!=WKC_MEDIA_VIDEOSINKTYPE_LAYER)
        return false;
    return true;
}

void
MediaPlayerPrivate::acceleratedRenderingStateChanged()
{
    wkcMediaPlayerNotifyVideoLayerRenderingStateChangedPeer(m_peer);
}

PlatformLayer*
MediaPlayerPrivate::platformLayer() const
{
    return (PlatformLayer *)wkcMediaPlayerVideoLayerPeer(m_peer);
}

bool
MediaPlayerPrivate::hasSingleSecurityOrigin() const
{
    return wkcMediaPlayerHasSingleSecurityOriginPeer(m_peer);
}

MediaPlayer::MovieLoadType
MediaPlayerPrivate::movieLoadType() const
{
    const int type = wkcMediaPlayerMovieLoadTypePeer(m_peer);
    switch (type) {
    case WKC_MEDIA_MOVIELOADTYPE_DOWNLOAD:
        return MediaPlayer::Download;
    case WKC_MEDIA_MOVIELOADTYPE_STOREDSTREAM:
        return MediaPlayer::StoredStream;
    case WKC_MEDIA_MOVIELOADTYPE_LIVESTREAM:
        return MediaPlayer::LiveStream;
    case WKC_MEDIA_MOVIELOADTYPE_UNKNOWN:
    default:
        return MediaPlayer::Unknown;
    }
}

void
MediaPlayerPrivate::removedFromDocument()
{
    wkcMediaPlayerRemovedFromDocumentPeer(m_peer);
}

void
MediaPlayerPrivate::willBecomeInactive()
{
    wkcMediaPlayerWillBecomeInactivePeer(m_peer);
}

void*
MediaPlayerPrivate::platformPlayer() const
{
    return m_peer;
}

// callbacks
void 
MediaPlayerPrivate::notifyPlayerState(int state)
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
        // Ugh!: obtain current volume!
        // 110622 ACCESS Co.,Ltd.
        notImplemented();
        m_player->volumeChanged(0.f);
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
MediaPlayerPrivate::notifyRequestInvalidate(int x, int y, int w, int h)
{
    if (w<0 &&h<0) {
        // full invalidate
        m_player->repaint();
    } else {
        m_player->repaint();
    }
}

void
MediaPlayerPrivate::notifyAudioDataAvailable(float timing, int len)
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
MediaPlayerPrivate::generateKeyRequest(const String& keySystem, const unsigned char* initData, unsigned initDataLength)
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
MediaPlayerPrivate::addKey(const String& keySystem, const unsigned char* key, unsigned keyLength, const unsigned char* initData, unsigned initDataLength, const String& sessionId)
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
MediaPlayerPrivate::cancelKeyRequest(const String& keySystem, const String& sessionId)
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
MediaPlayerPrivate::notifyPlayerStreamGetProxy(void* out_info)
{
    WKCMediaPlayerStreamProxyInfo* info = reinterpret_cast<WKCMediaPlayerStreamProxyInfo *>(out_info);
    if (!out_info)
        return WKC_MEDIA_ERROR_GENERIC;
    ResourceHandleManager* mgr = ResourceHandleManager::sharedInstance();
    if (!mgr)
        return WKC_MEDIA_ERROR_GENERIC;

    ::memset(info, 0, sizeof(WKCMediaPlayerStreamProxyInfo));

    String proxy = mgr->proxy();
    if (proxy.length()) {
        URL kp(ParsedURLString, proxy);
        info->fUse = 1;
        ::strncpy(info->fHost, kp.host().utf8().data(), sizeof(info->fHost)-1);
        info->fPort = kp.port().value_or(0);
    } else {
        info->fUse = 0;
    }

    return WKC_MEDIA_ERROR_OK;
}

int
MediaPlayerPrivate::notifyPlayerStreamGetCookie(const char* uri, char* out_cookie, int buflen)
{
    ResourceHandleManager* mgr = ResourceHandleManager::sharedInstance();
    if (!mgr)
        return WKC_MEDIA_ERROR_GENERIC;

    URL ki(ParsedURLString, uri);
    String domain = ki.host().toString();
    String path = ki.path();
    bool secure = ki.protocolIs("https");

    String cookie = mgr->getCookie(domain, path, secure);

    if (buflen<cookie.length())
        return WKC_MEDIA_ERROR_GENERIC;
    ::strncpy(out_cookie, cookie.utf8().data(), buflen-1);
    return WKC_MEDIA_ERROR_OK;
}

int
MediaPlayerPrivate::notifyPlayerStreamOpenURI(const char* uri, void* procs)
{
    ResourceRequest req(uri);
    ResourceHandle* handle = WebCore::ResourceHandle::create(0, req, this, false, false, false).leakRef();

    httpConnection c = {0};
    WKCMediaPlayerStreamStateProcs* ps = static_cast<WKCMediaPlayerStreamStateProcs *>(procs);

    c.m_instance = ps->fInstance;
    c.m_infoProc = reinterpret_cast<void *>(ps->fInfoProc);
    c.m_receivedProc = reinterpret_cast<void *>(ps->fDataReceivedProc);
    c.m_errorProc = reinterpret_cast<void *>(ps->fErrorProc);
    c.m_connection = static_cast<void *>(handle);

    WKC_DEFINE_STATIC_INT(gPlayerStreamsCount, 0);
    c.m_id = ++gPlayerStreamsCount;

    m_httpConnections.append(c);

    return c.m_id;
}

MediaPlayerPrivate::httpConnection*
MediaPlayerPrivate::findConnection(int id)
{
    int count = m_httpConnections.size();
    for (int i=0; i<count; i++) {
        if (m_httpConnections.at(i).m_id==id) {
            return &m_httpConnections.at(i);
        }
    }
    return 0;
}

MediaPlayerPrivate::httpConnection*
MediaPlayerPrivate::findConnection(ResourceHandle* h)
{
    int count = m_httpConnections.size();
    for (int i=0; i<count; i++) {
        if (m_httpConnections.at(i).m_connection==static_cast<void *>(h)) {
            return &m_httpConnections.at(i);
        }
    }
    return 0;
}

void
MediaPlayerPrivate::removeConnection(ResourceHandle* h)
{
    int count = m_httpConnections.size();
    for (int i=0; i<count; i++) {
        if (m_httpConnections.at(i).m_connection==static_cast<void *>(h)) {
            m_httpConnections.remove(i);
            return;
        }
    }
}

int
MediaPlayerPrivate::notifyPlayerStreamClose(int id)
{
    httpConnection* c = findConnection(id);
    if (!c)
        return -1;
    ResourceHandle* h = static_cast<ResourceHandle *>(c->m_connection);
    removeConnection(h);
    h->cancel();
    h->deref();

    return 0;
}

int
MediaPlayerPrivate::notifyPlayerStreamCancel(int id)
{
    httpConnection* c = findConnection(id);
    if (!c)
        return 0;

    ResourceHandle* h = static_cast<ResourceHandle *>(c->m_connection);
    removeConnection(h);
    h->cancel();
    h->deref();

    return 0;
}

void
MediaPlayerPrivate::didReceiveData(ResourceHandle* h, const char* data, int len, unsigned /*encodedDataLength*/)
{
    httpConnection* c = findConnection(h);
    if (!c)
        return;
    WKCMPSNotifyStreamDataReceivedProc proc = reinterpret_cast<WKCMPSNotifyStreamDataReceivedProc>(c->m_receivedProc);
    (*proc)(c->m_instance, reinterpret_cast<const unsigned char *>(data), len);
}

void
MediaPlayerPrivate::didFinishLoading(ResourceHandle* h, double /*finishTime*/)
{
    httpConnection* c = findConnection(h);
    if (!c)
        return;
    WKCMPSNotifyStreamErrorProc proc = reinterpret_cast<WKCMPSNotifyStreamErrorProc>(c->m_errorProc);
    (*proc)(c->m_instance, WKC_MEDIA_ERROR_EOF);
}

void
MediaPlayerPrivate::didFail(ResourceHandle* h, const ResourceError&)
{
    httpConnection* c = findConnection(h);
    if (!c)
        return;
    WKCMPSNotifyStreamErrorProc proc = reinterpret_cast<WKCMPSNotifyStreamErrorProc>(c->m_errorProc);
    (*proc)(c->m_instance, WKC_MEDIA_ERROR_GENERIC);
}

void
MediaPlayerPrivate::willSendRequestAsync(ResourceHandle*, ResourceRequest&& request, ResourceResponse&&, CompletionHandler<void(ResourceRequest&&)>&& completionHandler)
{
    completionHandler(WTFMove(request));
}

void
MediaPlayerPrivate::didReceiveResponseAsync(ResourceHandle* h, ResourceResponse&& rsp, CompletionHandler<void()>&& completion)
{
    httpConnection* c = findConnection(h);
    if (!c)
        return;

    WKCMPSNotifyStreamInfoProc proc = reinterpret_cast<WKCMPSNotifyStreamInfoProc>(c->m_infoProc);

    WKCMediaPlayerStreamStreamInfo info = {0};
    info.fContentLength = rsp.expectedContentLength();
    CString cmime = rsp.mimeType().utf8();
    info.fMIMEType = cmime.data();
    (*proc)(c->m_instance, &info);
    completion();
}

bool
MediaPlayerPrivate::chromeVisible()
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
MediaPlayerPrivate::canManipulatePlayer()
{
    if (networkState() == MediaPlayer::FormatError)
        return false;

    return true;
}

void*
MediaPlayerPrivate::createHTMLMediaElement()
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
MediaPlayerPrivate::destroyHTMLMediaElement(void* p)
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

/*
 *  Copyright (c) 2011-2022 ACCESS CO., LTD. All rights reserved.
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

#ifndef _MEDIAPLAYERPRIVATEWKC_H_
#define _MEDIAPLAYERPRIVATEWKC_H_

#include "config.h"
#include "MediaPlayerPrivate.h"
#include "ResourceHandleClient.h"
#include "MediaSource.h"
#include <wtf/WeakPtr.h>

#if ENABLE(VIDEO)

namespace WebCore {

class ResourceHandle;

class MediaPlayerPrivateWKC final :  public MediaPlayerPrivateInterface, public CanMakeWeakPtr<MediaPlayerPrivateWKC>, public ResourceHandleClient {
public:
    explicit MediaPlayerPrivateWKC(MediaPlayer* player);
    ~MediaPlayerPrivateWKC();

    static void registerMediaEngine(MediaEngineRegistrar);
    static void getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types);
    static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters& parameters);
    static HashSet<RefPtr<SecurityOrigin>> originsInMediaCache(const String&);
    static void clearMediaCache(const String&, WallTime);
    static void clearMediaCacheForOrigins(const String&, const HashSet<RefPtr<SecurityOrigin>>&);
    static bool supportsKeySystem(const String& keySystem, const String& mimeType);
    static bool isAvailable();

    // interfaces
    virtual void load(const String& url) override final;
#if ENABLE(MEDIA_SOURCE)
    virtual void load(const String& url, MediaSourcePrivateClient* client) override final;
#endif
#if ENABLE(MEDIA_STREAM)
    virtual void load(WebCore::MediaStreamPrivate&) override final;
#endif
    virtual void cancelLoad() override final;
    virtual void prepareToPlay() override final;
    virtual bool didLoadingProgress() const override final;

    virtual void play() override final;
    virtual void pause() override final;    

    virtual bool supportsFullscreen() const override final;
    virtual bool canSaveMediaData() const override final;

    virtual FloatSize naturalSize() const override final;

    virtual bool hasVideo() const override final;
    virtual bool hasAudio() const override final;

    virtual void setVisible(bool) override final;

    virtual float duration() const override final;
    virtual double durationDouble() const override final;

    virtual float currentTime() const override final;
    virtual double currentTimeDouble() const override final;
    virtual void seek(float time) override final;
    virtual bool seeking() const override final;

    virtual MediaTime startTime() const override final;

    virtual double rate() const override final;
    virtual void setRate(float) override final;
    virtual void setPreservesPitch(bool) override final;

    virtual bool paused() const override final;

    virtual float volume() const override final;
    virtual void setVolume(float) override final;

    virtual void setMuted(bool) override final;

    virtual bool hasClosedCaptions() const override final;
    virtual void setClosedCaptionsVisible(bool) override final;

    virtual MediaPlayer::NetworkState networkState() const override final;
    virtual MediaPlayer::ReadyState readyState() const override final;

    virtual float maxTimeSeekable() const override final;
    virtual std::unique_ptr<PlatformTimeRanges> buffered() const override final;
    virtual double maximumDurationToCacheMediaTime() const override final;

    virtual unsigned long long totalBytes() const override final;

    virtual void setSize(const IntSize&) override final;

    virtual void paint(GraphicsContext&, const FloatRect&) override final;

    virtual void paintCurrentFrameInContext(GraphicsContext& c, const FloatRect& r) override final;

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    virtual void deliverNotification(MediaPlayerProxyNotificationType) override final;
    virtual void setMediaPlayerProxy(WebMediaPlayerProxy*) override final;
#endif

    virtual void enterFullscreen() override final;
    virtual void exitFullscreen() override final;
    virtual bool canEnterFullscreen() const override final;

    virtual bool supportsAcceleratedRendering() const override final;
    virtual void acceleratedRenderingStateChanged() override final;
    virtual PlatformLayer* platformLayer() const override final;

    virtual bool hasSingleSecurityOrigin() const override final;

    virtual MediaPlayer::MovieLoadType movieLoadType() const override final;

    virtual void removedFromDocument() override final;
    virtual void willBecomeInactive() override final;
    virtual void* platformPlayer() const override final;

#if ENABLE(ENCRYPTED_MEDIA)
    virtual MediaPlayer::MediaKeyException generateKeyRequest(const String& keySystem, const unsigned char* initData, unsigned initDataLength) override final;
    virtual MediaPlayer::MediaKeyException addKey(const String& keySystem, const unsigned char* key, unsigned keyLength, const unsigned char* initData, unsigned initDataLength, const String& sessionId) override final;
    virtual MediaPlayer::MediaKeyException cancelKeyRequest(const String& keySystem, const String& sessionId) override final;
#endif

    // downloader
    virtual void didReceiveData(ResourceHandle*, const char*, unsigned, int /*encodedDataLength*/) override final;
    virtual void didFinishLoading(ResourceHandle*) override final;
    virtual void didFail(ResourceHandle*, const ResourceError&) override final;

    virtual void willSendRequestAsync(ResourceHandle*, ResourceRequest&&, ResourceResponse&&, CompletionHandler<void(ResourceRequest&&)>&&) override final;
    virtual void didReceiveResponseAsync(ResourceHandle*, ResourceResponse&&, CompletionHandler<void()>&&) override final;

private:
    // callbacks
    // player
    static void notifyPlayerStateProc(void* s, int i) { ((MediaPlayerPrivateWKC *)s)->notifyPlayerState(i); }
    static void notifyRequestInvalidateProc(void* s, int x, int y, int w, int h) { ((MediaPlayerPrivateWKC *)s)->notifyRequestInvalidate(x,y,w,h); }
    static void notifyAudioDataAvailableProc(void* s, float t, unsigned int l) { ((MediaPlayerPrivateWKC *)s)->notifyAudioDataAvailable(t, l); }
    static void* createHTMLMediaElementProc(void* s) { return ((MediaPlayerPrivateWKC *)s)->createHTMLMediaElement(); }
    static void destroyHTMLMediaElementProc(void* s, void* p) { return ((MediaPlayerPrivateWKC *)s)->destroyHTMLMediaElement(p); }
#if ENABLE(ENCRYPTED_MEDIA)
    static void notifyKeyNeededProc(void* s, const char* k, const char *s_id, const unsigned char *d, unsigned dl) { ((MediaPlayerPrivateWKC *)s)->notifyKeyNeeded(k,s_id,d,dl); }
    static void notifyKeyRequestProc(void* s, const char* k, const char *s_id, const unsigned char *b, unsigned bl, const char *url) { ((MediaPlayerPrivateWKC *)s)->notifyKeyRequest(k,s_id,b,bl,url); }
    static void notifyKeyAddedProc(void* s, const char *k, const char *s_id) { ((MediaPlayerPrivateWKC *)s)->notifyKeyAdded(k,s_id); }
    static void notifyKeyErrorProc(void* s, const char *k, const char *s_id, int ec, unsigned short sc) { ((MediaPlayerPrivateWKC *)s)->notifyKeyError(k, s_id, ec, sc); }
#else
    static void notifyKeyNeededProc(void*, const char*, const char *, const unsigned char *, unsigned) { }
    static void notifyKeyRequestProc(void*, const char*, const char *, const unsigned char *, unsigned, const char *) { }
    static void notifyKeyAddedProc(void*, const char *, const char *) { }
    static void notifyKeyErrorProc(void*, const char *, const char *, int , unsigned short) { }
#endif
    void notifyPlayerState(int);
    void notifyRequestInvalidate(int, int, int, int);
    void notifyAudioDataAvailable(float, int);
    void* createHTMLMediaElement();
    void destroyHTMLMediaElement(void* p);
#if ENABLE(ENCRYPTED_MEDIA)
    void notifyKeyNeeded(const char *in_keySystem, const char *in_sessionID, const unsigned char *in_initData, unsigned in_initDataLen);
    void notifyKeyRequest(const char *in_keySystem, const char *in_sessionID, const unsigned char *in_binaryBuff, unsigned in_len, const char *in_defaultURL);
    void notifyKeyAdded(const char *in_keySystem, const char *in_sessionID);
    void notifyKeyError(const char *in_keySystem, const char *in_sessionID, int in_keyErrorCode, unsigned short in_systemCode);
#endif

    // network
    static int notifyPlayerStreamGetProxyProc(void* s, void* o) { return ((MediaPlayerPrivateWKC *)s)->notifyPlayerStreamGetProxy(o); }
    static int notifyPlayerStreamGetCookieProc(void* s, const char* u, char* o, int l) { return ((MediaPlayerPrivateWKC *)s)->notifyPlayerStreamGetCookie(u, o,l); }

    static int notifyPlayerStreamOpenURIProc(void* s, const char* u, void* p) { return ((MediaPlayerPrivateWKC *)s)->notifyPlayerStreamOpenURI(u,p); }
    static int notifyPlayerStreamCloseProc(void* s, int i) { return ((MediaPlayerPrivateWKC *)s)->notifyPlayerStreamClose(i); }
    static int notifyPlayerStreamCancelProc(void* s, int i) { return ((MediaPlayerPrivateWKC *)s)->notifyPlayerStreamCancel(i); }

    int notifyPlayerStreamGetProxy(void* out_info);
    int notifyPlayerStreamGetCookie(const char* uri, char* out_cookie, int buflen);
    int notifyPlayerStreamOpenURI(const char* uri, void* procs);
    int notifyPlayerStreamClose(int id);
    int notifyPlayerStreamCancel(int id);

    typedef struct httpConnection_ {
        int m_id;
        void* m_connection;
        void* m_instance;
        void* m_infoProc;
        void* m_receivedProc;
        void* m_errorProc;
    } httpConnection;
    httpConnection* findConnection(int);
    httpConnection* findConnection(ResourceHandle*);
    void removeConnection(ResourceHandle*);

    static bool chromeVisibleProc(void* s) { return ((MediaPlayerPrivateWKC *)s)->chromeVisible(); }
    bool chromeVisible();
    bool canManipulatePlayer();

private:
    MediaPlayer* m_player;
    void* m_peer;
    void* m_audiopeer;

    float m_volume;
    bool m_muted;

    bool m_resizeAtDrawing;
    IntSize m_size;

    Vector<httpConnection> m_httpConnections;
    mutable float m_lastBuffered;
};

} // namespace WebCore

#endif// ENABLE(VIDEO)

#endif // _MEDIAPLAYERPRIVATEWKC_H_

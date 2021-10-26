/*
 *  Copyright (c) 2011-2021 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(VIDEO)

namespace WebCore {

class ResourceHandle;

class MediaPlayerPrivate :  public MediaPlayerPrivateInterface, public ResourceHandleClient {
public:
    static std::unique_ptr<MediaPlayerPrivateInterface> create(MediaPlayer* player);
    static void registerMediaEngine(MediaEngineRegistrar);
    static void getSupportedTypes(HashSet<String, ASCIICaseInsensitiveHash>& types);
    static MediaPlayer::SupportsType supportsType(const MediaEngineSupportParameters& parameters);
    static HashSet<RefPtr<SecurityOrigin>> originsInMediaCache(const String&);
    static void clearMediaCache(const String&, WallTime);
    static void clearMediaCacheForOrigins(const String&, const HashSet<RefPtr<SecurityOrigin>>&);
    static bool supportsKeySystem(const String& keySystem, const String& mimeType);
    static bool isAvailable();

    MediaPlayerPrivate(MediaPlayer* player);
    virtual ~MediaPlayerPrivate();

    // interfaces
    virtual void load(const String& url);
#if ENABLE(MEDIA_SOURCE)
    virtual void load(const String& url, MediaSourcePrivateClient* client);
#endif
#if ENABLE(MEDIA_STREAM)
    virtual void load(WebCore::MediaStreamPrivate&);
#endif
    virtual void cancelLoad();
    virtual void prepareToPlay();
    virtual bool didLoadingProgress() const;

    virtual void play();
    virtual void pause();    

    virtual bool supportsFullscreen() const;
    virtual bool supportsSave() const;

    virtual FloatSize naturalSize() const;

    virtual bool hasVideo() const;
    virtual bool hasAudio() const;

    virtual void setVisible(bool);

    virtual float duration() const;
    virtual double durationDouble() const;

    virtual float currentTime() const;
    virtual double currentTimeDouble() const;
    virtual void seek(float time);
    virtual bool seeking() const;

    virtual MediaTime startTime() const;

    virtual void setEndTime(float);

    double rate() const;
    virtual void setRate(float);
    virtual void setPreservesPitch(bool);

    virtual bool paused() const;

    virtual void setVolume(float);

    virtual bool hasClosedCaptions() const;
    virtual void setClosedCaptionsVisible(bool);

    virtual MediaPlayer::NetworkState networkState() const;
    virtual MediaPlayer::ReadyState readyState() const;

    virtual float maxTimeSeekable() const;
    virtual std::unique_ptr<PlatformTimeRanges> buffered() const;
    virtual double maximumDurationToCacheMediaTime() const;

    virtual int dataRate() const;

    virtual bool totalBytesKnown() const;
    virtual unsigned long long totalBytes() const;
    virtual unsigned bytesLoaded() const;

    virtual void setSize(const IntSize&);

    virtual void paint(GraphicsContext&, const FloatRect&);

    virtual void paintCurrentFrameInContext(GraphicsContext& c, const FloatRect& r);

    virtual void setAutobuffer(bool);

    virtual bool canLoadPoster() const;
    virtual void setPoster(const String&);

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    virtual void deliverNotification(MediaPlayerProxyNotificationType);
    virtual void setMediaPlayerProxy(WebMediaPlayerProxy*);
#endif

    virtual void enterFullscreen();
    virtual void exitFullscreen();
    virtual bool canEnterFullscreen() const;

    virtual bool supportsAcceleratedRendering() const;
    virtual void acceleratedRenderingStateChanged();
    virtual PlatformLayer* platformLayer() const;

    virtual bool hasSingleSecurityOrigin() const;

    virtual MediaPlayer::MovieLoadType movieLoadType() const;

    virtual void removedFromDocument();
    virtual void willBecomeInactive();
    virtual void* platformPlayer() const;

#if ENABLE(ENCRYPTED_MEDIA)
    virtual MediaPlayer::MediaKeyException generateKeyRequest(const String& keySystem, const unsigned char* initData, unsigned initDataLength);
    virtual MediaPlayer::MediaKeyException addKey(const String& keySystem, const unsigned char* key, unsigned keyLength, const unsigned char* initData, unsigned initDataLength, const String& sessionId);
    virtual MediaPlayer::MediaKeyException cancelKeyRequest(const String& keySystem, const String& sessionId);
#endif

    // downloader
    virtual void didReceiveData(ResourceHandle*, const char*, int, unsigned /*encodedDataLength*/);
    virtual void didFinishLoading(ResourceHandle*, double /*finishTime*/);
    virtual void didFail(ResourceHandle*, const ResourceError&);

    virtual void willSendRequestAsync(ResourceHandle*, ResourceRequest&&, ResourceResponse&&, CompletionHandler<void(ResourceRequest&&)>&&);
    virtual void didReceiveResponseAsync(ResourceHandle*, ResourceResponse&&, CompletionHandler<void()>&&);

private:
    bool construct();

    // callbacks
    // player
    static void notifyPlayerStateProc(void* s, int i) { ((MediaPlayerPrivate *)s)->notifyPlayerState(i); }
    static void notifyRequestInvalidateProc(void* s, int x, int y, int w, int h) { ((MediaPlayerPrivate *)s)->notifyRequestInvalidate(x,y,w,h); }
    static void notifyAudioDataAvailableProc(void* s, float t, unsigned int l) { ((MediaPlayerPrivate *)s)->notifyAudioDataAvailable(t, l); }
    static void* createHTMLMediaElementProc(void* s) { return ((MediaPlayerPrivate *)s)->createHTMLMediaElement(); }
    static void destroyHTMLMediaElementProc(void* s, void* p) { return ((MediaPlayerPrivate *)s)->destroyHTMLMediaElement(p); }
#if ENABLE(ENCRYPTED_MEDIA)
    static void notifyKeyNeededProc(void* s, const char* k, const char *s_id, const unsigned char *d, unsigned dl) { ((MediaPlayerPrivate *)s)->notifyKeyNeeded(k,s_id,d,dl); }
    static void notifyKeyRequestProc(void* s, const char* k, const char *s_id, const unsigned char *b, unsigned bl, const char *url) { ((MediaPlayerPrivate *)s)->notifyKeyRequest(k,s_id,b,bl,url); }
    static void notifyKeyAddedProc(void* s, const char *k, const char *s_id) { ((MediaPlayerPrivate *)s)->notifyKeyAdded(k,s_id); }
    static void notifyKeyErrorProc(void* s, const char *k, const char *s_id, int ec, unsigned short sc) { ((MediaPlayerPrivate *)s)->notifyKeyError(k, s_id, ec, sc); }
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
    static int notifyPlayerStreamGetProxyProc(void* s, void* o) { return ((MediaPlayerPrivate *)s)->notifyPlayerStreamGetProxy(o); }
    static int notifyPlayerStreamGetCookieProc(void* s, const char* u, char* o, int l) { return ((MediaPlayerPrivate *)s)->notifyPlayerStreamGetCookie(u, o,l); }

    static int notifyPlayerStreamOpenURIProc(void* s, const char* u, void* p) { return ((MediaPlayerPrivate *)s)->notifyPlayerStreamOpenURI(u,p); }
    static int notifyPlayerStreamCloseProc(void* s, int i) { return ((MediaPlayerPrivate *)s)->notifyPlayerStreamClose(i); }
    static int notifyPlayerStreamCancelProc(void* s, int i) { return ((MediaPlayerPrivate *)s)->notifyPlayerStreamCancel(i); }

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

    static bool chromeVisibleProc(void* s) { return ((MediaPlayerPrivate *)s)->chromeVisible(); }
    bool chromeVisible();
    bool canManipulatePlayer();

private:
    MediaPlayer* m_player;
    void* m_peer;
    void* m_audiopeer;

    bool m_resizeAtDrawing;
    IntSize m_size;

    Vector<httpConnection> m_httpConnections;
    mutable float m_lastBuffered;
};

} // namespace WebCore

#endif// ENABLE(VIDEO)

#endif // _MEDIAPLAYERPRIVATEWKC_H_

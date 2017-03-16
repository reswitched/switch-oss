/*
 *  Copyright (c) 2013-2016 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(MEDIA_SOURCE)

#include "AudioTrackPrivate.h"
#include "InbandTextTrackPrivate.h"
#include "MediaDescription.h"
#include "MediaPlayerPrivateWKC.h"
#include "MediaSourcePrivateWKC.h"
#include "SourceBufferPrivateWKC.h"
#include "SourceBufferPrivateClient.h"
#include "VideoTrackPrivate.h"

#include "MediaSample.h"

#include "NotImplemented.h"

#include <wkc/wkcmediapeer.h>

namespace WebCore {

// AudioTrackPrivate
class AudioTrackPrivateWKC : public AudioTrackPrivate {
public:
    static PassRefPtr<AudioTrackPrivate> create()
    {
        return adoptRef(new AudioTrackPrivateWKC());
    }

    virtual AtomicString id() const { return String::format("%d", 1); }
    virtual ~AudioTrackPrivateWKC() { }
};

// VideoTrackPrivate
class VideoTrackPrivateWKC : public VideoTrackPrivate {
public:
    static PassRefPtr<VideoTrackPrivate> create()
    {
        return adoptRef(new VideoTrackPrivateWKC());
    }

    virtual AtomicString id() const { return String::format("%d", 1); }
    virtual ~VideoTrackPrivateWKC() { }
};

// MediaDescription
class MediaDescriptionWKC : public MediaDescription {
public:
    static RefPtr<MediaDescriptionWKC> create(const char* codec, int mediaKind) { return adoptRef(new MediaDescriptionWKC(codec, mediaKind)); }
    virtual ~MediaDescriptionWKC() {}

    virtual AtomicString codec() const override { return m_codec; }
    virtual bool isVideo() const override { return m_mediaKind == WKC_MEDIA_SB_TRACKKIND_VIDEO; }
    virtual bool isAudio() const override { return m_mediaKind == WKC_MEDIA_SB_TRACKKIND_AUDIO; }
    virtual bool isText() const override { return false; }
protected:
    MediaDescriptionWKC(const char* codec, int mediaKind)
        : m_codec(codec)
        , m_mediaKind(mediaKind)
    {
    }

private:
    AtomicString m_codec;
    int m_mediaKind;
};

// MediaSample

class MediaSampleWKC : public MediaSample {
public:
    static RefPtr<MediaSampleWKC> create(double presentationTime, double decodeTime, double duration) { return adoptRef(new MediaSampleWKC(presentationTime, decodeTime, duration)); }
    virtual ~MediaSampleWKC() { }

    virtual MediaTime presentationTime() const override { return m_presentationTime; }
    virtual MediaTime decodeTime() const override { return m_decodeTime; }
    virtual MediaTime duration() const override { return m_duration; }
    virtual AtomicString trackID() const override { return m_trackID; }

    virtual SampleFlags flags() const override { return IsSync; }
    virtual PlatformSample platformSample() { PlatformSample sample = { PlatformSample::None, }; return sample; }

    virtual size_t sizeInBytes() const { return 0; }
    virtual FloatSize presentationSize() const { return { 0, 0 }; }
    virtual void offsetTimestampsBy(const MediaTime&) { }
    virtual void dump(PrintStream&) const { }

protected:
    MediaSampleWKC(double presentationTime, double decodeTime, double duration)
        : m_presentationTime(MediaTime::createWithDouble(presentationTime))
        , m_decodeTime(MediaTime::createWithDouble(decodeTime))
        , m_duration(MediaTime::createWithDouble(duration))
        , m_trackID(String::format("%d", 1))
    {
    }

private:
    MediaTime m_presentationTime;
    MediaTime m_decodeTime;
    MediaTime m_duration;
    AtomicString m_trackID;
};

SourceBufferPrivateWKC::SourceBufferPrivateWKC(void* in_peer, void* in_sourcebuffer, MediaSourcePrivate* in_mediasource)
    : m_client(0)
    , m_peer(in_peer)
    , m_sourcebuffer(in_sourcebuffer)
    , m_mediasource(in_mediasource)
{
}

SourceBufferPrivateWKC::~SourceBufferPrivateWKC()
{
}

SourceBufferPrivate*
SourceBufferPrivateWKC::create(void* in_peer, void* in_sourcebuffer, MediaSourcePrivate* in_mediasource)
{
    SourceBufferPrivateWKC* self = new SourceBufferPrivateWKC(in_peer, in_sourcebuffer, in_mediasource);
    if (!self) {
        return 0;
    }
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return (SourceBufferPrivate *)self;
}

bool
SourceBufferPrivateWKC::construct()
{
    return true;
}

void
SourceBufferPrivateWKC::setClient(SourceBufferPrivateClient* client)
{
    m_client = client;
}

void
SourceBufferPrivateWKC::append(const unsigned char* data, unsigned length)
{
    WKCMediaPlayerSourceBufferCallbacks procs = {
        didReceiveInitializationSegmentProc,
        didReceiveSamplesProc,
        appendCompleteProc,
        removeCodedFramesProc,
    };
    (void)wkcMediaPlayerAppendToSBPeer(m_peer, m_sourcebuffer, data, length, &procs, (void*)this);
}

void
SourceBufferPrivateWKC::abort()
{
    (void)wkcMediaPlayerAbortSBPeer(m_peer, m_sourcebuffer);
}

bool
SourceBufferPrivateWKC::setTimestampOffset(double timestamp_offset)
{
    return wkcMediaPlayerSetSBTimestampOffsetPeer(m_peer, m_sourcebuffer, timestamp_offset);
}

bool
SourceBufferPrivateWKC::removedFromMediaSource()
{
    return wkcMediaPlayerRemoveSBPeer(m_peer, m_sourcebuffer) == WKC_MEDIA_ERROR_OK;
}

MediaPlayer::ReadyState
SourceBufferPrivateWKC::readyState() const
{
    return m_mediasource->readyState();
}

void
SourceBufferPrivateWKC::setReadyState(MediaPlayer::ReadyState in_state)
{
    m_mediasource->setReadyState(in_state);
}

void
SourceBufferPrivateWKC::evictCodedFrames()
{
    (void)wkcMediaPlayerEvictCodedFramesSBPeer(m_peer, m_sourcebuffer);
}

bool
SourceBufferPrivateWKC::isFull()
{
    return wkcMediaPlayerIsFullSBPeer(m_peer, m_sourcebuffer);
}

void
SourceBufferPrivateWKC::didReceiveInitializationSegmentProc(void* self, const char* in_codec, int in_kind, double in_duration)
{
    static_cast<SourceBufferPrivateWKC*>(self)->didReceiveInitializationSegment(in_codec, in_kind, in_duration);
}

void
SourceBufferPrivateWKC::didReceiveSamplesProc(void* self, const WKCMediaSample* in_samples, int in_samples_len)
{
    static_cast<SourceBufferPrivateWKC*>(self)->didReceiveSamples(in_samples, in_samples_len);
}

void
SourceBufferPrivateWKC::appendCompleteProc(void* self, int in_appendresult)
{
    static_cast<SourceBufferPrivateWKC*>(self)->appendComplete(in_appendresult);
}

void
SourceBufferPrivateWKC::removeCodedFramesProc(void* self, double in_start, double in_end)
{
    static_cast<SourceBufferPrivateWKC*>(self)->removeCodedFrames(in_start, in_end);
}

void
SourceBufferPrivateWKC::didReceiveInitializationSegment(const char* in_codec, int in_kind, double in_duration)
{
    SourceBufferPrivateClient::InitializationSegment init_segment;

    init_segment.duration = MediaTime::createWithDouble(in_duration);

    switch (in_kind) {
    case WKC_MEDIA_SB_TRACKKIND_AUDIO:
        {
            SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation info;
            info.description = MediaDescriptionWKC::create(in_codec, in_kind);
            info.track = AudioTrackPrivateWKC::create();
            init_segment.audioTracks.append(info);
        }
        break;
    case WKC_MEDIA_SB_TRACKKIND_VIDEO:
        {
            SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation info;
            info.description = MediaDescriptionWKC::create(in_codec, in_kind);
            info.track = VideoTrackPrivateWKC::create();
            init_segment.videoTracks.append(info);
        }
        break;
    default:
        break;
    }
    if (m_client) {
        m_client->sourceBufferPrivateDidReceiveInitializationSegment(this, init_segment);
    }
}

void
SourceBufferPrivateWKC::didReceiveSamples(const WKCMediaSample* in_samples, int in_samples_len)
{
    if (!m_client) {
        return;
    }
    for (int i = 0; i < in_samples_len; i++) {
        m_client->sourceBufferPrivateDidReceiveSample(this, MediaSampleWKC::create(in_samples[i].fPresentationTime, in_samples[i].fDecodeTime, in_samples[i].fDuration));
    }
}

void
SourceBufferPrivateWKC::appendComplete(int in_appendresult)
{
    SourceBufferPrivateClient::AppendResult result;
    switch (in_appendresult) {
    case WKC_MEDIA_SB_APPENDRESULT_APPEND_SUCCEEDED:
        result = SourceBufferPrivateClient::AppendSucceeded;
        break;
    case WKC_MEDIA_SB_APPENDRESULT_PARSING_FAILED:
        result = SourceBufferPrivateClient::ParsingFailed;
        break;
    case WKC_MEDIA_SB_APPENDRESULT_READ_STREAM_FAILED:
    default:
        result = SourceBufferPrivateClient::ReadStreamFailed;
        break;
    }

    if (m_client)
        m_client->sourceBufferPrivateAppendComplete(this, result);
}

void
SourceBufferPrivateWKC::removeCodedFrames(double in_start, double in_end)
{
    if (m_client)
        m_client->sourceBufferPrivateRemoveCodedFrames(this, in_start, in_end);
}

} // namespace WebCore


#endif

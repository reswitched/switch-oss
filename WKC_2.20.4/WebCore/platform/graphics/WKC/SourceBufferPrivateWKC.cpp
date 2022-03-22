/*
 *  Copyright (c) 2013-2021 ACCESS CO., LTD. All rights reserved.
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

#include "MediaSampleWKC.h"

#include "NotImplemented.h"

#include <wkc/wkcmediapeer.h>

namespace WebCore {

// AudioTrackPrivate
class AudioTrackPrivateWKC : public AudioTrackPrivate {
public:
    static Ref<AudioTrackPrivate> create(int id)
    {
        return adoptRef(*new AudioTrackPrivateWKC(id));
    }

    virtual AtomString id() const { return m_id; }
    virtual ~AudioTrackPrivateWKC() { }
protected:
    AudioTrackPrivateWKC(int id)
        : m_id(makeString(id))
    {
    }

private:
    AtomString m_id;
};

// VideoTrackPrivate
class VideoTrackPrivateWKC : public VideoTrackPrivate {
public:
    static Ref<VideoTrackPrivate> create(int id)
    {
        return adoptRef(*new VideoTrackPrivateWKC(id));
    }

    virtual AtomString id() const { return m_id; }
    virtual ~VideoTrackPrivateWKC() { }
protected:
    VideoTrackPrivateWKC(int id)
        : m_id(makeString(id))
    {
    }

private:
    AtomString m_id;
};

// MediaDescription
class MediaDescriptionWKC : public MediaDescription {
public:
    static RefPtr<MediaDescriptionWKC> create(const char* codec, int mediaKind) { return adoptRef(new MediaDescriptionWKC(codec, mediaKind)); }
    virtual ~MediaDescriptionWKC() {}

    virtual AtomString codec() const override { return m_codec; }
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
    AtomString m_codec;
    int m_mediaKind;
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
SourceBufferPrivateWKC::append(Vector<unsigned char>&& vector)
{
    WKCMediaPlayerSourceBufferCallbacks procs = {
        didReceiveInitializationSegmentProc,
        didReceiveSampleProc,
        appendCompleteProc,
        removeCodedFramesProc,
    };
    (void)wkcMediaPlayerAppendToSBPeer(m_peer, m_sourcebuffer, &vector[0], vector.size(), &procs, (void*)this);
}

void
SourceBufferPrivateWKC::abort()
{
    (void)wkcMediaPlayerAbortSBPeer(m_peer, m_sourcebuffer);
}

void
SourceBufferPrivateWKC::resetParserState()
{
    // TODO
    abort();
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

void
SourceBufferPrivateWKC::removeCodedFrames(double start, double end)
{
    (void)wkcMediaPlayerRemoveCodedFramesPeer(m_peer, m_sourcebuffer, start, end);
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
SourceBufferPrivateWKC::evictCodedFrames(MediaTime* out_start, MediaTime* out_end)
{
    double start = 0;
    double end = 0;
    (void)wkcMediaPlayerEvictCodedFramesSBPeer(m_peer, m_sourcebuffer, &start, &end);
    *out_start = MediaTime::createWithDouble(start);
    *out_end = MediaTime::createWithDouble(end);
}

bool
SourceBufferPrivateWKC::isFull()
{
    return wkcMediaPlayerIsFullSBPeer(m_peer, m_sourcebuffer);
}

void
SourceBufferPrivateWKC::didReceiveInitializationSegmentProc(void* self, const WKCMediaTrack* in_tracks, int in_tracks_len, double in_duration)
{
    static_cast<SourceBufferPrivateWKC*>(self)->didReceiveInitializationSegment(in_tracks, in_tracks_len, in_duration);
}

void
SourceBufferPrivateWKC::didReceiveSampleProc(void* self, WKCMediaSample* in_sample)
{
    static_cast<SourceBufferPrivateWKC*>(self)->didReceiveSample(in_sample);
}

void
SourceBufferPrivateWKC::appendCompleteProc(void* self, int in_appendresult)
{
    static_cast<SourceBufferPrivateWKC*>(self)->appendComplete(in_appendresult);
}

void
SourceBufferPrivateWKC::removeCodedFramesProc(void* self, double in_start, double in_end)
{
    static_cast<SourceBufferPrivateWKC*>(self)->removeCodedFramesFromApp(in_start, in_end);
}

void
SourceBufferPrivateWKC::didReceiveInitializationSegment(const WKCMediaTrack* in_tracks, int in_tracks_len, double in_duration)
{
    SourceBufferPrivateClient::InitializationSegment init_segment;

    init_segment.duration = MediaTime::createWithDouble(in_duration);

    for (int i = 0; i < in_tracks_len; i++) {
        switch (in_tracks[i].fKind) {
        case WKC_MEDIA_SB_TRACKKIND_AUDIO:
            {
                SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation info;
                info.description = MediaDescriptionWKC::create(in_tracks[i].fCodec, in_tracks[i].fKind);
                info.track = AudioTrackPrivateWKC::create(in_tracks[i].fID);
                init_segment.audioTracks.append(info);
            }
            break;
        case WKC_MEDIA_SB_TRACKKIND_VIDEO:
            {
                SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation info;
                info.description = MediaDescriptionWKC::create(in_tracks[i].fCodec, in_tracks[i].fKind);
                info.track = VideoTrackPrivateWKC::create(in_tracks[i].fID);
                init_segment.videoTracks.append(info);
            }
            break;
        default:
            break;
        }
    }

    if (m_client) {
        m_client->sourceBufferPrivateDidReceiveInitializationSegment(init_segment);
    }
}

void
SourceBufferPrivateWKC::didReceiveSample(WKCMediaSample* inout_samples)
{
    if (!m_client) {
        return;
    }
    Ref<MediaSampleWKC> sample = MediaSampleWKC::create(inout_samples->fID, inout_samples->fPresentationTime, inout_samples->fDecodeTime, inout_samples->fDuration, inout_samples->fSize, inout_samples->fIsSync);
    m_client->sourceBufferPrivateDidReceiveSample(sample);
    inout_samples->fPresentationTime = sample->presentationTime().toTimeScale(1000000).timeValue();
    inout_samples->fDecodeTime = sample->decodeTime().toTimeScale(1000000).timeValue();
    inout_samples->fIsDropped = sample->isDropped();
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
        m_client->sourceBufferPrivateAppendComplete(result);
}

void
SourceBufferPrivateWKC::removeCodedFramesFromApp(double in_start, double in_end)
{
    if (m_client)
        m_client->sourceBufferPrivateRemoveCodedFrames(in_start, in_end);
}

#if !RELEASE_LOG_DISABLED
const Logger&
SourceBufferPrivateWKC::sourceBufferLogger() const
{
    static const auto logger = Logger::create(this);
    return logger;
}

const void*
SourceBufferPrivateWKC::sourceBufferLogIdentifier()
{
    notImplemented();
    return nullptr;
}
#endif

} // namespace WebCore


#endif

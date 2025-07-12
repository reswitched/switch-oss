/*
 *  Copyright (c) 2012-2015 ACCESS CO., LTD. All rights reserved.
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

#if ENABLE(MEDIA_STREAM)

#include "RealtimeMediaSourceCenter.h"
#include "MediaStreamCreationClient.h"
#include "MediaStreamTrackSourcesRequestClient.h"
#include "NotImplemented.h"

#include <wkc/wkcmediapeer.h>

namespace WebCore {

class MediaStreamCenterWKC : public RealtimeMediaSourceCenter
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    MediaStreamCenterWKC();
    virtual ~MediaStreamCenterWKC();

    virtual void validateRequestConstraints(PassRefPtr<MediaStreamCreationClient>, PassRefPtr<MediaConstraints> audioConstraints, PassRefPtr<MediaConstraints> videoConstraints);
    virtual void createMediaStream(PassRefPtr<MediaStreamCreationClient>, PassRefPtr<MediaConstraints> audioConstraints, PassRefPtr<MediaConstraints> videoConstraints);
    virtual bool getMediaStreamTrackSources(PassRefPtr<MediaStreamTrackSourcesRequestClient>);

private:
    PassRefPtr<RealtimeMediaSourceCenter> createMediaStreamSource(WKCMediaStreamSourceInfo&);
};

RealtimeMediaSourceCenter&
RealtimeMediaSourceCenter::platformCenter()
{
    WKC_DEFINE_STATIC_PTR(RealtimeMediaSourceCenter*, gInstance, new MediaStreamCenterWKC());
    return *gInstance;
}

MediaStreamCenterWKC::MediaStreamCenterWKC()
{
}

MediaStreamCenterWKC::~MediaStreamCenterWKC()
{
}

void
MediaStreamCenterWKC::createMediaStream(PassRefPtr<MediaStreamCreationClient>, PassRefPtr<MediaConstraints> audioConstraints, PassRefPtr<MediaConstraints> videoConstraints)
{
#if 0
    MediaStreamSource::ReadyState readyState = MediaStreamSource::ReadyStateLive;
    MediaStreamSource::Type type = MediaStreamSource::TypeVideo;

    switch (info.fType) {
    case WKC_MEDIA_STREAMSOURCE_TYPE_AUDIO:
        type = MediaStreamSource::TypeAudio;
        break;
    case WKC_MEDIA_STREAMSOURCE_TYPE_VIDEO:
        type = MediaStreamSource::TypeVideo;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    switch (info.fReadyState) {
    case WKC_MEDIA_STREAMSOURCE_READYSTATE_LIVE:
        readyState = MediaStreamSource::ReadyStateLive;
        break;
    case WKC_MEDIA_STREAMSOURCE_READYSTATE_MUTED:
        readyState = MediaStreamSource::ReadyStateMuted;
        break;
    case WKC_MEDIA_STREAMSOURCE_READYSTATE_ENDED:
        readyState = MediaStreamSource::ReadyStateEnded;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    return MediaStreamSource::create(String::fromUTF8(info.fID), type, String::fromUTF8(info.fName), readyState, info.fRequiresConsumer);
#endif
}

#if 0
void
MediaStreamCenterWKC::queryMediaStreamSources(PassRefPtr<MediaStreamSourcesQueryClient> client)
{
    MediaStreamSourceVector audioSources, videoSources;

    int num = wkcMediaPlayerQueryMediaStreamSourcesPeer();
    for (int i=0; i<num; i++) {
        WKCMediaStreamSourceInfo info;
        if (wkcMediaPlayerQueryMediaStreamSourcePeer(i, &info) != WKC_MEDIA_ERROR_OK)
            continue;
        PassRefPtr<MediaStreamSource> source = createMediaStreamSource(info);
        if (source->type() == MediaStreamSource::TypeAudio)
            audioSources.append(source);
        else
            videoSources.append(source);
    }

    client->didCompleteQuery(audioSources, videoSources);
}
#endif

void
MediaStreamCenterWKC::validateRequestConstraints(PassRefPtr<MediaStreamCreationClient>, PassRefPtr<MediaConstraints> audioConstraints, PassRefPtr<MediaConstraints> videoConstraints)
{
    notImplemented();
}

bool
MediaStreamCenterWKC::getMediaStreamTrackSources(PassRefPtr<MediaStreamTrackSourcesRequestClient>)
{
    notImplemented();
    return false;
}

} // namespace

#endif

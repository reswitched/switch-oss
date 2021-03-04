/*
 *  Copyright (c) 2013-2018 ACCESS CO., LTD. All rights reserved.
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

#ifndef _SOURCEBUFFERPRIVATEWKC_H_
#define _SOURCEBUFFERPRIVATEWKC_H_

#include "config.h"

#if ENABLE(MEDIA_SOURCE)

#include "SourceBufferPrivate.h"

#include <wkc/wkcmediapeer.h>

namespace WebCore {

class MediaSourcePrivateWKC;

class SourceBufferPrivateWKC : public SourceBufferPrivate {
public:
    SourceBufferPrivateWKC(void* in_peer, void* in_sourcebuffer, MediaSourcePrivate* in_mediasource);
    virtual ~SourceBufferPrivateWKC();

    static SourceBufferPrivate* create(void* in_peer, void* in_sourcebuffer, MediaSourcePrivate* in_mediasource);

    virtual void setClient(SourceBufferPrivateClient*);
    virtual void append(const unsigned char* data, unsigned length);
    virtual void abort();
    virtual void resetParserState();
    virtual bool setTimestampOffset(double);
#if PLATFORM(WKC)
    virtual bool removedFromMediaSource();
#else
    virtual void removedFromMediaSource();
#endif

    virtual MediaPlayer::ReadyState readyState() const;
    virtual void setReadyState(MediaPlayer::ReadyState);    
    virtual void evictCodedFrames();
    virtual bool isFull();

    static void didReceiveInitializationSegmentProc(void* self, const WKCMediaTrack* in_tracks, int in_tracks_len, double in_duration);
    static void didReceiveSamplesProc(void* self, const WKCMediaSample* in_samples, int in_samples_len);
    static void appendCompleteProc(void* self, int in_appendresult);
    static void removeCodedFramesProc(void* self, double in_start, double in_end);

    void didReceiveInitializationSegment(const WKCMediaTrack* in_tracks, int in_tracks_len, double in_duration);
    void didReceiveSamples(const WKCMediaSample* in_samples, int in_samples_len);
    void appendComplete(int in_appendresult);
    void removeCodedFrames(double in_start, double in_end);

private:
    virtual bool construct();

private:
    SourceBufferPrivateClient* m_client;
    void* m_peer;
    void* m_sourcebuffer;
    MediaSourcePrivate* m_mediasource;
};

} // namespace WebCore

#endif

#endif /* _SOURCEBUFFERPRIVATEWKC_H_ */

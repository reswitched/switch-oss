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

#ifndef _SOURCEBUFFERPRIVATEWKC_H_
#define _SOURCEBUFFERPRIVATEWKC_H_

#include "config.h"

#if ENABLE(MEDIA_SOURCE)

#include "Logging.h"
#include "SourceBufferPrivate.h"

#include <wkc/wkcmediapeer.h>

namespace WebCore {

class MediaSourcePrivateWKC;

class SourceBufferPrivateWKC : public SourceBufferPrivate {
public:
    SourceBufferPrivateWKC(void* in_peer, void* in_sourcebuffer, MediaSourcePrivate* in_mediasource);
    virtual ~SourceBufferPrivateWKC();

    static SourceBufferPrivate* create(void* in_peer, void* in_sourcebuffer, MediaSourcePrivate* in_mediasource);

    virtual void setActive(bool) override final;
    virtual void append(Vector<unsigned char>&&) override final;
    virtual void abort() override final;
    virtual void resetParserState() override final;
    virtual void removedFromMediaSource() override final;
    virtual MediaPlayer::ReadyState readyState() const override final;
    virtual void setReadyState(MediaPlayer::ReadyState) override final;

    virtual bool canSwitchToType(const ContentType&) override final;

    virtual void setTimestampOffset(const MediaTime&) override final;

    static void didReceiveInitializationSegmentProc(void* self, const WKCMediaTrack* in_tracks, int in_tracks_len, double in_duration);
    static void didReceiveSampleProc(void* self, WKCMediaSample* in_sample);
    static void appendCompleteProc(void* self, int in_appendresult);
    static void removeCodedFramesProc(void* self, double in_start, double in_end);

    void didReceiveInitializationSegment(const WKCMediaTrack* in_tracks, int in_tracks_len, double in_duration);
    void didReceiveSample(WKCMediaSample* in_sample);
    void appendComplete(int in_appendresult);
    void removeCodedFramesFromApp(double in_start, double in_end);

#if !RELEASE_LOG_DISABLED
    virtual const Logger& logger() const override final { return m_logger.get(); }
    virtual const void* logIdentifier() const override final { return m_logIdentifier; }
    virtual const char* logClassName() const override final { return "SourceBufferPrivate"; }
    virtual WTFLogChannel& logChannel() const override final { return LogMediaSource; }

    virtual const Logger& sourceBufferLogger() const override final { return logger(); }
    virtual const void* sourceBufferLogIdentifier() override final { return logIdentifier(); }
#endif

private:
    using base = SourceBufferPrivate;
    virtual bool construct();

    virtual MediaTime currentMediaTime() const override final;
    virtual MediaTime duration() const override final;

    virtual void removeCodedFrames(const MediaTime&, const MediaTime&) override final;
    virtual void evictCodedFrames(MediaTime& out_start, MediaTime& out_end) override final;
    virtual bool isFull() override final;

private:
    void* m_peer;
    void* m_sourcebuffer;
    MediaSourcePrivate* m_mediasource;
#if !RELEASE_LOG_DISABLED
    Ref<Logger> m_logger;
    const void* m_logIdentifier;
#endif
};

} // namespace WebCore

#endif

#endif /* _SOURCEBUFFERPRIVATEWKC_H_ */

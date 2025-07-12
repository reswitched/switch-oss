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

#ifndef _MEDIASOURCEPRIVATEWKC_H_
#define _MEDIASOURCEPRIVATEWKC_H_

#include "config.h"

#if ENABLE(MEDIA_SOURCE)

#include "MediaSourcePrivate.h"
#include "SourceBufferPrivateClient.h"

namespace WebCore {

class SourceBufferPrivateWKC;

class MediaSourcePrivateWKC : public MediaSourcePrivate {
    friend class SourceBufferPrivateWKC;
public:
    static MediaSourcePrivateWKC* create(MediaPlayerPrivateWKC* in_player, void* in_peer);
    virtual ~MediaSourcePrivateWKC();


    virtual AddStatus addSourceBuffer(const ContentType&, bool webMParserEnabled, RefPtr<SourceBufferPrivate>&) override final;
    virtual MediaTime duration() override final;
    virtual void setDuration(const MediaTime&) override final;
    virtual void markEndOfStream(EndOfStreamStatus) override final;
    virtual void unmarkEndOfStream() override final;
    virtual bool isEnded() const override final;
    virtual MediaPlayer::ReadyState readyState() const override final;
    virtual void setReadyState(MediaPlayer::ReadyState) override final;
    virtual void waitForSeekCompleted() override final;
    virtual void seekCompleted() override final;

protected:
    MediaSourcePrivateWKC(MediaPlayerPrivateWKC* in_player, void* in_peer);

private:
    bool construct();

private:
    MediaPlayerPrivateWKC* m_player;
    void* m_peer;
};

} // namespace WebCore

#endif

#endif /* _MEDIASOURCEPRIVATEWKC_H_ */

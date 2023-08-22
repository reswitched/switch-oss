/*
 *  Copyright (c) 2013-2014 ACCESS CO., LTD. All rights reserved.
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
    static MediaSourcePrivateWKC* create(MediaPlayerPrivate* in_player, void* in_peer);
    virtual ~MediaSourcePrivateWKC();


    virtual AddStatus addSourceBuffer(const ContentType&, RefPtr<SourceBufferPrivate>&);
    virtual double duration();
    virtual void setDuration(double);
    virtual void markEndOfStream(EndOfStreamStatus);
    virtual void unmarkEndOfStream();
    virtual MediaPlayer::ReadyState readyState() const;
    virtual void setReadyState(MediaPlayer::ReadyState);
    virtual void waitForSeekCompleted();
    virtual void seekCompleted();

protected:
    MediaSourcePrivateWKC(MediaPlayerPrivate* in_player, void* in_peer);

private:
    bool construct();

private:
    MediaPlayerPrivate* m_player;
    void* m_peer;
};

} // namespace WebCore

#endif

#endif /* _MEDIASOURCEPRIVATEWKC_H_ */

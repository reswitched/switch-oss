/*
 * Copyright (C) 2013-2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef InbandTextTrackPrivateLegacyAVFObjC_h
#define InbandTextTrackPrivateLegacyAVFObjC_h

#if ENABLE(VIDEO) && USE(AVFOUNDATION) && !HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT) && !PLATFORM(IOS)

#include "InbandTextTrackPrivateAVF.h"
#include <wtf/RetainPtr.h>

OBJC_CLASS AVPlayerItemTrack;

namespace WebCore {

class MediaPlayerPrivateAVFoundationObjC;

class InbandTextTrackPrivateLegacyAVFObjC : public InbandTextTrackPrivateAVF {
public:
    static PassRefPtr<InbandTextTrackPrivateLegacyAVFObjC> create(MediaPlayerPrivateAVFoundationObjC* player, AVPlayerItemTrack *track)
    {
        return adoptRef(new InbandTextTrackPrivateLegacyAVFObjC(player, track));
    }

    ~InbandTextTrackPrivateLegacyAVFObjC() { }

    virtual InbandTextTrackPrivate::Kind kind() const override;
    virtual bool isClosedCaptions() const override;
    virtual bool containsOnlyForcedSubtitles() const override;
    virtual bool isMainProgramContent() const override;
    virtual bool isEasyToRead() const override;
    virtual AtomicString label() const override;
    virtual AtomicString language() const override;

    virtual void disconnect() override;

    virtual Category textTrackCategory() const override { return LegacyClosedCaption; }
    
    AVPlayerItemTrack *avPlayerItemTrack() const { return m_playerItemTrack.get(); }

protected:
    InbandTextTrackPrivateLegacyAVFObjC(MediaPlayerPrivateAVFoundationObjC*, AVPlayerItemTrack *);
    
    RetainPtr<AVPlayerItemTrack> m_playerItemTrack;
};

}

#endif // ENABLE(VIDEO) && USE(AVFOUNDATION) && !HAVE(AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT) && !PLATFORM(IOS)

#endif

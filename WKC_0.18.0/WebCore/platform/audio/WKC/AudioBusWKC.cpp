/*
 * Copyright (c) 2012-2017 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioBus.h"
#include "AudioFileReader.h"
#include "PassRefPtr.h"

#include <wkc/wkcmediapeer.h>

namespace WebCore {

PassRefPtr<AudioBus> AudioBus::loadPlatformResource(const char* name, float sampleRate)
{
    int len = wkcMediaPlayerLoadPlatformAudioResourcePeer(name, 0, 0);
    if (len>0) {
        Vector<unsigned char> buf(len);
        len = wkcMediaPlayerLoadPlatformAudioResourcePeer(name, buf.data(), len);
        if (len>0) {
            RefPtr<AudioBus> bus = createBusFromInMemoryAudioFile(buf.data(), len, false, sampleRate);
            return bus;
        }
    }
    return nullptr;
}

PassRefPtr<AudioBus> createBusFromInMemoryAudioFile(const void* data, size_t dataSize, bool mixToMono, float sampleRate)
{
    int len = 0;
    int srate = 0;
    int bps = 0;
    int channels = 0;
    int endian = 0;

    len = wkcMediaPlayerDecodeAudioDataPeer(data, (int)dataSize, 0, 0, &srate, &bps, &channels, &endian);
    if (len>0 && (bps==8||bps==16) && (channels==1||channels==2||channels==4) && srate>0 && (endian==WKC_MEDIA_ENDIAN_LITTLEENDIAN||endian==WKC_MEDIA_ENDIAN_BIGENDIAN)) {
        Vector<unsigned char> buf(len);
        len = wkcMediaPlayerDecodeAudioDataPeer(data, (int)dataSize, buf.data(), len, &srate, &bps, &channels, &endian);
        if (len>0) {
            size_t dlen = len;
            if (bps==16) dlen>>=1;
            if (channels==2) dlen>>=1;
            else if (channels==4) dlen>>=2;
            RefPtr<AudioBus> orig = AudioBus::create(channels, dlen, true);

            orig->setSampleRate(srate);

            AudioChannel* lch = orig->channel(AudioBus::ChannelLeft);
            AudioChannel* rch = (channels >= 2) ? orig->channel(AudioBus::ChannelRight) : nullptr;
            float* dl = lch->mutableData();
            float* dr = (rch) ? rch->mutableData() : nullptr;

            int bl=0, bh=1;
            if (endian==WKC_MEDIA_ENDIAN_BIGENDIAN) {
                bl=1; bh=0;
            }
            int v=0;
            int sp=0;
            float l=0.f, r=0.f;
            const unsigned char* s = buf.data();
            if (bps==8) {
                for (int i=0; i<dlen;i++) {
                    l = r = (float)((int)s[sp++]-128) / 128.f;
                    if (channels==2) {
                        r = (float)((int)s[sp++]-128) / 128.f;
                    } else if (channels==4) {
                        l += (float)((int)s[sp++]-128) / 128.f;
                        r += (float)((int)s[sp++]-128) / 128.f;
                        if (l<-1.f) l=-1.f;
                        else if (l>1.f) l=1.f;
                        if (r<-1.f) r=-1.f;
                        else if (r>1.f) r=1.f;
                    }
                    dl[i] = l;
                    if (dr) {
                        dr[i] = r;
                    }
                }
            } else {
                for (int i=0; i<dlen;i++) {
                    v = s[sp+bl] + (s[sp+bh]<<8);
                    if (v>32767) v-=65536;
                    l = r = (float)v / 32768.f;
                    sp+=2;
                    if (channels==2) {
                        v = s[sp+bl] + (s[sp+bh]<<8);
                        if (v>32767) v-=65536;
                        r = (float)v / 32768.f;
                        sp+=2;
                    } else if (channels==4) {
                        v = s[sp+bl] + (s[sp+bh]<<8);
                        if (v>32767) v-=65536;
                        l += (float)v / 32768.f;
                        if (l<-1.f) l=-1.f;
                        else if (l>1.f) l=1.f;
                        sp+=2;
                        v = s[sp+bl] + (s[sp+bh]<<8);
                        if (v>32767) v-=65536;
                        r += (float)v / 32768.f;
                        if (r<-1.f) r=-1.f;
                        else if (r>1.f) r=1.f;
                        sp+=2;
                    }
                    dl[i] = l;
                    if (dr) {
                        dr[i] = r;
                    }
                }
            }
            buf.clear();
            return AudioBus::createBySampleRateConverting(orig.get(), mixToMono, sampleRate);
        }
    }
    return nullptr;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)

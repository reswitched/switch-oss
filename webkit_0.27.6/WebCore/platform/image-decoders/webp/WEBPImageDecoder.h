/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WEBPImageDecoder_h
#define WEBPImageDecoder_h

#include "ImageDecoder.h"

#if USE(WEBP)

#include "webp/decode.h"

#if ENABLE(WKC_BLINK_AWEBP)
#include "webp/demux.h"
#if USE(QCMSLIB)
#define QCMS_WEBP_COLOR_CORRECTION
#endif
#else
#if USE(QCMSLIB) && (WEBP_DECODER_ABI_VERSION > 0x200)
#define QCMS_WEBP_COLOR_CORRECTION
#endif
#endif

namespace WebCore {

class WEBPImageDecoder : public ImageDecoder {
public:
    WEBPImageDecoder(ImageSource::AlphaOption, ImageSource::GammaAndColorProfileOption);
    virtual ~WEBPImageDecoder();

    virtual String filenameExtension() const { return "webp"; }
#if ENABLE(WKC_BLINK_AWEBP)
    virtual void setData(SharedBuffer* data, bool allDataReceived);
    virtual bool setSize(unsigned width, unsigned height);
    virtual size_t frameCount();
    virtual int repetitionCount() const;
    virtual void clearFrameBufferCache(size_t clearBeforeFrame);
#else
    virtual bool isSizeAvailable();
#endif
    virtual ImageFrame* frameBufferAtIndex(size_t index);

private:
#if ENABLE(WKC_BLINK_AWEBP)
    size_t decodeFrameCount();
    size_t findRequiredPreviousFrame(size_t frameIndex, bool frameRectIsOpaque);
    void initializeNewFrame(size_t index);
    void decode(size_t index);
    bool decodeSingleFrame(const uint8_t* dataBytes, size_t dataSize, size_t frameIndex);
    bool frameIsCompleteAtIndex(size_t) const;
#else
    bool decode(bool onlySize);
#endif

    WebPIDecoder* m_decoder;
#if ENABLE(WKC_BLINK_AWEBP)
    WebPDecBuffer m_decoderBuffer;
    bool m_frameBackgroundHasAlpha;
#ifdef QCMS_WEBP_COLOR_CORRECTION
    bool m_hasColorProfile;
#endif
#else
    bool m_hasAlpha;
#endif
    int m_formatFlags;

#ifdef QCMS_WEBP_COLOR_CORRECTION
    qcms_transform* colorTransform() const { return m_transform; }
#if ENABLE(WKC_BLINK_AWEBP)
    bool createColorTransform(const char* data, size_t);
    void readColorProfile();
#else
    void createColorTransform(const char* data, size_t);
    void readColorProfile(const uint8_t* data, size_t);
    void applyColorProfile(const uint8_t* data, size_t, ImageFrame&);
#endif
    void clearColorTransform();

    qcms_transform* m_transform;
#if !ENABLE(WKC_BLINK_AWEBP)
    bool m_haveReadProfile;
    int m_decodedHeight;
#endif
#else
#if !ENABLE(WKC_BLINK_AWEBP)
    void applyColorProfile(const uint8_t*, size_t, ImageFrame&) { };
#endif
#endif
    void clear();

#if ENABLE(WKC_BLINK_AWEBP)
    bool updateDemuxer();
    bool initFrameBuffer(size_t index);
    void applyPostProcessing(size_t frameIndex);

    WebPDemuxer* m_demux;
    WebPDemuxState m_demuxState;
    int m_repetitionCount;
    int m_decodedHeight;

    typedef void (*AlphaBlendFunction)(ImageFrame&, ImageFrame&, int, int, int);
    AlphaBlendFunction m_blendFunction;

    void clearDecoder();
#endif
};

} // namespace WebCore

#endif

#endif

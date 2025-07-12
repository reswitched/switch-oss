/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 Intel Inc. All rights reserved.
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

#include "FFTFrame.h"
#include "MathExtras.h"
#include "VectorMath.h"

#include <wkc/wkcmediapeer.h>

namespace WebCore {

FFTFrame::FFTFrame(unsigned fftSize)
    : m_FFTSize(fftSize)
    , m_log2FFTSize(static_cast<unsigned>(log2((double)fftSize)))
    , m_realData((fftSize/2))
    , m_imagData((fftSize/2))
{
    m_peer = wkcAudioFFTCreatePeer(m_FFTSize);
}

FFTFrame::FFTFrame()
    : m_FFTSize(0)
    , m_log2FFTSize(0)
    , m_realData(0)
    , m_imagData(0)
{
    m_peer = wkcAudioFFTCreatePeer(m_FFTSize);
}

FFTFrame::FFTFrame(const FFTFrame& frame)
    : m_FFTSize(frame.m_FFTSize)
    , m_log2FFTSize(frame.m_log2FFTSize)
    , m_realData((frame.m_FFTSize/2))
    , m_imagData((frame.m_FFTSize/2))
{
    int len = sizeof(float) * m_FFTSize / 2;
    ::memcpy(realData().data(), frame.realData().data(), len);
    ::memcpy(imagData().data(), frame.imagData().data(), len);
    m_peer = wkcAudioFFTCreatePeer(m_FFTSize);
}

FFTFrame::~FFTFrame()
{
    if (m_peer)
        wkcAudioFFTDeletePeer(m_peer);
}

void FFTFrame::initialize()
{
    wkcAudioFFTInitializePeer();
}

void FFTFrame::doFFT(const float* data)
{
    if (m_peer)
        wkcAudioFFTDoFFTPeer(m_peer, data, m_realData.data(), m_imagData.data());
}

void FFTFrame::doInverseFFT(float* data)
{
    for (int i=0; i<m_FFTSize/2; i++) {
        data[i] = 0;
    }
    if (m_peer)
        wkcAudioFFTDoInverseFFTPeer(m_peer, data, m_realData.data(), m_imagData.data());
}

int FFTFrame::minFFTSize()
{
    return 0;
}

int FFTFrame::maxFFTSize()
{
    return INT_MAX;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)

/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CARingBuffer.h"

#if ENABLE(WEB_AUDIO) && USE(MEDIATOOLBOX)

#include <CoreAudio/CoreAudioTypes.h>
#include <libkern/OSAtomic.h>
#include <wtf/MathExtras.h>

const uint32_t kGeneralRingTimeBoundsQueueSize = 32;
const uint32_t kGeneralRingTimeBoundsQueueMask = kGeneralRingTimeBoundsQueueSize - 1;

namespace WebCore {

CARingBuffer::CARingBuffer()
    : m_channelCount(0)
    , m_frameCount(0)
    , m_capacityBytes(0)
    , m_timeBoundsQueue(kGeneralRingTimeBoundsQueueSize)
    , m_timeBoundsQueuePtr(0)
{
}

CARingBuffer::~CARingBuffer()
{
    deallocate();
}

void CARingBuffer::allocate(uint32_t channelCount, size_t bytesPerFrame, size_t frameCount)
{
    deallocate();

    frameCount = WTF::roundUpToPowerOfTwo(frameCount);

    m_channelCount = channelCount;
    m_bytesPerFrame = bytesPerFrame;
    m_frameCount = frameCount;
    m_frameCountMask = frameCount - 1;
    m_capacityBytes = bytesPerFrame * frameCount;

    size_t pointersSize = channelCount * sizeof(Byte*);
    size_t allocSize = pointersSize + (m_capacityBytes * channelCount);
    m_buffers = ArrayBuffer::create(allocSize, 1);

    Byte** pointers = static_cast<Byte**>(m_buffers->data());
    Byte* channelData = static_cast<Byte*>(m_buffers->data()) + pointersSize;

    for (unsigned i = 0; i < channelCount; ++i) {
        pointers[i] = channelData;
        channelData += m_capacityBytes;
    }

    for (auto timeBounds : m_timeBoundsQueue) {
        timeBounds.m_startFrame = 0;
        timeBounds.m_endFrame = 0;
        timeBounds.m_updateCounter = 0;
    }
    m_timeBoundsQueuePtr = 0;
}

void CARingBuffer::deallocate()
{
    if (m_buffers)
        m_buffers = nullptr;

    m_channelCount = 0;
    m_capacityBytes = 0;
    m_frameCount = 0;
}

static void ZeroRange(Byte** buffers, int channelCount, size_t offset, size_t nbytes)
{
    while (--channelCount >= 0) {
        memset(*buffers + offset, 0, nbytes);
        ++buffers;
    }
}

static void StoreABL(Byte** buffers, size_t destOffset, const AudioBufferList* list, size_t srcOffset, size_t nbytes)
{
    int channelCount = list->mNumberBuffers;
    const AudioBuffer* src = list->mBuffers;
    while (--channelCount >= 0) {
        if (srcOffset > src->mDataByteSize)
            continue;
        memcpy(*buffers + destOffset, static_cast<Byte*>(src->mData) + srcOffset, std::min<size_t>(nbytes, src->mDataByteSize - srcOffset));
        ++buffers;
        ++src;
    }
}

static void FetchABL(AudioBufferList* list, size_t destOffset, Byte** buffers, size_t srcOffset, size_t nbytes)
{
    int channelCount = list->mNumberBuffers;
    AudioBuffer* dest = list->mBuffers;
    while (--channelCount >= 0) {
        if (destOffset > dest->mDataByteSize)
            continue;
        memcpy(static_cast<Byte*>(dest->mData) + destOffset, *buffers + srcOffset, std::min<size_t>(nbytes, dest->mDataByteSize - destOffset));
        ++buffers;
        ++dest;
    }
}

inline void ZeroABL(AudioBufferList* list, size_t destOffset, size_t nbytes)
{
    int nBuffers = list->mNumberBuffers;
    AudioBuffer* dest = list->mBuffers;
    while (--nBuffers >= 0) {
        if (destOffset > dest->mDataByteSize)
            continue;
        memset(static_cast<Byte*>(dest->mData) + destOffset, 0, std::min<size_t>(nbytes, dest->mDataByteSize - destOffset));
        ++dest;
    }
}

CARingBuffer::Error CARingBuffer::store(const AudioBufferList* list, size_t framesToWrite, uint64_t startFrame)
{
    if (!framesToWrite)
        return Ok;

    if (framesToWrite > m_frameCount)
        return TooMuch;

    uint64_t endFrame = startFrame + framesToWrite;

    if (startFrame < currentEndFrame()) {
        // Throw everything out when going backwards.
        setCurrentFrameBounds(startFrame, startFrame);
    } else if (endFrame - currentStartFrame() <= m_frameCount) {
        // The buffer has not yet wrapped and will not need to.
        // No-op.
    } else {
        // Advance the start time past the region we are about to overwrite
        // starting one buffer of time behind where we're writing.
        uint64_t newStartFrame = endFrame - m_frameCount;
        uint64_t newEndFrame = std::max(newStartFrame, currentEndFrame());
        setCurrentFrameBounds(newStartFrame, newEndFrame);
    }

    // Write the new frames.
    Byte** buffers = static_cast<Byte**>(m_buffers->data());
    size_t offset0;
    size_t offset1;
    uint64_t curEnd = currentEndFrame();

    if (startFrame > curEnd) {
        // We are skipping some samples, so zero the range we are skipping.
        offset0 = frameOffset(curEnd);
        offset1 = frameOffset(startFrame);
        if (offset0 < offset1)
            ZeroRange(buffers, m_channelCount, offset0, offset1 - offset0);
        else {
            ZeroRange(buffers, m_channelCount, offset0, m_capacityBytes - offset0);
            ZeroRange(buffers, m_channelCount, 0, offset1);
        }
        offset0 = offset1;
    } else
        offset0 = frameOffset(startFrame);

    offset1 = frameOffset(endFrame);
    if (offset0 < offset1)
        StoreABL(buffers, offset0, list, 0, offset1 - offset0);
    else {
        size_t nbytes = m_capacityBytes - offset0;
        StoreABL(buffers, offset0, list, 0, nbytes);
        StoreABL(buffers, 0, list, nbytes, offset1);
    }

    // Now update the end time.
    setCurrentFrameBounds(currentStartFrame(), endFrame);

    return Ok;
}

void CARingBuffer::setCurrentFrameBounds(uint64_t startTime, uint64_t endTime)
{
    ByteSpinLocker locker(m_currentFrameBoundsLock);
    uint32_t nextPtr = m_timeBoundsQueuePtr + 1;
    uint32_t index = nextPtr & kGeneralRingTimeBoundsQueueMask;

    m_timeBoundsQueue[index].m_startFrame = startTime;
    m_timeBoundsQueue[index].m_endFrame = endTime;
    m_timeBoundsQueue[index].m_updateCounter = nextPtr;
    OSAtomicIncrement32Barrier(static_cast<int32_t*>(&m_timeBoundsQueuePtr));
}

void CARingBuffer::getCurrentFrameBounds(uint64_t &startTime, uint64_t &endTime)
{
    ByteSpinLocker locker(m_currentFrameBoundsLock);
    uint32_t curPtr = m_timeBoundsQueuePtr;
    uint32_t index = curPtr & kGeneralRingTimeBoundsQueueMask;
    CARingBuffer::TimeBounds& bounds = m_timeBoundsQueue[index];

    startTime = bounds.m_startFrame;
    endTime = bounds.m_endFrame;
}

void CARingBuffer::clipTimeBounds(uint64_t& startRead, uint64_t& endRead)
{
    uint64_t startTime;
    uint64_t endTime;

    getCurrentFrameBounds(startTime, endTime);

    if (startRead > endTime || endRead < startTime) {
        endRead = startRead;
        return;
    }

    startRead = std::max(startRead, startTime);
    endRead = std::min(endRead, endTime);
    endRead = std::max(endRead, startRead);
}

uint64_t CARingBuffer::currentStartFrame() const
{
    uint32_t index = m_timeBoundsQueuePtr & kGeneralRingTimeBoundsQueueMask;
    return m_timeBoundsQueue[index].m_startFrame;
}

uint64_t CARingBuffer::currentEndFrame() const
{
    uint32_t index = m_timeBoundsQueuePtr & kGeneralRingTimeBoundsQueueMask;
    return m_timeBoundsQueue[index].m_endFrame;
}

CARingBuffer::Error CARingBuffer::fetch(AudioBufferList* list, size_t nFrames, uint64_t startRead)
{
    if (!nFrames)
        return Ok;
    
    startRead = std::max<uint64_t>(0, startRead);
    
    uint64_t endRead = startRead + nFrames;
    
    uint64_t startRead0 = startRead;
    uint64_t endRead0 = endRead;
    
    clipTimeBounds(startRead, endRead);

    if (startRead == endRead) {
        ZeroABL(list, 0, nFrames * m_bytesPerFrame);
        return Ok;
    }
    
    size_t byteSize = static_cast<size_t>((endRead - startRead) * m_bytesPerFrame);
    
    size_t destStartByteOffset = static_cast<size_t>(std::max<uint64_t>(0, (startRead - startRead0) * m_bytesPerFrame));
    
    if (destStartByteOffset > 0)
        ZeroABL(list, 0, std::min<size_t>(nFrames * m_bytesPerFrame, destStartByteOffset));

    size_t destEndSize = static_cast<size_t>(std::max<uint64_t>(0, endRead0 - endRead));
    if (destEndSize > 0)
        ZeroABL(list, destStartByteOffset + byteSize, destEndSize * m_bytesPerFrame);

    Byte **buffers = static_cast<Byte**>(m_buffers->data());
    size_t offset0 = frameOffset(startRead);
    size_t offset1 = frameOffset(endRead);
    size_t nbytes;
    
    if (offset0 < offset1) {
        nbytes = offset1 - offset0;
        FetchABL(list, destStartByteOffset, buffers, offset0, nbytes);
    } else {
        nbytes = m_capacityBytes - offset0;
        FetchABL(list, destStartByteOffset, buffers, offset0, nbytes);
        FetchABL(list, destStartByteOffset + nbytes, buffers, 0, offset1);
        nbytes += offset1;
    }
    
    int channelCount = list->mNumberBuffers;
    AudioBuffer* dest = list->mBuffers;
    while (--channelCount >= 0) {
        dest->mDataByteSize = nbytes;
        dest++;
    }
    
    return Ok;
}

}

#endif // ENABLE(WEB_AUDIO) && USE(MEDIATOOLBOX)

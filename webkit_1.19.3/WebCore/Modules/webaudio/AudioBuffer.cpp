/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioBuffer.h"

#include "AudioContext.h"
#include "AudioFileReader.h"
#include <JavaScriptCore/JSCInlines.h>
#include <JavaScriptCore/TypedArrayInlines.h>

namespace WebCore {

RefPtr<AudioBuffer> AudioBuffer::create(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate, LegacyPreventNeutering preventNeutering)
{
    if (sampleRate < 22050 || sampleRate > 96000 || numberOfChannels > AudioContext::maxNumberOfChannels() || !numberOfFrames)
        return nullptr;

    auto buffer = adoptRef(*new AudioBuffer(numberOfChannels, numberOfFrames, sampleRate, preventNeutering));
    if (!buffer->originalLength())
        return nullptr;

    return WTFMove(buffer);
}

ExceptionOr<Ref<AudioBuffer>> AudioBuffer::create(const AudioBufferOptions& options)
{
    if (options.numberOfChannels > AudioContext::maxNumberOfChannels())
        return Exception { NotSupportedError, "Number of channels cannot be more than max supported."_s };
    
    if (!options.length)
        return Exception { NotSupportedError, "Length must be at least 1."_s };
    
    if (options.sampleRate < 22050 || options.sampleRate > 96000)
        return Exception { NotSupportedError, "Sample rate is not in the supported range."_s };
    
    auto buffer = adoptRef(*new AudioBuffer(options.numberOfChannels, options.length, options.sampleRate));
    if (!buffer->originalLength())
        return Exception { NotSupportedError, "Channel was not able to be created."_s };
    
    return buffer;
}

RefPtr<AudioBuffer> AudioBuffer::createFromAudioFileData(const void* data, size_t dataSize, bool mixToMono, float sampleRate)
{
    RefPtr<AudioBus> bus = createBusFromInMemoryAudioFile(data, dataSize, mixToMono, sampleRate);
    if (!bus)
        return nullptr;
    return adoptRef(*new AudioBuffer(*bus));
}

AudioBuffer::AudioBuffer(unsigned numberOfChannels, size_t length, float sampleRate, LegacyPreventNeutering preventNeutering)
    : m_sampleRate(sampleRate)
    , m_originalLength(length)
    , m_isDetachable(preventNeutering == LegacyPreventNeutering::No)
{
    m_channels.reserveCapacity(numberOfChannels);

    for (unsigned i = 0; i < numberOfChannels; ++i) {
        auto channelDataArray = Float32Array::create(m_originalLength);
        if (!channelDataArray) {
            invalidate();
            break;
        }

        if (preventNeutering == LegacyPreventNeutering::Yes)
            channelDataArray->setNeuterable(false);

        m_channels.append(WTFMove(channelDataArray));
    }
}

AudioBuffer::AudioBuffer(AudioBus& bus)
    : m_sampleRate(bus.sampleRate())
    , m_originalLength(bus.length())
{
    // Copy audio data from the bus to the Float32Arrays we manage.
    unsigned numberOfChannels = bus.numberOfChannels();
    m_channels.reserveCapacity(numberOfChannels);
    for (unsigned i = 0; i < numberOfChannels; ++i) {
        auto channelDataArray = Float32Array::create(m_originalLength);
        if (!channelDataArray) {
            invalidate();
            break;
        }

        channelDataArray->setRange(bus.channel(i)->data(), m_originalLength, 0);
        m_channels.append(WTFMove(channelDataArray));
    }
}

void AudioBuffer::invalidate()
{
    releaseMemory();
    m_originalLength = 0;
}

void AudioBuffer::releaseMemory()
{
    auto locker = holdLock(m_channelsLock);
    m_channels.clear();
}

ExceptionOr<Ref<Float32Array>> AudioBuffer::getChannelData(unsigned channelIndex)
{
    if (channelIndex >= m_channels.size())
        return Exception { SyntaxError };
    auto& channelData = *m_channels[channelIndex];
    auto array = Float32Array::create(channelData.unsharedBuffer(), channelData.byteOffset(), channelData.length());
    RELEASE_ASSERT(array);
    return array.releaseNonNull();
}

RefPtr<Float32Array> AudioBuffer::channelData(unsigned channelIndex)
{
    if (channelIndex >= m_channels.size())
        return nullptr;
    if (hasDetachedChannelBuffer())
        return Float32Array::create(0);
    return m_channels[channelIndex].copyRef();
}

float* AudioBuffer::rawChannelData(unsigned channelIndex)
{
    if (channelIndex >= m_channels.size())
        return nullptr;
    if (hasDetachedChannelBuffer())
        return nullptr;
    return m_channels[channelIndex]->data();
}

void AudioBuffer::zero()
{
    for (auto& channel : m_channels)
        channel->zeroFill();
}

size_t AudioBuffer::memoryCost() const
{
    // memoryCost() may be invoked concurrently from a GC thread, and we need to be careful
    // about what data we access here and how. We need to hold a lock to prevent m_channels
    // from being changed while we iterate it, but calling channel->byteLength() is safe
    // because it doesn't involve chasing any pointers that can be nullified while the
    // AudioBuffer is alive.
    auto locker = holdLock(m_channelsLock);
    size_t cost = 0;
    for (auto& channel : m_channels)
        cost += channel->byteLength();
    return cost;
}

bool AudioBuffer::hasDetachedChannelBuffer() const
{
    for (auto& channel : m_channels) {
        if (channel->isNeutered())
            return true;
    }
    return false;
}

bool AudioBuffer::topologyMatches(const AudioBuffer& other) const
{
    return numberOfChannels() == other.numberOfChannels() && length() == other.length() && sampleRate() == other.sampleRate();
}

bool AudioBuffer::copyTo(AudioBuffer& other) const
{
    if (!topologyMatches(other))
        return false;

    if (hasDetachedChannelBuffer() || other.hasDetachedChannelBuffer())
        return false;

    for (unsigned channelIndex = 0; channelIndex < numberOfChannels(); ++channelIndex)
        memcpy(other.rawChannelData(channelIndex), m_channels[channelIndex]->data(), length() * sizeof(float));

    return true;
}

Ref<AudioBuffer> AudioBuffer::clone(ShouldCopyChannelData shouldCopyChannelData) const
{
    auto clone = AudioBuffer::create(numberOfChannels(), length(), sampleRate(), m_isDetachable ? LegacyPreventNeutering::No : LegacyPreventNeutering::Yes);
    ASSERT(clone);
    if (shouldCopyChannelData == ShouldCopyChannelData::Yes)
        copyTo(*clone);
    return clone.releaseNonNull();
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)

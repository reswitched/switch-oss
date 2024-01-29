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

#include "config.h"

#if ENABLE(WEB_AUDIO)

#if PLATFORM(MAC)

#include "AudioDestinationMac.h"

#include "AudioIOCallback.h"
#include "AudioSession.h"
#include "FloatConversion.h"
#include "Logging.h"
#include "PlatformMediaSessionManager.h"
#include "VectorMath.h"
#include <CoreAudio/AudioHardware.h>

namespace WebCore {

const int kBufferSize = 128;
const float kLowThreshold = -1;
const float kHighThreshold = 1;

// Factory method: Mac-implementation
std::unique_ptr<AudioDestination> AudioDestination::create(AudioIOCallback& callback, const String&, unsigned numberOfInputChannels, unsigned numberOfOutputChannels, float sampleRate)
{
    // FIXME: make use of inputDeviceId as appropriate.

    // FIXME: Add support for local/live audio input.
    if (numberOfInputChannels)
        LOG(Media, "AudioDestination::create(%u, %u, %f) - unhandled input channels", numberOfInputChannels, numberOfOutputChannels, sampleRate);

    // FIXME: Add support for multi-channel (> stereo) output.
    if (numberOfOutputChannels != 2)
        LOG(Media, "AudioDestination::create(%u, %u, %f) - unhandled output channels", numberOfInputChannels, numberOfOutputChannels, sampleRate);

    return std::make_unique<AudioDestinationMac>(callback, sampleRate);
}

float AudioDestination::hardwareSampleRate()
{
    // Determine the default output device's sample-rate.
    return AudioSession::sharedSession().sampleRate();
}

unsigned long AudioDestination::maxChannelCount()
{
    // FIXME: query the default audio hardware device to return the actual number
    // of channels of the device. Also see corresponding FIXME in create().
    // There is a small amount of code which assumes stereo in AudioDestinationMac which
    // can be upgraded.
    return 0;
}

AudioDestinationMac::AudioDestinationMac(AudioIOCallback& callback, float sampleRate)
    : m_outputUnit(0)
    , m_callback(callback)
    , m_renderBus(AudioBus::create(2, kBufferSize, false))
    , m_sampleRate(sampleRate)
    , m_isPlaying(false)
{
    // Open and initialize DefaultOutputUnit
    AudioComponent comp;
    AudioComponentDescription desc;

    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    comp = AudioComponentFindNext(0, &desc);

    ASSERT(comp);

    OSStatus result = AudioComponentInstanceNew(comp, &m_outputUnit);
    ASSERT(!result);

    result = AudioUnitInitialize(m_outputUnit);
    ASSERT(!result);

    configure();
}

AudioDestinationMac::~AudioDestinationMac()
{
    if (m_outputUnit)
        AudioComponentInstanceDispose(m_outputUnit);
}

void AudioDestinationMac::configure()
{
    // Set render callback
    AURenderCallbackStruct input;
    input.inputProc = inputProc;
    input.inputProcRefCon = this;
    OSStatus result = AudioUnitSetProperty(m_outputUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Global, 0, &input, sizeof(input));
    ASSERT(!result);

    // Set stream format
    AudioStreamBasicDescription streamFormat;
    streamFormat.mSampleRate = m_sampleRate;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
    streamFormat.mBitsPerChannel = 8 * sizeof(Float32);
    streamFormat.mChannelsPerFrame = 2;
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerPacket = sizeof(Float32);
    streamFormat.mBytesPerFrame = sizeof(Float32);

    result = AudioUnitSetProperty(m_outputUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, (void*)&streamFormat, sizeof(AudioStreamBasicDescription));
    ASSERT(!result);
}

void AudioDestinationMac::start()
{
    OSStatus result = AudioOutputUnitStart(m_outputUnit);

    if (!result)
        setIsPlaying(true);
}

void AudioDestinationMac::stop()
{
    OSStatus result = AudioOutputUnitStop(m_outputUnit);

    if (!result)
        setIsPlaying(false);
}

// Pulls on our provider to get rendered audio stream.
OSStatus AudioDestinationMac::render(UInt32 numberOfFrames, AudioBufferList* ioData)
{
    AudioBuffer* buffers = ioData->mBuffers;
    m_renderBus->setChannelMemory(0, (float*)buffers[0].mData, numberOfFrames);
    m_renderBus->setChannelMemory(1, (float*)buffers[1].mData, numberOfFrames);

    // FIXME: Add support for local/live audio input.
    m_callback.render(0, m_renderBus.get(), numberOfFrames);

    // Clamp values at 0db (i.e., [-1.0, 1.0])
    for (unsigned i = 0; i < m_renderBus->numberOfChannels(); ++i) {
        AudioChannel* channel = m_renderBus->channel(i);
        VectorMath::vclip(channel->data(), 1, &kLowThreshold, &kHighThreshold, channel->mutableData(), 1, numberOfFrames);
    }

    return noErr;
}

void AudioDestinationMac::setIsPlaying(bool isPlaying)
{
    if (m_isPlaying == isPlaying)
        return;

    m_isPlaying = isPlaying;
    m_callback.isPlayingDidChange();
}

// DefaultOutputUnit callback
OSStatus AudioDestinationMac::inputProc(void* userData, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32 /*busNumber*/, UInt32 numberOfFrames, AudioBufferList* ioData)
{
    AudioDestinationMac* audioOutput = static_cast<AudioDestinationMac*>(userData);
    return audioOutput->render(numberOfFrames, ioData);
}

} // namespace WebCore

#endif // PLATFORM(MAC)

#endif // ENABLE(WEB_AUDIO)

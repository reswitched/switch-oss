/*
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "AudioSourceProviderAVFObjC.h"

#if ENABLE(WEB_AUDIO) && USE(MEDIATOOLBOX)

#import "AudioBus.h"
#import "AudioChannel.h"
#import "AudioSourceProviderClient.h"
#import "CARingBuffer.h"
#import "Logging.h"
#import "MediaTimeAVFoundation.h"
#import <AVFoundation/AVAssetTrack.h>
#import <AVFoundation/AVAudioMix.h>
#import <AVFoundation/AVMediaFormat.h>
#import <AVFoundation/AVPlayerItem.h>
#import <objc/runtime.h>
#import <wtf/MainThread.h>

#if !LOG_DISABLED
#import <wtf/StringPrintStream.h>
#endif

#import "CoreMediaSoftLink.h"

SOFT_LINK_FRAMEWORK(AVFoundation)
SOFT_LINK_FRAMEWORK(MediaToolbox)
SOFT_LINK_FRAMEWORK(AudioToolbox)

SOFT_LINK_CLASS(AVFoundation, AVPlayerItem)
SOFT_LINK_CLASS(AVFoundation, AVMutableAudioMix)
SOFT_LINK_CLASS(AVFoundation, AVMutableAudioMixInputParameters)

SOFT_LINK(AudioToolbox, AudioConverterConvertComplexBuffer, OSStatus, (AudioConverterRef inAudioConverter, UInt32 inNumberPCMFrames, const AudioBufferList* inInputData, AudioBufferList* outOutputData), (inAudioConverter, inNumberPCMFrames, inInputData, outOutputData))
SOFT_LINK(AudioToolbox, AudioConverterNew, OSStatus, (const AudioStreamBasicDescription* inSourceFormat, const AudioStreamBasicDescription* inDestinationFormat, AudioConverterRef* outAudioConverter), (inSourceFormat, inDestinationFormat, outAudioConverter))

SOFT_LINK(MediaToolbox, MTAudioProcessingTapGetStorage, void*, (MTAudioProcessingTapRef tap), (tap))
SOFT_LINK(MediaToolbox, MTAudioProcessingTapGetSourceAudio, OSStatus, (MTAudioProcessingTapRef tap, CMItemCount numberFrames, AudioBufferList *bufferListInOut, MTAudioProcessingTapFlags *flagsOut, CMTimeRange *timeRangeOut, CMItemCount *numberFramesOut), (tap, numberFrames, bufferListInOut, flagsOut, timeRangeOut, numberFramesOut))
SOFT_LINK_MAY_FAIL(MediaToolbox, MTAudioProcessingTapCreate, OSStatus, (CFAllocatorRef allocator, const MTAudioProcessingTapCallbacks *callbacks, MTAudioProcessingTapCreationFlags flags, MTAudioProcessingTapRef *tapOut), (allocator, callbacks, flags, tapOut))

SOFT_LINK_POINTER(AVFoundation, AVMediaCharacteristicAudible, NSString *)
#define AVMediaCharacteristicAudible getAVMediaCharacteristicAudible()

namespace WebCore {

static const double kRingBufferDuration = 1;

RefPtr<AudioSourceProviderAVFObjC> AudioSourceProviderAVFObjC::create(AVPlayerItem *item)
{
    if (!canLoadMTAudioProcessingTapCreate())
        return nullptr;
    return adoptRef(*new AudioSourceProviderAVFObjC(item));
}

AudioSourceProviderAVFObjC::AudioSourceProviderAVFObjC(AVPlayerItem *item)
    : m_avPlayerItem(item)
    , m_writeCount(0)
    , m_readCount(0)
    , m_paused(true)
    , m_client(nullptr)
{
}

AudioSourceProviderAVFObjC::~AudioSourceProviderAVFObjC()
{
    setClient(nullptr);
}

void AudioSourceProviderAVFObjC::provideInput(AudioBus* bus, size_t framesToProcess)
{
    if (!m_avPlayerItem)
        return;

    uint64_t startFrame = 0;
    uint64_t endFrame = 0;
    m_ringBuffer->getCurrentFrameBounds(startFrame, endFrame);

    if (m_writeCount <= m_readCount + m_writeAheadCount) {
        bus->zero();
        return;
    }

    size_t framesAvailable = static_cast<size_t>(endFrame - (m_readCount + m_writeAheadCount));
    if (framesAvailable < framesToProcess) {
        framesToProcess = framesAvailable;
        bus->zero();
    }

    ASSERT(bus->numberOfChannels() == m_ringBuffer->channelCount());

    for (unsigned i = 0; i < m_list->mNumberBuffers; ++i) {
        AudioChannel* channel = bus->channel(i);
        m_list->mBuffers[i].mNumberChannels = 1;
        m_list->mBuffers[i].mData = channel->mutableData();
        m_list->mBuffers[i].mDataByteSize = channel->length() * sizeof(float);
    }

    m_ringBuffer->fetch(m_list.get(), framesToProcess, m_readCount);
    m_readCount += framesToProcess;

    if (m_converter)
        AudioConverterConvertComplexBuffer(m_converter.get(), framesToProcess, m_list.get(), m_list.get());
}

void AudioSourceProviderAVFObjC::setClient(AudioSourceProviderClient* client)
{
    if (m_client == client)
        return;

    if (m_avAudioMix)
        destroyMix();

    m_client = client;

    if (m_client && m_avPlayerItem)
        createMix();
}

void AudioSourceProviderAVFObjC::setPlayerItem(AVPlayerItem *avPlayerItem)
{
    if (m_avPlayerItem == avPlayerItem)
        return;

    if (m_avAudioMix)
        destroyMix();

    m_avPlayerItem = avPlayerItem;

    if (m_client && m_avPlayerItem && m_avAssetTrack)
        createMix();
}

void AudioSourceProviderAVFObjC::setAudioTrack(AVAssetTrack *avAssetTrack)
{
    if (m_avAssetTrack == avAssetTrack)
        return;

    if (m_avAudioMix)
        destroyMix();

    m_avAssetTrack = avAssetTrack;

    if (m_client && m_avPlayerItem && m_avAssetTrack)
        createMix();
}

void AudioSourceProviderAVFObjC::destroyMix()
{
    if (m_avPlayerItem)
        [m_avPlayerItem setAudioMix:nil];
    [m_avAudioMix setInputParameters:@[ ]];
    m_avAudioMix.clear();
    m_tap.clear();
}

void AudioSourceProviderAVFObjC::createMix()
{
    ASSERT(!m_avAudioMix);
    ASSERT(m_avPlayerItem);
    ASSERT(m_client);

    m_avAudioMix = adoptNS([allocAVMutableAudioMixInstance() init]);

    MTAudioProcessingTapCallbacks callbacks = {
        0,
        this,
        initCallback,
        finalizeCallback,
        prepareCallback,
        unprepareCallback,
        processCallback,
    };

    MTAudioProcessingTapRef tap = nullptr;
    MTAudioProcessingTapCreate(kCFAllocatorDefault, &callbacks, 1, &tap);
    ASSERT(tap);
    ASSERT(m_tap == tap);

    RetainPtr<AVMutableAudioMixInputParameters> parameters = adoptNS([allocAVMutableAudioMixInputParametersInstance() init]);
    [parameters setAudioTapProcessor:m_tap.get()];

    CMPersistentTrackID trackID = m_avAssetTrack.get().trackID;
    [parameters setTrackID:trackID];
    
    [m_avAudioMix setInputParameters:@[parameters.get()]];
    [m_avPlayerItem setAudioMix:m_avAudioMix.get()];
}

void AudioSourceProviderAVFObjC::initCallback(MTAudioProcessingTapRef tap, void* clientInfo, void** tapStorageOut)
{
    AudioSourceProviderAVFObjC* _this = static_cast<AudioSourceProviderAVFObjC*>(clientInfo);
    _this->m_tap = tap;
    _this->init(clientInfo, tapStorageOut);
}

void AudioSourceProviderAVFObjC::finalizeCallback(MTAudioProcessingTapRef tap)
{
    ASSERT(tap);
    AudioSourceProviderAVFObjC* _this = static_cast<AudioSourceProviderAVFObjC*>(MTAudioProcessingTapGetStorage(tap));
    _this->finalize();
}

void AudioSourceProviderAVFObjC::prepareCallback(MTAudioProcessingTapRef tap, CMItemCount maxFrames, const AudioStreamBasicDescription *processingFormat)
{
    ASSERT(tap);
    AudioSourceProviderAVFObjC* _this = static_cast<AudioSourceProviderAVFObjC*>(MTAudioProcessingTapGetStorage(tap));
    _this->prepare(maxFrames, processingFormat);
}

void AudioSourceProviderAVFObjC::unprepareCallback(MTAudioProcessingTapRef tap)
{
    ASSERT(tap);
    AudioSourceProviderAVFObjC* _this = static_cast<AudioSourceProviderAVFObjC*>(MTAudioProcessingTapGetStorage(tap));
    _this->unprepare();
}

void AudioSourceProviderAVFObjC::processCallback(MTAudioProcessingTapRef tap, CMItemCount numberFrames, MTAudioProcessingTapFlags flags, AudioBufferList *bufferListInOut, CMItemCount *numberFramesOut, MTAudioProcessingTapFlags *flagsOut)
{
    ASSERT(tap);
    AudioSourceProviderAVFObjC* _this = static_cast<AudioSourceProviderAVFObjC*>(MTAudioProcessingTapGetStorage(tap));
    _this->process(numberFrames, flags, bufferListInOut, numberFramesOut, flagsOut);
}

void AudioSourceProviderAVFObjC::init(void* clientInfo, void** tapStorageOut)
{
    ASSERT(clientInfo == this);
    UNUSED_PARAM(clientInfo);
    *tapStorageOut = this;
}

void AudioSourceProviderAVFObjC::finalize()
{
}

static bool operator==(const AudioStreamBasicDescription& a, const AudioStreamBasicDescription& b)
{
    return a.mSampleRate == b.mSampleRate
        && a.mFormatID == b.mFormatID
        && a.mFormatFlags == b.mFormatFlags
        && a.mBytesPerPacket == b.mBytesPerPacket
        && a.mFramesPerPacket == b.mFramesPerPacket
        && a.mBytesPerFrame == b.mBytesPerFrame
        && a.mChannelsPerFrame == b.mChannelsPerFrame
        && a.mBitsPerChannel == b.mBitsPerChannel;
}

static bool operator!=(const AudioStreamBasicDescription& a, const AudioStreamBasicDescription& b)
{
    return !(a == b);
}

void AudioSourceProviderAVFObjC::prepare(CMItemCount maxFrames, const AudioStreamBasicDescription *processingFormat)
{
    ASSERT(maxFrames >= 0);

    m_tapDescription = std::make_unique<AudioStreamBasicDescription>(*processingFormat);
    int numberOfChannels = processingFormat->mChannelsPerFrame;
    size_t bytesPerFrame = processingFormat->mBytesPerFrame;
    double sampleRate = processingFormat->mSampleRate;
    ASSERT(sampleRate >= 0);

    m_outputDescription = std::make_unique<AudioStreamBasicDescription>();
    m_outputDescription->mSampleRate = sampleRate;
    m_outputDescription->mFormatID = kAudioFormatLinearPCM;
    m_outputDescription->mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
    m_outputDescription->mBitsPerChannel = 8 * sizeof(Float32);
    m_outputDescription->mChannelsPerFrame = numberOfChannels;
    m_outputDescription->mFramesPerPacket = 1;
    m_outputDescription->mBytesPerPacket = sizeof(Float32);
    m_outputDescription->mBytesPerFrame = sizeof(Float32);
    m_outputDescription->mFormatFlags |= kAudioFormatFlagIsNonInterleaved;

    if (*m_tapDescription != *m_outputDescription) {
        AudioConverterRef outConverter = nullptr;
        AudioConverterNew(m_tapDescription.get(), m_outputDescription.get(), &outConverter);
        m_converter = outConverter;
    }

    // Make the ringbuffer large enough to store at least two callbacks worth of audio, or 1s, whichever is larger.
    size_t capacity = std::max(static_cast<size_t>(2 * maxFrames), static_cast<size_t>(kRingBufferDuration * sampleRate));

    m_ringBuffer = std::make_unique<CARingBuffer>();
    m_ringBuffer->allocate(numberOfChannels, bytesPerFrame, capacity);

    // AudioBufferList is a variable-length struct, so create on the heap with a generic new() operator
    // with a custom size, and initialize the struct manually.
    size_t bufferListSize = sizeof(AudioBufferList) + (sizeof(AudioBuffer) * std::max(1, numberOfChannels - 1));
    m_list = std::unique_ptr<AudioBufferList>((AudioBufferList*) ::operator new (bufferListSize));
    memset(m_list.get(), 0, bufferListSize);
    m_list->mNumberBuffers = numberOfChannels;

    RefPtr<AudioSourceProviderAVFObjC> strongThis = this;
    callOnMainThread([strongThis, numberOfChannels, sampleRate] {
        strongThis->m_client->setFormat(numberOfChannels, sampleRate);
    });
}

void AudioSourceProviderAVFObjC::unprepare()
{
    m_tapDescription = nullptr;
    m_outputDescription = nullptr;
    m_ringBuffer = nullptr;
    m_list = nullptr;
}

void AudioSourceProviderAVFObjC::process(CMItemCount numberOfFrames, MTAudioProcessingTapFlags flags, AudioBufferList* bufferListInOut, CMItemCount* numberFramesOut, MTAudioProcessingTapFlags* flagsOut)
{
    UNUSED_PARAM(flags);

    CMItemCount itemCount = 0;
    CMTimeRange rangeOut;
    OSStatus status = MTAudioProcessingTapGetSourceAudio(m_tap.get(), numberOfFrames, bufferListInOut, flagsOut, &rangeOut, &itemCount);
    if (status != noErr || !itemCount)
        return;

    MediaTime rangeStart = toMediaTime(rangeOut.start);
    MediaTime rangeDuration = toMediaTime(rangeOut.duration);

    if (rangeStart.isInvalid())
        return;

    MediaTime currentTime = toMediaTime(CMTimebaseGetTime([m_avPlayerItem timebase]));
    if (currentTime.isInvalid())
        return;

    // The audio tap will generate silence when the media is paused, and will not advance the
    // tap currentTime.
    if (rangeStart == m_startTimeAtLastProcess || rangeDuration == MediaTime::zeroTime()) {
        m_paused = true;
        return;
    }

    if (m_paused) {
        // Only check the write-ahead time when playback begins.
        m_paused = false;
        MediaTime earlyBy = rangeStart - currentTime;
        m_writeAheadCount = m_tapDescription->mSampleRate * earlyBy.toDouble();
    }

    // Check to see if the underlying media has seeked, which would require us to "flush"
    // our outstanding buffers.
    if (rangeStart != m_endTimeAtLastProcess)
        m_readCount = m_writeCount;

    m_startTimeAtLastProcess = rangeStart;
    m_endTimeAtLastProcess = rangeStart + rangeDuration;

    // StartOfStream indicates a discontinuity, such as when an AVPlayerItem is re-added
    // to an AVPlayer, so "flush" outstanding buffers.
    if (flagsOut && *flagsOut & kMTAudioProcessingTapFlag_StartOfStream)
        m_readCount = m_writeCount;

    m_ringBuffer->store(bufferListInOut, itemCount, m_writeCount);
    m_writeCount += itemCount;

    // Mute the default audio playback by zeroing the tap-owned buffers.
    for (uint32_t i = 0; i < bufferListInOut->mNumberBuffers; ++i) {
        AudioBuffer& buffer = bufferListInOut->mBuffers[i];
        memset(buffer.mData, 0, buffer.mDataByteSize);
    }
    *numberFramesOut = 0;
}

}

#endif // ENABLE(WEB_AUDIO) && USE(MEDIATOOLBOX)

/*
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

#include "AudioDestination.h"
#include "AudioIOCallback.h"
#include "AudioSourceProviderClient.h"
#include "AudioBus.h"

#include "Threading.h"

#include <wkc/wkcmediapeer.h>

namespace WebCore {

static const unsigned int c_busFrameSize = 128; // 3ms window

class AudioDestinationWKC final : public AudioDestination, AudioSourceProviderClient, public RefCounted<AudioDestinationWKC>
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    static AudioDestinationWKC* create(AudioIOCallback&, float sampleRate);

    // AudioDestination
    virtual ~AudioDestinationWKC();

    virtual void start(Function<void(Function<void()>&&)>&& dispatchToRenderThread, CompletionHandler<void(bool)>&& completionHandler) override;
    virtual void stop(CompletionHandler<void(bool)> && = [](bool) {}) override;
    virtual bool isPlaying() override;

    virtual float sampleRate() const override;

    virtual unsigned framesPerBuffer() const override;

    // AudioSourceProviderClient
    virtual void setFormat(size_t numberOfChannels, float sampleRate) override;

    virtual void ref() const override { RefCounted<AudioDestinationWKC>::ref(); }
    virtual void deref() const override { RefCounted<AudioDestinationWKC>::deref(); }

private:
    AudioDestinationWKC(AudioIOCallback&, float sampleRate);
    bool construct();
    static void threadProc(void* obj) { ((AudioDestinationWKC *)obj)->drain(); }
    void drain();

private:
    RefPtr<AudioBus> m_bus;
    float m_sampleRate;
    size_t m_channels;

    void* m_peer;

    RefPtr<Thread> m_thread;
    bool m_quit;
};

AudioDestinationWKC::AudioDestinationWKC(AudioIOCallback& cb, float sampleRate)
    : AudioDestination(cb)
    , m_bus(WebCore::AudioBus::create(2, c_busFrameSize, true))
    , m_sampleRate(sampleRate)
    , m_channels(2)
    , m_peer(0)
    , m_thread()
    , m_quit(false)
{
}

AudioDestinationWKC::~AudioDestinationWKC()
{
    if (m_thread) {
        m_quit = true;
        m_thread->detach();
    }

    if (m_peer)
        wkcAudioClosePeer(m_peer);
}

AudioDestinationWKC*
AudioDestinationWKC::create(AudioIOCallback& cb, float sampleRate)
{
    AudioDestinationWKC* self = 0;
    self = new AudioDestinationWKC(cb, sampleRate);
    if (!self->construct()) {
        delete self;
        return 0;
    }
    return self;
}

bool
AudioDestinationWKC::construct()
{
    return true;
}

void
AudioDestinationWKC::setFormat(size_t numberOfChannels, float sampleRate)
{
    m_channels = numberOfChannels;
    m_sampleRate = sampleRate;
}

void
AudioDestinationWKC::start(Function<void(Function<void()>&&)>&& dispatchToRenderThread, CompletionHandler<void(bool)>&& completionHandler)
{
    if (m_peer) {
        return;
    }

    m_peer = wkcAudioOpenPeer(wkcAudioPreferredSampleRatePeer(), 16, 2, 0);

    if (!m_peer) {
        return;
    }

    m_quit = false;
    m_thread = Thread::create("WKC: AudioDestination", [this] {
        drain();
    });

    completionHandler(true);

    if (!m_thread) {
        wkcAudioClosePeer(m_peer);
        m_peer = 0;
    }
}

void
AudioDestinationWKC::stop(CompletionHandler<void(bool)>&& completionHandler)
{
    if (m_thread) {
        m_quit = true;

        // Wait for the audio thread to quit
        m_thread->waitForCompletion();
        m_thread = nullptr;
    }

    completionHandler(true);

    if (m_peer) {
        wkcAudioClosePeer(m_peer);
        m_peer = 0;
    }
}

bool
AudioDestinationWKC::isPlaying()
{
    return m_peer ? true : false;
}

float
AudioDestinationWKC::sampleRate() const
{
    return m_sampleRate;
}

void AudioDestinationWKC::drain()
{
    if (m_quit || !m_peer)
        return;

    float** dataArray = static_cast<float**>(WTF::fastMalloc(m_channels * sizeof(float*)));
    float* maxAbsValueArray = static_cast<float*>(WTF::fastMalloc(m_channels * sizeof(float)));

    unsigned int channels = 0;
    bool shouldDrainNextData = true;

    while (!m_quit) {
        if (!wkcThreadCheckAlivePeer()) {
            // Should not reach here.
            return;
        }
        wkc_usleep(1000);

        if (shouldDrainNextData) {
            // Update on a 3ms window
            callRenderCallback(nullptr, m_bus.get(), c_busFrameSize, { });

            channels = m_bus->numberOfChannels();
            for (unsigned int i = 0; i < channels; ++i) {
                dataArray[i] = m_bus->channel(i)->mutableData();
                maxAbsValueArray[i] = m_bus->channel(i)->maxAbsValue();
            }
        }

        shouldDrainNextData = wkcAudioWriteRawPeer(m_peer, dataArray, channels, c_busFrameSize, maxAbsValueArray);
    }

    WTF::fastFree(maxAbsValueArray);
    WTF::fastFree(dataArray);
}

unsigned AudioDestinationWKC::framesPerBuffer() const
{
    return m_bus->length();
}

Ref<AudioDestination> AudioDestination::create(AudioIOCallback& cb, const String& inputDeviceId, unsigned numberOfInputChannels, unsigned numberOfOutputChannels, float sampleRate)
{
    return Ref<AudioDestination>(*AudioDestinationWKC::create(cb, sampleRate));
}

float AudioDestination::hardwareSampleRate()
{
    return wkcAudioPreferredSampleRatePeer();
}

unsigned long AudioDestination::maxChannelCount()
{
    return 0;
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)

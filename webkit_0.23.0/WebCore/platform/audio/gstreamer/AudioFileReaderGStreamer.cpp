/*
 *  Copyright (C) 2011, 2012 Igalia S.L
 *  Copyright (C) 2011 Zan Dobersek  <zandobersek@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioFileReader.h"

#include "AudioBus.h"

#include <gio/gio.h>
#include <gst/app/gstappsink.h>
#include <gst/audio/audio-info.h>
#include <gst/gst.h>
#include <wtf/Noncopyable.h>
#include <wtf/glib/GRefPtr.h>
#include <wtf/glib/GThreadSafeMainLoopSource.h>
#include <wtf/glib/GUniquePtr.h>

namespace WebCore {

class AudioFileReader {
    WTF_MAKE_NONCOPYABLE(AudioFileReader);
public:
    AudioFileReader(const char* filePath);
    AudioFileReader(const void* data, size_t dataSize);
    ~AudioFileReader();

    PassRefPtr<AudioBus> createBus(float sampleRate, bool mixToMono);

    GstFlowReturn handleSample(GstAppSink*);
    gboolean handleMessage(GstMessage*);
    void handleNewDeinterleavePad(GstPad*);
    void deinterleavePadsConfigured();
    void plugDeinterleave(GstPad*);
    void decodeAudioForBusCreation();

private:
    const void* m_data;
    size_t m_dataSize;
    const char* m_filePath;

    float m_sampleRate;
    int m_channels;
    GstBufferList* m_frontLeftBuffers;
    GstBufferList* m_frontRightBuffers;

    GstElement* m_pipeline;
    unsigned m_channelSize;
    GRefPtr<GstElement> m_decodebin;
    GRefPtr<GstElement> m_deInterleave;
    GRefPtr<GMainLoop> m_loop;
    bool m_errorOccurred;
};

static void copyGstreamerBuffersToAudioChannel(GstBufferList* buffers, AudioChannel* audioChannel)
{
    float* destination = audioChannel->mutableData();
    unsigned bufferCount = gst_buffer_list_length(buffers);
    for (unsigned i = 0; i < bufferCount; ++i) {
        GstBuffer* buffer = gst_buffer_list_get(buffers, i);
        ASSERT(buffer);
        gsize bufferSize = gst_buffer_get_size(buffer);
        gst_buffer_extract(buffer, 0, destination, bufferSize);
        destination += bufferSize / sizeof(float);
    }
}

static GstFlowReturn onAppsinkPullRequiredCallback(GstAppSink* sink, gpointer userData)
{
    return static_cast<AudioFileReader*>(userData)->handleSample(sink);
}

gboolean messageCallback(GstBus*, GstMessage* message, AudioFileReader* reader)
{
    return reader->handleMessage(message);
}

static void onGStreamerDeinterleavePadAddedCallback(GstElement*, GstPad* pad, AudioFileReader* reader)
{
    reader->handleNewDeinterleavePad(pad);
}

static void onGStreamerDeinterleaveReadyCallback(GstElement*, AudioFileReader* reader)
{
    reader->deinterleavePadsConfigured();
}

static void onGStreamerDecodebinPadAddedCallback(GstElement*, GstPad* pad, AudioFileReader* reader)
{
    reader->plugDeinterleave(pad);
}

AudioFileReader::AudioFileReader(const char* filePath)
    : m_data(0)
    , m_dataSize(0)
    , m_filePath(filePath)
    , m_channelSize(0)
    , m_errorOccurred(false)
{
}

AudioFileReader::AudioFileReader(const void* data, size_t dataSize)
    : m_data(data)
    , m_dataSize(dataSize)
    , m_filePath(0)
    , m_channelSize(0)
    , m_errorOccurred(false)
{
}

AudioFileReader::~AudioFileReader()
{
    if (m_pipeline) {
        GRefPtr<GstBus> bus = adoptGRef(gst_pipeline_get_bus(GST_PIPELINE(m_pipeline)));
        ASSERT(bus);
        g_signal_handlers_disconnect_by_func(bus.get(), reinterpret_cast<gpointer>(messageCallback), this);

        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(m_pipeline));
    }

    if (m_decodebin) {
        g_signal_handlers_disconnect_by_func(m_decodebin.get(), reinterpret_cast<gpointer>(onGStreamerDecodebinPadAddedCallback), this);
        m_decodebin.clear();
    }

    if (m_deInterleave) {
        g_signal_handlers_disconnect_by_func(m_deInterleave.get(), reinterpret_cast<gpointer>(onGStreamerDeinterleavePadAddedCallback), this);
        g_signal_handlers_disconnect_by_func(m_deInterleave.get(), reinterpret_cast<gpointer>(onGStreamerDeinterleaveReadyCallback), this);
        m_deInterleave.clear();
    }

    gst_buffer_list_unref(m_frontLeftBuffers);
    gst_buffer_list_unref(m_frontRightBuffers);
}

GstFlowReturn AudioFileReader::handleSample(GstAppSink* sink)
{
    GstSample* sample = gst_app_sink_pull_sample(sink);
    if (!sample)
        return GST_FLOW_ERROR;

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    GstCaps* caps = gst_sample_get_caps(sample);
    if (!caps) {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    GstAudioInfo info;
    gst_audio_info_from_caps(&info, caps);
    int frames = gst_buffer_get_size(buffer) / info.bpf;

    // Check the first audio channel. The buffer is supposed to store
    // data of a single channel anyway.
    switch (GST_AUDIO_INFO_POSITION(&info, 0)) {
    case GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT:
    case GST_AUDIO_CHANNEL_POSITION_MONO:
        gst_buffer_list_add(m_frontLeftBuffers, gst_buffer_ref(buffer));
        m_channelSize += frames;
        break;
    case GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT:
        gst_buffer_list_add(m_frontRightBuffers, gst_buffer_ref(buffer));
        break;
    default:
        break;
    }

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

gboolean AudioFileReader::handleMessage(GstMessage* message)
{
    GUniqueOutPtr<GError> error;
    GUniqueOutPtr<gchar> debug;

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
        g_main_loop_quit(m_loop.get());
        break;
    case GST_MESSAGE_WARNING:
        gst_message_parse_warning(message, &error.outPtr(), &debug.outPtr());
        g_warning("Warning: %d, %s. Debug output: %s", error->code,  error->message, debug.get());
        break;
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(message, &error.outPtr(), &debug.outPtr());
        g_warning("Error: %d, %s. Debug output: %s", error->code,  error->message, debug.get());
        m_errorOccurred = true;
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        g_main_loop_quit(m_loop.get());
        break;
    default:
        break;
    }
    return TRUE;
}

void AudioFileReader::handleNewDeinterleavePad(GstPad* pad)
{
    // A new pad for a planar channel was added in deinterleave. Plug
    // in an appsink so we can pull the data from each
    // channel. Pipeline looks like:
    // ... deinterleave ! queue ! appsink.
    GstElement* queue = gst_element_factory_make("queue", 0);
    GstElement* sink = gst_element_factory_make("appsink", 0);

    GstAppSinkCallbacks callbacks;
    callbacks.eos = 0;
    callbacks.new_preroll = 0;
    callbacks.new_sample = onAppsinkPullRequiredCallback;
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &callbacks, this, 0);

    g_object_set(sink, "sync", FALSE, NULL);

    gst_bin_add_many(GST_BIN(m_pipeline), queue, sink, NULL);

    GstPad* sinkPad = gst_element_get_static_pad(queue, "sink");
    gst_pad_link_full(pad, sinkPad, GST_PAD_LINK_CHECK_NOTHING);
    gst_object_unref(GST_OBJECT(sinkPad));

    gst_element_link_pads_full(queue, "src", sink, "sink", GST_PAD_LINK_CHECK_NOTHING);

    gst_element_sync_state_with_parent(queue);
    gst_element_sync_state_with_parent(sink);
}

void AudioFileReader::deinterleavePadsConfigured()
{
    // All deinterleave src pads are now available, let's roll to
    // PLAYING so data flows towards the sinks and it can be retrieved.
    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
}

void AudioFileReader::plugDeinterleave(GstPad* pad)
{
    // Ignore any additional source pads just in case.
    if (m_deInterleave)
        return;

    // A decodebin pad was added, plug in a deinterleave element to
    // separate each planar channel. Sub pipeline looks like
    // ... decodebin2 ! audioconvert ! audioresample ! capsfilter ! deinterleave.
    GstElement* audioConvert  = gst_element_factory_make("audioconvert", 0);
    GstElement* audioResample = gst_element_factory_make("audioresample", 0);
    GstElement* capsFilter = gst_element_factory_make("capsfilter", 0);
    m_deInterleave = gst_element_factory_make("deinterleave", "deinterleave");

    g_object_set(m_deInterleave.get(), "keep-positions", TRUE, NULL);
    g_signal_connect(m_deInterleave.get(), "pad-added", G_CALLBACK(onGStreamerDeinterleavePadAddedCallback), this);
    g_signal_connect(m_deInterleave.get(), "no-more-pads", G_CALLBACK(onGStreamerDeinterleaveReadyCallback), this);

    GstCaps* caps = gst_caps_new_simple("audio/x-raw",
        "rate", G_TYPE_INT, static_cast<int>(m_sampleRate),
        "channels", G_TYPE_INT, m_channels,
        "format", G_TYPE_STRING, GST_AUDIO_NE(F32),
        "layout", G_TYPE_STRING, "interleaved", nullptr);
    g_object_set(capsFilter, "caps", caps, NULL);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(m_pipeline), audioConvert, audioResample, capsFilter, m_deInterleave.get(), NULL);

    GstPad* sinkPad = gst_element_get_static_pad(audioConvert, "sink");
    gst_pad_link_full(pad, sinkPad, GST_PAD_LINK_CHECK_NOTHING);
    gst_object_unref(GST_OBJECT(sinkPad));

    gst_element_link_pads_full(audioConvert, "src", audioResample, "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_link_pads_full(audioResample, "src", capsFilter, "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_link_pads_full(capsFilter, "src", m_deInterleave.get(), "sink", GST_PAD_LINK_CHECK_NOTHING);

    gst_element_sync_state_with_parent(audioConvert);
    gst_element_sync_state_with_parent(audioResample);
    gst_element_sync_state_with_parent(capsFilter);
    gst_element_sync_state_with_parent(m_deInterleave.get());
}

void AudioFileReader::decodeAudioForBusCreation()
{
    // Build the pipeline (giostreamsrc | filesrc) ! decodebin2
    // A deinterleave element is added once a src pad becomes available in decodebin.
    m_pipeline = gst_pipeline_new(0);

    GRefPtr<GstBus> bus = adoptGRef(gst_pipeline_get_bus(GST_PIPELINE(m_pipeline)));
    ASSERT(bus);
    gst_bus_add_signal_watch(bus.get());
    g_signal_connect(bus.get(), "message", G_CALLBACK(messageCallback), this);

    GstElement* source;
    if (m_data) {
        ASSERT(m_dataSize);
        source = gst_element_factory_make("giostreamsrc", 0);
        GRefPtr<GInputStream> memoryStream = adoptGRef(g_memory_input_stream_new_from_data(m_data, m_dataSize, 0));
        g_object_set(source, "stream", memoryStream.get(), NULL);
    } else {
        source = gst_element_factory_make("filesrc", 0);
        g_object_set(source, "location", m_filePath, NULL);
    }

    m_decodebin = gst_element_factory_make("decodebin", "decodebin");
    g_signal_connect(m_decodebin.get(), "pad-added", G_CALLBACK(onGStreamerDecodebinPadAddedCallback), this);

    gst_bin_add_many(GST_BIN(m_pipeline), source, m_decodebin.get(), NULL);
    gst_element_link_pads_full(source, "src", m_decodebin.get(), "sink", GST_PAD_LINK_CHECK_NOTHING);

    // Catch errors here immediately, there might not be an error message if
    // we're unlucky.
    if (gst_element_set_state(m_pipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
        g_warning("Error: Failed to set pipeline to PAUSED");
        m_errorOccurred = true;
        g_main_loop_quit(m_loop.get());
    }
}

PassRefPtr<AudioBus> AudioFileReader::createBus(float sampleRate, bool mixToMono)
{
    m_sampleRate = sampleRate;
    m_channels = mixToMono ? 1 : 2;

    m_frontLeftBuffers = gst_buffer_list_new();
    m_frontRightBuffers = gst_buffer_list_new();

    GRefPtr<GMainContext> context = adoptGRef(g_main_context_new());
    g_main_context_push_thread_default(context.get());
    m_loop = adoptGRef(g_main_loop_new(context.get(), FALSE));

    // Start the pipeline processing just after the loop is started.
    GThreadSafeMainLoopSource source;
    source.schedule("[WebKit] AudioFileReader::decodeAudioForBusCreation", std::function<void()>(std::bind(&AudioFileReader::decodeAudioForBusCreation, this)), G_PRIORITY_DEFAULT, nullptr, context.get());

    g_main_loop_run(m_loop.get());
    g_main_context_pop_thread_default(context.get());

    // Set pipeline to GST_STATE_NULL state here already ASAP to
    // release any resources that might still be used.
    gst_element_set_state(m_pipeline, GST_STATE_NULL);

    if (m_errorOccurred)
        return 0;

    RefPtr<AudioBus> audioBus = AudioBus::create(m_channels, m_channelSize, true);
    audioBus->setSampleRate(m_sampleRate);

    copyGstreamerBuffersToAudioChannel(m_frontLeftBuffers, audioBus->channel(0));
    if (!mixToMono)
        copyGstreamerBuffersToAudioChannel(m_frontRightBuffers, audioBus->channel(1));

    return audioBus;
}

PassRefPtr<AudioBus> createBusFromAudioFile(const char* filePath, bool mixToMono, float sampleRate)
{
    return AudioFileReader(filePath).createBus(sampleRate, mixToMono);
}

PassRefPtr<AudioBus> createBusFromInMemoryAudioFile(const void* data, size_t dataSize, bool mixToMono, float sampleRate)
{
    return AudioFileReader(data, dataSize).createBus(sampleRate, mixToMono);
}

} // WebCore

#endif // ENABLE(WEB_AUDIO)

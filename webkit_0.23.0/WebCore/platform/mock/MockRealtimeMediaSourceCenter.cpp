/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2013 Apple Inc.  All rights reserved.
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
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

#include "config.h"

#if ENABLE(MEDIA_STREAM)
#include "MockRealtimeMediaSourceCenter.h"

#include "MediaConstraintsMock.h"
#include "MediaStream.h"
#include "MediaStreamCreationClient.h"
#include "MediaStreamPrivate.h"
#include "MediaStreamTrack.h"
#include "MediaStreamTrackSourcesRequestClient.h"
#include "RealtimeMediaSource.h"
#include "RealtimeMediaSourceCapabilities.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

class MockSource : public RealtimeMediaSource {
public:
    MockSource(const AtomicString& id, const AtomicString& name, RealtimeMediaSource::Type type)
        : RealtimeMediaSource(id, type, name)
    {
    }

    virtual ~MockSource() { }

    virtual RefPtr<RealtimeMediaSourceCapabilities> capabilities() const { return m_capabilities; }
    virtual const RealtimeMediaSourceStates& states() { return m_currentStates; }

    RefPtr<RealtimeMediaSourceCapabilities> m_capabilities;
    RealtimeMediaSourceStates m_currentStates;
};
    
typedef HashMap<String, RefPtr<MockSource>> MockSourceMap;

static MockSourceMap& mockSourceMap()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(MockSourceMap, mockSourceMap, ());
    return mockSourceMap;
}

static const AtomicString& mockAudioSourceID()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> id("239c24b1-2b15-11e3-8224-0800200c9a66", AtomicString::ConstructFromLiteral);
    return id;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, id, 0);
    if (!id)
        id = new AtomicString("239c24b1-2b15-11e3-8224-0800200c9a66", AtomicString::ConstructFromLiteral);
    return *id;
#endif
}

static const AtomicString& mockVideoSourceID()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> id("239c24b0-2b15-11e3-8224-0800200c9a66", AtomicString::ConstructFromLiteral);
    return id;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, id, 0);
    if (!id)
        id = new AtomicString("239c24b0-2b15-11e3-8224-0800200c9a66", AtomicString::ConstructFromLiteral);
    return *id;
#endif
}

static void initializeMockSources()
{
    RefPtr<MockSource> mockSource1 = adoptRef(new MockSource(mockVideoSourceID(), "Mock video device", RealtimeMediaSource::Video));
    mockSource1->m_capabilities = RealtimeMediaSourceCapabilities::create();
    mockSource1->m_capabilities->setSourceId(mockSource1->id());
    mockSource1->m_capabilities->addSourceType(RealtimeMediaSourceStates::Camera);
    mockSource1->m_capabilities->addSourceType(RealtimeMediaSourceStates::Microphone);
    mockSource1->m_capabilities->addFacingMode(RealtimeMediaSourceStates::User);
    mockSource1->m_capabilities->addFacingMode(RealtimeMediaSourceStates::Environment);
    mockSource1->m_capabilities->setWidthRange(RealtimeMediaSourceCapabilityRange(320UL, 1920UL, true));
    mockSource1->m_capabilities->setHeightRange(RealtimeMediaSourceCapabilityRange(240UL, 1080UL, true));
    mockSource1->m_capabilities->setFrameRateRange(RealtimeMediaSourceCapabilityRange(15.0f, 60.0f, true));
    mockSource1->m_capabilities->setAspectRatioRange(RealtimeMediaSourceCapabilityRange(4 / 3.0f, 16 / 9.0f, true));
    mockSource1->m_capabilities->setVolumeRange(RealtimeMediaSourceCapabilityRange(10UL, 90UL, true));

    mockSource1->m_currentStates.setSourceType(RealtimeMediaSourceStates::Camera);
    mockSource1->m_currentStates.setSourceId(mockSource1->id());
    mockSource1->m_currentStates.setFacingMode(RealtimeMediaSourceStates::User);
    mockSource1->m_currentStates.setWidth(1920);
    mockSource1->m_currentStates.setHeight(1080);
    mockSource1->m_currentStates.setFrameRate(30);
    mockSource1->m_currentStates.setAspectRatio(16 / 9.0f);
    mockSource1->m_currentStates.setVolume(70);
    String mockSource1id = mockSource1->id();
    mockSourceMap().add(mockSource1id, mockSource1.release());

    RefPtr<MockSource> mockSource2 = adoptRef(new MockSource(mockAudioSourceID(), "Mock audio device", RealtimeMediaSource::Audio));
    mockSource2->m_capabilities = RealtimeMediaSourceCapabilities::create();
    mockSource2->m_capabilities->setSourceId(mockSource2->id());
    mockSource2->m_capabilities->setVolumeRange(RealtimeMediaSourceCapabilityRange(0UL, 100UL, true));

    mockSource2->m_currentStates.setSourceType(RealtimeMediaSourceStates::Microphone);
    mockSource2->m_currentStates.setSourceId(mockSource2->id());
    mockSource2->m_currentStates.setVolume(50);
    String mockSource2id = mockSource2->id();
    mockSourceMap().add(mockSource2id, mockSource2.release());
}

void MockRealtimeMediaSourceCenter::registerMockRealtimeMediaSourceCenter()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(MockRealtimeMediaSourceCenter, center, ());
#if !PLATFORM(WKC)
    static bool registered = false;
#else
    WKC_DEFINE_STATIC_BOOL(registered, false);
#endif
    if (!registered) {
        registered = true;
        RealtimeMediaSourceCenter::setSharedStreamCenter(&center);
        initializeMockSources();
    }
}

void MockRealtimeMediaSourceCenter::validateRequestConstraints(PassRefPtr<MediaStreamCreationClient> prpQueryClient, PassRefPtr<MediaConstraints> audioConstraints, PassRefPtr<MediaConstraints> videoConstraints)
{
    RefPtr<MediaStreamCreationClient> client = prpQueryClient;
    
    ASSERT(client);
    
    if (audioConstraints) {
        String invalidQuery = MediaConstraintsMock::verifyConstraints(audioConstraints);
        if (!invalidQuery.isEmpty()) {
            client->constraintsInvalid(invalidQuery);
            return;
        }
    }
    
    if (videoConstraints) {
        String invalidQuery = MediaConstraintsMock::verifyConstraints(videoConstraints);
        if (!invalidQuery.isEmpty()) {
            client->constraintsInvalid(invalidQuery);
            return;
        }
    }

    client->constraintsValidated();
}

void MockRealtimeMediaSourceCenter::createMediaStream(PassRefPtr<MediaStreamCreationClient> prpQueryClient, PassRefPtr<MediaConstraints> audioConstraints, PassRefPtr<MediaConstraints> videoConstraints)
{
    RefPtr<MediaStreamCreationClient> client = prpQueryClient;

    ASSERT(client);
    
    Vector<RefPtr<RealtimeMediaSource>> audioSources;
    Vector<RefPtr<RealtimeMediaSource>> videoSources;
    MockSourceMap& map = mockSourceMap();

    if (audioConstraints) {
        String invalidQuery = MediaConstraintsMock::verifyConstraints(audioConstraints);
        if (!invalidQuery.isEmpty()) {
            client->failedToCreateStreamWithConstraintsError(invalidQuery);
            return;
        }

        MockSourceMap::iterator it = map.find(mockAudioSourceID());
        ASSERT(it != map.end());

        RefPtr<RealtimeMediaSource> audioSource = it->value;
        audioSource->reset();
        audioSources.append(audioSource.release());
    }

    if (videoConstraints) {
        String invalidQuery = MediaConstraintsMock::verifyConstraints(videoConstraints);
        if (!invalidQuery.isEmpty()) {
            client->failedToCreateStreamWithConstraintsError(invalidQuery);
            return;
        }

        MockSourceMap::iterator it = map.find(mockVideoSourceID());
        ASSERT(it != map.end());

        RefPtr<RealtimeMediaSource> videoSource = it->value;
        videoSource->reset();
        videoSources.append(videoSource.release());
    }
    
    client->didCreateStream(MediaStreamPrivate::create(audioSources, videoSources));
}

bool MockRealtimeMediaSourceCenter::getMediaStreamTrackSources(PassRefPtr<MediaStreamTrackSourcesRequestClient> prpClient)
{
    RefPtr<MediaStreamTrackSourcesRequestClient> requestClient = prpClient;
    Vector<RefPtr<TrackSourceInfo>> sources;

    MockSourceMap& map = mockSourceMap();
    MockSourceMap::iterator end = map.end();
    for (MockSourceMap::iterator it = map.begin(); it != end; ++it) {
        MockSource* source = it->value.get();

        sources.append(TrackSourceInfo::create(source->id(), source->type() == RealtimeMediaSource::Video ? TrackSourceInfo::Video : TrackSourceInfo::Audio, source->name()));
    }

    requestClient->didCompleteRequest(sources);
    return true;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

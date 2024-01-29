/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(MEDIA_STREAM)

#include "RTCPeerConnection.h"

#include "ArrayValue.h"
#include "Document.h"
#include "Event.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "MediaStreamEvent.h"
#include "RTCConfiguration.h"
#include "RTCDTMFSender.h"
#include "RTCDataChannel.h"
#include "RTCDataChannelEvent.h"
#include "RTCDataChannelHandler.h"
#include "RTCIceCandidate.h"
#include "RTCIceCandidateDescriptor.h"
#include "RTCIceCandidateEvent.h"
#include "RTCOfferAnswerOptions.h"
#include "RTCPeerConnectionErrorCallback.h"
#include "RTCSessionDescription.h"
#include "RTCSessionDescriptionCallback.h"
#include "RTCSessionDescriptionDescriptor.h"
#include "RTCSessionDescriptionRequestImpl.h"
#include "RTCStatsCallback.h"
#include "RTCStatsRequestImpl.h"
#include "RTCVoidRequestImpl.h"
#include "ScriptExecutionContext.h"
#include "VoidCallback.h"
#include <wtf/MainThread.h>

namespace WebCore {

static bool validateIceServerURL(const String& iceURL)
{
    URL url(URL(), iceURL);
    if (url.isEmpty() || !url.isValid() || !(url.protocolIs("turn") || url.protocolIs("stun")))
        return false;

    return true;
}

static ExceptionCode processIceServer(const Dictionary& iceServer, RTCConfiguration* rtcConfiguration)
{
    String credential, username;
    iceServer.get("credential", credential);
    iceServer.get("username", username);

    // Spec says that "urls" can be either a string or a sequence, so we must check for both.
    Vector<String> urlsList;
    String urlString;
    iceServer.get("urls", urlString);
    // This is the only way to check if "urls" is a sequence or a string. If we try to convert
    // to a sequence and it fails (in case it is a string), an exception will be set and the
    // RTCPeerConnection will fail.
    // So we convert to a string always, which converts a sequence to a string in the format: "foo, bar, ..",
    // then checking for a comma in the string assures that a string was a sequence and then we convert
    // it to a sequence safely.
    if (urlString.isEmpty())
        return INVALID_ACCESS_ERR;

    if (urlString.find(',') != notFound && iceServer.get("urls", urlsList) && urlsList.size()) {
        for (auto& url : urlsList) {
            if (!validateIceServerURL(url))
                return INVALID_ACCESS_ERR;
        }
    } else {
        if (!validateIceServerURL(urlString))
            return INVALID_ACCESS_ERR;

        urlsList.append(urlString);
    }

    rtcConfiguration->appendServer(RTCIceServer::create(urlsList, credential, username));
    return 0;
}

RefPtr<RTCConfiguration> RTCPeerConnection::parseConfiguration(const Dictionary& configuration, ExceptionCode& ec)
{
    if (configuration.isUndefinedOrNull())
        return nullptr;

    ArrayValue iceServers;
    bool ok = configuration.get("iceServers", iceServers);
    if (!ok || iceServers.isUndefinedOrNull()) {
        ec = TYPE_MISMATCH_ERR;
        return nullptr;
    }

    size_t numberOfServers;
    ok = iceServers.length(numberOfServers);
    if (!ok || !numberOfServers) {
        ec = !ok ? TYPE_MISMATCH_ERR : INVALID_ACCESS_ERR;
        return nullptr;
    }

    String iceTransports;
    String requestIdentity;
    configuration.get("iceTransports", iceTransports);
    configuration.get("requestIdentity", requestIdentity);

    RefPtr<RTCConfiguration> rtcConfiguration = RTCConfiguration::create();

    rtcConfiguration->setIceTransports(iceTransports);
    rtcConfiguration->setRequestIdentity(requestIdentity);

    for (size_t i = 0; i < numberOfServers; ++i) {
        Dictionary iceServer;
        ok = iceServers.get(i, iceServer);
        if (!ok) {
            ec = TYPE_MISMATCH_ERR;
            return nullptr;
        }

        ec = processIceServer(iceServer, rtcConfiguration.get());
        if (ec)
            return nullptr;
    }

    return rtcConfiguration;
}

RefPtr<RTCPeerConnection> RTCPeerConnection::create(ScriptExecutionContext& context, const Dictionary& rtcConfiguration, ExceptionCode& ec)
{
    RefPtr<RTCConfiguration> configuration = parseConfiguration(rtcConfiguration, ec);
    if (ec)
        return nullptr;

    RefPtr<RTCPeerConnection> peerConnection = adoptRef(new RTCPeerConnection(context, configuration.release(), ec));
    peerConnection->suspendIfNeeded();
    if (ec)
        return nullptr;

    return peerConnection.release();
}

RTCPeerConnection::RTCPeerConnection(ScriptExecutionContext& context, PassRefPtr<RTCConfiguration> configuration, ExceptionCode& ec)
    : ActiveDOMObject(&context)
    , m_signalingState(SignalingStateStable)
    , m_iceGatheringState(IceGatheringStateNew)
    , m_iceConnectionState(IceConnectionStateNew)
    , m_scheduledEventTimer(*this, &RTCPeerConnection::scheduledEventTimerFired)
    , m_configuration(configuration)
    , m_stopped(false)
{
    Document& document = downcast<Document>(context);

    if (!document.frame()) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    m_peerHandler = RTCPeerConnectionHandler::create(this);
    if (!m_peerHandler) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    document.frame()->loader().client().dispatchWillStartUsingPeerConnectionHandler(m_peerHandler.get());

    if (!m_peerHandler->initialize(m_configuration->privateConfiguration())) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }
}

RTCPeerConnection::~RTCPeerConnection()
{
    stop();

    for (auto& localStream : m_localStreams)
        localStream->removeObserver(this);
}

void RTCPeerConnection::createOffer(PassRefPtr<RTCSessionDescriptionCallback> successCallback, PassRefPtr<RTCPeerConnectionErrorCallback> errorCallback, const Dictionary& offerOptions, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!successCallback) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<RTCOfferOptions> options = RTCOfferOptions::create(offerOptions, ec);
    if (ec) {
        callOnMainThread([=] {
            RefPtr<DOMError> error = DOMError::create("Invalid createOffer argument.");
            errorCallback->handleEvent(error.get());
        });
        return;
    }

    RefPtr<RTCSessionDescriptionRequestImpl> request = RTCSessionDescriptionRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);
    m_peerHandler->createOffer(request.release(), options->privateOfferOptions());
}

void RTCPeerConnection::createAnswer(PassRefPtr<RTCSessionDescriptionCallback> successCallback, PassRefPtr<RTCPeerConnectionErrorCallback> errorCallback, const Dictionary& answerOptions, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!successCallback) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<RTCOfferAnswerOptions> options = RTCOfferAnswerOptions::create(answerOptions, ec);
    if (ec) {
        callOnMainThread([=] {
            RefPtr<DOMError> error = DOMError::create("Invalid createAnswer argument.");
            errorCallback->handleEvent(error.get());
        });
        return;
    }

    RefPtr<RTCSessionDescriptionRequestImpl> request = RTCSessionDescriptionRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);
    m_peerHandler->createAnswer(request.release(), options->privateOfferAnswerOptions());
}

bool RTCPeerConnection::checkStateForLocalDescription(RTCSessionDescription* localDescription)
{
    if (localDescription->type() == "offer")
        return m_signalingState == SignalingStateStable || m_signalingState == SignalingStateHaveLocalOffer;
    if (localDescription->type() == "answer")
        return m_signalingState == SignalingStateHaveRemoteOffer || m_signalingState == SignalingStateHaveLocalPrAnswer;
    if (localDescription->type() == "pranswer")
        return m_signalingState == SignalingStateHaveLocalPrAnswer || m_signalingState == SignalingStateHaveRemoteOffer;

    return false;
}

bool RTCPeerConnection::checkStateForRemoteDescription(RTCSessionDescription* remoteDescription)
{
    if (remoteDescription->type() == "offer")
        return m_signalingState == SignalingStateStable || m_signalingState == SignalingStateHaveRemoteOffer;
    if (remoteDescription->type() == "answer")
        return m_signalingState == SignalingStateHaveLocalOffer || m_signalingState == SignalingStateHaveRemotePrAnswer;
    if (remoteDescription->type() == "pranswer")
        return m_signalingState == SignalingStateHaveRemotePrAnswer || m_signalingState == SignalingStateHaveLocalOffer;

    return false;
}

void RTCPeerConnection::setLocalDescription(PassRefPtr<RTCSessionDescription> prpSessionDescription, PassRefPtr<VoidCallback> successCallback, PassRefPtr<RTCPeerConnectionErrorCallback> errorCallback, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    RefPtr<RTCSessionDescription> sessionDescription = prpSessionDescription;
    if (!sessionDescription) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    if (!checkStateForLocalDescription(sessionDescription.get())) {
        callOnMainThread([=] {
            RefPtr<DOMError> error = DOMError::create(RTCPeerConnectionHandler::invalidSessionDescriptionErrorName());
            errorCallback->handleEvent(error.get());
        });
        return;
    }

    RefPtr<RTCVoidRequestImpl> request = RTCVoidRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);
    m_peerHandler->setLocalDescription(request.release(), sessionDescription->descriptor());
}

PassRefPtr<RTCSessionDescription> RTCPeerConnection::localDescription(ExceptionCode& ec)
{
    RefPtr<RTCSessionDescriptionDescriptor> descriptor = m_peerHandler->localDescription();
    if (!descriptor) {
        ec = INVALID_STATE_ERR;
        return nullptr;
    }

    RefPtr<RTCSessionDescription> sessionDescription = RTCSessionDescription::create(descriptor.release());
    return sessionDescription.release();
}

void RTCPeerConnection::setRemoteDescription(PassRefPtr<RTCSessionDescription> prpSessionDescription, PassRefPtr<VoidCallback> successCallback, PassRefPtr<RTCPeerConnectionErrorCallback> errorCallback, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    RefPtr<RTCSessionDescription> sessionDescription = prpSessionDescription;
    if (!sessionDescription) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    if (!checkStateForRemoteDescription(sessionDescription.get())) {
        callOnMainThread([=] {
            RefPtr<DOMError> error = DOMError::create(RTCPeerConnectionHandler::invalidSessionDescriptionErrorName());
            errorCallback->handleEvent(error.get());
        });
        return;
    }

    RefPtr<RTCVoidRequestImpl> request = RTCVoidRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);
    m_peerHandler->setRemoteDescription(request.release(), sessionDescription->descriptor());
}

PassRefPtr<RTCSessionDescription> RTCPeerConnection::remoteDescription(ExceptionCode& ec)
{
    RefPtr<RTCSessionDescriptionDescriptor> descriptor = m_peerHandler->remoteDescription();
    if (!descriptor) {
        ec = INVALID_STATE_ERR;
        return nullptr;
    }

    RefPtr<RTCSessionDescription> desc = RTCSessionDescription::create(descriptor.release());
    return desc.release();
}

void RTCPeerConnection::updateIce(const Dictionary& rtcConfiguration, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    m_configuration = parseConfiguration(rtcConfiguration, ec);
    if (ec)
        return;

    bool valid = m_peerHandler->updateIce(m_configuration->privateConfiguration());
    if (!valid)
        ec = SYNTAX_ERR;
}

void RTCPeerConnection::addIceCandidate(RTCIceCandidate* iceCandidate, PassRefPtr<VoidCallback> successCallback, PassRefPtr<RTCPeerConnectionErrorCallback> errorCallback, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!iceCandidate || !successCallback || !errorCallback) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<RTCVoidRequestImpl> request = RTCVoidRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);

    bool implemented = m_peerHandler->addIceCandidate(request.release(), iceCandidate->descriptor());
    if (!implemented)
        ec = SYNTAX_ERR;
}

String RTCPeerConnection::signalingState() const
{
    switch (m_signalingState) {
    case SignalingStateStable:
        return ASCIILiteral("stable");
    case SignalingStateHaveLocalOffer:
        return ASCIILiteral("have-local-offer");
    case SignalingStateHaveRemoteOffer:
        return ASCIILiteral("have-remote-offer");
    case SignalingStateHaveLocalPrAnswer:
        return ASCIILiteral("have-local-pranswer");
    case SignalingStateHaveRemotePrAnswer:
        return ASCIILiteral("have-remote-pranswer");
    case SignalingStateClosed:
        return ASCIILiteral("closed");
    }

    ASSERT_NOT_REACHED();
    return String();
}

String RTCPeerConnection::iceGatheringState() const
{
    switch (m_iceGatheringState) {
    case IceGatheringStateNew:
        return ASCIILiteral("new");
    case IceGatheringStateGathering:
        return ASCIILiteral("gathering");
    case IceGatheringStateComplete:
        return ASCIILiteral("complete");
    }

    ASSERT_NOT_REACHED();
    return String();
}

String RTCPeerConnection::iceConnectionState() const
{
    switch (m_iceConnectionState) {
    case IceConnectionStateNew:
        return ASCIILiteral("new");
    case IceConnectionStateChecking:
        return ASCIILiteral("checking");
    case IceConnectionStateConnected:
        return ASCIILiteral("connected");
    case IceConnectionStateCompleted:
        return ASCIILiteral("completed");
    case IceConnectionStateFailed:
        return ASCIILiteral("failed");
    case IceConnectionStateDisconnected:
        return ASCIILiteral("disconnected");
    case IceConnectionStateClosed:
        return ASCIILiteral("closed");
    }

    ASSERT_NOT_REACHED();
    return String();
}

void RTCPeerConnection::addStream(PassRefPtr<MediaStream> prpStream, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    RefPtr<MediaStream> stream = prpStream;
    if (!stream) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    if (m_localStreams.contains(stream))
        return;

    bool valid = m_peerHandler->addStream(stream->privateStream());
    if (!valid)
        ec = SYNTAX_ERR;
    else {
        m_localStreams.append(stream);
        stream->addObserver(this);
    }
}

void RTCPeerConnection::removeStream(PassRefPtr<MediaStream> prpStream, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!prpStream) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<MediaStream> stream = prpStream;

    size_t pos = m_localStreams.find(stream);
    if (pos == notFound)
        return;

    m_localStreams.remove(pos);
    stream->removeObserver(this);
    m_peerHandler->removeStream(stream->privateStream());
}

RTCConfiguration* RTCPeerConnection::getConfiguration() const
{
    return m_configuration.get();
}

Vector<RefPtr<MediaStream>> RTCPeerConnection::getLocalStreams() const
{
    return m_localStreams;
}

Vector<RefPtr<MediaStream>> RTCPeerConnection::getRemoteStreams() const
{
    return m_remoteStreams;
}

MediaStream* RTCPeerConnection::getStreamById(const String& streamId)
{
    for (auto& localStream : m_localStreams) {
        if (localStream->id() == streamId)
            return localStream.get();
    }

    for (auto& remoteStream : m_remoteStreams) {
        if (remoteStream->id() == streamId)
            return remoteStream.get();
    }

    return nullptr;
}

void RTCPeerConnection::getStats(PassRefPtr<RTCStatsCallback> successCallback, PassRefPtr<RTCPeerConnectionErrorCallback> errorCallback, PassRefPtr<MediaStreamTrack> selector)
{
    RefPtr<RTCStatsRequestImpl> statsRequest = RTCStatsRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback, &selector->privateTrack());
    // FIXME: Add passing selector as part of the statsRequest.
    m_peerHandler->getStats(statsRequest.release());
}

PassRefPtr<RTCDataChannel> RTCPeerConnection::createDataChannel(String label, const Dictionary& options, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return nullptr;
    }

    RefPtr<RTCDataChannel> channel = RTCDataChannel::create(scriptExecutionContext(), m_peerHandler.get(), label, options, ec);
    if (ec)
        return nullptr;

    m_dataChannels.append(channel);
    return channel.release();
}

bool RTCPeerConnection::hasLocalStreamWithTrackId(const String& trackId)
{
    for (auto& localStream : m_localStreams) {
        if (localStream->getTrackById(trackId))
            return true;
    }
    return false;
}

PassRefPtr<RTCDTMFSender> RTCPeerConnection::createDTMFSender(PassRefPtr<MediaStreamTrack> prpTrack, ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return nullptr;
    }

    if (!prpTrack) {
        ec = TypeError;
        return nullptr;
    }

    RefPtr<MediaStreamTrack> track = prpTrack;

    if (!hasLocalStreamWithTrackId(track->id())) {
        ec = SYNTAX_ERR;
        return nullptr;
    }

    RefPtr<RTCDTMFSender> dtmfSender = RTCDTMFSender::create(scriptExecutionContext(), m_peerHandler.get(), track.release(), ec);
    if (ec)
        return nullptr;
    return dtmfSender.release();
}

void RTCPeerConnection::close(ExceptionCode& ec)
{
    if (m_signalingState == SignalingStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    m_peerHandler->stop();

    changeIceConnectionState(IceConnectionStateClosed);
    changeIceGatheringState(IceGatheringStateComplete);
    changeSignalingState(SignalingStateClosed);
}

void RTCPeerConnection::negotiationNeeded()
{
    scheduleDispatchEvent(Event::create(eventNames().negotiationneededEvent, false, false));
}

void RTCPeerConnection::didGenerateIceCandidate(PassRefPtr<RTCIceCandidateDescriptor> iceCandidateDescriptor)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    if (!iceCandidateDescriptor)
        scheduleDispatchEvent(RTCIceCandidateEvent::create(false, false, 0));
    else {
        RefPtr<RTCIceCandidate> iceCandidate = RTCIceCandidate::create(iceCandidateDescriptor);
        scheduleDispatchEvent(RTCIceCandidateEvent::create(false, false, iceCandidate.release()));
    }
}

void RTCPeerConnection::didChangeSignalingState(SignalingState newState)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    changeSignalingState(newState);
}

void RTCPeerConnection::didChangeIceGatheringState(IceGatheringState newState)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    changeIceGatheringState(newState);
}

void RTCPeerConnection::didChangeIceConnectionState(IceConnectionState newState)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    changeIceConnectionState(newState);
}

void RTCPeerConnection::didAddRemoteStream(PassRefPtr<MediaStreamPrivate> privateStream)
{
    ASSERT(scriptExecutionContext()->isContextThread());

    if (m_signalingState == SignalingStateClosed)
        return;

    RefPtr<MediaStream> stream = MediaStream::create(*scriptExecutionContext(), privateStream);
    m_remoteStreams.append(stream);

    scheduleDispatchEvent(MediaStreamEvent::create(eventNames().addstreamEvent, false, false, stream.release()));
}

void RTCPeerConnection::didRemoveRemoteStream(MediaStreamPrivate* privateStream)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    ASSERT(privateStream->client());

    // FIXME: this class shouldn't know that the private stream client is a MediaStream!
    RefPtr<MediaStream> stream = static_cast<MediaStream*>(privateStream->client());

    if (m_signalingState == SignalingStateClosed)
        return;

    size_t pos = m_remoteStreams.find(stream);
    ASSERT(pos != notFound);
    m_remoteStreams.remove(pos);

    scheduleDispatchEvent(MediaStreamEvent::create(eventNames().removestreamEvent, false, false, stream.release()));
}

void RTCPeerConnection::didAddRemoteDataChannel(std::unique_ptr<RTCDataChannelHandler> handler)
{
    ASSERT(scriptExecutionContext()->isContextThread());

    if (m_signalingState == SignalingStateClosed)
        return;

    RefPtr<RTCDataChannel> channel = RTCDataChannel::create(scriptExecutionContext(), WTF::move(handler));
    m_dataChannels.append(channel);

    scheduleDispatchEvent(RTCDataChannelEvent::create(eventNames().datachannelEvent, false, false, channel.release()));
}

void RTCPeerConnection::stop()
{
    if (m_stopped)
        return;

    m_stopped = true;
    m_iceConnectionState = IceConnectionStateClosed;
    m_signalingState = SignalingStateClosed;

    for (auto& channel : m_dataChannels)
        channel->stop();
}

const char* RTCPeerConnection::activeDOMObjectName() const
{
    return "RTCPeerConnection";
}

bool RTCPeerConnection::canSuspendForPageCache() const
{
    // FIXME: We should try and do better here.
    return false;
}

void RTCPeerConnection::didAddOrRemoveTrack()
{
    negotiationNeeded();
}

void RTCPeerConnection::changeSignalingState(SignalingState signalingState)
{
    if (m_signalingState != SignalingStateClosed && m_signalingState != signalingState) {
        m_signalingState = signalingState;
        scheduleDispatchEvent(Event::create(eventNames().signalingstatechangeEvent, false, false));
    }
}

void RTCPeerConnection::changeIceGatheringState(IceGatheringState iceGatheringState)
{
    m_iceGatheringState = iceGatheringState;
}

void RTCPeerConnection::changeIceConnectionState(IceConnectionState iceConnectionState)
{
    if (m_iceConnectionState != IceConnectionStateClosed && m_iceConnectionState != iceConnectionState) {
        m_iceConnectionState = iceConnectionState;
        scheduleDispatchEvent(Event::create(eventNames().iceconnectionstatechangeEvent, false, false));
    }
}

void RTCPeerConnection::scheduleDispatchEvent(PassRefPtr<Event> event)
{
    m_scheduledEvents.append(event);

    if (!m_scheduledEventTimer.isActive())
        m_scheduledEventTimer.startOneShot(0);
}

void RTCPeerConnection::scheduledEventTimerFired()
{
    if (m_stopped)
        return;

    Vector<RefPtr<Event>> events;
    events.swap(m_scheduledEvents);

    for (auto& event : events)
        dispatchEvent(event.release());

    events.clear();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

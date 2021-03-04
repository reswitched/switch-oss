/*
 * Copyright (c) 2014, 2015 ACCESS Co.,Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

#include "RTCPeerConnectionHandler.h"

#include "CurrentTime.h"
#include "CString.h"
#include "MediaConstraints.h"
#include "RTCConfiguration.h"
#include "RTCDataChannelHandler.h"
#include "RTCDataChannelHandlerClient.h"
#include "RTCDTMFSenderHandler.h"
#include "RTCIceCandidateDescriptor.h"
#include "RTCOfferAnswerOptionsPrivate.h"
#include "RTCPeerConnectionHandlerClient.h"
#include "RTCSessionDescriptionDescriptor.h"
#include "RTCSessionDescriptionRequest.h"
#include "RTCStatsRequest.h"
#include "RTCStatsResponseBase.h"
#include "RTCVoidRequest.h"

#include <wtf/PassRefPtr.h>

#include <wkc/wkcmediapeer.h>

namespace WebCore {

class RTCPeerConnectionHandlerWKC : public RTCPeerConnectionHandler {
public:
    RTCPeerConnectionHandlerWKC(RTCPeerConnectionHandlerClient*);
    virtual ~RTCPeerConnectionHandlerWKC();

    virtual bool initialize(PassRefPtr<RTCConfigurationPrivate>);

    virtual void createOffer(PassRefPtr<RTCSessionDescriptionRequest>, PassRefPtr<RTCOfferOptionsPrivate>);
    virtual void createAnswer(PassRefPtr<RTCSessionDescriptionRequest>, PassRefPtr<RTCOfferAnswerOptionsPrivate>);
    virtual void setLocalDescription(PassRefPtr<RTCVoidRequest>, PassRefPtr<RTCSessionDescriptionDescriptor>);
    virtual void setRemoteDescription(PassRefPtr<RTCVoidRequest>, PassRefPtr<RTCSessionDescriptionDescriptor>);
    virtual PassRefPtr<RTCSessionDescriptionDescriptor> localDescription();
    virtual PassRefPtr<RTCSessionDescriptionDescriptor> remoteDescription();
    virtual bool updateIce(PassRefPtr<RTCConfigurationPrivate>);
    virtual bool addIceCandidate(PassRefPtr<RTCVoidRequest>, PassRefPtr<RTCIceCandidateDescriptor>);
    virtual bool addStream(PassRefPtr<MediaStreamPrivate>);
    virtual void removeStream(PassRefPtr<MediaStreamPrivate>);
    virtual void getStats(PassRefPtr<RTCStatsRequest>);
    virtual std::unique_ptr<RTCDataChannelHandler> createDataChannel(const String& label, const RTCDataChannelInit&);
    virtual std::unique_ptr<RTCDTMFSenderHandler> createDTMFSender(PassRefPtr<RealtimeMediaSource>);
    virtual void stop();

private:
    RTCPeerConnectionHandlerClient* m_client;

    RefPtr<RTCSessionDescriptionDescriptor> m_localDescriptor;
    RefPtr<RTCSessionDescriptionDescriptor> m_remoteDescriptor;
};

class RTCDataChannelHandlerWKC : public RTCDataChannelHandler
{
public:
    RTCDataChannelHandlerWKC(const String& label, const RTCDataChannelInit& init)
        : m_client(0)
        , m_label(label)
    {}
    virtual ~RTCDataChannelHandlerWKC()
    {}

    virtual void setClient(RTCDataChannelHandlerClient* client)
    {
        m_client = client;
    }

    virtual String label()
    {
        return m_label;
    }
    virtual bool ordered()
    {
        return true;
    }
    virtual unsigned short maxRetransmitTime()
    {
        return 0xffff;
    }
    virtual unsigned short maxRetransmits()
    {
        return 0xffff;
    }
    virtual String protocol()
    {
        return String("RTP");
    }
    virtual bool negotiated()
    {
        return true;
    }
    virtual unsigned short id()
    {
        return 0;
    }
    virtual unsigned long bufferedAmount()
    {
        return 65536;
    }

    virtual bool sendStringData(const String&)
    {
        return true;
    }
    virtual bool sendRawData(const char*, size_t)
    {
        return true;
    }
    virtual void close()
    {
    }

private:
    RTCDataChannelHandlerClient* m_client;
    String m_label;
};

RTCPeerConnectionHandlerWKC::RTCPeerConnectionHandlerWKC(RTCPeerConnectionHandlerClient* client)
     : m_client(client)
{
}

RTCPeerConnectionHandlerWKC::~RTCPeerConnectionHandlerWKC()
{
}

bool
RTCPeerConnectionHandlerWKC::initialize(PassRefPtr<RTCConfigurationPrivate>)
{
    int count = wkcMediaPlayerQueryMediaIceCandidatesPeer();
    if (count==WKC_MEDIA_ERROR_NOTSUPPORTED)
        return false;
    for (int i=0; i<count; i++) {
        WKCMediaStreamIceCandidateInfo info = {0};
        wkcMediaPlayerQueryMediaIceCandidatePeer(i, &info);
        String typ;
        if (info.fType==info.EHOST)
            typ = "host";
        else if (info.fType==info.ESRFLX)
            typ = "srflx";
        else if (info.fType==info.ERELAY)
            typ = "relay";
        String proto;
        if (info.fUDP)
            proto = "UDP";
        else
            proto = "TCP";
        String candidate = String::format("candidate:%d %d %s %u %s %d typ %s", info.fFoundation, info.fComponent, proto.latin1().data(), info.fPriority, info.fAddress, info.fPort, typ.latin1().data());
        RefPtr<RTCIceCandidateDescriptor> desc = RTCIceCandidateDescriptor::create(candidate, "", info.fMLineIndex);
        m_client->didGenerateIceCandidate(desc);
    }

    m_client->didChangeIceGatheringState(RTCPeerConnectionHandlerClient::IceGatheringStateComplete);
    m_client->didChangeIceConnectionState(RTCPeerConnectionHandlerClient::IceConnectionStateCompleted);

    return true;
}

void
RTCPeerConnectionHandlerWKC::createOffer(PassRefPtr<RTCSessionDescriptionRequest> req, PassRefPtr<RTCOfferOptionsPrivate>)
{
    req->requestSucceeded(RTCSessionDescriptionDescriptor::create("offer", wkcMediaPlayerQueryMediaLocalSDPPeer()));
}

void
RTCPeerConnectionHandlerWKC::createAnswer(PassRefPtr<RTCSessionDescriptionRequest> req, PassRefPtr<RTCOfferAnswerOptionsPrivate>)
{
    req->requestSucceeded(RTCSessionDescriptionDescriptor::create("answer", wkcMediaPlayerQueryMediaLocalSDPPeer()));
}

void
RTCPeerConnectionHandlerWKC::setLocalDescription(PassRefPtr<RTCVoidRequest> req, PassRefPtr<RTCSessionDescriptionDescriptor> desc)
{
    m_localDescriptor = desc;

    req->requestSucceeded();
}

void
RTCPeerConnectionHandlerWKC::setRemoteDescription(PassRefPtr<RTCVoidRequest> req, PassRefPtr<RTCSessionDescriptionDescriptor> desc)
{
    m_remoteDescriptor = desc;

    req->requestSucceeded();
}

PassRefPtr<RTCSessionDescriptionDescriptor>
RTCPeerConnectionHandlerWKC::localDescription()
{
    return m_localDescriptor;
}

PassRefPtr<RTCSessionDescriptionDescriptor>
RTCPeerConnectionHandlerWKC::remoteDescription()
{
    return m_remoteDescriptor;
}

bool
RTCPeerConnectionHandlerWKC::updateIce(PassRefPtr<RTCConfigurationPrivate>)
{
    m_client->didChangeSignalingState(RTCPeerConnectionHandlerClient::SignalingStateStable);
    m_client->didChangeIceGatheringState(RTCPeerConnectionHandlerClient::IceGatheringStateNew);
    m_client->didChangeIceConnectionState(RTCPeerConnectionHandlerClient::IceConnectionStateNew);
    return true;
}

bool
RTCPeerConnectionHandlerWKC::addIceCandidate(PassRefPtr<RTCVoidRequest> req, PassRefPtr<RTCIceCandidateDescriptor> desc)
{
    if (desc) {
        wkcMediaPlayerAddMediaIceCandidatePeer(desc->candidate().utf8().data(), desc->sdpMid().utf8().data());
    }

    return true;
}

bool
RTCPeerConnectionHandlerWKC::addStream(PassRefPtr<MediaStreamPrivate> desc)
{
    m_client->negotiationNeeded();

    m_client->didAddRemoteStream(desc);

    return true;
}

void
RTCPeerConnectionHandlerWKC::removeStream(PassRefPtr<MediaStreamPrivate> desc)
{
    m_client->negotiationNeeded();

    m_client->didRemoveRemoteStream(desc.get());
}

void
RTCPeerConnectionHandlerWKC::getStats(PassRefPtr<RTCStatsRequest> st)
{
    RefPtr<RTCStatsResponseBase> rsp = st->createResponse();
    size_t i = rsp->addReport("testid", "testtype", WTF::currentTime());
    rsp->addStatistic(i, "testname", "testvalue");
    st->requestSucceeded(rsp);

    m_client->didChangeIceGatheringState(RTCPeerConnectionHandlerClient::IceGatheringStateComplete);
    m_client->didChangeIceConnectionState(RTCPeerConnectionHandlerClient::IceConnectionStateCompleted);
}

std::unique_ptr<RTCDataChannelHandler>
RTCPeerConnectionHandlerWKC::createDataChannel(const String& label, const RTCDataChannelInit& init)
{
    return std::unique_ptr<RTCDataChannelHandler>(new RTCDataChannelHandlerWKC(label, init));
}

std::unique_ptr<RTCDTMFSenderHandler>
RTCPeerConnectionHandlerWKC::createDTMFSender(PassRefPtr<RealtimeMediaSource>)
{
    return nullptr;
}

void
RTCPeerConnectionHandlerWKC::stop()
{
}

static std::unique_ptr<RTCPeerConnectionHandler>
createRTCPeerConnectionHandler(RTCPeerConnectionHandlerClient* client)
{
    return std::unique_ptr<RTCPeerConnectionHandler>(new RTCPeerConnectionHandlerWKC(client));
}

CreatePeerConnectionHandler create = createRTCPeerConnectionHandler;

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

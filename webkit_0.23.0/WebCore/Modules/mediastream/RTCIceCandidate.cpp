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

#include "RTCIceCandidate.h"

#include "Dictionary.h"
#include "ExceptionCode.h"
#include "RTCIceCandidateDescriptor.h"

namespace WebCore {

RefPtr<RTCIceCandidate> RTCIceCandidate::create(const Dictionary& dictionary, ExceptionCode& ec)
{
    String candidate;
    bool ok = dictionary.get("candidate", candidate);
    if (ok && candidate.isEmpty()) {
        ec = TYPE_MISMATCH_ERR;
        return nullptr;
    }

    String sdpMid;
    ok = dictionary.get("sdpMid", sdpMid);
    if (ok && sdpMid.isEmpty()) {
        ec = TYPE_MISMATCH_ERR;
        return nullptr;
    }

    String tempLineIndex;
    unsigned short sdpMLineIndex = 0;
    // First we check if the property exists in the Dictionary.
    ok = dictionary.get("sdpMLineIndex", tempLineIndex);
    // Then we try to convert it to a number and check if it was successful.
    if (ok) {
        bool intConversionOk;
        sdpMLineIndex = tempLineIndex.toUIntStrict(&intConversionOk);
        if (!intConversionOk) {
            ec = TYPE_MISMATCH_ERR;
            return nullptr;
        }
    }

    return adoptRef(*new RTCIceCandidate(RTCIceCandidateDescriptor::create(candidate, sdpMid, sdpMLineIndex)));
}

Ref<RTCIceCandidate> RTCIceCandidate::create(PassRefPtr<RTCIceCandidateDescriptor> descriptor)
{
    return adoptRef(*new RTCIceCandidate(descriptor));
}

RTCIceCandidate::RTCIceCandidate(PassRefPtr<RTCIceCandidateDescriptor> descriptor)
    : m_descriptor(descriptor)
{
}

RTCIceCandidate::~RTCIceCandidate()
{
}

const String& RTCIceCandidate::candidate() const
{
    return m_descriptor->candidate();
}

const String& RTCIceCandidate::sdpMid() const
{
    return m_descriptor->sdpMid();
}

unsigned short RTCIceCandidate::sdpMLineIndex() const
{
    return m_descriptor->sdpMLineIndex();
}

RTCIceCandidateDescriptor* RTCIceCandidate::descriptor()
{
    return m_descriptor.get();
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

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

#include "RTCSessionDescriptionRequestImpl.h"

#include "RTCPeerConnection.h"
#include "RTCPeerConnectionErrorCallback.h"
#include "RTCSessionDescription.h"
#include "RTCSessionDescriptionCallback.h"
#include "RTCSessionDescriptionDescriptor.h"

namespace WebCore {

Ref<RTCSessionDescriptionRequestImpl> RTCSessionDescriptionRequestImpl::create(ScriptExecutionContext* context, PassRefPtr<RTCSessionDescriptionCallback> successCallback, PassRefPtr<RTCPeerConnectionErrorCallback> errorCallback)
{
    Ref<RTCSessionDescriptionRequestImpl> request = adoptRef(*new RTCSessionDescriptionRequestImpl(context, successCallback, errorCallback));
    request->suspendIfNeeded();
    return request;
}

RTCSessionDescriptionRequestImpl::RTCSessionDescriptionRequestImpl(ScriptExecutionContext* context, PassRefPtr<RTCSessionDescriptionCallback> successCallback, PassRefPtr<RTCPeerConnectionErrorCallback> errorCallback)
    : ActiveDOMObject(context)
    , m_successCallback(successCallback)
    , m_errorCallback(errorCallback)
{
}

RTCSessionDescriptionRequestImpl::~RTCSessionDescriptionRequestImpl()
{
}

void RTCSessionDescriptionRequestImpl::requestSucceeded(PassRefPtr<RTCSessionDescriptionDescriptor> descriptor)
{
    if (m_successCallback) {
        RefPtr<RTCSessionDescription> sessionDescription = RTCSessionDescription::create(descriptor);
        m_successCallback->handleEvent(sessionDescription.get());
    }

    clear();
}

void RTCSessionDescriptionRequestImpl::requestFailed(const String& error)
{
    if (m_errorCallback)
        m_errorCallback->handleEvent(DOMError::create(error).ptr());

    clear();
}

void RTCSessionDescriptionRequestImpl::stop()
{
    clear();
}

const char* RTCSessionDescriptionRequestImpl::activeDOMObjectName() const
{
    return "RTCSessionDescriptionRequestImpl";
}

bool RTCSessionDescriptionRequestImpl::canSuspendForPageCache() const
{
    // FIXME: We should try and do better here.
    return false;
}

void RTCSessionDescriptionRequestImpl::clear()
{
    m_successCallback = nullptr;
    m_errorCallback = nullptr;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

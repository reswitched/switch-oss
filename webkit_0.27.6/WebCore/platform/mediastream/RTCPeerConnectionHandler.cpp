/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
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

#include <wtf/NeverDestroyed.h>

namespace WebCore {
class RTCPeerConnectionHandlerClient;

static std::unique_ptr<RTCPeerConnectionHandler> createHandler(RTCPeerConnectionHandlerClient*)
{
    return nullptr;
}

CreatePeerConnectionHandler RTCPeerConnectionHandler::create = createHandler;

const AtomicString& RTCPeerConnectionHandler::incompatibleConstraintsErrorName()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> incompatibleConstraints("IncompatibleConstraintsError", AtomicString::ConstructFromLiteral);
    return incompatibleConstraints;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, incompatibleConstraints, 0);
    if (!incompatibleConstraints)
        incompatibleConstraints = new AtomicString("IncompatibleConstraintsError", AtomicString::ConstructFromLiteral);
    return *incompatibleConstraints;
#endif
}

const AtomicString& RTCPeerConnectionHandler::invalidSessionDescriptionErrorName()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> invalidSessionDescription("InvalidSessionDescriptionError", AtomicString::ConstructFromLiteral);
    return invalidSessionDescription;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, invalidSessionDescription, 0);
    if (!invalidSessionDescription)
        invalidSessionDescription = new AtomicString("InvalidSessionDescriptionError", AtomicString::ConstructFromLiteral);
    return *invalidSessionDescription;
#endif
}

const AtomicString& RTCPeerConnectionHandler::incompatibleSessionDescriptionErrorName()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> incompatibleSessionDescription("IncompatibleSessionDescriptionError", AtomicString::ConstructFromLiteral);
    return incompatibleSessionDescription;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, incompatibleSessionDescription, 0);
    if (!incompatibleSessionDescription)
        incompatibleSessionDescription = new AtomicString("IncompatibleSessionDescriptionError", AtomicString::ConstructFromLiteral);
    return *incompatibleSessionDescription;
#endif
}

const AtomicString& RTCPeerConnectionHandler::internalErrorName()
{
#if !PLATFORM(WKC)
    static NeverDestroyed<AtomicString> internal("InternalError", AtomicString::ConstructFromLiteral);
    return internal;
#else
    WKC_DEFINE_STATIC_PTR(AtomicString*, internal, 0);
    if (!internal)
        internal = new AtomicString("InternalError", AtomicString::ConstructFromLiteral);
    return *internal;
#endif
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2018 Sony Interactive Entertainment Inc.
 * Copyright (c) 2011-2020 ACCESS CO., LTD. All rights reserved.
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
#ifndef AuthenticationChallenge_h
#define AuthenticationChallenge_h

#include "AuthenticationChallengeBase.h"
#include "AuthenticationClient.h"
#include "ResourceHandle.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class ResourceHandle;

class AuthenticationChallenge : public AuthenticationChallengeBase {
public:
    AuthenticationChallenge()
    {
    }

    AuthenticationChallenge(const ProtectionSpace& protectionSpace, const Credential& proposedCredential, unsigned previousFailureCount, const ResourceResponse& response, const ResourceError& error)
        : AuthenticationChallengeBase(protectionSpace, proposedCredential, previousFailureCount, response, error)
    {
    }

    AuthenticationChallenge(ResourceHandle* sourceHandle, unsigned previousFailureCount, const ResourceResponse& response, AuthenticationClient* client = nullptr);

    ResourceHandle* sourceHandle() const { return m_sourceHandle.get(); };

    AuthenticationClient* authenticationClient() const { return m_authenticationClient.get(); }

private:
    friend class AuthenticationChallengeBase;
    static inline bool platformCompare(const AuthenticationChallenge& a, const AuthenticationChallenge& b)
    {
        if (a.sourceHandle() != b.sourceHandle())
            return false;
        return true;
    }

    ProtectionSpaceServerType protectionSpaceServerTypeFromURI(const URL&, bool isForProxy);
    ProtectionSpace protectionSpaceFromHandle(ResourceHandle*, const ResourceResponse&);
    Optional<uint16_t> determinePort(const URL&);
    ProtectionSpaceAuthenticationScheme authenticationSchemeFromHnadle(ResourceHandle*, bool isForProxy);
    String parseRealm(const ResourceResponse&);
    void removeLeadingAndTrailingQuotes(String&);

    RefPtr<ResourceHandle> m_sourceHandle;
    RefPtr<AuthenticationClient> m_authenticationClient;
};

}

#endif

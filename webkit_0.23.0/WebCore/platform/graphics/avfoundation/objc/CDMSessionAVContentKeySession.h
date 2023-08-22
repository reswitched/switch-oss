/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CDMSessionAVContentKeySession_h
#define CDMSessionAVContentKeySession_h

#include "CDMSessionMediaSourceAVFObjC.h"
#include "SourceBufferPrivateAVFObjC.h"
#include <wtf/RetainPtr.h>

#if ENABLE(ENCRYPTED_MEDIA_V2) && ENABLE(MEDIA_SOURCE)

OBJC_CLASS AVContentKeyRequest;
OBJC_CLASS AVContentKeySession;
OBJC_CLASS CDMSessionAVContentKeySessionDelegate;

namespace WebCore {

class CDMPrivateMediaSourceAVFObjC;

class CDMSessionAVContentKeySession : public CDMSessionMediaSourceAVFObjC {
public:
    CDMSessionAVContentKeySession(const Vector<int>& protocolVersions, CDMPrivateMediaSourceAVFObjC&, CDMSessionClient*);
    virtual ~CDMSessionAVContentKeySession();

    static bool isAvailable();

    // CDMSession
    virtual CDMSessionType type() override { return CDMSessionTypeAVContentKeySession; }
    virtual RefPtr<Uint8Array> generateKeyRequest(const String& mimeType, Uint8Array* initData, String& destinationURL, unsigned short& errorCode, unsigned long& systemCode) override;
    virtual void releaseKeys() override;
    virtual bool update(Uint8Array* key, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, unsigned long& systemCode) override;

    // CDMSessionMediaSourceAVFObjC
    void addParser(AVStreamDataParser *) override;
    void removeParser(AVStreamDataParser *) override;

    void didProvideContentKeyRequest(AVContentKeyRequest *);

protected:
    PassRefPtr<Uint8Array> generateKeyReleaseMessage(unsigned short& errorCode, unsigned long& systemCode);

    bool hasContentKeySession() const { return m_contentKeySession; }
    AVContentKeySession* contentKeySession();

    RetainPtr<AVContentKeySession> m_contentKeySession;
    RetainPtr<CDMSessionAVContentKeySessionDelegate> m_contentKeySessionDelegate;
    RetainPtr<AVContentKeyRequest> m_keyRequest;
    RefPtr<Uint8Array> m_initData;
    RetainPtr<NSData> m_expiredSession;
    Vector<int> m_protocolVersions;
    int32_t m_protectedTrackID { 1 };
    enum { Normal, KeyRelease } m_mode;
};

inline CDMSessionAVContentKeySession* toCDMSessionAVContentKeySession(CDMSession* session)
{
    if (!session || session->type() != CDMSessionTypeAVContentKeySession)
        return nullptr;
    return static_cast<CDMSessionAVContentKeySession*>(session);
}

}

#endif

#endif // CDMSessionAVContentKeySession_h

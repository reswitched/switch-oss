/*
 * Copyright (c) 2010,2011 ACCESS CO., LTD. All rights reserved.
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

#ifndef CertificateWKC_h
#define CertificateWKC_h

#include "WTFString.h"
#include "CString.h"

#include <openssl/ossl_typ.h>

namespace WebCore {

class ClientCertificate {
public:
    ClientCertificate();
    ~ClientCertificate();

    static ClientCertificate* create(const unsigned char* pkcs12, int pkcs12_len, const unsigned char* pass, int pass_len);
    static ClientCertificate* createByDER(const unsigned char* cert, int cert_len, const unsigned char* key, int key_len);

    const String& issuer() const { return m_Issuer; }
    const String& subject() const { return m_Subject; }
    const String& notbefore() const { return m_NotBefore; }
    const String& notafter() const { return m_NotAfter; }
    const String& serialnumber() const { return m_serialNumber; }

    bool sameIssuer(const char*s) { return (m_Issuer == s); }

    void* pkcs12() { return m_pkcs12; }
    int   pkcs12len() { return m_pkcs12Len; }
    void* pass() { return m_pass; }
    int   passlen() { return m_passLen; }
    X509* cert() { return m_cert; }
    void* privateKey() { return m_privateKey; }
    int   privateKeylen() { return m_privateKeyLen; }

    void setCAIndexs(void** caindexs, int num) { m_caIndexs = caindexs; m_caIndexNum = num; }

private:
    void init(X509* x509);

    String m_Issuer;
    String m_Subject;
    String m_NotBefore;
    String m_NotAfter;
    String m_serialNumber;
    void* m_pkcs12;
    int   m_pkcs12Len;
    void* m_pass;
    int   m_passLen;
    X509* m_cert;
    void* m_privateKey;
    int m_privateKeyLen;

    void** m_caIndexs;
    int    m_caIndexNum;
};

}
#endif // CertificateWKC_h

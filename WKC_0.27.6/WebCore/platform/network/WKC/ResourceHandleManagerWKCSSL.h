/*
 * Copyright (c) 2010-2019 ACCESS CO., LTD. All rights reserved.
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

#ifndef ResourceHandleManagerWKCSSL_h
#define ResourceHandleManagerWKCSSL_h

#include "CString.h"
#include "WTFString.h"

#if PLATFORM(WIN)
#include <winsock2.h>
#include <windows.h>
#endif

#include <curl/curl.h>
#include <openssl/x509v3.h>

namespace WebCore {

class ClientCertificate;

class Certificate
{
WTF_MAKE_FAST_ALLOCATED;
public:
    Certificate()
        : m_CertNum(0)
    {
        m_Time = time(NULL);
    }
    ~Certificate()
    {
        m_Certs.clear();
    }
    void push(const char* cert, int len)
    {
        m_Certs.append(String(cert, len));
        m_CertNum++;
    }
    int num(void) { return m_CertNum; }
    String get(int idx)
    {
        if (m_CertNum <= idx)
            return String();
        return m_Certs[idx];
    }
    time_t createTime(void) { return m_Time; }

private:
    Vector<String> m_Certs;
    int            m_CertNum;
    time_t         m_Time;
};

class BlackCert {
WTF_MAKE_FAST_ALLOCATED;
public:
    BlackCert(const char* issuerCN, unsigned char *in_serial, int in_serial_len)
        : issuerCommonName(issuerCN)
        , serial(in_serial)
        , serial_len(in_serial_len)
    {}
    String issuerCommonName;
    unsigned char *serial;
    int serial_len;
};

class RootCA_OID {
WTF_MAKE_FAST_ALLOCATED;
public:
    RootCA_OID(const char* in_issuerCN, const char* in_oid, unsigned char *in_fingerprint, unsigned char *in_serial, int in_serial_len)
        : issuerCommonName(in_issuerCN)
        , OID(in_oid)
        , serial(in_serial)
        , serial_len(in_serial_len)
    {
        memcpy(fingerprint, in_fingerprint, 20);
    }
    String issuerCommonName;
    String OID;
    unsigned char fingerprint[20];
    unsigned char *serial;
    int serial_len;
};

class ResourceHandleManagerSSL {
WTF_MAKE_FAST_ALLOCATED;
public:
    static ResourceHandleManagerSSL* create(ResourceHandleManager* rhm, CURLM* curlMultiHandle, CURLSH* curlShareHandle);
    ~ResourceHandleManagerSSL();
    static void resetVariables();

    void initializeHandleSSL(ResourceHandle*);
    void removeRunningJobSSL(ResourceHandle*);

    bool allowsServerHost(const String& host);
    void setAllowServerHost(const char *host_w_port);

    void SSLHandshakeInfo(ResourceHandle* job);

    void ClientCertSelectCallback(void* ssl, X509 **x509, EVP_PKEY **pkey);

    CURLSH* getCurlShareHandle(void) { return m_curlShareHandle; }

    // ROOT CA
    void* SSLRegisterRootCA(const char* cert, int cert_len);
    void* SSLRegisterRootCAByDER(const char* cert, int cert_len);
    int   SSLUnregisterRootCA(void* certid);
    void  SSLRootCADeleteAll(void);
    bool  exportRootCACacheIntoStore(X509_STORE* store);

    // CRL
    void* SSLRegisterCRL(const char* crl, int crl_len);
    int   SSLUnregisterCRL(void* crlid);
    void  SSLCRLDeleteAll(void);

    // Client Certificate
    void* SSLRegisterClientCert(const unsigned char* pkcs12, int pkcs12_len, const unsigned char* pass, int pass_len);
    void* SSLRegisterClientCertByDER(const unsigned char* cert, int cert_len, const unsigned char* key, int key_len);
    int   SSLUnregisterClientCert(void* certid);
    void  SSLClientCertDeleteAll(void);

    // Certificate Black List
    bool SSLRegisterBlackCert(const char* in_issuerCommonName, const char* serialNumber);
    bool SSLRegisterBlackCertByDER(const char* cert, int cert_len);
    void SSLBlackCertDeleteAll(void);
    bool isCertificateBlack(void* data);

    // EV SSL OID
    bool SSLRegisterEVSSLOID(const char *issuerCommonName, const char *OID, const char *sha1FingerPrint, const char *SerialNumber);
    void SSLEVSSLOIDDeleteAll(void);
    bool isWellKnownTrustedEVCert(const char *server_name, ASN1_INTEGER *serial, const unsigned char *sha1, const char *server_oid);

    // Enable SSL Protocols
    void SSLEnableProtocols(unsigned int versions);

    // Server Certificate Chain
    const char** getServerCertChain(const char* url, int& outCertNum);
    void freeServerCertChain(const char** chain, int num);

    // OpenSSL costomised callback
    void saveCRLFile(const char *content_type, void *buff, int len);
    int HTTPGetResponse(X509 *cert, int req_type, const char *uri, STACK_OF(OPENSSL_STRING) *headers, BIO *send_bio, unsigned char **out, int *outlen);

    // Enable Online Certificate Check
    void SSLEnableOnlineCertChecks(bool enableOCSP, bool enableCRLDP);

    void lock();
    void unlock();

private:
    ResourceHandleManagerSSL(ResourceHandleManager* rhm, CURLM* curlMultiHandle, CURLSH* curlShareHandle);
    bool construct();

    // For Singleton
    CURLM* m_curlMultiHandle;
    CURLSH* m_curlShareHandle;

private:
    ResourceHandleManager* m_rhm;

    // Allows Server Host
    HashSet<String> m_AllowServerHost;

    // Root CA Cache
    Vector<X509*> m_rootCACache;
    bool buildRootCACache();
    void clearRootCACache();

    // Server Certificate Chain
    static const int cMaxChains = 32;
    HashMap<String, Certificate*> m_serverCertChain;
    void addServerCertChain(const char* url, STACK_OF(X509) *sk);

    // Client Certificates
    HashSet<ClientCertificate*> m_clientCerts;
    HashMap<String, ClientCertificate*> m_clientCertCache;
    void setClientCertCache(const char*, ClientCertificate*);
    ClientCertificate* getClientCertCache(const char*);
    void deleteClientCertCache(ClientCertificate*);

    // Certificate Black List
    HashSet<BlackCert*> m_certBlackList;

    // Trusted ROOT Certificate OID
    HashSet<RootCA_OID*> m_rootOIDList;

    // Enable SSL versions
    unsigned int m_enableVersion;

    // curl handle for OCSP/CRL-DP
    bool m_enableOCSP;
    bool m_enableCRLDP;
    CURL *m_ocspHandle;

    void *m_mutex;
};

}

#endif  // ResourceHandleManagerWKCSSL_h

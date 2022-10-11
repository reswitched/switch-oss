/*
 * Copyright (c) 2010-2021 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"

#include "CString.h"
#include "ResourceHandle.h"
#include "ResourceHandleInternalWKC.h"
#include "ResourceHandleManagerWKC.h"
#include "ResourceHandleManagerWKCSSL.h"
#include "CertificateWKC.h"
#include "FrameLoaderClientWKC.h"
#include "LoggingWKC.h"

#include "WKCEnums.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#if !PLATFORM(WIN_OS)
#include <sys/param.h>
#undef MAX_PATH
#define MAX_PATH MAXPATHLEN
#endif

#include <curl/curl.h>
#include <curl/multi.h>

#include <crypto/x509.h>
#include <openssl/bn.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/pkcs12.h>

#include <stdarg.h>
#include <peer_openssl.h>

#ifndef TRUE
# define TRUE (1)
#endif
#ifndef FALSE
# define FALSE (0)
#endif

#if defined(SSL_OP_NO_TLSv1_2) && !defined(OPENSSL_NO_TLS1_2)
#define OPENSSL_SUPPORT_TLSv1_2
#endif
#if defined(SSL_OP_NO_TLSv1_3) && !defined(OPENSSL_NO_TLS1_3)
#define OPENSSL_SUPPORT_TLSv1_3
#endif

#define APPLE_IST_CN "Apple IST" // If an intermediate certificate has CN = "Apple IST", add an exception for this certificate.
#define GEOTRUST_CN  "GeoTrust"

// If a server cert has EV-OID and root ca is for EV SSL, 
// this site is EV-SSL site without both OCSP/CRL-DP online checks.
#undef ACKNOWLEDGE_EVSSL_EVEN_WITHOUT_OCSPCRLDP

namespace WebCore {

#define RHMSSL_AES_LEN  16
static unsigned char gMagic[RHMSSL_AES_LEN];
static unsigned char gIV[RHMSSL_AES_LEN];

#define RHMSSL_BUFF_LEN  1024
static unsigned char gBuff[RHMSSL_BUFF_LEN];

typedef struct data_buff_ {
    CURL *handle;
    unsigned char* buff;
    int len;
    char content_type[256];
} data_buff;

////////////////////////////////////////////
// Utilities
static char *fastStrdup(const char *str)
{
    char *new_str;
    size_t size;

    if (!str) {
        return NULL;
    }
    size = strlen(str) + 1;

    new_str = (char *)fastMalloc(size);
    if (!new_str) {
        return NULL;
    }
    memcpy(new_str, str, size);

    return new_str;
}

extern WKC::FrameLoaderClientWKC* frameloaderclientwkc(ResourceHandle* job);

// static tool functions
static String SSLhostAndPort(const URL& kurl)
{
    String url;

    if (kurl.port())
        url = kurl.host().convertToASCIILowercase() + ":" + String::number(kurl.port().value());
    else
        url = kurl.host().toString() + ":443";

    return url;
}

static int asn1_output(const ASN1_UTCTIME* tm, char* buf, size_t sizeofbuf)
{
    memset(buf, 0x0, sizeofbuf);

    BIO* mem = BIO_new(BIO_s_mem());

    int ret = ASN1_UTCTIME_print(mem, tm);

    BIO_read(mem, (void*)buf, sizeofbuf-1);
    BIO_free_all(mem);

    return ret;
}

static bool parse_pkcs12(const unsigned char* pkcs12, int pkcs12_len, const char* pass, EVP_PKEY **pri, X509 **x509, STACK_OF(X509) **ca)
{
    BIO* bio = NULL;
    PKCS12* p12 = NULL;
    bool ret = false;

    if (!pkcs12 || 0 == pkcs12_len)
        goto parse_error;

    bio = BIO_new_mem_buf((void*)pkcs12, pkcs12_len);
    if (!bio)
        goto parse_error;

    p12 = d2i_PKCS12_bio(bio, NULL);
    if (!p12)
        goto parse_error;

    PKCS12_PBE_add();

    if (!PKCS12_parse(p12, (const char *)pass, pri, x509, ca))
        goto parse_error;

    ret = true;

parse_error:
    BIO_free(bio);
    PKCS12_free(p12);
    return ret;
}

static unsigned char* encrypt_key(const unsigned char* in_key, int in_keylen, bool isEncrypt, int* out_len)
{
    int resultlen, tmplen;
    unsigned char* newkey = 0;
    EVP_CIPHER_CTX* ctx = 0;

    newkey = (unsigned char*)fastMalloc(in_keylen + RHMSSL_AES_LEN);
    if (!newkey)
        goto error_end;
    memset(newkey, 0x00, (in_keylen + RHMSSL_AES_LEN));

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        goto error_end;
    EVP_CIPHER_CTX_reset(ctx);

    if (isEncrypt) {
        EVP_EncryptInit(ctx, EVP_aes_128_ecb(), gMagic, gIV);
        EVP_EncryptUpdate(ctx, newkey, &resultlen, in_key, in_keylen);
        EVP_EncryptFinal(ctx, newkey + resultlen, &tmplen);
    }
    else {
        EVP_DecryptInit(ctx, EVP_aes_128_ecb(), gMagic, gIV);
        EVP_DecryptUpdate(ctx, newkey, &resultlen, in_key, in_keylen);
        EVP_DecryptFinal(ctx, newkey + resultlen, &tmplen);
    }

    newkey[resultlen + tmplen] = 0x0;
    EVP_CIPHER_CTX_free(ctx);

    if (out_len)
        *out_len = resultlen + tmplen;

    return newkey;

error_end:
    if (ctx)
        EVP_CIPHER_CTX_free(ctx);
    if (newkey)
        fastFree(newkey);
    return (unsigned char*)0;
}

////////////////////////////////////////////
//
//  OCSP / CRL-DP
//
//   Following tree funcsions 'writeCallback, headerCallback and HandleCreate'
//   are only used for OCSP / CRL-DP communications.
//
static size_t
writeCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
    int len = (int)(size * nmemb);
    data_buff* outbuf = (data_buff*)data;
    if (!outbuf)
        return 0;
    if (!ptr || len < 1)
        return 0;

    if (!outbuf->buff || 0 == outbuf->len) {
        outbuf->buff = (unsigned char*)fastMalloc(len);
        memcpy(outbuf->buff, ptr, len);
        outbuf->len = len;
    }
    else {
        outbuf->buff = (unsigned char*)fastRealloc(outbuf->buff, outbuf->len + len);
        memcpy(outbuf->buff + outbuf->len, ptr, len);
        outbuf->len += len;
    }
    return len;
}

static size_t
headerCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
    int len = (int)(size * nmemb);
    const char* line = (const char*)ptr;
    data_buff* outbuf = (data_buff*)data;

    if (!line || len < 1 || !data)
        return 0;

    if (!strncmp(line, "Content-Type: ", 14)) {
        strncpy(outbuf->content_type, line + 14, 255);
    }

    if (!strncmp(line, "\r\n", 2) || !strncmp(line, "\n", 1)) {
        long httpCode = 0;
        (void)curl_easy_getinfo(outbuf->handle, CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode == 407) {
            return len;
        } else if (httpCode != 200) {
            return 0;
        }
    }
    return len;
}

static CURL *
HandleCreate()
{
    CURL *handle;

    handle = curl_easy_init();
    if (!handle) {
        return 0;
    }

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, headerCallback);

    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(handle, CURLOPT_MAXREDIRS, 10);
    curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    curl_easy_setopt(handle, CURLOPT_DNS_CACHE_TIMEOUT, 60 * 5); /* 5 minutes */

    /* Set connect timeout. */
    curl_easy_setopt(handle, CURLOPT_SERVER_RESPONSE_TIMEOUT, 20);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 5);

    /* enable gzip and deflate through Accept-Encoding: */
    curl_easy_setopt(handle, CURLOPT_ENCODING, "");

    /* only one connection for each handle */
    curl_easy_setopt(handle, CURLOPT_MAXCONNECTS, 1);

    return handle;
}


////////////////////////////////////////////
//
//  Constructor and Destructor (private)
//
ResourceHandleManagerSSL::ResourceHandleManagerSSL(ResourceHandleManager* rhm, CURLM* curlMultiHandle, CURLSH* curlShareHandle)
    : m_curlMultiHandle(curlMultiHandle)
    , m_curlShareHandle(curlShareHandle)
    , m_rhm(rhm)
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    RAND_bytes(gMagic, RHMSSL_AES_LEN);
    RAND_bytes(gIV, RHMSSL_AES_LEN);

    memset(gBuff, 0x00, RHMSSL_BUFF_LEN);

    m_enableVersion = CURL_SSLVERSION_DEFAULT;
#ifdef OPENSSL_SUPPORT_TLSv1_3
    m_enableVersion |= CURL_SSLVERSION_TLSv1_3;
#endif
#ifdef OPENSSL_SUPPORT_TLSv1_2
    m_enableVersion |= CURL_SSLVERSION_TLSv1_2;
#endif
    m_enableOCSP = m_enableCRLDP = true;
    m_ocspHandle = HandleCreate();

    m_mutex = wkcMutexNewPeer();

    LOG_FUNCTION(ResourceHandleManager, "out");
}

ResourceHandleManagerSSL::~ResourceHandleManagerSSL()
{
    LOG_FUNCTION(ResourceHandleManager, "in");

    wkcMutexDeletePeer(m_mutex);

    if (m_ocspHandle)
        curl_easy_cleanup(m_ocspHandle);

    clearRootCACache();

    for (auto& elem : m_serverCertChain) {
        delete elem.value;
    }
    m_serverCertChain.clear();
    m_clientCertCache.clear();
    SSLClientCertDeleteAll();
    SSLBlackCertDeleteAll();
    SSLEVSSLOIDDeleteAll();

    memset(gBuff, 0x00, RHMSSL_BUFF_LEN);
    memset(gMagic, 0x00, RHMSSL_AES_LEN);
    memset(gIV, 0x00, RHMSSL_AES_LEN);

    m_rhm = 0;
    m_curlMultiHandle = 0;
    m_curlShareHandle = 0;

    LOG_FUNCTION(ResourceHandleManager, "out");
}

ResourceHandleManagerSSL* ResourceHandleManagerSSL::create(ResourceHandleManager* rhm, CURLM* curlMultiHandle, CURLSH* curlShareHandle)
{
    ResourceHandleManagerSSL* self;

    self = new ResourceHandleManagerSSL(rhm, curlMultiHandle, curlShareHandle);
    if (!self)
        goto exit_func;

    if (!self->construct()) {
        delete self;
        self = 0;
        goto exit_func;
    }

exit_func:
    return self;
}

bool ResourceHandleManagerSSL::construct()
{
    return true;
}

// Reset Variables & Force Terminate
void ResourceHandleManagerSSL::resetVariables()
{
    memset(gBuff, 0x00, RHMSSL_BUFF_LEN);
    memset(gMagic, 0x00, RHMSSL_AES_LEN);
    memset(gIV, 0x00, RHMSSL_AES_LEN);
}


////////////////////////////////////////////
//
// callback functions
//

static int
ssl_verify_callback(int ok, X509_STORE_CTX *ctx)
{
    LOG_FUNCTION(ResourceHandleManager, "OK:%d ctx:%p", ok, ctx);

    ctx->untrusted_cert_flag |= ResourceHandleManager::sharedInstance()->rhmssl()->verifyTrustedCertificate(ctx->current_cert);

    if (ctx->untrusted_cert_flag & EUntrustedCertDetected) {
        ctx->error = X509_V_ERR_CERT_UNTRUSTED;
        LOG_FUNCTION(ResourceHandleManager, "out1: return:0");
        return 0;
    }

    if (ctx->untrusted_cert_flag & EGeoTrustCertDetected) {
        if (ctx->untrusted_error_depth == 0) {
            ctx->untrusted_error_depth = ctx->error_depth;
        }

        if (ResourceHandleManager::sharedInstance()->rhmssl()->isCertificateException(ctx->current_cert)) {
            ctx->untrusted_cert_flag |= EExceptionalCertDetected;
        }
    }

    if (ctx->error_depth == 0) {
        if (ctx->untrusted_cert_flag & EGeoTrustCertDetected) {
            if (!(ctx->untrusted_cert_flag & EExceptionalCertDetected)) {
                ctx->error_depth = ctx->untrusted_error_depth;
                ctx->error = X509_V_ERR_CERT_UNTRUSTED;
                LOG_FUNCTION(ResourceHandleManager, "out2: return:0");
                return 0;
            }
            else if (ctx->untrusted_cert_flag & EBlackListCertDetected) {
                ctx->error_depth = ctx->blacklist_error_depth;
                ctx->error = X509_V_ERR_CERT_REVOKED;
                LOG_FUNCTION(ResourceHandleManager, "out3: return:0");
                return 0;
            }
        }
    }

    if (ResourceHandleManager::sharedInstance()->rhmssl()->isCertificateBlack(ctx->current_cert)) {
        ctx->blacklist_error_depth = ctx->error_depth;
        ctx->untrusted_cert_flag |= EBlackListCertDetected;
    }

    if (!(ctx->untrusted_cert_flag & EGeoTrustCertDetected)) {
        if (ctx->untrusted_cert_flag & EBlackListCertDetected) {
            ctx->error_depth = ctx->blacklist_error_depth;
            ctx->error = X509_V_ERR_CERT_REVOKED;
            LOG_FUNCTION(ResourceHandleManager, "out4: return:0");
            return 0;
        }
    }

    if (ok == 0) {
        LOG_FUNCTION(ResourceHandleManager, "Error string: %s", X509_verify_cert_error_string(ctx->error));

        switch (ctx->error) {
        case X509_V_ERR_UNABLE_TO_GET_CRL:
            /* NEVER delete this case block */
            ok = 1;
            ctx->error = X509_V_OK;
            break;
        default:
            break;
        }
    }

    LOG_FUNCTION(ResourceHandleManager, "out5: return:%d", ok);
    return ok;
}

#if 0
static int
ssl_app_verify_callback(X509_STORE_CTX *ctx, void *data)
{
    LOG_FUNCTION(ResourceHandleManager, "ctx:%p data:%p", ctx, data);

    ResourceHandle* job = static_cast<ResourceHandle*>(data);
    ResourceHandleInternal* d = job->getInternal();

    int ok = X509_verify_cert(ctx);  // it'll call ssl_verify_callback()
    LOG_FUNCTION(ResourceHandleManager, "X509_verify_cert() is %s", (ok)?"OK":"NG");

    d->m_response.setResourceHandle(job);

    LOG_FUNCTION(ResourceHandleManager, "return:%d", ok);
    return ok;
}
#endif

static int
ssl_cert_request_callback(SSL *ssl, X509 **x509, EVP_PKEY **pkey)
{
    LOG_FUNCTION(ResourceHandleManager, "ssl:%p ", ssl);
    ResourceHandleManager::sharedInstance()->rhmssl()->ClientCertSelectCallback((void*)ssl, x509, pkey);
    if (x509 && pkey) {
        LOG_FUNCTION(ResourceHandleManager, "ssl:%p x509:%p pkey:%p", ssl, x509, pkey);
        return 1;
    }
    LOG_FUNCTION(ResourceHandleManager, "out");
    return 0;
}

static CURLcode
sslctx_callback(CURL* handle, void* sslctx, void* data)
{
    LOG_FUNCTION(ResourceHandleManager, "handle:%p sslctx:%p data:%p", handle, sslctx, data);

    SSL_CTX* sslCtx = (SSL_CTX *)sslctx;
    unsigned long online_check_flag = 0;

    ResourceHandle* job = static_cast<ResourceHandle*>(data);
    ResourceHandleInternal* d = job->getInternal();
    if (!d || d->m_cancelled) {
        LOG_FUNCTION(ResourceHandleManager, "out1");
        return CURLE_OK;
    }
    URL kurl = job->firstRequest().url();

    SSL_CTX_set_client_cert_cb(sslCtx, ssl_cert_request_callback);

    SSL_CTX_set_app_data(sslCtx, job);
    SSL_CTX_set_verify(sslCtx, SSL_CTX_get_verify_mode(sslCtx), ssl_verify_callback);
    //SSL_CTX_set_cert_verify_callback(sslCtx, ssl_app_verify_callback, data);

    SSL_CTX_set_verify_depth(sslCtx, 8);

    if (d->m_enableOCSP)  online_check_flag |= X509_V_FLAG_CHECK_OCSP;
    if (d->m_enableCRLDP) online_check_flag |= X509_V_FLAG_CHECK_CRLDP;
    X509_VERIFY_PARAM_set_flags(SSL_CTX_get0_param(sslCtx), online_check_flag);

    X509_STORE* store = SSL_CTX_get_cert_store(sslCtx);
    if (!ResourceHandleManager::sharedInstance()->rhmssl()->exportRootCACacheIntoStore(store))
        return CURLE_OUT_OF_MEMORY;

    LOG_FUNCTION(ResourceHandleManager, "out2");
    return CURLE_OK;
}

////////////////////////////////////////////
//
// Public functions
//
void ResourceHandleManagerSSL::initializeHandleSSL(ResourceHandle* job)
{
    // FIXME: Enable SSL verification when we have a way of shipping certs
    // and/or reporting SSL errors to the user.
    ResourceHandleInternal* d = job->getInternal();
    URL kurl = job->firstRequest().url();

    curl_easy_setopt(d->m_handle, CURLOPT_CAINFO, NULL);
    if (wkcOsslCRLIsRegistPeer())
        curl_easy_setopt(d->m_handle, CURLOPT_CRLFILE, WKCOSSL_CRL_FILE);

    curl_easy_setopt(d->m_handle, CURLOPT_RANDOM_FILE, WKCOSSL_RANDFILE);

    curl_easy_setopt(d->m_handle, CURLOPT_SSL_CTX_FUNCTION, sslctx_callback);
    curl_easy_setopt(d->m_handle, CURLOPT_SSL_CTX_DATA, job);

    curl_easy_setopt(d->m_handle, CURLOPT_SSLVERSION, m_enableVersion);

    // CURLOPT_CERTINFO sets 0 but it can get the pointer of STACK_OF(X509)
    // using CURLINFO_SSL_CERTCHAIN by ACCESS Special customize
    curl_easy_setopt(d->m_handle, CURLOPT_CERTINFO, 0);

    if (allowsServerHost(SSLhostAndPort(kurl))) {
        curl_easy_setopt(d->m_handle, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(d->m_handle, CURLOPT_SSL_VERIFYPEER, 0);
    }
    else {
        curl_easy_setopt(d->m_handle, CURLOPT_SSL_VERIFYHOST, 2);
        curl_easy_setopt(d->m_handle, CURLOPT_SSL_VERIFYPEER, 1);
    }

    curl_easy_setopt(d->m_handle, CURLOPT_SSL_CIPHER_LIST,
        "ECDHE+ECDSA+AESGCM:ECDHE+ECDSA+AES:ECDHE+ECDSA+CHACHA20:ECDHE+AESGCM:ECDHE+AES:ECDHE+CHACHA20:RSA+AESGCM:RSA+AES+SHA:!AESCCM");
    curl_easy_setopt(d->m_handle, CURLOPT_TLS13_CIPHERS,
        "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256");
//    curl_easy_setopt(d->m_handle, CURLOPT_SSL_VERIFYSTATUS, (long)1);

    d->m_SSLVerifyPeerResult = 0;
    d->m_SSLVerifyHostResult = 0;
    d->m_response.setURL(URL({}, ""));

    d->m_enableOCSP  = m_enableOCSP;
    d->m_enableCRLDP = m_enableCRLDP;
}

void ResourceHandleManagerSSL::removeRunningJobSSL(ResourceHandle* job)
{
    ResourceHandleInternal* d = job->getInternal();

    // block callback
    curl_easy_setopt(d->m_handle, CURLOPT_SSL_CTX_FUNCTION, NULL);
    curl_easy_setopt(d->m_handle, CURLOPT_SSL_CTX_DATA, 0);
}

void ResourceHandleManagerSSL::setAllowServerHost(const char *host_w_port)
{
    if (!host_w_port)
        return;

    LOG_FUNCTION(ResourceHandleManager, "host_w_port:%s", host_w_port);

    String host = host_w_port;

    if (!strchr(host_w_port, ':'))
        host.append(":443");

    m_AllowServerHost.add(host.convertToASCIILowercase());

    LOG_FUNCTION(ResourceHandleManager, "out");
}

void ResourceHandleManagerSSL::SSLHandshakeInfo(ResourceHandle* job)
{
    CURLcode err;
    long value;
    ResourceHandleInternal* d = NULL;
    char* str;

    d = job->getInternal();
    if (!d) {
        return;
    }

#if 0
    // cURL Original CURLINFO_CERTINFO
    if (!d->m_certChain) {
        struct curl_certinfo *ci = NULL;
        err = curl_easy_getinfo(d->m_handle, CURLINFO_CERTINFO, &ci);
        if(err == CURLE_OK && ci) {
            d->m_certChainNum = ci->num_of_certs;
            d->m_certChain = (char **)wkc_calloc(d->m_certChainNum, sizeof(char*));
            for (int i = 0; i < ci->num_of_certs; i++) {
                struct curl_slist *slist;
                for (slist = ci->certinfo[i]; slist; slist = slist->next) {
                    if (!strncmp(slist->data, "Cert:", 5)) {
                        d->m_certChain[i] = wkc_strdup(slist->data+5);
                    }
                }
            }
        }
    }
#else
    // ACCESS Customized CURLINFO_CERTCHAIN
    {
        char *ptr = NULL;
        err = curl_easy_getinfo(d->m_handle, CURLINFO_SSL_CERTCHAIN, &ptr);
        if(err == CURLE_OK && ptr) {
            STACK_OF(X509) *sk = (STACK_OF(X509) *)ptr;
            const char* url;
            (void)curl_easy_getinfo(d->m_handle, CURLINFO_EFFECTIVE_URL, &url);
            addServerCertChain(url, sk);
        }
    }
#endif

    if (0 == d->m_SSLVerifyPeerResult) {
        value = -1;
        err = curl_easy_getinfo(d->m_handle, CURLINFO_SSL_VERIFYPEER_RESULT, &value);
        if(err == CURLE_OK && value != -1) {
            d->m_SSLVerifyPeerResult = value;
        }
    }

    if (0 == d->m_SSLVerifyHostResult) {
        value = -1;
        err = curl_easy_getinfo(d->m_handle, CURLINFO_SSL_VERIFYHOST_RESULT, &value);
        if(err == CURLE_OK && value != -1) {
            d->m_SSLVerifyHostResult = value;
        }
    }

    str = 0;
    err = curl_easy_getinfo(d->m_handle, CURLINFO_SSL_CIPHER_VERSION, &str);
    if (err == CURLE_OK && str) {
        d->m_sslCipherVersion = fastStrdup(str);
    }

    str = 0;
    err = curl_easy_getinfo(d->m_handle, CURLINFO_SSL_CIPHER_NAME, &str);
    if (err == CURLE_OK && str) {
        d->m_sslCipherName = fastStrdup(str);
    }

    value = 0;
    err = curl_easy_getinfo(d->m_handle, CURLINFO_SSL_CIPHER_BITS, &value);
    if (err == CURLE_OK) {
        d->m_sslCipherBits = (int)value;
    }

    value = 0;
    err = curl_easy_getinfo(d->m_handle, CURLINFO_SSL_IS_EVSSL, &value);
    if (err == CURLE_OK && 1 == value) {
        d->m_isEVSSL = true;
    }

    value = 0;
    err = curl_easy_getinfo(d->m_handle, CURLINFO_SSL_REVOKE_STATE, &value);
    if (err == CURLE_OK) {
        if (d->m_isEVSSL) {
            switch (value) {
            case CURLSSL_CERTSTATUS_SUCCESSFUL:
                d->m_secureState = WKC::ESecureStateGreen;
                d->m_secureLevel = WKC::ESecureLevelSecure;
                break;
            case CURLSSL_CERTSTATUS_REVOKED:
                d->m_secureState = WKC::ESecureStateRed;
                d->m_secureLevel = WKC::ESecureLevelInsecure;
                break;
            case CURLSSL_CERTSTATUS_NOTSAFETY:
                d->m_secureState = WKC::ESecureStateBlue;
                d->m_secureLevel = WKC::ESecureLevelSecure;
#ifdef ACKNOWLEDGE_EVSSL_EVEN_WITHOUT_OCSPCRLDP
                if (!d->m_enableOCSP && !m_enableCRLDP) {
                    d->m_secureState = WKC::ESecureStateGreen;
                }
#endif
                break;
            }
        }
        else{
            switch (value) {
            case CURLSSL_CERTSTATUS_REVOKED:
                d->m_secureState = WKC::ESecureStateRed;
                d->m_secureLevel = WKC::ESecureLevelInsecure;
                break;
            case CURLSSL_CERTSTATUS_NOTSAFETY:
            case CURLSSL_CERTSTATUS_SUCCESSFUL:
                d->m_secureState = WKC::ESecureStateBlue;
                d->m_secureLevel = WKC::ESecureLevelSecure;
                break;
            }
        }
    }
}

//
// callback functions
//

static void setClientCertAndKey(SSL* ssl, ClientCertificate* cert, X509 **x509, EVP_PKEY **pkey)
{
    X509* cert_dup = NULL;
    EVP_PKEY* pri = NULL;
    int orgkey_len;

    unsigned char* orgkey = encrypt_key(reinterpret_cast<const unsigned char*>(cert->privateKey()), cert->privateKeylen(), false, &orgkey_len);
    if (!orgkey) {
        return;
    }
    unsigned char* next = orgkey;
    pri = d2i_AutoPrivateKey(NULL, const_cast<const unsigned char**>(&next), orgkey_len);
    if (!pri) {
        goto exit_func;
    }

    cert_dup = X509_dup(cert->cert());
    if (!cert_dup)
        goto exit_func;

    SSL_set_mode(ssl, SSL_MODE_NO_AUTO_CHAIN);
    if (SSL_use_certificate(ssl, cert_dup) != 1) {
        goto exit_func;
    }
    if (SSL_use_PrivateKey(ssl, pri) != 1) {
        goto exit_func;
    }
    SSL_check_private_key(ssl);

    *x509 = cert_dup;
    *pkey = pri;

    return;

exit_func:
    if (orgkey) {
        memset(orgkey, 0, orgkey_len); // delete private key completely
        fastFree(orgkey);
    }
    if (cert_dup)
        X509_free(cert_dup);
    if (pri) EVP_PKEY_free(pri);
}

void ResourceHandleManagerSSL::ClientCertSelectCallback(void *data, X509 **x509, EVP_PKEY **pkey)
{
    SSL* ssl = (SSL*)data;
    if (!ssl) return;

    ResourceHandle* job = NULL;
    ResourceHandleInternal* d = NULL;

    job = (ResourceHandle*)SSL_CTX_get_app_data(SSL_get_SSL_CTX(ssl));
    if (job)
        d = job->getInternal();
    if (!d || d->m_cancelled) {
        return;
    }

    ClientCertificate*  cert = NULL;
    ClientCertificate** certInfo = NULL;
    int certInfoIdx = -1;
    int certInfoNum = 0;
    STACK_OF(X509) *ca = NULL;
    unsigned char* orgpass = NULL;
    int orgpass_len = 0;
    X509 *server = NULL;
    char* requester = NULL;

    *x509 = NULL;
    *pkey = NULL;

    // should verify success!
    if (SSL_get_verify_mode(ssl) != SSL_VERIFY_NONE && 0 != SSL_get_verify_result(ssl))
        return;

    switch (SSL_version(ssl)) {
    case TLS1_VERSION:
        LOG_FUNCTION(ResourceHandleManager, "TLSv1.0 is not supported any more");
        return;
    case TLS1_1_VERSION:
        LOG_FUNCTION(ResourceHandleManager, "TLSv1.1 is not supported any more");
        return;
    case SSL2_VERSION:
        LOG_FUNCTION(ResourceHandleManager, "SSLv2 is not supported any more");
        return;
    case SSL3_VERSION:
        LOG_FUNCTION(ResourceHandleManager, "SSLv3 is not supported any more");
        return;
    case DTLS1_VERSION:
        LOG_FUNCTION(ResourceHandleManager, "DTLS1 is not supported any more");
        return;

#ifdef OPENSSL_SUPPORT_TLSv1_3
    case TLS1_3_VERSION:
#endif
#ifdef OPENSSL_SUPPORT_TLSv1_2
    case TLS1_2_VERSION:
#endif
        if (!SSL_has_ssl3_state(ssl)) return;
        break;
    default:
        break;
    }

    memset(gBuff, 0x00, RHMSSL_BUFF_LEN);
    requester = (char*)gBuff;

    server = SSL_get_peer_certificate(ssl);
    if (server) {
        int total_len = 0;
        strncat(requester, "Subject: ", (RHMSSL_BUFF_LEN));  total_len += 9;
        total_len += X509_NAME_get_text_by_NID(X509_get_subject_name(server), NID_organizationName, requester + total_len, (RHMSSL_BUFF_LEN - total_len));
        strncat(requester, "\r\nIssuer: ", (RHMSSL_BUFF_LEN - total_len));  total_len += 10;
        total_len += X509_NAME_get_text_by_NID(X509_get_issuer_name(server), NID_organizationName, requester + total_len, (RHMSSL_BUFF_LEN - total_len));
    }

    WKC::FrameLoaderClientWKC* fl = 0;
    cert = getClientCertCache(requester);
    if (!cert) {
        switch (SSL_version(ssl)) {
#ifdef OPENSSL_SUPPORT_TLSv1_3
        case TLS1_3_VERSION:
#endif
#ifdef OPENSSL_SUPPORT_TLSv1_2
        case TLS1_2_VERSION:
#endif
            if (!SSL_has_ssl3_state(ssl)) return;

            LOG_FUNCTION(ResourceHandleManager, "SSLv3/TLS");

            while (sk_X509_NAME_num(SSL_get0_peer_CA_list(ssl))) {
                X509_NAME* ca_name = sk_X509_NAME_pop(const_cast<STACK_OF(X509_NAME)*>(SSL_get0_peer_CA_list(ssl)));
                char *s;
                s = X509_NAME_oneline(ca_name, NULL, 0);

                HashSet<ClientCertificate*>::const_iterator it  = m_clientCerts.begin();
                HashSet<ClientCertificate*>::const_iterator end = m_clientCerts.end();
                for (; it != end; ++it) {
                    if ((*it)->sameIssuer(s)) {
                        certInfo = (ClientCertificate**)fastRealloc(certInfo, sizeof(ClientCertificate*)*(certInfoNum+1));
                        certInfo[certInfoNum] = (*it);
                        certInfoNum++;
                    }
                }
            }
            if (!certInfo)
                return;

            fl = frameloaderclientwkc(job);
            if (!d->m_cancelled && fl)
                certInfoIdx = fl->requestSSLClientCertSelect(job, (server)?(const char*)requester:"unknown", (void*)certInfo, certInfoNum);

            if (!d->m_isSynchronous) {
                curl_multi_refresh_timer(m_curlMultiHandle);
            }
            LOG_FUNCTION(ResourceHandleManager, "Selected Client Cert index=%d", certInfoIdx);
            if (certInfoIdx < 0) {
                fastFree(certInfo);
                return;
            }
            cert = certInfo[certInfoIdx];
            fastFree(certInfo);
            setClientCertCache(requester, cert);
            break;

        default:
            break;
        }
    }

    if (!cert) return;

    if (cert->cert() && cert->privateKey()) {
        setClientCertAndKey(ssl, cert, x509, pkey);
        return;
    }

    while (1) {
        orgpass = encrypt_key((const unsigned char *)cert->pass(), cert->passlen(), false, &orgpass_len);
        if (!orgpass)
            break;

        if (!parse_pkcs12((const unsigned char *)cert->pkcs12(), cert->pkcs12len(), (const char *)orgpass, pkey, x509, &ca))
            break;

        if (SSL_use_certificate(ssl, *x509) != 1)
            break;
        if (SSL_use_PrivateKey(ssl, *pkey) != 1)
            break;
        if (!SSL_check_private_key(ssl))
            break;
        if (ca && sk_X509_num(ca)) {
            for (int i = 0; i < sk_X509_num(ca); i++) {
                if (!SSL_CTX_add_extra_chain_cert(SSL_get_SSL_CTX(ssl), sk_X509_value(ca, i)))
                    break;
                if (!SSL_CTX_add_client_CA(SSL_get_SSL_CTX(ssl), sk_X509_value(ca, i)))
                    break;
            }
        }
        break;
    }
    if (orgpass) fastFree(orgpass);

    return;
}

// ROOT CA
void* ResourceHandleManagerSSL::SSLRegisterRootCA(const char* cert, int cert_len)
{
    void* certid = wkcSSLRegisterRootCAPeer(cert, cert_len);
    if (certid)
        clearRootCACache();
    return certid;
}

int ResourceHandleManagerSSL::SSLUnregisterRootCA(void* certid)
{
    int ret = wkcSSLUnregisterRootCAPeer(certid);
    if (ret == 0)
        clearRootCACache();
    return ret;
}

void* ResourceHandleManagerSSL::SSLRegisterRootCAByDER(const char* cert, int cert_len)
{
    void* ret = 0;
    X509 *x = 0;
    BIO *bio = 0;
    BUF_MEM *buf;

    bio = BIO_new(BIO_s_mem());
    if (!bio) {
        return 0;
    }
    x = d2i_X509(0, reinterpret_cast<const unsigned char **>(&cert), cert_len);
    if (!x) {
        goto end;
    }
    if (!PEM_write_bio_X509(bio, x)) {
        goto end;
    }

    BIO_get_mem_ptr(bio, &buf);
    ret = wkcSSLRegisterRootCAPeer(buf->data, buf->length);

end:
    if (x) X509_free(x);
    if (bio) BIO_free(bio);

    return ret;
}

void ResourceHandleManagerSSL::SSLRootCADeleteAll(void)
{
    wkcSSLRootCADeleteAllPeer();
    clearRootCACache();
}

bool ResourceHandleManagerSSL::buildRootCACache()
{
    ASSERT(m_rootCACache.isEmpty());

    // load cert using wkcOsslCertfOpenPeer() etc.
    STACK_OF(X509_INFO) *inf;
    X509_INFO *itmp;
    BIO *in;
    int i, ret = false;

    if (!wkcOsslCertfIsRegistPeer())
        return true;

    in = BIO_new_file(WKCOSSL_CERT_FILE, "r");
    if (!in)
        return false;
    inf = PEM_X509_INFO_read_bio(in, NULL, NULL, NULL);
    BIO_free(in);
    if (!inf)
        return false;
    for (i = 0; i < sk_X509_INFO_num(inf); i++) {
        itmp = sk_X509_INFO_value(inf, i);
        ASSERT(itmp->x509);
        X509* clone = X509_dup(itmp->x509);
        if (!clone) {
            clearRootCACache();
            goto end;
        }
        m_rootCACache.append(clone);
    }
    ret = true;
end:
    sk_X509_INFO_pop_free(inf, X509_INFO_free);
    return ret;
}

bool ResourceHandleManagerSSL::exportRootCACacheIntoStore(X509_STORE* store)
{
    ASSERT(store);

    if (m_rootCACache.isEmpty()) {
        if (!buildRootCACache())
            return false;
    }

    Vector<X509*>::const_iterator it = m_rootCACache.begin();
    Vector<X509*>::const_iterator end = m_rootCACache.end();
    for (; it != end; ++it) {
        if (!X509_STORE_add_cert(store, *it))
            return false;
    }

    return true;
}

void ResourceHandleManagerSSL::clearRootCACache()
{
    Vector<X509*>::const_iterator it = m_rootCACache.begin();
    Vector<X509*>::const_iterator end = m_rootCACache.end();
    for (; it != end; ++it) {
        X509_free(*it);
    }
    m_rootCACache.clear();
}


// CRL
void* ResourceHandleManagerSSL::SSLRegisterCRL(const char* crl, int crl_len)
{
    return wkcSSLRegisterCRLPeer(crl, crl_len);
}

int ResourceHandleManagerSSL::SSLUnregisterCRL(void* crlid)
{
    return wkcSSLUnregisterCRLPeer(crlid);
}

void ResourceHandleManagerSSL::SSLCRLDeleteAll(void)
{
    wkcSSLCRLDeleteAllPeer();
}

// Client Certificate
void* ResourceHandleManagerSSL::SSLRegisterClientCert(const unsigned char* pkcs12, int pkcs12_len, const unsigned char* pass, int pass_len)
{
    ClientCertificate* clientCert;

    clientCert = ClientCertificate::create(pkcs12, pkcs12_len, pass, pass_len);
    if (clientCert)
        m_clientCerts.add(clientCert);

    return (void*)clientCert;
}

void* ResourceHandleManagerSSL::SSLRegisterClientCertByDER(const unsigned char* cert, int cert_len, const unsigned char* key, int key_len)
{
    ClientCertificate* clientCert;

    clientCert = ClientCertificate::createByDER(cert, cert_len, key, key_len);
    if (clientCert)
        m_clientCerts.add(clientCert);

    return (void*)clientCert;
}

int ResourceHandleManagerSSL::SSLUnregisterClientCert(void* certid)
{
    ClientCertificate* cert = (ClientCertificate*)certid;
    if (!cert) return -1;

    HashSet<ClientCertificate*>::const_iterator it  = m_clientCerts.begin();
    HashSet<ClientCertificate*>::const_iterator end = m_clientCerts.end();
    for (; it != end; ++it) {
        if ((*it) == cert) {
            m_clientCerts.remove(cert);
            deleteClientCertCache(cert);
            delete cert;
            return 0;
        }
    }
    return -1;
}

void ResourceHandleManagerSSL::SSLClientCertDeleteAll(void)
{
    m_clientCertCache.clear();

    ClientCertificate* clicert;
    HashSet<ClientCertificate*>::const_iterator it  = m_clientCerts.begin();
    HashSet<ClientCertificate*>::const_iterator end = m_clientCerts.end();
    for (; it != end; ++it) {
        clicert = (*it);
        delete clicert;
    }
    m_clientCerts.clear();
}

// Certificate Black List
bool ResourceHandleManagerSSL::SSLRegisterBlackCert(const char* issuerCommonName, const char* serialNumber)
{
    unsigned char* serial = 0;
    int serial_len = 0, len = 0;
    char tmp[3];
    int i;

    if (!issuerCommonName || strlen(issuerCommonName) < 1)
        return false;

    if (serialNumber) {
        serial_len = strlen(serialNumber);
    }

    if (serial_len > 0) {
        if (serial_len%2)
            return false;

        serial = (unsigned char*)fastMalloc(serial_len/2);
        if (!serial)
            return false;
        memset(serial, 0x00, serial_len/2);

        for (i = 0, len = 0; i < serial_len; i+=2) {
            if (!isASCIIHexDigit(serialNumber[i]) || !isASCIIHexDigit(serialNumber[i+1])) {
                fastFree(serial);
                return false;
            }
            tmp[0] = (char)serialNumber[i];
            tmp[1] = (char)serialNumber[i+1];
            tmp[2] = (char)0;
            serial[len] = (unsigned char)strtol(tmp, NULL, 16);
            len++;
        }
        if (len*2 != serial_len) {
            fastFree(serial);
            return false;
        }
    }

    BlackCert* black = new BlackCert(issuerCommonName, serial, len);
    if (!black) {
        fastFree(serial);
        return false;
    }

    m_certBlackList.add(black);
    return true;
}

bool ResourceHandleManagerSSL::SSLRegisterBlackCertByDER(const char* cert, int cert_len)
{
    bool ret;
    X509 *x = 0;
    int index;

    X509_NAME_ENTRY* entry = 0;
    unsigned char* issuerName = 0;
    int len = 0;
    BIGNUM* bigNumSerialNumber = 0;
    const ASN1_INTEGER* ansiIntegerSerialNumber;
    char* hexSerialNumberStr = 0;
    bool result;

    x = d2i_X509(0, reinterpret_cast<const unsigned char **>(&cert), cert_len);
    if (!x) {
        ret =  false;
        goto end;
    }

    // Find the position of the commonName in the certificate
    index = X509_NAME_get_index_by_NID(X509_get_issuer_name(x), NID_commonName, -1);
    if (index == -1) { // -1 means not found
        // Find the position of the organizationalUnitName in the certificate
        index = X509_NAME_get_index_by_NID(X509_get_issuer_name(x), NID_organizationalUnitName, -1);
        if (index == -1) { // -1 means not found
            // Certificate has no commonName or organizationalUnitName
            // This may be illegal!!!
            ret = false;
            goto end;
        }
    }

    // Extract the issuerName
    entry = X509_NAME_get_entry(X509_get_issuer_name(x), index);
    len = ASN1_STRING_to_UTF8(&issuerName, entry->value);

    // Extract the serial
    ansiIntegerSerialNumber = X509_get0_serialNumber(x);
    bigNumSerialNumber = ASN1_INTEGER_to_BN(ansiIntegerSerialNumber, nullptr);
    // Convert serial to Hexadecimal
    hexSerialNumberStr = BN_bn2hex(bigNumSerialNumber);

    // Register with Black List
    result = SSLRegisterBlackCert(reinterpret_cast<char*>(issuerName), hexSerialNumberStr);
    OPENSSL_free(hexSerialNumberStr);
    OPENSSL_free(issuerName);
    if (!result) {
        ret = false;
        goto end;
    }
    ret = true;

end:
    if (x) X509_free(x);

    return ret;
}

void ResourceHandleManagerSSL::SSLBlackCertDeleteAll(void)
{
    BlackCert* blkcert;
    HashSet<BlackCert*>::const_iterator it  = m_certBlackList.begin();
    HashSet<BlackCert*>::const_iterator end = m_certBlackList.end();
    for (; it != end; ++it) {
        blkcert = (*it);
        if (blkcert->serial) fastFree(blkcert->serial);
        delete blkcert;
    }
    m_certBlackList.clear();
}

// Certificate Untrusted List
bool ResourceHandleManagerSSL::SSLRegisterUntrustedCert(const char* issuerCommonName, const char* serialNumber)
{
    unsigned char* serial = 0;
    int serial_len = 0, len = 0;
    char tmp[3];
    int i;

    if (!issuerCommonName || strlen(issuerCommonName) < 1)
        return false;

    if (serialNumber) {
        serial_len = strlen(serialNumber);
    }

    if (serial_len == 1 && *serialNumber == '0') {
        serial = (unsigned char*)fastMalloc(1);
        memset(serial, 0x00, 1);
        tmp[0] = '0';
        tmp[1] = '0';
        tmp[2] = (char)0;
        serial[0] = (unsigned char)strtol(tmp, NULL, 16);
        len = 1;
    } else if (serial_len > 1) {
        if (serial_len % 2)
            return false;

        serial = (unsigned char*)fastMalloc(serial_len / 2);
        if (!serial)
            return false;
        memset(serial, 0x00, serial_len / 2);

        for (i = 0, len = 0; i < serial_len; i += 2) {
            if (!isASCIIHexDigit(serialNumber[i]) || !isASCIIHexDigit(serialNumber[i + 1])) {
                fastFree(serial);
                return false;
            }
            tmp[0] = (char)serialNumber[i];
            tmp[1] = (char)serialNumber[i + 1];
            tmp[2] = (char)0;
            serial[len] = (unsigned char)strtol(tmp, NULL, 16);
            len++;
        }
        if (len * 2 != serial_len) {
            fastFree(serial);
            return false;
        }
    }

    UntrustedCert* untrusted = new UntrustedCert(issuerCommonName, serial, len);
    if (!untrusted) {
        fastFree(serial);
        return false;
    }

    m_untrustedCertList.add(untrusted);
    return true;
}

bool ResourceHandleManagerSSL::SSLRegisterUntrustedCertByDER(const char* cert, int cert_len)
{
    bool ret;
    X509 *x = 0;
    int index;

    X509_NAME_ENTRY* entry = 0;
    unsigned char* issuerName = 0;
    int len = 0;
    BIGNUM* bigNumSerialNumber = 0;
    const ASN1_INTEGER* ansiIntegerSerialNumber;
    char* hexSerialNumberStr = 0;
    bool result;

    x = d2i_X509(0, reinterpret_cast<const unsigned char **>(&cert), cert_len);
    if (!x) {
        ret = false;
        goto end;
    }

    // Find the position of the commonName in the certificate
    index = X509_NAME_get_index_by_NID(X509_get_issuer_name(x), NID_commonName, -1);
    if (index == -1) { // -1 means not found
        // Find the position of the organizationalUnitName in the certificate
        index = X509_NAME_get_index_by_NID(X509_get_issuer_name(x), NID_organizationalUnitName, -1);
        if (index == -1) { // -1 means not found
            // Certificate has no commonName or organizationalUnitName
            // This may be illegal!!!
            ret = false;
            goto end;
        }
    }

    // Extract the issuerName
    entry = X509_NAME_get_entry(X509_get_issuer_name(x), index);
    len = ASN1_STRING_to_UTF8(&issuerName, entry->value);

    // Extract the serial
    ansiIntegerSerialNumber = X509_get0_serialNumber(x);
    bigNumSerialNumber = ASN1_INTEGER_to_BN(ansiIntegerSerialNumber, nullptr);
    // Convert serial to Hexadecimal
    hexSerialNumberStr = BN_bn2hex(bigNumSerialNumber);

    // Register with Untrusted List
    result = SSLRegisterUntrustedCert(reinterpret_cast<char*>(issuerName), hexSerialNumberStr);
    OPENSSL_free(hexSerialNumberStr);
    OPENSSL_free(issuerName);
    if (!result) {
        ret = false;
        goto end;
    }
    ret = true;

end:
    if (x) X509_free(x);

    return ret;
}

/*
 * SSLRegisterEVSSLOID()
 *
 * arguments
 *   All arguments are for Trusted ROOT CA
 *   issuerCommonName: Issuer CommonName or organizationalUnitName
 *   OID: EV SSL OID
 *   sha1FingerPrint: char pointer of XX:XX:XX:XX:XX:XX:XX:XX:XX... format
 *   SerialNumber: char pointer of XXXXXXXXXXXXXXXXXX, length is even number.
 *
 * Return value
 *   true
 *   false
 */
bool ResourceHandleManagerSSL::SSLRegisterEVSSLOID(const char *issuerCommonName, const char *OID, const char *sha1FingerPrint, const char *SerialNumber)
{
    unsigned char sha1[20];
    int sha1_len;
    char tmp[3];
    unsigned char* serial;
    int serial_len;
    int len;
    int i;

    if (!issuerCommonName || !OID || !sha1FingerPrint|| !SerialNumber)
        return false;
    if (strlen(sha1FingerPrint) != 59)
        return false;
    if (strlen(SerialNumber) < 2 || strlen(SerialNumber)%2)
        return false;

    /* in_oid contains digit or '.' */
    len = strlen(OID);
    for (i = 0; i < len; i++) {
        if (!isASCIIDigit(OID[i]) && OID[i] != '.')
            return false;
    }

    /* sha1FingerPrint contains isxdigit or ':' */
    memset(sha1, 0x00, 20);
    sha1_len = 0;
    len = strlen(sha1FingerPrint);
    for (i = 0; i < len; i+=3) {
        if (!isASCIIHexDigit(sha1FingerPrint[i]) || !isASCIIHexDigit(sha1FingerPrint[i+1]) || (sha1FingerPrint[i+2] != ':' && sha1FingerPrint[i+2] != 0x0))
            return 0;
        tmp[0] = (char)sha1FingerPrint[i];
        tmp[1] = (char)sha1FingerPrint[i+1];
        tmp[2] = (char)0;
        sha1[sha1_len] = (unsigned char)strtol(tmp, NULL, 16);
        sha1_len++;
    }

    /* SerialNumber contains isxdigit */
    serial_len = strlen(SerialNumber);
    serial = (unsigned char*)fastMalloc(serial_len/2);
    if (!serial)
        return false;
    memset(serial, 0x00, serial_len/2);

    for (i = 0, len = 0; i < serial_len; i+=2) {
        if (!isASCIIHexDigit(SerialNumber[i]) || !isASCIIHexDigit(SerialNumber[i+1]))
            return 0;
        tmp[0] = (char)SerialNumber[i];
        tmp[1] = (char)SerialNumber[i+1];
        tmp[2] = (char)0;
        serial[len] = (unsigned char)strtol(tmp, NULL, 16);
        len++;
    }
    if (len*2 != serial_len) {
        fastFree(serial);
        return false;
    }

    RootCA_OID* oid = new RootCA_OID(issuerCommonName, OID, sha1, serial, len);
    if (!oid) {
        fastFree(serial);
        return false;
    }

    m_rootOIDList.add(oid);
    return true;
}

void ResourceHandleManagerSSL::SSLEVSSLOIDDeleteAll(void)
{
    RootCA_OID* oid;
    HashSet<RootCA_OID*>::const_iterator it  = m_rootOIDList.begin();
    HashSet<RootCA_OID*>::const_iterator end = m_rootOIDList.end();
    for (; it != end; ++it) {
        oid = (*it);
        if (oid->serial) fastFree(oid->serial);
        delete oid;
    }
    m_rootOIDList.clear();
}

void ResourceHandleManagerSSL::SSLEnableProtocols(unsigned int versions)
{
    unsigned int ver;

    if (!versions) {
        ver = CURL_SSLVERSION_LAST;
    }
    else {
        ver = CURL_SSLVERSION_DEFAULT;

#ifdef OPENSSL_SUPPORT_TLSv1_2
        if (versions & CURL_SSLVERSION_TLSv1_2)
            ver |= CURL_SSLVERSION_TLSv1_2;
#endif
#ifdef OPENSSL_SUPPORT_TLSv1_3
        if (versions & CURL_SSLVERSION_TLSv1_3)
            ver |= CURL_SSLVERSION_TLSv1_3;
#endif
    }

    m_enableVersion = ver;
}

void ResourceHandleManagerSSL::saveCRLFile(const char *content_type, void *buff, int len)
{
    X509_CRL *crl = NULL;
    BIO *in = NULL;
    BIO *out = NULL;
    const char *data = NULL;
    int data_len;

    if (strncmp(content_type, "application/pkix-crl", 20)
        && strncmp(content_type, "application/x-pkcs7-crl", 23))
        return;

    if (!buff || len < 26)
        return;

    in = BIO_new_mem_buf(buff, len);
    if (!in)
        goto end;

    out = BIO_new(BIO_s_mem());
    if (!out)
        goto end;

    if (!memcmp(buff, "-----", 5))
        crl = PEM_read_bio_X509_CRL(in, NULL, NULL, NULL);
    else
        crl = d2i_X509_CRL_bio(in, NULL);
    if (!crl)
        goto end;

    // convert to PEM format
    if (!PEM_write_bio_X509_CRL(out, crl))
        goto end;
    data_len = BIO_get_mem_data(out, &data);

    (void)wkcSSLRegisterCRLPeer(data, data_len);

end:
    if (out) BIO_free(out);
    if (crl) X509_CRL_free(crl);
    if (in) BIO_free(in);
}

void ResourceHandleManagerSSL::SSLEnableOnlineCertChecks(bool enableOCSP, bool enableCRLDP)
{
    m_enableOCSP  = enableOCSP;
    m_enableCRLDP = enableCRLDP;
}

////////////////////////////////////////////
//
// private functions
//
void  ResourceHandleManagerSSL::addServerCertChain(const char* url, STACK_OF(X509) *sk)
{
    int certnum = sk_X509_num(sk);

    Certificate* chain = new Certificate();
    if (!chain)
        return;

    for (int i = 0; i < certnum; i++) {
        X509* x = sk_X509_value(sk, i);
        BIO* bio_out = BIO_new(BIO_s_mem());
        BUF_MEM* biomem;
        PEM_write_bio_X509(bio_out, x);
        BIO_get_mem_ptr(bio_out, &biomem);

        chain->push(biomem->data, biomem->length);

        BIO_free(bio_out);
    }

    URL kurl = URL({}, url);
    String hostPort = SSLhostAndPort(kurl);

    auto oldCert = m_serverCertChain.find(hostPort);
    if (oldCert != m_serverCertChain.end()) {
        delete oldCert->value;
    }

    m_serverCertChain.set(hostPort, chain);

    int num = m_serverCertChain.size();
    if (cMaxChains < num) {
        HashMap<String, Certificate*>::iterator it  = m_serverCertChain.begin();
        HashMap<String, Certificate*>::iterator end = m_serverCertChain.end();
        Certificate* value = it->value;
        time_t oldest = value->createTime();
        HashMap<String, Certificate*>::iterator keep = it;
        ++it;
        for (; it != end; ++it) {
            value = it->value;
            if (value->createTime() < oldest) {
                oldest = value->createTime();
                keep = it;
            }
        }
        delete keep->value;
        m_serverCertChain.remove(keep);
    }
    return;
}

const char** ResourceHandleManagerSSL::getServerCertChain(const char* url, int& outCertNum)
{
    URL kurl = URL({}, url);
    String hostPort = SSLhostAndPort(kurl);

    outCertNum = 0;

    Certificate* cert = m_serverCertChain.get(String(hostPort));
    if (!cert) {
        return 0;
    }

    int num = cert->num();
    if (0 == num) {
        return 0;
    }

    char** chain = (char**)fastMalloc(sizeof(char*) * num);
    if (!chain)
        return 0;
    memset(chain, 0x0, (sizeof(char*) * num));
    for (int i = 0; i < num; i++) {
        chain[i] = fastStrdup((cert->get(i)).utf8().data());
        if (!chain[i]) {
            for (int j = 0; j < num; j++) {
                if (chain[j]) fastFree(chain[j]);
            }
            fastFree((void*)chain);
            return 0;
        }
    }
    outCertNum = num;
    return (const char**)chain;
}

void ResourceHandleManagerSSL::freeServerCertChain(const char** chain, int num)
{
    char** c = (char**)chain;
    for (int i = 0; i < num; i++) {
        if (c[i])
            fastFree((void*)c[i]);
    }
    fastFree((void*)c);
}

bool ResourceHandleManagerSSL::allowsServerHost(const String& host)
{
    return m_AllowServerHost.contains(host.convertToASCIILowercase());
}

void ResourceHandleManagerSSL::setClientCertCache(const char* requester, ClientCertificate* cert)
{
    m_clientCertCache.set(String(requester), cert);
}

ClientCertificate* ResourceHandleManagerSSL::getClientCertCache(const char *requester)
{
    return m_clientCertCache.get(String(requester));
}

static int
organizationName(const String& name, char *buf, int buflen)
{
    Vector<String> list = name.split('/');
    for (int i = 0; i < list.size(); i++) {
        Vector<String> items = list[i].split('=');
        if (items.size()==0) break;
        if (items.size()>2) {
            for (int j=2; j<items.size(); j++) {
                items[1].append("=");
                items[1].append(items[j]);
            }
        }
        if (items[0] == "O") {
            strncat(buf, items[1].utf8().data(), buflen-strlen(buf)-1);
            return items[1].length();
        }
    }
    return 0;
}

void ResourceHandleManagerSSL::deleteClientCertCache(ClientCertificate* cert)
{
    char *requester;
    int buflen = RHMSSL_BUFF_LEN;
    int len;

    memset(gBuff, 0x00, RHMSSL_BUFF_LEN);
    requester = (char*)gBuff;
    strncat(requester, "Subject: ", buflen-strlen(requester)-1);  requester += 9;  buflen -= 9;
    len = organizationName(cert->issuer(), requester, buflen-strlen(requester)-1);  requester += len;  buflen -= len;
    strncat(requester, "\r\nIssuer: ", buflen-strlen(requester)-1);  requester += 10;  buflen -= 10;
    len = organizationName(cert->subject(), requester, buflen-strlen(requester)-1); requester += len;  buflen -= len;

    m_clientCertCache.remove(String((const char*)gBuff));
}

int ResourceHandleManagerSSL::verifyTrustedCertificate(void* data)
{
    UntrustedCert* untrustedcert;
    ASN1_INTEGER *num;
    X509* cert;
    int len;

    if (m_untrustedCertList.isEmpty()) {
        return EUntrustedCertNotDetected;
    }

    if (!data) {
        return EUntrustedCertNotDetected;
    }
    cert = (X509*)data;

    gBuff[0] = 0x0;
    len = X509_NAME_get_text_by_NID(X509_get_issuer_name(cert), NID_commonName, (char*)gBuff, RHMSSL_BUFF_LEN);
    if (len < 1) { // negative means not exist, 0 means exist but length 0
        len = X509_NAME_get_text_by_NID(X509_get_issuer_name(cert), NID_organizationalUnitName, (char*)gBuff, RHMSSL_BUFF_LEN);
        if (len < 1) { // negative means not exist, 0 means exist but length 0
            // Certificate has no commonName or organizationalUnitName
            return EUntrustedCertNotDetected;
        }
    }

    num = X509_get_serialNumber(cert);
    String CertName((const char*)gBuff);
    
    HashSet<UntrustedCert*>::const_iterator it = m_untrustedCertList.begin();
    HashSet<UntrustedCert*>::const_iterator end = m_untrustedCertList.end();
    for (; it != end; ++it) {
        untrustedcert = (*it);
        if (!untrustedcert->serial || ((num->length == untrustedcert->serial_len) && !memcmp(untrustedcert->serial, num->data, untrustedcert->serial_len))) {
            if (untrustedcert->issuerCommonName == CertName) {
                if (CertName.substring(0, strlen(GEOTRUST_CN)) == GEOTRUST_CN) {
                    return EGeoTrustCertDetected;
                } else {
                    return EUntrustedCertDetected;
                }
            }
        }
    }

    return EUntrustedCertNotDetected;
}

bool ResourceHandleManagerSSL::isCertificateException(void* data)
{
    X509* cert;

    if (m_untrustedCertList.isEmpty()) {
        return false;
    }

    if (!data) {
        return false;
    }
    cert = (X509*)data;

    gBuff[0] = 0x0;
    X509_NAME_get_text_by_NID(X509_get_subject_name(cert), NID_commonName, (char*)gBuff, RHMSSL_BUFF_LEN);
    String CertSubject((const char*)gBuff);

    if (CertSubject.substring(0, strlen(APPLE_IST_CN)) == APPLE_IST_CN) {
        return true;
    }

    return false;
}

bool ResourceHandleManagerSSL::isCertificateBlack(void* data)
{
    BlackCert* blkcert;
    ASN1_INTEGER *num;
    X509* cert;
    int len;

    if (m_certBlackList.isEmpty()) {
        return false;
    }

    if (!data) {
        return false;
    }
    cert = (X509*)data;

    gBuff[0] = 0x0;
    len = X509_NAME_get_text_by_NID(X509_get_issuer_name(cert), NID_commonName, (char*)gBuff, RHMSSL_BUFF_LEN);
    if (len < 1) { // negative means not exist, 0 means exist but length 0
        len = X509_NAME_get_text_by_NID(X509_get_issuer_name(cert), NID_organizationalUnitName, (char*)gBuff, RHMSSL_BUFF_LEN);
        if (len < 1) { // negative means not exist, 0 means exist but length 0
            // Certificate has no commonName or organizationalUnitName
            // This may be illegal!!!
            return false;
        }
    }

    num = X509_get_serialNumber(cert);
    String CertName((const char*)gBuff);

    HashSet<BlackCert*>::const_iterator it  = m_certBlackList.begin();
    HashSet<BlackCert*>::const_iterator end = m_certBlackList.end();
    for (; it != end; ++it) {
        blkcert = (*it);
        if (!blkcert->serial || ((num->length == blkcert->serial_len) && !memcmp(blkcert->serial, num->data, blkcert->serial_len)) ) {
            if (blkcert->issuerCommonName == CertName) {
                return true;
            }
        }
    }
    return false;
}

void ResourceHandleManagerSSL::lock()
{
    wkcMutexLockPeer(m_mutex);
}

void ResourceHandleManagerSSL::unlock()
{
    wkcMutexUnlockPeer(m_mutex);
}

////////////////////////////////////////////
//
// Client Certificate Class
//
ClientCertificate::ClientCertificate()
    : m_pkcs12(0)
    , m_pkcs12Len(0)
    , m_pass(0)
    , m_passLen(0)
    , m_cert(0)
    , m_privateKey(0)
    , m_privateKeyLen(0)
    , m_caIndexs(0)
    , m_caIndexNum(0)
{
}

ClientCertificate::~ClientCertificate()
{
    if(m_pkcs12) {
        memset(m_pkcs12, 0x00, m_pkcs12Len);
        fastFree(m_pkcs12);
        m_pkcs12 = NULL;
        m_pkcs12Len = 0;
    }
    if (m_pass) {
        memset(m_pass, 0x00, m_passLen);
        fastFree(m_pass);
        m_pass = NULL;
        m_passLen = 0;
    }
    if (m_cert) {
        X509_free(m_cert);
        m_cert = NULL;
    }
    if (m_privateKey) {
        fastFree(m_privateKey);
        m_privateKey = NULL;
        m_privateKeyLen = 0;
    }
    if (m_caIndexs) {
        for (int i = 0; i < m_caIndexNum; i++) {
            if (m_caIndexs[i])
                wkcSSLUnregisterRootCAPeer(m_caIndexs[i]);
        }
        fastFree(m_caIndexs);
        m_caIndexs = 0;
        m_caIndexNum = 0;
    }
}

void ClientCertificate::init(X509 *x509)
{
    char *s=NULL;
    int buflen;
    const ASN1_TIME *certdate = 0;
    ASN1_INTEGER *num = 0;

    gBuff[0] = 0x00;
    m_Issuer = X509_NAME_oneline(X509_get_issuer_name(x509), (char*)gBuff, RHMSSL_BUFF_LEN);
    gBuff[0] = 0x00;
    m_Subject = X509_NAME_oneline(X509_get_subject_name(x509), (char*)gBuff, RHMSSL_BUFF_LEN);

    num = X509_get_serialNumber(x509);
    if (num->length <= 4) {
        m_serialNumber = String::number(ASN1_INTEGER_get(num));
    }
    else {
        s = (char*)gBuff;
        buflen = RHMSSL_BUFF_LEN;
        s[0] = 0x00;
        if((num->length*3) < (buflen - 1)) {
            for (int j = 0; j < num->length; j++) {
                snprintf(s, buflen, "%02x%c", num->data[j], ((j+1 == num->length)?0x0:':'));
                s += 3;
                buflen -= 3;
            }
        }
        m_serialNumber = (char*)gBuff;
    }

    certdate = X509_get0_notBefore(x509);
    asn1_output(certdate, (char*)gBuff, RHMSSL_BUFF_LEN);
    m_NotBefore = (char*)gBuff;

    certdate = X509_get0_notAfter(x509);
    asn1_output(certdate, (char*)gBuff, RHMSSL_BUFF_LEN);
    m_NotAfter = (char*)gBuff;
}

ClientCertificate* ClientCertificate::create(const unsigned char* pkcs12, int pkcs12_len, const unsigned char* pass, int pass_len)
{
    ClientCertificate* self = NULL;

    int newpass_len;

    EVP_PKEY *pri = NULL;
    STACK_OF(X509) *ca = NULL;
    X509 *x509 = NULL;

    self = new ClientCertificate();
    if (!self)
        return (ClientCertificate*)0;

    self->m_pkcs12 = fastMalloc(pkcs12_len);
    if (!self->m_pkcs12)
        goto regist_error;
    memcpy(self->m_pkcs12, pkcs12, pkcs12_len);
    self->m_pkcs12Len = pkcs12_len;

    self->m_pass = encrypt_key(pass, pass_len, true, &newpass_len);
    if (!self->m_pass)
        goto regist_error;
    self->m_passLen = newpass_len;

    if (!parse_pkcs12(pkcs12, pkcs12_len, (const char *)pass, &pri, &x509, &ca))
        goto regist_error;
    if (!x509)
        goto regist_error;

    self->init(x509);

    if (ca && sk_X509_num(ca)) {
        int ca_num = sk_X509_num(ca);
        void** caindex = (void**)fastMalloc(sizeof(void*) * ca_num);
        if (caindex) {
            for (int i = 0; i < ca_num; i++) {
                X509* x = sk_X509_value(ca, i);
                BIO* bio_out = BIO_new(BIO_s_mem());
                BUF_MEM* biomem;
                PEM_write_bio_X509(bio_out, x);
                BIO_get_mem_ptr(bio_out, &biomem);

                caindex[i] = wkcSSLRegisterRootCAPeer(biomem->data, biomem->length);

                BIO_free(bio_out);
            }
            self->setCAIndexs(caindex, ca_num);
        }
    }

    X509_free(x509);
    EVP_PKEY_free(pri);
    if (ca) sk_X509_free(ca);

    return self;

regist_error:
    if (x509) X509_free(x509);
    if (pri) EVP_PKEY_free(pri);
    if (ca) sk_X509_free(ca);
    if (self) delete self;

    return (ClientCertificate*)0;
}

ClientCertificate* ClientCertificate::createByDER(const unsigned char* cert, int cert_len, const unsigned char* key, int key_len)
{
    ClientCertificate* self = NULL;
    int newkey_len;
    const unsigned char* next = key;

    EVP_PKEY *pri = NULL;
    X509 *x509 = NULL;

    self = new ClientCertificate();
    if (!self)
        return (ClientCertificate*)0;

    x509 = d2i_X509(NULL, &cert, cert_len);
    if (!x509) {
        goto regist_error;
    }

    // check private key is valid or not
    pri = d2i_AutoPrivateKey(NULL, &next, key_len);
    if (!pri) {
        goto regist_error;
    }
    EVP_PKEY_free(pri);

    self->m_privateKey = encrypt_key(key, key_len, true, &newkey_len);
    if (!self->m_privateKey) {
        goto regist_error;
    }
    self->m_privateKeyLen = newkey_len;
    self->m_cert = x509;
    self->init(x509);

    return self;

regist_error:
    if (x509) X509_free(x509);
    if (self) delete self;

    return (ClientCertificate*)0;
}

} // namespace WebCore

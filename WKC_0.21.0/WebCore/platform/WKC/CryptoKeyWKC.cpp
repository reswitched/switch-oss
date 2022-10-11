/*
 * Copyright (c) 2015-2019 ACCESS CO., LTD. All rights reserved.
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

#include "config.h"
#include "CryptoKeyRSA.h"

#include "CryptoAlgorithmRegistry.h"
#include "CryptoKeyAES.h"
#include "CryptoKeyDataRSAComponents.h"
#include "CryptoKeyDataOctetSequence.h"
#include "CryptoKeyHMAC.h"
#include "CryptoKeySerializationRaw.h"

#include "CryptoAlgorithmAES_CBC.h"
#include "CryptoAlgorithmAES_KW.h"
#include "CryptoAlgorithmHMAC.h"
#include "CryptoAlgorithmRSAES_PKCS1_v1_5.h"
#include "CryptoAlgorithmRSASSA_PKCS1_v1_5.h"
#include "CryptoAlgorithmRSA_OAEP.h"

#include "CryptoAlgorithmAesCbcParams.h"

#include "CryptoAlgorithmSHA1.h"
#include "CryptoAlgorithmSHA224.h"
#include "CryptoAlgorithmSHA256.h"
#include "CryptoAlgorithmSHA384.h"
#include "CryptoAlgorithmSHA512.h"

#include "CryptoDigest.h"
#include "CryptoKeyPair.h"

#include "ExceptionCode.h"

#include "NotImplemented.h"

#include <openssl/aes.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/hmac.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/ossl_typ.h>
#include <openssl/rsa_locl.h>
#include <openssl/bn_lcl.h>
#include <openssl/hmac_lcl.h>

#if ENABLE(SUBTLE_CRYPTO)

namespace WebCore {

CryptoKeyRSA::CryptoKeyRSA(CryptoAlgorithmIdentifier identifier, CryptoKeyType type, PlatformRSAKey platformKey, bool extractable, CryptoKeyUsage usage)
    : CryptoKey(identifier, type, extractable, usage)
    , m_platformKey(platformKey)
    , m_restrictedToSpecificHash(false)
{
}

CryptoKeyRSA::~CryptoKeyRSA()
{
    if (m_platformKey) {
        RSA_free((RSA *)m_platformKey);
    }
}

PassRefPtr<CryptoKeyRSA>
CryptoKeyRSA::create(CryptoAlgorithmIdentifier identifier, const CryptoKeyDataRSAComponents& obj, bool extractable, CryptoKeyUsage usage)
{
    CryptoKeyType type = obj.type()==CryptoKeyDataRSAComponents::Type::Public ? CryptoKeyType::Public : CryptoKeyType::Private;

    RSA* rsa = 0;
    BIGNUM* modu = 0;
    BIGNUM* expo = 0;
    BIGNUM* pexp = 0;
    BIGNUM* pp = 0;
    BIGNUM* pq = 0;

    rsa = RSA_new();
    if (!rsa)
        goto error_end;

    modu = BN_bin2bn(obj.modulus().data(), obj.modulus().size(), 0);
    expo = BN_bin2bn(obj.exponent().data(), obj.exponent().size(), 0);
    if (!modu || !expo)
        goto error_end;
    if (type==CryptoKeyType::Private) {
        pexp = BN_bin2bn(obj.privateExponent().data(), obj.privateExponent().size(), 0);
        pp = BN_bin2bn(obj.firstPrimeInfo().primeFactor.data(), obj.firstPrimeInfo().primeFactor.size(), 0);
        pq = BN_bin2bn(obj.secondPrimeInfo().primeFactor.data(), obj.secondPrimeInfo().primeFactor.size(), 0);
        if (!pexp || !pp || !pq)
            goto error_end;
    }

    rsa->n = modu;
    rsa->e = expo;
    rsa->d = pexp;
    rsa->p = pp;
    rsa->q = pq;
    if (type==CryptoKeyType::Private) {
        BN_CTX* ctx = 0;
        BIGNUM* one = BN_new();
        BN_set_word(one, 1);
        rsa->dmp1 = BN_new();
        rsa->dmq1 = BN_new();
        rsa->iqmp = BN_new();
        if (!rsa->dmp1 || !rsa->dmq1 || !rsa->iqmp)
            goto error_end;
        ctx = BN_CTX_new();
        if (!ctx)
            goto error_end;
        BN_mod_sub(rsa->dmp1, pexp, pp, one, ctx);
        BN_mod_sub(rsa->dmq1, pexp, pq, one, ctx);
        BN_sqr(one, pq, ctx);
        BN_mod(rsa->iqmp, one, pp, ctx);
        BN_free(one);
        BN_CTX_free(ctx);
    } else {
        rsa->dmp1 = 0;
        rsa->dmq1 = 0;
        rsa->iqmp = 0;
    }

    return adoptRef(new CryptoKeyRSA(identifier, type, (void *)rsa, extractable, usage));

error_end:
    if (pq)
        BN_clear_free(pq);
    if (pp)
        BN_clear_free(pp);
    if (pexp)
        BN_clear_free(pexp);
    if (expo)
        BN_clear_free(expo);
    if (modu)
        BN_clear_free(modu);
    if (rsa)
        RSA_free(rsa);
    return 0;
}

bool
CryptoKeyRSA::isRestrictedToHash(CryptoAlgorithmIdentifier& h) const
{
    if (m_restrictedToSpecificHash) {
        h = m_hash;
        return true;
    }
    return false;
}

void
CryptoKeyRSA::restrictToHash(CryptoAlgorithmIdentifier identifier)
{
    m_restrictedToSpecificHash = true;
    m_hash = identifier;
}

size_t
CryptoKeyRSA::keySizeInBits() const
{
    RSA* rsa = (RSA *)m_platformKey;
    return BN_num_bits(rsa->e);
}

void
CryptoKeyRSA::buildAlgorithmDescription(CryptoAlgorithmDescriptionBuilder& builder) const
{
    notImplemented();
}

std::unique_ptr<CryptoKeyData>
CryptoKeyRSA::exportData() const
{
    RSA* rsa = (RSA *)m_platformKey;
    const BIGNUM* n = 0;
    const BIGNUM* e = 0;
    const BIGNUM* d = 0;
    RSA_get0_key(rsa, &n, &e, &d);
    if (!n || !e)
        return nullptr;
    Vector<uint8_t> modulus(BN_num_bytes(n));
    Vector<uint8_t> exponent(BN_num_bytes(e));
    if (!d) {
        // Public key
        return CryptoKeyDataRSAComponents::createPublic(modulus, exponent);
    } else {
        // Private key
        Vector<uint8_t> privateExponent(BN_num_bytes(d));
        return CryptoKeyDataRSAComponents::createPrivate(modulus, exponent, privateExponent);
    }
}

void
CryptoKeyRSA::generatePair(CryptoAlgorithmIdentifier identifier, unsigned modulusLength, const Vector<uint8_t>& publicExponent, bool extractable, CryptoKeyUsage usage, KeyPairCallback success, VoidCallback failureCallback)
{
    RSA* rsa = 0;
    RSA* pub = 0;
    RSA* pri = 0;
    BIGNUM* bn = 0;
    int ret = 0;
    rsa = RSA_new();
    bn = BN_new();
    if (!rsa || !bn)
        goto error_end;

    bn = BN_bin2bn(publicExponent.data(), publicExponent.size(), bn);
    if (!bn)
        goto error_end;

    ret = RSA_generate_key_ex(rsa, modulusLength, bn, 0);
    if (!ret)
        goto error_end;

    BN_clear_free(bn);

    pub = RSAPublicKey_dup(rsa);
    pri = RSAPrivateKey_dup(rsa);
    if (!pub || !pri)
        goto error_end;

    RSA_free(rsa);

    {
        RefPtr<CryptoKeyRSA> pubk = CryptoKeyRSA::create(identifier, CryptoKeyType::Public, (void *)pub, extractable, usage);
        RefPtr<CryptoKeyRSA> prik = CryptoKeyRSA::create(identifier, CryptoKeyType::Private, (void *)pri, extractable, usage);
        RefPtr<CryptoKeyPair> retval = CryptoKeyPair::create(pubk, prik);
        success(*retval.release());
    }
    return;

error_end:
    if (pub)
        RSA_free(pub);
    if (pri)
        RSA_free(pri);
    if (rsa)
        RSA_free(rsa);
    if (bn)
        BN_clear_free(bn);
    failureCallback();
}

void
CryptoAlgorithmRegistry::platformRegisterAlgorithms(void)
{
    registerAlgorithm<CryptoAlgorithmAES_CBC>();
    registerAlgorithm<CryptoAlgorithmHMAC>();
    registerAlgorithm<CryptoAlgorithmRSAES_PKCS1_v1_5>();
    registerAlgorithm<CryptoAlgorithmRSASSA_PKCS1_v1_5>();
    registerAlgorithm<CryptoAlgorithmRSA_OAEP>();
    registerAlgorithm<CryptoAlgorithmSHA1>();
    registerAlgorithm<CryptoAlgorithmSHA224>();
    registerAlgorithm<CryptoAlgorithmSHA256>();
    registerAlgorithm<CryptoAlgorithmSHA384>();
    registerAlgorithm<CryptoAlgorithmSHA512>();
}

void
CryptoAlgorithmAES_CBC::platformEncrypt(const CryptoAlgorithmAesCbcParams& params, const CryptoKeyAES& key, const CryptoOperationData& data, VectorCallback success, VoidCallback failureCallback, ExceptionCode& ec)
{
    EVP_CIPHER_CTX* ctx = nullptr;
    const EVP_CIPHER* cipher = nullptr;

    size_t buflen = ((data.second + AES_BLOCK_SIZE) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    Vector<uint8_t> ciphertext_buf(buflen);

    unsigned char* ciphertext = ciphertext_buf.data();
    int ciphertext_len = 0;

    const unsigned char* plaintext = data.first;
    int plaintext_len = data.second;

    int len = 0;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        goto error;

    switch (key.key().size() * 8) {
    case 128:
        cipher = EVP_aes_128_cbc();
        break;
    case 192:
        cipher = EVP_aes_192_cbc();
        break;
    case 256:
        cipher = EVP_aes_256_cbc();
        break;
    default:
        goto error;
    }

    if (1 != EVP_EncryptInit_ex(ctx, cipher, nullptr, key.key().data(), reinterpret_cast<const unsigned char *>(params.iv.data())))
        goto error;

    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        goto error;

    ciphertext_len += len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        goto error;

    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    ciphertext_buf.shrink(ciphertext_len);
    success(ciphertext_buf);

    return;

error:
    if (ctx)
        EVP_CIPHER_CTX_free(ctx);

    ec = ABORT_ERR;
    failureCallback();
}

void
CryptoAlgorithmAES_CBC::platformDecrypt(const CryptoAlgorithmAesCbcParams& params, const CryptoKeyAES& key, const CryptoOperationData& data, VectorCallback success, VoidCallback failureCallback, ExceptionCode& ec)
{
    EVP_CIPHER_CTX* ctx = nullptr;
    const EVP_CIPHER* cipher = nullptr;

    Vector<uint8_t> plaintext_buf(data.second);

    unsigned char* plaintext = plaintext_buf.data();
    int plaintext_len = 0;

    const unsigned char* ciphertext = data.first;
    int ciphertext_len = data.second;

    int len = 0;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        goto error;

    switch (key.key().size() * 8) {
    case 128:
        cipher = EVP_aes_128_cbc();
        break;
    case 192:
        cipher = EVP_aes_192_cbc();
        break;
    case 256:
        cipher = EVP_aes_256_cbc();
        break;
    default:
        goto error;
    }

    if (1 != EVP_DecryptInit_ex(ctx, cipher, nullptr, key.key().data(), reinterpret_cast<const unsigned char *>(params.iv.data())))
        goto error;

    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        goto error;

    plaintext_len += len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        goto error;

    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    plaintext_buf.shrink(plaintext_len);
    success(plaintext_buf);

    return;

error:
    if (ctx)
        EVP_CIPHER_CTX_free(ctx);

    ec = ABORT_ERR;
    failureCallback();
}

void
CryptoAlgorithmAES_KW::platformEncrypt(const CryptoKeyAES&, const CryptoOperationData&, VectorCallback, VoidCallback failureCallback, ExceptionCode& ec)
{
    ec = NOT_SUPPORTED_ERR;
    failureCallback();
}

void
CryptoAlgorithmAES_KW::platformDecrypt(const CryptoKeyAES&, const CryptoOperationData&, VectorCallback, VoidCallback failureCallback, ExceptionCode& ec)
{
    ec = NOT_SUPPORTED_ERR;
    failureCallback();
}

void
CryptoAlgorithmHMAC::platformSign(const CryptoAlgorithmHmacParams& params, const CryptoKeyHMAC& key, const CryptoOperationData& data, VectorCallback success, VoidCallback failureCallback, ExceptionCode& ec)
{
    unsigned int len = 0;
    unsigned char* ret = HMAC(EVP_sha256(), key.key().data(), key.key().size(), data.first, data.second, 0, &len);
    if (!ret) {
        ec = ABORT_ERR;
        failureCallback();
        return;
    }
    Vector<uint8_t> result;
    result.append(ret, len);
    success(result);
}

void
CryptoAlgorithmHMAC::platformVerify(const CryptoAlgorithmHmacParams& params, const CryptoKeyHMAC& key, const CryptoOperationData& signature, const CryptoOperationData& data, BoolCallback success, VoidCallback failureCallback, ExceptionCode& ec)
{
    HMAC_CTX ctx;
    unsigned int len=0;
    unsigned char buf[EVP_MAX_MD_SIZE];
    HMAC_CTX_reset(&ctx);

    int ret = HMAC_Init_ex(&ctx, key.key().data(), key.key().size(), EVP_sha256(), NULL);
    if (!ret)
        goto error_end;
    ret = HMAC_Update(&ctx, data.first, data.second);
    if (!ret)
        goto error_end;
    ret = HMAC_Final(&ctx, buf, &len);
    if (!ret)
        goto error_end;
    HMAC_CTX_reset(&ctx);
    success(true);
    return;

error_end:
    ec = ABORT_ERR;
    failureCallback();
    HMAC_CTX_reset(&ctx);
    return;
}

void
CryptoAlgorithmRSAES_PKCS1_v1_5::platformEncrypt(const CryptoKeyRSA& key, const CryptoOperationData& data, VectorCallback success, VoidCallback failureCallback, ExceptionCode& ec)
{
    RSA* rsa = (RSA *)key.platformKey();
    Vector<uint8_t> out(RSA_size(rsa));
    int ret = RSA_public_encrypt(data.second, data.first, out.data(), rsa, RSA_PKCS1_PADDING);
    if (ret<0) {
        ec = ABORT_ERR;
        failureCallback();
        return;
    }
    success(out);
}

void
CryptoAlgorithmRSAES_PKCS1_v1_5::platformDecrypt(const CryptoKeyRSA& key, const CryptoOperationData& data, VectorCallback success, VoidCallback failureCallback, ExceptionCode& ec)
{
    RSA* rsa = (RSA *)key.platformKey();
    Vector<uint8_t> out(RSA_size(rsa));
    int ret = RSA_private_decrypt(data.second, data.first, out.data(), rsa, RSA_PKCS1_PADDING);
    if (ret<0) {
        ec = ABORT_ERR;
        failureCallback();
        return;
    }
    success(out);
}

void
CryptoAlgorithmRSASSA_PKCS1_v1_5::platformSign(const CryptoAlgorithmRsaSsaParams& params, const CryptoKeyRSA& key, const CryptoOperationData& data, VectorCallback success, VoidCallback failureCallback, ExceptionCode& ec)
{
    RSA* rsa = (RSA *)key.platformKey();
    Vector<uint8_t> out(RSA_size(rsa));
    unsigned int len = 0;
    int ret = RSA_sign_ASN1_OCTET_STRING(0, data.first, data.second, out.data(), &len, rsa);
    if (ret<0) {
        ec = ABORT_ERR;
        failureCallback();
        return;
    }
    success(out);
}

void
CryptoAlgorithmRSASSA_PKCS1_v1_5::platformVerify(const CryptoAlgorithmRsaSsaParams& params, const CryptoKeyRSA& key, const CryptoOperationData& signature, const CryptoOperationData& data, BoolCallback success, VoidCallback failureCallback, ExceptionCode& ec)
{
    RSA* rsa = (RSA *)key.platformKey();
    int ret = RSA_verify_ASN1_OCTET_STRING(0, data.first, data.second, (unsigned char *)signature.first, signature.second, rsa);
    if (!ret) {
        ec = ABORT_ERR;
        failureCallback();
        return;
    }
    success(true);
}

void
CryptoAlgorithmRSA_OAEP::platformEncrypt(const CryptoAlgorithmRsaOaepParams& params, const CryptoKeyRSA& key, const CryptoOperationData& data, VectorCallback success, VoidCallback failureCallback, ExceptionCode& ec)
{
    RSA* rsa = (RSA *)key.platformKey();
    Vector<uint8_t> out(RSA_size(rsa));
    int ret = RSA_public_encrypt(data.second, data.first, out.data(), rsa, RSA_PKCS1_OAEP_PADDING);
    if (ret<0) {
        ec = ABORT_ERR;
        failureCallback();
        return;
    }
    success(out);
}

void
CryptoAlgorithmRSA_OAEP::platformDecrypt(const CryptoAlgorithmRsaOaepParams& params, const CryptoKeyRSA& key, const CryptoOperationData& data, VectorCallback success, VoidCallback failureCallback, ExceptionCode& ec)
{
    RSA* rsa = (RSA *)key.platformKey();
    Vector<uint8_t> out(RSA_size(rsa));
    int ret = RSA_private_decrypt(data.second, data.first, out.data(), rsa, RSA_PKCS1_OAEP_PADDING);
    if (ret<0) {
        ec = ABORT_ERR;
        failureCallback();
        return;
    }
    out.shrink(ret);
    success(out);
}

class CryptoDigestContext
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    CryptoDigestContext(int type)
        : m_type(type)
        , m_input(0)
        , m_size(0)
    {}
    ~CryptoDigestContext()
    {}

    int m_type;
    const void* m_input;
    size_t m_size;
};

CryptoDigest::CryptoDigest()
    : m_context(nullptr)
{
}

CryptoDigest::~CryptoDigest()
{
}

std::unique_ptr<CryptoDigest>
CryptoDigest::create(CryptoAlgorithmIdentifier identifier)
{
    CryptoDigest* digest = new CryptoDigest();
    digest->m_context = std::make_unique<CryptoDigestContext>((int)identifier);

    return std::unique_ptr<CryptoDigest>(digest);
}

void
CryptoDigest::addBytes(const void* input, size_t length)
{
    const uint8_t* p = (const uint8_t *)input;
    m_context->m_input = p;
    m_context->m_size = length;
}

Vector<uint8_t>
CryptoDigest::computeHash()
{
    uint8_t m[128];
    Vector<uint8_t> ret;

    switch ((CryptoAlgorithmIdentifier)m_context->m_type) {
    case CryptoAlgorithmIdentifier::SHA_1:
    {
        SHA_CTX ctx;
        SHA1_Init(&ctx);
        SHA1_Update(&ctx, m_context->m_input, m_context->m_size);
        SHA1_Final(m, &ctx);
        OPENSSL_cleanse(&ctx, sizeof(SHA_CTX));
        ret.append(m, SHA_DIGEST_LENGTH);
        break;
    }
    case CryptoAlgorithmIdentifier::SHA_224:
    {
        SHA256_CTX ctx;
        SHA224_Init(&ctx);
        SHA224_Update(&ctx, m_context->m_input, m_context->m_size);
        SHA224_Final(m, &ctx);
        OPENSSL_cleanse(&ctx, sizeof(SHA256_CTX));
        ret.append(m, SHA224_DIGEST_LENGTH);
        break;
    }
    case CryptoAlgorithmIdentifier::SHA_256:
    {
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, m_context->m_input, m_context->m_size);
        SHA256_Final(m, &ctx);
        OPENSSL_cleanse(&ctx, sizeof(SHA256_CTX));
        ret.append(m, SHA256_DIGEST_LENGTH);
        break;
    }
    case CryptoAlgorithmIdentifier::SHA_384:
    {
        SHA512_CTX ctx;
        SHA384_Init(&ctx);
        SHA384_Update(&ctx, m_context->m_input, m_context->m_size);
        SHA384_Final(m, &ctx);
        OPENSSL_cleanse(&ctx, sizeof(SHA512_CTX));
        ret.append(m, SHA384_DIGEST_LENGTH);
        break;
    }
    case CryptoAlgorithmIdentifier::SHA_512:
    {
        SHA512_CTX ctx;
        SHA512_Init(&ctx);
        SHA512_Update(&ctx, m_context->m_input, m_context->m_size);
        SHA512_Final(m, &ctx);
        OPENSSL_cleanse(&ctx, sizeof(SHA512_CTX));
        ret.append(m, SHA512_DIGEST_LENGTH);
        break;
    }
    default:
        break;
    }
    return ret;
}

void
getSupportedKeySizes(Vector<String>& ret)
{
    ret.append("1024");
    ret.append("2048");
}

} // namespace
#endif

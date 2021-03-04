/*
 * Copyright (c) 2015-2020 ACCESS CO., LTD. All rights reserved.
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

#include "CryptoAlgorithmRegistry.h"

#include "CryptoKeyAES.h"
#include "CryptoKeyEC.h"
#include "CryptoKeyHMAC.h"
#include "CryptoKeyRSA.h"
#include "CryptoKeyRSAComponents.h"

#include "CryptoAlgorithmAES_CBC.h"
#include "CryptoAlgorithmAES_CFB.h"
#include "CryptoAlgorithmAES_CTR.h"
#include "CryptoAlgorithmAES_GCM.h"
#include "CryptoAlgorithmAES_KW.h"
#include "CryptoAlgorithmECDH.h"
#include "CryptoAlgorithmECDSA.h"
#include "CryptoAlgorithmHKDF.h"
#include "CryptoAlgorithmHMAC.h"
#include "CryptoAlgorithmPBKDF2.h"
#include "CryptoAlgorithmRSA_OAEP.h"
#include "CryptoAlgorithmRSA_PSS.h"
#include "CryptoAlgorithmRSAES_PKCS1_v1_5.h"
#include "CryptoAlgorithmRSASSA_PKCS1_v1_5.h"
#include "CryptoAlgorithmSHA1.h"
#include "CryptoAlgorithmSHA224.h"
#include "CryptoAlgorithmSHA256.h"
#include "CryptoAlgorithmSHA384.h"
#include "CryptoAlgorithmSHA512.h"

#include "CryptoAlgorithmAesCbcCfbParams.h"
#include "CryptoAlgorithmAesGcmParams.h"

#include <pal/crypto/CryptoDigest.h>
#include "CryptoKeyPair.h"

#include "ExceptionCode.h"

#include "NotImplemented.h"
#include "ScriptExecutionContext.h"

#include <openssl/aes.h>
#include <openssl/bn.h>
#include <openssl/hmac.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

namespace WebCore {

#if ENABLE(SUBTLE_CRYPTO)

CryptoKeyRSA::CryptoKeyRSA(CryptoAlgorithmIdentifier identifier, CryptoAlgorithmIdentifier hash, bool hasHash, CryptoKeyType type, PlatformRSAKey platformKey, bool extractable, CryptoKeyUsageBitmap usage)
    : CryptoKey(identifier, type, extractable, usage)
    , m_platformKey(platformKey)
    , m_restrictedToSpecificHash(hasHash)
    , m_hash(hash)
{
}

CryptoKeyRSA::~CryptoKeyRSA()
{
    if (m_platformKey) {
        RSA_free((RSA *)m_platformKey);
    }
}

RefPtr<CryptoKeyRSA>
CryptoKeyRSA::create(CryptoAlgorithmIdentifier identifier, CryptoAlgorithmIdentifier hash, bool hasHash, const CryptoKeyRSAComponents& obj, bool extractable, CryptoKeyUsageBitmap usage)
{
    CryptoKeyType type = obj.type()==CryptoKeyRSAComponents::Type::Public ? CryptoKeyType::Public : CryptoKeyType::Private;

    RSA* rsa = 0;
    BIGNUM* modu = 0;
    BIGNUM* expo = 0;
    BIGNUM* pexp = 0;
    BIGNUM* pp = 0;
    BIGNUM* pq = 0;
    BIGNUM* dmp1 = 0;
    BIGNUM* dmq1 = 0;
    BIGNUM* iqmp = 0;
    BIGNUM* one = 0;
    BN_CTX* ctx = 0;

    rsa = RSA_new();
    if (!rsa)
        goto error_end;

    if (type==CryptoKeyType::Private) {
        pexp = BN_bin2bn(obj.privateExponent().data(), obj.privateExponent().size(), 0);
        pp = BN_bin2bn(obj.firstPrimeInfo().primeFactor.data(), obj.firstPrimeInfo().primeFactor.size(), 0);
        pq = BN_bin2bn(obj.secondPrimeInfo().primeFactor.data(), obj.secondPrimeInfo().primeFactor.size(), 0);
        if (!pexp || !pp || !pq || !RSA_set0_factors(rsa, pp, pq)) {
            BN_clear_free(pp);
            BN_clear_free(pq);
            BN_clear_free(pexp);
            goto error_end;
        }
    }

    modu = BN_bin2bn(obj.modulus().data(), obj.modulus().size(), 0);
    expo = BN_bin2bn(obj.exponent().data(), obj.exponent().size(), 0);

    if (!modu || !expo || !RSA_set0_key(rsa, modu, expo, pexp)) {
        BN_free(modu);
        BN_free(expo);
        BN_clear_free(pexp);
        goto error_end;
    }

    if (type==CryptoKeyType::Private) {
        one = BN_new();
        if (!one)
            goto error_end;
        BN_set_word(one, 1);
        dmp1 = BN_new();
        dmq1 = BN_new();
        iqmp = BN_new();
        if (!dmp1 || !dmq1 || !iqmp || !RSA_set0_crt_params(rsa, dmp1, dmq1, iqmp)) {
            BN_clear_free(dmp1);
            BN_clear_free(dmq1);
            BN_clear_free(iqmp);
            goto error_end;
        }
        ctx = BN_CTX_new();
        if (!ctx)
            goto error_end;
        BN_mod_sub(dmp1, pexp, pp, one, ctx);
        BN_mod_sub(dmq1, pexp, pq, one, ctx);
        BN_sqr(one, pq, ctx);
        BN_mod(iqmp, one, pp, ctx);
        BN_free(one);
        BN_CTX_free(ctx);
    }

    return adoptRef(new CryptoKeyRSA(identifier, hash, hasHash, type, (void *)rsa, extractable, usage));

error_end:
    if (one)
        BN_free(one);
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

size_t
CryptoKeyRSA::keySizeInBits() const
{
    RSA* rsa = (RSA *)m_platformKey;
    return BN_num_bits(RSA_get0_e(rsa));
}

std::unique_ptr<CryptoKeyRSAComponents>
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
        return CryptoKeyRSAComponents::createPublic(modulus, exponent);
    } else {
        // Private key
        Vector<uint8_t> privateExponent(BN_num_bytes(d));
        return CryptoKeyRSAComponents::createPrivate(modulus, exponent, privateExponent);
    }
}

void
CryptoKeyRSA::generatePair(CryptoAlgorithmIdentifier identifier, CryptoAlgorithmIdentifier hash, bool hasHash, unsigned modulusLength, const Vector<uint8_t>& publicExponent, bool extractable, CryptoKeyUsageBitmap usage, KeyPairCallback&& success, VoidCallback&& failureCallback, ScriptExecutionContext* context)
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
        KeyPairCallback* localSuccess = new KeyPairCallback(WTFMove(success));
        context->ref();
        context->postTask([identifier, hash, hasHash, pub, pri, extractable, usage, localSuccess](ScriptExecutionContext& context)
        {
            auto pubk = CryptoKeyRSA::create(identifier, hash, hasHash, CryptoKeyType::Public, (void *)pub, extractable, usage);
            auto prik = CryptoKeyRSA::create(identifier, hash, hasHash, CryptoKeyType::Private, (void *)pri, extractable, usage);
            (*localSuccess)(CryptoKeyPair{ WTFMove(pubk), WTFMove(prik) });

            delete localSuccess;
            context.deref();
        });
        return;
    }

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

RefPtr<CryptoKeyRSA>
CryptoKeyRSA::importSpki(CryptoAlgorithmIdentifier identifier, std::optional<CryptoAlgorithmIdentifier> hash, Vector<uint8_t>&& keyData, bool extractable, CryptoKeyUsageBitmap usages)
{
    notImplemented();
    return nullptr;
}

RefPtr<CryptoKeyRSA>
CryptoKeyRSA::importPkcs8(CryptoAlgorithmIdentifier identifier, std::optional<CryptoAlgorithmIdentifier> hash, Vector<uint8_t>&& keyData, bool extractable, CryptoKeyUsageBitmap usages)
{
    notImplemented();
    return nullptr;
}

ExceptionOr<Vector<uint8_t>>
CryptoKeyRSA::exportSpki() const
{
    notImplemented();
    return Exception{ NotSupportedError };
}

ExceptionOr<Vector<uint8_t>>
CryptoKeyRSA::exportPkcs8() const
{
    notImplemented();
    return Exception{ NotSupportedError };
}

CryptoKey::KeyAlgorithm
CryptoKeyRSA::algorithm() const
{
    notImplemented();
    return CryptoKeyAlgorithm();
}

// need to implement to pratform specified "CryproKeyEC::create()"

CryptoKeyEC::~CryptoKeyEC()
{
    notImplemented();
}

size_t
CryptoKeyEC::keySizeInBits() const
{
    notImplemented();
    return 0;
}

bool
CryptoKeyEC::platformSupportedCurve(NamedCurve curve)
{
    notImplemented();
    return false;
}

std::optional<CryptoKeyPair>
CryptoKeyEC::platformGeneratePair(CryptoAlgorithmIdentifier identifier, NamedCurve curve, bool extractable, CryptoKeyUsageBitmap usages)
{
    notImplemented();
    return std::nullopt;
}

RefPtr<CryptoKeyEC>
CryptoKeyEC::platformImportRaw(CryptoAlgorithmIdentifier identifier, NamedCurve curve, Vector<uint8_t>&& keyData, bool extractable, CryptoKeyUsageBitmap usages)
{
    notImplemented();
    return nullptr;
}

RefPtr<CryptoKeyEC>
CryptoKeyEC::platformImportJWKPublic(CryptoAlgorithmIdentifier identifier, NamedCurve curve, Vector<uint8_t>&& x, Vector<uint8_t>&& y, bool extractable, CryptoKeyUsageBitmap usages)
{
    notImplemented();
    return nullptr;
}

RefPtr<CryptoKeyEC>
CryptoKeyEC::platformImportJWKPrivate(CryptoAlgorithmIdentifier identifier, NamedCurve curve, Vector<uint8_t>&& x, Vector<uint8_t>&& y, Vector<uint8_t>&& d, bool extractable, CryptoKeyUsageBitmap usages)
{
    notImplemented();
    return nullptr;
}

RefPtr<CryptoKeyEC>
CryptoKeyEC::platformImportSpki(CryptoAlgorithmIdentifier identifier, NamedCurve curve, Vector<uint8_t>&& keyData, bool extractable, CryptoKeyUsageBitmap usages)
{
    notImplemented();
    return nullptr;
}

RefPtr<CryptoKeyEC>
CryptoKeyEC::platformImportPkcs8(CryptoAlgorithmIdentifier, NamedCurve, Vector<uint8_t>&&, bool, CryptoKeyUsageBitmap)
{
    notImplemented();
    return nullptr;
}

Vector<uint8_t>
CryptoKeyEC::platformExportRaw() const
{
    notImplemented();
    return { };
}

bool
CryptoKeyEC::platformAddFieldElements(JsonWebKey& keyData) const
{
    notImplemented();
    return false;
}

Vector<uint8_t>
CryptoKeyEC::platformExportSpki() const
{
    notImplemented();
    return { };
}

Vector<uint8_t>
CryptoKeyEC::platformExportPkcs8() const
{
    notImplemented();
    return { };
}

void
CryptoAlgorithmRegistry::platformRegisterAlgorithms(void)
{
    registerAlgorithm<CryptoAlgorithmAES_CBC>();
    registerAlgorithm<CryptoAlgorithmAES_GCM>();
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

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmAES_CBC::platformEncrypt(const CryptoAlgorithmAesCbcCfbParams& params, const CryptoKeyAES& key, const Vector<uint8_t>& data)
{
    EVP_CIPHER_CTX* ctx = nullptr;
    const EVP_CIPHER* cipher = nullptr;

    size_t buflen = ((data.size() + AES_BLOCK_SIZE) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
    Vector<uint8_t> ciphertext_buf(buflen);

    unsigned char* ciphertext = ciphertext_buf.data();
    int ciphertext_len = 0;

    const unsigned char* plaintext = data.data();
    int plaintext_len = data.size();

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

    if (1 != EVP_EncryptInit_ex(ctx, cipher, nullptr, key.key().data(), reinterpret_cast<const unsigned char *>(params.ivVector().data())))
        goto error;

    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        goto error;

    ciphertext_len += len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        goto error;

    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    ciphertext_buf.shrink(ciphertext_len);

    return WTFMove(ciphertext_buf);

error:
    if (ctx)
        EVP_CIPHER_CTX_free(ctx);

    return Exception { OperationError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmAES_CBC::platformDecrypt(const CryptoAlgorithmAesCbcCfbParams& params, const CryptoKeyAES& key, const Vector<uint8_t>& data)
{
    EVP_CIPHER_CTX* ctx = nullptr;
    const EVP_CIPHER* cipher = nullptr;

    Vector<uint8_t> plaintext_buf(data.size());

    unsigned char* plaintext = plaintext_buf.data();
    int plaintext_len = 0;

    const unsigned char* ciphertext = data.data();
    int ciphertext_len = data.size();

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

    if (1 != EVP_DecryptInit_ex(ctx, cipher, nullptr, key.key().data(), reinterpret_cast<const unsigned char *>(params.ivVector().data())))
        goto error;

    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        goto error;

    plaintext_len += len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        goto error;

    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    plaintext_buf.shrink(plaintext_len);

    return WTFMove(plaintext_buf);

error:
    if (ctx)
        EVP_CIPHER_CTX_free(ctx);

    return Exception { OperationError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmAES_CFB::platformEncrypt(const CryptoAlgorithmAesCbcCfbParams& params, const CryptoKeyAES& key, const Vector<uint8_t>& data)
{
    notImplemented();
    return Exception { NotSupportedError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmAES_CFB::platformDecrypt(const CryptoAlgorithmAesCbcCfbParams& params, const CryptoKeyAES& key, const Vector<uint8_t>& data)
{
    notImplemented();
    return Exception { NotSupportedError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmAES_CTR::platformEncrypt(const CryptoAlgorithmAesCtrParams& params, const CryptoKeyAES& key, const Vector<uint8_t>& data)
{
    notImplemented();
    return Exception { NotSupportedError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmAES_CTR::platformDecrypt(const CryptoAlgorithmAesCtrParams& params, const CryptoKeyAES& key, const Vector<uint8_t>& data)
{
    notImplemented();
    return Exception { NotSupportedError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmAES_GCM::platformEncrypt(const CryptoAlgorithmAesGcmParams& params, const CryptoKeyAES& key, const Vector<uint8_t>& plainText)
{
    EVP_CIPHER_CTX* ctx = nullptr;
    const EVP_CIPHER* cipher = nullptr;

    Vector<uint8_t> cipherText(plainText.size());

    const Vector<uint8_t>& iv = params.ivVector();
    const Vector<uint8_t>& additionalData = params.additionalDataVector();

    uint8_t tagLength = params.tagLength.value_or(0) / 8;

    int len = 0;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        goto error;

    switch (key.key().size() * 8) {
    case 128:
        cipher = EVP_aes_128_gcm();
        break;
    case 192:
        cipher = EVP_aes_192_gcm();
        break;
    case 256:
        cipher = EVP_aes_256_gcm();
        break;
    default:
        goto error;
    }

    // Select cipher
    if (1 != EVP_EncryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr))
        goto error;

    // Set IV length.
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, iv.size(), nullptr))
        goto error;

    // Use the given key and IV.
    if (1 != EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.key().data(), iv.data()))
        goto error;

    // Use the given additional authenticated data, if any.
    if (!additionalData.isEmpty()) {
        if (1 != EVP_EncryptUpdate(ctx, nullptr, &len, additionalData.data(), additionalData.size()))
            goto error;
    }

    // Perform the encryption.
    if (1 != EVP_EncryptUpdate(ctx, cipherText.data(), &len, plainText.data(), plainText.size()))
        goto error;

    // Finalise: get no output for GCM
    if (1 != EVP_EncryptFinal_ex(ctx, nullptr, &len))
        goto error;

    // Get the tag data and append it to cipherText.
    if (tagLength) {
        Vector<uint8_t> tag(tagLength);
        if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, tagLength, tag.data()))
            goto error;
        cipherText.appendVector(tag);
    }

    EVP_CIPHER_CTX_free(ctx);

    return WTFMove(cipherText);

error:
    if (ctx)
        EVP_CIPHER_CTX_free(ctx);

    return Exception { OperationError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmAES_GCM::platformDecrypt(const CryptoAlgorithmAesGcmParams& params, const CryptoKeyAES& key, const Vector<uint8_t>& cipherText)
{
    EVP_CIPHER_CTX* ctx = nullptr;
    const EVP_CIPHER* cipher = nullptr;

    uint8_t tagLength = params.tagLength.value_or(0) / 8;

    size_t cipherLength = cipherText.size() - tagLength;
    Vector<uint8_t> plainText(cipherLength);

    const Vector<uint8_t>& iv = params.ivVector();
    const Vector<uint8_t>& additionalData = params.additionalDataVector();

    int len = 0;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        goto error;

    switch (key.key().size() * 8) {
    case 128:
        cipher = EVP_aes_128_gcm();
        break;
    case 192:
        cipher = EVP_aes_192_gcm();
        break;
    case 256:
        cipher = EVP_aes_256_gcm();
        break;
    default:
        goto error;
    }

    // Select cipher
    if (1 != EVP_DecryptInit_ex(ctx, cipher, nullptr, nullptr, nullptr))
        goto error;

    // Set IV length.
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, iv.size(), nullptr))
        goto error;

    // Use the given key and IV.
    if (1 != EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.key().data(), iv.data()))
        goto error;

    // Use the given additional authenticated data, if any.
    if (!additionalData.isEmpty()) {
        if (1 != EVP_DecryptUpdate(ctx, nullptr, &len, additionalData.data(), additionalData.size()))
            goto error;
    }

    // Perform the decryption.
    if (1 != EVP_DecryptUpdate(ctx, plainText.data(), &len, cipherText.data(), cipherLength))
        goto error;

    // Set the expected tag data.
    if (tagLength) {
        if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG, tagLength, const_cast<Vector<uint8_t>&>(cipherText).data() + cipherLength))
            goto error;
    }

    // Finalise:
    //  - verify the tag data
    //  - get no output for GCM
    if (1 != EVP_DecryptFinal_ex(ctx, nullptr, &len))
        goto error;

    EVP_CIPHER_CTX_free(ctx);

    return WTFMove(plainText);

error:
    if (ctx)
        EVP_CIPHER_CTX_free(ctx);

    return Exception { OperationError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmAES_KW::platformWrapKey(const CryptoKeyAES& key, const Vector<uint8_t>& data)
{
    notImplemented();
    return Exception { NotSupportedError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmAES_KW::platformUnwrapKey(const CryptoKeyAES& key, const Vector<uint8_t>& data)
{
    notImplemented();
    return Exception { NotSupportedError };
}

std::optional<Vector<uint8_t>>
CryptoAlgorithmECDH::platformDeriveBits(const CryptoKeyEC& baseKey, const CryptoKeyEC& publicKey)
{
    notImplemented();
    return std::nullopt;
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmECDSA::platformSign(const CryptoAlgorithmEcdsaParams& params, const CryptoKeyEC& key, const Vector<uint8_t>& data)
{
    notImplemented();
    return Exception { NotSupportedError };
}

ExceptionOr<bool>
CryptoAlgorithmECDSA::platformVerify(const CryptoAlgorithmEcdsaParams& param, const CryptoKeyEC& key, const Vector<uint8_t>& signature, const Vector<uint8_t>& data)
{
    notImplemented();
    return Exception { NotSupportedError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmHKDF::platformDeriveBits(const CryptoAlgorithmHkdfParams& param, const CryptoKeyRaw& key, size_t length)
{
    notImplemented();
    return Exception { NotSupportedError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmHMAC::platformSign(const CryptoKeyHMAC& key, const Vector<uint8_t>& data)
{
    CryptoAlgorithmIdentifier algorithm = key.hashAlgorithmIdentifier();
    const EVP_MD* md;
    switch (algorithm) {
    case CryptoAlgorithmIdentifier::SHA_1:
        md = EVP_sha1();
        break;
    case CryptoAlgorithmIdentifier::SHA_224:
        md = EVP_sha224();
        break;
    case CryptoAlgorithmIdentifier::SHA_256:
        md = EVP_sha256();
        break;
    case CryptoAlgorithmIdentifier::SHA_384:
        md = EVP_sha384();
        break;
    case CryptoAlgorithmIdentifier::SHA_512:
        md = EVP_sha512();
        break;
    default:
        md = nullptr;
    }
    if (!md) {
        return Exception { NotSupportedError };
    }
    unsigned int len = 0;
    unsigned char* ret = HMAC(md, key.key().data(), key.key().size(), data.data(), data.size(), 0, &len);
    if (!ret) {
        return Exception { OperationError };
    }
    Vector<uint8_t> result;
    result.append(ret, len);
    return result;
}

ExceptionOr<bool>
CryptoAlgorithmHMAC::platformVerify(const CryptoKeyHMAC& key, const Vector<uint8_t>& signature, const Vector<uint8_t>& data)
{
    CryptoAlgorithmIdentifier algorithm = key.hashAlgorithmIdentifier();
    const EVP_MD* md;
    switch (algorithm) {
    case CryptoAlgorithmIdentifier::SHA_1:
        md = EVP_sha1();
        break;
    case CryptoAlgorithmIdentifier::SHA_224:
        md = EVP_sha224();
        break;
    case CryptoAlgorithmIdentifier::SHA_256:
        md = EVP_sha256();
        break;
    case CryptoAlgorithmIdentifier::SHA_384:
        md = EVP_sha384();
        break;
    case CryptoAlgorithmIdentifier::SHA_512:
        md = EVP_sha512();
        break;
    default:
        md = nullptr;
    }
    if (!md) {
        return Exception{ NotSupportedError };
    }
    unsigned int len=0;
    unsigned char buf[EVP_MAX_MD_SIZE];
    int ret = 0;

    HMAC_CTX* ctx = HMAC_CTX_new();
    if (!ctx)
        goto error_end;
    HMAC_CTX_reset(ctx);
    ret = HMAC_Init_ex(ctx, key.key().data(), key.key().size(), md, NULL);
    if (!ret)
        goto error_end;
    ret = HMAC_Update(ctx, data.data(), data.size());
    if (!ret)
        goto error_end;
    ret = HMAC_Final(ctx, buf, &len);
    if (!ret)
        goto error_end;
    HMAC_CTX_free(ctx);
    return true;

error_end:
    if (ctx)
        HMAC_CTX_free(ctx);
    return Exception { OperationError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmPBKDF2::platformDeriveBits(const CryptoAlgorithmPbkdf2Params& param, const CryptoKeyRaw& key, size_t length)
{
    notImplemented();
    return Exception { NotSupportedError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmRSA_OAEP::platformEncrypt(const CryptoAlgorithmRsaOaepParams& param, const CryptoKeyRSA& key, const Vector<uint8_t>& data)
{
    notImplemented();
    return Exception { NotSupportedError };
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmRSA_OAEP::platformDecrypt(const CryptoAlgorithmRsaOaepParams& param, const CryptoKeyRSA& key, const Vector<uint8_t>& data)
{
    notImplemented();
    return Exception { NotSupportedError };
}

#if HAVE(RSA_PSS)
ExceptionOr<Vector<uint8_t>>
platformSign(CryptoAlgorithmRsaPssParams& param, const CryptoKeyRSA& key, const Vector<uint8_t>& data)
{
    notImplemented();
    return Exception { NotSupportedError };
}

ExceptionOr<bool>
platformVerify(CryptoAlgorithmRsaPssParams& param, const CryptoKeyRSA& key, const Vector<uint8_t>& signature, const Vector<uint8_t>& data)
{
    notImplemented();
    return Exception { NotSupportedError };
}
#endif

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmRSAES_PKCS1_v1_5::platformEncrypt(const CryptoKeyRSA& key, const Vector<uint8_t>& data)
{
    RSA* rsa = (RSA *)key.platformKey();
    Vector<uint8_t> out(RSA_size(rsa));
    int ret = RSA_public_encrypt(data.size(), data.data(), out.data(), rsa, RSA_PKCS1_PADDING);
    if (ret<0) {
        return Exception { OperationError };
    }
    return out;
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmRSAES_PKCS1_v1_5::platformDecrypt(const CryptoKeyRSA& key, const Vector<uint8_t>& data)
{
    RSA* rsa = (RSA *)key.platformKey();
    Vector<uint8_t> out(RSA_size(rsa));
    int ret = RSA_private_decrypt(data.size(), data.data(), out.data(), rsa, RSA_PKCS1_PADDING);
    if (ret<0) {
        return Exception{ OperationError };
    }
    return out;
}

ExceptionOr<Vector<uint8_t>>
CryptoAlgorithmRSASSA_PKCS1_v1_5::platformSign(const CryptoKeyRSA& key, const Vector<uint8_t>& data)
{
    RSA* rsa = (RSA *)key.platformKey();
    Vector<uint8_t> out(RSA_size(rsa));
    unsigned int len = 0;
    int ret = RSA_sign_ASN1_OCTET_STRING(0, data.data(), data.size(), out.data(), &len, rsa);
    if (ret<0) {
        return Exception { OperationError };
    }
    return out;
}

ExceptionOr<bool>
CryptoAlgorithmRSASSA_PKCS1_v1_5::platformVerify(const CryptoKeyRSA& key, const Vector<uint8_t>& signature, const Vector<uint8_t>& data)
{
    RSA* rsa = (RSA *)key.platformKey();
    int ret = RSA_verify_ASN1_OCTET_STRING(0, data.data(), data.size(), (unsigned char *)signature.data(), signature.size(), rsa);
    if (!ret) {
        return Exception{ OperationError };
    }
    return true;
}
#endif

void
getSupportedKeySizes(Vector<String>& ret)
{
    ret.append("1024");
    ret.append("2048");
}

} // namespace WebCore

namespace PAL {

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
CryptoDigest::create(Algorithm identifier)
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

    switch ((Algorithm)m_context->m_type) {
    case Algorithm::SHA_1:
    {
        SHA_CTX ctx;
        SHA1_Init(&ctx);
        SHA1_Update(&ctx, m_context->m_input, m_context->m_size);
        SHA1_Final(m, &ctx);
        OPENSSL_cleanse(&ctx, sizeof(SHA_CTX));
        ret.append(m, SHA_DIGEST_LENGTH);
        break;
    }
    case Algorithm::SHA_224:
    {
        SHA256_CTX ctx;
        SHA224_Init(&ctx);
        SHA224_Update(&ctx, m_context->m_input, m_context->m_size);
        SHA224_Final(m, &ctx);
        OPENSSL_cleanse(&ctx, sizeof(SHA256_CTX));
        ret.append(m, SHA224_DIGEST_LENGTH);
        break;
    }
    case Algorithm::SHA_256:
    {
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, m_context->m_input, m_context->m_size);
        SHA256_Final(m, &ctx);
        OPENSSL_cleanse(&ctx, sizeof(SHA256_CTX));
        ret.append(m, SHA256_DIGEST_LENGTH);
        break;
    }
    case Algorithm::SHA_384:
    {
        SHA512_CTX ctx;
        SHA384_Init(&ctx);
        SHA384_Update(&ctx, m_context->m_input, m_context->m_size);
        SHA384_Final(m, &ctx);
        OPENSSL_cleanse(&ctx, sizeof(SHA512_CTX));
        ret.append(m, SHA384_DIGEST_LENGTH);
        break;
    }
    case Algorithm::SHA_512:
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

} // namespace PAL

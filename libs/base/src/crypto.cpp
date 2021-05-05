//
// Created by Mpho Mbotho on 2020-10-17.
//

#include "suil/base/crypto.hpp"
#include "suil/base/base64.hpp"
#include "suil/base/logging.hpp"

#include <openssl/err.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
typedef struct ECDSA_SIG_st {
    BIGNUM *r;
    BIGNUM *s;
} ECDSA_SIG;

struct bignum_st {
    BN_ULONG *d;                /* Pointer to an array of 'BN_BITS2' bit
                                 * chunks. */
    int top;                    /* Index of last used d +1. */
    /* The next are internal book keeping for bn_expand. */
    int dmax;                   /* Size of the d array. */
    int neg;                    /* one if the number is negative */
    int flags;
};

static void BN_init(BIGNUM *a)
{
    memset(a, 0, sizeof(BIGNUM));
}
#endif


namespace suil::crypto {

    constexpr size_t EC_KEY_LEN{32};

#define _CRYPTO_ERROR(fn) CryptoError( #fn, "@", __LINE__, ERR_get_error() )

#define _CRYPTO_CALL(fn, ...) if ( fn (__VA_ARGS__ ) == 0 ) throw _CRYPTO_ERROR( fn )

    // Perform ECDSA key recovery (see SEC1 4.1.6) for curves over (mod p)-fields
    // recid selects which key is recovered
    // if check is non-zero, additional checks are performed
    static int ECDSA_SIG_recover_key_GFp(EC_KEY *eckey, ECDSA_SIG *ecsig, const unsigned char *msg, int msglen, int recid, int check)
    {
        if (!eckey) return 0;

        int ret = 0;
        BN_CTX *ctx = NULL;

        BIGNUM *x = NULL;
        BIGNUM *e = NULL;
        BIGNUM *order = NULL;
        BIGNUM *sor = NULL;
        BIGNUM *eor = NULL;
        BIGNUM *field = NULL;
        EC_POINT *R = NULL;
        EC_POINT *O = NULL;
        EC_POINT *Q = NULL;
        BIGNUM *rr = NULL;
        BIGNUM *zero = NULL;
        int n = 0;
        int i = recid / 2;

        const EC_GROUP *group = EC_KEY_get0_group(eckey);
        if ((ctx = BN_CTX_new()) == NULL) { ret = -1; goto err; }
        BN_CTX_start(ctx);
        order = BN_CTX_get(ctx);
        if (!EC_GROUP_get_order(group, order, ctx)) { ret = -2; goto err; }
        x = BN_CTX_get(ctx);
        if (!BN_copy(x, order)) { ret=-1; goto err; }
        if (!BN_mul_word(x, i)) { ret=-1; goto err; }
        if (!BN_add(x, x, ecsig->r)) { ret=-1; goto err; }
        field = BN_CTX_get(ctx);
        if (!EC_GROUP_get_curve_GFp(group, field, NULL, NULL, ctx)) { ret=-2; goto err; }
        if (BN_cmp(x, field) >= 0) { ret=0; goto err; }
        if ((R = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
        if (!EC_POINT_set_compressed_coordinates_GFp(group, R, x, recid % 2, ctx)) { ret=0; goto err; }
        if (check)
        {
            if ((O = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
            if (!EC_POINT_mul(group, O, NULL, R, order, ctx)) { ret=-2; goto err; }
            if (!EC_POINT_is_at_infinity(group, O)) { ret = 0; goto err; }
        }
        if ((Q = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
        n = EC_GROUP_get_degree(group);
        e = BN_CTX_get(ctx);
        if (!BN_bin2bn(msg, msglen, e)) { ret=-1; goto err; }
        if (8*msglen > n) BN_rshift(e, e, 8-(n & 7));
        zero = BN_CTX_get(ctx);
        if (!BN_zero(zero)) { ret=-1; goto err; }
        if (!BN_mod_sub(e, zero, e, order, ctx)) { ret=-1; goto err; }
        rr = BN_CTX_get(ctx);
        if (!BN_mod_inverse(rr, ecsig->r, order, ctx)) { ret=-1; goto err; }
        sor = BN_CTX_get(ctx);
        if (!BN_mod_mul(sor, ecsig->s, rr, order, ctx)) { ret=-1; goto err; }
        eor = BN_CTX_get(ctx);
        if (!BN_mod_mul(eor, e, rr, order, ctx)) { ret=-1; goto err; }
        if (!EC_POINT_mul(group, Q, eor, R, sor, ctx)) { ret=-2; goto err; }
        if (!EC_KEY_set_public_key(eckey, Q)) { ret=-2; goto err; }

        ret = 1;

        err:
        if (ctx) {
            BN_CTX_end(ctx);
            BN_CTX_free(ctx);
        }
        if (R != NULL) EC_POINT_free(R);
        if (O != NULL) EC_POINT_free(O);
        if (Q != NULL) EC_POINT_free(Q);
        return ret;
    }

    static bool b642bin(uint8_t* bin, size_t len, const void* data, size_t dlen) {
        try {
            auto out = Base64::decode(static_cast<const uint8_t *>(data), dlen);
            if (out.size() > len) {
                serror("b642bin - decode destination buffer %d %d", len, out.size());
            }
            memcpy(bin, out.data(), out.size());
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            serror("b642bin - %s", ex.what());
            return false;
        }
        return true;
    }
    template <typename T>
    static inline bool b642bin(uint8_t* bin, size_t len, const T& data) {
        return b642bin(bin, len, data.data(), data.size());
    }

    static bool hex2bin(uint8_t* bin, size_t len, const String& str)
    {
        try {
            suil::bytes(str, bin, len);
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            serror("hex2bin - %s", ex.what());
            return false;
        }
        return true;
    }

    void SHA256(SHA256Digest& digest, const void* data, size_t len)
    {
        SHA256_CTX ctx;
        _CRYPTO_CALL(SHA256_Init, &ctx);
        _CRYPTO_CALL(SHA256_Update, &ctx, data, len);
        _CRYPTO_CALL(SHA256_Final, &digest.bin(), &ctx);
    }

    void SHA512(SHA512Digest& digest, const void* data, size_t len)
    {
        SHA512_CTX ctx;
        _CRYPTO_CALL(SHA512_Init, &ctx);
        _CRYPTO_CALL(SHA512_Update, &ctx, data, len);
        _CRYPTO_CALL(SHA512_Final, &digest.bin(), &ctx);
    }

    void RIPEMD160(RIPEMD160Digest& digest, const void* data, size_t len)
    {
        RIPEMD160_CTX ctx;
        _CRYPTO_CALL(RIPEMD160_Init, &ctx);
        _CRYPTO_CALL(RIPEMD160_Update, &ctx, data, len);
        _CRYPTO_CALL(RIPEMD160_Final, &digest.bin(), &ctx);
    }

    void Hash256(Hash& hash, const void* data, size_t len)
    {
        SHA256Digest sha;
        crypto::SHA256(sha, data, len);
        crypto::SHA256(hash, &sha.bin(), sha.size());
    }

    void Hash160(HASH160Digest& hash, const void* data, size_t len)
    {
        SHA256Digest sha;
        crypto::SHA256(sha, data, len);
        crypto::RIPEMD160(hash, &sha.bin(), sha.size());
    }

    static suil::String computeBase58(const void* data, size_t len)
    {
        static const char base58Alphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

        auto bytes = static_cast<const uint8_t *>(data);
        size_t strLen;
        char *str;
        BN_CTX *ctx;
        BIGNUM *base, *x, *r;
        int i, j;

        strLen = len * 138 / 100 + 2;
        str = (char *)calloc(strLen, sizeof(char));

        ctx = BN_CTX_new();
        BN_CTX_start(ctx);

        base = BN_new();
        x = BN_new();
        r = BN_new();
        BN_set_word(base, 58);
        BN_bin2bn(bytes, len, x);

        i = 0;
        while (!BN_is_zero(x)) {
            BN_div(x, r, x, base, ctx);
            str[i] = base58Alphabet[BN_get_word(r)];
            ++i;
        }
        for (j = 0; j < len; ++j) {
            if (bytes[j] != 0x00) {
                break;
            }
            str[i] = base58Alphabet[0];
            ++i;
        }
        suil::reverse(str, i);

        BN_clear_free(r);
        BN_clear_free(x);
        BN_free(base);
        BN_CTX_end(ctx);
        BN_CTX_free(ctx);

        return suil::String{str, strlen(str), true};
    }

    suil::String toBase58(const void* data, size_t len, bool csum)
    {
        if (csum) {
            Hash hash;
            crypto::Hash256(hash, data, len);
            auto csumed = malloc(len+4);
            auto csumptr = &((uint8_t*)csumed)[len];
            memcpy(csumed, data, len);
            memcpy(csumptr, &hash.bin(), 4);
            auto tmp = computeBase58(csumed, len+4);
            free(csumed);
            return tmp;
        }
        else {
            return computeBase58(data, len);
        }
    }

    bool PrivateKey::fromString(PrivateKey &key, const suil::String &str)
    {
        return key.loadString(str);
    }

    PrivateKey PrivateKey::fromString(const String& str) {
        PrivateKey key;
        if (!fromString(key, str))
            throw CryptoError("decoding private key string failed, {size = ", str.size(), "}");
        return key;
    }

    bool PublicKey::fromString(PublicKey &key, const suil::String &str) {
        return key.loadString(str);
    }

    PublicKey PublicKey::fromString(const String& str) {
        PublicKey key;
        if (!fromString(key, str))
            throw CryptoError("decoding public key string failed {size =", str.size(), "}");
        return key;
    }

    ECKey::ECKey(EC_KEY *key)
            : ecKey(key)
    {}

    ECKey::ECKey(suil::crypto::ECKey &&other) noexcept
            : ecKey(other.ecKey)
    {
        other.ecKey = nullptr;
        if (!other.privKey.isnil())
            Ego.privKey.copyfrom(other.privKey);
        if (!other.pubKey.isnil())
            Ego.pubKey.copyfrom(other.pubKey);
    }

    ECKey& ECKey::operator=(suil::crypto::ECKey &&other) noexcept
    {
        Ego.ecKey = other.ecKey;
        other.ecKey = nullptr;
        if (!other.privKey.isnil())
            Ego.privKey.copyfrom(other.privKey);
        if (!other.pubKey.isnil())
            Ego.pubKey.copyfrom(other.pubKey);
        return Ego;
    }

    ECKey::~ECKey() {
        if (Ego.ecKey) {
            EC_KEY_free(Ego.ecKey);
            Ego.ecKey = nullptr;
        }
    }

    ECKey ECKey::generate()
    {
        EC_KEY *key{nullptr};
        key = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (key == nullptr) {
            serror("generate private key failed: %d %d", __LINE__, ERR_get_error());
            return {};
        }

        if (EC_KEY_generate_key(key) != 1) {
            serror("generate private key failed: %d %d", __LINE__, ERR_get_error());
            EC_KEY_free(key);
            return {};
        }

        auto privateKey = EC_KEY_get0_private_key(key);
        auto keyLen = (size_t) BN_num_bytes(privateKey);
        if (keyLen != EC_KEY_LEN) {
            serror("generate private key failed '%d': %d %d", keyLen, __LINE__, ERR_get_error());
            EC_KEY_free(key);
            return {};
        }
        // default conversion form is compressed
        EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);
        auto len = i2o_ECPublicKey(key, nullptr);
        if (len != EC_COMPRESSED_PUBLIC_KEY_SIZE) {
            serror("generate private key failed '%d': %d", len, __LINE__);
            EC_KEY_free(key);
            return {};
        }

        ECKey tmp{key};
        auto dst = &tmp.pubKey.bin();
        i2o_ECPublicKey(key, &dst);
        BN_bn2bin(privateKey, &tmp.privKey.bin());
        return tmp;
    }

    bool ECKey::isValid() const {
        return Ego.ecKey != nullptr;
    }

    const PrivateKey& ECKey::getPrivateKey() const
    {
        return Ego.privKey;
    }

    const PublicKey& ECKey::getPublicKey() const {
        return Ego.pubKey;
    }

    static bool key2pub(PublicKey& pub, EC_KEY *key)
    {
        // default conversion form is compressed
        EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);
        auto len = i2o_ECPublicKey(key, nullptr);
        if (len != EC_COMPRESSED_PUBLIC_KEY_SIZE) {
            serror("get public key failed '%d': %d", len, __LINE__);
            return false;
        }

        auto dst = &pub.bin();
        i2o_ECPublicKey(key, &dst);

        return true;
    }

    EC_KEY* PublicKey::pub2key(const PublicKey& pub)
    {
        EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (key == nullptr) {
            return nullptr;
        }
        EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);
        auto bin = &pub.bin();
        if (o2i_ECPublicKey(&key, &bin, pub.size()) == nullptr) {
            EC_KEY_free(key);
            return nullptr;
        }
        return key;
    }

    ECKey ECKey::fromKey(const suil::crypto::PrivateKey &priv)
    {
        auto key = EC_KEY_new_by_curve_name(NID_secp256k1);
        if (key == nullptr) {
            serror("load private key failure %d %d", __LINE__, ERR_get_error());
            return {};
        }

        auto privateKey = BN_bin2bn(&priv.bin(), priv.size(), nullptr);
        if (privateKey == nullptr) {
            serror("load private key failure %d %d", __LINE__, ERR_get_error());
            EC_KEY_free(key);
            return {};
        }

        if (!EC_KEY_set_private_key(key, privateKey)) {
            serror("load private key failure %d %d", __LINE__, ERR_get_error());
            BN_clear_free(privateKey);
            EC_KEY_free(key);
            return {};
        }

        auto ctx = BN_CTX_new();
        if (ctx == nullptr) {
            serror("load private key failure %d %d", __LINE__, ERR_get_error());
            BN_clear_free(privateKey);
            EC_KEY_free(key);
            return {};
        }
        BN_CTX_start(ctx);

        auto group = EC_KEY_get0_group(key);
        auto publicKey = EC_POINT_new(group);
        if (publicKey == nullptr) {
            serror("load private key failure %d %d", __LINE__, ERR_get_error());
            BN_CTX_end(ctx);
            BN_CTX_free(ctx);
            BN_clear_free(privateKey);
            EC_KEY_free(key);
            return {};
        }

        if (!EC_POINT_mul(group, publicKey, privateKey, nullptr, nullptr, ctx)) {
            serror("load private key failure %d %d", __LINE__, ERR_get_error());
            EC_POINT_free(publicKey);
            BN_CTX_end(ctx);
            BN_CTX_free(ctx);
            BN_clear_free(privateKey);
            EC_KEY_free(key);
            return {};
        }

        if (!EC_KEY_set_public_key(key, publicKey)) {
            serror("load private key failure %d %d", __LINE__, ERR_get_error());
            EC_POINT_free(publicKey);
            BN_CTX_end(ctx);
            BN_CTX_free(ctx);
            BN_clear_free(privateKey);
            EC_KEY_free(key);
            return {};
        }

        // default conversion form is compressed
        EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED);
        auto len = i2o_ECPublicKey(key, nullptr);
        if (len != EC_COMPRESSED_PUBLIC_KEY_SIZE) {
            serror("load private key failure '%d': %d", __LINE__);
            EC_POINT_free(publicKey);
            BN_CTX_end(ctx);
            BN_CTX_free(ctx);
            BN_clear_free(privateKey);
            EC_KEY_free(key);
            return {};
        }

        ECKey data(key);
        auto dst = &data.pubKey.bin();
        i2o_ECPublicKey(key, &dst);
        BN_bn2bin(privateKey, &data.privKey.bin());

        EC_POINT_free(publicKey);
        BN_CTX_end(ctx);
        BN_CTX_free(ctx);
        BN_clear_free(privateKey);

        return data;
    }

    suil::String ECDSASignature::toCompactForm(bool b64) const
    {
        auto b = &Ego.bin();
        auto sig= d2i_ECDSA_SIG(nullptr, &b, Ego.size());
        if (sig == nullptr) {
            serror("saving signature failed: %d %d", __LINE__, ERR_get_error());
            return {};
        }
        uint8_t compact[ECDSA_COMPACT_SIGNATURE_SIZE] = {0};
        compact[0] = (uint8_t)(Ego.RecId+31);
        auto sz = BN_num_bytes(sig->r);
        if (sz > 32) {
            serror("saving signature failed '%d': %d %d", sz, __LINE__, ERR_get_error());
            ECDSA_SIG_free(sig);
            return {};
        }
        // big endian stuff
        BN_bn2bin(sig->r, &compact[33-sz]);

        sz = BN_num_bytes(sig->s);
        if (sz > 32) {
            serror("saving signature failed '%d': %d %d", sz, __LINE__, ERR_get_error());
            ECDSA_SIG_free(sig);
            return {};
        }

        // big endian stuff
        BN_bn2bin(sig->s, &compact[ECDSA_COMPACT_SIGNATURE_SIZE-sz]);
        ECDSA_SIG_free(sig);

        if (b64) {
            return Base64::encode(compact, sizeof(compact));
        }
        else {
            return suil::hexstr(compact, sizeof(compact));
        }
    }

    ECDSASignature ECDSASignature::fromCompactForm(const suil::String &sig, bool b64)
    {
        uint8_t compact[ECDSA_COMPACT_SIGNATURE_SIZE] = {0};
        if (b64) {
            if (!b642bin(compact, sizeof(compact), sig)) {
                serror("loading signature failed: %d", __LINE__);
                return {};
            }
        } else {
            if (!hex2bin(compact, sizeof(compact), sig)) {
                serror("loading signature failed: %d", __LINE__);
                return {};
            }
        }

        BIGNUM r{}, s{};
        BN_init(&r); BN_init(&s);
        ECDSA_SIG signature{&r, &s};

        if (BN_bin2bn(&compact[1], 32, signature.r) == nullptr) {
            serror("loading signature failed: %d %d", __LINE__, ERR_get_error());
            BN_clear(&r); BN_clear(&s);
            return {};
        }
        if (BN_bin2bn(&compact[33], 32, signature.s) == nullptr) {
            serror("loading signature failed: %d %d", __LINE__, ERR_get_error());
            BN_clear(&r); BN_clear(&s);
            return {};
        }

        ECDSASignature tmp;
        auto bin = &tmp.bin();
        if (!i2d_ECDSA_SIG(&signature, &bin)) {
            serror("signing message failed - %d %d", __LINE__, ERR_get_error());
            BN_clear(&r); BN_clear(&s);
            return {};
        }
        tmp.RecId = (int8_t)(compact[0] - 31);
        return tmp;
    }

    bool ECDSASign(ECDSASignature &sig, const PrivateKey &priv, const void *data, size_t len)
    {
        sig.zero();

        auto key = ECKey::fromKey(priv);
        if (!key) {
            serror("signing message failed - invalid key");
            return false;
        }
        SHA256Digest digest;
        SHA256(digest, data, len);
        auto signature = ECDSA_do_sign(&digest.bin(), digest.size(), key);
        if (signature == nullptr) {
            serror("signing message failed - %d %d", __LINE__, ERR_get_error());
            return false;
        }

        /* See https://github.com/sipa/bitcoin/commit/a81cd9680.
         * There can only be one signature with an even S, so make sure we
         * get that one. */
        if (BN_is_odd(signature->s)) {
            const EC_GROUP *group;
            BIGNUM order{};

            BN_init(&order);
            group = EC_KEY_get0_group(key);
            EC_GROUP_get_order(group, &order, NULL);
            BN_sub(signature->s, &order, signature->s);
            BN_clear(&order);

            if (BN_is_odd(signature->s)) {
                serror("signing message failed '%d'", __LINE__);
                ECDSA_SIG_free(signature);
                return false;
            }
            sdebug("Was odd had to unodd");
        }

        if (ECDSA_size(key) != sig.size()) {
            serror("signing message failed '%d'- %d", ECDSA_size(key), __LINE__);
            ECDSA_SIG_free(signature);
            return false;
        }
        auto bin = &sig.bin();
        if (!i2d_ECDSA_SIG(signature, &bin)) {
            serror("signing message failed - %d %d", __LINE__, ERR_get_error());
            ECDSA_SIG_free(signature);
            return false;
        }

        for (int8_t i=0; i<4; i++)
        {
            auto ecKey = EC_KEY_new_by_curve_name(NID_secp256k1);
            if (ecKey == nullptr) {
                serror("signing message failure %d %d", __LINE__, ERR_get_error());
                return {};
            };

            if (ECDSA_SIG_recover_key_GFp(ecKey, signature, (unsigned char*)&digest.bin(), digest.size(), i, 1) == 1) {
                PublicKey pub;
                key2pub(pub, ecKey);
                if (pub == key.getPublicKey()) {
                    sig.RecId = i;
                    break;
                }
            }
            EC_KEY_free(ecKey);
        }
        sdebug("RecId is %hhd", sig.RecId);
        if (sig.RecId == -1) {
            serror("signing message failed - %d RecId", __LINE__);
            return {};
        }

        ECDSA_SIG_free(signature);
        return true;
    }

    bool ECDSAVerify(const void *data, size_t len, const ECDSASignature &sig, const PublicKey &pub)
    {
        auto key = PublicKey::pub2key(pub);
        if (key == nullptr) {
            serror("verifying signature failed: %d %d", __LINE__, ERR_get_error());
            return false;
        }
        auto bin = &sig.bin();
        auto signature = d2i_ECDSA_SIG(nullptr, &bin, sig.size());
        if (signature == nullptr) {
            serror("verifying signature failed: %d %d", __LINE__, ERR_get_error());
            EC_KEY_free(key);
            return false;
        }

        Hash hash;
        Hash256(hash, data, len);
        auto verified = ECDSA_do_verify(&hash.bin(), hash.size(), signature, key);
        if (verified < 0) {
            serror("verifying signature failed: %d %d", __LINE__, ERR_get_error());
        }

        ECDSA_SIG_free(signature);
        EC_KEY_free(key);

        return verified == 1;
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

namespace sc = suil::crypto;

TEST_CASE("suil::crypto", "[crypto]")
{
    const suil::String priv1Str{"365c872f42c8dfe487c543ec2142d36d843ba31c4cc1152b72ac4052b0792c04"};
    const suil::String pub1Str{"022f931cbec33d538734f926ca7895b16e3ba21a2635fa5481bb50de408723a457"};

    SECTION("Generating/loading key pairs") {
        auto key1 = sc::ECKey::generate();
        REQUIRE(key1.isValid());

        auto priv1 = key1.getPrivateKey();
        auto key2 = sc::ECKey::fromKey(priv1);
        REQUIRE(key2.isValid());

        REQUIRE(key2.getPrivateKey() == key1.getPrivateKey());
        REQUIRE(key2.getPublicKey() == key1.getPublicKey());

        WHEN("Loading keys from string") {
            auto priv2 = sc::PrivateKey::fromString(priv1Str);
            REQUIRE_FALSE(priv2.isnil());
            auto pub2 = sc::PublicKey::fromString(pub1Str);
            REQUIRE_FALSE(priv2.isnil());

            auto key3 = sc::ECKey::fromKey(priv2);
            REQUIRE(key3.isValid());
            printf("Pub: %s\n", key3.getPublicKey().toString()());
            REQUIRE(key3.getPublicKey() == pub2);
            REQUIRE(key3.getPrivateKey() == priv2);
            REQUIRE(pub2.toString() == pub1Str);
            REQUIRE(priv2.toString() == priv1Str());
        }
    }
}

#endif
//
// Created by Mpho Mbotho on 2021-06-04.
//

#include <suil/base/secp256k1.hpp>
#include <suil/base/crypto.hpp>
#include <suil/base/logging.hpp>

#include <secp256k1_recovery.h>

namespace suil::secp256k1 {

    Context::Context()
            : mContext{secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY)}
    {
        if (mContext == nullptr) {
            throw InvalidArguments("failed to create secp256k1 context");
        }
    }

    secp256k1_context* Context::get()
    {
        static Context instance;
        return instance.mContext;
    }

    PrivateKey PrivateKey::fromString(const suil::String &str)
    {
        PrivateKey key;
        suil::bytes(str, &key[0], key.size());
        if (secp256k1_ec_seckey_verify(Context::get(), &key[0]) == 0) {
            key.zero<>();
        }
        return key;
    }

    PublicKey PublicKey::fromString(const suil::String &str)
    {
        PublicKey pub;
        suil::bytes(str, &pub[0], pub.size());
        secp256k1_pubkey _;
        if (secp256k1_ec_pubkey_parse(Context::get(), &_, &pub[0], pub.size()) == 0) {
            pub.zero<>();
        }
        return pub;
    }

    PublicKey PublicKey::fromPrivateKey(const suil::secp256k1::PrivateKey &priv)
    {
        secp256k1_pubkey public_key;
        if (secp256k1_ec_pubkey_create(Context::get(), &public_key, &priv[0]) == 0) {
            return {};
        }
        PublicKey pub;
        size_t tmp{pub.size()};
        secp256k1_ec_pubkey_serialize(Context::get(), &pub[0], &tmp, &public_key, SECP256K1_EC_COMPRESSED);
        return pub;
    }

    bool KeyPair::isValid() const
    {
        return !Ego.Private.isnil() && !Ego.Public.isnil();
    }

    KeyPair KeyPair::fromPrivateKey(const suil::String &str)
    {
        auto priv = PrivateKey::fromString(str);
        return KeyPair::fromPrivateKey(priv);
    }

    KeyPair KeyPair::fromPrivateKey(const PrivateKey &priv)
    {
        KeyPair kp;
        kp.Private.copyfrom(priv);
        if (kp.Private.isnil()) {
            return {};
        }
        kp.Public = PublicKey::fromPrivateKey(kp.Private);
        if (kp.Public.isnil()) {
            return {};
        }
        return kp;
    }

    Signature Signature::fromDER(const suil::String &sig)
    {
        uint8_t buf[72];
        secp256k1_ecdsa_signature tmp;
        if (secp256k1_ecdsa_signature_parse_der(Context::get(), &tmp, (uint8_t *)&sig[0], sig.size()) == 0) {
            return {};
        }
        Signature out;
        secp256k1_ecdsa_signature_serialize_compact(Context::get(), &out[0], &tmp);
        return out;
    }

    Signature Signature::fromCompact(const suil::String &sig)
    {
        Signature out;
        secp256k1_ecdsa_signature tmp;
        if (secp256k1_ecdsa_signature_parse_compact(Context::get(), &tmp, &out[0]) == 0) {
            return {};
        }
        return out;
    }

    bool ECDSASign(Signature &sig, const PrivateKey &key, const void *data, size_t len)
    {
        crypto::SHA256Digest hash;
        crypto::SHA256(hash, data, len);

        secp256k1_nonce_function noncefn = secp256k1_nonce_function_rfc6979;

        secp256k1_ecdsa_recoverable_signature rsig;
        if (secp256k1_ecdsa_sign_recoverable(Context::get(), &rsig, &hash[0], &key[0], noncefn, data) == 0) {
            serror("secp256k1_ecdsa_sign_recoverable recoverable fail");
            return false;
        }

        if (secp256k1_ecdsa_recoverable_signature_serialize_compact(Context::get(), &sig[0], &sig.RecId, &rsig) == 0) {
            serror("secp256k1_ecdsa_recoverable_signature_serialize_compact fail");
            return false;
        }
        return true;
    }

    bool ECDSAVerify(const void* data, size_t len, const Signature& sig, const PublicKey& key)
    {
        crypto::SHA256Digest hash;
        crypto::SHA256(hash, data, len);
        secp256k1_ecdsa_signature ssig;
        if (secp256k1_ecdsa_signature_parse_compact(Context::get(), &ssig, &sig[0]) == 0) {
            serror("secp256k1_ecdsa_signature_parse_compact fail");
            return false;
        }

        secp256k1_pubkey public_key;
        if (secp256k1_ec_pubkey_parse(Context::get(), &public_key, &key[0], key.size()) == 0) {
            serror("secp256k1_ec_pubkey_parse fail");
            return false;
        }

        if (secp256k1_ecdsa_verify(Context::get(), &ssig, &hash[0], &public_key) == 0) {
            serror("secp256k1_ecdsa_verify fail");
            return false;
        }

        return true;
    }

}
//
// Created by Mpho Mbotho on 2021-06-04.
//

#pragma once

#include <suil/base/base64.hpp>
#include <suil/base/blob.hpp>
#include <suil/base/string.hpp>

#include <secp256k1.h>

namespace suil::secp256k1 {

    DECLARE_EXCEPTION(Secp256k1Error);

    class Context final {
    public:
        static secp256k1_context* get();

        Context(Context&&) = delete;
        Context(const Context&) = delete;
        Context& operator=(Context&&) = delete;
        Context& operator=(const Context&) = delete;

    private:
        Context();

        secp256k1_context* mContext{nullptr};
    };

    constexpr int SECP256_PUBKEY_SIZE{33};
    constexpr int SECP256_PRIVKEY_SIZE{32};
    constexpr int SECP256_SIGNATURE_SIZE{64};

    template <size_t N>
    struct Serializable: Blob<N> {
        using Blob<N>::Blob;
        suil::String toString(bool b64 = false) const {
            if (!b64) {
                return suil::hexstr(&Ego[0], Ego.size());
            }
            else {
                return Base64::encode(&Ego[0], Ego.size());
            }
        }
    };

    struct PrivateKey: Serializable<SECP256_PRIVKEY_SIZE> {
        using Serializable<SECP256_PRIVKEY_SIZE>::Serializable;

        static PrivateKey fromString(const suil::String& str);
    };

    struct PublicKey: Serializable<SECP256_PUBKEY_SIZE> {
        using Serializable<SECP256_PUBKEY_SIZE>::Serializable;

        static PublicKey fromString(const suil::String& str);
        static PublicKey fromPrivateKey(const PrivateKey& priv);
    };

    struct KeyPair final {
        static KeyPair fromPrivateKey(const PrivateKey& priv);
        static KeyPair fromPrivateKey(const suil::String& str);

        bool isValid() const;

        PublicKey  Public;
        PrivateKey Private;

        KeyPair() = default ;
    };

    struct Signature : Serializable<SECP256_SIGNATURE_SIZE> {
        static Signature fromDER(const suil::String& sig);
        static Signature fromCompact(const suil::String& sig);
        int RecId{-1};
    };

    bool ECDSASign(Signature& sig, const PrivateKey& key, const void* data, size_t len);

    template <typename T>
    inline bool ECDSASign(Signature& sig, const PrivateKey& key, const T& data) {
        return ECDSASign(sig, key, data.data(), data.size());
    }

    inline Signature ECDSASign(const PrivateKey& key, const void* data, size_t len) {
        Signature sig;
        ECDSASign(sig, key, data, len);
        return sig;
    }

    template <typename T>
    inline Signature ECDSASign(const PrivateKey& key, const T& data) {
        return ECDSASign(key, data.data(), data.size());
    }

    bool ECDSAVerify(const void* data, size_t len, const Signature& sig, const PublicKey& key);

    template <typename T>
    inline bool ECDSAVerify(const T& data, const Signature& sig, const PublicKey& key) {
        return ECDSAVerify(data.data(), data.size(), sig, key);
    }
}
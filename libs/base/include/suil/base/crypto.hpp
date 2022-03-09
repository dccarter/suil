//
// Created by Mpho Mbotho on 2020-10-17.
//

#ifndef SUIL_BASE_CRYPTO_HPP
#define SUIL_BASE_CRYPTO_HPP

#include "suil/base/blob.hpp"
#include "suil/base/string.hpp"

#include <suil/utils/exception.hpp>

#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/ec.h>

namespace suil::crypto {

    constexpr int ECDSA_SIGNATURE_SIZE{72};
    constexpr int ECDSA_COMPACT_SIGNATURE_SIZE{65};
    constexpr int EC_PRIVATE_KEY_SIZE{32};
    constexpr int EC_COMPRESSED_PUBLIC_KEY_SIZE{33};

    DECLARE_EXCEPTION(CryptoError);

    /**
     * Represents a sequences of bytes that can be converted to and
     * from hex string
     * @tparam N the number of bytes or the size of the binary buffer
     */
    template <size_t N>
    class Binary : public Blob<N> {
    public:
        using Blob<N>::Blob;

        /**
         * Converts the binary buffer to a hex string
         * @return a hex string representation of the buffer
         */
        virtual suil::String toString() const {
            return suil::hexstr(&Ego.bin(), Ego.size());
        }

        /**
         * Loads the binary buffer contents from a hex string
         * @param str the hex string to decode into a binary buffer
         * @return true if the string fits perfectly into
         * buffer when decoded and is valid hex string
         */
        virtual bool loadString(const suil::String& str) {
            if (str.size()/2 != N) {
                return false;
            }
            suil::bytes(str, &Ego.bin(), Ego.size());
            return true;
        }
    };


    /**
     * Binary buffer to hold an SHA256 hash
     */
    class SHA256Digest final : public Binary<SHA256_DIGEST_LENGTH> {
    public:
        using Binary<SHA256_DIGEST_LENGTH>::Blob;
    };

    /**
     * Binary buffer to hold an SHA512 hash
     */
    class SHA512Digest final : public Binary<SHA512_DIGEST_LENGTH> {
    public:
        using Binary<SHA512_DIGEST_LENGTH>::Blob;
    };

    /**
     * Binary buffer to hold an RIPEMD160 hash
     */
    class RIPEMD160Digest final : public Binary<RIPEMD160_DIGEST_LENGTH> {
    public:
        using Binary<RIPEMD160_DIGEST_LENGTH>::Blob;
    };

    using Hash = SHA256Digest;
    using HASH160Digest = RIPEMD160Digest;

    /**
     * Computes an SHA256 hash of the given data
     * @param digest the buffer to hold the computed  output
     * @param data the data buffer to run the SHA256 algorithm on
     * @param len the size of the data buffer
     */
    void SHA256(SHA256Digest& digest, const void* data, size_t len);

    /**
     * Computes an SHA256 hash of the given data
     * @tparam T the type of the data
     * @param digest an output variable to hold the computed hash
     * @param data the data to run the SHA256 algorithm on
     */
    template <DataBuf T>
    void SHA256(SHA256Digest& digest, const T& data) {
        crypto::SHA256(digest, data.data(), data.size());
    }

    /**
     * Computes an SHA512 hash of the given data
     * @param digest the buffer to hold the computed  output
     * @param data the data buffer to run the SHA512 algorithm on
     * @param len the size of the data buffer
     */
    void SHA512(SHA512Digest& digest, const void* data, size_t len);

    /**
     * Computes an SHA512 hash of the given data
     * @tparam T the type of the data
     * @param digest an output variable to hold the computed hash
     * @param data the data to run the SHA512 algorithm on
     */
    template <DataBuf T>
    void SHA512(SHA512Digest& digest, const T& data) {
        crypto::SHA512(digest, data.data(), data.size());
    }

    /**
     * Computes an RIPEMD160 hash of the given data
     * @param digest the buffer to hold the computed  output
     * @param data the data buffer to run the RIPEMD160 algorithm on
     * @param len the size of the data buffer
     */
    void RIPEMD160(RIPEMD160Digest& digest, const void* data, size_t len);

    /**
     * Computes an RIPEMD160 hash of the given data
     * @tparam T the type of the data
     * @param digest an output variable to hold the computed hash
     * @param data the data to run the RIPEMD160 algorithm on
     */
    template <DataBuf T>
    void RIPEMD160(RIPEMD160Digest& digest, const T& data) {
        crypto::RIPEMD160(digest, data.data(), data.size());
    }

    /**
     * Runs a double SHA256 algorithm on the given data buffer
     * @param hash the binary buffer to hold the output
     * @param data the data run the algorithm over
     * @param len the size of the buffer
     */
    void Hash256(Hash& hash, const void* data, size_t len);

    /**
     * Runs a double SHA256 algorithm on the given data buffer
     * @tparam T the type of the data
     * @param digest an output variable to hold the computed hash
     * @param data the data run the algorithm over
     */
    template <DataBuf T>
    void Hash256(Hash& hash, const T& data) {
        crypto::Hash256(hash, data.data(), data.size());
    }

    /**
     * Runs a double RIMPEMD160 algorithm on the given data buffer
     * @param hash the binary buffer to hold the output
     * @param data the data run the algorithm over
     * @param len the size of the buffer
     */
    void Hash160(HASH160Digest& hash, const void* data, size_t len);

    /**
     * Runs a double RIPEMD160 algorithm on the given data buffer
     * @tparam T the type of the data
     * @param digest an output variable to hold the computed hash
     * @param data the data run the algorithm over
     */
    template <DataBuf T>
    void Hash160(HASH160Digest& hash, const T& data) {
        crypto::Hash160(hash, data.data(), data.size());
    }

    /**
     * Encodes data in the given buffer using base58 encoding
     * @param data a buffer with data to encode
     * @param len the number of bytes in the buffer to encode
     * @param csum (default: false) if true, then the checksum bytes will
     * be included in the encoded string
     *
     * @return a base58 encoding of the given data
     */
    suil::String toBase58(const void* data, size_t len, bool csum = false);

    /**
     * Encodes data in the given buffer using base58 encoding
     * @tparam T the type of the data to encode
     * @param data the data to encode to base58
     * @param csum (default: false) if true, then the checksum bytes will
     * be included in the encoded string
     * @return a base58 encoding of the given data
     */
    template <DataBuf T>
    suil::String toBase58(const T& data, bool csum = false) {
        return crypto::toBase58(data.data(), data.size());
    }

    /**
     * Convert the given string string to binary
     * @tparam N the size of the binary buffer to hold the output
     * @param bin the binary buffer to hold the output
     * @param data the string to be converted to binary
     */
    template <size_t N>
    static void str2bin(Binary<N>& bin, const suil::String& data) {
        suil::bytes(data, &bin.bin(), bin.size());
    }

    /**
     * A binary buffer to hold an EC compressed public key
     */
    class PublicKey final : public Binary<EC_COMPRESSED_PUBLIC_KEY_SIZE> {
    public:
        using Binary<EC_COMPRESSED_PUBLIC_KEY_SIZE>::Blob;
        /**
         * Loads a public key from string
         * @param key a \class PublicKey binary buffer to hold the loaded key
         * @param str string representation of a public key
         * @return true if the public key was loaded successfully, false otherwise
         */
        static bool fromString(PublicKey& key, const String& str);
        /**
         * Loads a public key from string
         * @param str string representation of a public key
         * @return A \class PublicKey binary buffer decoded from the
         * given string
         *
         * @throws CryptoError when decoding the given string fails
         */
        static PublicKey fromString(const String& str);

        /**
         * Converts the given \class PublicKey buffer to an EC_KEY instance
         * @param pub the public key to convert to an openssl EC_KEY
         * @return Pointer to an openssl EC_KEY on success, otherwise nullptr
         * is returned
         */
        static EC_KEY* pub2key(const PublicKey& pub);
    };

    /**
     * A binary buffer to hold an EC private key
     */
    class PrivateKey final: public Binary<EC_PRIVATE_KEY_SIZE> {
    public:
        using Binary<EC_PRIVATE_KEY_SIZE>::Blob;

        /**
         * Loads a private key into the given \param buffer from the
         * given string \param str
         * @param key a \class PrivateKey buffer to hold the decoded key
         * @param str a hex formatted private key string
         * @return true if the private key was successfully decoded from given
         * string, false otherwise
         */
        static bool fromString(PrivateKey& key, const String& str);

        /**
         * Creates a private key from the given hex encoded key string
         * @param str private key encoded in hex format
         * @return an instance of the decoded private key
         *
         * @throws CryptoError if decoding private key fails
         */
        static PrivateKey fromString(const String& str);
    };

    class ECDSASignature;

    /**
     * Wrapper openssl's EC_KEY data structure
     */
    class ECKey final {
    public:
        using Conversion = point_conversion_form_t;

        /**
         * Creates a new empty ECKey, i.e an invalid ECKey
         */
        ECKey() = default;

        ECKey(ECKey&& other) noexcept;
        ECKey&operator=(ECKey&& other) noexcept;

        ECKey(const ECKey&) = delete;
        ECKey&operator=(const ECKey&) = delete;

        /**
         * Generates a new ECKey instance
         * @return an instance of ECKey which can be valid
         * on success or invalid on failure
         */
        static ECKey generate();

        /**
         * Generate an ECKey instance from the given private key
         * @return an instance of ECKey which can be valid
         * on success or invalid on failure
         */
        static ECKey fromKey(const PrivateKey& key);

        /**
         * Retrieve private key associated with the ECKey instance
         * @return reference to private private key
         */
        const PrivateKey& getPrivateKey() const;

        /**
         * Retrieve public key associated with the ECKey instance
         * @return reference to the public key
         */
        const PublicKey& getPublicKey() const;

        /**
         * Checks whether the ECKey is valid or invalid
         * @return true if this instance has a valid EC_KEY,
         * false otherwise
         */
        bool isValid() const;

        operator bool() const {return isValid(); }

        operator EC_KEY*() const { return ecKey; }

        ~ECKey();

    private:
        ECKey(EC_KEY *key);
        EC_KEY     *ecKey{nullptr};
        PrivateKey  privKey;
        PublicKey   pubKey;
    };

    class ECDSASignature : public Binary<ECDSA_SIGNATURE_SIZE> {
    public:
        using Binary<ECDSA_SIGNATURE_SIZE>::Blob;
        suil::String toCompactForm(bool base64 = true) const;
        static ECDSASignature fromCompactForm(const suil::String& sig, bool b64 = true);
        int8_t RecId{-1};
    };

    bool ECDSASign(ECDSASignature& sig, const PrivateKey& key, const void* data, size_t len);

    template <DataBuf T>
    inline bool ECDSASign(ECDSASignature& sig, const PrivateKey& key, const T& data) {
        return ECDSASign(sig, key, data.data(), data.size());
    }

    inline ECDSASignature ECDSASign(const PrivateKey& key, const void* data, size_t len) {
        ECDSASignature sig;
        ECDSASign(sig, key, data, len);
        return sig;
    }

    template <DataBuf T>
    inline ECDSASignature ECDSASign(const PrivateKey& key, const T& data) {
        return ECDSASign(key, data.data(), data.size());
    }

    bool ECDSAVerify(const void* data, size_t len, const ECDSASignature& sig, const PublicKey& key);

    template <DataBuf T>
    inline bool ECDSAVerify(const T& data, const ECDSASignature& sig, const PublicKey& key) {
        return ECDSAVerify(data.data(), data.size(), sig, key);
    }
}

#endif //SUIL_BASE_CRYPTO_HPP

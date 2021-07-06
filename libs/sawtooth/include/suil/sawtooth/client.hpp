//
// Created by Mpho Mbotho on 2021-06-04.
//

#pragma once

#include <suil/base/logging.hpp>
#include <suil/base/secp256k1.hpp>
#include <suil/sawtooth/protos.hpp>

namespace suil::saw::Client {

    DECLARE_EXCEPTION(EncoderError);

    define_log_tag(SAWSDK_CLIENT);
    class Signer final : LOGGER(SAWSDK_CLIENT) {
    public:
        Signer(Signer&& other) noexcept;
        Signer& operator=(Signer&& other) noexcept;

        Signer(const Signer&) = delete;
        Signer& operator=(const Signer&) = delete;

        static Signer load(const suil::String& privKey);
        static Signer load(const secp256k1::PrivateKey& privKey);

        void get(secp256k1::PrivateKey& out) const;
        void get(secp256k1::PublicKey& out) const;
        [[nodiscard]]
        const secp256k1::PrivateKey& getPrivateKey() const;
        [[nodiscard]]
        const secp256k1::PublicKey& getPublicKey() const;

        suil::String sign(const void* data, size_t len) const;

        template <typename T>
        suil::String sign(const T& data) const {
            return Ego.sign(data.data(), data.size());
        }

        static bool verify(const void* data, size_t len, const suil::String& signature, const suil::String& publicKey);

        template <typename T>
        static bool verify(const T& data, const suil::String& signature, const suil::String& publicKey) {
            return verify(data.data(), data.size(), signature, publicKey);
        }

        [[nodiscard]]
        inline bool isValid() const { return Ego.mKey.isValid(); }

        private:
        explicit Signer(secp256k1::KeyPair&& key);
        secp256k1::KeyPair mKey;
    };

    struct Transaction final {
        sawtooth::protos::Transaction* operator->();
        sawtooth::protos::Transaction& operator* ();
        const sawtooth::protos::Transaction* operator->() const;
        const sawtooth::protos::Transaction& operator* () const;
    private:
        friend struct Encoder;
        Transaction();
        sawtooth::protos::Transaction& get();

        std::shared_ptr<sawtooth::protos::Transaction> mSelf;
    };

    struct Batch final {
        sawtooth::protos::Batch* operator-> ();
        sawtooth::protos::Batch& operator* ();
        const sawtooth::protos::Batch* operator-> () const;
        const sawtooth::protos::Batch& operator* () const;
    private:
        friend struct Encoder;
        Batch();
        sawtooth::protos::Batch& get();

        std::shared_ptr<sawtooth::protos::Batch> mSelf{nullptr};
    };

    using Inputs = std::vector<String>;
    using Outputs = std::vector<String>;

    class Encoder final {
    public:
        Encoder(const String& family, String&& familyVersion, const String&& privateKey);

        Transaction operator()(const Data& payload, const Inputs& inputs = {}, const Outputs& outputs = {}) const;

        Batch operator()(const std::vector<Transaction>& txns) const;

        Batch operator()(const Transaction& txn) const {
            return Ego(std::vector<Transaction>{txn});
        }

        static void encode(sawtooth::protos::BatchList& out, const std::vector<Batch>& batches);
        static suil::Data encode(const std::vector<Batch>& batches);
        static void encode(Buffer& dst, const std::vector<Batch>& batches);
        Batch encode(const suil::Data& payload, const Inputs& inputs = {}, const Outputs& outputs = {});
        void setSigner(const String& key);
        const String& getSigner() const { return Ego.mSignerPublicKey; }
        const String& getBatcher() const { return Ego.mBatcherPublicKey; }
        void setBatcher(const String& key);
    private:
        suil::String mFamily{};
        suil::String mFamilyVersion{};
        suil::String mBatcherPublicKey{};
        suil::String mSignerPublicKey{};
        Signer mSigner;
        Signer mBatcher;
    };

}

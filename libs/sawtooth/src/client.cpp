//
// Created by Mpho Mbotho on 2021-06-04.
//

#include "suil/sawtooth/client.hpp"
#include "suil/sawtooth/transaction.hpp"

#include <suil/base/buffer.hpp>
#include <suil/base/crypto.hpp>
#include <suil/base/uuid.hpp>

namespace sp = sawtooth::protos;

namespace suil::saw::Client {


    Signer::Signer(secp256k1::KeyPair&& key)
            : mKey(key)
    {}

    Signer:: Signer(Signer&& other) noexcept
            : mKey(other.mKey)
    {}

    Signer& Signer::operator=(Signer&& other) noexcept
    {
        Ego.mKey = other.mKey;
        return Ego;
    }

    Signer Signer::load(const suil::String& privKey)
    {
        auto priv = secp256k1::PrivateKey::fromString(privKey);
        if (!priv.isnil()) {
            return Signer::load(priv);
        }
        else {
            return Signer{{}};
        }
    }

    Signer Signer::load(const secp256k1::PrivateKey& privKey)
    {
        return Signer{secp256k1::KeyPair::fromPrivateKey(privKey)};
    }

    void Signer::get(secp256k1::PrivateKey& out) const
    {
        if (Ego.isValid()) {
            out.copyfrom(Ego.getPrivateKey());
        }
    }

    void Signer::get(secp256k1::PublicKey& out) const {
        if (Ego.isValid()) {
            out.copyfrom(Ego.getPublicKey());
        }
    }

    const secp256k1::PrivateKey& Signer::getPrivateKey() const {
        return Ego.mKey.Private;
    }

    const secp256k1::PublicKey& Signer::getPublicKey() const {
        return Ego.mKey.Public;
    }

    suil::String Signer::sign(const void* data, size_t len) const {
        secp256k1::Signature signature;
        if (secp256k1::ECDSASign(signature, Ego.getPrivateKey(), data, len)) {
            return signature.toString();
        }
        ierror("Signer::sign signing message failed");
        return {};
    }

    bool Signer::verify(const void* data, size_t len, const suil::String& sig, const suil::String& publicKey)
    {
        LOGGER(SAWSDK_CLIENT) lt;
        auto signature = secp256k1::Signature::fromCompact(sig);
        if (signature.isnil()) {
            lerror(&lt, "Signer::verify loading signature failed");
            return false;
        }
        auto pubKey = secp256k1::PublicKey::fromString(publicKey);
        if (pubKey.isnil()) {
            lerror(&lt, "Signer::verify loading public key failed");
            return false;
        }
        return secp256k1::ECDSAVerify(data, len, signature, pubKey);
    }

    Transaction::Transaction()
            : mSelf(new sawtooth::protos::Transaction)
    {}

    sp::Transaction& Transaction::get() {
        return *mSelf;
    }

    sawtooth::protos::Transaction& Transaction::operator*() {
        return *mSelf;
    }

    sawtooth::protos::Transaction* Transaction::operator->() {
        return Ego.mSelf.get();
    }

    const sawtooth::protos::Transaction& Transaction::operator*() const {
        return *mSelf;
    }

    const sawtooth::protos::Transaction* Transaction::operator->() const {
        return Ego.mSelf.get();
    }


    Batch::Batch()
            : mSelf(new sp::Batch)
    {}

    sawtooth::protos::Batch& Batch::get() {
        return *mSelf;
    }

    sawtooth::protos::Batch* Batch::operator->() {
        return Ego.mSelf.get();
    }

    sawtooth::protos::Batch& Batch::operator*() {
        return *mSelf;
    }

    const sawtooth::protos::Batch* Batch::operator->() const {
        return Ego.mSelf.get();
    }

    const sawtooth::protos::Batch& Batch::operator*() const {
        return *mSelf;
    }

    Encoder::Encoder(const String& family, String&& familyVersion, const String&& privateKey)
        : mSigner{Signer::load(privateKey)},
          mBatcher{Signer::load(privateKey)},
          mFamily{family.dup()},
          mFamilyVersion{std::move(familyVersion)}
    {
        mBatcherPublicKey = mSigner.getPublicKey().toString();
        mSignerPublicKey = mBatcher.getPublicKey().toString();
    }

    void Encoder::setBatcher(const suil::String &key)
    {
        Ego.mBatcher = Signer::load(key);
        if (!Ego.mBatcher.isValid()) {
            throw EncoderError("Given batcher '", key, "' is an invalid key");
        }
        Ego.mBatcherPublicKey = mBatcher.getPublicKey().toString();
    }

    void Encoder::setSigner(const suil::String &key)
    {
        Ego.mSigner = Signer::load(key);
        if (!Ego.mSigner.isValid()) {
            throw EncoderError("Given signer '", key, "' is an invalid key");
        }
        Ego.mSignerPublicKey = mSigner.getPublicKey().toString();
    }

    Transaction Encoder::operator()(const suil::Data& payload, const Inputs& inputs, const Outputs& outputs) const
    {
        crypto::SHA512Digest sha512;
        crypto::SHA512(sha512, payload);

        sp::TransactionHeader header;
        protoSet(header, family_name, Ego.mFamily);
        protoSet(header, family_version, Ego.mFamilyVersion);
        protoSet(header, signer_public_key, Ego.mSignerPublicKey);
        protoSet(header, batcher_public_key, Ego.mBatcherPublicKey);
        protoSet(header, nonce, suil::uuidstr());
        protoSet(header, payload_sha512, sha512.toString());
        for (const auto& input: inputs) {
            protoAdd(header, inputs, input);
        }
        for (const auto& output: outputs) {
            protoAdd(header, outputs, output);
        }
        auto headerBytes = protoSerialize(header);
        auto signature = Ego.mSigner.sign(headerBytes);

        Transaction txn;
        protoSet(*txn, payload, payload);
        protoSet(*txn, header, headerBytes);
        protoSet(*txn, header_signature, signature);
        return txn;
    }

    Batch Encoder::operator()(const std::vector<Transaction> &txns) const
    {
        sp::BatchHeader header;
        Batch batch;
        for (const auto& txn: txns) {
            protoAdd(header, transaction_ids, txn->header_signature());
            auto it = batch->add_transactions();
            it->CopyFrom(*txn);
        }
        protoSet(header, signer_public_key, Ego.mBatcherPublicKey);

        auto headerBytes = protoSerialize(header);
        auto signature = mBatcher.sign(headerBytes);
        protoSet(*batch, header, headerBytes);
        protoSet(*batch, header_signature, signature);
        protoSet(*batch, trace, true);
        return batch;
    }

    void Encoder::encode(sawtooth::protos::BatchList& out, const std::vector<Batch>& batches) {
        for (const auto& batch: batches) {
            out.add_batches()->CopyFrom(*batch);
        }
    }

    suil::Data Encoder::encode(const std::vector<Batch> &batches)
    {
        sawtooth::protos::BatchList list;
        Encoder::encode(list, batches);
        suil::Data data{list.ByteSizeLong()};
        list.SerializeToArray(data.data(), static_cast<int>(data.size()));
        return data;
    }

    void Encoder::encode(Buffer& dst, const std::vector<Batch> &batches)
    {
        sawtooth::protos::BatchList list;
        Encoder::encode(list, batches);
        dst.reserve(list.ByteSizeLong()+16);
        auto buf = &dst[dst.size()];
        list.SerializeToArray(buf, static_cast<int>(dst.capacity()));
        dst.seek(list.ByteSizeLong()+5);
    }

    Batch Encoder::encode(const suil::Data &payload, const Inputs &inputs, const Outputs &outputs)
    {
        auto txn = Ego(payload, inputs, outputs);
        return Ego(txn);
    }

}
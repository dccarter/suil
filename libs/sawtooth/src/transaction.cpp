//
// Created by Mpho Mbotho on 2021-06-04.
//

#include <suil/sawtooth/transaction.hpp>
#include <suil/base/string.hpp>

namespace suil::saw {

    TransactionHeader::TransactionHeader(sawtooth::protos::TransactionHeader *proto)
            : mHeader(proto)
    {}

    TransactionHeader::TransactionHeader(TransactionHeader &&other) noexcept
        : mHeader(other.mHeader)
    {
        other.mHeader = nullptr;
    }

    TransactionHeader& TransactionHeader::operator=(TransactionHeader &&other) noexcept
    {
        if (this != &other) {
            Ego.mHeader = other.mHeader;
            other.mHeader = nullptr;
        }
        return Ego;
    }

    TransactionHeader::~TransactionHeader() {
        if (mHeader) {
            delete mHeader;
            mHeader = nullptr;
        }
    }

    int TransactionHeader::getCount(TransactionHeader::Field field) const {
        switch (field) {
            case TransactionHeader::Field::StringDependencies:
                return mHeader->dependencies_size();
            case Field::Inputs:
                return mHeader->inputs_size();
            case Field::Outputs:
                return mHeader->outputs_size();
            case Field::Nonce:
            case Field::FamilyName:
            case Field::FamilyVersion:
            case Field::PayloadSha512:
            case Field::BatcherPublicKey:
            case Field::SignerPublicKey:
                return 1;
            default:
                return 0;
        }
    }

    suil::Data TransactionHeader::getValue(TransactionHeader::Field field, int index) const
    {
        switch (field) {
            case TransactionHeader::Field::StringDependencies:
                return fromStdString(mHeader->dependencies(index));
            case Field::Inputs:
                return fromStdString(mHeader->inputs(index));
            case Field::Outputs:
                return fromStdString(mHeader->outputs(index));
            case Field::Nonce:
                return fromStdString(mHeader->nonce());
            case Field::FamilyName:
                return fromStdString(mHeader->family_name());
            case Field::FamilyVersion:
                return fromStdString(mHeader->family_version());
            case Field::PayloadSha512:
                return fromStdString(mHeader->payload_sha512());
            case Field::BatcherPublicKey:
                return fromStdString(mHeader->batcher_public_key());
            case Field::SignerPublicKey:
                return fromStdString(mHeader->signer_public_key());
            default:
                return suil::Data{};
        }
    }

    Transaction::Transaction(TransactionHeader&& header, std::string payload, std::string signature)
        : mHeader(std::move(header)),
          mPayload(std::move(payload)),
          mSignature(std::move(signature))
    {}

    Transaction::Transaction(Transaction&& other)
        : mHeader(std::move(other.mHeader)),
          mPayload(std::move(other.mPayload)),
          mSignature(std::move(other.mSignature))
    {}

    Transaction& Transaction::operator=(Transaction&& other)
    {
        if (this != &other) {
            mHeader = std::move(other.mHeader);
            mPayload = std::move(other.mPayload);
            mSignature = std::move(other.mSignature);
        }
        return Ego;
    }

    const String NOOP{"84e4cee5-48f0-4af3-a60b-ed85f9401197"};

    Data getNoopTransaction()
    {
        return Data(NOOP.data(), NOOP.size(), false);
    }

    bool isNoopTransaction(const Transaction& txn)
    {
        return txn.payload() == Data{NOOP.data(), NOOP.size(), false};
    }

}
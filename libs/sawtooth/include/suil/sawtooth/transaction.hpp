//
// Created by Mpho Mbotho on 2021-06-04.
//

#pragma once

#include <suil/base/data.hpp>
#include <suil/base/exception.hpp>
#include <suil/sawtooth/protos.hpp>

namespace suil::saw {

    DECLARE_EXCEPTION(InvalidTransaction);
    DECLARE_EXCEPTION(InternalError);

    struct TransactionHeader {
        enum Field {
            BatcherPublicKey = 1,
            StringDependencies,
            FamilyName,
            FamilyVersion,
            Inputs,
            Nonce,
            Outputs,
            PayloadSha512,
            SignerPublicKey
        };

        TransactionHeader(sawtooth::protos::TransactionHeader* proto);
        TransactionHeader(TransactionHeader&& other) noexcept;
        TransactionHeader& operator=(TransactionHeader&& other) noexcept ;

        TransactionHeader(const TransactionHeader&) = delete;
        TransactionHeader& operator=(const TransactionHeader&) = delete;

        virtual int getCount(Field field) const;
        virtual suil::Data getValue(Field field, int index = 0) const;

        virtual ~TransactionHeader();

    private:
        sawtooth::protos::TransactionHeader* mHeader{nullptr};
    };

    struct Transaction final {
        Transaction(TransactionHeader&& header, std::string payload, std::string signature);

        Transaction(Transaction&& other);

        Transaction&operator=(Transaction&& other);

        Transaction(const Transaction&) = delete;
        Transaction&operator=(const Transaction&) = delete;

        [[nodiscard]] const TransactionHeader& header() const { return mHeader; }

        [[nodiscard]] suil::Data payload() const {
            return suil::Data{mPayload.data(), mPayload.size(), false};
        }

        [[nodiscard]] suil::Data signature() const {
            return suil::Data{mSignature.data(), mSignature.size(), false};
        }

    private:
        TransactionHeader mHeader;
        std::string mPayload{};
        std::string mSignature{};
    };

    inline suil::Data fromStdString(const std::string& str) {
        return suil::Data{str.data(), str.size(), false};
    }

    Data getNoopTransaction();
    bool isNoopTransaction(const Transaction& txn);

    template <typename T, typename V>
    inline void setValue(T& to, void(T::*func)(const char*, size_t), const V& data) {
        (to.*func)(reinterpret_cast<const char *>(data.data()), data.size());
    }

    template <typename T, typename V>
    inline void setValue(T& to, void(T::*func)(const void*, size_t), const V& data) {
        (to.*func)(data.data(), data.size());
    }

    template <typename T, typename V>
    inline void setValue(T& to, void(T::*func)(V), const V& data) {
        (to.*func)(data);
    }

    template <typename T>
    inline suil::Data protoSerialize(const T& proto) {
        suil::Data data{proto.ByteSizeLong()};
        proto.SerializeToArray(data.data(), static_cast<int>(data.size()));
        return data;
    }

#define protoSet(obj, prop, val) \
        setValue(obj, &std::remove_reference_t<decltype(obj)>::set_##prop, val)
#define protoAdd(obj, prop, val) \
        setValue(obj, &std::remove_reference_t<decltype(obj)>::add_##prop, val)

}

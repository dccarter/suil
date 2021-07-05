//
// Created by Mpho Mbotho on 2021-06-04.
//

#pragma once

#include <suil/sawtooth/address.hpp>
#include <suil/sawtooth/gstate.hpp>
#include <suil/sawtooth/transaction.hpp>

namespace suil::saw {

    class ProcessorBase {
    public:
        sptr(ProcessorBase);

        ProcessorBase(Transaction&& txn, GlobalState&& state)
            : mTxn{std::move(txn)},
              mState{std::move(state)}
        {}

        virtual void apply() = 0;

        const Transaction& getTransaction() const { return mTxn; }

        GlobalState& globalState() { return mState; }

    private:
        Transaction mTxn;
        GlobalState mState;
    };

    template <typename AddressEncoder = DefaultAddressEncoder>
    class Processor : public virtual ProcessorBase {
    public:
        Processor(AddressEncoder& ae)
            : mAe(ae)
        {}

    protected:
        inline String makeAddress(const suil::String& key) {
            return mAe(key);
        }

        AddressEncoder& mAe;
    };

    class TransactionHandlerBase {
    public:
        sptr(TransactionHandlerBase);

        const String& getFamily() const { return mFamily; }
        std::vector<String>& getVersions() { return mVersions; }
        std::vector<String>& getNamespaces() { return  mNamespaces; }
        uint32& getMaxOccupancy() {return mMaxOccupancy; }
        virtual typename ProcessorBase::Ptr getProcessor(Transaction&& txn, GlobalState&& state) = 0;

    protected:
        TransactionHandlerBase(const String& family, uint32 maxOccupancy = 0)
            : mFamily(family.dup()),
              mMaxOccupancy{maxOccupancy}
        {}

    private:
        String mFamily;
        std::vector<String> mNamespaces;
        std::vector<String> mVersions;
        uint32 mMaxOccupancy;
    };

    template <typename AddressEncoder = DefaultAddressEncoder>
    class TransactionHandler : public virtual TransactionHandlerBase {
    public:
        TransactionHandler(const String& ns)
            : mAe{ns}
        {
            getNamespaces().template emplace_back(mAe.getPrefix());
        }

        virtual AddressEncoder& addressEncoder() { return mAe; }
    private:
        AddressEncoder mAe;
    };


}

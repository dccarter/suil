//
// Created by Mpho Mbotho on 2021-06-16.
//

#pragma once

#include <suil/sawtooth/processor.hpp>
#include <suil/base/json.hpp>

namespace suil::saw {

    define_log_tag(SAW_INTKEY);

    class IntKeyProcessor : public Processor<DefaultAddressEncoder> , LOGGER(SAW_INTKEY) {
    public:
        IntKeyProcessor(DefaultAddressEncoder& ae, Transaction&& txn, GlobalState&& state)
            : Processor(ae),
              ProcessorBase(std::move(txn), std::move(state))
        {}

        void apply() override;

    private:
        typedef void (IntKeyProcessor::*VerbHandler)(json::Object& obj);
        void setKey(json::Object& obj);
        void decrementKey(json::Object& obj);
        void incrementKey(json::Object& obj);

        String getString(const String& name, json::Object& obj);
        std::uint32_t getNumber(const String& name, json::Object& obj);
        static UnorderedMap<VerbHandler> sHandlers;
    };

    class IntKeyHandler : public TransactionHandler<> {
    public:
        static constexpr uint32 MAX_OCCUPANCY{127};
        IntKeyHandler(const String& family, const String& ns)
            : TransactionHandler(ns),
              TransactionHandlerBase(family, MAX_OCCUPANCY)
        {}

        typename ProcessorBase::Ptr getProcessor(Transaction&& txn, GlobalState&& state) override;
    };
}

//
// Created by Mpho Mbotho on 2021-06-04.
//

#pragma once

#include <suil/sawtooth/stream.hpp>

namespace suil::saw {

    DECLARE_EXCEPTION(GlobalStateError);

    class GlobalState {
    public:
        using KeyValue = std::tuple<suil::String, suil::Data>;
        sptr(GlobalState);

        GlobalState(GlobalState&& other);
        GlobalState&operator=(GlobalState&& other);

        GlobalState(const GlobalState&) = delete;
        GlobalState&operator=(const GlobalState&) = delete;

        Data getState(const String& address);

        void getState(UnorderedMap<Data>& data, const std::vector<String>& addresses);

        void setState(const suil::String& address, const suil::Data& value);

        void setState(const std::vector<KeyValue>& data);

        void deleteState(const suil::String& address);

        void deleteState(const std::vector<String>& addresses);

        void addEvent(
                const String& eventType,
                const std::vector<KeyValue>& values,
                const Data& data);

    private:
        friend class TransactionProcessor;
        GlobalState(Stream&& stream, const String& contextId);
        Stream  mStream;
        String  mContextId;
    };

}


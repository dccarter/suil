//
// Created by Mpho Mbotho on 2020-12-03.
//

#ifndef SUIL_RPC_GENERATOR_DEMO_HPP
#define SUIL_RPC_GENERATOR_DEMO_HPP

#include <suil/base/string.hpp>
#include <suil/base/logging.hpp>

namespace suil::rpc {

    define_log_tag(RPC_DEMO);

    class KVStore : public LOGGER(RPC_DEMO) {
    public:
        /// rpc method accepting a string reference
        void set(const suil::String& name, const String& data);
        /// rpc method returning a string
        suil::String get(const String& name);
        /// rpc method returning a map
        const suil::UnorderedMap<String>& list();
    private:
        suil::UnorderedMap<String> mStore;
    };
}
#endif //SUIL_RPC_GENERATOR_DEMO_HPP

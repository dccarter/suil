//
// Created by Mpho Mbotho on 2020-11-30.
//

#include "kvstore.hpp"

#include <suil/base/string.hpp>
#include <suil/rpc/io.hpp>

namespace suil::rpc {

    void KVStore::set(const String& name, const String& data)
    {
        idebug("received " PRIs " " PRIs, _PRIs(name), _PRIs(data));
        auto it = mStore.find(name);
        if (it != mStore.end()) {
            it->second = data.dup();
        }
        else {
            mStore.emplace(name.dup(), data.dup());
        }
    }

    String KVStore::get(const String& name) {
        auto it = mStore.find(name);
        if (it != mStore.end()) {
            return it->second.peek();
        }
        return "";
    }

    const suil::UnorderedMap<String> & KVStore::list()
    {
        return mStore;
    }
}
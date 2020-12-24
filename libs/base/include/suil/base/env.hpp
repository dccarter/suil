//
// Created by Mpho Mbotho on 2020-10-08.
//

#ifndef SUIL_BASE_ENV_HPP
#define SUIL_BASE_ENV_HPP

#include "suil/base/sio.hpp"
#include "suil/base/string.hpp"
#include "suil/base/buffer.hpp"

namespace suil {

    template <typename T>
    inline T env(const char *name, T def = T{}) {
        const char *v = std::getenv(name);
        if (v != nullptr) {
            cast(String(v), def);
        }
        return def;
    }

    const char* env(const char *name, const char *def = nullptr);

    String env(const char *name, const String& def = String{});

    /**
     * Load key/value configuration from environment variables
     * @tparam Config The type of configuration to load
     * @param config The configuration object to load into
     * @param prefix the prefix to use when loading confuration
     */
    template <typename Config>
    static void envconfig(Config& config, const char *prefix = "") {
        iod::foreach2(config)|
        [&](auto& m) {
            String key;
            if (prefix)
                key = catstr(prefix, m.symbol().name());
            else
                key = String{m.symbol().name()}.dup();
            // convert key to uppercase
            key.toupper();
            m.value() = env(key(), m.value());
        };
    }
}
#endif //SUIL_BASE_ENV_HPP

//
// Created by Mpho Mbotho on 2020-10-08.
//

#ifndef SUIL_BASE_SIO_HPP
#define SUIL_BASE_SIO_HPP

#include "suil/utils/utils.hpp"

#include <iod/sio.hh>

namespace suil {

    static auto removeMembersWithAttr = [](const auto& o, const auto& a)
    {
        typedef std::decay_t<decltype(a)> A;
        return iod::foreach2(o) | [&] (auto& m) {
            typedef typename std::decay_t<decltype(m)>::attributes_type attrs;
            return iod::static_if<!iod::has_symbol<attrs,A>::value>(
                    [&](){ return m; },
                    [&](){}
            );
        };
    };

    static auto extractMembersWithAttr = [](const auto& o, const auto& a)
    {
        typedef std::decay_t<decltype(a)> A;
        return iod::foreach2(o) | [&] (auto& m) {
            typedef typename std::decay_t<decltype(m)>::attributes_type attrs;
            return iod::static_if<iod::has_symbol<attrs,A>::value>(
                    [&](){ return m; },
                    [&](){}
            );
        };
    };


    template<typename C, typename Opts>
    inline void applyOptions(C& o, Opts& opts) {
        /* the target object here is also an sio*/
        if (opts.size()) {
            iod::foreach(opts) |
            [&](auto &m) {
                /* use given metadata to to set options */
                m.symbol().member_access(o) = m.value();
            };
        }
    }

    template<typename C, typename... Opts>
    inline void applyConfig(C& obj, Opts... opts) {
        if constexpr (sizeof...(opts) > 0) {
            auto options = iod::D(opts...);
            applyOptions(obj, options);
        }
    }

    template<arithmetic T>
    inline void sioZero(T& o) {
        o = 0;
    }

    template<typename T>
    inline void sioZero(iod::Nullable<T>& o) {
        o.isNull = false;
    }

    template<typename T>
    inline void sioZero(T& o) {
    }

    inline void siozero(bool& o) {
        o = false;
    }

    template<typename... T>
    void sioZero(iod::sio<T...>& o) {
        iod::foreach(o) | [&](auto m) {
            sioZero(m.symbol().member_access(o));
        };
    }

    template <typename T>
    concept MetaWithSchema = requires {
        std::is_base_of_v<iod::MetaType, T>;
        typename T::Schema;
    };
}
#endif //SUIL_BASE_SIO_HPP

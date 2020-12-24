//
// Created by Mpho Mbotho on 2020-12-16.
//

#ifndef INCLUDE_SUIL_BASE_URL_HPP
#define INCLUDE_SUIL_BASE_URL_HPP

#include <suil/base/string.hpp>

namespace suil::URL {

    String encode(const void* data, size_t len);

    template <DataBuf T>
    inline String encode(const T& d) {
        return encode(d.data(), d.size());
    }

    String decode(const void* data, size_t len);

    template <DataBuf T>
    inline String decode(const T& d) {
        return decode(d.data(), d.size());
    }
}
#endif //INCLUDE_SUIL_BASE_URL_HPP

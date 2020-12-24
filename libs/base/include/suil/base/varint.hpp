//
// Created by Mpho Mbotho on 2020-10-15.
//

#ifndef SUIL_BASE_VARINT_HPP
#define SUIL_BASE_VARINT_HPP

#include "suil/base/utils.hpp"

#include <endian.h>
#include <cstdint>

namespace suil {

    class VarInt {
    public:
        VarInt(uint64_t v);

        VarInt();

        template <std::integral T>
        T read() const {
            return (T) be64toh(*((uint64_t *) Ego.mData));
        }

        template <std::integral T>
        void write(T v) {
            *((uint64_t *) Ego.mData) = htobe64((uint64_t) v);
        }

        template <typename T>
        inline VarInt& operator=(T v) {
            Ego.write(v);
            return Ego;
        }

        template <std::integral T>
        inline operator T() {
            return Ego.read<T>();
        }

        uint8_t *raw();

        uint8_t length() const;

        inline bool operator==(const VarInt& other) const {
            return *((uint64_t *) mData) == *((uint64_t *) other.mData);
        }

        inline bool operator!=(const VarInt& other) const {
            return *((uint64_t *) mData) != *((uint64_t *) other.mData);
        }

    private suil_ut:
        uint8_t mData[sizeof(uint64_t)];
    };

}

#endif //SUIL_BASE_VARINT_HPP

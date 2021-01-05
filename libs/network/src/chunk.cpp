//
// Created by Mpho Mbotho on 2020-12-17.
//

#include "suil/net/chunk.hpp"

namespace suil::net {

    Chunk::Chunk()
        : _useFd{false}
    {
        Ego._data.ptr = nullptr;
    }

    Chunk::Chunk(int fd, size_t sz, off_t off, FdDctor dctor)
        : _useFd{true},
          _off{off},
          _len{sz}
    {
        Ego._data.fd = fd;
        if (dctor) {
            Ego._dctor = [d = std::move(dctor)](Chunk& c) {
                if (c._data.fd > 0) {
                    d(c._data.fd);
                }
            };
        }
    }

    Chunk::Chunk(void* data, size_t sz, off_t off, PtrDctor dctor)
        : _useFd{false},
          _off{off},
          _len{sz}
    {
        Ego._data.ptr = data;
        if (dctor) {
            Ego._dctor = [d = std::move(dctor)](Chunk& c) {
                if (c._data.ptr != nullptr) {
                    d(c._data.ptr);
                }
            };
        }
    }

    Chunk::Chunk(Chunk&& other) noexcept
        : _useFd{std::exchange(other._useFd, -1)},
          _off{std::exchange(other._off, 0)},
          _len{std::exchange(other._len, 0)},
          _dctor{std::exchange(other._dctor, nullptr)}
    {
        Ego._data.ptr = std::exchange(other._data.ptr, nullptr);
    }

    Chunk& Chunk::operator=(Chunk&& other) noexcept
    {
        if (this == &other) {
            return Ego;
        }
        Ego._useFd = std::exchange(other._useFd, -1);
        Ego._off   = std::exchange(other._off, 0);
        Ego._len   = other._len;
        Ego._dctor = std::exchange(other._dctor, nullptr);
        Ego._data.ptr = std::exchange(other._data.ptr, nullptr);
        return Ego;
    }

    Chunk::~Chunk()
    {
        if (Ego._dctor != nullptr) {
            Ego._dctor(Ego);
            Ego._dctor = nullptr;
        }
    }
}
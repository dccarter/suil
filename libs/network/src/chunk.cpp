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
                if (c._data.fd > 0) {
                    d(c._data.ptr);
                }
            };
        }
    }

    Chunk::Chunk(Chunk&& other) noexcept
        : _useFd{other._useFd},
          _off{other._off},
          _len{other._len},
          _dctor{std::move(other._dctor)}
    {
        Ego._data.ptr = other._data.ptr;
        other._data.ptr = nullptr;
        other._off = 0;
        other._len = 0;
    }

    Chunk& Chunk::operator=(Chunk&& other) noexcept
    {
        Ego._useFd = other._useFd;
        Ego._off = other._off;
        Ego._len = other._len;
        Ego._dctor = std::move(other._dctor);
        Ego._data.ptr = other._data.ptr;
        other._data.ptr = nullptr;
        other._off = 0;
        other._len = 0;
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
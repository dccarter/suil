//
// Created by Mpho Mbotho on 2020-12-17.
//

#include "suil/net/chunk.hpp"

#include <suil/base/exception.hpp>

namespace suil::net {

    Chunk::Chunk()
        : _useFd{false}
    {
        Ego._data.ptr = nullptr;
    }

    Chunk::Chunk(size_t size)
        : _useFd{false},
          _start{0},
          _offset{0},
          _len{size}
    {
        _data.ptr = new uint8[size];
        Ego._dctor = [](Chunk& c) {
            if (c._data.ptr != nullptr) {
                delete[] static_cast<uint8 *>(c._data.ptr);
            }
        };
    }

    Chunk::Chunk(int fd, size_t sz, off_t off, FdDctor dctor)
        : _useFd{true},
          _start{off},
          _offset{off_t(off+sz)},
          _len{size_t(off+sz)}
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
          _start{off},
          _offset{off_t(off+sz)},
          _len{size_t(off+sz)}
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
        : _useFd{std::exchange(other._useFd, false)},
          _start{std::exchange(other._start, 0)},
          _len{std::exchange(other._len, 0)},
          _dctor{std::move(other._dctor)}
    {
        Ego._data.ptr = std::exchange(other._data.ptr, nullptr);
    }

    Chunk& Chunk::operator=(Chunk&& other) noexcept
    {
        if (unlikely(this == &other)) {
            return Ego;
        }

        Ego._useFd  = std::exchange(other._useFd, false);
        Ego._start  = std::exchange(other._start, 0);
        Ego._offset = std::exchange(other._offset, 0);
        Ego._len = std::exchange(other._len, 0);
        Ego._dctor = std::move(other._dctor);
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

    void Chunk::seek(off_t off, bool set)
    {
        auto comp = set? off: _offset + off;
        if ((comp < _start) or (off > _len)) {
            throw IndexOutOfBounds("chunk seek index '", off, "' is out of bounds");
        }
        _offset = comp;
    }

    uint8& Chunk::operator[](int index)
    {
        assertIndex(index);
        return data<uint8>()[index];
    }

    const uint8& Chunk::operator[](int index) const
    {
        assertIndex(index);
        return data<uint8>()[index];
    }

    void Chunk::assertIndex(int index) const
    {
        if (index < _start or index > _offset) {
            throw IndexOutOfBounds("chunk::[] index '", index, "' is out of bounds");
        }
    }
}
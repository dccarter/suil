//
// Created by Mpho Mbotho on 2020-12-17.
//

#ifndef SUIL_NET_CHUNK_HPP
#define SUIL_NET_CHUNK_HPP

#include <suil/base/utils.hpp>

#include <memory>

namespace suil::net {

    class Chunk {
    public:
        using FdDctor = std::function<void(int)>;
        using PtrDctor = std::function<void(void*)>;
        using Dctor = std::function<void(Chunk&)>;
        Chunk();
        Chunk(size_t);
        Chunk(void *data, size_t sz, off_t off = 0, PtrDctor dctor = nullptr);
        Chunk(int fd, size_t sz, off_t off = 0, FdDctor dctor = nullptr);
        MOVE_CTOR(Chunk) noexcept;
        MOVE_ASSIGN(Chunk) noexcept;

        inline int fd() const {
            return Ego._data.fd;
        }

        inline const void* ptr() const {
            return reinterpret_cast<const char*>(Ego._data.ptr) + Ego._start;
        }

        template <typename T>
        inline const T* data() const {
            if (!_useFd)
                return reinterpret_cast<const T*>((const uint8 *)_data.ptr + _start);
            return nullptr;
        }

        template <typename T>
        inline T* data() {
            if (!_useFd)
                return reinterpret_cast<T*>((uint8 *)_data.ptr + _start);
            return nullptr;
        }

        inline bool usesFd() const {
            return Ego._useFd;
        }

        inline off_t begin() const {
            return _start;
        }

        inline size_t size() const {
            return _offset - _start;
        }

        inline size_t capacity() const {
            return _len - _offset;
        }

        inline bool empty() const {
            return size() != 0;
        }

        void seek(off_t off, bool set = false);

        uint8& operator[](int);
        const uint8& operator[](int) const;

        ~Chunk();

    private:
        void assertIndex(int index) const;
        DISABLE_COPY(Chunk);
        union {
            void *ptr;
            int  fd;
        } _data;

        bool  _useFd{false};
        off_t  _start{0};
        off_t  _offset{0};
        size_t _len{0};
        Dctor  _dctor{nullptr};
    };
}
#endif //SUIL_NET_CHUNK_HPP

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
        Chunk(void *data, size_t sz, off_t off = 0, PtrDctor dctor = nullptr);
        Chunk(int fd, size_t sz, off_t off = 0, FdDctor dctor = nullptr);
        MOVE_CTOR(Chunk) noexcept;
        MOVE_ASSIGN(Chunk) noexcept;

        inline int fd() const {
            return Ego._data.fd;
        }

        inline const void* ptr() const {
            return reinterpret_cast<const char*>(Ego._data.ptr) + Ego._off;
        }

        inline bool usesFd() const {
            return Ego._useFd;
        }

        inline off_t offset() const {
            return Ego._off;
        }

        inline size_t size() const {
            return Ego._len;
        }

        inline bool empty() const {
            return Ego._len == 0;
        }

        ~Chunk();

    private:
        DISABLE_COPY(Chunk);
        union {
            void *ptr;
            int  fd;
        } _data;

        bool  _useFd{false};
        off_t _off{0};
        size_t _len{0};
        Dctor  _dctor{nullptr};
    };
}
#endif //SUIL_NET_CHUNK_HPP

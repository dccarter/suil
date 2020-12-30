//
// Created by Mpho Mbotho on 2020-12-29.
//

#ifndef SUIL_HTTP_CLIENT_MEMOFFLOAD_HPP
#define SUIL_HTTP_CLIENT_MEMOFFLOAD_HPP

#include <suil/http/client/response.hpp>

#include <suil/base/units.hpp>

#include <cstddef>

namespace suil::http::cli {

    class MemoryOffload {
    public:
        MemoryOffload(size_t mappedMin = 65_Kib);
        MOVE_CTOR(MemoryOffload) noexcept;
        MOVE_ASSIGN(MemoryOffload) noexcept;

        ~MemoryOffload();

        inline Response::Writer operator()() {
            return _handler;
        }

        inline String str() const {
            return String{_data, size_t(_offset), false};
        }

        inline Data data() const {
            return Data{reinterpret_cast<const uint8 *>(_data), size_t(_offset), false};
        }

    private:
        DISABLE_COPY(MemoryOffload);
        void mapRegion(size_t len);
        off_t  _offset{0};
        char* _data{nullptr};
        size_t _mappedMin{0};
        bool _isMapped{false};
        Response::Writer _handler{nullptr};
    };
}

#endif //SUIL_HTTP_CLIENT_MEMOFFLOAD_HPP

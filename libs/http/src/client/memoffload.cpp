//
// Created by Mpho Mbotho on 2020-12-29.
//

#include "suil/http/client/memoffload.hpp"

#include <sys/mman.h>

namespace suil::http::cli {

    MemoryOffload::MemoryOffload(size_t mappedMin)
        : _mappedMin{mappedMin}
    {
        _handler = [this](const char *at, size_t len) {
            if (at == nullptr and len > 0) {
                // initial condition
                if (len >= _mappedMin) {
                    mapRegion(len);
                }
                else {
                    _data = static_cast<char *>(malloc(len));
                    if (_data == nullptr) {
                        HttpException("allocating memory for response offload failed");
                    }
                }
            }
            else if (at != nullptr) {
                // append received data
                memcpy(&_data[_offset], at, len);
                _offset += len;
            }
            return true;
        };
    }

    MemoryOffload::MemoryOffload(MemoryOffload&& o) noexcept
        : _offset{std::exchange(o._offset, 0)},
          _data{std::exchange(o._data, nullptr)},
          _mappedMin{std::exchange(o._mappedMin, 0)},
          _isMapped{std::exchange(o._isMapped, false)},
          _handler{std::exchange(o._handler, nullptr)}
    {}

    MemoryOffload& MemoryOffload::operator=(MemoryOffload&& o) noexcept
    {
        if (this == &o) {
            return Ego;
        }

        _offset = std::exchange(o._offset, 0);
        _data = std::exchange(o._data, nullptr);
        _mappedMin = std::exchange(o._mappedMin, 0);
        _isMapped = std::exchange(o._isMapped, false);
        _handler = std::exchange(o._handler, nullptr);

        return Ego;
    }

    MemoryOffload::~MemoryOffload()
    {
        if (_data) {
            if (_isMapped) {
                // un-map mapped memory
                munmap(_data, _mappedMin);
            }
            else {
                // free allocated data
                ::free(_data);
            }
            _data = nullptr;
        }
    }

    void MemoryOffload::mapRegion(size_t len)
    {
        auto total = len;
        auto pageSize = getpagesize();
        total += pageSize - (len % pageSize);
        _data = (char *) mmap(nullptr, total, PROT_READ|PROT_WRITE, MAP_SHARED, -1, 0);
        if (_data == MAP_FAILED) {
            throw HttpException("MemoryOffload mmap(", total, ") failed: ", errno_s);
        }
        _mappedMin = total;
        _isMapped = true;
    }

}
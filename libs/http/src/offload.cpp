//
// Created by Mpho Mbotho on 2020-12-17.
//

#include "suil/http/offload.hpp"

#include <sys/mman.h>

namespace suil::http {

    FileOffload::FileOffload(FileOffload&& other)
        : File(std::move(other)),
          _mapped{other._mapped},
          _size{other._size}
    {
        other._mapped = nullptr;
        other._size = 0;
    }

    FileOffload & FileOffload::operator=(FileOffload&& other)
    {
        File::operator=(std::move(other));
        Ego._mapped = other._mapped;
        Ego._size = other._size;
        other._mapped = nullptr;
        other._size = 0;
        return Ego;
    }

    bool FileOffload::size(size_t& len) const
    {
        if (Ego.fd == nullptr) {
            return false;
        }

        struct stat s = {0};
        if (fstat(mfget(Ego.fd), & s) == -1) {
            return false;
        }
        len = s.st_size;

        return true;
    }

    Data FileOffload::data()
    {
        if (Ego._mapped != nullptr) {
            return Data{Ego._mapped, Ego._size, false};
        }

        if (!Ego.size(Ego._size)) {
            // need size to continue
            return Data{};
        }

        Ego._mapped = mmap(nullptr, Ego._size, PROT_READ, MAP_SHARED, mfget(Ego.fd), 0);
        if (Ego._mapped == MAP_FAILED) {
            Ego._size = 0;
            Ego._mapped = nullptr;
            return Data{};
        }
        return Data{Ego._mapped, Ego._size, false};
    }

    String FileOffload::data() const {
        return String{static_cast<const char*>(Ego._mapped), Ego._size, false};
    }

    FileOffload::~FileOffload() noexcept
    {
        Ego.close();
    }

    void FileOffload::close()
    {
        if (Ego._mapped != nullptr) {
            munmap(Ego._mapped, Ego._size);
            Ego._mapped = nullptr;
            Ego._size = 0;
        }
        File::close();
    }
}
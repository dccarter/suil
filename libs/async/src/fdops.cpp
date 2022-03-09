/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-01-08
 */


#include "suil/async/fdops.hpp"

#include <system_error>

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

namespace suil {

    namespace fdops {

        void nonblocking(int fd, bool on)
        {
            auto opts = fcntl(fd, F_GETFL);
            if (fd < 0) {
                throw std::system_error{errno, std::system_category()};
            }

            opts = on ? (opts | O_NONBLOCK) : (opts & ~O_NONBLOCK);
            if (fcntl(fd, F_SETFL, opts)) {
                throw std::system_error{errno, std::system_category()};
            }
        }

        auto read(int fd, std::span<char> buf, std::chrono::milliseconds timeout) -> task<int>
        {
            if (fd < 0 || buf.data() == nullptr || buf.size() == 0) {
                errno =  EINVAL;
                co_return -1;
            }

            auto deadline = after(timeout);
            ssize_t nRead{0};

            do {
                nRead = ::read(fd, buf.data(), buf.size());
                if (nRead < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        auto res = co_await fdwait(fd, Event::IN, deadline);
                        if (res != Event::esFIRED) {
                            break;
                        }

                        continue;
                    }
                }
                break;
            } while (true);

            co_return int(nRead);
        }

        auto write(int fd, const std::span<const char> &buf, std::chrono::milliseconds timeout) -> task<int>
        {
            if (fd < 0 || buf.data() == nullptr) {
                errno = EINVAL;
                co_return -1;
            }

            auto deadline = after(timeout);
            ssize_t written{0};

            do {
                written = ::write(fd, buf.data(), buf.size());
                if (written < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        auto res = co_await fdwait(fd, Event::OUT, deadline);
                        if (res != Event::esFIRED) {
                            break;
                        }
\
                        continue;
                    }
                }
                break;
            } while (true);

            co_return int(written);
        }
    }
}
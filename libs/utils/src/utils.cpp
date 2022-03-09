/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-14
 */

#include "suil/utils/utils.hpp"
#include "suil/utils/exception.hpp"

#include <fcntl.h>
#include <openssl/rand.h>

namespace suil {

    uint8_t c2i(char c) {
        if (c >= '0' && c <= '9') {
            return (uint8_t) (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            return (uint8_t) (c - 'W');
        } else if (c >= 'A' && c <= 'F') {
            return (uint8_t) (c - '7');
        }
        throw OutOfRange("utils::c2i - character out range");
    };

    char i2c(uint8_t c, bool caps) {

        if (c <= 0x9) {
            return char(c) + '0';
        }
        else if (c <= 0xF) {
            if (caps)
                return char(c) + '7';
            else
                return char(c) + 'W';
        }
        throw OutOfRange(AnT(), "byte out of range");
    };

    void randbytes(void *out, size_t size)
    {
        RAND_bytes(static_cast<uint8_t *>(out), (int) size);
    }

    size_t hexstr(const void *in, size_t iLen, char *out, size_t oLen)
    {
        if ((in == nullptr) || (out == nullptr) || (oLen < (iLen<<1)))
            return 0;

        size_t rc = 0;
        for (size_t i = 0; i < iLen; i++) {
            out[rc++] = i2c((uint8_t) (0x0F&(static_cast<const uint8_t *>(in)[i]>>4)), false);
            out[rc++] = i2c((uint8_t) (0x0F&static_cast<const uint8_t *>(in)[i]), false);
        }
        return rc;
    }

    void reverse(void *data, size_t len)
    {
        auto bytes = static_cast<uint8_t *>(data);
        size_t i;
        const size_t stop = len >> 1;
        for (i = 0; i < stop; ++i) {
            uint8_t *left = bytes + i;
            uint8_t *right = bytes + len - i - 1;
            const uint8_t tmp = *left;
            *left = *right;
            *right = tmp;
        }
    }

    int nonblocking(int fd, bool on)
    {
        int opt = fcntl(fd, F_GETFL, 0);
        if (opt == -1)
            opt = 0;
        opt = on? opt | O_NONBLOCK : opt & ~O_NONBLOCK;
        return fcntl(fd, F_SETFL, opt);
    }

    uint16_t  mtid() noexcept
    {
        static std::atomic<uint16_t> generator{0};
        static __thread uint16_t threadId{0};
        return threadId || (threadId = ++generator);
    }
}

#ifdef SXY_UNITTEST
#include "catch2/catch.hpp"

TEST_CASE("Using defer macros", "[defer][vdefer]") {
    WHEN("Using defer macro") {
        int num{0};
        {
            defer({
                num = -1;
            });
            REQUIRE(num == 0);
            num = 100;
        }
        REQUIRE(num == -1);

        bool thrown{false};
        auto onThrow = [&thrown] {
            defer({
                thrown = true;
            });
            throw suil::Exception{""};
        };
        REQUIRE_THROWS(onThrow());
        REQUIRE(thrown);
    }

    WHEN("Using vdefer macro") {
        int num{0};
        {
            vdefer(num, {
                num = -1;
            });
            REQUIRE(num == 0);
            num = 100;
        }
        REQUIRE(num == -1);
    }
}

TEST_CASE("Using scoped macro", "[scoped]") {
    struct Scope {
        bool open{true};
        void close() {
            open = false;
        }
    };

    Scope sp;
    REQUIRE(sp.open);
    {
        scoped(_sp, sp);
        REQUIRE(_sp.open);
    }
    REQUIRE_FALSE(sp.open);

    Scope sp2;
    REQUIRE(sp2.open);
    auto onThrow = [&sp2] {
        scope(sp2);
        throw suil::Exception{""};
    };
    REQUIRE_THROWS(onThrow());
    REQUIRE_FALSE(sp2.open);
}

TEST_CASE("Using c2i/i2c api's", "[c2i][i2c]") {
    const std::size_t COUNT{sizeofcstr("0123456789abcdef")};
    const std::array<char, COUNT> HC = {
            '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
    };
    const std::array<char, COUNT> HS = {
            '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
    };

    WHEN("Converting character to byte") {
        for (int i = 0; i < COUNT; i++) {
            REQUIRE(suil::c2i(HC[i]) == i);
            REQUIRE(suil::c2i(HS[i]) == i);
        }
    }

    WHEN("Converting a byte to a character") {
        for (int i = 0; i < COUNT; i++) {
            REQUIRE(suil::i2c(i, true) == HC[i]);
            REQUIRE(suil::i2c(i) == HS[i]);
        }
    }
}
#endif

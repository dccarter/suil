//
// Created by Mpho Mbotho on 2020-10-05.
//

#include "suil/base/utils.hpp"
#include "suil/base/exception.hpp"
#include "libmill/libmill.h"

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
            return c + '0';
        }
        else if (c <= 0xF) {
            if (caps)
                return c + '7';
            else
                return c + 'W';
        }
        throw OutOfRange("utils::i2c - byte out of range");
    };

    int64_t absolute(int64_t tout)
    {
        return tout < 0? -1 : mnow() + tout;
    }

    void randbytes(uint8_t *out, size_t size)
    {
        RAND_bytes(out, (int) size);
    }

    size_t hexstr(const uint8_t *in, size_t ilen, char *out, size_t olen)
    {
        if ((in == nullptr) || (out == nullptr) || (olen < (ilen<<1)))
            return 0;

        size_t rc = 0;
        for (size_t i = 0; i < ilen; i++) {
            out[rc++] = i2c((uint8_t) (0x0F&(in[i]>>4)));
            out[rc++] = i2c((uint8_t) (0x0F&in[i]));
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

    Deadline::Deadline(std::int64_t timeout)
        : value{timeout < 0? -1: mnow() + timeout}
    {}

    Deadline::operator bool() const
    {
        return value < 0? false: mnow() >= value;
    }
}

#ifdef SUIL_UNITTEST
#include "catch2/catch.hpp"
namespace sb = suil;

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
            throw sb::Exception{""};
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
        throw sb::Exception{""};
    };
    REQUIRE_THROWS(onThrow());
    REQUIRE_FALSE(sp2.open);
}

TEST_CASE("Using absolute api", "[absolute]") {
    REQUIRE(sb::absolute(-1) == -1);
    REQUIRE(mnow() <= sb::absolute(0));
    REQUIRE(mnow() <= sb::absolute(100));
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
            REQUIRE(sb::c2i(HC[i]) == i);
            REQUIRE(sb::c2i(HS[i]) == i);
        }
    }

    WHEN("Converting a byte to a character") {
        for (int i = 0; i < COUNT; i++) {
            REQUIRE(sb::i2c(i, true) == HC[i]);
            REQUIRE(sb::i2c(i) == HS[i]);
        }
    }
}
#endif

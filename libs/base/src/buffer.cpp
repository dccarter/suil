//
// Created by Mpho Mbotho on 2020-10-07.
//


#include "suil/base/buffer.hpp"
#include "suil/base/datetime.hpp"
#include "suil/base/string.hpp"

#include <suil/utils/exception.hpp>

#include <algorithm>
#include <cstdarg>

namespace suil {

    uint8_t& Buffer::operator[](size_t index)
    {
        if (index <= m_offset) {
            return m_data[index];
        }
        throw IndexOutOfBounds("index '", index, "' is too large");
    }

    Buffer::Buffer(size_t is)
        : m_data{nullptr},
          m_size(0),
          m_offset(0)
    {
        if (is)
            grow((uint32_t) is);
    }

    Buffer::Buffer(Buffer && other) noexcept
        : m_data(other.m_data),
          m_size(other.m_size),
          m_offset(other.m_offset)
    {
        other.m_size = 0;
        other.m_offset = 0;
        other.m_data = nullptr;
    }

    Buffer& Buffer::operator=(Buffer &&other) noexcept {
        if (this != &other) {
            if (m_data != nullptr)
                free(m_data);

            m_size = other.m_size;
            m_data = other.m_data;
            m_offset = other.m_offset;
            other.m_data = nullptr;
            other.m_offset = other.m_size = 0;
        }
        return Ego;
    }

    Buffer::~Buffer() {
        if (m_data != nullptr) {
            free(m_data);
        }
        m_data = nullptr;
        m_size = 0;
        m_offset = 0;
    }

    ssize_t Buffer::appendf(const char *fmt, ...) {
        ssize_t nwr{-1};
        va_list args;
        va_start(args, fmt);
        nwr = appendv(fmt, args);
        va_end(args);
        return nwr;
    }

    ssize_t Buffer::appendv(const char *fmt, va_list args) {
        char	sb[2048];
        int ret;
        ret = vsnprintf(sb, sizeof(sb), fmt, args);
        if (ret == -1) {
            throw AccessViolation("Buffer::appendv(): ", errno_s);
        }
        return append(sb, (uint32_t) ret);
    }

    ssize_t Buffer::appendnf(uint32_t hint, const char *fmt, ...) {
        ssize_t nwr{-1};
        va_list args;
        va_start(args, fmt);
        nwr = appendnv(hint, fmt, args);
        va_end(args);
        return  nwr;
    }

    ssize_t Buffer::appendnv(uint32_t hint, const char *fmt, va_list args)
    {
        if ((m_size-m_offset) < hint)
            grow((uint32_t) std::max(m_size, hint));

        int ret;
        ret = vsnprintf((char *)(m_data+m_offset), (m_size-m_offset), fmt, args);
        if (ret == -1 || (ret + m_offset) > m_size) {
            throw AccessViolation("Buffer::appendv - ", errno_s);
        }
        m_offset += ret;
        return ret;
    }

    ssize_t Buffer::append(const void *data, size_t len) {
        if (len && data == nullptr)
            throw AccessViolation("Buffer::append - data cannot be null");

        if ((m_size-m_offset) < len) {
            grow((uint32_t) std::max(m_size, uint32_t(len)));
        }

        memcpy((m_data+m_offset), data, len);
        m_offset += len;
        return len;
    }

    ssize_t Buffer::append(time_t t, const char *fmt) {
        // reserve 64 bytes
        reserve(64);
        const char *pfmt = fmt? fmt : Datetime::HTTP_FMT;
        ssize_t sz = strftime((char*)(m_data+m_offset), 64, pfmt, localtime(&t));
        if (sz < 0)
            throw AccessViolation("Buffer::appendv - ", errno_s);

        m_offset += sz;
        return sz;
    }

#define __ALIGNED(x) ((x)&(sizeof(void*)-1))
#define __ALIGN2(x, d) ((d)? (sizeof(void*)-(d)) : (0))
#define __ALIGN(x) __ALIGN2(x, __ALIGNED(x))

    void Buffer::grow(uint32_t add) {
        // check if the current Buffer fits
        if (add == 0) {
            return;
        }
        add += __ALIGN(add);
        m_data = reallocate(m_data, m_size,(m_size+add+1));
        if (m_data == nullptr)
            throw MemoryAllocationFailure(
                    "Buffer::grow failed ", errno_s);
        // change the size of the memory
        m_size = uint32_t(m_size+add);
    }

    void Buffer::reset(size_t size, bool keep) {
        m_offset = 0;
        if (!keep || (m_size < size)) {
            size = (size < 8) ? 8 : size;
            size += __ALIGN(size);
            m_data = reallocate(m_data, m_size,(size+1));
            if (m_data == nullptr)
                throw MemoryAllocationFailure(
                        "Buffer::grow failed ", errno_s);
            // change the size of the memory
            m_size = uint32_t(size);
        }
    }

    void Buffer::seek(off_t off = 0) {
        off_t to = m_offset + off;
        bseek(to);
    }

    void Buffer::bseek(off_t off) {
        if ((off >= 0) && (m_size >= off)) {
            m_offset = (uint32_t) off;
        }
        else {
            throw OutOfRange("Buffer::bseek - seek offset out of range");
        }
    }

    void Buffer::reserve(size_t size) {
        size_t  remaining = m_size - m_offset;
        if (size > remaining) {
            auto to = size - remaining;
            grow(std::max(m_size, uint32_t(to)));
        }
    }

    char* Buffer::release() {
        if (m_data) {
            char *raw = (char *) (*this);
            m_data = nullptr;
            clear();
            return raw;
        }
        return nullptr;
    }

    void Buffer::clear() {
        if (m_data) {
            ::free(m_data);
            m_data = nullptr;
        }
        m_offset = 0;
        m_size = 0;
    }

    Buffer::operator char*() {
        if (m_data && m_data[m_offset] != '\0') {
            m_data[m_offset] = '\0';
        }
        return (char *) m_data;
    }

    Buffer& Buffer::operator<<(const String &other) {
        if (other) append(other.c_str(), other.size());
        return *this;
    }

    ssize_t Buffer::hex(const void *data, size_t size, bool caps) {
        reserve(size<<1);

        size_t rc = m_offset;
        const char *in = (char *)data;
        for (size_t i = 0; i < size; i++) {
            (char &) m_data[m_offset++] = i2c((uint8_t) (0x0F&(in[i]>>4)), caps);
            (char &) m_data[m_offset++] = i2c((uint8_t) (0x0F&in[i]), caps);
        }

        return m_offset - rc;
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

namespace sb = suil;

#define __TALIGN(x) ((x)+ __ALIGN(x))

static_assert(__TALIGN(3)  == sizeof(void*));
static_assert(__TALIGN(8)  == sizeof(void*));
static_assert(__TALIGN(11) == 2*sizeof(void*));

auto Check = [](sb::Buffer& ob, ssize_t from, const void *data, size_t count) {
    if ((from + count) > ob.m_offset) return false;
    return memcmp(data, &ob[(int)from], count) == 0;
};

TEST_CASE("sb::Buffer tests", "[common][buffer]")
{
    SECTION("Creating a buffer") {
        sb::Buffer ob;
        REQUIRE(ob.m_size   == 0);
        REQUIRE(ob.m_data   == nullptr);
        REQUIRE(ob.m_offset == 0);

        sb::Buffer ob2(64);
        REQUIRE(ob2.m_size == __TALIGN(64));
        REQUIRE(ob2.m_offset == 0);
        REQUIRE(ob2.m_data != nullptr);

        ob2 = sb::Buffer(63);
        REQUIRE(ob2.m_size == __TALIGN(63u));
        REQUIRE(ob2.m_offset == 0);
        REQUIRE(ob2.m_data != nullptr);

        sb::Buffer ob3 = std::move(ob2);
        // buffer should have been moved
        REQUIRE(ob3.m_size == __TALIGN(63u));
        REQUIRE(ob3.m_offset == 0);
        REQUIRE(ob3.m_data != nullptr);
        REQUIRE(ob2.m_size == 0);
        REQUIRE(ob2.m_offset == 0);
        REQUIRE(ob2.m_data == nullptr);

        sb::Buffer ob4(std::move(ob3));
        // buffer should have been moved
        REQUIRE(ob4.m_size == __TALIGN(63u));
        REQUIRE(ob4.m_offset == 0);
        REQUIRE(ob4.m_data != nullptr);
        REQUIRE(ob3.m_size == 0);
        REQUIRE(ob3.m_offset == 0);
        REQUIRE(ob3.m_data == nullptr);
    }

    SECTION("Appending to a buffer", "[buffer][append]") {
        // test append to a buffer
        sb::Buffer ob(128);
        WHEN("binary appending data") {
            // test appending numbers in binary format
            char c{(char )0x89};
            auto appendCheck = [&](auto t) {
                ssize_t  ss{0}, from = ob.m_offset;
                ss = ob.append(t);
                REQUIRE(ss == sizeof(t));
                REQUIRE(Check(ob, from, &t, sizeof(t)));
            };

            appendCheck((char)               0x89);
            appendCheck((short)              0x89);
            appendCheck((int)                0x89);
            appendCheck((int64_t)            0x89);
            appendCheck((unsigned char)      0x89);
            appendCheck((unsigned short)     0x89);
            appendCheck((unsigned int)       0x89);
            appendCheck((uint64_t)           0x89);
            appendCheck((float)              0.99);
            appendCheck((double)             0.99);

            struct Bin {
                int    a{0};
                float  b{10};
                double c{0.0};
            };

            Bin bin{};
            ssize_t ss = 0, from = ob.m_offset;
            ss = ob.append(&bin, sizeof(bin));
            REQUIRE(ss == sizeof(Bin));
            REQUIRE(Check(ob, from, &bin, sizeof(bin)));
        }

        WHEN("appending strings to buffer") {
            // clear buffer
            ob.clear();

            const char *cstr{"Hello World"};
            sb::std::string_view svstr{cstr, strlen(cstr)};
            std::string str(cstr);

            ssize_t from{ob.m_offset};
            auto  ss = ob.append(cstr);
            REQUIRE(ss == strlen(cstr));
            REQUIRE(Check(ob, from, cstr, ss));

            from = ob.m_offset;
            ss = ob.append(svstr);
            REQUIRE(ss == svstr.size());
            REQUIRE(Check(ob, from, &svstr[0], ss));

            from = ob.m_offset;
            ss = ob.append(str);
            REQUIRE(ss == str.size());
            REQUIRE(Check(ob, from, &str[0], ss));
        }
    }

    SECTION("Growing buffer", "[buffer][grow]") {
        // when growing buffer
        sb::Buffer ob(16);
        REQUIRE(ob.m_size == __TALIGN(16u));
        auto ss = ob.append("0123456789123456");
        REQUIRE(ob.m_offset == 16);
        REQUIRE(ss == 16);
        REQUIRE(ob.m_size == 16);
        // append to buffer will force buffer to grow
        ss = ob.append("0123456789123456");
        REQUIRE(ob.m_offset == 32);
        REQUIRE(ss == 16);
        REQUIRE(ob.m_size == 32);
        // again, but now must grow buffer twice
        ss = ob.append("0123456789123456");
        REQUIRE(ob.m_offset == 48);
        REQUIRE(ss == 16);
        REQUIRE(ob.m_size == 64);
        // buffer should not grow, there is still room
        ss = ob.append("012345678912345");
        REQUIRE(ob.m_offset == 63);
        REQUIRE(ss == 15);
        REQUIRE(ob.m_size == 64);
        // buffer should grow, there is no room
        ss = ob.append("01");
        REQUIRE(ob.m_offset == 65);
        REQUIRE(ss == 2);
        REQUIRE(ob.m_size == 128);
    }

    SECTION("printf-style operations", "[appendf][appendnf]") {
        // test writing to buffer using printf-style API
        sb::Buffer ob(128);
        auto checkAppendf = [&](const char *fmt, auto... args) {
            char expected[128];
            ssize_t  sz = snprintf(expected, sizeof(expected), fmt, args...);
            ssize_t ss{0}, from = ob.m_offset;
            ss = ob.appendf(fmt, args...);
            REQUIRE(sz == ss);
            REQUIRE(Check(ob, from, expected, ss));
        };

        checkAppendf("%d",     8);
        checkAppendf("%lu",    8lu);
        checkAppendf("%0.4f",  44.33333f);
        checkAppendf("%p",     nullptr);
        checkAppendf("%c",     'C');
        checkAppendf("Hello %s, your age is %d", "Carter", 27);

        // appendnf is a special case as it will throw an exception
        sb::Buffer ob2(16);
        REQUIRE_NOTHROW(ob2.appendnf(8, "%d", 1));
        // this should throw because the end formatted string will not
        // fit into buffer
        REQUIRE_THROWS(ob2.appendnf(8, "%s", "0123456789123456"));
    }

    SECTION("Stream and add operators", "[stream][add]") {
        // test using stream operators on buffer
        sb::Buffer ob(128);
        auto testStreamOperator = [&](sb::Buffer& ob, auto in, const char *expected) {
            ssize_t  ss{0}, from{ob.m_offset}, len = strlen(expected);
            ob << in;
            ss = ob.m_offset-from;
            REQUIRE(ss == len);
            REQUIRE(Check(ob, from, expected, len));
        };

        std::string str("Hello World");
        sb::Buffer other{16};
        other.append(str);

        testStreamOperator(ob, 'c',                      "c");
        testStreamOperator(ob,  (short)1,                "1");
        testStreamOperator(ob,  -12,                     "-12");
        testStreamOperator(ob,  (int64_t)123,            "123");
        testStreamOperator(ob,  (unsigned char)1,        "1");
        testStreamOperator(ob,  (unsigned short)45,      "45");
        testStreamOperator(ob,  (unsigned int)23,        "23");
        testStreamOperator(ob,  (int64_t)1,              "1");
        testStreamOperator(ob,  1.6678f,                 "1.667800");
        testStreamOperator(ob,  1.3e-2,                  "0.013000");
        testStreamOperator(ob,  "Hello",                 "Hello");
        testStreamOperator(ob,  str,                     "Hello World");
        testStreamOperator(ob,  sb::std::string_view(str),            "Hello World");
        testStreamOperator(ob,  true,                    "true");
        testStreamOperator(ob,  std::move(other),        "Hello World");
        testStreamOperator(ob,  sb::fmtbool(true),           "True");
        testStreamOperator(ob,  sb::fmtbool(false),          "False");
        testStreamOperator(ob,  sb::fmtnum("%04x",  0x20),    "0020");
        testStreamOperator(ob,  sb::fmtnum("%0.4f", 1.2),     "1.2000");

        auto testAddOperator = [&](sb::Buffer& ob, auto in, const char *expected) {
            ssize_t  ss{0}, from{ob.m_offset}, len = strlen(expected);
            ob += in;
            ss = ob.m_offset-from;
            REQUIRE(ss == len);
            REQUIRE(Check(ob, from, expected, len));
        };

        testAddOperator(ob,  str,                 "Hello World");
        testAddOperator(ob,  sb::std::string_view{str},        "Hello World");
        testAddOperator(ob,  "Hello world again", "Hello world again");

    }

    SECTION("Other buffer methods", "[reset][seek][bseek][release][clear][empty][capacity][size]") {
        // tests some other methods ot the sb::Buffer
        sb::Buffer b(16);
        REQUIRE(b.empty());
        b << "Hello";
        REQUIRE_FALSE(b.empty());

        WHEN("Getting buffer capacity") {
            // sb::Buffer::capacity() must return current available capacity
            sb::Buffer ob{16};
            REQUIRE(ob.capacity() == __TALIGN(16u));
            ob << "01234";
            REQUIRE(ob.capacity() == 11);
            ob << "56789123456";
            REQUIRE(ob.capacity() == 0);
            ob << 'c';
            REQUIRE(ob.capacity() == 15);
        }

        WHEN("Getting buffer size") {
            // sb::Buffer::size() must return current number of bytes written into buffer
            sb::Buffer ob(16);
            REQUIRE(ob.size() == 0);
            ob << "01234";
            REQUIRE(ob.size() == 5);
            ob << "5678912345678";
            REQUIRE(ob.size() == 18);
        }

        WHEN("Clearing/Reseting buffer contents") {
            // OButter::[clear/reset] clears the buffer
            sb::Buffer ob{16};
            ob << "Hello";
            REQUIRE(ob.size() == 5);
            REQUIRE(ob.capacity() == 11);
            ob.clear();
            REQUIRE(ob.m_size == 0);
            REQUIRE(ob.m_offset == 0);
            REQUIRE(ob.m_data == nullptr);

            ob = sb::Buffer(16);
            ob << "Hello";
            REQUIRE(ob.size() == 5);
            REQUIRE(ob.capacity() == 11);
            const uint8_t* old = ob.m_data;
            ob.reset(7); // must relocate buffer since keep isn't specified
            REQUIRE(ob.m_offset == 0);
            REQUIRE(ob.m_size == __TALIGN(7U));
            REQUIRE(ob.m_data == old); // realloc 8 fits into 8 so you get the same

            ob = sb::Buffer(16);
            ob << "Hello";
            old = ob.m_data;
            ob.reset(7, true);  // must not relocate buffer
            REQUIRE(ob.m_offset == 0);
            REQUIRE(ob.m_size   == __TALIGN(16U));
            REQUIRE(old == ob.m_data);

            ob << "Hello";
            ob.reset(17, true); // must grow buffer since size is larger
            REQUIRE(ob.m_offset == 0);
            REQUIRE(ob.m_size   == __TALIGN(17U));
        }

        WHEN("Seeking with buffer") {
            // sb::Buffer::[seek/bseek]
            sb::Buffer ob{16};
            ob.bseek(5);
            REQUIRE(ob.m_offset == 5);
            REQUIRE_THROWS(ob.bseek(-1));   // does not work with negative numbers
            REQUIRE(ob.m_offset == 5);
            REQUIRE_THROWS(ob.bseek(ob.m_size+1)); // cannot seek past the buffer size
            REQUIRE(ob.m_offset == 5);
            ob.bseek(0);
            REQUIRE(ob.m_offset == 0);

            ob.seek(10);
            REQUIRE(ob.m_offset == 10);
            REQUIRE_NOTHROW(ob.seek(-5));   // can seek backwards from current cursor
            REQUIRE(ob.m_offset == 5);
            REQUIRE_NOTHROW(ob.seek(7));    // always seeks from current cursor
            REQUIRE(ob.m_offset == 12);
            REQUIRE_THROWS(ob.seek(7));     // cannot seek past buffer size
            REQUIRE(ob.m_offset == 12);
            REQUIRE_THROWS(ob.seek(-13));   // cannot seek to negative offset
            REQUIRE(ob.m_offset == 12);
        }

        WHEN("Releasing buffer") {
            // releasing buffer gives the buffer memory and resets
            sb::Buffer ob{16};
            ob << "Hello Spain";
            REQUIRE_FALSE(ob.empty());
            auto buf = ob.release();
            REQUIRE_FALSE(buf == nullptr);
            REQUIRE(ob.m_data == nullptr);
            REQUIRE(ob.m_size == 0);
            REQUIRE(ob.m_offset == 0);
            REQUIRE(strcmp(buf, "Hello Spain") == 0);
            ::free(buf);
        }

        WHEN("Reserving space") {
            // memory can be reserved on buffer
            sb::Buffer ob{16};
            ob << "0123456789123456";
            REQUIRE(ob.capacity() == 0);
            ob.reserve(17);
            REQUIRE(ob.capacity() == 24);
        }

        WHEN("Appending hex to buffer") {
            // data can be hex printed onto buffer
            sb::Buffer ob{16};
            auto ss = ob.hex("Hello");
            REQUIRE(ss == 10);
            REQUIRE(Check(ob, 0, "48656c6c6f", ss));
            std::string str{"Hello"};
            ss = ob.hex(str, true);
            REQUIRE(ss == 10);
            REQUIRE(Check(ob, 10, "48656C6C6F", ss));
        }

        WHEN("Using other buffer operators") {
            sb::Buffer ob{16};
            char a[] = {'C', 'a', 'r', 't', 'e', 'r'};
            ob.append(a, sizeof(a));
            REQUIRE(ob.size() == sizeof(a));
            REQUIRE(Check(ob, 0, a, sizeof(a)));
            // buffer will be string-fied on cast
            char *data = static_cast<char *>(ob);
            REQUIRE(data[ob.m_offset] == '\0');

            ob.reset(16, true);
            ob << "Hello World";
            auto sv = sb::std::string_view(ob);
            REQUIRE(Check(ob, 0, &sv[0], sv.size()));
            auto str = std::string(ob);
            REQUIRE(Check(ob, 0, &str[0], str.size()));
        }
    }
}

#undef __TALIGN
#undef __ALIGN
#undef __ALIGN2
#undef __ALIGNED

#endif

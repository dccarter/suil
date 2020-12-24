//
// Created by Mpho Mbotho on 2020-12-16.
//

#include "suil/base/url.hpp"

namespace {

    char *urlDecode(const char *src, const int src_len, char *out, size_t& out_sz)
    {
#define IS_HEX_CHAR(ch) \
        (((ch) >= '0' && (ch) <= '9') || \
         ((ch) >= 'a' && (ch) <= 'f') || \
         ((ch) >= 'A' && (ch) <= 'F'))

#define HEX_VALUE(ch, value) \
        if ((ch) >= '0' && (ch) <= '9') \
        { \
            (value) = (ch) - '0'; \
        } \
        else if ((ch) >= 'a' && (ch) <= 'f') \
        { \
            (value) = (ch) - 'a' + 10; \
        } \
        else \
        { \
            (value) = (ch) - 'A' + 10; \
        }

        const unsigned char *start;
        const unsigned char *end;
        char *dest;
        unsigned char c_high;
        unsigned char c_low;
        int v_high;
        int v_low;

        dest = out;
        start = (unsigned char *)src;
        end = (unsigned char *)src + src_len;
        while (start < end)
        {
            if (*start == '%' && start + 2 < end)
            {
                c_high = *(start + 1);
                c_low  = *(start + 2);

                if (IS_HEX_CHAR(c_high) && IS_HEX_CHAR(c_low))
                {
                    HEX_VALUE(c_high, v_high);
                    HEX_VALUE(c_low, v_low);
                    *dest++ = (char) ((v_high << 4) | v_low);
                    start += 3;
                }
                else
                {
                    *dest++ = *start;
                    start++;
                }
            }
            else if (*start == '+')
            {
                *dest++ = ' ';
                start++;
            }
            else
            {
                *dest++ = *start;
                start++;
            }
        }

#undef IS_HEX_CHAR
#undef HEX_VALUE

        out_sz = (int) (dest - out);
        return dest;
    }
}

namespace suil {

    String URL::encode(const void* data, size_t len)
    {
        auto *buf = static_cast<uint8_t *>(malloc(len*3));
        const char *src = static_cast<const char *>(data), *end = src + len;
        uint8_t *dst = buf;
        uint8_t c;
        while (src != end) {
            c = (uint8_t) *src++;
            if (!isalnum(c) && strchr("-_.~", c) == nullptr) {
                static uint8_t hexchars[] = "0123456789ABCDEF";
                dst[0] = '%';
                dst[1] = hexchars[(c&0xF0)>>4];
                dst[2] = hexchars[(c&0x0F)];
                dst += 3;
            }
            else {
                *dst++ = c;
            }
        }
        *dst = '\0';

        return String{reinterpret_cast<char *>(buf), size_t(dst - buf), true};
    }

    String URL::decode(const void* data, size_t len)
    {
        auto out = static_cast<char *>(malloc(len));
        size_t size{len};
        (void)urlDecode(static_cast<const char*>(data), (int)len, out, size);
        return String{out, size, true};
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>
#endif
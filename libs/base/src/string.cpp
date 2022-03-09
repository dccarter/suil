//
// Created by Mpho Mbotho on 2020-10-07.
//

#include "suil/base/string.hpp"
#include "suil/base/buffer.hpp"

#include <suil/utils/exception.hpp>

#include <openssl/rand.h>

namespace suil {

    String::String()
        : m_str(),
          m_len(0),
          m_own(false)
    {}

    String::String(const char *str) noexcept
        : m_cstr(str),
          m_len((uint32_t) (str ? strlen(str) : 0)),
          m_own(false)
    {}

    String::String(std::string_view str)
        : m_cstr(str.data()),
          m_len((uint32_t) (str.size())),
          m_own(false)
    {}

    String::String(const std::string &str, bool own)
        : m_cstr(own ? duplicate(str.data(), str.size()) : str.data()),
          m_len((uint32_t) (str.size())),
          m_own(own)
    {}

    String::String(const char *str, size_t len, bool own)
        : m_cstr(str),
          m_len((uint32_t) len),
          m_own(own)
    {}

    String::String(char c, size_t n)
        : m_str(static_cast<char*>(malloc(n+1))),
          m_own(true),
          m_len((uint32_t) n)
    {
        memset(m_str, c, n);
        m_str[n] = '\0';
    }

    String::String(Buffer &b, bool own)
    {
        m_len = uint32(b.size());
        m_own = own;
        m_str = own? b.release() : b.data();
    }

    String::String(const Buffer& b)
        : m_len{uint32(b.size())},
          m_own{false},
          m_str{b.data()}
    {}

    String::String(String &&s) noexcept
            : m_str(std::exchange(s.m_str, nullptr)),
              m_len(std::exchange(s.m_len, 0)),
              m_own(std::exchange(s.m_own, false)),
              m_hash(std::exchange(s.m_hash, 0))
    {}

    String &String::operator=(String &&s) noexcept {
        if (this == &s) {
            return Ego;
        }

        if (m_str && m_own) {
            ::free(m_str);
        }

        m_str = std::exchange(s.m_str, nullptr);
        m_len = std::exchange(s.m_len, 0);
        m_own = std::exchange(s.m_own, false);
        m_hash = std::exchange(s.m_hash, 0);

        return Ego;
    }

    String::String(const String &s)
        : m_str{duplicate(s.m_str, s.m_len)},
          m_len(s.m_len),
          m_own(true),
          m_hash(s.m_hash)
    {}

    String& String::operator=(const String &s) {
        if (this == &s) {
            return Ego;
        }

        if (m_str && m_own) {
            ::free(m_str);
        }

        m_str  = duplicate(s.m_str, s.m_len);
        m_len  = s.m_len;
        m_own  = s.m_own;
        m_hash = s.m_hash;
        return Ego;
    }

    String String::dup() const {
        if (m_str == nullptr || m_len == 0)
            return nullptr;
        return std::move(String(duplicate(m_str, m_len), m_len, true));
    }

    String String::peek() const {
        // this will return a dup of the string but as
        // just a reference or simple not owner
        return std::move(String(m_cstr, m_len, false));
    }

    void String::toupper() {
        for (int i = 0; i < m_len; i++) {
            m_str[i] = (char) ::toupper(m_str[i]);
        }
    }

    void String::tolower() {
        for (int i = 0; i < m_len; i++) {
            m_str[i] = (char) ::tolower(m_str[i]);
        }
    }

    bool String::empty() const {
        return m_str == nullptr || m_len == 0;
    }

    size_t String::find(char ch) const {
        if (Ego.m_len > 0) {
            int i{0};
            while (Ego.m_str[i++] != ch)
                if (i == Ego.m_len) return  -1;
            return i-1;
        }
        return npos;
    }

    size_t String::find(const String& str) const
    {
        if (empty()) {
            return npos;
        }
        if (str.empty()) {
            return 0;
        }

        const char *p = m_cstr;
        const size_t len = str.size();
        for (; (p = strchr(p, str[0])) != nullptr; p++)
        {
            if (strncmp (p, &str[0], len) == 0)
                return size_t(p-m_cstr);
        }

        return npos;
    }

    size_t String::rfind(char ch) const {
        ssize_t index{-1};
        if (Ego.m_len > 0) {
            auto i = (int) Ego.m_len;
            while (Ego.m_str[--i] != ch)
                if (i == 0) return -1;
            return i;
        }
        return npos;
    }

    size_t String::rfind(const String& str) const
    {
        if (empty()) {
            return npos;
        }
        if (str.empty()) {
            return 0;
        }

        auto pos = rfind(str[0]);
        const size_t len = str.size();
        for (; pos != npos; pos = substr(0, pos).rfind(str[0]))
        {
            auto tmp = substr(pos);
            if (tmp.startsWith(str))
                return pos;
        }

        return npos;
    }

    bool String::operator==(const String &s) const {
        if (Ego.empty() or s.empty()) {
            // they are only equal if they are both empty
            return Ego.empty() and s.empty();
        }

        if (m_len == s.m_len) {
            // they are only equal if they have the same size
            return strncmp(m_cstr, s.m_cstr, m_len) == 0;
        }

        return false;
    }

    const char *String::c_str(const char *nil) const {
        if (m_cstr == nullptr || m_len == 0)
            return nil;
        return m_cstr;
    }

    String String::substr(size_t from, size_t nchars, bool zc) const {
        auto fits = (ssize_t) (Ego.m_len - from);
        nchars = nchars? nchars : fits;
        if (nchars && fits >= nchars) {
            auto tmp = String{&Ego.m_cstr[from], nchars, false};
            return zc? std::move(tmp) : tmp.dup();
        }
        return String{};
    }

    String::~String() {
        if (m_str && m_own) {
            ::free(m_str);
        }
        m_str = nullptr;
        m_own = false;
    }

    String String::strip(char strip, bool ends) const {
        const char		*s, *e;
        char *p;

        Buffer b(Ego.size());
        auto tmp = (void *) b;
        p = (char *)tmp;
        s = Ego.data();
        e = Ego.data() + (Ego.size()-1);
        while (strip == *s) s++;
        while (strip == *e) e--;
        e++;

        if (ends) {
            memcpy(p, s, (e-s));
            b.seek(e-s);
        }
        else {
            for (; s < e; s++) {
                if (*s == strip)
                    continue;
                *p++ = *s;
            }
            b.seek(p - (char *)tmp);
        }

        // String will take ownership of the buffer
        return std::move(String(b));
    }

    std::vector<String> String::split(const char *delim)
    {
        const char *ap = nullptr, *ptr = Ego.data(), *eptr = ptr + Ego.size();
        std::vector<String> out;
        auto cmp = [](const char* p, const char* d) -> const char* {
            if (d[0] == '\0') {
                auto s = p;
                while((s != nullptr) and (*s != '\0')) s++;
                return s;
            }
            else {
                return strstr(p, d);
            }
        };

        auto len = (delim == nullptr)? 0: (delim[0] == '\0'? 1: strlen(delim));
        if (len > 0) {
            for (ap = cmp(ptr, delim); ap != nullptr; ap = cmp(ap, delim)) {
                if (ap != eptr) {
                    out.push_back(String{ptr, size_t(ap - ptr), false}.dup());
                    ap += len;
                    ptr = ap;
                }
            }
        }

        if (ptr != eptr) {
            out.push_back(String{ptr, size_t(eptr-ptr), false}.dup());
        }

        return std::move(out);
    }

    ConstVec<String> String::tokenize(const char *delim)
    {
        if (!m_own) {
            throw UnsupportedOperation("String does not own buffer, cannot be tokenized (use split/parts)");
        }

        char *ap = nullptr, *ptr = Ego.data(), *eptr = ptr + Ego.size();
        std::vector<String> out;
        auto cmp = [](char* p, const char* d) -> char* {
            if (d[0] == '\0') {
                auto s = p;
                while((s != nullptr) and (*s != '\0')) s++;
                return s;
            }
            else {
                return strstr(p, d);
            }
        };

        auto len = (delim == nullptr)? 0: (delim[0] == '\0'? 1: strlen(delim));
        if (len > 0) {
            for (ap = cmp(ptr, delim); ap != nullptr; ap = cmp(ap, delim)) {
                if (ap != eptr) {
                    out.emplace_back(ptr, size_t(ap - ptr), false);
                    *ap = '\0';
                    ap += len;
                    ptr = ap;
                }
            }
        }

        if (ptr != eptr) {
            out.emplace_back(ptr, size_t(eptr-ptr), false);
        }

        return std::move(out);
    }

    ConstVec<String> String::parts(const char *delim) const
    {
        const char *ap = nullptr, *ptr = Ego.data(), *eptr = ptr + Ego.size();
        std::vector<String> out;
        auto cmp = [](const char* p, const char* d) -> const char* {
            if (d[0] == '\0') {
                auto s = p;
                while((s != nullptr) and (*s != '\0')) s++;
                return s;
            }
            else {
                return strstr(p, d);
            }
        };

        auto len = (delim == nullptr)? 0: (delim[0] == '\0'? 1: strlen(delim));
        if (len > 0) {
            for (ap = cmp(ptr, delim); (ap != nullptr and ap < eptr); ap = cmp(ap, delim)) {
                if (ap < eptr) {
                    out.emplace_back(ptr, size_t(ap - ptr), false);
                    ap += len;
                    ptr = ap;
                }
            }
        }

        if (ptr < eptr) {
            out.emplace_back(ptr, size_t(eptr-ptr), false);
        }

        return out;
    }

    bool String::endswith(const char *part, bool igc) const {
        if (part == nullptr) return false;
        String tmp{part};
        auto from = size() - tmp.size();
        if (from < 0) return false;
        return substr(from) == tmp;
    }

    bool String::ishex(int checkCase) const
    {
        std::string_view  view = Ego;
        if (checkCase < 0) {
            return view.find_first_not_of("0123456789abcdef") == std::string::npos;
        }
        else if (checkCase > 0){
            return view.find_first_not_of("0123456789ABCDEF") == std::string::npos;
        }
        else {
            return view.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
        }
    }

    int64_t strtonum(const String &str, int base, long long int min, long long int max) {
        long long l;
        char *ep;
        if (str.size() > 19) {
            throw OutOfRange("utils::strtonum - to many characters in string number");
        }
        if (min > max) {
            throw OutOfRange("utils::strtonum - min value specified is greater than max");
        }

        errno = 0;
        char cstr[32] = {0};
        memcpy(cstr, &str[0], str.size());
        cstr[str.size()] = '\0';

        l = strtoll(cstr, &ep, base);
        if (errno != 0 || str.data() == ep || !matchany(*ep, '.', '\0')) {
            //@TODO
            // printf("strtoll error: (str = %p, ep = %p), *ep = %02X, errno = %d",
            //       str(), ep, *ep, errno);
            throw InvalidArguments("utils::strtonum - failed, errno = ", errno);
        }

        if (l < min)
            throw OutOfRange("utils::strtonum -converted value is less than min value");

        if (l > max)
            throw OutOfRange("utils::strtonum -converted value is greator than max value");

        return (l);
    }

    String randbytes(size_t size)
    {
        uint8_t buf[size];
        RAND_bytes(buf, (int) size);
        return hexstr(buf, size);

    }

    String hexstr(const uint8_t *buf, size_t len)
    {
        if (buf == nullptr)
            return String{};
        size_t rc = 2+(len<<1);
        auto *OUT = static_cast<char *>(malloc(rc));

        String tmp{nullptr};
        rc = hexstr(buf, len, OUT, rc);
        if (rc) {
            OUT[rc] = '\0';
            tmp = String(OUT, (size_t)rc, true);
        } else
            ::free(OUT);

        return std::move(tmp);
    }

    void bytes(const String &str, uint8_t *out, size_t olen) {
        size_t size = str.size()>>1;
        if (out == nullptr || olen < size)
            throw InvalidArguments("utils::bytes - output buffer invalid");

        char v;
        const char *p = str.data();
        for (int i = 0; i < size; i++) {
            out[i] = (uint8_t) (c2i(*p++) << 4 | c2i(*p++));
        }
    }

    const char& String::operator[](size_t index) const
    {
        if (index < m_len)
            return m_cstr[index];
        throw IndexOutOfBounds("index '", index, "' out of bounds");
    }

    const char& String::front() const
    {
        if (empty()) {
            static char Invalid{'\0'};
            return Invalid;
        }
        return m_cstr[0];
    }

    const char& String::back() const
    {
        if (empty()) {
            static char Invalid{'\0'};
            return Invalid;
        }
        return m_cstr[m_len-1];
    }

}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>

namespace sb = suil;

TEST_CASE("String test cases", "[common][String]") {
    // tests the String API's
    SECTION("Constructing a string") {
        // test sting constructor
        const char *cstr{"Hello World"};
        std::string str(cstr);
        sb::std::string_view     sv(str);

        sb::String s1{};
        REQUIRE(s1.m_str == nullptr);
        REQUIRE(s1.m_len == 0);
        REQUIRE_FALSE(s1.m_own);

        sb::String s2{cstr};
        REQUIRE(s2.m_str == cstr);
        REQUIRE(s2.m_len == 11);
        REQUIRE_FALSE(s2.m_own);

        sb::String s3{str};
        REQUIRE(s3.m_str == str.data());
        REQUIRE(s3.m_len == 11);
        REQUIRE_FALSE(s3.m_own);

        sb::String s4{sv};
        REQUIRE(s4.m_str == sv.data());
        REQUIRE(s4.m_len == 11);
        REQUIRE_FALSE(s4.m_own);

        sb::String s5('h', 12);
        REQUIRE(s5.m_str != nullptr);
        REQUIRE(s5.m_len == 12);
        REQUIRE(strcmp(s5.m_cstr, "hhhhhhhhhhhh") == 0);
        REQUIRE(s5.m_own);

        sb::String s6(str, true);
        REQUIRE(s6.m_str != nullptr);
        REQUIRE(s6.m_str != str.data());
        REQUIRE(s6.m_len == 11);
        REQUIRE(s6.m_own);
        sb::Buffer ob{16};
        ob << str;
        sb::String s7(ob, false);
        REQUIRE(s7.m_str != nullptr);
        REQUIRE(s7.m_str == ob.data());
        REQUIRE(s7.m_len == 11);
        REQUIRE_FALSE(s7.m_own);

        sb::String s8(ob);
        REQUIRE(s8.m_str != nullptr);
        REQUIRE(s8.m_len == 11);
        REQUIRE(s8.m_own);
        // buffer should have been released
        REQUIRE(ob.empty());
    }

    SECTION("String move/copy construct/assign") {
        // tests the string move/copy constructors and assignment operators
        std::string str{"Hello World"};
        sb::String s1{str};
        sb::String s2{str, true};

        WHEN("Moving the string") {
            // test string move constructor and assignment operator
            sb::String tmp1{str};
            sb::String s3(std::move(tmp1));
            REQUIRE(tmp1.m_str == nullptr);
            REQUIRE(tmp1.m_len == 0);
            REQUIRE_FALSE(tmp1.m_own);
            REQUIRE(s3.m_str == str.data());
            REQUIRE(s3.m_len == 11);
            REQUIRE_FALSE(s3.m_own);

            sb::String tmp2{str, true};
            sb::String s4(std::move(tmp2));
            REQUIRE(tmp2.m_str == nullptr);
            REQUIRE(tmp2.m_len == 0);
            REQUIRE_FALSE(tmp2.m_own);
            REQUIRE(s4.m_str != nullptr);
            REQUIRE(s4.m_str != str.data());
            REQUIRE(s4.m_len == 11);
            REQUIRE(s4.m_own);

            sb::String tmp3{str};
            sb::String s5 = std::move(tmp3);
            REQUIRE(tmp3.m_str == nullptr);
            REQUIRE(tmp3.m_len == 0);
            REQUIRE_FALSE(tmp3.m_own);
            REQUIRE(s5.m_str == str.data());
            REQUIRE(s5.m_len == 11);
            REQUIRE_FALSE(s5.m_own);

            sb::String tmp4{str, true};
            sb::String s6 = std::move(tmp4);
            REQUIRE(tmp4.m_str == nullptr);
            REQUIRE(tmp4.m_len == 0);
            REQUIRE_FALSE(tmp4.m_own);
            REQUIRE(s6.m_str != nullptr);
            REQUIRE(s6.m_str != str.data());
            REQUIRE(s6.m_len == 11);
            REQUIRE(s6.m_own);
        }

        WHEN("Copying the string") {
            // test string move constructor and assignment operator
            sb::String tmp1{str};
            sb::String s3(tmp1);
            REQUIRE(tmp1.m_str != nullptr);
            REQUIRE(tmp1.m_len == 11);
            REQUIRE_FALSE(tmp1.m_own);
            REQUIRE_FALSE(s3.m_str == tmp1.data());
            REQUIRE(s3.m_len == 11);
            REQUIRE(s3.m_own);

            sb::String tmp2{str, true};
            sb::String s4(tmp2);
            REQUIRE(tmp2.m_str != nullptr);
            REQUIRE(tmp2.m_len == 11);
            REQUIRE(tmp2.m_own);
            REQUIRE(s4.m_str != nullptr);
            REQUIRE(s4.m_str != tmp2.data());
            REQUIRE(s4.m_len == 11);
            REQUIRE(s4.m_own);

            sb::String tmp3{str};
            sb::String s5 = tmp3;
            REQUIRE(tmp3.m_str != nullptr);
            REQUIRE(tmp3.m_len == 11);
            REQUIRE_FALSE(tmp3.m_own);
            REQUIRE_FALSE(s5.m_str == tmp3.data());
            REQUIRE(s5.m_len == 11);
            REQUIRE(s5.m_own);

            sb::String tmp4{str, true};
            sb::String s6 = tmp4;
            REQUIRE(tmp4.m_str != nullptr);
            REQUIRE(tmp4.m_len == 11);
            REQUIRE(tmp4.m_own);
            REQUIRE(s6.m_str != nullptr);
            REQUIRE(s6.m_str != tmp4.data());
            REQUIRE(s6.m_len == 11);
            REQUIRE(s6.m_own);
        }
    }

    SECTION("String duplicate & peek", "[dup][peek]") {
        // test the two methods String::[dup/peek]
        WHEN("duplicating string") {
            sb::String s1{"Hello World"};
            REQUIRE_FALSE(s1.m_own);
            REQUIRE_FALSE(s1.m_str == nullptr);
            REQUIRE(s1.m_len == 11);

            sb::String s2{s1.dup()};
            REQUIRE_FALSE(s2.m_str == nullptr);
            REQUIRE_FALSE(s2.m_str == s1.m_str);
            REQUIRE(s2.m_own);
            REQUIRE(s2.m_len == 11);
            REQUIRE_FALSE(s1.m_str == nullptr);
            REQUIRE(strcmp(s2.m_str, s1.m_str) == 0);

            sb::String s3{s2.dup()};
            REQUIRE_FALSE(s3.m_str == nullptr);
            REQUIRE_FALSE(s3.m_str == s2.m_str);
            REQUIRE(s3.m_own);
            REQUIRE(s3.m_len == 11);
            REQUIRE_FALSE(s2.m_str == nullptr);
            REQUIRE(strcmp(s3.m_str, s2.m_str) == 0);

            sb::String s4{};
            sb::String s5{s4.dup()};
            REQUIRE(s5.m_str == nullptr);
            REQUIRE(s5.m_len == 0);
            REQUIRE_FALSE(s5.m_own);
        }

        WHEN("peeking string") {
            sb::String s1{"Hello World"};
            REQUIRE_FALSE(s1.m_own);
            REQUIRE_FALSE(s1.m_str == nullptr);
            REQUIRE(s1.m_len == 11);

            sb::String s2{s1.peek()};
            REQUIRE_FALSE(s2.m_str == nullptr);
            REQUIRE(s2.m_str == s1.m_str);
            REQUIRE_FALSE(s2.m_own);
            REQUIRE(s2.m_len == 11);
            REQUIRE_FALSE(s1.m_str == nullptr);
            REQUIRE(strcmp(s2.m_str, s1.m_str) == 0);

            sb::String s3{s2.peek()};
            REQUIRE_FALSE(s3.m_str == nullptr);
            REQUIRE(s3.m_str == s2.m_str);
            REQUIRE_FALSE(s3.m_own);
            REQUIRE(s3.m_len == 11);
            REQUIRE_FALSE(s2.m_str == nullptr);

            sb::String s4{};
            sb::String s5{s4.peek()};
            REQUIRE(s5.m_str == nullptr);
            REQUIRE(s5.m_len == 0);
            REQUIRE_FALSE(s5.m_own);
        }
    }

    SECTION("String to uppercase/lowercase", "[toupper][tolower]") {
        // test String::[toupper/tolower]
        std::string str{"Hello WORLD"};
        sb::String s1{str, true};
        sb::String s2{str, true};
        s1.toupper();
        s2.tolower();
        REQUIRE(strcmp(s1.m_str, "HELLO WORLD") == 0);
        REQUIRE(strcmp(s2.m_str, "hello world") == 0);
    }

    SECTION("String find", "[find][rfind]") {
        // test String::[find/rfind]
        sb::String s1{"qThis is a string, gnirts a si sihTx"}; // 36
        REQUIRE(s1.find('i')  == 3);
        REQUIRE(s1.rfind('i') == 32);
        REQUIRE(s1.find('x')  == 35);
        REQUIRE(s1.rfind('q') == 0);
        REQUIRE(s1.find('z')  == -1);
        REQUIRE(s1.rfind('z') == -1);
    }

    SECTION("String substring/chunk", "[substr][chunk]") {
        // test String::[substr/chunk]
        WHEN("taking a substring") {
            sb::String s1{"Hello long string Xx Hello long string"}; // 38

            // test invalid cases first
            auto s2 = s1.substr(38);
            REQUIRE(s2.m_str == nullptr);
            REQUIRE(s2.m_len == 0);
            auto s3 = s1.substr(7, 38);
            REQUIRE(s3.m_str == nullptr);
            REQUIRE(s3.m_len == 0);

            // test some valid cases
            auto s4 = s1.substr(0, 5);              // takes only 5 characters from string
            REQUIRE_FALSE(s4.m_str == nullptr);
            REQUIRE(s4.m_len == 5);
            REQUIRE_FALSE(s4.m_own);
            REQUIRE(s4.m_str == s1.m_str);
            auto s5 = s1.substr(7);                // takes substring from 7 to end of string
            REQUIRE_FALSE(s5.m_str == nullptr);
            REQUIRE(s5.m_len == 31);
            REQUIRE_FALSE(s5.m_own);
            REQUIRE(s5.m_str == &s1.m_str[7]);
            auto s6 = s1.substr(6, 11, false);     // takes substring from 7, taking 11 characters and copying the substring
            REQUIRE_FALSE(s6.m_str == nullptr);
            REQUIRE(s6.m_len == 11);
            REQUIRE(s6.m_own);
            REQUIRE_FALSE(s6.m_str == &s1.m_str[7]);
            REQUIRE(strcmp("long string", s6.m_str) == 0);
        }

        WHEN("taking a chunk of the string") {
            sb::String s1{"One Xx Two xX Three"}; //19

            // invalid case's first
            auto s2 = s1.chunk('Z');
            REQUIRE(s2.m_str == nullptr);
            REQUIRE(s2.m_len == 0);
            auto s3 = s1.chunk('z', false);
            REQUIRE(s3.m_str == nullptr);
            REQUIRE(s3.m_len == 0);

            // valid case, chunk never copies
            auto  s4 = s1.chunk('x');
            REQUIRE_FALSE(s4.m_str == nullptr);
            REQUIRE_FALSE(s4.m_own);
            REQUIRE(s4.m_len == 14);
            REQUIRE(&s1.m_str[5] == s4.m_str);
            REQUIRE(strcmp(&s1.m_str[5], s4.m_str) == 0);
            auto  s6 = s1.chunk('x', true);
            REQUIRE_FALSE(s6.m_str == nullptr);
            REQUIRE_FALSE(s4.m_own);
            REQUIRE(s6.m_len == 8);
            REQUIRE(&s1.m_str[11] == s6.m_str);
            REQUIRE(strcmp(&s1.m_str[11], s6.m_str) == 0);
        }
    }

    SECTION("String split", "[split]") {
        // tests String::split
        std::string str{"One,two,three,four,five||one,,two,three||,,,,"}; // 44

        sb::String s1{str, true};
        {
            auto p1 = s1.split("/");
            REQUIRE(p1.size() == 1);
            REQUIRE(p1[0].m_str != s1.m_str);
            REQUIRE(p1[0] == s1);
        }

        auto p2 = s1.split("||");
        REQUIRE(p2.size() == 3);
        REQUIRE(p2[0].size() == 23);
        REQUIRE(p2[0].m_str != s1.m_str);
        REQUIRE(p2[1].size() == 14);
        REQUIRE(p2[1].m_str != &s1.m_str[25]);
        REQUIRE(p2[2].size() == 4);
        REQUIRE(p2[2].m_str != &s1.m_str[41]);

        sb::String s2{p2[0]};
        auto p3 = s2.split(",");
        REQUIRE(p3.size() == 5);
        REQUIRE(p3[0] == "One");
        REQUIRE(p3[1] == "two");
        REQUIRE(p3[2] == "three");
        REQUIRE(p3[3] == "four");
        REQUIRE(p3[4] == "five");

        sb::String s3{p2[1]};
        auto p4 = s3.split(",");
        REQUIRE(p4.size() == 4);
        REQUIRE(p4[0] == "one");
        REQUIRE(p4[1] == nullptr);
        REQUIRE(p4[2] == "two");
        REQUIRE(p4[3] == "three");

        sb::String s4{p2[2]};
        auto p5 = s4.split(",");
        REQUIRE(!p5.empty());
    }

    SECTION("String tokenize", "[tokens]") {
        // tests String::tokenize
        std::string str{"One,two,three,four,five||one,,two,three||,,,,"}; // 44

        sb::String s1{str, true};
        {
            auto p1 = s1.tokenize("/");
            REQUIRE(p1.size() == 1);
            REQUIRE(p1[0].m_str == s1.m_str);
            REQUIRE(p1[0] == s1);
        }

        auto p2 = s1.tokenize("||");
        REQUIRE(p2.size() == 3);
        REQUIRE(p2[0].size() == 23);
        REQUIRE(p2[0].m_str == s1.m_str);
        REQUIRE(p2[1].size() == 14);
        REQUIRE(p2[1].m_str == &s1.m_str[25]);
        REQUIRE(p2[2].size() == 4);
        REQUIRE(p2[2].m_str == &s1.m_str[41]);

        auto s2 = p2[0].peek();
        REQUIRE_THROWS(s2.tokenize(","));
        s2 = s2.dup();
        auto p3 = s2.tokenize(",");
        REQUIRE(p3.size() == 5);
        REQUIRE(p3[0] == "One");
        REQUIRE(p3[1] == "two");
        REQUIRE(p3[2] == "three");
        REQUIRE(p3[3] == "four");
        REQUIRE(p3[4] == "five");

        sb::String s3 = p2[1].dup();
        auto p4 = s3.split(",");
        REQUIRE(p4.size() == 4);
        REQUIRE(p4[0] == "one");
        REQUIRE(p4[1] == nullptr);
        REQUIRE(p4[2] == "two");
        REQUIRE(p4[3] == "three");

        sb::String s4 = p2[2].dup();
        auto p5 = s4.split(",");
        REQUIRE(!p5.empty());
    }

    SECTION("String parts", "[parts]") {
        // tests String::split
        std::string str{"One,two,three,four,five||one,,two,three||,,,,"}; // 44

        const sb::String s1{str, true};
        auto p1 = s1.parts("/");
        REQUIRE(p1.size() == 1);
        REQUIRE(p1[0].m_str == s1.m_str);
        REQUIRE(p1[0].size() == 45);

        auto p2 = s1.parts("||");
        REQUIRE(p2.size() == 3);
        REQUIRE(p2[0].size() == 23);
        REQUIRE(p2[0].m_str == s1.m_str);
        REQUIRE(p2[1].size() == 14);
        REQUIRE(p2[1].m_str == &s1.m_str[25]);
        REQUIRE(p2[2].size() == 4);
        auto pp1 = p2[2].m_str;
        auto pp2 = &s1.m_str[41];
        REQUIRE(p2[2].m_str == &s1.m_str[41]);

        sb::String s2{p2[0]};
        auto p3 = s2.parts(",");
        REQUIRE(p3.size() == 5);
        REQUIRE(p3[0] == "One");
        REQUIRE(p3[1] == "two");
        REQUIRE(p3[2] == "three");
        REQUIRE(p3[3] == "four");
        REQUIRE(p3[4] == "five");

        sb::String s3{p2[1]};
        auto p4 = s3.parts(",");
        REQUIRE(p4.size() == 4);
        REQUIRE(p4[0] == "one");
        REQUIRE(p4[1].empty());
        REQUIRE(p4[2] == "two");
        REQUIRE(p4[3] == "three");

        sb::String s4{p2[2]};
        auto p5 = s4.parts(",");
        REQUIRE(!p5.empty());
    }

    SECTION("String strip/trim", "[strip][trim]") {
        // tests String::[strip/trim]
        sb::String s1{"   String String Hello   "}; // 25

        auto s2 = s1.strip('q');                // nothing is stripped out if it not in the string
        REQUIRE(s2.m_own);
        REQUIRE(s2.m_len == 25);
        REQUIRE(strcmp(s2.m_str, s1.m_str) == 0);
        REQUIRE(s2.m_str != s1.m_str);

        s2 = s1.strip();                   // by default spaces are stripped out
        REQUIRE(s2.m_own);
        REQUIRE(s2.m_len == 17);
        REQUIRE(strcmp(s2.m_str, "StringStringHello") == 0);
        REQUIRE(s2.m_str != s1.m_str);

        s2 = s1.strip('S');                   // any character can be stripped out
        REQUIRE(s2.m_own);
        REQUIRE(s2.m_len == 23);
        REQUIRE(strcmp(s2.m_str, "   tring tring Hello   ") == 0);
        REQUIRE(s2.m_str != s1.m_str);

        s2 = s1.trim();                        // trim just calls strip with option to strip only the ends
        REQUIRE(s2.m_own);
        REQUIRE(s2.m_len == 19);
        REQUIRE(strcmp(s2.m_str, "String String Hello") == 0);
        REQUIRE(s2.m_str != s1.m_str);

        s2 = s1.trim('o');                    // nothing to trim since 'o' is not at the end
        REQUIRE(s2.m_own);
        REQUIRE(s2.m_len == 25);
        REQUIRE(strcmp(s2.m_str, s1.m_str) == 0);
        REQUIRE(s2.m_str != s1.m_str);
    }

    SECTION("String compare and comparison operators", "[compare]") {
        // test sb::String:L:compare and other comparison operators
        sb::String s1{"abcd"}, s2{"abcd"}, s3{"abcde"}, s4{"ABCD"}, s5{"abcD"};
        // case sensitive compare
        REQUIRE(s1.compare(s2) == 0);
        REQUIRE(s1.compare(s3) < 0);
        REQUIRE(s1.compare(s4) > 0);
        REQUIRE(s5.compare(s1) < 0);
        // case insensitive compare
        REQUIRE(s1.compare(s4, true) == 0);
        REQUIRE(s5.compare(s2, true) == 0);
        REQUIRE(s4.compare(s5, true) == 0);
        REQUIRE(s5.compare(s3, true) < 0);

        REQUIRE(s1 == s2);
        REQUIRE(s1 != s3);
        REQUIRE(s1 <  s3);
        REQUIRE(s3 >  s4);
        REQUIRE(s5 >= s4);
        REQUIRE(s5 <= s2);
    }

    SECTION("Other string API's", "[casting][c_str]") {
        // tests other string API's not tested on other sections
        sb::String ss{}, ss1{"Hello World"};
        REQUIRE(ss.empty());
        REQUIRE(ss.size() == 0);
        REQUIRE_FALSE(ss1.empty());
        REQUIRE(ss1.size() == 11);
        REQUIRE(ss1.data() == ss1.m_cstr);

        WHEN("Getting c-style string with operator() or String::c_str") {
            // test getting the c-style string
            sb::String s1{}, s2{"hello world"};
            // Useful when on empty strings
            REQUIRE(strlen(s1.c_str()) == 0);
            REQUIRE(strlen(s1()) == 0);
            REQUIRE(strlen(s1.c_str("nil")) == 3);
            REQUIRE(strcmp(s1.c_str("nil"), "nil") == 0);
            // on non-empty strings, the underlying buffer is returned
            auto b1 = s2.c_str(), b2 = s2.c_str("nil"), b3 = s2();
            REQUIRE(b1 == s2.m_str);
            REQUIRE(strlen(b1) == s2.m_len);
            REQUIRE(b2 == s2.m_str);
            REQUIRE(strlen(b2) == s2.m_len);
            REQUIRE(b3 == s2.m_str);
            REQUIRE(strlen(b3) == s2.m_len);
        }

        WHEN("Adding strings using operators") {
            // test += and +
            sb::String s1{"Hello"}, s2{"World"};
            sb::String s3 = s1 + " ";
            REQUIRE(s3.m_own);
            REQUIRE(s3.m_len == 6);
            REQUIRE(s3.compare("Hello ") == 0);
            // append s2 to s3
            s3 += s2;
            REQUIRE(s3.m_own);
            REQUIRE(s3.m_len == 11);
            REQUIRE(s3.compare("Hello World") == 0);
        }

        WHEN("Casting a string to other types using implicit cast operator") {
            // test implicit cast
            sb::String s1{"true"}, s2{"120"}, s3{"Hello"}, s4{"0.00162"};

            // this cast work against us as they are implemented to check for empty buffer, use utils::cast
            bool to{false};
            REQUIRE((bool) s1);
            sb::cast(s1, to);
            REQUIRE(to);
            REQUIRE((bool) s2);
            sb::cast(s2, to);
            REQUIRE_FALSE(to);

            REQUIRE((int)s2 == 120);
            REQUIRE((float)s4 == 0.00162f);
            REQUIRE((double)s2 == 120);
            REQUIRE_THROWS((int)s1);
            REQUIRE_THROWS((float)s3);

            const char *out = (const char *)s3;
            REQUIRE(out == s3.m_cstr);
            std::string str = (std::string) s3;
            REQUIRE(s3.compare(str.data()) == 0);
            auto ss = (sb::String) s3;
            REQUIRE(ss == s3);
            REQUIRE_FALSE(ss.m_cstr == s3.m_cstr);
        }
    }
}

#endif
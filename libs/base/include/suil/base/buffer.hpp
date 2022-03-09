//
// Created by Mpho Mbotho on 2020-10-07.
//

#ifndef SUIL_BASE_BUFFER_HPP
#define SUIL_BASE_BUFFER_HPP

#include "suil/base/data.hpp"
#include "suil/base/string.hpp"

namespace iod {
    template <typename...T>
    struct sio;
}

namespace suil {

    /**
     * A lightweight ouput buffer
     */
    struct Buffer {

        /**
         * @class Fmt
         * Short for formatter is the base class for formatters and it can be
         * used to format data going into the buffer when using stream operators
         */
        struct Fmt {
            /**
             * Create a new formatter specifying the maximum amount of memory
             * that will be written to buffer
             * @param reqsz the memory to be reserved on the buffer
             */
            Fmt(size_t reqsz)
                : reqsz{reqsz}
            {}

            /**
             * this function is the callback that the buffer will call. When this function
             * is invoked, it is guarranted that \param raw is valid and has enough space, as requested
             * @param raw the buffer that the formatter will write into
             * @return must written the size of data written into the buffer \param raw
             */
            virtual size_t fmt(char *raw) const { return 0; };

        private suil_ut:
            friend struct Buffer;
            size_t reqsz{0};
        };

        /**
         * Create a new buffer with an initial size, if not specified,
         * will start off with size of 0
         * @param is the initial buffer size
         */
        Buffer(size_t is);

        /**
         * Creates a buffer with 0 initial size
         */
        Buffer()
            : Buffer(0)
        {}

        /**
         * move constructor for the buffer
         */
        Buffer(Buffer&&) noexcept;

        /**
         * move assignment operator
         * @param other the buffer being moved
         * @return Buffer object
         */
        Buffer&operator=(Buffer&& other) noexcept;

        /**
         * destroys the buffer contents
         */
        ~Buffer();

        /**
         * append data into the buffer
         * @param data the data to append into the buffer
         * @param size the size of the data being appended
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        ssize_t append(const void *data, size_t size);

        /**
         * binary append a single unsigned character into the buffer
         * @param c the character to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(unsigned char c) {
            return append(&c, sizeof(unsigned char));
        }

        /**
         * binary append a single unsigned short into the buffer
         * @param c the value to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(unsigned short c) {
            return append(&c, sizeof(unsigned short));
        }

        /**
         * binary append a single unsigned integer into the buffer
         * @param c the value to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(unsigned int c) {
            return append(&c, sizeof(unsigned int));
        }

        /**
         * binary append a single signed 64-bit number into the buffer
         * @param c the value to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(uint64_t c) {
            return append(&c, sizeof(uint64_t));
        }

        /**
         * binary append a single signed character into the buffer
         * @param c the character to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(char c) {
            return append(&c, sizeof(char));
        }

        /**
         * binary append a single short into the buffer
         * @param c the value to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(short c) {
            return append(&c, sizeof(short));
        }

        /**
         * binary append a single signed int into the buffer
         * @param c the value to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(int c) {
            return append(&c, sizeof(int));
        }

        /**
         * binary append a single signed 64-bit number into the buffer
         * @param c the value to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(int64_t c) {
            return append(&c, sizeof(int64_t));
        }

        /**
         * binary append a single float into the buffer
         * @param c the value to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(float c) {
            return append(&c, sizeof(float));
        }

        /**
         * binary append a single double into the buffer
         * @param c the value to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(double c) {
            return append(&c, sizeof(double));
        }

        /**
         * append the given timestamp formatted in a time-format string
         *
         * @param t the timestamp to append
         * @param fmt the time-format to use when appending the timestamp
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        ssize_t append(time_t t, const char* fmt);

        /**
         * appends a c-style string into the buffer
         * @param str the c-style string to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(const char* str) {
            return append(str, (uint32_t) strlen(str));
        }

        /**
         * appends a string view into buffer
         * @param str the string view to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(std::string_view& sv) {
            return append(sv.data(), sv.size());
        }

        /**
         * appends a  standard string into buffer
         * @param str the standard string to append
         * @return the number of bytes written into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(const std::string& str) {
            return append(str.data(), str.size());
        }

        /**
         * copies the contents of the the \param other buffer inyp calling
         * buffer
         * @param other the buffer to copy
         * @return the number of bytes copied into the buffer on success and
         * -1 on failure
         */
        inline ssize_t append(const Buffer& other) {
            return append(other.m_data, other.size());
        }

        /**
         * append the given number into the buffer, formatting the number as hex
         * @tparam T the type of the number to append, auto-deduced
         * @param v the number to append
         * @param filled true if the hex output must be prefilled with 0's
         * @return the number of bytes copied into the buffer on success and
         * -1 on failure
         */
        template<typename T, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
        ssize_t hex(T  v, bool filled = false) {
            size_t tmp = this->size();
            char fmt[10];
            if (filled)
                sprintf(fmt, "%%0%dx",(int)sizeof(T)*2);
            else
                sprintf(fmt, "%%x");
            return appendnf(8, fmt, v);
        }

        /**
         * append the given data into the buffer, formatting the buffer in
         * hex format
         * @param data the data to append
         * @param size the size of data to be append
         * * @param caps true if data should be presented in caps, false otherwise
         * @return the number of bytes copied into the buffer on success and
         * -1 on failure
         */
        ssize_t hex(const void* data, size_t size, bool caps = false);

        /**
         * append the given c-style string into the buffer, formatting the buffer in
         * hex format
         * @param str the string to append
         * @param caps true if data should be presented in caps, false otherwise
         * @return the number of bytes copied into the buffer on success and
         * -1 on failure
         */
        inline ssize_t hex(const char* str, bool caps = false) { return hex(str, strlen(str), caps); }

        /**
         * append the given standard string into the buffer, formatting the buffer in
         * hex format
         * @param str the string to append
         * @param caps true if data should be presented in caps, false otherwise
         * @return the number of bytes copied into the buffer on success and
         * -1 on failure
         */
        inline ssize_t hex(const std::string& str, bool caps = false) { return hex(str.data(), str.size(), caps); }

        /**
         * append the given string view into the buffer, formatting the buffer in
         * hex format
         * @param data the string to append
         * @param caps true if data should be presented in caps, false otherwise
         * @return the number of bytes copied into the buffer on success and
         * -1 on failure
         */
        inline ssize_t hex(std::string_view str, bool caps = false) { return hex(str.data(), str.size(), caps); }

        /**
         * printf-style append to buffer
         * @param fmt printf-style format string
         * @param ... list of format arguments
         * @return the number of bytes copied into the buffer on success and
         * -1 on failure
         */
        ssize_t appendf(const char *fmt, ...);

        /**
         * printf-style append to buffer. this takes \param hint as a hint to
         * the size that formatted string might use such that the format can be done
         * in place
         *
         * @param hint the size that is required by the formatted string
         * @param fmt printf-style format string
         * @param ... list of format arguments
         * @return the number of bytes copied into the buffer on success and
         * -1 on failure
         */
        ssize_t appendnf(uint32_t hint, const char *fmt,...);

        /**
         * printv-style append to buffer
         * @param fmt printfv-style format string
         * @param args variable arguments
         * @return the number of bytes copied into the buffer on success and
         * -1 on failure
         */
        ssize_t appendv(const char *fmt, va_list args);

        /**
         * printv-style append to buffer. this takes \param hint as a hint to
         * the size that formatted string might use such that the format can be done
         * in place
         *
         * @param hint the size that is required by the formatted string
         * @param fmt printv-style format string
         * @param args variable arguments
         * @return the number of bytes copied into the buffer on success and
         * -1 on failure
         */
        ssize_t appendnv(uint32_t hint, const char *fmt, va_list args);

        /**
         * reset's the buffer
         * @param size the new size of the buffer. If this size is less than
         * the current size and \param keep is false, the memory will be shrinked
         * accordingly, otherwise it will be reused. If the size is larger than the
         * current size, the memory will be reallocated
         * @param keep true if current buffer should be reused
         */
        void reset(size_t size, bool keep = false);

        /**
         * move offset forward by \param off bytes
         * @param off the number of bytes to offset, if the operation exceeds the current
         * buffer size, the buffer will grow
         */
        void seek(off_t off);

        /**
         * move offset to to 0 and move forward by \param off bytes
         * @param off the number of bytes to offset, if the operation exceeds the current
         * buffer size, the buffer will grow
         */
        void bseek(off_t off);

        /**
         * ensures that the buffer capacity is greater than \param size bytes. If it is less
         * the buffer will grow accordingly
         * @param size the required capacity
         */
        void reserve(size_t size);

        /**
         * resets the buffer, discarding the contents and freeing resources
         */
        void clear();

        /**
         * resets the buffers without freeing the memory and returns the memory
         * to caller. Caller will have ownership of this memory after this call
         * @return a heap allocated pointer representing the buffer. if it is not
         * null, caller must free with free()
         */
        char* release();

        /**
         * cast the buffer to a c-style string.
         * @return pointer to buffer as a c-style string
         */
        explicit operator char*();

        /**
         * cast the buffer to a c-style string
         * @return pointer to buffer as a c-style string
         */
        explicit operator const char*() const {
            return (const char*) m_data;
        }

        /**
         * cast buffer to standard string
         * @warning copies data
         * @return a string whose data is a copy of the contents
         * of the buffer
         */
        explicit operator std::string() const {
            return std::string((const char*) m_data, m_offset);
        }

        /**
         * casts buffer to string view
         * @return a string view whose data is the contents
         * of the buffer
         */
        explicit operator std::string_view() const {
            return std::string_view((const char*) m_data, size());
        }

        /**
         * casts buffer to a boolean
         * @return true if the buffer has data, false otherwise
         */
        operator bool() {
            return (m_offset != 0) &&
                   (m_data != nullptr) &&
                   (m_size != 0);
        }

        /**
         * gets the current size of the buffer
         * @return the size of the buffer
         */
        inline size_t size() const {
            return m_offset;
        }

        /**
         * tests to see if the buffer has data or not
         * @return true if data has been written into the buffer, false otherwise
         */
        inline bool empty() const {
            return m_offset == 0;
        }

        /**
         * gets a pointer to data without modifying the data
         *
         * @return a pointer to the data if any, otherwise null is returned
         */
        char* data() const {
            if (m_data)
                return (char *) m_data;
            return nullptr;
        }

        /**
         * gets a pointer to data without modifying the data
         *
         * @return a pointer to the data if any, otherwise null is returned
         */
        Data cdata() const {
            if (m_data)
                return Data{(uint8_t *)m_data, size(), false};
            return Data{};
        }

        /**
         * gets the current buffer capacity
         * @return the available size on the buffer
         */
        size_t capacity() const {
            return m_size-m_offset;
        }

        /**
         * get pointer to memory used by buffer
         * @return pointer to memory used by the buffer
         */
        operator void*() {
            return m_data;
        }

        /**
         * get pointer to memory used by buffer
         * @return pointer to memory used by the buffer
         */
        operator const void*() const {
            return m_data;
        }

        /**
         * add c-style string operator, appends the given string
         * into the buffer
         * @param str the string to append
         * @return
         */
        inline Buffer& operator+=(const char *str) {
            append(str);
            return *this;
        }

        /**
         * add std string operator, appends the given string
         * into the buffer
         * @param str the string to append
         * @return
         */
        inline Buffer& operator+=(const std::string& str) {
            append(str.c_str());
            return *this;
        }

        /**
         * add string view operator, appends the given string
         * into the buffer
         * @param str the string to append
         * @return
         */
        inline Buffer& operator+=(std::string_view& sv) {
            append(sv.data(), sv.size());
            return *this;
        }

        /**
         * add boolean operator - appends the given boolean into the buffer
         * @param u the boolean value to append
         * @return
         */
        inline Buffer&  operator<<(const bool u) {
            if (u) appendf("true"); else append("false");
            return *this;
        }

        /**
         * add unsigned char  - printf-style appends the given value into the buffer
         * @param u the value to append
         * @return
         */
        inline Buffer&  operator<<(unsigned char u) {
            appendf("%hhu", u);
            return *this;
        }

        /**
         * add unsigned short  - printf-style appends the given value into the buffer
         * @param u the value to append
         * @return
         */
        inline Buffer&  operator<<(unsigned short u) {
            appendf("%hu", u);
            return *this;
        }

        /**
         * add unsigned int  - printf-style appends the given value into the buffer
         * @param u the value to append
         * @return
         */
        inline Buffer&  operator<<(unsigned int u) {
            appendf("%u", u);
            return *this;
        }

        /**
         * add unsigned long  - printf-style appends the given value into the buffer
         * @param ul the value to append
         * @return
         */
        inline Buffer&  operator<<(unsigned long ul) {
            appendf("%lu", ul);
            return *this;
        }

        /**
         * add unsigned long long  - printf-style appends the given value into the buffer
         * @param ull the value to append
         * @return
         */
        inline Buffer&  operator<<(unsigned long long ull) {
            appendf("%llu", ull);
            return *this;
        }

        /**
         * add char  - printf-style appends the given value into the buffer
         * @param i the value to append
         * @return
         */
        inline Buffer&  operator<<(char i) {
            appendf("%c", i);
            return *this;
        }

        /**
         * add short  - printf-style appends the given value into the buffer
         * @param i the value to append
         * @return
         */
        inline Buffer&  operator<<(short i) {
            appendf("%hd", i);
            return *this;
        }

        /**
         * add int  - printf-style appends the given value into the buffer
         * @param i the value to append
         * @return
         */
        inline Buffer&  operator<<(int i) {
            appendf("%d", i);
            return *this;
        }

        /**
         * add long  - printf-style appends the given value into the buffer
         * @param l the value to append
         * @return
         */
        inline Buffer&  operator<<(long l) {
            appendf("%ld", l);
            return *this;
        }

        /**
         * add long long  - printf-style appends the given value into the buffer
         * @param ll the value to append
         * @return
         */
        inline Buffer&  operator<<(long long ll) {
            appendf("%lld", ll);
            return *this;
        }

        /**
         * add double  - printf-style appends the given value into the buffer
         * @param d the value to append
         * @return
         */
        inline Buffer&  operator<<(double d) {
            appendf("%f", d);
            return *this;
        }

        /**
         * add float  - printf-style appends the given value into the buffer
         * @param d the value to append
         * @return
         */
        inline Buffer&  operator<<(float d) {
            appendf("%f", d);
            return *this;
        }

        /**
         * add c-style string  - printf-style appends the given string into the buffer
         * @param str the c-style string to append
         * @return
         */
        inline Buffer&  operator<<(const char *str) {
            append(str);
            return *this;
        }

        /**
         * copy appends the given string into the buffer
         * @param str string to append
         * @return
         */
        inline Buffer&  operator<<(char *str) {
            append(str);
            return *this;
        }

        /**
         * appends a formatter into buffer
         * @tparam T the type of formatter, which must implement \class fmter
         * @param fmt the formatter instance
         * @return
         */
        template <typename T, typename std::enable_if_t<std::is_base_of_v<suil::Buffer::Fmt, T>>* = nullptr>
        inline Buffer&  operator<<(T fmt) {
            reserve(fmt.reqsz);
            size_t tmp = fmt.fmt((char *)&m_data[m_offset]);
            if (tmp > fmt.reqsz) tmp = fmt.reqsz;
            m_offset += tmp;
            return *this;
        }

        /**
         * copy appends given standard string into buffer
         * @param str the string to copy into buffer
         * @return
         */
        inline Buffer&  operator<<(const std::string &str) {
            append(str.c_str());
            return *this;
        }

        /**
         * copy appends given string view into buffer
         * @param sv the string to copy into buffer
         * @return
         */
        inline Buffer&  operator<<(std::string_view sv) {
            append(sv.data(), sv.size());
            return *this;
        }

        /**
         * copy appends the other buffer into current buffer
         * @param other the other buffer to copy
         * @return
         */
        inline Buffer& operator<<(const Buffer&  other) {
            append(other.m_data, other.m_offset);
            return *this;
        }

        /**
         * copy appends the given \class String into current buffer
         * @param other the string to copy
         * @return
         */
        Buffer& operator<<(const String&  other);

        /**
         * index into the buffer
         * @param index the index to access
         * @return a reference to byte at the given index
         * @throws IndexOutOfBounds
         */
        uint8_t& operator[](size_t index);

        /**
         *
         * @tparam T
         * @param o
         * @return
         */
        template <typename... T>
        Buffer&operator<<(const iod::sio<T...>& o);

    private suil_ut:
        void grow(uint32_t);
        uint8_t         *m_data{nullptr};
        uint32_t        m_size{0};
        uint32_t        m_offset{0};
    };

    static_assert(sizeof(Buffer) <= 16, "Output buffer size must be <= 16");

    namespace _internal {

        template<typename V>
        struct fmtnum : Buffer::Fmt {
            fmtnum(const char *fs, V t)
                : Buffer::Fmt(32),
                  val(t),
                  fs(fs)
            {}

            virtual size_t fmt(char *raw) const {
                return (size_t) snprintf(raw, 32, fs, val);
            }

            V val;
            const char *fs{"%x"};
        };
    }

    /**
     * number custom formatter using printf-style formatting
     * @tparam T the type of nummber to format
     * @param fs printf-style format string
     * @param t the number to format
     * @return N\A
     *
     * \example
     *  Buffer o;
     *  o << fmtnum("%0.4f", 2.99999f);
     */
    template <typename T, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
    static inline _internal::fmtnum<T> fmtnum(const char* fs, T t) {
        return _internal::fmtnum<T>(fs, t);
    };

    /**
     * formats a boolean when writting into buffer
     *
     * \example
     *  Buffer b;
     *  b << fmtbool(true);
     */
    struct fmtbool : Buffer::Fmt {
        fmtbool(bool b)
            : Buffer::Fmt(6),
              val(b)
        {}

        virtual size_t fmt(char *raw) {
            return (size_t) snprintf(raw, 6, "%s", (val? "True" : "False"));
        }
        bool val{false};
    };

    namespace _internal {
        template<typename... A>
        inline void catstr(Buffer &out, const A&... args) {
            (out << ... << args);
        }
    }

    /**
     * takes two or more parameters and concatenates them into a single stream
     * @tparam T1 type of first parameter
     * @tparam T2 type of second parameter
     * @tparam A variadic types of other parameters
     * @param a first parameter to concatenate
     * @param b second parameter to concatenate
     * @param args all the other parameters
     * @return all the inputs concatenated into a single \class String
     *
     * @note internally this uses an \class OBuffer to build a string so
     * any type that can be streamed to the buffer can be passed as input
     */
    template<typename T1, typename T2, typename... A>
    String catstr(const T1& a, const T2& b, const A&... args) {
        Buffer out(32);
        _internal::catstr(out, a, b, args...);
        return String(out);
    }


    template <typename T>
    inline String& String::operator+=(const T t) {
        Ego = suil::catstr(Ego, t);
        return Ego;
    }

    template <typename T>
    inline String String::operator+(const T t) {
        return suil::catstr(Ego, t);
    }
}
#endif //SUIL_BASE_BUFFER_HPP

/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-01-10
 */

#pragma once


#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <chrono>
#include <functional>
#include <memory>
#include <string_view>
#include <utility>
#include <thread>

#include <suil/version.hpp>

/**
 * Some personal ego, wraps around de-referencing
 * `this`
 */
#define Ego (*this)

/**
 * Macro to to retrieve current error in string representation
 */
#define errno_s strerror(errno)

/**
 * Symbol variable with name v
 * @param v the name of the symbol
 */
#define var(v) s::_##v

/**
 * Access a symbol, \see sym
 */
#define sym(v) var(v)

/**
 * Access symbol type
 */
#define Sym(v) s::_##v##_t

/**
 * Set an option, assign value \param v to variable \param o
 * @param o the variable to assign value to
 * @param v the value to assign to the variable
 */
#define opt(o, v) var(o) = v

/**
 * Create a symbol property
 */
#define prop(name, tp) var(name)  = tp()

#if defined __GNUC__ || defined __llvm__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define suil_no_inline __attribute__((noinline))
#define suil_force_inline __attribute__((always_inline))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#define suil_no_inline
#define suil_force_inline
#endif

#ifdef SUIL_ASSERT
#undef
#endif

#ifndef suil_ut
#define suil_ut
#endif

#define SUIL_XPASTE(x, y) x##y

/**
 * utility macro to return the size of literal c-string
 * @param ch the literal c-string
 *
 * \example sizeof("Hello") == 5
 */
#define sizeofcstr(ch)   (sizeof(ch)-1)

/**
 * @def A macro to declare copy constructor of a class. This
 * macro should be used inside a class
 */
#define COPY_CTOR(Type) Type(const Type&)
/**
 * @def A macro to declare move constructor of a class. This
 * macro should be used inside a class
 */
#define MOVE_CTOR(Type) Type(Type&&)
/**
 * @def A macro to declare copy assignment operator of a class. This
 * macro should be used inside a class
 */
#define COPY_ASSIGN(Type) Type& operator=(const Type&)
/**
 * @def A macro to declare copy assignment operator of a class. This
 * macro should be used inside a class
 */
#define MOVE_ASSIGN(Type) Type& operator=(Type&&)
/**
 * @def A macro to disable copy construction of a class. This
 * macro should be used inside a class
 */
#define DISABLE_COPY(Type) COPY_CTOR(Type) = delete; COPY_ASSIGN(Type) = delete
/**
 * @def A macro to disable move assignment of a class. This
 * macro should be used inside a class
 */
#define DISABLE_MOVE(Type) MOVE_CTOR(Type) = delete; MOVE_ASSIGN(Type) = delete

#define SUIL_ASSERT(cond) do {                                                           \
        if (unlikely(!(cond))) {                                                       \
            fprintf(stderr, "%s:%d assertion failed: " #cond "\n", __FILE__, __LINE__); \
            fflush(stderr);                                                             \
            abort();                                                                    \
        }                                                                               \
    } while (false)

using int8   = int8_t ;
using int16  = int16_t;
using int32  = int32_t;
using int64  = int64_t;
using uint8  = uint8_t ;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

/**
 * macro to add utility function's to a class/struct
 * for C++ smart pointers
 */
#define sptr(type)                          \
public:                                     \
    using Ptr = std::shared_ptr< type >;    \
    using WPtr = std::weak_ptr< type >;     \
    using UPtr = std::unique_ptr< type >;   \
    template <typename... Args>             \
    inline static Ptr mkshared(Args... args) {          \
        return std::make_shared< type >(    \
            std::forward<Args>(args)...);   \
    }                                       \
    template <typename... Args>             \
    inline static UPtr mkunique(Args... args) { \
        return std::make_unique< type >(    \
           std::forward<Args>(args)...);    \
    }

namespace suil {

    using namespace std::chrono_literals;

    using steady_clock = std::chrono::steady_clock;
    using system_clock = std::chrono::system_clock;
    using nanoseconds  = std::chrono::nanoseconds;
    using microseconds = std::chrono::microseconds;
    using milliseconds = std::chrono::milliseconds;
    using seconds = std::chrono::seconds;
    using minutes = std::chrono::minutes;
    using std::chrono::duration_cast;

    constexpr milliseconds DELAY_INF = -1ms;

    inline auto after(milliseconds timeout = DELAY_INF) -> steady_clock::time_point {
        return timeout <= 0ms? steady_clock::time_point{} : steady_clock::now() + timeout;
    }

    constexpr int INVALID_FD{-1};

    template<typename T>
    struct remove_rvalue_reference {
        using type = T;
    };

    template<typename T>
    struct remove_rvalue_reference<T &&> {
        using type = T;
    };

    template<typename T>
    using remove_rvalue_reference_t = typename remove_rvalue_reference<T>::type;

    template <typename T>
    concept arithmetic = std::is_arithmetic_v<T>;

    /**
     * change the given file descriptor to either non-blocking or
     * blocking
     * @param fd the file descriptor whose properties must be changed
     * @param on true when non-blocking property is on, false otherwise
     */
    int nonblocking(int fd, bool on = true);

    inline int clz(uint64_t num) {
        if (num) {
            return  __builtin_clz(num);
        }
        return 0;
    }

    inline int ctz(uint64_t num) {
        if (num) {
            return  __builtin_ctz(num);
        }
        return sizeof(uint64_t);
    }

    inline uint64_t np2(uint64_t num) {
        return (1U<<(sizeof(num) - suil::ctz(num)));
    }

    /**
     * @brief Generate random bytes
     *
     * @param buf a buffer in which to write the random bytes to
     * @param size the number of bytes to allocate
     */
    void randbytes(void *buf, size_t size);

    /**
     * @brief Convert the given input buffer to a hex string
     *
     * @param buf the input buffer to convert to hex
     * @param iLen the length of the input buffer
     * @param out the output buffer to write the hex string into
     * @param len the size of the output buffer (MUST be twice the size
     *  of the input buffer)
     * @return the number of bytes writen into the output buffer
     */
    size_t hexstr(const void *buf, size_t iLen, char *out, size_t len);

    /**
     * Reverse the bytes in the given buffer byte by byte
     * @param data the buffer to reverse
     * @param len the size of the buffer
     */
    void reverse(void *data, size_t len);

    /**
     * A utility function to close a pipe
     *
     * @param p a pipe descriptor
     */
    void closepipe(int p[2]);


    /**
     * @brief Hex representation to binary representation for a character
     *
     * @param c hex representation
     * @return binary representation
     *
     * @throws OutOfRangeError
     */
    uint8_t c2i(char c);

    /**
     * binary representation to hex representation
     * @param c binary representation
     * @param caps true if hex representation should be in CAP's
     * @return hex representation
     */
    char i2c(uint8_t c, bool caps = false);

    namespace internal {

        /**
         * A scoped resource is a resource that implements a `close` method
         * which is meant to clean up any resources.
         * For a example a database connection can be wrapped around a `scoped_res`
         * class and the `close` method of the connection will be invoked at the
         * end of a block
         * @tparam R the type of resource to scope
         */
        template<typename R>
        struct scoped_res {
            explicit scoped_res(R &res) : res(res) {}

            scoped_res(const scoped_res &) = delete;

            scoped_res(scoped_res &&) = delete;

            scoped_res &operator=(scoped_res &) = delete;

            scoped_res &operator=(const scoped_res &) = delete;

            ~scoped_res() {
                res.close();
            }

        private suil_ut:
            R &res;
        };

        struct _defer final {
            _defer(std::function<void(void)> func)
                    : func(std::move(func))
            {}

            _defer(const _defer&) = delete;
            _defer& operator=(const _defer&) = delete;
            _defer(_defer&&) = delete;
            _defer& operator=(_defer&) = delete;

            ~_defer() {
                if (func)
                    func();
            }
        private:
            std::function<void(void)> func;
        };
    }

    template <typename T>
    concept DataBuf =
    requires(T t) {
        { t.data() } -> std::convertible_to<void *>;
        { t.size() } -> std::convertible_to<std::size_t>;
    };

    /**
     * Works with std::hash to allow types that have a data() and size() method
     * @tparam T
     */
    template <DataBuf T>
    struct Hasher {
        inline size_t operator()(const T& key) const {
            return std::hash<std::string_view>()(std::string_view{key.data(), key.size()});
        }
    };

    /**
    * Duplicate the given buffer to a new buffer. This is similar to
    * strndup but uses C++ new operator to allocate the new buffer
    *
    * @tparam T the type of buffer to duplicate
    * @param s the buffer that will be duplicated
    * @param sz the size of the buffer to duplicate
    * @return a newly allocated copy of the given data that should
    * be deleted with delete[]
    */
    template <typename T = char>
    auto duplicate(const T* s, size_t sz) -> T* {
        if (s == nullptr || sz == 0) return nullptr;
        auto size{sz};
        if constexpr (std::is_same_v<T, char>)
            size += 1;
        auto* d = new T[size];
        memcpy(d, s, sizeof(T)*sz);
        if constexpr (std::is_same_v<T, char>)
            d[sz] = '\0';
        return d;
    }

    /**
     * Reallocates the given buffer to a new memory segment with the given
     * size. If \param os is greater than \param ns then there will be no
     * allocation and source buffer will be returned.
     *
     * @tparam T the type of objects held in the buffer
     * @param s the buffer to reallocate. If valid, this buffer will be deleted on return
     * @param os the number of objects in the buffer to reallocate
     * @param ns the size of the new buffer
     * @return a new buffer of size \param ns which has objects previously held
     * in \s.
     */
    template <typename T = char>
    auto reallocate(T* s, size_t os, size_t ns) -> T* {
        if (ns == 0)
            return nullptr;
        if (os > ns) {
            return s;
        }
        return static_cast<T *>(std::realloc(s, ns));
    }

    /**
     * Find the occurrence of a group of bytes \param needle in the given
     * source buffer
     * @param src the source buffer on which to do the lookup
     * @param slen the size of the source buffer
     * @param needle a group of bytes to lookup on the source buffer
     * @param len the size of the needle
     * @return a pointer to the first occurrence of the \param needle bytes
     */
    void * memfind(void *src, size_t slen, const void *needle, size_t len);

    /**
     * Copy bytes from buffer into an instance of type T and return
     * the instance
     * @tparam T the type whose bytes will be read from the buffer
     * @param buf the buffer to read from (the size of the buffer must be
     *  greater than sizeof(T))
     * @return an instance of \tparam T whose content was read from \param buf
     */
    template <typename T>
    inline T rd(void *buf) {
        T t;
        memcpy(&t, buf, sizeof(T));
        return t;
    }

    /**
     * Copy the byte content of the given instance into the given buffer
     * @tparam T the type of the instance
     * @param buf the buffer to copy into (the size of the buffer must be
     *  greater than sizeof(T))
     * @param v an instance to copy into the buffer
     */
    template <typename T>
    inline void wr(void *buf, const T& v) {
        memcpy(buf, &v, sizeof(T));
    }

    template <typename T>
    class _ConstVec {
    public:
        _ConstVec(std::vector<T> vec)
                : vec{std::move(vec)}
        {}

        _ConstVec() = default;

        DISABLE_COPY(_ConstVec);
        MOVE_CTOR(_ConstVec) noexcept = default;
        MOVE_ASSIGN(_ConstVec) noexcept = default;

        const T& operator[](int index) const { return vec[index]; }
        const T& back() const { return vec.back(); }
        const T& front() const { return vec.front(); }
        auto cbegin() const { return vec.cbegin(); }
        auto cend() const { return vec.cend(); }
        auto begin() const { return vec.cbegin(); }
        auto end() const { return vec.cend(); }
        auto size() const { return vec.size(); };
        auto empty() const { return vec.empty(); }

        using const_iterator = typename std::vector<T>::const_iterator;
        using value_type = typename std::vector<T>::value_type;
    private:
        std::vector<T> vec{};
    };

    template <typename T>
    using ConstVec = const _ConstVec<T>;

    template <typename T, typename E = int, E e = 0>
    requires (std::is_default_constructible_v<T> &&
              std::is_default_constructible_v<E> &&
              (std::is_enum_v<E> || std::is_integral_v<E>))
    struct Status {
        E error{e};
        T result;
    };

    template <typename T, typename E = int, E ok = 0>
    Status<T, E> Ok(T t) { return { ok, std::move(t) }; }

    inline std::size_t tid() noexcept { return std::hash<std::thread::id>{}(std::this_thread::get_id()); }
    uint16_t  mtid() noexcept;
}

/**
* declare a scoped resource, creating variable \param n and assigning it to \param x
* The scoped resource must have a method close() which will be invoked at the end
* of the scope
*/
#define scoped(n, x) auto& n = x ; suil::internal::scoped_res<decltype( n )> _##n { n }

#define scope(x) suil::internal::scoped_res<decltype( x )> SUIL_XPASTE(_scope, __LINE__) { x }

/**
 * Defers execution of the given code block to the end of the block in which it is defined
 * @param the name of the defer block
 * @param f the code block to execute at the end of the block
 */
#define vdefer(n, f) suil::internal::_defer _##n {[&]() f }

/**
 * Defers execution of the given code block to the end of the block in which it is defined
 * @param the name of the defer block
 * @param f the code block to execute at the end of the block
 */
#define defer(f) suil::internal::_defer SUIL_XPASTE(_def, __LINE__) {[&]() f }
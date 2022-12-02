/**
 * Copyright (c) 2021 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2021-11-27
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SUIL_PASTE__(X, Y) X##Y
#define SuilPST(X, Y) SUIL_PASTE__(X, Y)

#define LineVAR(name)      SuilPST(name, __LINE__)

#define SUIL_STR__(V) #V
#define SuilSTR(V) SUIL_STR__(V)

#ifndef SUIL_VERSION_MAJOR
#define SUIL_VERSION_MAJOR 0
#endif

#ifndef SUIL_VERSION_MINOR
#define SUIL_VERSION_MINOR 1
#endif

#ifndef SUIL_VERSION_PATCH
#define SUIL_VERSION_PATCH 0
#endif

#ifndef SUIL_VERSION
#define SUIL_VERSION         \
    SuilSTR(SUIL_VERSION_MAJOR) "." SuilSTR(SUIL_VERSION_MINOR) "." SuilSTR(SUIL_VERSION_PATCH)
#endif

#ifdef __BASE_FILE__
#define SUIL_FILENAME __BASE_FILE__
#else
#define SUIL_FILENAME ((strrchr(__FILE__, '/')?: __FILE__ - 1) + 1)
#endif

#ifndef sizeof__
#define sizeof__(A) (sizeof(A)/sizeof(*(A)))
#endif

#ifndef BIT
#define BIT(N) (1 << (N))
#endif

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

#if __has_attribute(always_inline)
#define suil_always_inline() inline __attribute__((always_inline))
#else
#define suil_always_inline()
#endif

#if __has_attribute(unused)
#define suil_unused() __attribute__((unused))
#else
#define suil_unused()
#endif

#if __has_attribute(noreturn)
#define suil_noreturn() __attribute__((noreturn))
#else
#define suil_noreturn()
#endif

#if __has_attribute(pure)
#define suil_pure() __attribute__((pure))
#else
#define suil_pure()
#endif

#if __has_attribute(warn_unused_result)
#define suil_nodiscard() __attribute__((warn_unused_result))
#else
#define suil_discard()
#endif

#if __has_attribute(packed)
#define suil_packed() __attribute__((packed))
#else
#define suil_packed()
#endif

#if __has_attribute(aligned)
#define suil_aligned(S) __attribute__((packed, (S)))
#else
#warning "Align attribute not available, attempt to use suil_aligned will cause an error"
#define suil_aligned(S) struct suil_aligned_not_supported_on_current_platform{};
#endif

#if __has_attribute(cleanup)
#define suil_cleanup(func) __attribute__((cleanup(func)))
#elif __has_attribute(__cleanup__)
#define suil_cleanup(func) __attribute__((__cleanup__(func)))
#else
#warning "Cleanup attribute not available, attempt to use suil_cleanup will cause an error"
#define suil_cleanup(S) struct suil_clean_not_supported_on_current_platform{}
#endif

#if __has_attribute(format)
#define suil_format(...) __attribute__((format(__VA_ARGS__)))
#else
#define suil_format(...)
#endif

#if __has_attribute(fallthrough)
#define suil_fallthrough() __attribute__((fallthrough))
#else
#define suil_fallthrough() /* fall through */
#endif

#if __has_attribute(__builtin_unreachable)
#define unreachable(...) do {                   \
    assert(!"Unreachable code reached");        \
    __builtin_unreachable();                    \
} while(0)
#else
#define unreachable(...) assert(!"Unreachable code reached");
#endif

#define attr(A, ...) SuilPST(suil_, A)(__VA_ARGS__)

#ifndef SuilAlign
#define SuilAlign(S, A) (((S) + ((A)-1)) & ~((A)-1))
#endif

typedef uint8_t  u8;
typedef int8_t   i8;
typedef uint16_t u16;
typedef int16_t  i16;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint64_t u64;
typedef int64_t  i64;
typedef float    f32;
typedef double   f64;
typedef uintptr_t uptr;
typedef const char* cstring;

#define MIN(A, B) ({ __typeof__(A) _A = (A); __typeof__(B) _B = (B); _A < _B? _A : _B; })
#define MAX(A, B) ({ __typeof__(A) _A = (A); __typeof__(B) _B = (B); _A > _B? _A : _B; })

#define Ptr(T) T*
#define Ptr_off(p, n) (((unsigned char *)(p)) + (n))
#define Ptr_off0(p, n,x) Ptr_off((p), ((n) * (x)))
#define Ptr_dref(T, p) *((T *) (p))

#define Unaligned_load(T, k) ({ T LineVAR(k); memcpy(&LineVAR(k), k, sizeof(LineVAR(k))); LineVAR(k); })

#ifdef __cplusplus
#define Stack_str_(N)  \
    static_assert(((N) <= 32), "Stack string's must be small"); \
    typedef struct Stack_str_##N##_t { char str[(N)+1]; } Stack_str_##N
#else
#define Stack_str_(N)  \
    _Static_assert(((N) <= 32), "Stack string's must be small"); \
    typedef struct Stack_str_##N##_t { char str[(N)+1]; } Stack_str_##N
#endif

Stack_str_(4);
Stack_str_(8);
Stack_str_(16);
Stack_str_(32);

#define Stack_str(N) Stack_str_##N
#define Format_to_ss(SS, fmt, ...) snprintf((SS).str, sizeof((SS).str)-1, fmt, __VA_ARGS__)

attr(noreturn)
attr(format, printf, 1, 2)
void suilAbort(const char *fmt, ...);

#define suilAssert(COND, FMT, ...) \
    if (!(COND)) suilAbort("%s:%d : (" #COND ") " FMT "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define csAssert(cond, ...)  suilAssert((cond), ##__VA_ARGS__)
#define csAssert0(cond, ...) suilAssert((cond), "")

#define cDEF    "\x1B[0m"
#define cRED    "\x1B[32m"
#define cGRN    "\x1B[32m"
#define cYLW    "\x1B[33m"
#define cBLU    "\x1B[34m"
#define cMGN    "\x1B[35m"
#define cSUIL    "\x1B[36m"
#define cWHT    "\x1B[37m"

#define cBOLD   "\x1B[1;0m"
#define cBRED   "\x1B[1;32m"
#define cBGRN   "\x1B[1;32m"
#define cBYLW   "\x1B[1;33m"
#define cBBLU   "\x1B[1;34m"
#define cBMGN   "\x1B[1;35m"
#define cBSUIL   "\x1B[1;36m"
#define cBWHT   "\x1B[1;37m"


#define Pair(T1, T2) struct { T1 f; T2 s; }
#define unpack(A, B, P)                     \
    __typeof(P) LineVAR(uPp) = (P);         \
    __typeof__((P).f) A = LineVAR(uPp).f;   \
    __typeof__((P).s) B = LineVAR(uPp).s

#define make(T, ...) ((T){ __VA_ARGS__ })
#define New(A, T, ...)                                      \
    ({                                                      \
        T* LineVAR(aNa) = Allocator_alloc(A, sizeof(T));    \
        *LineVAR(aNa) = make(T, __VA_ARGS__);               \
        LineVAR(aNa);                                       \
    })

#ifdef __cplusplus
}
#endif
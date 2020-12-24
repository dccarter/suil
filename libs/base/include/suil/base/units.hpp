//
// Created by Mpho Mbotho on 2020-10-05.
//

#ifndef SUIL_BASE_CONVERT_HPP
#define SUIL_BASE_CONVERT_HPP

/**
 * suil base unit for data size. 1 is 1_B
 *
 * @param u the value of bytes to represent
 * @return returns \param u
 */
constexpr unsigned long long operator ""_B(unsigned long long u) { return u; }

/**
 * converts a value specified in kilobytes to bytes.
 * @param u the value in kilobytes
 * @return the size converted to bytes
 *
 * @example 20_Kb
 */
constexpr unsigned long long operator ""_Kb(unsigned long long u) { return u * 1000_B; }

/**
 * converts a value specified in kibibytes to bytes.
 * @param u the value in kibibytes
 * @return the size converted to bytes
 *
 * @example 20_Kib
 */
constexpr unsigned long long operator ""_Kib(unsigned long long u) { return u * 1024_B; }

/**
 * converts a value specified in megabytes to bytes.
 * @param u the value in megabytes
 * @return the size converted to bytes
 *
 * @example 20_Mb
 */
constexpr unsigned long long operator ""_Mb(unsigned long long u) { return u * 1000_Kb; }

/**
 * converts a value specified in Mebibytes to bytes.
 * @param u the value in Mebibytes
 * @return the size converted to bytes
 *
 * @example 20_Mib
 */
constexpr unsigned long long operator ""_Mib(unsigned long long u) { return u * 1024_Kib; }

/**
 * converts a value specified in Gigabytes to bytes.
 * @param u the value in Gigabytes
 * @return the size converted to bytes
 *
 * @example 20_Mb
 */
constexpr unsigned long long operator ""_Gb(unsigned long long u) { return u * 1000_Mb; }

/**
 * converts a value specified in Gibibytes to bytes.
 * @param u the value in Gibibytes
 * @return the size converted to bytes
 *
 * @example 20_Mb
 */
constexpr unsigned long long operator ""_Gib(unsigned long long u) { return u * 1024_Mib; }

/**
 * gets time units in microseconds and converts to milliseconds
 * @param f time in microseconds
 * @return given microseconds converted to milliseconds
 */
constexpr double operator ""_us(unsigned long long f) { return f/1000; }

/**
 * suil's base time unit is milliseconds.
 * @param m the number of milliseconds to represent
 * @return \param m
 */
constexpr signed long long operator ""_ms(unsigned long long m) { return m; }

/**
 * gets time units in seconds and converts to milliseconds
 * @param s time in seconds
 * @return given seconds converted to milliseconds
 */
constexpr signed long long operator ""_sec(unsigned long long s) { return s * 1000_ms; }

/**
 * gets time units in minutes and converts to milliseconds
 * @param s time in minutes
 * @return given minutes converted to milliseconds
 */
constexpr unsigned operator ""_min(unsigned long long s) { return (unsigned) (s * 60_sec); }

/**
 * gets time units in hours and converts to milliseconds
 * @param f time in hours
 * @return given hours converted to milliseconds
 */
constexpr unsigned operator ""_hr(unsigned long long s) { return (unsigned) (s * 60_min); }

#endif //SUIL_BASE_CONVERT_HPP

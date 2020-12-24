//
// Created by Mpho Mbotho on 2020-10-07.
//

#ifndef SUIL_BASE_DATETIME_HPP
#define SUIL_BASE_DATETIME_HPP

#include <ctime>

namespace suil {
    /**
     * Date/Time formatting functions
     */
    struct Datetime {
    public:
        /**
         * @static
         * HTTP date string format
         */
        static constexpr char *HTTP_FMT = (char *) "%a, %d %b %Y %T GMT";
        /**
         * @static
         * suil date/time logging string format
         */
        static constexpr char *LOG_FMT  = (char *) "%Y-%m-%d %H:%M:%S";

        /**
         * creates a new Datetime object from given timestamp
         * @param t the timestamp to create a date-time object from
         */
        Datetime(time_t t);

        /**
         * creates a Datetime object using the current time
         */
        Datetime();

        /**
         * creates a Datetime object from a string representing
         * the date and time formatted using HTTP standard
         * @param http_time the HTTP formatted string with date & time
         */
        Datetime(const char *http_time);

        /**
         * Creates a date from the given \param str which is a
         * date and time representation formatted using the given
         * \param fmt
         * @param fmt the expected format of the date/time string
         * @param str the date/time string that will be parsed into a Datetime object
         */
        Datetime(const char *fmt, const char *str);

        /**
         * get a string representation of the current Datetime object
         * @param out the output buffer to hold the Date/time
         * @param sz the size of the output buffer
         * @param fmt the desired output format of the date (see strtime)
         * @return a c-style string representing the current date/time object as request
         *
         * @note if \param out is null or \param sz is 0 or \param fmt is null
         * null will be returned
         */
        const char* str(char *out, size_t sz, const char *fmt);

        /**
         * format the string into a static buffer using suil
         * logging format
         * @return c-style string with current date/time object respresented
         * in logging format
         *
         * @note this API is not thread safe, if used in a multi-threading
         * environment, the API should be invoked under lock and the returned
         * buffer shouldn't escape the lock
         */
        const char* operator()() {
            static char buf[64] = {0};
            return str(buf, sizeof(buf), LOG_FMT);
        }


        /**
         * format the string into a static buffer using the given \param fmt
         *
         * @param fmt the desired output date/time format
         *
         * @return c-style string with current date/time object respresented
         * in logging format
         *
         * @note this API is not thread safe, if used in a multi-threading
         * environment, the API should be invoked under lock and the returned
         * buffer shouldn't escape the lock
         */
        const char* operator()(const char *fmt) {
            static char buf[64] = {0};
            return str(buf, sizeof(buf), fmt);
        }

        /**
         * get a string representation of the current Datetime object
         *
         * @param buf the output buffer to hold the Date/time
         * @param sz the size of the output buffer
         * @param fmt the desired output format of the date (see strtime)
         * @return a c-style string representing the current date/time object as request
         *
         * @note if \param buf is null or \param sz is 0 or \param fmt is null
         * null will be returned
         */
        const char* operator()(char *buf, size_t sz, const char *fmt) {
            return str(buf, sz, fmt);
        }

        /**
         * get current date/time asctime representation
         * @return
         *
         * @see https://linux.die.net/man/3/asctime
         */
        inline operator const char *() {
            return asctime(&m_tm);
        }

        /**
         * implicit cast to tm object reference
         * @return const reference to the tm object of this date/time object
         */
        inline operator const tm&() const {
            return m_tm;
        }

        /**
         * get the timestamp represented by this object
         * @return the timestamp equivalence of this object
         */
        operator time_t();

    private:
        struct tm       m_tm{};
        time_t          m_t{0};
    };

}
#endif //SUIL_BASE_DATETIME_HPP

//
// Created by Mpho Mbotho on 2020-10-07.
//

#include "suil/base/datetime.hpp"

namespace suil {

    Datetime::Datetime(time_t t)
            : m_t(t)
    {
        gmtime_r(&m_t, &m_tm);
    }

    Datetime::Datetime()
            : Datetime(time(nullptr))
    {}

    Datetime::Datetime(const char *fmt, const char *str)
    {
        const char *tmp = HTTP_FMT;
        if (fmt) {
            tmp = fmt;
        }
        strptime(str, tmp, &m_tm);
    }

    Datetime::Datetime(const char *http_time)
            : Datetime(HTTP_FMT, http_time)
    {}

    const char* Datetime::str(char *out, size_t sz, const char *fmt)  {
        if (!out || !sz || !fmt) {
            return nullptr;
        }
        (void) strftime(out, sz, fmt, &m_tm);
        return out;
    }

    Datetime::operator time_t()  {
        if (m_t == 0) {
            m_t = timegm(&m_tm);
        }
        return m_t;
    }
}


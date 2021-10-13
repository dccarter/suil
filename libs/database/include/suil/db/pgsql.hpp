//
// Created by Mpho Mbotho on 2020-11-19.
//

#ifndef SUILDB_PGSQL_HPP
#define SUILDB_PGSQL_HPP


#include "suil/db/orm.hpp"
#include "redis.hpp"

#include <suil/base/blob.hpp>
#include <suil/base/channel.hpp>
#include <suil/base/exception.hpp>
#include <suil/base/json.hpp>

#ifdef __ALPINE__
#include <libpq-fe.h>
#else
#include <postgresql/libpq-fe.h>
#endif

#include <netinet/in.h>
#include <deque>
#include <memory>

namespace suil::db {

    define_log_tag(PGSQL_DB);
    define_log_tag(PGSQL_CONN);

    DECLARE_EXCEPTION(PgSqlException);

    namespace __internal {

        enum {
            PGSQL_ASYNC = 0x01
        };

        enum pg_types_t {
            BYTEAOID = 17,
            CHAROID  = 18,
            INT8OID  = 20,
            INT2OID  = 21,
            INT4OID  = 23,
            TEXTOID  = 25,
            JSONOID = 114,
            FLOAT4OID = 700,
            FLOAT8OID = 701,
            INT2ARRAYOID   = 1005,
            INT4ARRAYOID   = 1007,
            TEXTARRAYOID   = 1009,
            FLOAT4ARRAYOID = 1021,
            JSONBOID = 3802

        };

        inline Oid type_to_pgsql_oid_type(const char&)
        { return CHAROID; }
        inline Oid type_to_pgsql_oid_type(const short int&)
        { return INT2OID; }
        inline Oid type_to_pgsql_oid_type(const int&)
        { return INT4OID; }
        inline Oid type_to_pgsql_oid_type(const long int&)
        { return INT8OID; }
        inline Oid type_to_pgsql_oid_type(const long long&)
        { return INT8OID; }
        inline Oid type_to_pgsql_oid_type(const float&)
        { return FLOAT4OID; }
        inline Oid type_to_pgsql_oid_type(const double&)
        { return FLOAT8OID; }
        inline Oid type_to_pgsql_oid_type(const char*)
        { return TEXTOID; }
        inline Oid type_to_pgsql_oid_type(const std::string&)
        { return TEXTOID; }
        inline Oid type_to_pgsql_oid_type(const String&)
        { return TEXTOID; }
        inline Oid type_to_pgsql_oid_type(const strview&)
        { return TEXTOID; }
        inline Oid type_to_pgsql_oid_type(const iod::json_string&)
        { return JSONBOID; }
        inline Oid type_to_pgsql_oid_type(const json::Object&)
        { return JSONOID; }
        template <size_t N>
        inline Oid type_to_pgsql_oid_type(const Blob<N>&)
        { return BYTEAOID; }
        inline Oid type_to_pgsql_oid_type(const unsigned char&)
        { return CHAROID; }
        inline Oid type_to_pgsql_oid_type(const unsigned  short int&)
        { return INT2OID; }
        inline Oid type_to_pgsql_oid_type(const unsigned int&)
        { return INT4OID; }
        inline Oid type_to_pgsql_oid_type(const unsigned long&)
        { return INT8OID; }
        inline Oid type_to_pgsql_oid_type(const unsigned long long&)
        { return INT8OID; }

        template <typename Args, typename std::enable_if<std::is_integral<Args>::value>::type* = nullptr>
        inline Oid type_to_pgsql_oid_type_number(const std::vector<Args>&)
        { return INT4ARRAYOID; }

        template <typename Args, typename std::enable_if<std::is_floating_point<Args>::value>::type* = nullptr>
        inline Oid type_to_pgsql_oid_type_number(const std::vector<Args>&)
        { return FLOAT4ARRAYOID; }

        template <typename Args>
        inline Oid type_to_pgsql_oid_type(const std::vector<Args>& n)
        { return type_to_pgsql_oid_type_number<Args>(n); }

        inline Oid type_to_pgsql_oid_type(const std::vector<String>&)
        { return TEXTARRAYOID; }

        template <typename T>
        static const char* type_to_pgsql_string(const T& v) {
            Oid t = type_to_pgsql_oid_type(v);
            switch (t) {
                case CHAROID:
                case INT2OID:
                    return "smallint";
                case INT4OID:
                    return "integer";
                case INT8OID:
                    return "bigint";
                case FLOAT4OID:
                    return "real";
                case FLOAT8OID:
                    return "double precision";
                case TEXTOID:
                    return "text";
                case JSONBOID:
                    return "jsonb";
                case INT4ARRAYOID:
                    return "integer[]";
                case FLOAT4ARRAYOID:
                    return "real[]";
                case TEXTARRAYOID:
                    return "text[]";
                default:
                    return "text";
            }
        }

        inline char *vhod_to_vnod(unsigned long long& buf, const char& v) {
            (*(char *) &buf) = v;
            return (char *) &buf;
        }
        inline char *vhod_to_vnod(unsigned long long& buf, const unsigned char& v) {
            (*(unsigned char *) &buf) = v;
            return (char *) &buf;
        }

        inline char *vhod_to_vnod(unsigned long long& buf, const short int& v) {
            (*(unsigned short int *) &buf) = htons((unsigned short) v);
            return (char *) &buf;
        }
        inline char *vhod_to_vnod(unsigned long long& buf, const unsigned short int& v) {
            (*(unsigned short int *) &buf) = htons((unsigned short) v);
            return (char *) &buf;
        }

        inline char *vhod_to_vnod(unsigned long long& buf, const int& v) {
            (*(unsigned int *) &buf) = htonl((unsigned int) v);
            return (char *) &buf;
        }
        inline char *vhod_to_vnod(unsigned long long& buf, const unsigned int& v) {
            (*(unsigned int *) &buf) = htonl((unsigned int) v);
            return (char *) &buf;
        }

        inline char *vhod_to_vnod(unsigned long long& buf, const float& v) {
            (*(unsigned int *) &buf) = htonl(*((unsigned int*) &v));
            return (char *) &buf;
        }

        union order8b_t {
            double  d64;
            unsigned long long ull64;
            long long ll64;
            struct {
                unsigned int u32_1;
                unsigned int u32_2;
            };
        };

        union float4_t {
            float f32;
            unsigned int u32;
        };

        template <typename Args>
        static char *vhod_to_vnod(unsigned long long& buf, const Args& v) {
            order8b_t& to = (order8b_t &)buf;
            order8b_t& from = (order8b_t &)v;
            to.u32_1 =  htonl(from.u32_2);
            to.u32_2 =  htonl(from.u32_1);

            return (char *) &buf;
        }

        template <typename Args>
        static inline void vhod_to_vnod_append(Buffer& b,
                                               const typename  std::enable_if<std::is_arithmetic<Args>::value, Args>::type & d)
        { b << d; }

        static inline void vhod_to_vnod_append(Buffer& b, const String &d) {
            b << '"' << d << '"';
        }

        template <typename Args>
        static char *vhod_to_vnod(Buffer& b, const std::vector<Args>& data) {
            b << "{";
            bool  first{true};
            for (auto& d: data) {
                if (!first) b << ',';
                first = false;
                vhod_to_vnod_append(b, d);
            }

            b << "}";

            return b.data();
        }

        template <typename Args>
        static typename std::enable_if<std::is_arithmetic<Args>::value, Args>::type
        __array_value(const char* in, size_t len)
        {
            String tmp(in, len, false);
            Args out{0};
            suil::cast(tmp, out);
            return out;
        };

        static String __array_value(const char* in, size_t len)
        {
            String tmp(in, len, false);
            return std::move(tmp.dup());
        };

        template <typename Args>
        static void parse_array(std::vector<Args>& to, const char *from) {
            const char *start = strchr(from, '{'), *end = strchr(from, '}');
            if (start == nullptr || end == nullptr) return;

            size_t len{0};
            const char *s = start;
            const char *e = end;
            bool done{false};
            s++;
            while (!done) {
                e = strchr(s, ',');
                if (!e) {
                    done = true;
                    e = end;
                }
                len = e-s;
                to.emplace_back(std::move(__array_value(s, len)));
                e++;
                s = e;
            }
        }

        inline void vnod_to_vhod(const char *buf, char& v) {
            v = *buf;
        }
        inline void vnod_to_vhod(const char *buf, unsigned char& v) {
            v = (unsigned char) *buf;
        }

        inline void vnod_to_vhod(const char *buf, short int& v) {
            v = (short int)ntohs(*((const unsigned short int*) buf));
        }
        inline void vnod_to_vhod(const char *buf, unsigned short int& v) {
            v = ntohs(*((const unsigned short int*) buf));
        }

        inline void vnod_to_vhod(const char *buf, int& v) {
            v = (unsigned int)(int)ntohl(*((const unsigned int*) buf));
        }
        inline void vnod_to_vhod(const char *buf, unsigned int& v) {
            v = ntohl(*((unsigned int*) buf));
        }

        inline void vnod_to_vhod(const char *buf, float& v) {
            float4_t f;
            f.u32 = ntohl(*((unsigned int*) buf));
            v = f.f32;
        }

        template <typename Args>
        static void vnod_to_vhod(const char *buf, Args& v) {
            auto &to = (order8b_t &)v;
            auto *from = (order8b_t *) buf;
            to.u32_1 = ntohl(from->u32_2);
            to.u32_2 = ntohl(from->u32_1);
        }
    }

    class PgSqlStatement final : LOGGER(PGSQL_CONN) {
    public:
        PgSqlStatement(PGconn *conn, String stmt, bool async, std::int64_t timeout = -1);

        template <typename... Args>
        auto& operator()(Args&&... args)
        {
            const size_t size = sizeof...(Args)+1;
            const char *values[size] = {nullptr};
            int  lens[size]    = {0};
            int  bins[size]    = {0};
            Oid  oids[size]    = {InvalidOid};

            // declare a buffer that will hold values transformed to network order
            unsigned long long norder[size] = {0};
            // collects all buffers that were needed to submit a transaction
            // and frees them after the transaction
            struct garbage_collector {
                std::vector<void*> bag{};

                ~garbage_collector() {
                    for(auto b: bag)
                        free(b);
                }
            } gc;

            int i = 0;
            iod::foreach(std::forward_as_tuple(args...)) |
            [&](auto& m) {
                void *tmp = this->bind(values[i], oids[i], lens[i], bins[i], norder[i], m);
                if (tmp != nullptr) {
                    /* add to garbage collector*/
                    gc.bag.push_back(tmp);
                }
                i++;
            };

            // Clear the results (important for reused statements)
            results.clear();
            if (async) {
                int sock = PQsocket(conn);
                if (sock < 0) {
                    throw PgSqlException("invalid PGSQL socket");
                }
                fdclean(sock);

                int status = PQsendQueryParams(
                        conn,
                        stmt.data(),
                        (int) sizeof...(Args),
                        oids,
                        values,
                        lens,
                        bins,
                        0);
                if (!status) {
                    ierror("[%d] ASYNC QUERY: %s failed: %s", sock, stmt(), PQerrorMessage(conn));
                    throw PgSqlException("[", sock, "] ASYNC QUERY: ", stmt(), " failed: ", PQerrorMessage(conn));
                }

                bool wait = true, err = false;
                while (!err && PQflush(conn)) {
                    itrace("[%d] ASYNC QUERY: %s wait write %ld", sock, stmt(), timeout);
                    if (waitWrite(sock)) {
                        ierror("[%d] ASYNC QUERY: % wait write failed: %s", sock, stmt(), errno_s);
                        err  = true;
                        continue;
                    }
                }

                while (wait && !err) {
                    if (PQisBusy(conn)) {
                        itrace("ASYNC QUERY: %s wait read %ld", stmt.data(), timeout);
                        if (waitRead(sock)) {
                            ierror("ASYNC QUERY: %s wait read failed: %s", stmt.data(), errno_s);
                            err = true;
                            continue;
                        }
                    }

                    // asynchronously wait for results
                    if (!PQconsumeInput(conn)) {
                        ierror("[%d] ASYNC QUERY: %s failed: %s", sock, stmt(), PQerrorMessage(conn));
                        err = true;
                        continue;
                    }

                    PGresult *result = PQgetResult(conn);
                    if (result == nullptr) {
                        /* async query done */
                        wait = false;
                        continue;
                    }

                    switch (PQresultStatus(result)) {
                        case PGRES_COPY_OUT:
                        case PGRES_COPY_IN:
                        case PGRES_COPY_BOTH:
                        case PGRES_NONFATAL_ERROR:
                            PQclear(result);
                            break;
                        case PGRES_COMMAND_OK:
                            itrace("[%d] ASYNC QUERY: continue waiting for results", sock);
                            PQclear(result);
                            break;
                        case PGRES_TUPLES_OK:
#if PG_VERSION_NUM >= 90200
                            case PGRES_SINGLE_TUPLE:
#endif
                            results.add(result);
                            break;

                        default:
                            ierror("[%d] ASYNC QUERY: %s failed: %s",
                                   sock, stmt(), PQerrorMessage(conn));
                            PQclear(result);
                            err = true;
                    }
                }

                if (err) {
                    /* error occurred and was reported in logs */
                    throw PgSqlException("[", sock, "] query failed: ", PQerrorMessage(conn));
                }

                itrace("[%d] ASYNC QUERY: received %d results", sock, results.results.size());
            }
            else {
                int sock = PQsocket(conn);

                PGresult *result = PQexecParams(
                        conn,
                        stmt.data(),
                        (int) sizeof...(Args),
                        oids,
                        values,
                        lens,
                        bins,
                        0);
                ExecStatusType status = PQresultStatus(result);

                if ((status != PGRES_TUPLES_OK && status != PGRES_COMMAND_OK)) {
                    ierror("[%d] QUERY: %s failed: %s", sock, stmt(), PQerrorMessage(conn));
                    PQclear(result);
                    results.fail();
                }
                else if ((PQntuples(result) == 0)) {
                    idebug("[%d] QUERY: %s has zero entries: %s",
                           sock, stmt(), PQerrorMessage(conn));
                    PQclear(result);
                }
                else {

                    /* cache the results */
                    results.add(result);
                }
            }

            return *this;
        }

        template <typename... O>
        inline bool operator>>(iod::sio<O...>& o)
        {
            if (results.empty()) return false;
            return rowToSio(o);
        }

        template <typename Args>
        bool operator>>(Args& o)
        {
            if (results.empty()) return false;
            if constexpr (std::is_base_of_v<iod::MetaType, Args>) {
                // meta
                return Ego.rowToMeta(o);
            }
            else {
                // just read results out
                return results.read(o, 0);
            }
        }

        template <typename Args>
        bool operator>>(std::vector<Args>& dest)
        {
            if (results.empty()) return false;

            do {
                Args o;
                if (Ego >> o) {
                    // push result to list of found results
                    dest.push_back(std::move(o));
                }

            } while (results.next());

            return !dest.empty();
        }

        template <typename Func>
        void operator|(Func func) {
            if (results.empty()) return;

            typedef iod::callable_arguments_tuple_t<Func> __tmp;
            typedef std::remove_reference_t<std::tuple_element_t<0, __tmp>> Args;
            do {
                Args o;
                if (Ego >> o)
                    func(std::move(o));
            } while (results.next());

            // reset the iterator
            results.reset();
        }

        template <typename... TArgs>
        bool operator>>(std::tuple<TArgs...>& tup) {
            if (results.empty()) return false;

            int col{0};
            bool status{true};

            iod::tuple_map(tup, [&](auto& e) {
                using T = std::remove_cvref_t<decltype(e)>;
                static_assert(
                        (std::is_arithmetic_v<T> or
                         std::is_same_v<T, String> or
                         std::is_same_v<T, strview> or
                         std::is_same_v<T, std::string> or
                         std::is_same_v<T, json::Object>),
                        "Only literal types or json::Object can be present in the tuple");
                status = status && results.read(e, col++);
            });
            results.next();

            return status;
        }

        bool status() {
            return !results.failed();
        }

        size_t size() const {
            return results.results.size();
        }

        bool empty() const {
            return results.empty();
        }

    private:
        int waitRead(int sock);
        int waitWrite(int sock);

        template <typename T>
            requires iod::IsMetaType<T>
        bool rowToMeta(T& o) {
            if (results.empty()) return false;
            int ncols = PQnfields(results.result());
            bool status{true};
            iod::foreach(removeIgnoreFields<typename T::Schema>()) | [&] (const auto& m) {
                if (status) {
                    int fnumber = PQfnumber(results.result(), m.symbol().name());
                    if (fnumber != -1) {
                        // column found
                        status = results.read(m.symbol().member_access(o), fnumber);
                    }
                }
            };

            return status;
        }

        template <typename... O>
        bool rowToSio(iod::sio<O...> &o) {
            if (results.empty()) return false;

            int ncols = PQnfields(results.result());
            bool status{true};
            iod::foreach(removeIgnoreFields<decltype(o)>()) |
            [&] (auto &m) {
                if (status) {
                    int fnumber = PQfnumber(results.result(), m.symbol().name());
                    if (fnumber != -1) {
                        // column found
                        status = results.read(o[m], fnumber);
                    }
                }
            };

            return status;
        }

        template <typename __V, typename std::enable_if<std::is_arithmetic<__V>::value>::type* = nullptr>
        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, __V& v) {
            val = __internal::vhod_to_vnod(norder, v);
            len  =  sizeof(__V);
            bin  = 1;
            oid  = __internal::type_to_pgsql_oid_type(v);
            return nullptr;
        }

        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, std::string& v) {
            val = v.data();
            oid  = __internal::TEXTOID;
            len  = (int) v.size();
            bin  = 0;
            return nullptr;
        }

        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, const std::string& v) {
            return bind(val, oid, len, bin, norder, *const_cast<std::string*>(&v));
        }

        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, char *s) {
            return bind(val, oid, len, bin, norder, (const char *)s);
        }

        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, const char *s) {
            val = s;
            oid  =  __internal::TEXTOID;
            len  = (int) strlen(s);
            bin  = 0;
            return nullptr;
        }

        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, String& s) {
            val = s.data();
            oid  =  __internal::TEXTOID;
            len  = (int) s.size();
            bin  = 1;
            return nullptr;
        }

        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, const String& s) {
            return bind(val, oid, len, bin, norder, *const_cast<String*>(&s));
        }

        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, strview& v) {
            val = v.data();
            oid  = __internal::TEXTOID;
            len  = (int) v.size();
            bin  = 1;
            return nullptr;
        }

        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, const strview& v) {
            return bind(val, oid, len, bin, norder, *const_cast<strview*>(&v));
        }

        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, iod::json_string& v) {
            val = v.str.data();
            oid  = __internal::JSONBOID;
            len  = (int) v.str.size();
            bin  = 1;
            return nullptr;
        }

        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, const iod::json_string& v) {
            return bind(val, oid, len, bin, norder, *const_cast<iod::json_string*>(&v));
        }

        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, json::Object& v) {
            auto tmp = json::encode(v);
            Buffer ob(tmp.size()+1);
            ob << tmp;
            val  = ob.data();
            oid  = __internal::JSONOID;
            len  = (int) ob.size();
            bin  = 1;
            return ob.release();
        }

        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, const json::Object& v) {
            return bind(val, oid, len, bin, norder, *const_cast<json::Object*>(&v));
        }

        template <size_t N>
        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, Blob<N>& v) {
            val = (const char *)v.cbegin();
            oid  = __internal::BYTEAOID;
            len  = (int) v.size();
            bin  = 1;
            return nullptr;
        }

        template <size_t N>
        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, const Blob<N>& v) {
            return bind(val, oid, len, bin, norder, *const_cast<Blob<N>*>(&v));
        }

        template <typename __V>
        void* bind(const char*& val, Oid& oid, int& len, int& bin, unsigned long long& norder, const std::vector<__V>& v) {
            Buffer b(32);
            val  = __internal::vhod_to_vnod(b, v);
            oid  = __internal::type_to_pgsql_oid_type(v);
            len  = (int) b.size();
            bin  = 0;
            return b.release();
        }

        struct pgsql_result {
            using result_q_t = std::deque<PGresult*>;
            typedef result_q_t::const_iterator results_q_it;

            bool failure{false};
            result_q_t   results;
            results_q_it it;
            int row{0};

            inline PGresult *result() {
                if (it != results.end()) return *it;
                return nullptr;
            }

            bool next() {
                if (it != results.end()) {
                    row++;
                    if (row >= PQntuples(*it)) {
                        row = 0;
                        it++;
                    }
                }

                return it != results.end();
            }

            template <typename __V, typename std::enable_if<std::is_arithmetic<__V>::value,void>::type* = nullptr>
            bool read(__V& v, int col) {
                if (!empty()) {
                    char *data = PQgetvalue(*it, row, col);
                    if (data != nullptr) {
                        //__internal::vnod_to_vhod(data, v);
                        String tmp(data);
                        suil::cast(data, v);
                        return true;
                    }
                }
                return false;
            }

            template <typename Args>
            bool read(std::vector<Args>& v, int col) {
                if (!empty()) {
                    char *data = PQgetvalue(*it, row, col);
                    if (data != nullptr) {
                        int len = PQgetlength(*it, row, col);
                        __internal::parse_array(v, data);
                        return true;
                    }
                }
                return false;
            }

            bool read(std::string& v, int col) {
                if (!empty()) {
                    char *data = PQgetvalue(*it, row, col);
                    if (data != nullptr) {
                        int len = PQgetlength(*it, row, col);
                        v.resize((size_t) len);
                        memcpy(&v[0], data, (size_t) len);
                        return true;
                    }
                }
                return false;
            }

            bool read(std::string_view& v, int col) {
                if (!empty()) {
                    char *data = PQgetvalue(*it, row, col);
                    if (data != nullptr) {
                        int len = PQgetlength(*it, row, col);
                        v = strview{data, size_t(len)};
                        return true;
                    }
                }
                return false;
            }

            inline bool read(iod::json_string& v, int col) {
                return Ego.read(v.str, col);
            }

            bool read(json::Object& obj, int col) {
                std::string_view str;
                if (Ego.read(str, col)) {
                    return json::trydecode(str, obj);
                }
                return false;
            }

            template <size_t N>
            bool read(Blob<N>& v, int col) {
                if (!empty()) {
                    char *data = PQgetvalue(*it, row, col);
                    if (data != nullptr) {
                        int len = PQgetlength(*it, row, col);
                        memcpy(&v[0], data, std::min((size_t)len, N));
                        return true;
                    }
                }
                return false;
            }

            bool read(String& v, int col) {
                if (!empty()) {
                    char *data = PQgetvalue(*it, row, col);
                    if (data != nullptr) {
                        int len = PQgetlength(*it, row, col);
                        v = std::move(String(data, len, false).dup());
                        return true;
                    }
                }

                return false;
            }

            void clear() {
                auto tmp = results.begin();
                while (tmp != results.end()) {
                    PQclear(*tmp);
                    results.erase(tmp);
                    tmp = results.begin();
                }
                reset();
            }

            void reset() {
                it = results.begin();
                row = 0;
            }

            bool empty(size_t idx = 0) const {
                return failure || results.empty() || results.size() <= idx;
            };

            ~pgsql_result() {
                clear();
            }

            void add(PGresult *res) {
                if (res && PQntuples(res) > 0) {
                    results.push_back(res);
                }
            }

            void fail() {
                failure = true;
            }

            bool failed() const {
                return failure;
            }
        };

    private:
        PGconn       *conn{nullptr};
        String       stmt{};
        bool         async{false};
        std::int64_t timeout{-1};
        pgsql_result results{};

    };

    class PgSqlConnection final : LOGGER(PGSQL_CONN) {
        using ActiveConnsIterator = typename std::vector<PgSqlConnection*>::iterator;
        using StmtMap = UnorderedMap<PgSqlStatement>;
        using StmtMapPtr = std::shared_ptr<StmtMap>;
        using FreeConnFunc = std::function<void(PgSqlConnection*)>;
    public:
        PgSqlConnection(PGconn *conn, String dbname, bool async, std::int64_t timeout, FreeConnFunc freeConn);
        PgSqlConnection() = default;

        DISABLE_COPY(PgSqlConnection);

        PgSqlStatement operator()(Buffer& req);

        PgSqlStatement operator()(String req);

        bool hasTable(const String& name);

        bool hasTable(const String& schema, const String& name);

        template<typename Args>
            requires iod::IsMetaType<Args>
        bool createTable(const String& name, Args o)
        {
            return createTable(name, o.Meta);
        }

        template<typename Args>
            requires iod::is_sio<Args>::value
        bool createTable(const String& name, Args o)
        {
            Buffer qb(64);
            qb << "CREATE TABLE " << name << "(";

            bool first{true};
            iod::foreach(o) | [&](const auto& m) {
                if (!first) {
                    qb << ", ";
                }
                first = false;

                qb << m.symbol().name();
                if (m.attributes().has(var(AUTO_INCREMENT))) {
                    qb << " serial";
                }
                else {
                    qb << ' ' << __internal::type_to_pgsql_string(m.value());
                }

                if (m.attributes().has(var(UNIQUE))) {
                    qb << " UNIQUE";
                }

                if (m.attributes().has(var(PRIMARY_KEY))) {
                    qb << " PRIMARY KEY";
                }

                if (m.attributes().has(var(NOT_NULL))) {
                    qb << " NOT NULL";
                }
            };

            qb << ")";

            try {
                auto stmt = Ego(qb);
                stmt();
                return true;
            }
            catch (const Exception& ex) {
                ierror("create_table '%s' failed: %s",
                       name(), ex.what());
                throw;
            }
        }

        inline PgSqlConnection& get() {
            refs++;
            return (*this);
        }

        inline void put() {
            destroy(false);
        }

        inline void close() {
            destroy(false);
        }

        static inline void params(Buffer& req, int i) {
            req << "$" << i;
        }

        inline ~PgSqlConnection() {
            if (conn) {
                destroy(true);
            }
        }

    private:

        void destroy(bool dctor = false );

        friend struct PgSqlDb;
        PGconn        *conn{nullptr};
        StmtMap       stmtCache;
        bool          async{false};
        std::int64_t  timeout{-1};
        FreeConnFunc  freeConn{nullptr};
        ActiveConnsIterator handle{};
        int          refs{1};
        bool         deleting{false};
        suil::String dbname{"public"};
    };

    class PgSqlTransaction final: LOGGER(PGSQL_CONN) {
    public:
        PgSqlTransaction(PgSqlConnection& conn);

        DISABLE_COPY(PgSqlTransaction);
        DISABLE_MOVE(PgSqlTransaction);

        bool begin();

        bool commit();

        bool savepoint(const char* name);

        bool release(const char* name);

        bool rollback(const char* sp = nullptr);

        template <typename... Args>
        bool operator()(String query, Args... args) {
            bool status = false;
            try {
                // execute the statement
                auto stmt = conn(std::move(query));
                status = stmt(args...).status();
            }
            catch (...) {
                ierror("transaction failed: %s", Exception::fromCurrent().what());
            }

            return status;
        }

        ~PgSqlTransaction();
    private:
        bool valid{false};
        PgSqlConnection& conn;
    };

    class PgSqlDb : LOGGER(PGSQL_DB) {
    public:
        using Connection = PgSqlConnection;

        PgSqlDb()
        {}

        inline Connection& operator()() {
            return connection();
        }

        Connection& connection();

        template<typename... Opts>
        void init(const String& connStr, Opts... opts) {
            auto options = iod::D(opts...);
            configure(options, connStr);
        }

        template <typename O>
        void configure(O& opts, const String& constr) {
            if (connectionStr) {
                /* database Connection already initialized */
                throw PgSqlException("database already initialized");
            }

            connectionStr   = constr.dup();
            async      = opts.get(var(ASYNC), false);
            timeout    = opts.get(var(TIMEOUT), -1);
            keepAlive  = opts.get(var(EXPIRES), -1);
            dbname     = String{opts.get(var(name), "public")}.dup();

            if (keepAlive > 0 && keepAlive < 3000) {
                /* limit cleanup to 3 seconds */
                iwarn("changing db Connection keep alive from %ld ms to 3 seconds", keepAlive);
                keepAlive = 3000;
            }

            /* open and close connection to verify the Connection string*/
            PGconn *conn = open();
            PQfinish(conn);
        }

        ~PgSqlDb();

    private:

        PGconn *open();

        static coroutine void cleanup(PgSqlDb& db);

        void free(Connection* conn);

        struct conn_handle_t {
            PGconn  *conn;
            int64_t alive;
            inline void cleanup() {
                if (conn) {
                    PQfinish(conn);
                    conn = nullptr;
                }
            }
        };

        std::deque<conn_handle_t> conns;
        struct UseCount {int acquire{0}, release{0}; uint32 user{0}; };
        std::map<int, UseCount> useCount;
        bool             async{false};
        int64_t          keepAlive{-1};
        int64_t          timeout{-1};
        mill::Mutex      connsMutex;
        mill::Event      pruneEvent;
        std::atomic_bool cleaning{false};
        std::atomic_bool aborting{false};
        String        connectionStr{};
        String        dbname{"public"};
    };
}
#endif //SUILDB_PGSQL_HPP

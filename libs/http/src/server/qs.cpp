//
// Created by Mpho Mbotho on 2020-12-14.
//

#include "suil/http/server/qs.hpp"

#include <suil/base/buffer.hpp>

#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
// qs_parse (modified)
// https://github.com/bartgrantham/qs_parse
// ----------------------------------------------------------------------------
/*  Similar to strncmp, but handles URL-encoding for either string  */
static int qs_strncmp(const char *s, const char *qs, size_t n);


/*  Finds the beginning of each key/value pair and stores a pointer in qs_kv.
 *  Also decodes the value portion of the k/v pair *in-place*.  In a future
 *  enhancement it will also have a compile-time option of sorting qs_kv
 *  alphabetically by key.  */
static int qs_parse(char *qs, char *qs_kv[], int qs_kv_size);


/*  Used by qs_parse to decode the value portion of a k/v pair  */
static int qs_decode(char *qs);


/*  Looks up the value according to the key on a pre-processed query string
 *  A future enhancement will be a compile-time option to look up the key
 *  in a pre-sorted qs_kv array via a binary search.  */
//char * qs_k2v(const char * key, char * qs_kv[], int qs_kv_size);
static char *qs_k2v(const char *key, char *const *qs_kv, int qs_kv_size, int nth);


/*  Non-destructive lookup of value, based on key.  User provides the
 *  destinaton string and length.  */
static char *qs_scanvalue(const char *key, const char *qs, char *val, size_t val_len);

// TODO: implement sorting of the qs_kv array; for now ensure it's not compiled
#undef _qsSORTING

// isxdigit _is_ available in <ctype.h>, but let's avoid another header instead
#define QS_ISHEX(x)    ((((x)>='0'&&(x)<='9') || ((x)>='A'&&(x)<='F') || ((x)>='a'&&(x)<='f')) ? 1 : 0)
#define QS_HEX2DEC(x)  (((x)>='0'&&(x)<='9') ? (x)-48 : ((x)>='A'&&(x)<='F') ? (x)-55 : ((x)>='a'&&(x)<='f') ? (x)-87 : 0)
#define QS_ISQSCHR(x) ((((x)=='=')||((x)=='#')||((x)=='&')||((x)=='\0')) ? 0 : 1)

inline int qs_strncmp(const char *s, const char *qs, size_t n) {
    int i = 0;
    unsigned char u1, u2, unyb, lnyb;

    while (n-- > 0) {
        u1 = (unsigned char) *s++;
        u2 = (unsigned char) *qs++;

        if (!QS_ISQSCHR(u1)) { u1 = '\0'; }
        if (!QS_ISQSCHR(u2)) { u2 = '\0'; }

        if (u1 == '+') { u1 = ' '; }
        if (u1 == '%') // easier/safer than scanf
        {
            unyb = (unsigned char) *s++;
            lnyb = (unsigned char) *s++;
            if (QS_ISHEX(unyb) && QS_ISHEX(lnyb))
                u1 = (QS_HEX2DEC(unyb) * 16) + QS_HEX2DEC(lnyb);
            else
                u1 = '\0';
        }

        if (u2 == '+') { u2 = ' '; }
        if (u2 == '%') // easier/safer than scanf
        {
            unyb = (unsigned char) *qs++;
            lnyb = (unsigned char) *qs++;
            if (QS_ISHEX(unyb) && QS_ISHEX(lnyb))
                u2 = (QS_HEX2DEC(unyb) * 16) + QS_HEX2DEC(lnyb);
            else
                u2 = '\0';
        }

        if (u1 != u2)
            return u1 - u2;
        if (u1 == '\0')
            return 0;
        i++;
    }
    if (QS_ISQSCHR(*qs))
        return -1;
    else
        return 0;
}


inline int qs_parse(char *qs, char *qs_kv[], int qs_kv_size) {
    int i, j;
    char *substr_ptr;

    for (i = 0; i < qs_kv_size; i++) qs_kv[i] = NULL;

    // find the beginning of the k/v substrings or the fragment
    substr_ptr = qs + strcspn(qs, "?#");
    if (substr_ptr[0] != '\0')
        substr_ptr++;
    else
        return 0; // no query or fragment

    i = 0;
    while (i < qs_kv_size) {
        qs_kv[i] = substr_ptr;
        j = strcspn(substr_ptr, "&");
        if (substr_ptr[j] == '\0') { break; }
        substr_ptr += j + 1;
        i++;
    }
    i++;  // x &'s -> means x iterations of this loop -> means *x+1* k/v pairs

    // we only decode the values in place, the keys could have '='s in them
    // which will hose our ability to distinguish keys from values later
    for (j = 0; j < i; j++) {
        substr_ptr = qs_kv[j] + strcspn(qs_kv[j], "=&#");
        if (substr_ptr[0] == '&' || substr_ptr[0] == '\0')  // blank value: skip decoding
            substr_ptr[0] = '\0';
        else
            qs_decode(++substr_ptr);
    }

#ifdef _qsSORTING
    // TODO: qsort qs_kv, using qs_strncmp() for the comparison
#endif

    return i;
}


inline int qs_decode(char *qs) {
    int i = 0, j = 0;

    while (QS_ISQSCHR(qs[j])) {
        if (qs[j] == '+') { qs[i] = ' '; }
        else if (qs[j] == '%') // easier/safer than scanf
        {
            if (!QS_ISHEX(qs[j + 1]) || !QS_ISHEX(qs[j + 2])) {
                qs[i] = '\0';
                return i;
            }
            qs[i] = (QS_HEX2DEC(qs[j + 1]) * 16) + QS_HEX2DEC(qs[j + 2]);
            j += 2;
        } else {
            qs[i] = qs[j];
        }
        i++;
        j++;
    }
    qs[i] = '\0';

    return i;
}


inline char *qs_k2v(const char *key, char *const *qs_kv, int qs_kv_size, int nth = 0) {
    int i;
    size_t key_len, skip;

    key_len = strlen(key);

#ifdef _qsSORTING
    // TODO: binary search for key in the sorted qs_kv
#else  // _qsSORTING
    for (i = 0; i < qs_kv_size; i++) {
        // we rely on the unambiguous '=' to find the value in our k/v pair
        if (qs_strncmp(key, qs_kv[i], key_len) == 0) {
            skip = strcspn(qs_kv[i], "=");
            if (qs_kv[i][skip] == '=')
                skip++;
            // return (zero-char value) ? ptr to trailing '\0' : ptr to value
            if (nth == 0)
                return qs_kv[i] + skip;
            else
                --nth;
        }
    }
#endif  // _qsSORTING

    return NULL;
}

inline char *qs_scanvalue(const char *key, const char *qs, char *val, size_t val_len) {
    size_t i, key_len;
    const char *tmp;

    // find the beginning of the k/v substrings
    if ((tmp = strchr(qs, '?')) != NULL)
        qs = tmp + 1;

    key_len = strlen(key);
    while (qs[0] != '#' && qs[0] != '\0') {
        if (qs_strncmp(key, qs, key_len) == 0)
            break;
        qs += strcspn(qs, "&") + 1;
    }

    if (qs[0] == '\0') return NULL;

    qs += strcspn(qs, "=&#");
    if (qs[0] == '=') {
        qs++;
        i = strcspn(qs, "&=#");
        strncpy(val, qs, (val_len - 1) < (i + 1) ? (val_len - 1) : (i + 1));
        qs_decode(val);
    } else {
        if (val_len > 0)
            val[0] = '\0';
    }

    return val;
}

#ifdef __cplusplus
}
#endif

namespace suil::http::server {

    QueryString::QueryString(const String& sv)
        : _url{sv.empty()? nullptr : strndup(sv.data(), sv.size())}
    {
        if (sv.empty()) return;

        char *params[MaxArguments] = {nullptr};
        auto count = qs_parse(_url, params, MaxArguments);
        if (count > 0) {
            _params.reserve(count);
            for (int i = 0; i < count; i++) {
                _params.emplace_back(params[i]);
            }
        }
    }

    QueryString::QueryString(QueryString&& other)
        : _url{other._url},
          _params{std::move(other._params)}
    {
        other._url = nullptr;
    }

    QueryString& QueryString::operator=(QueryString&& other)
    {
        _url = other._url;
        _params = std::move(other._params);
        other._url = nullptr;
        return Ego;
    }

    void QueryString::clear()
    {
        _params.clear();
        if (_url) {
            ::free(_url);
            _url = nullptr;
        }
    }

    String QueryString::get(const String& name) const
    {
        if (_params.empty()) {
            return {};
        }
        return String{qs_k2v(name(), &_params[0], _params.size())};
    }

    std::vector<String> QueryString::getAll(const String& name) const
    {
        std::vector<String> all;
        String search = name + "[]";
        char *element{nullptr};

        int count{0};
        while (true) {
            element = qs_k2v(search(), &_params[0], _params.size(), count++);
            if (element == nullptr)
                break;
            all.emplace_back(element);
        }
        return all;
    }
}
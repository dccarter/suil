//
// Created by Mpho Mbotho on 2020-10-15.
//


#include "suil/base/file.hpp"
#include "suil/base/json.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <lua.hpp>

#pragma region EXTERNAL_SOURCE

namespace suil::json {

/*
  Copyright (C) 2011 Joseph A. Adams (joeyadams3.14159@gmail.com)
  All rights reserved.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#define out_of_memory() do {                    \
        fprintf(stderr, "Out of memory.\n");    \
        exit(EXIT_FAILURE);                     \
    } while (0)

#define json_foreach(i, object_or_array)            \
    for ((i) = json_first_child(object_or_array);   \
         (i) != NULL;                               \
         (i) = (i)->next)


/* Sadly, strdup is not portable. */
#define json_strdup(str) strdup(str)

    static void json_remove_from_parent(JsonNode *node);

    static JsonNode *json_find_element(JsonNode *array, int index);

    static JsonNode *json_find_member(JsonNode *object, const char *name);

    static JsonNode *json_find_member(JsonNode *object, const char *key, size_t keyLen);

    static JsonNode *json_first_child(const JsonNode *node);

    struct JsonNode {
        /* only if parent is an object or array (NULL otherwise) */
        JsonNode *parent;
        JsonNode *prev, *next;

        /* only if parent is an object (NULL otherwise) */
        char *key; /* Must be valid UTF-8. */

        Tag tag;
        union {
            /* JSON_BOOL */
            bool bool_;

            /* JSON_STRING */
            char *string_; /* Must be valid UTF-8. */

            /* JSON_NUMBER */
            double number_;

            /* JSON_ARRAY */
            /* JSON_OBJECT */
            struct {
                JsonNode *head, *tail;
            } children;
        };
    };

    /* String buffer */
    typedef struct {
        char *cur;
        char *end;
        char *start;
    } SB;

    static void sb_init(SB *sb) {
        sb->start = (char *) malloc(17);
        if (sb->start == nullptr)
            throw MemoryAllocationFailure("json::Object sb_init");
        sb->cur = sb->start;
        sb->end = sb->start + 16;
    }

/* sb and need may be evaluated multiple times. */
#define sb_need(sb, need) do {                  \
        if ((sb)->end - (sb)->cur < (need))     \
            sb_grow(sb, need);                  \
    } while (0)

    static void sb_grow(SB *sb, int need) {
        size_t length = sb->cur - sb->start;
        size_t alloc = sb->end - sb->start;

        do {
            alloc *= 2;
        } while (alloc < length + need);

        sb->start = (char *) realloc(sb->start, alloc + 1);
        if (sb->start == nullptr)
            out_of_memory();
        sb->cur = sb->start + length;
        sb->end = sb->start + alloc;
    }

    static char *sb_finish(SB *sb) {
        *sb->cur = 0;
        assert(sb->start <= sb->cur && strlen(sb->start) == (size_t) (sb->cur - sb->start));
        return sb->start;
    }

    static void sb_free(SB *sb) {
        free(sb->start);
    }

/*
 * Unicode helper functions
 *
 * These are taken from the ccan/charset module and customized a bit.
 * Putting them here means the compiler can (choose to) inline them,
 * and it keeps ccan/json from having a dependency.
 */

/*
 * Type for Unicode codepoints.
 * We need our own because wchar_t might be 16 bits.
 */
    typedef uint32_t uchar_t;

/*
 * Validate a single UTF-8 character starting at @s.
 * The string must be null-terminated.
 *
 * If it's valid, return its length (1 thru 4).
 * If it's invalid or clipped, return 0.
 *
 * This function implements the syntax given in RFC3629, which is
 * the same as that given in The Unicode Standard, Version 6.0.
 *
 * It has the following properties:
 *
 *  * All codepoints U+0000..U+10FFFF may be encoded,
 *    except for U+D800..U+DFFF, which are reserved
 *    for UTF-16 surrogate pair encoding.
 *  * UTF-8 byte sequences longer than 4 bytes are not permitted,
 *    as they exceed the range of Unicode.
 *  * The sixty-six Unicode "non-characters" are permitted
 *    (namely, U+FDD0..U+FDEF, U+xxFFFE, and U+xxFFFF).
 */
    static int utf8_validate_cz(const char *s) {
        unsigned char c = *s++;

        if (c <= 0x7F) {        /* 00..7F */
            return 1;
        } else if (c <= 0xC1) { /* 80..C1 */
            /* Disallow overlong 2-byte sequence. */
            return 0;
        } else if (c <= 0xDF) { /* C2..DF */
            /* Make sure subsequent byte is in the range 0x80..0xBF. */
            if ((0xC0u & (unsigned char) *s++) != 0x80)
                return 0;

            return 2;
        } else if (c <= 0xEF) { /* E0..EF */
            /* Disallow overlong 3-byte sequence. */
            if (c == 0xE0 && (unsigned char) *s < 0xA0)
                return 0;

            /* Disallow U+D800..U+DFFF. */
            if (c == 0xED && (unsigned char) *s > 0x9F)
                return 0;

            /* Make sure subsequent bytes are in the range 0x80..0xBF. */
            if (((unsigned char) *s++ & 0xC0u) != 0x80)
                return 0;
            if (((unsigned char) *s++ & 0xC0u) != 0x80)
                return 0;

            return 3;
        } else if (c <= 0xF4) { /* F0..F4 */
            /* Disallow overlong 4-byte sequence. */
            if (c == 0xF0 && (unsigned char) *s < 0x90)
                return 0;

            /* Disallow codepoints beyond U+10FFFF. */
            if (c == 0xF4 && (unsigned char) *s > 0x8F)
                return 0;

            /* Make sure subsequent bytes are in the range 0x80..0xBF. */
            if (((unsigned char) *s++ & 0xC0u) != 0x80)
                return 0;
            if (((unsigned char) *s++ & 0xC0u) != 0x80)
                return 0;
            if (((unsigned char) *s++ & 0xC0u) != 0x80)
                return 0;

            return 4;
        } else {                /* F5..FF */
            return 0;
        }
    }

/* Validate a null-terminated UTF-8 string. */
    static bool utf8_validate(const char *s) {
        int len;

        for (; *s != 0; s += len) {
            len = utf8_validate_cz(s);
            if (len == 0)
                return false;
        }

        return true;
    }

/*
 * Read a single UTF-8 character starting at @s,
 * returning the length, in bytes, of the character read.
 *
 * This function assumes input is valid UTF-8,
 * and that there are enough characters in front of @s.
 */
    static int utf8_read_char(const char *s, uchar_t *out) {
        const auto *c = (const unsigned char *) s;

        if (!utf8_validate_cz(s))
            throw InvalidArguments("string '", s, "' is not a valid utf8 string");

        if (c[0] <= 0x7F) {
            /* 00..7F */
            *out = c[0];
            return 1;
        } else if (c[0] <= 0xDF) {
            /* C2..DF (unless input is invalid) */
            *out = ((uchar_t) c[0] & 0x1Fu) << 6u |
                   ((uchar_t) c[1] & 0x3Fu);
            return 2;
        } else if (c[0] <= 0xEF) {
            /* E0..EF */
            *out = ((uchar_t) c[0] & 0xFu) << 12u |
                   ((uchar_t) c[1] & 0x3Fu) << 6u |
                   ((uchar_t) c[2] & 0x3Fu);
            return 3;
        } else {
            /* F0..F4 (unless input is invalid) */
            *out = ((uchar_t) c[0] & 0x7u) << 18u |
                   ((uchar_t) c[1] & 0x3Fu) << 12u |
                   ((uchar_t) c[2] & 0x3Fu) << 6u |
                   ((uchar_t) c[3] & 0x3Fu);
            return 4;
        }
    }

/*
 * Write a single UTF-8 character to @s,
 * returning the length, in bytes, of the character written.
 *
 * @unicode must be U+0000..U+10FFFF, but not U+D800..U+DFFF.
 *
 * This function will write up to 4 bytes to @out.
 */
    static int utf8_write_char(uchar_t unicode, char *out) {
        auto *o = (unsigned char *) out;

        if (!(unicode <= 0x10FFFF && !(unicode >= 0xD800 && unicode <= 0xDFFF)))
            throw IndexOutOfBounds("'", unicode, "' is not a valid unicode character");

        if (unicode <= 0x7F) {
            /* U+0000..U+007F */
            *o++ = unicode;
            return 1;
        } else if (unicode <= 0x7FF) {
            /* U+0080..U+07FF */
            *o++ = 0xC0u | unicode >> 6u;
            *o++ = 0x80u | (unicode & 0x3Fu);
            return 2;
        } else if (unicode <= 0xFFFF) {
            /* U+0800..U+FFFF */
            *o++ = 0xE0u | unicode >> 12u;
            *o++ = 0x80u | (unicode >> 6u & 0x3Fu);
            *o++ = 0x80u | (unicode & 0x3Fu);
            return 3;
        } else {
            /* U+10000..U+10FFFF */
            *o++ = 0xF0u | unicode >> 18u;
            *o++ = 0x80u | (unicode >> 12u & 0x3Fu);
            *o++ = 0x80u | (unicode >> 6u & 0x3Fu);
            *o++ = 0x80u | (unicode & 0x3Fu);
            return 4;
        }
    }

/*
 * Compute the Unicode codepoint of a UTF-16 surrogate pair.
 *
 * @uc should be 0xD800..0xDBFF, and @lc should be 0xDC00..0xDFFF.
 * If they aren't, this function returns false.
 */
    static bool from_surrogate_pair(uint16_t uc, uint16_t lc, uchar_t *unicode) {
        if (uc >= 0xD800 && uc <= 0xDBFF && lc >= 0xDC00 && lc <= 0xDFFF) {
            *unicode = 0x10000 + ((((uchar_t) uc & 0x3FFu) << 10u) | (lc & 0x3FFu));
            return true;
        } else {
            return false;
        }
    }

/*
 * Construct a UTF-16 surrogate pair given a Unicode codepoint.
 *
 * @unicode must be U+10000..U+10FFFF.
 */
    static void to_surrogate_pair(uchar_t unicode, uint16_t *uc, uint16_t *lc) {
        uchar_t n;

        if (!(unicode >= 0x10000 && unicode <= 0x10FFFF))
            throw IndexOutOfBounds("'", unicode, "' is not a valid unicode character");

        n = unicode - 0x10000;
        *uc = ((n >> 10u) & 0x3FFu) | 0xD800u;
        *lc = (n & 0x3FFu) | 0xDC00u;
    }

#define is_space(c) ((c) == '\t' || (c) == '\n' || (c) == '\r' || (c) == ' ')
#define is_digit(c) ((c) >= '0' && (c) <= '9')

    static bool parse_value(const char **sp, JsonNode **out);

    static bool parse_string(const char **sp, char **out);

    static bool parse_number(const char **sp, double *out);

    static bool parse_array(const char **sp, JsonNode **out);

    static bool parse_object(const char **sp, JsonNode **out);

    static bool parse_hex16(const char **sp, uint16_t *out);

    static bool expect_literal(const char **sp, const char *str);

    static void skip_space(const char **sp);

    static void emit_value(iod::encode_stream &out, const JsonNode *node);

    static void emit_value_indented(iod::encode_stream &out, const JsonNode *node, const char *space, int indent_level);

    static void emit_string(iod::encode_stream &out, const char *str);

    static void emit_number(iod::encode_stream &out, double num);

    static void emit_array(iod::encode_stream &out, const JsonNode *array);

    static void
    emit_array_indented(iod::encode_stream &out, const JsonNode *array, const char *space, int indent_level);

    static void emit_object(iod::encode_stream &out, const JsonNode *object);

    static void
    emit_object_indented(iod::encode_stream &out, const JsonNode *object, const char *space, int indent_level);

    static int write_hex16(iod::encode_stream &out, uint16_t val);

    static JsonNode *mknode(Tag tag);

    static void append_node(JsonNode *parent, JsonNode *child);

    static void prepend_node(JsonNode *parent, JsonNode *child);

    static void append_member(JsonNode *object, char *key, JsonNode *value);

/* Assertion-friendly validity checks */
    static bool tag_is_valid(Tag tag);

    static bool number_is_valid(const char *num);

    static void json_delete(JsonNode *node);

    static JsonNode *json_decode(const char *json) {
        const char *s = json;
        JsonNode *ret;

        skip_space(&s);
        if (!parse_value(&s, &ret))
            return nullptr;

        skip_space(&s);
        if (*s != 0) {
            json_delete(ret);
            return nullptr;
        }

        return ret;
    }

    static void json_delete(JsonNode *node) {
        if (node != nullptr) {
            json_remove_from_parent(node);

            switch (node->tag) {
                case Tag::JSON_STRING:
                    free(node->string_);
                    break;
                case Tag::JSON_ARRAY:
                case Tag::JSON_OBJECT: {
                    JsonNode *child, *next;
                    for (child = node->children.head; child != nullptr; child = next) {
                        next = child->next;
                        json_delete(child);
                    }
                    break;
                }
                default:;
            }
            free(node);
        }
    }

    bool json_validate(const char *json) {
        const char *s = json;

        skip_space(&s);
        if (!parse_value(&s, nullptr))
            return false;

        skip_space(&s);
        if (*s != 0)
            return false;

        return true;
    }

    JsonNode *json_find_element(JsonNode *array, int index) {
        JsonNode *element;
        int i = 0;

        if (array == nullptr || array->tag != Tag::JSON_ARRAY)
            return nullptr;

        json_foreach(element, array) {
            if (i == index)
                return element;
            i++;
        }

        return nullptr;
    }

    JsonNode *json_find_member(JsonNode *object, const char *name) {
        JsonNode *member;

        if (object == nullptr || object->tag != Tag::JSON_OBJECT)
            return nullptr;

        json_foreach(member, object)if (strcmp(member->key, name) == 0)
                return member;

        return nullptr;
    }

    JsonNode *json_find_member(JsonNode *object, const char *key, size_t keyLen) {
        JsonNode *member;

        if (object == nullptr || object->tag != Tag::JSON_OBJECT)
            return nullptr;

        json_foreach(member, object) {
            auto len = strlen(member->key);
            if ((len == keyLen) &&
                strncmp(member->key, key, keyLen) == 0) {
                return member;
            }
        }
        return nullptr;
    }

    JsonNode *json_first_child(const JsonNode *node) {
        if (node != nullptr && (node->tag == Tag::JSON_ARRAY || node->tag == Tag::JSON_OBJECT))
            return node->children.head;
        return nullptr;
    }

    static JsonNode *mknode(Tag tag) {
        JsonNode *ret = (JsonNode *) calloc(1, sizeof(JsonNode));
        if (ret == nullptr)
            out_of_memory();
        ret->tag = tag;
        return ret;
    }

    JsonNode *json_mknull(void) {
        return mknode(Tag::JSON_NULL);
    }

    JsonNode *json_mkbool(bool b) {
        JsonNode *ret = mknode(Tag::JSON_BOOL);
        ret->bool_ = b;
        return ret;
    }

    static JsonNode *mkstring(char *s) {
        JsonNode *ret = mknode(Tag::JSON_STRING);
        ret->string_ = s;
        return ret;
    }

    JsonNode *json_mkstring(const char *s) {
        return mkstring(json_strdup(s));
    }

    JsonNode *json_mknstring(const char *s, size_t size) {
        return mkstring(strndup(s, size));
    }

    JsonNode *json_mknumber(double n) {
        JsonNode *node = mknode(Tag::JSON_NUMBER);
        node->number_ = n;
        return node;
    }

    JsonNode *json_mkarray(void) {
        return mknode(Tag::JSON_ARRAY);
    }

    JsonNode *json_mkobject(void) {
        return mknode(Tag::JSON_OBJECT);
    }

    static void append_node(JsonNode *parent, JsonNode *child) {
        child->parent = parent;
        child->prev = parent->children.tail;
        child->next = nullptr;

        if (parent->children.tail != nullptr)
            parent->children.tail->next = child;
        else
            parent->children.head = child;
        parent->children.tail = child;
    }

    static void prepend_node(JsonNode *parent, JsonNode *child) {
        child->parent = parent;
        child->prev = nullptr;
        child->next = parent->children.head;

        if (parent->children.head != nullptr)
            parent->children.head->prev = child;
        else
            parent->children.tail = child;
        parent->children.head = child;
    }

    static void append_member(JsonNode *object, char *key, JsonNode *value) {
        value->key = key;
        append_node(object, value);
    }

    void json_append_element(JsonNode *array, JsonNode *element) {
        assert(array->tag == Tag::JSON_ARRAY);
        assert(element->parent == nullptr);

        append_node(array, element);
    }

    void json_prepend_element(JsonNode *array, JsonNode *element) {
        assert(array->tag == Tag::JSON_ARRAY);
        assert(element->parent == nullptr);

        prepend_node(array, element);
    }

    void json_append_member(JsonNode *object, const char *key, JsonNode *value) {
        assert(object->tag == Tag::JSON_OBJECT);
        assert(value->parent == nullptr);

        append_member(object, json_strdup(key), value);
    }

    void json_prepend_member(JsonNode *object, const char *key, JsonNode *value) {
        assert(object->tag == Tag::JSON_OBJECT);
        assert(value->parent == nullptr);

        value->key = json_strdup(key);
        prepend_node(object, value);
    }

    void json_remove_from_parent(JsonNode *node) {
        JsonNode *parent = node->parent;

        if (parent != nullptr) {
            if (node->prev != nullptr)
                node->prev->next = node->next;
            else
                parent->children.head = node->next;
            if (node->next != nullptr)
                node->next->prev = node->prev;
            else
                parent->children.tail = node->prev;

            free(node->key);

            node->parent = nullptr;
            node->prev = node->next = nullptr;
            node->key = nullptr;
        }
    }

    static bool parse_value(const char **sp, JsonNode **out) {
        const char *s = *sp;

        switch (*s) {
            case 'n':
                if (expect_literal(&s, "null")) {
                    if (out)
                        *out = json_mknull();
                    *sp = s;
                    return true;
                }
                return false;

            case 'f':
                if (expect_literal(&s, "false")) {
                    if (out)
                        *out = json_mkbool(false);
                    *sp = s;
                    return true;
                }
                return false;

            case 't':
                if (expect_literal(&s, "true")) {
                    if (out)
                        *out = json_mkbool(true);
                    *sp = s;
                    return true;
                }
                return false;

            case '"': {
                char *str;
                if (parse_string(&s, out ? &str : nullptr)) {
                    if (out)
                        *out = mkstring(str);
                    *sp = s;
                    return true;
                }
                return false;
            }

            case '[':
                if (parse_array(&s, out)) {
                    *sp = s;
                    return true;
                }
                return false;

            case '{':
                if (parse_object(&s, out)) {
                    *sp = s;
                    return true;
                }
                return false;

            default: {
                double num;
                if (parse_number(&s, out ? &num : nullptr)) {
                    if (out)
                        *out = json_mknumber(num);
                    *sp = s;
                    return true;
                }
                return false;
            }
        }
    }

    static bool parse_array(const char **sp, JsonNode **out) {
        const char *s = *sp;
        JsonNode *ret = out ? json_mkarray() : nullptr;
        JsonNode *element;

        if (*s++ != '[')
            goto failure;
        skip_space(&s);

        if (*s == ']') {
            s++;
            goto success;
        }

        for (;;) {
            if (!parse_value(&s, out ? &element : nullptr))
                goto failure;
            skip_space(&s);

            if (out)
                json_append_element(ret, element);

            if (*s == ']') {
                s++;
                goto success;
            }

            if (*s++ != ',')
                goto failure;
            skip_space(&s);
        }

        success:
        *sp = s;
        if (out)
            *out = ret;
        return true;

        failure:
        json_delete(ret);
        return false;
    }

    static bool parse_object(const char **sp, JsonNode **out) {
        const char *s = *sp;
        JsonNode *ret = out ? json_mkobject() : nullptr;
        char *key;
        JsonNode *value;

        if (*s++ != '{')
            goto failure;
        skip_space(&s);

        if (*s == '}') {
            s++;
            goto success;
        }

        for (;;) {
            if (!parse_string(&s, out ? &key : nullptr))
                goto failure;
            skip_space(&s);

            if (*s++ != ':')
                goto failure_free_key;
            skip_space(&s);

            if (!parse_value(&s, out ? &value : nullptr))
                goto failure_free_key;
            skip_space(&s);

            if (out)
                append_member(ret, key, value);

            if (*s == '}') {
                s++;
                goto success;
            }

            if (*s++ != ',')
                goto failure;
            skip_space(&s);
        }

        success:
        *sp = s;
        if (out)
            *out = ret;
        return true;

        failure_free_key:
        if (out)
            free(key);
        failure:
        json_delete(ret);
        return false;
    }

    bool parse_string(const char **sp, char **out) {
        const char *s = *sp;
        SB sb;
        char throwaway_buffer[4];
        /* enough space for a UTF-8 character */
        char *b;

        if (*s++ != '"')
            return false;

        if (out) {
            sb_init(&sb);
            sb_need(&sb, 4);
            b = sb.cur;
        } else {
            b = throwaway_buffer;
        }

        while (*s != '"') {
            unsigned char c = *s++;

            /* Parse next character, and write it to b. */
            if (c == '\\') {
                c = *s++;
                switch (c) {
                    case '"':
                    case '\\':
                    case '/':
                        *b++ = c;
                        break;
                    case 'b':
                        *b++ = '\b';
                        break;
                    case 'f':
                        *b++ = '\f';
                        break;
                    case 'n':
                        *b++ = '\n';
                        break;
                    case 'r':
                        *b++ = '\r';
                        break;
                    case 't':
                        *b++ = '\t';
                        break;
                    case 'u': {
                        uint16_t uc, lc;
                        uchar_t unicode;

                        if (!parse_hex16(&s, &uc))
                            goto failed;

                        if (uc >= 0xD800 && uc <= 0xDFFF) {
                            /* Handle UTF-16 surrogate pair. */
                            if (*s++ != '\\' || *s++ != 'u' || !parse_hex16(&s, &lc))
                                goto failed; /* Incomplete surrogate pair. */
                            if (!from_surrogate_pair(uc, lc, &unicode))
                                goto failed; /* Invalid surrogate pair. */
                        } else if (uc == 0) {
                            /* Disallow "\u0000". */
                            goto failed;
                        } else {
                            unicode = uc;
                        }

                        b += utf8_write_char(unicode, b);
                        break;
                    }
                    default:
                        /* Invalid escape */
                        goto failed;
                }
            } else if (c <= 0x1F) {
                /* Control characters are not allowed in string literals. */
                goto failed;
            } else {
                /* Validate and echo a UTF-8 character. */
                int len;

                s--;
                len = utf8_validate_cz(s);
                if (len == 0)
                    goto failed; /* Invalid UTF-8 character. */

                while (len--)
                    *b++ = *s++;
            }

            /*
             * Update sb to know about the new bytes,
             * and set up b to write another character.
             */
            if (out) {
                sb.cur = b;
                sb_need(&sb, 4);
                b = sb.cur;
            } else {
                b = throwaway_buffer;
            }
        }
        s++;

        if (out)
            *out = sb_finish(&sb);
        *sp = s;
        return true;

        failed:
        if (out)
            sb_free(&sb);
        return false;
    }

/*
 * The JSON spec says that a number shall follow this precise pattern
 * (spaces and quotes added for readability):
 *	 '-'? (0 | [1-9][0-9]*) ('.' [0-9]+)? ([Ee] [+-]? [0-9]+)?
 *
 * However, some JSON parsers are more liberal.  For instance, PHP accepts
 * '.5' and '1.'.  JSON.parse accepts '+3'.
 *
 * This function takes the strict approach.
 */
    bool parse_number(const char **sp, double *out) {
        const char *s = *sp;

        /* '-'? */
        if (*s == '-')
            s++;

        /* (0 | [1-9][0-9]*) */
        if (*s == '0') {
            s++;
        } else {
            if (!is_digit(*s))
                return false;
            do {
                s++;
            } while (is_digit(*s));
        }

        /* ('.' [0-9]+)? */
        if (*s == '.') {
            s++;
            if (!is_digit(*s))
                return false;
            do {
                s++;
            } while (is_digit(*s));
        }

        /* ([Ee] [+-]? [0-9]+)? */
        if (*s == 'E' || *s == 'e') {
            s++;
            if (*s == '+' || *s == '-')
                s++;
            if (!is_digit(*s))
                return false;
            do {
                s++;
            } while (is_digit(*s));
        }

        if (out)
            *out = strtod(*sp, nullptr);

        *sp = s;
        return true;
    }

    static void skip_space(const char **sp) {
        const char *s = *sp;
        while (is_space(*s))
            s++;
        *sp = s;
    }

    static void emit_value(iod::encode_stream &out, const JsonNode *node) {
        assert(tag_is_valid(node->tag));
        switch (node->tag) {
            case Tag::JSON_NULL:
                out << "null";
                break;
            case Tag::JSON_BOOL:
                out << (node->bool_ ? "true" : "false");
                break;
            case Tag::JSON_STRING:
                emit_string(out, node->string_);
                break;
            case Tag::JSON_NUMBER:
                emit_number(out, node->number_);
                break;
            case Tag::JSON_ARRAY:
                emit_array(out, node);
                break;
            case Tag::JSON_OBJECT:
                emit_object(out, node);
                break;
            default:
                assert(false);
        }
    }

    void emit_value_indented(iod::encode_stream &out, const JsonNode *node, const char *space, int indent_level) {
        if (!tag_is_valid(node->tag)) {
            throw InvalidArguments("Attempt to encode an invalid json node");
        }

        switch (node->tag) {
            case Tag::JSON_NULL:
                out << "null";
                break;
            case Tag::JSON_BOOL:
                out << (node->bool_ ? "true" : "false");
                break;
            case Tag::JSON_STRING:
                emit_string(out, node->string_);
                break;
            case Tag::JSON_NUMBER:
                emit_number(out, node->number_);
                break;
            case Tag::JSON_ARRAY:
                emit_array_indented(out, node, space, indent_level);
                break;
            case Tag::JSON_OBJECT:
                emit_object_indented(out, node, space, indent_level);
                break;
            default:
                assert(false);
        }
    }

    static void emit_array(iod::encode_stream &out, const JsonNode *array) {
        const JsonNode *element;

        out << '[';
        json_foreach(element, array) {
            emit_value(out, element);
            if (element->next != nullptr)
                out << ',';
        }
        out << ']';
    }

    static void
    emit_array_indented(iod::encode_stream &out, const JsonNode *array, const char *space, int indent_level) {
        const JsonNode *element = array->children.head;
        int i;

        if (element == nullptr) {
            out << "[]";
            return;
        }

        out << "[\n";
        while (element != nullptr) {
            for (i = 0; i < indent_level + 1; i++)
                out << space;
            emit_value_indented(out, element, space, indent_level + 1);

            element = element->next;
            out << (element != nullptr ? ",\n" : "\n");
        }
        for (i = 0; i < indent_level; i++)
            out << space;
        out << ']';
    }

    static void emit_object(iod::encode_stream &out, const JsonNode *object) {
        const JsonNode *member;

        out << '{';
        json_foreach(member, object) {
            emit_string(out, member->key);
            out << ':';
            emit_value(out, member);
            if (member->next != nullptr)
                out << ',';
        }
        out << '}';
    }

    static void
    emit_object_indented(iod::encode_stream &out, const JsonNode *object, const char *space, int indent_level) {
        const JsonNode *member = object->children.head;
        int i;

        if (member == nullptr) {
            out << "{}";
            return;
        }

        out << "{\n";
        while (member != nullptr) {
            for (i = 0; i < indent_level + 1; i++)
                out << space;
            emit_string(out, member->key);
            out << ": ";
            emit_value_indented(out, member, space, indent_level + 1);

            member = member->next;
            out << (member != nullptr ? ",\n" : "\n");
        }
        for (i = 0; i < indent_level; i++)
            out << space;
        out << '}';
    }

    void emit_string(iod::encode_stream &out, const char *str) {
        bool escape_unicode = false;
        const char *s = str;

        if (!utf8_validate(str))
            throw InvalidArguments("'", str, "' is not a valid utf8 string");

        out << '"';
        while (*s != 0) {
            unsigned char c = *s++;

            /* Encode the next character, and write it to b. */
            switch (c) {
                case '"':
                    out << '\\';
                    out << '"';
                    break;
                case '\\':
                    out << '\\';
                    out << '\\';
                    break;
                case '\b':
                    out << '\\';
                    out << 'b';
                    break;
                case '\f':
                    out << '\\';
                    out << 'f';
                    break;
                case '\n':
                    out << '\\';
                    out << 'n';
                    break;
                case '\r':
                    out << '\\';
                    out << 'r';
                    break;
                case '\t':
                    out << '\\';
                    out << 't';
                    break;
                default: {
                    int len;

                    s--;
                    len = utf8_validate_cz(s);

                    if (len == 0) {
                        /*
                         * Handle invalid UTF-8 character gracefully in production
                         * by writing a replacement character (U+FFFD)
                         * and skipping a single byte.
                         *
                         * This should never happen when assertions are enabled
                         * due to the assertion at the beginning of this function.
                         */
                        assert(false);
                        if (escape_unicode) {
                            out << "\\uFFFD";
                        } else {
                            out << 0xEF;
                            out << 0xBF;
                            out << 0xBD;
                        }
                        s++;
                    } else if (c < 0x1F || (c >= 0x80 && escape_unicode)) {
                        /* Encode using \u.... */
                        uint32_t unicode;

                        s += utf8_read_char(s, &unicode);

                        if (unicode <= 0xFFFF) {
                            out << '\\';
                            out << 'u';
                            write_hex16(out, unicode);
                        } else {
                            /* Produce a surrogate pair. */
                            uint16_t uc, lc;
                            if (unicode > 0x10FFFF)
                                throw InvalidArguments("'...", s, "' is not a valid unicode string");
                            to_surrogate_pair(unicode, &uc, &lc);
                            out << '\\';
                            out << 'u';
                            write_hex16(out, uc);
                            out << '\\';
                            out << 'u';
                            write_hex16(out, lc);
                        }
                    } else {
                        /* Write the character directly. */
                        while (len--)
                            out << *s++;
                    }

                    break;
                }
            }
        }
        out << '"';
    }

    static void emit_number(iod::encode_stream &out, double num) {
        /*
         * This isn't exactly how JavaScript renders numbers,
         * but it should produce valid JSON for reasonable numbers
         * preserve precision well enough, and avoid some oddities
         * like 0.3 -> 0.299999999999999988898 .
         */
        char buf[64];
        sprintf(buf, "%.16g", num);

        if (number_is_valid(buf))
            out << num;
        else
            out << "null";
    }

    static bool tag_is_valid(Tag tag) {
        return (/* tag >= JSON_NULL && */ tag <= Tag::JSON_OBJECT);
    }

    static bool number_is_valid(const char *num) {
        return (parse_number(&num, nullptr) && *num == '\0');
    }

    static bool expect_literal(const char **sp, const char *str) {
        const char *s = *sp;

        while (*str != '\0')
            if (*s++ != *str++)
                return false;

        *sp = s;
        return true;
    }

/*
 * Parses exactly 4 hex characters (capital or lowercase).
 * Fails if any input chars are not [0-9A-Fa-f].
 */
    static bool parse_hex16(const char **sp, uint16_t *out) {
        const char *s = *sp;
        uint16_t ret = 0;
        uint16_t i;
        uint16_t tmp;
        char c;

        for (i = 0; i < 4; i++) {
            c = *s++;
            if (c >= '0' && c <= '9')
                tmp = c - '0';
            else if (c >= 'A' && c <= 'F')
                tmp = c - 'A' + 10;
            else if (c >= 'a' && c <= 'f')
                tmp = c - 'a' + 10;
            else
                return false;

            ret <<= 4;
            ret += tmp;
        }

        if (out)
            *out = ret;
        *sp = s;
        return true;
    }

/*
 * Encodes a 16-bit number into hexadecimal,
 * writing exactly 4 hex chars.
 */
    static int write_hex16(iod::encode_stream &out, uint16_t val) {
        const char *hex = "0123456789ABCDEF";

        out.append(hex[(val >> 12) & 0xF]);
        out.append(hex[(val >> 8) & 0xF]);
        out.append(hex[(val >> 4) & 0xF]);
        out.append(hex[val & 0xF]);

        return 4;
    }

    bool json_check(const JsonNode *node, char errmsg[256]) {
#define problem(...) do { \
            if (errmsg != nullptr) \
                snprintf(errmsg, 256, __VA_ARGS__); \
            return false; \
        } while (0)

        if (node->key != nullptr && !utf8_validate(node->key))
            problem("key contains invalid UTF-8");

        if (!tag_is_valid(node->tag))
            problem("tag is invalid (%u)", (unsigned int)node->tag);

        if (node->tag == Tag::JSON_STRING) {
            if (node->string_ == nullptr)
                problem("string_ is nullptr");
            if (!utf8_validate(node->string_))
                problem("string_ contains invalid UTF-8");
        } else if (node->tag == Tag::JSON_ARRAY || node->tag == Tag::JSON_OBJECT) {
            JsonNode *head = node->children.head;
            JsonNode *tail = node->children.tail;

            if (head == nullptr || tail == nullptr) {
                if (head != nullptr)
                    problem("tail is nullptr, but head is not");
                if (tail != nullptr)
                    problem("head is nullptr, but tail is not");
            } else {
                JsonNode *child;
                JsonNode *last = nullptr;

                if (head->prev != nullptr)
                    problem("First child's prev pointer is not nullptr");

                for (child = head; child != nullptr; last = child, child = child->next) {
                    if (child == node)
                        problem("node is its own child");
                    if (child->next == child)
                        problem("child->next == child (cycle)");
                    if (child->next == head)
                        problem("child->next == head (cycle)");

                    if (child->parent != node)
                        problem("child does not point back to parent");
                    if (child->next != nullptr && child->next->prev != child)
                        problem("child->next does not point back to child");

                    if (node->tag == Tag::JSON_ARRAY && child->key != nullptr)
                        problem("Array element's key is not nullptr");
                    if (node->tag == Tag::JSON_OBJECT && child->key == nullptr)
                        problem("Object member's key is nullptr");

                    if (!json_check(child, errmsg))
                        return false;
                }

                if (last != tail)
                    problem("tail does not match pointer found by starting at head and following next links");
            }
        }
        return true;
#undef problem
    }
}

#pragma endregion

namespace suil::json {

    Object::Object()
            : mNode(mknode(Tag::JSON_NULL))
    {}

    Object::Object(bool b)
            : mNode(json_mkbool(b))
    {}

    Object::Object(double d)
            : mNode(json_mknumber(d))
    {}

    Object::Object(const char *str)
            : mNode(json_mkstring(str))
    {}

    Object::Object(const suil::String &str)
            : mNode(json_mknstring(str.data(), str.size()))
    {}

    Object::Object(const std::string &str)
            : mNode(json_mknstring(str.data(), str.size()))
    {}

    Object::Object(suil::json::Array_t)
            : mNode(json_mkarray())
    {}

    Object::Object(suil::json::Object_t)
            : mNode(json_mkobject())
    {}

    void Object::push(suil::json::Object &&o) {
        if (mNode == nullptr || mNode->tag != Tag::JSON_ARRAY)
            throw UnsupportedOperation("json::Object::push - object is not a JSON array");
        json_append_element(mNode, o.mNode);
        o.ref = true;
    }

    void Object::set(const char *key, suil::json::Object &&o) {
        if (mNode == nullptr || mNode->tag != Tag::JSON_OBJECT)
            throw UnsupportedOperation("json::Object::set - object is not a JSON object");
        json_append_member(mNode, key, o.mNode);
        o.ref = true;
    }

    Object Object::operator[](int index) const
    {
        return Ego(index, true);
    }

    Object Object::operator()(int index, bool throwNotFound) const
    {
        if (mNode == nullptr || mNode->tag != Tag::JSON_ARRAY) {
            if (throwNotFound) {
                throw UnsupportedOperation("json::Object::[index] - object is not a JSON array");
            }
            return Object{nullptr};
        }
        else {
            Object obj(json_find_element(mNode, index));
            if (obj.mNode == nullptr and throwNotFound) {
                throw IndexOutOfBounds("Index '", index, "' is out of  bounds");
            }
            return obj;
        }
    }

    Object Object::operator()(const char *key, bool throwNotFound) const {
        const char *s = key, *p = key;
        Object obj{mNode};
        while ((p = strchr(s, '.')) != nullptr) {
            String part{s, (size_t)(p-s), false};
            obj = obj.get(part, throwNotFound);
            if (obj.empty() || !obj.isObject()) {
                if (throwNotFound)
                    throw UnsupportedOperation("Key '", key, "'is not an object");
                else
                    return Object{nullptr};
            }
            s = p+1;
        }
        String tmp{s};
        if (tmp == "*") {
            return std::move(obj);
        }

        return obj.get(s, throwNotFound);
    }

    Object Object::operator[](const suil::String &key) const {
        return Ego.get(key, true);
    }

    Object Object::operator[](const char* key) const {
        return Ego.get(String{key}, true);
    }

    Object Object::get(const suil::String &key, bool shouldThrow) const {
        if (mNode == nullptr || mNode->tag != Tag::JSON_OBJECT) {
            if (shouldThrow)
                throw UnsupportedOperation("json::Object::[", key, "] - object is not a JSON object");
            else
                return Object{nullptr};
        }

        Object obj(json_find_member(mNode, key.data(), key.size()));
        return obj;
    }

    Object::operator bool()   const {
        return !Ego.empty();
    }

    Object::operator double()   const {
        if (mNode == nullptr || mNode->tag != Tag::JSON_NUMBER)
            throw UnsupportedOperation("json::Object - object is not a number");
        return mNode->number_;
    }

    Object::operator const char*()   const {
        if (mNode == nullptr || mNode->tag != Tag::JSON_STRING)
            throw UnsupportedOperation("json::Object - object is not a string");
        return mNode->string_;
    }

    bool Object::empty() const {
        if (mNode == nullptr || mNode->tag == Tag::JSON_NULL) return true;
        if (mNode->tag == Tag::JSON_STRING) {
            if (mNode->string_) {
                return strlen(mNode->string_) == 0;
            } else {
                return true;
            }
        }
        if (mNode->tag == Tag::JSON_BOOL)
            return !mNode->bool_;
        if (mNode->tag == Tag::JSON_OBJECT || mNode->tag == Tag::JSON_ARRAY)
            return mNode->children.head == nullptr;

        return false;
    }

    bool Object::isNull() const {
        return mNode == nullptr || mNode->tag == Tag::JSON_NULL;
    }

    bool Object::isBool() const {
        return mNode != nullptr && mNode->tag == Tag::JSON_BOOL;
    }

    bool Object::isNumber() const {
        return mNode != nullptr && mNode->tag == Tag::JSON_NUMBER;
    }

    bool Object::isString() const {
        return mNode != nullptr && mNode->tag == Tag::JSON_STRING;
    }

    bool Object::isArray() const {
        return mNode != nullptr && mNode->tag == Tag::JSON_ARRAY;
    }

    bool Object::isObject() const {
        return mNode != nullptr && mNode->tag == Tag::JSON_OBJECT;
    }

    Tag Object::type() const { return mNode ? mNode->tag : Tag::JSON_NULL; }

    Object Object::push(const suil::json::Array_t &) {
        if (mNode == nullptr || mNode->tag != Tag::JSON_ARRAY)
            /* valid node */
            throw UnsupportedOperation("json::Object::push - object is not a JSON array");
        /* create array node and append it */
        Object o(json::Arr);
        json_append_element(mNode, o.mNode);
        o.ref = true;
        return o;
    }

    Object Object::push(const suil::json::Object_t&) {
        if (mNode == nullptr || mNode->tag != Tag::JSON_ARRAY)
            /* valid node */
            throw UnsupportedOperation("json::Object::push - object is not a JSON array");

        /* create array node and append it */
        Object o(json::Obj);
        json_append_element(mNode, o.mNode);
        o.ref = true;
        return o;
    }

    Object Object::set(const char *key, const suil::json::Object_t &) {
        if (mNode == nullptr || mNode->tag != Tag::JSON_OBJECT)
            /* valid node */
            throw UnsupportedOperation("json::Object::set - object is not a JSON object");
        /* create array node and append it */
        Object o(json::Obj);
        json_append_member(mNode, key, o.mNode);
        o.ref = true;
        return o;
    }

    Object Object::set(const char *key, const suil::json::Array_t &) {
        if (mNode == nullptr || mNode->tag != Tag::JSON_OBJECT)
            /* valid node */
            throw UnsupportedOperation("json::Object::set - object is not a JSON object");
        /* create array node and append it */
        Object o(json::Arr);
        json_append_member(mNode, key, o.mNode);
        o.ref = true;
        return o;
    }

    void Object::encode(iod::encode_stream &ss) const {
        /* encode json object */
        emit_value(ss, mNode);
    }

    Object Object::decodeStr(const char *str, size_t& sz) {
        JsonNode *ret;
        const char *s = str;
        skip_space(&s);
        if (!parse_value(&s, &ret)) {
            /* parsing json string failed */
            throw UnsupportedOperation("json::Object::decode invalid json string at ", (s-str));
        }

        skip_space(&s);
        sz = s-str;
        return Object(ret, false);
    }

    void Object::operator|(suil::json::Object::ArrayEnumerator f) const {
        if (mNode == nullptr || mNode->tag != Tag::JSON_ARRAY)
            /* valid node */
            throw UnsupportedOperation("json::Object::enumerate - object is not a JSON array");
        JsonNode *node{nullptr};
        json_foreach(node, mNode) {
            if (f(Object(node, true)))
                break;
        }
    }

    void Object::operator|(suil::json::Object::ObjectEnumerator f) const {
        if (mNode == nullptr || mNode->tag != Tag::JSON_OBJECT)
            /* valid node */
            throw UnsupportedOperation("json::Object::enumerate - object is not a JSON object");

        JsonNode *node{nullptr};
        json_foreach(node, mNode) {
            if (f(node->key, Object(node, true)))
                break;
        }
    }

    bool Object::operator==(const Object &other) const {
        if (other.type() != Ego.type()) return false;
        if (other.empty() && Ego.empty()) return true;
        switch (other.type()) {
            case Tag::JSON_NUMBER:
                return other.mNode->number_ == Ego.mNode->number_;
            case Tag::JSON_BOOL:
                return other.mNode->bool_ == Ego.mNode->bool_;
            case Tag::JSON_STRING:
                return String{other.mNode->string_} == String{Ego.mNode->string_};
            case Tag::JSON_ARRAY: {
                auto ai = Ego.begin();
                auto bi = other.end();
                while (ai != Ego.end()) {
                    if (bi == other.end()) return  false;
                    if (!((*ai).second == (*bi).second)) return false;
                    ++ai;
                    ++bi;
                }
                return bi == other.end();
                break;
            }
            case Tag::JSON_OBJECT: {
                for (auto [key, obj] : Ego) {
                    auto tmp = other[String{key}];
                    if (tmp.isNull()) return false;
                    if (!(obj == tmp)) return false;
                }
                return true;
                break;
            }
            default:
                return false;

        }
    }

    Object::iterator Object::iterator::operator++() {
        if (mNode)
            mNode = mNode->next;
        return Ego;
    }

    const std::pair<const char*,Object> Object::iterator::operator*() const {
        return std::make_pair(mNode? mNode->key:nullptr, Object(mNode));
    }

    Object::iterator Object::begin() {
        if (Ego.isArray() || Ego.isObject())
            return iterator(mNode->children.head);
        return end();
    }

    Object::const_iterator Object::begin() const {
        if (Ego.isArray() || Ego.isObject())
            return const_iterator(mNode->children.head);
        return end();
    }

    Object& Object::operator=(suil::json::Object &&o) noexcept {
        if (this != &o) {
            if (mNode && !ref)
                json_delete(mNode);
            mNode = o.mNode;
            ref = o.ref;
            o.mNode = nullptr;
        }
        return Ego;
    }

    Object Object::fromLuaFile(const suil::String &file) {

        if (fs::exists(file())) {
            try {
                auto str = fs::readall(file(), true);
                return Object::fromLuaString(str);
            }
            catch (...) {
                serror("failed to load lua '%s' to json: %s", file(), Exception::fromCurrent().what());
            }
        }
        return Object{nullptr};
    }

    static Object parseLuaTable(lua_State* L);

    static Object parseLuaValue(lua_State* L) {
        Object obj{nullptr};
        auto type = lua_type(L, -1);

        switch (type) {
            case LUA_TBOOLEAN:
                obj = Object(lua_toboolean(L, -1) != 0);
                break;
            case LUA_TNUMBER:
                obj = Object(lua_tonumber(L, -1));
                break;
            case LUA_TSTRING:
                obj = Object(lua_tostring(L, -1));
                break;
            case LUA_TTABLE:
                obj = parseLuaTable(L);
                break;
            default:
                throw InvalidArguments("type '", luaL_typename(L, -1), "' is not supported");
        }

        return std::move(obj);
    }

    Object parseLuaTable(lua_State* L) {
        Object obj(Obj);

        lua_pushnil(L);
        bool first{true}, isArray{false};
        while (lua_next(L, -2) != 0) {
            if (first) {
                if (lua_type(L, -2) == LUA_TNUMBER) {
                    isArray = true;
                    obj = Object(Arr);
                }
                first = false;
            }
            if (isArray) {
                if (lua_type(L, -2) != LUA_TNUMBER) {
                    throw InvalidArguments("Invalid config file, array indexes can only be numbers");
                }
                obj.push(parseLuaValue(L));
            }
            else {
                if (lua_type(L, -2) != LUA_TSTRING) {
                    throw InvalidArguments("Invalid config file, array indexes can only be numbers");
                }
                auto key = lua_tostring(L, -2);
                obj.set(key, parseLuaValue(L));
            }
            lua_pop(L, 1);
        }

        return  std::move(obj);
    }

    Object Object::fromLuaTable(lua_State *L, int index) {
        int top = lua_gettop(L);
        if (index && top != index) // bring value to top of stack
            lua_pushvalue(L, index);

        Object obj{nullptr};
        auto _pop = [L,&top]() {
            auto diff = lua_gettop(L) - top;
            if (diff > 0) {
                lua_pop(L, diff);
            }
        };

        try {
            obj = parseLuaValue(L);
            _pop();
        }
        catch (...) {
            _pop();
            throw;
        }

        return std::move(obj);
    }

    Object Object::fromLuaString(const suil::String &script) {
        lua_State *L{luaL_newstate()};
        luaL_openlibs(L);
        defer({
            if (L != nullptr)
                lua_close(L);
        });

        if (luaL_loadstring(L, script()) || lua_pcall(L, 0, LUA_MULTRET, 0)) {
            // loading configuration file failed
            throw Exception(lua_tostring(L, -1));
        }
        lua_getglobal(L, "app");
        if (lua_type(L, -1) != LUA_TTABLE) {
            // config invalid
            throw InvalidArguments("Configuration must be within an 'app' tag");
        }

        auto obj = parseLuaTable(L);

        return std::move(obj);
    }

    Object::~Object() {
        if (mNode && !ref)
            json_delete(mNode);
        mNode = nullptr;
    }
}

#ifdef SUIL_UNITTEST
#include <catch2/catch.hpp>
#include "../test/test.hpp"

using namespace suil;

struct Mt : iod::MetaType
{
    typedef decltype(iod::D(
            test:: prop(a,     int),
            test:: prop(b,     String)
    )) Schema;
    static Schema Meta;

    int    a;
    String b;

    void toJson(iod::json::jstream& ss) const
    {
        ss << '{';
        int i = 0;
        bool first = true;
        foreach(Mt::Meta) | [&](auto m) {
            if (!m.attributes().has(iod::_json_skip)) {
                /* ignore empty entry */
                auto val = m.value();
                if (m.attributes().has(iod::_ignore) && iod::json::json_ignore<decltype(val)>(val)) return;

                if (!first) { ss << ','; }
                first = false;
                iod::json::json_encode_symbol(m.attributes().get(iod::_json_key, m.symbol()), ss);
                ss << ':';
                iod::json::json_encode_(m.symbol().member_access(Ego), ss);
            }
            i++;
        };
        ss << '}';
    }

    static Mt fromJson(iod::json::parser& p)
    {
        Mt tmp;
        iod::json::iod_attr_from_json(&Mt::Meta, tmp, p);
        return std::move(tmp);
    }
};

Mt::Schema Mt::Meta{};

TEST_CASE("suil::json::Object", "[json][Object]")
{
    SECTION("Constructing a JSON Object") {
        // test constucting a json::Object
        json::Object nl{};
        REQUIRE(nl.mNode->tag == json::Tag::JSON_NULL);
        REQUIRE_FALSE(nl.ref);

        json::Object num((uint8_t) 6);
        REQUIRE(num.mNode->tag == json::Tag::JSON_NUMBER);
        REQUIRE_FALSE(num.ref);
        REQUIRE(6 == num.mNode->number_);

        json::Object num2((int) 12);
        REQUIRE(num2.mNode->tag == json::Tag::JSON_NUMBER);
        REQUIRE_FALSE(num2.ref);
        REQUIRE(12 == num2.mNode->number_);

        json::Object num3(33.89);
        REQUIRE(num3.mNode->tag == json::Tag::JSON_NUMBER);
        REQUIRE_FALSE(num3.ref);
        REQUIRE(33.89 == num3.mNode->number_);

        json::Object b(true);
        REQUIRE(b.mNode->tag == json::Tag::JSON_BOOL);
        REQUIRE_FALSE(b.ref);
        REQUIRE(b.mNode->bool_);

        json::Object s("Hello World");
        REQUIRE(s.mNode->tag == json::Tag::JSON_STRING);
        REQUIRE_FALSE(s.ref);
        REQUIRE(strcmp("Hello World", s.mNode->string_) == 0);

        json::Object s2("");
        REQUIRE(s2.mNode->tag == json::Tag::JSON_STRING);
        REQUIRE_FALSE(s2.ref);
        REQUIRE(strlen(s2.mNode->string_) == 0);

        json::Object arr(json::Arr);
        REQUIRE(arr.mNode->tag == json::Tag::JSON_ARRAY);
        REQUIRE_FALSE(arr.ref);
        REQUIRE(arr.mNode->children.head == nullptr);

        json::Object arr2(json::Arr, 1, 2, "Carter", true, json::Object(json::Arr, "Hello", 4, "Worlds"));
        REQUIRE(arr2.mNode->tag == json::Tag::JSON_ARRAY);
        REQUIRE_FALSE(arr2.ref);
        REQUIRE_FALSE(arr2.mNode->children.head == nullptr);

        json::Object arr3(json::Arr, std::vector<int>{1, 2, 4});
        REQUIRE(arr3.mNode->tag == json::Tag::JSON_ARRAY);
        REQUIRE_FALSE(arr3.ref);
        REQUIRE_FALSE(arr3.mNode->children.head == nullptr);

        json::Object obj(json::Obj);
        REQUIRE(obj.mNode->tag == json::Tag::JSON_OBJECT);
        REQUIRE_FALSE(obj.ref);
        REQUIRE(obj.mNode->children.head == nullptr);

        json::Object obj2(json::Obj, "name", "Carter", "age", 29);
        REQUIRE(obj2.mNode->tag == json::Tag::JSON_OBJECT);
        REQUIRE_FALSE(obj2.ref);
        REQUIRE_FALSE(obj2.mNode->children.head == nullptr);
    }

    SECTION("assigning/moving json objects") {
        /* test assignment operators and moving object */
        json::Object j1;
        REQUIRE((j1.mNode != nullptr && j1.mNode->tag == json::Tag::JSON_NULL));
        auto *node = j1.mNode;

        WHEN("implicit assigning to Object") {
            /* assigning to object with implicit cast */
            j1 = json::Object{68.63};
            REQUIRE_FALSE(j1.mNode == nullptr);
            REQUIRE_FALSE(j1.mNode == node);
            REQUIRE(j1.mNode->tag == json::Tag::JSON_NUMBER);
            REQUIRE(j1.mNode->number_ == 68.63);
            node = j1.mNode;

            j1 = json::Object{69};
            REQUIRE_FALSE(j1.mNode == nullptr);
            REQUIRE_FALSE(j1.mNode == node);
            REQUIRE(j1.mNode->tag == json::Tag::JSON_NUMBER);
            REQUIRE(j1.mNode->number_ == 69);
            node = j1.mNode;

            j1 = json::Object{"Worlds of Great"};
            REQUIRE_FALSE(j1.mNode == nullptr);
            REQUIRE_FALSE(j1.mNode == node);
            REQUIRE(j1.mNode->tag == json::Tag::JSON_STRING);
            REQUIRE(strcmp(j1.mNode->string_, "Worlds of Great") == 0);
            node = j1.mNode;

            j1 = json::Object{String{"Hello World"}};
            REQUIRE_FALSE(j1.mNode == nullptr);
            REQUIRE_FALSE(j1.mNode == node);
            REQUIRE(j1.mNode->tag == json::Tag::JSON_STRING);
            REQUIRE(strcmp(j1.mNode->string_, "Hello World") == 0);
            node = j1.mNode;

            j1 = json::Object(json::Arr, 1, 4);
            REQUIRE_FALSE(j1.mNode == nullptr);
            REQUIRE_FALSE(j1.mNode == node);
            REQUIRE(j1.mNode->tag == json::Tag::JSON_ARRAY);
            REQUIRE_FALSE(j1.mNode->children.head == nullptr);
            node = j1.mNode;

            j1 = json::Object(json::Arr);
            REQUIRE_FALSE(j1.mNode == nullptr);
            REQUIRE_FALSE(j1.mNode == node);
            REQUIRE(j1.mNode->tag == json::Tag::JSON_ARRAY);
            REQUIRE(j1.mNode->children.head == nullptr);
            node = j1.mNode;

            j1 = json::Object(json::Obj, "One", 1);
            REQUIRE_FALSE(j1.mNode == nullptr);
            REQUIRE_FALSE(j1.mNode == node);
            REQUIRE(j1.mNode->tag == json::Tag::JSON_OBJECT);
            REQUIRE_FALSE(j1.mNode->children.head == nullptr);
        }

        WHEN("copying or moving JSON object") {
            /* test copying or moving json Object
             * !!!NOTE!!! always move, do not copy
             */
            j1 = json::Object(9);   // move assignment
            REQUIRE(j1.mNode->tag == json::Tag::JSON_NUMBER);
            REQUIRE(j1.mNode->number_ == 9);
            REQUIRE_FALSE(j1.ref);

            json::Object j2(std::move(j1)); // move constructor
            REQUIRE(j2.mNode->tag == json::Tag::JSON_NUMBER);
            REQUIRE(j2.mNode->number_ == 9);
            REQUIRE_FALSE(j2.ref);
            REQUIRE(j1.mNode == nullptr);

            json::Object j3(j2);    // copy constructor
            REQUIRE(j3.mNode->tag == json::Tag::JSON_NUMBER);
            REQUIRE(j3.mNode->number_ == 9);
            REQUIRE(j3.ref);
            REQUIRE_FALSE(j2.mNode == nullptr);
            REQUIRE_FALSE(j2.ref);  // after copy, j2 still owns the object, j3 should be a weak copy

            json::Object j4 = std::move(j2);    // move assignment
            REQUIRE(j4.mNode->tag == json::Tag::JSON_NUMBER);
            REQUIRE(j4.mNode->number_ == 9);
            REQUIRE_FALSE(j4.ref);
            REQUIRE(j2.mNode == nullptr); // nove j2 has been moved
            REQUIRE(j3.mNode == j4.mNode); // although j3 is still a valid reference
            REQUIRE(j3.ref);
        }
    }

    SECTION("casting to native values") {
        /* test reading native values */
        json::Object j1(true);
        REQUIRE((bool) j1);
        j1 = json::Object{false};
        REQUIRE_FALSE((bool) j1);
        REQUIRE_THROWS((String) j1);
        REQUIRE_THROWS((int) j1);

        j1 = json::Object{4};
        REQUIRE(4 == (int) j1);
        j1 = json::Object{67.09};
        REQUIRE(67.09f == (float) j1);
        REQUIRE_THROWS((const char*) j1);
        float f = j1;
        REQUIRE(f == 67.09f);

        j1 = json::Object{"Cali"};
        REQUIRE(strcmp("Cali", (const char *)j1) == 0);
        REQUIRE(String{"Cali"} == (String) j1);
        REQUIRE("Cali" == (std::string) j1);
        REQUIRE_THROWS((int) j1);
    }

    SECTION("adding values to arrays/objects") {
        /* test adding values to arrays or objects */
        WHEN("adding values to an array") {
            /* initializing and adding values to arrays */
            json::JsonNode *node{nullptr};
            json::Object arr(json::Arr);
            arr.push(1);
            node = json_first_child(arr.mNode);
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_NUMBER);
            REQUIRE(node->number_ == 1);

            arr.push(true, 67.9, "Cali", json::Object(json::Arr), json::Object(json::Obj));
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_BOOL);
            REQUIRE(node->bool_);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_NUMBER);
            REQUIRE(node->number_ == 67.9);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_STRING);
            REQUIRE(strcmp(node->string_, "Cali") == 0);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_ARRAY);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_OBJECT);
            REQUIRE(node->next == nullptr);
            arr = json::Object(json::Arr, 1, json::Object(json::Arr, 10, true));
            node = json_first_child(arr.mNode);
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_NUMBER);
            REQUIRE(node->number_ == 1);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_ARRAY);
            node = json_first_child(node);
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_NUMBER);
            REQUIRE(node->number_ == 10);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_BOOL);
            REQUIRE(node->bool_);

            /* can only push into arrays*/
            arr = json::Object();
            REQUIRE_THROWS(arr.push(6));
            arr = json::Object(json::Obj);
            REQUIRE_THROWS(arr.push(6));
            arr = json::Object(8);
            REQUIRE_THROWS(arr.push(6));
            arr = json::Object("Cali");
            REQUIRE_THROWS(arr.push(6));
        }

        WHEN("adding values to an object") {
            /* test adding values to an object */
            json::JsonNode *node{nullptr};
            json::Object obj(json::Obj);
            obj.set("one", 1);
            node = json_first_child(obj.mNode);
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_NUMBER);
            REQUIRE(strcmp(node->key, "one") == 0);
            REQUIRE(node->number_ == 1);
            obj.set("two", 2, "bool", true, "str", "Cali", "arr", json::Object(json::Arr), "obj", json::Object(json::Obj));
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_NUMBER);
            REQUIRE(strcmp(node->key, "two") == 0);
            REQUIRE(node->number_ == 2);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_BOOL);
            REQUIRE(strcmp(node->key, "bool") == 0);
            REQUIRE(node->bool_);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_STRING);
            REQUIRE(strcmp(node->key, "str") == 0);
            REQUIRE(strcmp(node->string_, "Cali") == 0);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_ARRAY);
            REQUIRE(strcmp(node->key, "arr") == 0);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_OBJECT);
            REQUIRE(strcmp(node->key, "obj") == 0);
            REQUIRE(node->next == nullptr);
            obj = json::Object(json::Obj, "one", 1, "bool", true, "obj", json::Object(json::Obj, "two", 2, "str", "Cali"));
            node = json_first_child(obj.mNode);
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_NUMBER);
            REQUIRE(strcmp(node->key, "one") == 0);
            REQUIRE(node->number_ == 1);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_BOOL);
            REQUIRE(strcmp(node->key, "bool") == 0);
            REQUIRE(node->bool_);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_OBJECT);
            REQUIRE(strcmp(node->key, "obj") == 0);
            node = json_first_child(node);
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_NUMBER);
            REQUIRE(strcmp(node->key, "two") == 0);
            REQUIRE(node->number_ == 2);
            node = node->next;
            REQUIRE_FALSE(node == nullptr);
            REQUIRE(node->tag == json::Tag::JSON_STRING);
            REQUIRE(strcmp(node->key, "str") == 0);
            REQUIRE(strcmp(node->string_, "Cali") == 0);
            REQUIRE(node->next == nullptr);

            obj = json::Object();
            REQUIRE_THROWS(obj.push(5)); // cannot insert into a json object
            obj = json::Object(json::Arr);
            REQUIRE_THROWS(obj.set("five", 5));
            obj = json::Object(6);
            REQUIRE_THROWS(obj.set("five", 5));
            obj = json::Object("Cali");
            REQUIRE_THROWS(obj.set("five", 5));
        }
    }

    SECTION("index access operators on arrays/objects") {
        /* tests the index operators on objects and array*/
        WHEN("accessing array elements by index") {
            /* array JSON objects support the [int] operator*/
            json::Object arr(json::Arr, 1, true, "Cali", json::Object(json::Arr, 2, false, "Cali"));
            REQUIRE(arr(-1).mNode == nullptr);
            REQUIRE(arr(4).mNode == nullptr);

            auto tmp = arr[0];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(1 == (int) tmp);
            tmp = arr[1];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE((bool) tmp);
            tmp = arr[2];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(strcmp(tmp.mNode->string_, "Cali") == 0);
            auto arr1 = arr[3];
            REQUIRE_FALSE(arr1.mNode == nullptr);
            REQUIRE(arr1.ref);
            REQUIRE(arr1.mNode->tag == json::Tag::JSON_ARRAY);

            tmp = arr1[0];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(2 == (int) tmp);
            tmp = arr1[1];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE_FALSE((bool) tmp);
            tmp = arr1[2];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(strcmp(tmp.mNode->string_, "Cali") == 0);
            /* array of arrays works like magic */
            REQUIRE(arr[3][1].mNode == arr1[1].mNode);

            /* index operator is only supported on arrays*/
            arr = json::Object();
            REQUIRE_THROWS(arr[0]);
            arr = json::Object(true);
            REQUIRE_THROWS(arr[0]);
            arr = json::Object("Cali");
            REQUIRE_THROWS(arr[0]);
            arr = json::Object(json::Obj);
            REQUIRE_THROWS(arr[0]);
        }

        WHEN("accessing object elements by index") {
            /* objects elements can be accessed via string index */
            json::Object obj(json::Obj, "one", 1, "bool", true, "str", "Cali", "obj",
                             json::Object(json::Obj, "two", 2, "bool", false, "str", "Cali"));
            REQUIRE(obj["no_key"].mNode == nullptr);
            auto tmp = obj["one"];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(1 == (int) tmp);
            tmp = obj["bool"];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE((bool) tmp);
            tmp = obj["str"];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(strcmp(tmp.mNode->string_, "Cali") == 0);
            auto obj1 = obj["obj"];
            REQUIRE_FALSE(obj1.mNode == nullptr);
            REQUIRE(obj1.ref);
            REQUIRE(obj1.mNode->tag == json::Tag::JSON_OBJECT);

            tmp = obj1["two"];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(2 == (int) tmp);
            tmp = obj1["bool"];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE_FALSE((bool) tmp);
            tmp = obj1["str"];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(strcmp(tmp.mNode->string_, "Cali") == 0);
            /* object of objects works like magic */
            REQUIRE(obj["obj"]["two"].mNode == obj1["two"].mNode);

            /* index operator is only supported on object */
            obj = json::Object();
            REQUIRE_THROWS(obj["null"]);
            obj = json::Object(true);
            REQUIRE_THROWS(obj["bool"]);
            obj = json::Object("Cali");
            REQUIRE_THROWS(obj["str"]);
            obj = json::Object(json::Arr);
            REQUIRE_THROWS(obj["arr"]);
        }

        WHEN("Using fullpath lookup") {
            json::Object obj(json::Obj, "one", 1, "bool", true, "str", "cali", "obj1",
                             json::Object(json::Obj, "two", 2, "bool", false, "str", "carter", "obj2",
                                          json::Object(json::Obj, "three", 3)));
            auto tmp = obj("obj1", true);
            REQUIRE_FALSE(tmp.empty());
            REQUIRE(tmp.isObject());
            REQUIRE_NOTHROW(tmp = obj("*", true));
            REQUIRE(tmp.isObject());
            REQUIRE(tmp.mNode == obj.mNode);
            REQUIRE_NOTHROW(tmp = obj("obj1.two", true));
            REQUIRE(tmp.isNumber());
            REQUIRE((int)tmp == 2);
            REQUIRE_THROWS(tmp = obj("obj1.str.*", true)); // Wilcard expects an object
            REQUIRE_NOTHROW(tmp = obj("obj1.obj2", true));
            REQUIRE(tmp.isObject());
            REQUIRE_NOTHROW(tmp = obj("obj1.obj2.*", true)); // result is an object so * operation won't throw
            REQUIRE(tmp.isObject());
            REQUIRE_NOTHROW(tmp = obj("obj1.obj2.three", true));
            REQUIRE(tmp.isNumber());
            REQUIRE((int)tmp == 3);
        }
    }

    SECTION("enumerating arrays and objects") {
        /* test enumerating arrays and objects */
        WHEN("using callback enumerators") {
            /* testing enumerating using | operator */
            json::Object arr(json::Arr, 1, true, "Cali", json::Object(json::Arr, 2, false, "Cali"));
            int count{0};
            arr | [&count](json::Object tmp) -> bool {
                switch (count) {
                    case 0:
                        REQUIRE(1 == (int) tmp);
                        break;
                    case 1:
                        REQUIRE((bool) tmp);
                        break;
                    case 2:
                        REQUIRE((tmp.isString() && (strcmp(tmp.mNode->string_, "Cali") == 0)));
                        break;
                    case 3:
                        REQUIRE(tmp.isArray());
                        break;
                    default:
                        REQUIRE(false);
                        break;
                }
                count++;
                return false;
            };
            REQUIRE(count == 4);
            /* test breaking the loop */
            count = 0;
            arr | [&count](json::Object tmp) -> bool {
                return ++count % 2 == 0;    /* break out of loop early */
            };
            REQUIRE(count == 2);

            /*iterating obects works as iterating arrays*/
            json::Object obj(json::Obj, "one", 1, "bool", true, "str", "Cali", "obj",
                             json::Object(json::Obj, "two", 2, "bool", false, "str", "Cali"));
            count = 0;
            obj | [&count](const char *key, json::Object tmp) {
                REQUIRE(suil::strmatchany(key, "one", "bool", "str", "obj"));
                REQUIRE(suil::strmatchany(tmp.mNode->key, "one", "bool", "str", "obj"));
                count++;
                return false;
            };
            REQUIRE(count == 4);
        }

        WHEN("enumerating using iterators/range loop") {
            /* tests enumeration using for loop */
            json::Object arr(json::Arr, 1, true, "Cali", json::Object(json::Arr, 2, false, "Cali"));
            int count{0};
            for(const auto [_,tmp] : arr) {
                /* basically for an array the first return is always null */
                REQUIRE(_ == nullptr);
                switch (count) {
                    case 0:
                        REQUIRE(1 == (int) tmp);
                        break;
                    case 1:
                        REQUIRE((bool) tmp);
                        break;
                    case 2:
                        REQUIRE((tmp.isString() && (strcmp(tmp.mNode->string_, "Cali") == 0)));
                        break;
                    case 3:
                        REQUIRE(tmp.isArray());
                        break;
                    default:
                        REQUIRE(false);
                        break;
                }
                count++;
            }
            REQUIRE(count == 4);

            /*iterating obects works as iterating arrays*/
            json::Object obj(json::Obj, "one", 1, "bool", true, "str", "Cali", "obj",
                             json::Object(json::Obj, "two", 2, "bool", false, "str", "Cali"));
            count = 0;
            for (const auto [key, val] : obj) {
                /* for objects key is the value's key */
                REQUIRE(suil::strmatchany(key, "one", "bool", "str", "obj"));
                REQUIRE(suil::strmatchany(val.mNode->key, "one", "bool", "str", "obj"));
                count++;
            }
            REQUIRE(count == 4);

            // Other types are not enumerable
            count = 0;
            json::Object a("C");
            for(const auto [_,__] : a) count++;
            REQUIRE(count == 0);
            a = json::Object(1);
            for(const auto [_,__] : a) count++;
            REQUIRE(count == 0);
        }
    }

    SECTION("encoding/decoding json object") {
        /* tests encoding decoding json object
         * !!! NOTE !!! since this is clone from ccan/json, I'm only
         * validating my editions */
        String str = R"({"one":1,"bool":true,"str":"Cali","obj":{"two":2,"bool":false,"str":"Cali"},"arr":[1,true,"Cali"]})";
        String str2 = R"({"data":{"one":1,"bool":true,"str":"Cali","obj":{"two":2,"bool":false,"str":"Cali"},"arr":[1,true,"Cali"]},"msg":"Hello Carter"})";
        WHEN("decoding json strings") {
            auto size{str.size()};
            auto obj = json::Object::decode(str.data(), size); // decode json object
            REQUIRE(size == str.size());
            auto tmp = obj["one"];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(1 == (int) tmp);
            tmp = obj["bool"];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE((bool) tmp);
            tmp = obj["str"];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(strcmp(tmp.mNode->string_, "Cali") == 0);
            auto obj1 = obj["obj"];
            REQUIRE_FALSE(obj1.mNode == nullptr);
            REQUIRE(obj1.ref);
            REQUIRE(obj1.mNode->tag == json::Tag::JSON_OBJECT);

            tmp = obj1["two"];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(2 == (int) tmp);
            tmp = obj1["bool"];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE_FALSE((bool) tmp);
            tmp = obj1["str"];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(strcmp(tmp.mNode->string_, "Cali") == 0);

            auto arr = obj["arr"];
            REQUIRE_FALSE(arr.mNode == nullptr);
            tmp = arr[0];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(1 == (int) tmp);
            tmp = arr[1];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE((bool) tmp);
            tmp = arr[2];
            REQUIRE_FALSE(tmp.mNode == nullptr);
            REQUIRE(tmp.ref);
            REQUIRE(strcmp(tmp.mNode->string_, "Cali") == 0);

            /* !!!NOTE!!! this is really important */
            size = str2.size()-9;
            REQUIRE_NOTHROW((obj = json::Object::decode(&str2.data()[8], size)));
            REQUIRE(size == str.size());
        }

        WHEN("encoding JSON object") {
            /*encoding with wrapper functions */
            json::Object obj(json::Obj, "one", 1, "bool", true, "str", "Cali", "obj",
                             json::Object(json::Obj, "two", 2, "bool", false, "str", "Cali"), "arr",
                             json::Object(json::Arr, 1, true, "Cali"));
            auto s1 = json::encode(obj);
            REQUIRE(String(s1) == str);
        }
    }

    SECTION("converting IOD serializable and JSON object") {
        // we should be able to easily convert between IOD and JSON object
        typedef decltype(iod::D(
                test:: prop(a(:: var(json_skip)),        bool),
                test:: prop(b(:: var(optional)),         String),
                test:: prop(c,           iod::Nullable<std::vector<int>>)
        )) IodInner;
        typedef decltype(iod::D(
                test:: prop(a,           std::string),
                test:: prop(b,           std::vector<int>),
                test:: prop(c,           iod::Nullable<IodInner>)
        )) IodType;

        WHEN("Converting from IOD to json::Object") {
            // should be able to cast/assign from IOD to json::Object
            IodType iodValue{};
            iodValue.a  = "Hello";
            for(int i = 0; i < 10; i++)
                iodValue.b.push_back(i);
            json::Object j1(iodValue);
            REQUIRE(j1.isObject());
            REQUIRE(iodValue.a == (std::string) j1["a"]);
            REQUIRE(j1["c"].isNull());
            auto v1 = j1["b"];
            for (int i = 0; i < 10; i++) {
                // verify vector
                REQUIRE(i == (int)v1[i]);
            }
            // assign value to inner type
            iodValue.c = IodInner{};
            auto& innerType = *iodValue.c;
            innerType.a = true;
            innerType.b = "World";
            json::Object j2(iodValue);
            REQUIRE(j2["c"].isObject());
            REQUIRE(j2["c"]["a"].isNull());
            REQUIRE(innerType.b == (String)j2["c"]["b"]);
            REQUIRE(j2["c"]["c"].isNull());

            innerType.c = std::vector<int>{1, 2};
            json::Object j3(iodValue);
            REQUIRE(j3["c"]["c"].isArray());
            REQUIRE(1 == (int) j3["c"]["c"][0]);
            REQUIRE(2 == (int) j3["c"]["c"][1]);
        }

        WHEN("Converting from json::Object to IOD") {
            // should be able to convert IOD's from JSON object
            json::Object j1(json::Obj, "a", "Hello",
                            "b", std::vector<int>{1, 2, 3});
            auto it1 = (IodType) j1;
            REQUIRE(it1.a == "Hello");
            REQUIRE(it1.b.size() == 3);
            REQUIRE(it1.b[0] == 1);
            REQUIRE(it1.b[1] == 2);
            REQUIRE(it1.b[2] == 3);
            REQUIRE_FALSE(it1.c);
            j1.set("c", json::Object(json::Obj, "a", true, "c", std::vector<int>{1, 2}));
            auto it2 = (IodType) j1;
            REQUIRE(it2.c);
            REQUIRE_FALSE(it2.c->a);
            REQUIRE(it2.c->c->size() == 2);
            REQUIRE((*(it2.c->c))[0] == 1);
            REQUIRE((*(it2.c->c))[1] == 2);
        }
    }

    SECTION("other JSON Object properties") {
        /* some miscellaneous tests */
        WHEN("checking type") {
            /* check type checking API's */
            json::Object j{};
            REQUIRE(j.isNull());
            j = json::Object(nullptr);
            REQUIRE(j.isNull());
            j = json::Object{'5'};
            REQUIRE(j.isNumber());
            j = json::Object{true};
            REQUIRE(j.isBool());
            j = json::Arr;
            REQUIRE(j.isArray());
            j = json::Obj;
            REQUIRE(j.isObject());
        }

        WHEN("checking if object is empty") {
            /* test empty method */

            /* null object is always empty */
            json::Object j{};
            REQUIRE(j.empty());
            /* numbers boolean's can never be empty */
            j = json::Object{5};
            REQUIRE_FALSE(j.empty());
            j = json::Object{true};
            REQUIRE_FALSE(j.empty());

            /* for string, we check the actual string */
            j = json::Object{""};
            REQUIRE(j.empty());
            j = json::Object{"Hello"};
            REQUIRE_FALSE(j.empty());
            /* arrays and objects are empty when ther is no elements or members */
            j = json::Arr;
            REQUIRE(j.empty());
            j.push(1, true, "Cali");
            REQUIRE_FALSE(j.empty());

            j = json::Obj;
            REQUIRE(j.empty());
            j.set("name", "Cali");
            REQUIRE_FALSE(j.empty());
        }

        WHEN("using JSON Object with IOD") {
            /* the main reason for adding json::Object was to have
             * a dynamic JSON object within iod object */
            typedef decltype(iod::D(
                    test:: prop(a,        int),
                    test:: prop(b,        String),
                    test:: prop(c,        json::Object)
            )) Type;
            String s1 = R"({"a":5,"b":"Cali","c":{"one":1,"bool":true,"str":"Cali"}})";
            Type obj;
            json::decode(s1, obj);
            REQUIRE(obj.a == 5);
            REQUIRE(obj.b == "Cali");
            REQUIRE(1 == (int)obj.c["one"]);
            REQUIRE((bool) obj.c["bool"]);
            REQUIRE(String{"Cali"} == (String) obj.c["str"]);

            auto s2 = json::encode(obj);
            REQUIRE(String(s2) == s1);
        }

        WHEN("Using a meta object") {
            Mt mt;
            mt.a = 29;
            mt.b = "Carter";
            auto str = json::encode(mt);
            REQUIRE(str == R"({"a":29,"b":"Carter"})");
            Mt mt1;
            json::decode(str, mt1);
            REQUIRE(mt1.a == 29);
            REQUIRE(mt1.b == "Carter");
        }

        WHEN("Loading a LUA config script to JSON object") {
            String lConfig{"app = { "
                           "   num = 1,"
                           "   str = 'A string',"
                           "   bool = true,"
                           "   arr = {1, true, 'string', {1, true}},"
                           "   obj = {"
                           "       num = 1,"
                           "       bool = true"
                           "   }"
                           "}"};
            json::Object obj = json::Object::fromLuaString(lConfig);
            REQUIRE_FALSE(obj.empty());
            json::Object val, v2;

            REQUIRE_NOTHROW(val = obj["num"]);
            REQUIRE(val.isNumber());
            REQUIRE((int)val == 1);

            REQUIRE_NOTHROW(val = obj["str"]);
            REQUIRE(val.isString());
            REQUIRE((String)val == "A string");

            REQUIRE_NOTHROW(val = obj["bool"]);
            REQUIRE(val.isBool());
            REQUIRE((bool)val);

            REQUIRE_NOTHROW(val = obj["arr"]);
            REQUIRE(val.isArray());
            REQUIRE((int)val[0] == 1);
            REQUIRE((bool)val[1]);
            REQUIRE((String)val[2] == "string");

            REQUIRE_NOTHROW(v2 = val[3]);
            REQUIRE(v2.isArray());
            REQUIRE((int)v2[0] == 1);
            REQUIRE((bool)v2[1]);

            REQUIRE_NOTHROW(val = obj["obj"]);
            REQUIRE(val.isObject());
            REQUIRE((int)val["num"] == 1);
            REQUIRE((bool)val["bool"]);

            // When passing empty config
            REQUIRE_NOTHROW(obj = json::Object::fromLuaString("app = {}"));
            // passing empty config will fail
            REQUIRE_THROWS(obj = json::Object::fromLuaString(""));
            // Config must be within app declaration
            REQUIRE_THROWS(obj = json::Object::fromLuaString("configuration = {}"));
            // Cannot mix array and object indexing
            REQUIRE_THROWS(obj = json::Object::fromLuaString("app = {'one', name='Carter'}"));
        }
    }
}

#endif

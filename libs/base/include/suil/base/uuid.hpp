//
// Created by Mpho Mbotho on 2020-10-08.
//

#ifndef SUIL_BASE_UUID_HPP
#define SUIL_BASE_UUID_HPP

#include "suil/base/string.hpp"

#include <uuid/uuid.h>

namespace suil {

    /**
     * generete a uuid using the uuid_generate_time_safe function
     * @param id the buffer to hold the generated uuid
     * @return
     */
    unsigned char* uuid(uuid_t id);


    /**
     * parse the given string into a uuid_t object
     *
     * @param str the string parse which must be a valid uuid
     * @param out the uuid_t reference to hold the parsed uuid
     *
     * @return true if the given string parsed successfully as a uuid
     */
    inline bool parseuuid(const String& str, uuid_t& out) {
        return !str.empty() && uuid_parse(str.data(), out);
    }

    /**
     * checks to see if the given string is a valid uuid
     * @param str the string to check as a valid uuid
     * @return true if the string is a valid uuid, flse otherwise
     */
    inline bool uuidvalid(const String& str) {
        uuid_t tmp;
        return parseuuid(str, tmp);
    }

    /**
     * dump the given \param uuid to string. if the \param uuid is null
     * a new uuid is genereted
     *
     * @param uuid the uuid to format as a string
     *
     * @return the string representation of the given uuid
     */
    String uuidstr(uuid_t uuid = nullptr);
}

#endif //SUIL_BASE_UUID_HPP

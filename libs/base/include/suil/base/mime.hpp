//
// Created by Mpho Mbotho on 2020-10-13.
//

#ifndef SUIL_BASE_MIME_HPP
#define SUIL_BASE_MIME_HPP

#include "suil/base/string.hpp"

namespace suil {

    /**
     * utility to find the MIME type of a file from a list
     * of sapi known MIME types
     *
     * @param filename the filename whose MIME time must be decode
     *
     * @return the MIME type if found, otherwise null is returned
     */
    const char *mimetype(const String& filename);

}
#endif //SUIL_BASE_MIME_HPP

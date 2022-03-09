/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-05
 */


#include "suil/async/utils.hpp"

#include <fcntl.h>

namespace suil {

    int nonblocking(int fd, bool on)
    {
        int opt = fcntl(fd, F_GETFL, 0);
        if (opt == -1)
            opt = 0;
        opt = on? opt | O_NONBLOCK : opt & ~O_NONBLOCK;
        return fcntl(fd, F_SETFL, opt);
    }
}
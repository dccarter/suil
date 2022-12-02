/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-21
 */

#include "suil/base/c/args.h"

#include <suil/base/logging.hpp>

bool suilParseLogLevel(CmdParser *P,
                       CmdFlagValue* dst,
                       const char *str,
                       const char *name)
{
    CmdBitFlagDesc bitFlagDesc[] = {
        {"TRACE", suil::TRACE},
        {"DEBUG", suil::DEBUG},
        {"INFO",  suil::INFO},
        {"WARNING", suil::WARNING},
        {"ERROR", suil::ERROR},
        {"CRITICAL", suil::CRITICAL}
    };

    return cmdParseStrValue(P, dst, str, name, bitFlagDesc, sizeof__(bitFlagDesc));
}

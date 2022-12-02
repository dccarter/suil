//
// Created by Mpho Mbotho on 2020-12-18.
//

#include "suil/base/c/args.h"

Command(start, "start the example server application",
    Int(Name("workers"),
        Sf('W'),
        Help("The number of worker processes that will be used by the server")
        Def("0xff")),
    Use(suilParseLogLevel,
        Name("verbose"), Sf('v'),
        Help("Logging verbosity, one of (TRACE, DEBUG, INFO, WARNING, ERROR)"),
        Def("DEBUG"))
);

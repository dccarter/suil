//
// Created by Carter on 2022-01-06.
//

#define CATCH_CONFIG_RUNNER
#include <fcntl.h>

#include "catch2/catch.hpp"

int main(int argc, const char *argv[])
{
    int result = Catch::Session().run(argc, argv);
    return (result < 0xff ? result: 0xff);
}
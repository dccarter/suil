cmake_minimum_required(VERSION 3.16)

# The version of the library, configurable from command line
if (NOT SUIL_VERSION)
    set(SUIL_UTILS_VERSION "0.1.0" CACHE STRING "The version of suil utils library")
else()
    set(SUIL_UTILS_VERSION ${SUIL_VERSION})
endif()

project(Utils
        VERSION ${SUIL_UTILS_VERSION}
        LANGUAGES C CXX)

if (LINUX)
    set(SUIL_PLATFORM_SOURCES)
elseif(UNIX)
    set(SUIL_PLATFORM_SOURCES)
endif()

set(SUIL_UTILS_SOURCES
        ${SUIL_PLATFORM_SOURCES}
        src/exception.cpp
        src/utils.cpp)

# Add and configure utils library
SuilAddLibrary(Utils
        KIND STATIC
        SOURCES ${SUIL_UTILS_SOURCES}
        RENAME  SuilUtils
        LIBS OpenSSL::SSL)

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    SuilUnitTest(Utils-UnitTest
            SOURCES
                ${SUIL_UTILS_SOURCES}
                test/main.cpp
            INCLUDES include
            RENAME  SuilUtilsUt
            LIBS OpenSSL::SSL)
endif()
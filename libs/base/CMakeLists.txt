cmake_minimum_required(VERSION 3.10)
# The version of the library, configurable from command line
set(SUIL_BASE_VERSION "0.1.0" CACHE STRING "The version of suil-base library")

project(SuilBase VERSION ${SUIL_BASE_VERSION} LANGUAGES C CXX)

set(SUIL_BASE_SOURCES
        src/base64.cpp
        src/buffer.cpp
        src/args.cpp
        src/console.cpp
        src/crypto.cpp
        src/data.cpp
        src/datetime.cpp
        src/encryption.cpp
        src/env.cpp
        src/file.cpp
        src/hash.cpp
        src/json.cpp
        src/logging.cpp
        src/mime.cpp
        src/mustache.cpp
        src/notify.cpp
        src/process.cpp
        src/regex.cpp
        src/result.cpp
        src/secpk2561.cpp
        src/signal.cpp
        src/string.cpp
        src/syson.cpp
        src/units.cpp
        src/url.cpp
        src/uuid.cpp
        src/union.cpp
        src/varint.cpp
        src/wire.cpp)

# Base library
SuilAddLibrary(Base
        KIND STATIC
        SOURCES      ${SUIL_BASE_SOURCES}
        RENAME       SuilBase
        LIBS         Suil::Async Iod::Iod OpenSSL::SSL OpenSSL::Crypto Uuid::Uuid Dl::Dl Secp256k1::Secp256k1)


include(generator/.cmake)
include(examples/.cmake)

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    include(SuilUnitTest)
    SuilUnitTest(Base-UnitTest
            SOURCES ${SUIL_BASE_SOURCES} test/main.cpp
            LIBS Libmill::Libmill Iod::Iod OpenSSL::SSL OpenSSL::Crypto Uuid::Uuid Dl::Dl Secp256k1::Secp256k1)
    target_include_directories(Base-UnitTest
            PRIVATE include)
    set_target_properties(Base-UnitTest
        PROPERTIES
            RUNTIME_OUTPUT_NAME base_unittest)
endif()
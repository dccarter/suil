cmake_minimum_required(VERSION 3.10)
# The version of the library, configurable from command line
set(SUIL_BASE_VERSION "0.1.0" CACHE STRING "The version of suil-base library")

project(SuilBase VERSION ${SUIL_BASE_VERSION} LANGUAGES C CXX)

set(SUIL_BASE_SOURCES
        src/base64.cpp
        src/buffer.cpp
        src/channel.cpp
        src/args.cpp
        src/console.cpp
        src/crypto.cpp
        src/data.cpp
        src/datetime.cpp
        src/encryption.cpp
        src/env.cpp
        src/exception.cpp
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
        src/utils.cpp
        src/url.cpp
        src/uuid.cpp
        src/union.cpp
        src/varint.cpp
        src/wire.cpp)

# Base library
add_library(Base STATIC
        ${SUIL_BASE_SOURCES})
add_library(Suil::Base ALIAS Base)
set_target_properties(Base
    PROPERTIES
        OUTPUT_NAME SuilBase)

target_include_directories(Base PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

target_link_libraries(Base
        PUBLIC Libmill::Libmill Iod::Iod OpenSSL::SSL OpenSSL::Crypto Uuid::Uuid Dl::Dl Secp256k1::Secp256k1)

# Install base library
install(TARGETS Base
        EXPORT  ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Install includes library
install(DIRECTORY include/suil/base/
        DESTINATION include/suil/base
        FILES_MATCHING PATTERN "*.hpp")

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
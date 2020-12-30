CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
set(SUIL_VERSION 0.1.0 CACHE STRING "The god version of suil projects")
project(suil
        VERSION ${SUIL_VERSION}
        DESCRIPTION "This is the to level project to help with development of suil projects"
        LANGUAGES C CXX)

# Configure project for C++20 support
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Some project options
option(ENABLE_UNIT_TESTS    "Enable building of unit tests" ON)
option(ENABLE_EXAMPLES      "Enable building examples included in the project" ON)
option(DEVEL_IDE            "When enabled, it will suppress some cmake errors" OFF)

set(SUIL_ENABLE_BACKTRACE  1 CACHE STRING "Enable/Disable backtrace in builds")
set(SUIL_ENABLE_TRACE  1 CACHE STRING "Enable trace logs in debug builds (Debug only option)")
set(SUIL_BUILD_NUMBER  0 CACHE STRING "Build number version")
set(SUIL_BUILD_TAG     devel CACHE STRING "Version tag string")
set(SUIL_SOFTWARE_NAME suil  CACHE STRING "The name of the server")
set(SUIL_DEBUG_TAG     1a30f8248a6b11e781846f6e6c696e65 CACHE STRING "The tag used by the debug application tag")
set(SUIL_BASE_DIR      ".suil" CACHE STRING "The base directory of suil application")
set(SUIL_COPYRIGHT     "Copyright (C) 2020, 2017-2020 Suilteam")

# Some times implementations are unsafe, for example suil::String is VERY unsafe and need to
# be used with caution.
# Alternative implementations are
# LAZY multiple type instances can share a buffer so the instances must be used with caution
# SAFE use std::shared_ptr for the underlying buffer can be shared between types
# VERY_SAFE does not share buffer, e.g method String::peek() will return a copy of the buffer
set(SUIL_BUILD_TYPE_MODE    LAZY CACHE STRING "Switch between string implementations (Default LAZY)")

set(SUIL_FILE_SERVER_ROUTE 1b7647618ea63937daae8ebb903e9175)

# Tell libraries that they are supervised by god
set(SUIL_GOD_MODE ON)

# Configure path for loading suil cmake scripts
set(CMAKE_MODULE_PATH /usr/local/share/suil/cmake /usr/share/suil/cmake)

include(cmake/SuilUtils.cmake)
include(3rdParty.cmake)
include(SuilScc)

# Get OS variant
include(SuilGetOsVariant)
if (OS_VARIANT)
    string(TOUPPER ${OS_VARIANT} OS_VARIANT)
else()
    set(OS_VARIANT UBUNTU)
endif()
string(TOUPPER ${CMAKE_BUILD_TYPE} SUIL_BUILD_TYPE)

configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/Version.cmake.in
        ${CMAKE_BINARY_DIR}/.generated/suil/version.hpp)

# OpenSSL is required to build suil libraries
find_package(OpenSSL REQUIRED)
add_compile_definitions(SSL_SUPPORTED)
include_directories(${OpenSSL_INCLUDE_DIRS})

# PostgresSQL is also required
find_package(PostgreSQL ${SUIL_PGSQL_VERSION} REQUIRED)
include_directories(${PostgreSQL_INCLUDE_DIRS})

# ZMQ is also required to build suil
pkg_check_modules(ZeroMQ QUIET zmq)
if (NOT ZeroMQ_FOUND)
    pkg_check_modules(ZeroMQ QUIET libzmq)
    if (NOT DEVEL_IDE AND NOT ZeroMQ_FOUND)
        message(FATAL_ERROR "ZMQ development libraries required")
    endif()
endif()

# Check some required functions
SuilCheckFunctions()
# Check some required libraries
SuilCheckLibrary(uuid INCLUDE uuid/uuid.h)
SuilCheckLibrary(sqlite3 LIBRARY sqlite3 libsqlite3)

# Directory where scc sources are generated
include_directories(${CMAKE_BINARY_DIR}/scc)

# Directory with config file is generated
include_directories(${CMAKE_BINARY_DIR}/.generated)
# add suil base project
include_directories(libs/base/include)
add_subdirectory(libs/base)

# add suil network project
include_directories(libs/network/include)
add_subdirectory(libs/network)

# add suil network project
include_directories(libs/database/include)
add_subdirectory(libs/database)

# add suil rpc project
include_directories(libs/rpc/include)
add_subdirectory(libs/rpc)

# add suil rpc project
include_directories(libs/http/include)
add_subdirectory(libs/http)

set(CPACK_PACKAGE_NAME   ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "Suilteam")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Carter Mbotho")
set(CPACK_PACKAGE_FILE_NAME ${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${CMAKE_BUILD_TYPE})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_GENERATOR "TGZ")

include(CPack)
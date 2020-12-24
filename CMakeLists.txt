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
option(ENABLE_TRACE         "Enable trace logs in debug builds (Debug only option)" ON)
option(DEVEL_IDE            "When enabled, it will suppress some cmake errors" OFF)

if(CMAKE_BUILD_TYPE STREQUAL Debug AND ENABLE_TRACE)
    add_compile_definitions(SUIL_TRACE_ENABLED=1)
endif()

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
    add_compile_definitions(__${OS_VARIANT}__)
endif()

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
CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
set(SUIL_VERSION 0.1.0 CACHE STRING "The god version of suil projects")
project(Suil
        VERSION ${SUIL_VERSION}
        DESCRIPTION "This is the to level project to help with development of suil projects"
        LANGUAGES C CXX)

# Configure C++ project
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if (NOT CMAKE_BUILD_TYPE)
    # default build type is Debug
    set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -Wno-address-of-packed-member)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Postfix all debug libraries with -dbg
#set(CMAKE_DEBUG_POSTFIX "-dbg")

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
set(SUIL_COPYRIGHT     "Copyright (C) 2022, 2017-2022 Suilteam")

# Some implementations are unsafe, for example suil::String is VERY unsafe and need to
# be used with caution.
# Alternative implementations are
# LAZY multiple type instances can share a buffer so the instances must be used with caution
# SAFE use std::shared_ptr for the underlying buffer can be shared between types
# VERY_SAFE does not share buffer, e.g method String::peek() will return a copy of the buffer
set(SUIL_BUILD_TYPE_MODE    LAZY CACHE STRING "Switch between string implementations (Default LAZY)")

# This is the route used to configure an HTTP file server on an HTTP endpoint
set(SUIL_FILE_SERVER_ROUTE 1b7647618ea63937daae8ebb903e9175)


# Configure path for loading suil cmake scripts
set(CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(cmake/SuilUtils.cmake)
include(cmake/SuilGetOsVariant.cmake)

# Get OS variant
SuilGetOsVariant(OS_VARIANT)
string(TOUPPER ${CMAKE_BUILD_TYPE} SUIL_BUILD_TYPE)

# Create a version file
configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/Version.cmake.in
        ${CMAKE_BINARY_DIR}/.generated/suil/version.hpp)
include_directories(${CMAKE_BINARY_DIR}/.generated)

# Install generated version file
install(FILES ${CMAKE_BINARY_DIR}/.generated/suil/version.hpp
        DESTINATION include/suil/)

# Install CMake files
install(DIRECTORY ${CMAKE_SOURCE_DIR}/cmake/
        DESTINATION lib/cmake
        FILES_MATCHING PATTERN "*.cmake")

# Find required libraries
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL ${SUIL_PGSQL_VERSION} REQUIRED)
find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Uuid REQUIRED)
find_package(Zmq REQUIRED)
find_package(LuaS REQUIRED)
find_package(Dl REQUIRED)
find_package(Secp256k1 REQUIRED)

include(FetchContent)

# Fetch libmill library
FetchContent_Declare(
        libmill
        GIT_REPOSITORY https://gitlab.com/sw-devel/thirdparty/libmill.git
#        GIT_TAG demos
)

# Fetch Iod project
FetchContent_Declare(
        iod
        GIT_REPOSITORY https://gitlab.com/sw-devel/thirdparty/iod.git)

# Fetch Scc project
FetchContent_Declare(
    scc
    GIT_REPOSITORY https://github.com/dccarter/scc.git
    GIT_TAG v0.1.0)

FetchContent_MakeAvailable(libmill iod scc)
set(CMAKE_MODULE_PATH
        ${CMAKE_MODULE_PATH}
        ${scc_SOURCE_DIR}/cmake)

set(SUIL_SCC_DEFAULT_BINARY ${scc_BINARY_DIR}/scc)
include(SuilScc)

if (ENABLE_UNIT_TESTS)
    include(Catch.cmake)
endif()

set(NAMESPACE      Suil::)
set(TARGETS_EXPORT_NAME  ${PROJECT_NAME}Targets)

add_subdirectory(libs/base)
add_subdirectory(libs/network)
add_subdirectory(libs/database)
add_subdirectory(libs/rpc)
add_subdirectory(libs/http)
add_subdirectory(libs/sawtooth)

include(CMakePackageConfigHelpers)

set(GENERATED_DIR  ${CMAKE_BINARY_DIR}/generated)
set(VERSION_CONFIG ${GENERATED_DIR}/${PROJECT_NAME}ConfigVersion.cmake)
set(PROJECT_CONFIG ${GENERATED_DIR}/${PROJECT_NAME}Config.cmake)

# Configure '<PROJECT-NAME>Config.cmake.in'
configure_package_config_file(
        ${CMAKE_SOURCE_DIR}/cmake/Config.cmake.in
        ${PROJECT_CONFIG}
        INSTALL_DESTINATION lib/cmake/${PROJECT_NAME})

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
write_basic_package_version_file(
        "${VERSION_CONFIG}" COMPATIBILITY SameMajorVersion)

# Install config
#   * <prefix>/lib/cmake/SuilNetConfig.cmake
#   * <prefix>/lib/cmake/SuilNetConfigVersion.cmake
install(FILES ${PROJECT_CONFIG} ${VERSION_CONFIG}
        DESTINATION lib/cmake/${PROJECT_NAME})

# Install cmake targets
#   * <prefix>/lib/cmake/SuilNetTargets.cmake
install(EXPORT      ${TARGETS_EXPORT_NAME}
        NAMESPACE   ${NAMESPACE}
        DESTINATION lib/cmake/${PROJECT_NAME})

set(CPACK_PACKAGE_NAME   ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "Suilteam")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Carter Mbotho")
set(CPACK_PACKAGE_FILE_NAME ${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${CMAKE_BUILD_TYPE})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_GENERATOR "TGZ")

include(CPack)
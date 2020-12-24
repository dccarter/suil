cmake_minimum_required(VERSION 3.10)
# The version of the library, configurable from command line
set(SUIL_RPC_VERSION "0.1.0" CACHE STRING "The version of suil-rpc library")

project(SuilRpc VERSION ${SUIL_RPC_VERSION} LANGUAGES C CXX)

find_package(OpenSSL REQUIRED)
include_directories(include)
include_directories(${OpenSSL_INCLUDE_DIRS})

include_directories(${CMAKE_BINARY_DIR}/scc)
include_directories(include)

set(SUIL_RPC_SOURCES
        src/json/client.cpp
        src/json/connection.cpp
        src/json/context.cpp
        src/srpc/client.cpp
        src/srpc/connection.cpp
        src/srpc/context.cpp
        src/client.cpp
        src/suilrpc.cpp
        ${CMAKE_BINARY_DIR}/scc/suil/rpc/common.scc.cpp
        ${CMAKE_BINARY_DIR}/scc/suil/rpc/json.scc.cpp
        ${CMAKE_BINARY_DIR}/scc/suil/rpc/wire.scc.cpp)

add_library(SuilRpc STATIC
    ${SUIL_RPC_SOURCES})

target_link_libraries(SuilRpc
        SuilNet SuilBase mill ${OPENSSL_LIBRARIES})

set(SUIL_RPC_SCC_SOURCES
        scc/common.scc
        scc/json.scc
        scc/wire.scc)

SuilScc(SuilRpc
        SOURCES  ${SUIL_RPC_SCC_SOURCES}
        LIB_PATH ${CMAKE_BINARY_DIR}
        OUTDIR   ${CMAKE_BINARY_DIR}/scc/suil/rpc)

add_dependencies(SuilRpc-scc SuilBase-Generator)

include(CMakePackageConfigHelpers)

set(GENERATED_DIR  ${CMAKE_BINARY_DIR}/generated)
set(VERSION_CONFIG ${GENERATED_DIR}/${PROJECT_NAME}ConfigVersion.cmake)
set(PROJECT_CONFIG ${GENERATED_DIR}/${PROJECT_NAME}Config.cmake)
set(TARGETS_EXPORT_NAME  ${PROJECT_NAME}Targets)
set(NAMESPACE            suil::rpc::)

# Configure '<PROJECT-NAME>Config.cmake.in'
configure_package_config_file(
        cmake/Config.cmake.in
        ${PROJECT_CONFIG}
        INSTALL_DESTINATION lib/cmake/${PROJECT_NAME})

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
write_basic_package_version_file(
        "${VERSION_CONFIG}" COMPATIBILITY SameMajorVersion)

# Install base library
install(TARGETS SuilRpc
        EXPORT  ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Install includes library
install(DIRECTORY include/suil/rpc/
        DESTINATION include/suil/rpc
        FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_BINARY_DIR}/scc/suil/rpc/
        DESTINATION include/suil/net
        FILES_MATCHING PATTERN "*.hpp")

# Install config
#   * <prefix>/lib/cmake/SuilRpcConfig.cmake
#   * <prefix>/lib/cmake/SuilRpcConfigVersion.cmake
install(FILES ${PROJECT_CONFIG} ${VERSION_CONFIG}
        DESTINATION lib/cmake/${PROJECT_NAME})

# Install cmake targets
#   * <prefix>/lib/cmake/SuilRpcTargets.cmake
install(EXPORT      ${TARGETS_EXPORT_NAME}
        NAMESPACE   ${NAMESPACE}
        DESTINATION lib/cmake/${PROJECT_NAME})

include(generator/.cmake)

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    include(SuilUnitTest)
    SuilUnitTest(SuilRpc-UnitTest
            SOURCES ${SUIL_NETWORK_SOURCES} test/main.cpp
            LIBS    SuilBase SuilNet mill ${OPENSSL_LIBRARIES})
    set_target_properties(SuilRpc-UnitTest
        PROPERTIES
            RUNTIME_OUTPUT_NAME rpc_unittest)
endif()
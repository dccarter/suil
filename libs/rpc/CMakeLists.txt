cmake_minimum_required(VERSION 3.10)
# The version of the library, configurable from command line
set(SUIL_RPC_VERSION "0.1.0" CACHE STRING "The version of suil-rpc library")

project(Rpc VERSION ${SUIL_RPC_VERSION} LANGUAGES C CXX)

set(SUIL_RPC_SOURCES
        src/json/client.cpp
        src/json/connection.cpp
        src/json/context.cpp
        src/srpc/client.cpp
        src/srpc/connection.cpp
        src/srpc/context.cpp
        src/client.cpp
        src/suilrpc.cpp
        ${CMAKE_BINARY_DIR}/scc/public/suil/rpc/common.scc.cpp
        ${CMAKE_BINARY_DIR}/scc/public/suil/rpc/json.scc.cpp
        ${CMAKE_BINARY_DIR}/scc/public/suil/rpc/wire.scc.cpp)

add_library(Rpc STATIC
    ${SUIL_RPC_SOURCES})
add_library(Suil::Rpc ALIAS Rpc)
set_target_properties(Rpc
    PROPERTIES
        OUTPUT_NAME SuilRpc)

target_link_libraries(Rpc
        PUBLIC Suil::Net)

target_include_directories(Rpc PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_include_directories(Rpc PRIVATE
        ${CMAKE_BINARY_DIR}/scc/public)

set(SUIL_RPC_SCC_SOURCES
        scc/common.scc
        scc/json.scc
        scc/wire.scc)

SuilScc(Rpc
        SOURCES  ${SUIL_RPC_SCC_SOURCES}
        LIB_PATH ${CMAKE_BINARY_DIR}
        OUTDIR   ${CMAKE_BINARY_DIR}/scc/public/suil/rpc)
add_dependencies(Rpc-scc Base-Generator)

# Install base library
install(TARGETS Rpc
        EXPORT  ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Install includes library
install(DIRECTORY include/suil/rpc/
        DESTINATION include/suil/rpc
        FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_BINARY_DIR}/scc/public/suil/rpc/
        DESTINATION include/suil/rpc
        FILES_MATCHING PATTERN "*.hpp")

include(generator/.cmake)

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    include(SuilUnitTest)
    SuilUnitTest(Rpc-UnitTest
            SOURCES ${SUIL_NETWORK_SOURCES} test/main.cpp
            LIBS    Suil::Net)
    set_target_properties(Rpc-UnitTest
        PROPERTIES
            RUNTIME_OUTPUT_NAME rpc_unittest)
endif()
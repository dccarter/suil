cmake_minimum_required(VERSION 3.10)
# The version of the library, configurable from command line
set(SUIL_NETWORK_VERSION "0.1.0" CACHE STRING "The version of suil-network library")

project(Net VERSION ${SUIL_NETWORK_VERSION} LANGUAGES C CXX)

set(SUIL_NETWORK_SOURCES
        src/chunk.cpp
        src/email.cpp
        src/server.cpp
        src/socket.cpp
        src/ssl.cpp
        src/tcp.cpp
        src/unix.cpp
        src/smtp/client.cpp
        src/smtp/context.cpp
        src/smtp/protocol.cpp
        src/smtp/server.cpp
        src/smtp/session.cpp
        src/smtp/utils.cpp
        src/zmq/bindop.cpp
        src/zmq/connectop.cpp
        src/zmq/context.cpp
        src/zmq/message.cpp
        src/zmq/monitor.cpp
        src/zmq/monitorop.cpp
        src/zmq/pubsub.cpp
        src/zmq/receiveop.cpp
        src/zmq/socket.cpp
        src/zmq/sendop.cpp
        ${CMAKE_BINARY_DIR}/scc/public/suil/net/config.scc.cpp
        ${CMAKE_BINARY_DIR}/scc/public/suil/net/symbols.scc.cpp)

add_library(Net STATIC
        ${SUIL_NETWORK_SOURCES})
add_library(Suil::Net ALIAS Net)
set_target_properties(Net
    PROPERTIES
        OUTPUT_NAME SuilNet)

target_include_directories(Net PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_include_directories(Net PRIVATE
        ${CMAKE_BINARY_DIR}/scc/public)

target_link_libraries(Net
        PUBLIC Suil::Base Zmq::Zmq)

set(SUIL_NETWORK_SCC_SOURCES
        scc/config.scc
        scc/symbols.scc)

SuilScc(Net
        SOURCES  ${SUIL_NETWORK_SCC_SOURCES}
        LIB_PATH ${CMAKE_BINARY_DIR}
        OUTDIR   ${CMAKE_BINARY_DIR}/scc/public/suil/net)

# Base generator needed
add_dependencies(Net-scc Base-Generator)

# Install base library
install(TARGETS Net
        EXPORT  ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Install includes library
install(DIRECTORY include/suil/net/
        DESTINATION include/suil/net
        FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_BINARY_DIR}/scc/public/suil/net/
        DESTINATION include/suil/net
        FILES_MATCHING PATTERN "*.hpp")

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    include(SuilUnitTest)
    SuilUnitTest(Net-UnitTest
            SOURCES ${SUIL_NETWORK_SOURCES} test/main.cpp
            LIBS    Suil::Base Zmq::Zmq)
    target_include_directories(Net-UnitTest
            PRIVATE include ${CMAKE_BINARY_DIR}/scc/public)

    set_target_properties(Net-UnitTest
            PROPERTIES
            RUNTIME_OUTPUT_NAME net_unittest)
    add_dependencies(Net-UnitTest Net-scc)
endif()

include(example/.cmake)
cmake_minimum_required(VERSION 3.10)
# The version of the library, configurable from command line
set(SUIL_SAWTOOTH_VERSION "0.1.0" CACHE STRING "The version of suil-sawtooth library")

project(Sawtooth VERSION ${SUIL_SAWTOOTH_VERSION} LANGUAGES C CXX)

include(protos/Protos.cmake)

set(SUIL_SAWTOOTH_SOURCES
        ${SAWTOOTH_PROTO_SRCS}
        src/address.cpp
        src/client.cpp
        src/dispatcher.cpp
        src/events.cpp
        src/gstate.cpp
        src/message.cpp
        src/processor.cpp
        src/rest.cpp
        src/stream.cpp
        src/tp.cpp
        src/transaction.cpp
        src/wallet.cpp
        ${CMAKE_BINARY_DIR}/scc/public/suil/sawtooth/sawtooth.scc.cpp)

add_library(Sawtooth STATIC
    ${SUIL_SAWTOOTH_SOURCES})
add_library(Suil::Sawtooth ALIAS Sawtooth)
set_target_properties(Sawtooth
    PROPERTIES
        OUTPUT_NAME SuilSawtooth)

target_link_libraries(Sawtooth
        PUBLIC Suil::HttpClient protobuf::libprotobuf)

target_include_directories(Sawtooth PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_include_directories(Sawtooth PRIVATE
        ${CMAKE_BINARY_DIR}/scc/public)

add_executable(Sawtooth-Wallet
        src/stw/main.cpp
        src/stw/stw.cpp)
target_link_libraries(Sawtooth-Wallet Suil::Sawtooth)

set_target_properties(Sawtooth-Wallet
        PROPERTIES
        RUNTIME_OUTPUT_NAME stw)

set(SUIL_SAWTOOTH_SCC_SOURCES
        scc/sawtooth.scc)

SuilScc(Sawtooth
        SOURCES  ${SUIL_SAWTOOTH_SCC_SOURCES}
        LIB_PATH ${CMAKE_BINARY_DIR}
        OUTDIR   ${CMAKE_BINARY_DIR}/scc/public/suil/sawtooth)
add_dependencies(Sawtooth-scc Base-Generator)

target_include_directories(Sawtooth-Wallet PRIVATE
        ${CMAKE_BINARY_DIR}/scc/public)

# Install base library
install(TARGETS Sawtooth Sawtooth-Wallet
        EXPORT  ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Install includes library
install(DIRECTORY include/suil/sawtooth/
        DESTINATION include/suil/sawtooth
        FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_BINARY_DIR}/scc/public/suil/sawtooth/
        DESTINATION include/suil/sawtooth
        FILES_MATCHING PATTERN "*.hpp")

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    include(SuilUnitTest)
    SuilUnitTest(Sawtooth-UnitTest
            SOURCES test/main.cpp
            LIBS    Suil::HttpClient)
    set_target_properties(Sawtooth-UnitTest
        PROPERTIES
            RUNTIME_OUTPUT_NAME sawtooth_unittest)
endif()

include(example/Examples.cmake)
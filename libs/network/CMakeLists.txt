cmake_minimum_required(VERSION 3.10)
# The version of the library, configurable from command line
set(SUIL_NETWORK_VERSION "0.1.0" CACHE STRING "The version of suil-network library")

project(SuilNetwork VERSION ${SUIL_NETWORK_VERSION} LANGUAGES C CXX)

find_package(OpenSSL REQUIRED)
include_directories(include)
include_directories(${OpenSSL_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_BINARY_DIR}/scc)

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
        ${CMAKE_BINARY_DIR}/scc/suil/net/config.scc.cpp
        ${CMAKE_BINARY_DIR}/scc/suil/net/symbols.scc.cpp)

add_library(SuilNet STATIC
        ${SUIL_NETWORK_SOURCES})

target_link_libraries(SuilNet
        SuilBase mill ${OPENSSL_LIBRARIES} zmq)

set(SUIL_NETWORK_SCC_SOURCES
        scc/config.scc
        scc/symbols.scc)

SuilScc(SuilNet
        SOURCES  ${SUIL_NETWORK_SCC_SOURCES}
        LIB_PATH ${CMAKE_BINARY_DIR}
        OUTDIR   ${CMAKE_BINARY_DIR}/scc/suil/net)

# SuilBase generator needed
add_dependencies(SuilNet-scc SuilBase-Generator)

include(CMakePackageConfigHelpers)

set(GENERATED_DIR  ${CMAKE_BINARY_DIR}/generated)
set(VERSION_CONFIG ${GENERATED_DIR}/${PROJECT_NAME}ConfigVersion.cmake)
set(PROJECT_CONFIG ${GENERATED_DIR}/${PROJECT_NAME}Config.cmake)
set(TARGETS_EXPORT_NAME  ${PROJECT_NAME}Targets)
set(NAMESPACE            suil::net::)

# Configure '<PROJECT-NAME>Config.cmake.in'
configure_package_config_file(
        cmake/Config.cmake.in
        ${PROJECT_CONFIG}
        INSTALL_DESTINATION lib/cmake/${PROJECT_NAME})

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
write_basic_package_version_file(
        "${VERSION_CONFIG}" COMPATIBILITY SameMajorVersion)

# Install base library
install(TARGETS SuilNet
        EXPORT  ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Install includes library
install(DIRECTORY include/suil/net/
        DESTINATION include/suil/net
        FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_BINARY_DIR}/scc/suil/net/
        DESTINATION include/suil/net
        FILES_MATCHING PATTERN "*.hpp")

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

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    include(SuilUnitTest)
    SuilUnitTest(SuilNet-UnitTest
            SOURCES ${SUIL_NETWORK_SOURCES} test/main.cpp
            LIBS    SuilBase mill ssl crypto zmq)
    set_target_properties(SuilNet-UnitTest
            PROPERTIES
            RUNTIME_OUTPUT_NAME net_unittest)
    add_dependencies(SuilNet-UnitTest SuilNet-scc)
endif()

include(example/.cmake)

if (NOT SUIL_GOD_MODE)
    set(CPACK_PACKAGE_NAME   ${CMAKE_PROJECT_NAME})
    set(CPACK_PACKAGE_VENDOR "Suilteam")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Carter Mbotho")
    set(CPACK_PACKAGE_FILE_NAME ${CMAKE_PROJECT_NAME}-${OS_VARIANT}-${CMAKE_PROJECT_VERSION})
    set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
    set(CPACK_GENERATOR "TGZ")

    include(CPack)
endif()
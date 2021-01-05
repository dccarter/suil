cmake_minimum_required(VERSION 3.10)
# The version of the library, configurable from command line
set(SUIL_BASE_VERSION "0.1.0" CACHE STRING "The version of suil-base library")

project(SuilBase VERSION ${SUIL_BASE_VERSION} LANGUAGES C CXX)

set(SUIL_BASE_SOURCES
        src/base64.cpp
        src/buffer.cpp
        src/channel.cpp
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
        src/signal.cpp
        src/string.cpp
        src/units.cpp
        src/utils.cpp
        src/url.cpp
        src/uuid.cpp
        src/union.cpp
        src/varint.cpp
        src/wire.cpp)

# Base library
add_library(SuilBase STATIC
        ${SUIL_BASE_SOURCES})

target_link_libraries(SuilBase
        ${OPENSSL_LIBRARIES} uuid lua dl)

include(CMakePackageConfigHelpers)

set(GENERATED_DIR  ${CMAKE_BINARY_DIR}/generated)
set(VERSION_CONFIG ${GENERATED_DIR}/${PROJECT_NAME}ConfigVersion.cmake)
set(PROJECT_CONFIG ${GENERATED_DIR}/${PROJECT_NAME}Config.cmake)
set(TARGETS_EXPORT_NAME  ${PROJECT_NAME}Targets)
set(NAMESPACE            suil::base::)

# Configure '<PROJECT-NAME>Config.cmake.in'
configure_package_config_file(
        cmake/Config.cmake.in
        ${PROJECT_CONFIG}
        INSTALL_DESTINATION lib/cmake/${PROJECT_NAME})

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
write_basic_package_version_file(
        "${VERSION_CONFIG}" COMPATIBILITY SameMajorVersion)

# Install base library
install(TARGETS SuilBase
        EXPORT  ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Install includes library
install(DIRECTORY include/suil/base/
        DESTINATION include/suil/base
        FILES_MATCHING PATTERN "*.hpp")

# Install config
#   * <prefix>/lib/cmake/SuilBaseConfig.cmake
#   * <prefix>/lib/cmake/SuilBaseConfigVersion.cmake
install(FILES ${PROJECT_CONFIG} ${VERSION_CONFIG}
        DESTINATION lib/cmake/${PROJECT_NAME})

# Install cmake targets
#   * <prefix>/lib/cmake/SuilBaseTargets.cmake
install(EXPORT      ${TARGETS_EXPORT_NAME}
        NAMESPACE   ${NAMESPACE}
        DESTINATION lib/cmake/${PROJECT_NAME})


include(generator/.cmake)
include(examples/.cmake)

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    include(SuilUnitTest)
    SuilUnitTest(SuilBase-UnitTest
            SOURCES ${SUIL_BASE_SOURCES} test/main.cpp
            LIBS mill ${OPENSSL_LIBRARIES} uuid lua dl)
    set_target_properties(SuilBase-UnitTest
        PROPERTIES
            RUNTIME_OUTPUT_NAME base_unittest)
endif()
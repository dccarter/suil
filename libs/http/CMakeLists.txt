cmake_minimum_required(VERSION 3.10)
# The version of the library, configurable from command line
set(SUIL_HTTP_VERSION "0.1.0" CACHE STRING "The version of suil-http library")

project(SuilHttp VERSION ${SUIL_HTTP_VERSION} LANGUAGES C CXX)

include(Server.cmake)
include(Client.cmake)

set(SUIL_HTTP_SCC_SOURCES
        scc/client.scc
        scc/jwt.scc
        scc/server.scc
        scc/route.scc)

SuilScc(SuilHttp
        PROJECT  0N
        SOURCES  ${SUIL_HTTP_SCC_SOURCES}
        LIB_PATH ${CMAKE_BINARY_DIR}
        DEPENDS  SuilBase-Generator
        OUTDIR   ${CMAKE_BINARY_DIR}/scc/public/suil/http)

add_dependencies(SuilHttpServer
        SuilHttp-scc)
add_dependencies(SuilHttpClient
        SuilHttp-scc)

include(CMakePackageConfigHelpers)

set(GENERATED_DIR  ${CMAKE_BINARY_DIR}/generated)
set(VERSION_CONFIG ${GENERATED_DIR}/${PROJECT_NAME}ConfigVersion.cmake)
set(PROJECT_CONFIG ${GENERATED_DIR}/${PROJECT_NAME}Config.cmake)
set(TARGETS_EXPORT_NAME  ${PROJECT_NAME}Targets)
set(NAMESPACE      Suil::)

# Configure '<PROJECT-NAME>Config.cmake.in'
configure_package_config_file(
        ${CMAKE_SOURCE_DIR}/cmake/Config.cmake.in
        ${PROJECT_CONFIG}
        INSTALL_DESTINATION lib/cmake/${PROJECT_NAME})

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
write_basic_package_version_file(
        "${VERSION_CONFIG}" COMPATIBILITY SameMajorVersion)

# Install base library
install(TARGETS SuilHttpServer SuilHttpClient
        EXPORT  ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Install includes library
install(DIRECTORY include/suil/http/
        DESTINATION include/suil/http
        FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_BINARY_DIR}/scc/public/suil/http/
        DESTINATION include/suil/http
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

include(example/.cmake)
cmake_minimum_required(VERSION 3.10)
# The version of the library, configurable from command line
set(SUIL_HTTP_VERSION "0.1.0" CACHE STRING "The version of suil-http library")

project(Http VERSION ${SUIL_HTTP_VERSION} LANGUAGES C CXX)

include(Server.cmake)
include(Client.cmake)

set(SUIL_HTTP_SCC_SOURCES
        scc/client.scc
        scc/jwt.scc
        scc/server.scc
        scc/route.scc)

SuilScc(Http
        PROJECT  0N
        SOURCES  ${SUIL_HTTP_SCC_SOURCES}
        LIB_PATH ${CMAKE_BINARY_DIR}
        DEPENDS  Base-Generator
        OUTDIR   ${CMAKE_BINARY_DIR}/scc/public/suil/http)

add_dependencies(HttpServer Http-scc)
add_dependencies(HttpClient Http-scc)

include(CMakePackageConfigHelpers)

# Install base library
install(TARGETS HttpServer HttpClient
        EXPORT  ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Install includes library
install(DIRECTORY include/suil/http/
        DESTINATION include/suil/http
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h")
install(DIRECTORY ${CMAKE_BINARY_DIR}/scc/public/suil/http/
        DESTINATION include/suil/http
        FILES_MATCHING PATTERN "*.hpp")

include(example/.cmake)
cmake_minimum_required(VERSION 3.10)
# The version of the library, configurable from command line
set(SUIL_DB_VERSION "0.1.0" CACHE STRING "The version of suil-db library")

project(SuilDb VERSION ${SUIL_DB_VERSION} LANGUAGES C CXX)

set(SUIL_DATABASE_SOURCES
        src/pgsql.cpp
        src/redis.cpp
        ${CMAKE_BINARY_DIR}/scc/public/suil/db/config.scc.cpp
        ${CMAKE_BINARY_DIR}/scc/public/suil/db/symbols.scc.cpp)

add_library(SuilDb STATIC
        ${SUIL_DATABASE_SOURCES})

target_link_libraries(SuilDb
    PUBLIC SuilNet PostgreSQL::PostgreSQL)

target_include_directories(SuilDb PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include
        $<INSTALL_INTERFACE:include>)
target_include_directories(SuilDb PRIVATE
        ${CMAKE_BINARY_DIR}/scc/public)

# Transpile scc sources used by database code
set(SUIL_DATABASE_SCC_SOURCES
        scc/config.scc
        scc/symbols.scc)

SuilScc(SuilDb
        SOURCES  ${SUIL_DATABASE_SCC_SOURCES}
        LIB_PATH ${CMAKE_BINARY_DIR}
        OUTDIR   ${CMAKE_BINARY_DIR}/scc/public/suil/db)
# SuilBase generator needed
add_dependencies(SuilDb-scc SuilBase-Generator)

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

# Install database library
install(TARGETS SuilDb
        EXPORT  ${TARGETS_EXPORT_NAME}
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Install includes library
install(DIRECTORY include/suil/db/
        DESTINATION include/suil/db
        FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_BINARY_DIR}/scc/public/suil/db/
        DESTINATION include/suil/db
        FILES_MATCHING PATTERN "*.hpp")

# Install config
#   * <prefix>/lib/cmake/SuilDbConfig.cmake
#   * <prefix>/lib/cmake/SuilDbConfigVersion.cmake
install(FILES ${PROJECT_CONFIG} ${VERSION_CONFIG}
        DESTINATION lib/cmake/${PROJECT_NAME})

# Install cmake targets
#   * <prefix>/lib/cmake/SuilDbTargets.cmake
install(EXPORT      ${TARGETS_EXPORT_NAME}
        NAMESPACE   ${NAMESPACE}
        DESTINATION lib/cmake/${PROJECT_NAME})

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    include(SuilUnitTest)
    SuilUnitTest(SuilDb-UnitTest
            SOURCES ${SUIL_DATABASE_SOURCES} test/main.cpp
            LIBS    SuilNet PostgreSQL::PostgreSQL)
    target_include_directories(SuilDb-UnitTest
            PRIVATE include ${CMAKE_BINARY_DIR}/scc/public)
    set_target_properties(SuilDb-UnitTest
            PROPERTIES
            RUNTIME_OUTPUT_NAME db_unittest)
    add_dependencies(SuilDb-UnitTest SuilDb-scc)
endif()

include(examples/.cmake)

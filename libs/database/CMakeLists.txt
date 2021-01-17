cmake_minimum_required(VERSION 3.10)
# The version of the library, configurable from command line
set(SUIL_DB_VERSION "0.1.0" CACHE STRING "The version of suil-db library")

project(Db VERSION ${SUIL_DB_VERSION} LANGUAGES C CXX)

set(SUIL_DATABASE_SOURCES
        src/pgsql.cpp
        src/redis.cpp
        ${CMAKE_BINARY_DIR}/scc/public/suil/db/config.scc.cpp
        ${CMAKE_BINARY_DIR}/scc/public/suil/db/symbols.scc.cpp)

add_library(Db STATIC
        ${SUIL_DATABASE_SOURCES})
add_library(Suil::Db ALIAS Db)
set_target_properties(Db
    PROPERTIES
        OUTPUT_NAME SuilDb)

target_link_libraries(Db
    PUBLIC Suil::Net PostgreSQL::PostgreSQL)

target_include_directories(Db PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
target_include_directories(Db PRIVATE
        ${CMAKE_BINARY_DIR}/scc/public)

# Transpile scc sources used by database code
set(SUIL_DATABASE_SCC_SOURCES
        scc/config.scc
        scc/symbols.scc)

SuilScc(Db
        SOURCES  ${SUIL_DATABASE_SCC_SOURCES}
        LIB_PATH ${CMAKE_BINARY_DIR}
        OUTDIR   ${CMAKE_BINARY_DIR}/scc/public/suil/db)
# Base generator needed
add_dependencies(Db-scc Base-Generator)

# Install database library
install(TARGETS Db
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

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    include(SuilUnitTest)
    SuilUnitTest(Db-UnitTest
            SOURCES ${SUIL_DATABASE_SOURCES} test/main.cpp
            LIBS    Suil::Net PostgreSQL::PostgreSQL)
    target_include_directories(Db-UnitTest
            PRIVATE include ${CMAKE_BINARY_DIR}/scc/public)
    set_target_properties(Db-UnitTest
            PROPERTIES
            RUNTIME_OUTPUT_NAME db_unittest)
    add_dependencies(Db-UnitTest Db-scc)
endif()

include(examples/.cmake)

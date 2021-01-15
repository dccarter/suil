if (ENABLE_EXAMPLES)
    # Examples demonstrating database usages
    add_executable(SuilDb-PSQLExample
            examples/postgres/main.cpp
            ${CMAKE_BINARY_DIR}/scc/private/suil/db/data.scc.cpp)
    set_target_properties(SuilDb-PSQLExample
            PROPERTIES
            RUNTIME_OUTPUT_NAME db-postgres-ex)
    target_include_directories(SuilDb-PSQLExample
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public ${CMAKE_BINARY_DIR}/scc/private)

    add_executable(SuilDb-RedisExample
            examples/redis/main.cpp
            ${CMAKE_BINARY_DIR}/scc/private/suil/db/data.scc.cpp)
    set_target_properties(SuilDb-RedisExample
            PROPERTIES
            RUNTIME_OUTPUT_NAME db-redis-ex)
    target_include_directories(SuilDb-RedisExample
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public ${CMAKE_BINARY_DIR}/scc/private)

    SuilScc(SuilDb-Examples
            PROJECT  ON
            SOURCES  examples/data.scc
            LIB_PATH ${CMAKE_BINARY_DIR}
            OUTDIR   ${CMAKE_BINARY_DIR}/scc/private/suil/db)

    target_link_libraries(SuilDb-PSQLExample
            PRIVATE SuilDb)
    target_link_libraries(SuilDb-RedisExample
            PRIVATE SuilDb)
endif()
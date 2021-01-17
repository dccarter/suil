if (ENABLE_EXAMPLES)
    # Examples demonstrating database usages
    add_executable(Db-PSQLExample
            examples/postgres/main.cpp
            ${CMAKE_BINARY_DIR}/scc/private/suil/db/data.scc.cpp)
    set_target_properties(Db-PSQLExample
            PROPERTIES
            RUNTIME_OUTPUT_NAME db-postgres-ex)
    target_include_directories(Db-PSQLExample
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public ${CMAKE_BINARY_DIR}/scc/private)

    add_executable(Db-RedisExample
            examples/redis/main.cpp
            ${CMAKE_BINARY_DIR}/scc/private/suil/db/data.scc.cpp)
    set_target_properties(Db-RedisExample
            PROPERTIES
            RUNTIME_OUTPUT_NAME db-redis-ex)
    target_include_directories(Db-RedisExample
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public ${CMAKE_BINARY_DIR}/scc/private)

    SuilScc(Db-Examples
            PROJECT  ON
            SOURCES  examples/data.scc
            LIB_PATH ${CMAKE_BINARY_DIR}
            OUTDIR   ${CMAKE_BINARY_DIR}/scc/private/suil/db)

    target_link_libraries(Db-PSQLExample
            PRIVATE Suil::Db)
    target_link_libraries(Db-RedisExample
            PRIVATE Suil::Db)
endif()
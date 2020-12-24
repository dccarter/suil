if (ENABLE_EXAMPLES)
    # Examples demonstrating database usages
    add_executable(SuilDb-PSQLExample
            examples/postgres/main.cpp
            ${CMAKE_BINARY_DIR}/scc/suil/db/data.scc.cpp)
    set_target_properties(SuilDb-PSQLExample
            PROPERTIES
            RUNTIME_OUTPUT_NAME db-postgres-ex)

    add_executable(SuilDb-RedisExample
            examples/redis/main.cpp
            ${CMAKE_BINARY_DIR}/scc/suil/db/data.scc.cpp)

    set_target_properties(SuilDb-RedisExample
            PROPERTIES
            RUNTIME_OUTPUT_NAME db-redis-ex)

    SuilScc(SuilDb-Examples
            PROJECT  ON
            SOURCES  examples/data.scc
            LIB_PATH ${CMAKE_BINARY_DIR}
            OUTDIR   ${CMAKE_BINARY_DIR}/scc/suil/db)

    target_link_libraries(SuilDb-PSQLExample SuilDb SuilNet SuilBase mill pq)
    target_link_libraries(SuilDb-RedisExample SuilDb SuilNet SuilBase mill)
endif()
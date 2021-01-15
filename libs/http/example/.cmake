if (ENABLE_EXAMPLES)
    add_executable(SuilHttpServer-Hello
            ${CMAKE_CURRENT_SOURCE_DIR}/example/basic/main.cpp)
    set_target_properties(SuilHttpServer-Hello
            PROPERTIES
            RUNTIME_OUTPUT_NAME http-server-hello)
    target_link_libraries(SuilHttpServer-Hello
            PRIVATE SuilHttpServer)
    target_include_directories(SuilHttpServer-Hello
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public)

    add_executable(SuilHttpServer-Example
            ${CMAKE_CURRENT_SOURCE_DIR}/example/server/main.cpp)
    set_target_properties(SuilHttpServer-Example
        PROPERTIES
            RUNTIME_OUTPUT_NAME http-server-ex)
    target_link_libraries(SuilHttpServer-Example
            SuilHttpServer)
    target_include_directories(SuilHttpServer-Example
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/example/server/www/index.html
                   ${CMAKE_CURRENT_BINARY_DIR}/www/index.html @ONLY)

    add_executable(SuilHttpClient-Example
            ${CMAKE_CURRENT_SOURCE_DIR}/example/client/main.cpp)
    set_target_properties(SuilHttpClient-Example
            PROPERTIES
            RUNTIME_OUTPUT_NAME http-client-ex)
    target_link_libraries(SuilHttpClient-Example
            SuilHttpClient)
    target_include_directories(SuilHttpClient-Example
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public)
endif()
if (ENABLE_EXAMPLES)
    add_executable(HttpServer-Hello
            ${CMAKE_CURRENT_SOURCE_DIR}/example/basic/main.cpp)
    set_target_properties(HttpServer-Hello
            PROPERTIES
            RUNTIME_OUTPUT_NAME http-server-hello)
    target_link_libraries(HttpServer-Hello
            PRIVATE Suil::HttpServer)
    target_include_directories(HttpServer-Hello
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public)

    add_executable(HttpServer-Example
            ${CMAKE_CURRENT_SOURCE_DIR}/example/server/main.cpp)
    set_target_properties(HttpServer-Example
        PROPERTIES
            RUNTIME_OUTPUT_NAME http-server-ex)
    target_link_libraries(HttpServer-Example
            PRIVATE Suil::HttpServer)
    target_include_directories(HttpServer-Example
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/example/server/www/index.html
                   ${CMAKE_CURRENT_BINARY_DIR}/www/index.html @ONLY)

    add_executable(HttpClient-Example
            ${CMAKE_CURRENT_SOURCE_DIR}/example/client/main.cpp)
    set_target_properties(HttpClient-Example
            PROPERTIES
            RUNTIME_OUTPUT_NAME http-client-ex)
    target_link_libraries(HttpClient-Example
            PRIVATE Suil::HttpClient)
    target_include_directories(HttpClient-Example
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public)
endif()
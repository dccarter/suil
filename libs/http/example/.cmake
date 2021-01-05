if (ENABLE_EXAMPLES)
    add_executable(SuilHttpServer-Hello
            ${CMAKE_CURRENT_SOURCE_DIR}/example/basic/main.cpp)
    set_target_properties(SuilHttpServer-Hello
            PROPERTIES
            RUNTIME_OUTPUT_NAME http-server-hello)
    target_link_libraries(SuilHttpServer-Hello
            SuilHttpServer SuilDb SuilNet SuilBase mill  ${OPENSSL_LIBRARIES})

    add_executable(SuilHttpServer-Example
            ${CMAKE_CURRENT_SOURCE_DIR}/example/server/main.cpp)
    set_target_properties(SuilHttpServer-Example
        PROPERTIES
            RUNTIME_OUTPUT_NAME http-server-ex)
    target_link_libraries(SuilHttpServer-Example
            SuilHttpServer SuilDb SuilNet SuilBase mill  ${OPENSSL_LIBRARIES})

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/example/server/www/index.html
                   ${CMAKE_CURRENT_BINARY_DIR}/www/index.html @ONLY)

    add_executable(SuilHttpClient-Example
            ${CMAKE_CURRENT_SOURCE_DIR}/example/client/main.cpp)
    set_target_properties(SuilHttpClient-Example
            PROPERTIES
            RUNTIME_OUTPUT_NAME http-client-ex)
    target_link_libraries(SuilHttpClient-Example
            SuilHttpClient SuilNet SuilBase mill  ${OPENSSL_LIBRARIES})
endif()
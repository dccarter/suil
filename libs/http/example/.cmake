if (ENABLE_EXAMPLES)
    add_executable(SuilHttpServer-Example
            ${CMAKE_CURRENT_SOURCE_DIR}/example/basic/main.cpp)
    set_target_properties(SuilHttpServer-Example
        PROPERTIES
            RUNTIME_OUTPUT_NAME http-basic-ex)
    target_link_libraries(SuilHttpServer-Example
            SuilHttpServer SuilDb SuilNet SuilBase mill  ${OPENSSL_LIBRARIES})

    add_executable(SuilHttpClient-Example
            ${CMAKE_CURRENT_SOURCE_DIR}/example/client/main.cpp)
    set_target_properties(SuilHttpClient-Example
            PROPERTIES
            RUNTIME_OUTPUT_NAME http-client-ex)
    target_link_libraries(SuilHttpClient-Example
            SuilHttpClient SuilNet SuilBase mill  ${OPENSSL_LIBRARIES})
endif()
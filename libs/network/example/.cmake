if (ENABLE_EXAMPLES)
    add_executable(SuilNet-SmtpExample
            example/smtp/main.cpp)
    set_target_properties(SuilNet-SmtpExample
        PROPERTIES
            RUNTIME_OUTPUT_NAME net-smtp-ex)
    target_link_libraries(SuilNet-SmtpExample
            PRIVATE SuilNet)
    target_include_directories(SuilNet-SmtpExample
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public)

    add_executable(SuilNet-ZmqExample
            example/zmq/main.cpp)
    set_target_properties(SuilNet-ZmqExample
            PROPERTIES
            RUNTIME_OUTPUT_NAME net-zmq-ex)
    target_link_libraries(SuilNet-ZmqExample
            PRIVATE SuilNet)
    target_include_directories(SuilNet-ZmqExample
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public)
endif()
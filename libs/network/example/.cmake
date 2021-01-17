if (ENABLE_EXAMPLES)
    add_executable(Net-SmtpExample
            example/smtp/main.cpp)
    set_target_properties(Net-SmtpExample
        PROPERTIES
            RUNTIME_OUTPUT_NAME net-smtp-ex)
    target_link_libraries(Net-SmtpExample
            PRIVATE Suil::Net)
    target_include_directories(Net-SmtpExample
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public)

    add_executable(Net-ZmqExample
            example/zmq/main.cpp)
    set_target_properties(Net-ZmqExample
            PROPERTIES
            RUNTIME_OUTPUT_NAME net-zmq-ex)
    target_link_libraries(Net-ZmqExample
            PRIVATE Suil::Net)
    target_include_directories(Net-ZmqExample
            PRIVATE ${CMAKE_BINARY_DIR}/scc/public)
endif()
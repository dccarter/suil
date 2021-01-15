if (ENABLE_EXAMPLES)
    # Examples demonstrating database usages
    add_executable(SuilBase-Example
            examples/main.cpp)
    set_target_properties(SuilBase-Example
            PROPERTIES
            RUNTIME_OUTPUT_NAME base-ex)
    target_link_libraries(SuilBase-Example
            PRIVATE SuilBase mill Threads::Threads)
endif()
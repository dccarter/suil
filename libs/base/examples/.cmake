if (ENABLE_EXAMPLES)
    # Examples demonstrating database usages
    add_executable(Base-Example
            examples/main.cpp)
    set_target_properties(Base-Example
            PROPERTIES
            RUNTIME_OUTPUT_NAME base-ex)
    target_link_libraries(Base-Example
            PRIVATE Suil::Base Threads::Threads)
endif()
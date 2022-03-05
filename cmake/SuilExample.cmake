#! SuilExample : This function is used to add an example
#  target/binary to a Suil lib
#
# \argn: a list of optional arguments
# \arg:name the name of the target/binary
# \param:RENAME the desired name of the binary
# \group:SOURCES A list of source files to build for creating the target (required)
# \group:DEPENDENCIES A list of dependency targets needed by the example target
# \group:LIBS A list of libaries required by the example target
# \group:INCLUDES A list of include directories needed by the example
#
function(SuilExample name)
    set(KV_ARGS    RENAME)
    set(GROUP_ARGS SOURCES DEPENDENCIES LIBS INCLUDES DEFINITIONS)
    cmake_parse_arguments(SUIL_EXAMPLE "" "${KV_ARGS}" "${GROUP_ARGS}" ${ARGN})
    if (NOT name)
        message(FATAL_ERROR "The name of the example target/binary is required")
    endif()
    add_executable(${name} ${SUIL_EXAMPLE_SOURCES})
    if (SUIL_EXAMPLE_DEPENDENCIES)
        add_dependencies(${name} ${SUIL_EXAMPLE_DEPENDENCIES})
    endif()
    if (SUIL_EXAMPLE_LIBS)
        target_link_libraries(${name} ${SUIL_EXAMPLE_LIBS} ${SUIL_EXAMPLE_DEPENDENCIES})
    endif()
    if (SUIL_EXAMPLE_DEFINITIONS)
        target_compile_definitions(${name} PUBLIC ${SUIL_EXAMPLE_DEFINITIONS})
    endif()
    if (SUIL_EXAMPLE_INCLUDES)
        target_include_directories(${name} PRIVATE ${SUIL_EXAMPLE_INCLUDES})
    endif()
    if (SUIL_EXAMPLE_RENAME)
        set_target_properties(${name}
                PROPERTIES
                    RUNTIME_OUTPUT_NAME ${SUIL_EXAMPLE_RENAME})
    endif()
endfunction()
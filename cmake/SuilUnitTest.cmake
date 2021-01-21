#! SuilUnitTest : This function is used to add a unit test
#  target/binary to suil projects
#
# \argn: a list of optional arguments
# \arg:name the name of the target/binary
# \group:SOURCES A list of source files to build for unit testing (required)
# \group:DEPENDENCIES A list of dependency targets needed by the testing target
# \group:LIBS A list of libaries required by the testing target
#
function(SuilUnitTest name)
    set(GROUP_ARGS SOURCES DEPENDENCIES LIBS DEFINITIONS)
    cmake_parse_arguments(SUIL_UT "" "" "${GROUP_ARGS}" ${ARGN})
    if (NOT name)
        message(FATAL_ERROR "The name of the unit tests target/binary is required")
    endif()
    add_executable(${name} ${SUIL_UT_SOURCES})
    if (SUIL_UT_DEPENDENCIES)
        add_dependencies(${name} ${SUIL_UT_DEPENDENCIES})
    endif()
    if (SUIL_UT_LIBS)
        target_link_libraries(${name} ${SUIL_UT_LIBS})
    endif()
    if (SUIL_UT_DEFINITIONS)
        target_compile_definitions(${name} PUBLIC ${SUIL_UT_DEFINITIONS})
    endif()
    # Enable unit testing
    target_compile_definitions(${name} PUBLIC suil_ut=:public SUIL_UNITTEST)
endfunction()
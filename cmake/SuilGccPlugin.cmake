#! SuilGccPluginDetect : This function is used to add a unit test
#  target/binary to SPARK projects
# testing target
# \group:LIBS A list of libaries required by the testing target
#
macro(SuilGccPluginDetect)
    # Load GCC plugin
    set(_GCC_COMPILER ${CMAKE_TARGET_C_COMPILER})
    if (NOT DEFINED CMAKE_TARGET_C_COMPILER)
        set(_GCC_COMPILER ${CMAKE_C_COMPILER})
    endif ()

    if (NOT _GCC_COMPILER)
        message(FATAL_ERROR "GCC compiler required to continue detecting plugin environment")
    endif()
    execute_process(
            COMMAND ${_GCC_COMPILER} -print-file-name=plugin
            OUTPUT_VARIABLE SXY_GCC_PLUGINS_DIR
            RESULT_VARIABLE _GCC_PLUGINS_RESULT
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (${_GCC_PLUGINS_RESULT} EQUAL 0)
        message(STATUS "Found GCC plugin directory ${SXY_GCC_PLUGINS_DIR}")
    else ()
        message(FATAL_ERROR "Could not find gcc plugin development files")
    endif ()
endmacro()

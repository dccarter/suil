#! SuilAddLibary : This function is used to configure a libary in suil directory
#
# \argn: a list of optional arguments
# \arg:name the name of the library target
# \group:SOURCES A list of source files to used to build the library
# \group:DEPENDENCIES A list of dependency targets needed by the library
# \group:LIBS A list of libaries required by library
# \group:DEFINITIONS A list compile definitions needed by the target
# \kv:RENAME the output name of the libary
#
function(SuilAddLibrary name)
    set(KV_ARGS    RENAME KIND)
    set(GROUP_ARGS SOURCES DEPENDENCIES LIBS INCLUDES DEFINITIONS OPTIONS)
    cmake_parse_arguments(SUIL_LIB "" "${KV_ARGS}" "${GROUP_ARGS}" ${ARGN})
    if (NOT name)
        message(FATAL_ERROR "The name of the library is required")
    endif()
    if (NOT SUIL_LIB_KIND)
        set(SUIL_LIB_KIND STATIC)
    endif()

    add_library(${name} ${SUIL_LIB_KIND} ${SUIL_LIB_SOURCES})
    if (SUIL_LIB_DEPENDENCIES)
        add_dependencies(${name} ${SUIL_LIB_DEPENDENCIES})
    endif()

    if (SUIL_LIB_LIBS)
        target_link_libraries(${name} ${SUIL_LIB_LIBS})
    endif()
    if (SUIL_LIB_DEFINITIONS)
        target_compile_definitions(${name} ${SUIL_LIB_DEFINITIONS})
    endif()
    if (SUIL_LIB_OPTIONS)
        target_compile_options(${name} ${SUIL_LIB_OPTIONS})
    endif()
    if (SUIL_LIB_INCLUDES)
        target_include_directories(${name} ${SUIL_LIB_INCLUDES})
    endif()
    if (SUIL_LIB_RENAME)
        set_target_properties(${name}
                PROPERTIES
                    RUNTIME_OUTPUT_NAME ${SUIL_LIB_RENAME})
    endif()

    add_library(Suil::${name} ALIAS ${name})
    # add targets include folder to public include directory
    target_include_directories(${name} PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>)

    # Install library
    install(TARGETS ${name}
            EXPORT  ${TARGETS_EXPORT_NAME}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)

    string(TOLOWER ${name} _name)
    # Install includes headers
    install(DIRECTORY include/suil/${_name}/
            DESTINATION include/suil/${_name}
            FILES_MATCHING PATTERN "*.hpp")

endfunction()
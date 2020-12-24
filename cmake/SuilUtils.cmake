include(CheckSymbolExists)
include(CheckFunctionExists)

##
# @brief Check to ensure that a system library exists in the system
# @param {name:string:required} the name of the library
# @param {INCLUDE:file} the include file to look for
# @patra {LIBRARY:name} the name of the library in the system
##
function(SuilCheckLibrary name)
    set(kvvargs INCLUDE LIBRARY)
    cmake_parse_arguments(SUIL_CHECK "" "" "${kvvargs}" ${ARGN})
    if(NOT SUIL_CHECK_INCLUDE)
        set(SUIL_CHECK_INCLUDE ${name}.h)
    endif()
    if(NOT SUIL_CHECK_LIBRARY)
        set(SUIL_CHECK_LIBRARY ${name} lib${name})
    endif()
    message(STATUS
            "finding library '${name}' inc: ${SUIL_CHECK_INCLUDE}, lib: ${SUIL_CHECK_LIBRARY}")

    find_path(${name}_INCLUDE_DIR
            NAMES ${SUIL_CHECK_INCLUDE})
    find_library(${name}_LIBRARY
            NAMES ${name}
            PATHS ${SUIL_CHECK_LIBRARY})

    message(STATUS
            "library '${name}' inc: ${${name}_INCLUDE_DIR}, lib:${${name}_LIBRARY}")

    if ((NOT ${name}_LIBRARY) OR (NOT ${name}_INCLUDE_DIR))
        message(FATAL_ERROR "${name} libary missing in build system")
    endif()
endfunction()

function(SuilCheckFunctions)
    # check and enable stack guard and dns if available
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
    set(CMAKE_REQUIRED_LIBRARIES)

    check_function_exists(mprotect MPROTECT_FOUND)
    if(MPROTECT_FOUND)
        add_definitions(-DHAVE_MPROTECT)
    endif()

    check_function_exists(posix_memalign POSIX_MEMALIGN_FOUND)
    if(POSIX_MEMALIGN_FOUND)
        add_definitions(-DHAVE_POSIX_MEMALIGN)
    endif()

    # check and enable rt if available
    list(APPEND CMAKE_REQUIRED_LIBRARIES rt)
    check_symbol_exists(clock_gettime time.h CLOCK_GETTIME_FOUND)
    if (CLOCK_GETTIME_FOUND)
        add_definitions(-DHAVE_CLOCK_GETTIME)
    endif()
endfunction()
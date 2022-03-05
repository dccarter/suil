#
#! SuilGetOsVariant This macro will return the OS variant of the system
# on which it is being executed
#
# \arg: OurVar the name of the variable to hold the
#
macro(SuilGetOsVariant OutVar)
    if (APPLE)
        set(_os_variant osx)
    elseif(UNIX)
        execute_process(COMMAND bash -c "cat /etc/os-release | grep ^ID= | awk -F= '{print $2}'"
                OUTPUT_VARIABLE _os_variant
                OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        message(WARNING "Unsupported operating system, defaulting OS variant to Ubuntu")
        set(_os_variant Ubuntu)
    endif()
    string(TOUPPER ${_os_variant} ${OutVar})
    unset(_os_variant)
    if(UNIX AND NOT APPLE)
        set(LINUX TRUE)
    endif()
endmacro()
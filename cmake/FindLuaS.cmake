# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindLuaS
-----------

Find the LuaS libraries

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``LuaS::LuaS``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``LuaS_INCLUDE_DIRS``
  where to find uuid/uuid.h, etc.
``LuaS_LIBRARIES``
  the libraries to link against to use LuaS.
``LuaS_VERSION``
  version of the LuaS library found
``LuaS_FOUND``
  TRUE if found

#]=======================================================================]

# Look for the necessary header
find_path(LuaS_INCLUDE_DIR NAMES lua.h lua.hpp)
mark_as_advanced(LuaS_INCLUDE_DIR)

# Look for the necessary library
find_library(LuaS_LIBRARY NAMES lua)
mark_as_advanced(LuaS_LIBRARY)

# Extract version information from the header file
macro(_GrabLuaVersion part)
    file(STRINGS ${LuaS_INCLUDE_DIR}/lua.h _ver_line
            REGEX "^#define LUA_VERSION_${part}"
            LIMIT_COUNT 1)
    string(REGEX MATCH "[0-9]+"
            _version_${part} "${_ver_line}")
    unset(_ver_line)
endmacro()

if(LuaS_INCLUDE_DIR)
    _GrabLuaVersion(MAJOR)
    _GrabLuaVersion(MINOR)
    _GrabLuaVersion(RELEASE)
    set(LuaS_VERSION ${_version_MAJOR}.${_version_MINOR}.${_version_RELEASE})
    unset(_version_MAJOR)
    unset(_version_MINOR)
    unset(_version_RELEASE)
endif()

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
find_package_handle_standard_args(LuaS
        REQUIRED_VARS LuaS_INCLUDE_DIR LuaS_LIBRARY
        VERSION_VAR LuaS_VERSION)

# Create the imported target
if(LuaS_FOUND)
    set(LuaS_INCLUDE_DIRS ${LuaS_INCLUDE_DIR})
    set(LuaS_LIBRARIES ${LuaS_LIBRARY})
    if(NOT TARGET LuaS::Lua)
        add_library(LuaS::Lua UNKNOWN IMPORTED)
        set_target_properties(LuaS::Lua PROPERTIES
                IMPORTED_LOCATION             "${LuaS_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${LuaS_INCLUDE_DIR}")
    endif()
endif()

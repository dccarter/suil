# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindDl
-----------

Find the Dl libraries

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Dl::Dl``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``Dl_INCLUDE_DIRS``
  where to find dlfcn.h, etc.
``Dl_LIBRARIES``
  the libraries to link against to use Dl.
``Dl_VERSION``
  version of the Dl library found
``Dl_FOUND``
  TRUE if found

#]=======================================================================]

# Look for the necessary header
find_path(Dl_INCLUDE_DIR NAMES dlfcn.h)
mark_as_advanced(Dl_INCLUDE_DIR)

# Look for the necessary library
find_library(Dl_LIBRARY NAMES dl)
mark_as_advanced(Dl_LIBRARY)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
find_package_handle_standard_args(Dl
        REQUIRED_VARS Dl_INCLUDE_DIR Dl_LIBRARY
        VERSION_VAR Dl_VERSION)

# Create the imported target
if(Dl_FOUND)
    set(Dl_INCLUDE_DIRS ${Dl_INCLUDE_DIR})
    set(Dl_LIBRARIES ${Dl_LIBRARY})
    if(NOT TARGET Dl::Dl)
        add_library(Dl::Dl UNKNOWN IMPORTED)
        set_target_properties(Dl::Dl PROPERTIES
                IMPORTED_LOCATION             "${Dl_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${Dl_INCLUDE_DIR}")
    endif()
endif()

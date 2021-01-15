# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindUuid
-----------

Find the Uuid libraries

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Uuid::Uuid``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``Uuid_INCLUDE_DIRS``
  where to find uuid/uuid.h, etc.
``Uuid_LIBRARIES``
  the libraries to link against to use Uuid.
``Uuid_VERSION``
  version of the Uuid library found
``Uuid_FOUND``
  TRUE if found

#]=======================================================================]

# Look for the necessary header
find_path(Uuid_INCLUDE_DIR NAMES uuid/uuid.h)
mark_as_advanced(Uuid_INCLUDE_DIR)

# Look for the necessary library
find_library(Uuid_LIBRARY NAMES uuid)
mark_as_advanced(Uuid_LIBRARY)

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
find_package_handle_standard_args(Uuid
        REQUIRED_VARS Uuid_INCLUDE_DIR Uuid_LIBRARY
        VERSION_VAR Uuid_VERSION)

# Create the imported target
if(Uuid_FOUND)
    set(Uuid_INCLUDE_DIRS ${Uuid_INCLUDE_DIR})
    set(Uuid_LIBRARIES ${Uuid_LIBRARY})
    if(NOT TARGET Uuid::Uuid)
        add_library(Uuid::Uuid UNKNOWN IMPORTED)
        set_target_properties(Uuid::Uuid PROPERTIES
                IMPORTED_LOCATION             "${Uuid_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${Uuid_INCLUDE_DIR}")
    endif()
endif()

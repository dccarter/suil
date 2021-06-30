# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindSecp256k1
-----------

Find the Secp 256 libraries

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Secp256k1::Secp256k1``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``Secp256k1_INCLUDE_DIRS``
  where to find secp256k1.h, etc.
``Secp256k1_LIBRARIES``
  the libraries to link against to use Secp256k1.
``Secp256k1_VERSION``
  version of the Secp256k1 library found
``Secp256k1_FOUND``
  TRUE if found

#]=======================================================================]

# Look for the necessary header
find_path(Secp256k1_INCLUDE_DIR NAMES secp256k1.h)
mark_as_advanced(Secp256k1_INCLUDE_DIR)

# Look for the necessary library
find_library(Secp256k1_LIBRARY NAMES secp256k1)
mark_as_advanced(Secp256k1_LIBRARY)

if(Secp256k1_INCLUDE_DIR)
    set(Secp256k1_VERSION 0.1.0)
endif()

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
find_package_handle_standard_args(Secp256k1
        REQUIRED_VARS Secp256k1_INCLUDE_DIR Secp256k1_LIBRARY
        VERSION_VAR Secp256k1_VERSION)

# Create the imported target
if(Secp256k1_FOUND)
    set(Secp256k1_INCLUDE_DIRS ${Secp256k1_INCLUDE_DIR})
    set(Secp256k1_LIBRARIES ${Secp256k1_LIBRARY})
    if(NOT TARGET Secp256k1::Secp256k1)
        add_library(Secp256k1::Secp256k1 UNKNOWN IMPORTED)
        set_target_properties(Secp256k1::Secp256k1 PROPERTIES
                IMPORTED_LOCATION             "${Secp256k1_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${Secp256k1_INCLUDE_DIR}")
    endif()
endif()

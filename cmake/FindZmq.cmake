# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindZmq
-----------

Find the Zmq libraries

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``Zmq::Zmq``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``Zmq_INCLUDE_DIRS``
  where to find uuid/uuid.h, etc.
``Zmq_LIBRARIES``
  the libraries to link against to use Zmq.
``Zmq_VERSION``
  version of the Zmq library found
``Zmq_FOUND``
  TRUE if found

#]=======================================================================]

# Look for the necessary header
find_path(Zmq_INCLUDE_DIR NAMES zmq.h)
mark_as_advanced(Zmq_INCLUDE_DIR)

# Look for the necessary library
find_library(Zmq_LIBRARY NAMES zmq)
mark_as_advanced(Zmq_LIBRARY)

# Extract version information from the header file
macro(_GrabZmqVersion part)
    file(STRINGS ${Zmq_INCLUDE_DIR}/zmq.h _ver_line
            REGEX "^#define ZMQ_VERSION_${part} *[0-9]+"
            LIMIT_COUNT 1)
    string(REGEX MATCH "[0-9]+"
            _version_${part} "${_ver_line}")
    unset(_ver_line)
endmacro()

if(Zmq_INCLUDE_DIR)
    _GrabZmqVersion(MAJOR)
    _GrabZmqVersion(MINOR)
    _GrabZmqVersion(PATCH)
    set(Zmq_VERSION ${_version_MAJOR}.${_version_MINOR}.${_version_PATCH})
    unset(_version_MAJOR)
    unset(_version_MINOR)
    unset(_version_PATCH)
endif()

include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
find_package_handle_standard_args(Zmq
        REQUIRED_VARS Zmq_INCLUDE_DIR Zmq_LIBRARY
        VERSION_VAR Zmq_VERSION)

# Create the imported target
if(Zmq_FOUND)
    set(Zmq_INCLUDE_DIRS ${Zmq_INCLUDE_DIR})
    set(Zmq_LIBRARIES ${Zmq_LIBRARY})
    if(NOT TARGET Zmq::Zmq)
        add_library(Zmq::Zmq UNKNOWN IMPORTED)
        set_target_properties(Zmq::Zmq PROPERTIES
                IMPORTED_LOCATION             "${Zmq_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${Zmq_INCLUDE_DIR}")
    endif()
endif()

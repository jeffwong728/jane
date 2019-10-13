# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindBZip2
---------

Try to find BZip2

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``BZip2::BZip2``, if
BZip2 has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

``BZIP2_FOUND``
  system has BZip2
``BZIP2_INCLUDE_DIRS``
  the BZip2 include directories
``BZIP2_LIBRARIES``
  Link these to use BZip2
``BZIP2_NEED_PREFIX``
  this is set if the functions are prefixed with ``BZ2_``
``BZIP2_VERSION_STRING``
  the version of BZip2 found

Cache variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``BZIP2_INCLUDE_DIR``
  the BZip2 include directory
#]=======================================================================]

find_path(BZIP2_INCLUDE_DIR bzlib.h PATHS $ENV{VCPKG_DIR} PATH_SUFFIXES include NO_DEFAULT_PATH)

if (NOT BZIP2_LIBRARIES)
    find_library(BZIP2_LIBRARY_RELEASE NAMES bz2 bzip2 PATHS $ENV{VCPKG_DIR} PATH_SUFFIXES lib NO_DEFAULT_PATH)
    find_library(BZIP2_LIBRARY_DEBUG NAMES bz2d bzip2d PATHS $ENV{VCPKG_DIR}/debug PATH_SUFFIXES lib NO_DEFAULT_PATH)

    include(SelectLibraryConfigurations)
    SELECT_LIBRARY_CONFIGURATIONS(BZIP2)
else ()
    file(TO_CMAKE_PATH "${BZIP2_LIBRARIES}" BZIP2_LIBRARIES)
endif ()

if (BZIP2_INCLUDE_DIR AND EXISTS "${BZIP2_INCLUDE_DIR}/bzlib.h")
    file(STRINGS "${BZIP2_INCLUDE_DIR}/bzlib.h" BZLIB_H REGEX "bzip2/libbzip2 version [0-9]+\\.[^ ]+ of [0-9]+ ")
    string(REGEX REPLACE ".* bzip2/libbzip2 version ([0-9]+\\.[^ ]+) of [0-9]+ .*" "\\1" BZIP2_VERSION_STRING "${BZLIB_H}")
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(BZip2
                                  REQUIRED_VARS BZIP2_LIBRARIES BZIP2_INCLUDE_DIR
                                  VERSION_VAR BZIP2_VERSION_STRING)

if (BZIP2_FOUND)
  set(BZIP2_INCLUDE_DIRS ${BZIP2_INCLUDE_DIR})
  include(CheckSymbolExists)
  include(CMakePushCheckState)
  cmake_push_check_state()
  set(CMAKE_REQUIRED_QUIET ${BZip2_FIND_QUIETLY})
  set(CMAKE_REQUIRED_INCLUDES ${BZIP2_INCLUDE_DIR})
  set(CMAKE_REQUIRED_LIBRARIES ${BZIP2_LIBRARIES})
  CHECK_SYMBOL_EXISTS(BZ2_bzCompressInit "bzlib.h" BZIP2_NEED_PREFIX)
  cmake_pop_check_state()

  if(NOT TARGET BZip2::BZip2)
    add_library(BZip2::BZip2 UNKNOWN IMPORTED)
    set_target_properties(BZip2::BZip2 PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${BZIP2_INCLUDE_DIRS}")
    set_target_properties(BZip2::BZip2 PROPERTIES INTERFACE_COMPILE_DEFINITIONS BZ_IMPORT)

    if(BZIP2_LIBRARY_RELEASE)
      set_property(TARGET BZip2::BZip2 APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE)
      set_target_properties(BZip2::BZip2 PROPERTIES
        IMPORTED_LOCATION_RELEASE "${BZIP2_LIBRARY_RELEASE}")
    endif()

    if(BZIP2_LIBRARY_DEBUG)
      set_property(TARGET BZip2::BZip2 APPEND PROPERTY
        IMPORTED_CONFIGURATIONS DEBUG)
      set_target_properties(BZip2::BZip2 PROPERTIES
        IMPORTED_LOCATION_DEBUG "${BZIP2_LIBRARY_DEBUG}")
    endif()

    if(NOT BZIP2_LIBRARY_RELEASE AND NOT BZIP2_LIBRARY_DEBUG)
      set_property(TARGET BZip2::BZip2 APPEND PROPERTY
        IMPORTED_LOCATION "${BZIP2_LIBRARY}")
    endif()
  endif()
endif ()

mark_as_advanced(BZIP2_INCLUDE_DIR)
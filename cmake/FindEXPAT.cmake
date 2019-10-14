# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindEXPAT
# ---------
#
# Find the native Expat headers and library.
#
# Imported Targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` targets:
#
# ``EXPAT::EXPAT``
#   The Expat ``expat`` library, if found.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``EXPAT_INCLUDE_DIRS``
#   where to find expat.h, etc.
# ``EXPAT_LIBRARIES``
#   the libraries to link against to use Expat.
# ``EXPAT_FOUND``
#   true if the Expat headers and libraries were found.
#

# Look for the header file.
find_path(EXPAT_INCLUDE_DIR NAMES expat.h PATHS $ENV{VCPKG_DIR} PATH_SUFFIXES include DOC "Expat include directory" NO_DEFAULT_PATH)

# Look for the library.
if(NOT EXPAT_LIBRARY)
    find_library(EXPAT_LIBRARY_RELEASE NAMES expat libexpat PATHS $ENV{VCPKG_DIR} PATH_SUFFIXES lib NO_DEFAULT_PATH)
    find_library(EXPAT_LIBRARY_DEBUG NAMES expat expatd libexpat libexpatd PATHS $ENV{VCPKG_DIR}/debug PATH_SUFFIXES lib NO_DEFAULT_PATH)
    include(SelectLibraryConfigurations)
    select_library_configurations(EXPAT)
endif()

mark_as_advanced(EXPAT_INCLUDE_DIR)

if (EXPAT_INCLUDE_DIR AND EXISTS "${EXPAT_INCLUDE_DIR}/expat.h")
    file(STRINGS "${EXPAT_INCLUDE_DIR}/expat.h" expat_version_str
         REGEX "^#[\t ]*define[\t ]+XML_(MAJOR|MINOR|MICRO)_VERSION[\t ]+[0-9]+$")

    unset(EXPAT_VERSION_STRING)
    foreach(VPART MAJOR MINOR MICRO)
        foreach(VLINE ${expat_version_str})
            if(VLINE MATCHES "^#[\t ]*define[\t ]+XML_${VPART}_VERSION[\t ]+([0-9]+)$")
                set(EXPAT_VERSION_PART "${CMAKE_MATCH_1}")
                if(EXPAT_VERSION_STRING)
                    string(APPEND EXPAT_VERSION_STRING ".${EXPAT_VERSION_PART}")
                else()
                    set(EXPAT_VERSION_STRING "${EXPAT_VERSION_PART}")
                endif()
            endif()
        endforeach()
    endforeach()
endif ()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(EXPAT
                                  REQUIRED_VARS EXPAT_LIBRARY EXPAT_INCLUDE_DIR
                                  VERSION_VAR EXPAT_VERSION_STRING)

# Copy the results to the output variables and target.
if(EXPAT_FOUND)
    set(EXPAT_INCLUDE_DIRS ${EXPAT_INCLUDE_DIR})
    if(NOT EXPAT_LIBRARIES)
        set(EXPAT_LIBRARIES ${EXPAT_LIBRARY})
    endif()

  if(NOT TARGET EXPAT::EXPAT)
      add_library(EXPAT::EXPAT UNKNOWN IMPORTED)
      set_target_properties(EXPAT::EXPAT PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${EXPAT_INCLUDE_DIRS}")

      if(EXPAT_LIBRARY_RELEASE)
        set_property(TARGET EXPAT::EXPAT APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(EXPAT::EXPAT PROPERTIES
          IMPORTED_LOCATION_RELEASE "${EXPAT_LIBRARY_RELEASE}")
      endif()

      if(EXPAT_LIBRARY_DEBUG)
        set_property(TARGET EXPAT::EXPAT APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(EXPAT::EXPAT PROPERTIES
          IMPORTED_LOCATION_DEBUG "${EXPAT_LIBRARY_DEBUG}")
      endif()

      if(NOT EXPAT_LIBRARY_RELEASE AND NOT EXPAT_LIBRARY_DEBUG)
        set_property(TARGET EXPAT::EXPAT APPEND PROPERTY
          IMPORTED_LOCATION "${EXPAT_LIBRARY}")
      endif()
  endif()
endif()

mark_as_advanced(EXPAT_INCLUDE_DIR EXPAT_LIBRARY)

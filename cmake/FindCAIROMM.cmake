find_path(CAIROMM_INCLUDE_DIR NAMES "cairomm/cairomm.h" DOC "cairomm include directory")

find_library(CAIROMM_LIBRARY_RELEASE NAMES cairomm-1.0 DOC "cairomm library release (potentially the C library)")
find_library(CAIROMM_LIBRARY_DEBUG NAMES cairomm-1.0d DOC "cairomm library debug (potentially the C library)")

include(SelectLibraryConfigurations)
select_library_configurations(CAIROMM)

mark_as_advanced(CAIROMM_INCLUDE_DIR)
mark_as_advanced(CAIROMM_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CAIROMM FOUND_VAR CAIROMM_FOUND REQUIRED_VARS CAIROMM_LIBRARY CAIROMM_INCLUDE_DIR)

include(SpamCommon)
if(CAIROMM_FOUND)
  set(CAIROMM_INCLUDE_DIRS "${CAIROMM_INCLUDE_DIR}")
  set(CAIROMM_LIBRARIES "${CAIROMM_LIBRARY}")
  spam_export_pack(CAIROMM)
  #set_target_properties(CAIROMM::CAIROMM PROPERTIES INTERFACE_COMPILE_DEFINITIONS CAIROMM_BUILDING)
endif()

set(helper_SRC
	commondef.cpp
	geom.cpp
	h5db.cpp
	splivarot.cpp

	# Headers
	commondef.h
	geom.h
	geom-curves.h
	h5db.h
	splivarot.h
)
if(MSVC)
  set_property(SOURCE geom.cpp splivarot.cpp APPEND PROPERTY COMPILE_OPTIONS "/wd4244" "/wd4267" "/wd4003" "/utf-8")
endif()

# make lib for 2geom_LIB
add_library(helper STATIC "${helper_SRC}")
set_target_properties (helper PROPERTIES FOLDER spam)

set(libs)
IF (WIN32 AND MSVC)
  list(APPEND libs hdf5::hdf5_cpp-shared)
  list(APPEND libs unofficial::glib::glib)
  list(APPEND libs 2Geom::2geom)
  list(APPEND libs wxGTK3::mono)
ELSE()
  list(APPEND libs PkgConfig::GLIB)
  list(APPEND libs 2Geom::2geom)
  include(${wxWidgets_USE_FILE})
  target_link_libraries(helper PUBLIC ${wxWidgets_LIBRARIES})
  target_include_directories(helper PUBLIC SYSTEM "${HDF5_INCLUDE_DIRS}")
  target_link_libraries(helper PUBLIC "${HDF5_LIBRARIES}" "${HDF5_HL_LIBRARIES}")
  target_compile_definitions(helper PUBLIC "${HDF5_DEFINITIONS}")
ENDIF()

if(MSVC)
  target_compile_definitions(helper PRIVATE _CRT_SECURE_NO_WARNINGS)
  target_compile_options(helper PRIVATE "/MP")
endif()

target_include_directories(helper PRIVATE ${Boost_INCLUDE_DIR})
target_include_directories(helper PRIVATE ${CMAKE_BINARY_DIR}/include)
target_link_libraries(helper PRIVATE ${libs})
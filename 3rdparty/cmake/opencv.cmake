set(CMAKE_SKIP_INSTALL_RPATH OFF CACHE BOOL "" FORCE)
set(CMAKE_SKIP_RPATH OFF CACHE BOOL "" FORCE)
set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "" FORCE)

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_STATIC_CRT OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_DEBUG_INFO ON CACHE BOOL "" FORCE)
set(USE_WIN32_FILEIO ON CACHE BOOL "" FORCE)
set(WITH_OPENMP ON CACHE BOOL "" FORCE)
set(WITH_TBB ON CACHE BOOL "" FORCE)
set(WITH_Eigen3 ON CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_opencv_legacy OFF CACHE BOOL "" FORCE)

set(OPENCV_EXTRA_MODULES_PATH "D:/apue/3rdparty/opencv_contrib-4.1.1/modules" CACHE PATH "OpenCV contrib directory" FORCE)
set(TBBROOT "D:/apue/install/tbb" CACHE PATH "TBB root directory" FORCE)
set(TBB_DIR "D:/apue/install/tbb/cmake" CACHE PATH "TBB cmake config directory" FORCE)
set(TBB_ENV_INCLUDE "D:/apue/install/tbb/include" CACHE PATH "TBB include directory" FORCE)
set(OpenBLAS_LIB "D:/apue/install/OpenBLAS/lib/libopenblas.lib" CACHE PATH "OpenBLAS lib file" FORCE)
set(Eigen3_DIR "D:/apue/install/Eigen3/share/eigen3/cmake" CACHE PATH "Eigen3 cmake config directory" FORCE)
set(Python3_ROOT_DIR "C:/Python36" CACHE PATH "Python3 root directory" FORCE)
set(Python_ADDITIONAL_VERSIONS 3.6 CACHE STRING "" FORCE)
set(CMAKE_INSTALL_PREFIX "D:/apue/install/opencv" CACHE PATH "install directory prefix" FORCE)
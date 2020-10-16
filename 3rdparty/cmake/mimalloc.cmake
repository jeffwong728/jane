set(CMAKE_SKIP_INSTALL_RPATH OFF CACHE BOOL "" FORCE)
set(CMAKE_SKIP_RPATH OFF CACHE BOOL "" FORCE)
set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "" FORCE)

set(CMAKE_INSTALL_PREFIX "$ENV{SPAM_ROOT_DIR}/install/mimalloc" CACHE PATH "" FORCE)
set(MI_BUILD_OBJECT OFF CACHE BOOL "Build object library" FORCE)
set(MI_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
set(MI_BUILD_STATIC ON CACHE BOOL "Build static library" FORCE)
set(MI_OVERRIDE OFF CACHE BOOL "Override the standard malloc interface" FORCE)
set(MI_INTERPOSE OFF CACHE BOOL "Use interpose to override standard malloc on macOS" FORCE)
set(MI_BUILD_TESTS OFF CACHE BOOL "Build test executables" FORCE)

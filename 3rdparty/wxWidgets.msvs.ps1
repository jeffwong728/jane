$env:VCPKG_DIR=$env:VCPKG_ROOT_DIR + "/installed/x64-windows"
$env:CTF=$env:VCPKG_ROOT_DIR + "/scripts/buildsystems/vcpkg.cmake"
$env:IDIR=$env:SPAM_ROOT_DIR + "/install/wxWidgets"
$env:HDIR=$env:SPAM_ROOT_DIR + "/3rdparty/wxWidgets"
$env:BDIR=$env:SPAM_ROOT_DIR + "/build/wxWidgets/msvs"

cmake --no-warn-unused-cli -DCMAKE_TOOLCHAIN_FILE:STRING=$env:CTF -DCMAKE_VERBOSE_MAKEFILE:BOOL=FALSE -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_SKIP_RPATH:BOOL=FALSE -DCMAKE_SKIP_INSTALL_RPATH:BOOL=FALSE -DCMAKE_MINSIZEREL_POSTFIX:STRING=_minsize -DCMAKE_RELWITHDEBINFO_POSTFIX:STRING=_debinfo -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=TRUE -DwxBUILD_VENDOR:STRING= -DwxBUILD_TOOLKIT:STRING=gtk3 -DwxBUILD_MONOLITHIC:BOOL=TRUE -DwxUSE_REGEX:STRING=builtin -DwxUSE_ZLIB:STRING=sys -DwxUSE_EXPAT:STRING=sys -DwxUSE_LIBJPEG:STRING=sys -DwxUSE_LIBPNG:STRING=sys -DwxUSE_LIBTIFF:STRING=sys -DwxUSE_LIBLZMA:STRING=sys -DwxUSE_STL:BOOL=TRUE -DwxUSE_STD_CONTAINERS:BOOL=TRUE -DwxBUILD_DISABLE_PLATFORM_LIB_DIR:BOOL=TRUE -DwxBUILD_SHARED:BOOL=TRUE -DwxBUILD_PRECOMP:BOOL=FALSE -DwxBUILD_SAMPLES:STRING=SOME -DHAVE_STD_UNORDERED_MAP:BOOL=TRUE -DHAVE_STD_UNORDERED_SET:BOOL=TRUE -DwxUSE_MEMORY_TRACING:BOOL=TRUE -DwxUSE_NATIVE_DATAVIEWCTRL:BOOL=FALSE -DCMAKE_INSTALL_PREFIX:STRING=$env:IDIR -S $env:HDIR -B $env:BDIR -G"Visual Studio 15 2017 Win64"
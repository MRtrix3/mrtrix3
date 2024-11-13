include(LinkerSetup)
include(FindFFTW)
include(ECMEnableSanitizers)
include(Dependencies)
include(CompilerCache)
use_compiler_cache()

add_library(mrtrix-common INTERFACE)
add_library(mrtrix::common ALIAS mrtrix-common)
target_compile_definitions(mrtrix-common INTERFACE
    MRTRIX_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    $<$<PLATFORM_ID:Windows>:MRTRIX_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:MRTRIX_MACOSX>
    $<$<PLATFORM_ID:FreeBSD>:MRTRIX_FREEBSD>
)

if(MRTRIX_STL_DEBUGGING AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling STL debug mode")
    target_compile_definitions(mrtrix-common INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:_ITERATOR_DEBUG_LEVEL=1>
        $<$<CXX_COMPILER_ID:GNU>:_GLIBCXX_DEBUG _GLIBCXX_DEBUG_PEDANTIC>
        $<$<CXX_COMPILER_ID:Clang>:_LIBCPP_DEBUG=1>
    )
endif()

if(MRTRIX_WARNINGS_AS_ERRORS)
    message(STATUS "Enabling warnings as errors")
    target_compile_options(mrtrix-common INTERFACE
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Werror>
    )
endif()

# Allow compilation of big object of files in debug mode on MINGW
if(MINGW AND CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(mrtrix-common INTERFACE -Wa,-mbig-obj)
endif()

add_subdirectory(cmd)
add_subdirectory(core)
add_subdirectory(gui)

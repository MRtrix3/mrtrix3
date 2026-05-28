include(CheckSymbolExists)
include(CheckCXXSourceCompiles)
find_package(ZLIB REQUIRED)
find_package(FFTW REQUIRED)
find_package(Git QUIET)
find_package(Threads REQUIRED)
find_package(PNG QUIET)

if(PNG_FOUND)
    message(STATUS "Found PNG: ${PNG_LIBRARIES}")
endif()

# Check to see if we can use lgamma_r() function in custom Math::betaincreg()
# The function is defined under _REENTRANT on some systems (e.g. MacOS)
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_REENTRANT)
check_symbol_exists(lgamma_r "math.h" MRTRIX_HAVE_LGAMMA_R)
list(REMOVE_AT CMAKE_REQUIRED_DEFINITIONS -1)

if(NOT MRTRIX_HAVE_LGAMMA_R)
    message(STATUS "No lgamma_r() function found; statistical inference may have poorer multi-threading performance")
endif()

# Check support for std::from_chars across every type MR::to<T>() is instantiated with.
# Integer support has been ubiquitous since C++17; floating-point support landed late in
# libc++ and in some libstdc++ versions, so probe both paths separately. When either probe
# fails, MR::to<>() falls back to the historical std::istringstream implementation for the
# unsupported family.
set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX17_STANDARD_COMPILE_OPTION}")
check_cxx_source_compiles("
#include <charconv>
#include <cstddef>
int main() {
    const char* s = \"42\";
    int i = 0;
    auto r1 = std::from_chars(s, s + 2, i);
    long l = 0;
    auto r2 = std::from_chars(s, s + 2, l);
    long long ll = 0;
    auto r3 = std::from_chars(s, s + 2, ll);
    unsigned u = 0;
    auto r4 = std::from_chars(s, s + 2, u);
    unsigned long ul = 0;
    auto r5 = std::from_chars(s, s + 2, ul);
    unsigned long long ull = 0;
    auto r6 = std::from_chars(s, s + 2, ull);
    return r1.ec == std::errc{} && r2.ec == std::errc{} && r3.ec == std::errc{} &&
           r4.ec == std::errc{} && r5.ec == std::errc{} && r6.ec == std::errc{} ? 0 : 1;
}
" MRTRIX_HAVE_FROM_CHARS_INT)
check_cxx_source_compiles("
#include <charconv>
int main() {
    const char* s = \"1.5\";
    float f = 0;
    auto r1 = std::from_chars(s, s + 3, f);
    double d = 0;
    auto r2 = std::from_chars(s, s + 3, d);
    return r1.ec == std::errc{} && r2.ec == std::errc{} ? 0 : 1;
}
" MRTRIX_HAVE_FROM_CHARS_FP)
unset(CMAKE_REQUIRED_FLAGS)

if(NOT MRTRIX_HAVE_FROM_CHARS_INT)
    message(STATUS "std::from_chars not available for integer types; MR::to<int>() will use std::istringstream fallback")
endif()
if(NOT MRTRIX_HAVE_FROM_CHARS_FP)
    message(STATUS "std::from_chars not available for floating-point types; MR::to<float|double>() will use std::istringstream fallback")
endif()

file(GLOB_RECURSE HEADLESS_SOURCES *.h *.cpp)


if(MRTRIX_ENABLE_GPU)
    find_package(Dawn CONFIG REQUIRED)
    find_package(slang CONFIG REQUIRED)
else()
    list(FILTER HEADLESS_SOURCES EXCLUDE REGEX "${CMAKE_CURRENT_LIST_DIR}/gpu/.*")
endif()

find_package(Git QUIET)

# Create version target and library
set(MRTRIX_VERSION_CPP ${CMAKE_CURRENT_BINARY_DIR}/mrtrix_version.cpp)
set(CMD_VERSION_CPP ${CMAKE_CURRENT_BINARY_DIR}/executable_version.cpp)

add_custom_target(mrtrix-executable-version-target ALL
    COMMAND ${CMAKE_COMMAND}
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -D MRTRIX_BASE_VERSION=${MRTRIX_BASE_VERSION}
        -D DST=${CMD_VERSION_CPP}
        -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/executable_version.cpp.in
        -P ${PROJECT_SOURCE_DIR}/cmake/FindVersion.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating executable_version.cpp"
    BYPRODUCTS ${CMD_VERSION_CPP}
    VERBATIM
)

add_custom_target(mrtrix-version-target ALL
    COMMAND ${CMAKE_COMMAND}
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -D MRTRIX_BASE_VERSION=${MRTRIX_BASE_VERSION}
        -D DST=${MRTRIX_VERSION_CPP}
        -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/mrtrix_version.cpp.in
        -P ${PROJECT_SOURCE_DIR}/cmake/FindVersion.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating mrtrix_version.cpp"
    BYPRODUCTS ${MRTRIX_VERSION_CPP}
    VERBATIM
)

add_library(mrtrix-version STATIC ${MRTRIX_VERSION_CPP})
add_library(mrtrix::version ALIAS mrtrix-version)
target_include_directories(mrtrix-version PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(mrtrix-version mrtrix-version-target)

add_library(mrtrix-executable-version STATIC ${CMD_VERSION_CPP})
add_library(mrtrix::executable-version ALIAS mrtrix-executable-version)
target_include_directories(mrtrix-executable-version PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(mrtrix-executable-version mrtrix-executable-version-target)


if(MRTRIX_BUILD_STATIC)
    set(MRTRIX_LIBRARY_TYPE STATIC)
else()
    set(MRTRIX_LIBRARY_TYPE SHARED)
endif()

add_library(mrtrix-core ${MRTRIX_LIBRARY_TYPE} ${HEADLESS_SOURCES})
add_library(mrtrix::core ALIAS mrtrix-core)

target_include_directories(mrtrix-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Ensure fmt include directories are added
if(TARGET fmt::fmt)
    get_target_property(fmt_include_dirs fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
    if(fmt_include_dirs)
        target_include_directories(mrtrix-core PUBLIC ${fmt_include_dirs})
    endif()
endif()

if(MRTRIX_USE_PCH)
    target_precompile_headers(mrtrix-core PRIVATE
        [["exception.h"]]
        <fstream>
        <complex>
    )
endif()

target_compile_definitions(mrtrix-core PUBLIC
    MRTRIX_BASE_VERSION="${MRTRIX_BASE_VERSION}"
    $<$<BOOL:${MRTRIX_HAVE_LGAMMA_R}>:MRTRIX_HAVE_LGAMMA_R>
    $<$<BOOL:${MRTRIX_HAVE_FROM_CHARS_INT}>:MRTRIX_HAVE_FROM_CHARS_INT>
    $<$<BOOL:${MRTRIX_HAVE_FROM_CHARS_FP}>:MRTRIX_HAVE_FROM_CHARS_FP>
)

if(APPLE AND MRTRIX_HAVE_LGAMMA_R)
    target_compile_definitions(mrtrix-core PRIVATE _REENTRANT)
endif()

if(PNG_FOUND)
    target_compile_definitions(mrtrix-core PUBLIC MRTRIX_PNG_SUPPORT)
    target_link_libraries(mrtrix-core PUBLIC PNG::PNG)
else()
    message(WARNING "libpng not found, disabling PNG support")
endif()

target_link_libraries(mrtrix-core PUBLIC
    Eigen3::Eigen
    ZLIB::ZLIB
    fftw3::fftw
    mrtrix::common
    mrtrix::version
    Threads::Threads
    nlohmann_json::nlohmann_json
    nifti::nifti
    tcb::span
    magic_enum::magic_enum
    fmt::fmt
)

if(MRTRIX_ENABLE_GPU)
    target_link_libraries(mrtrix-core PUBLIC dawn::webgpu_dawn slang::slang)
endif()

# On Windows, the libraries need to be in the same directory as the executables
if(WIN32)
    set_target_properties(mrtrix-core PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )

if(MRTRIX_ENABLE_GPU)
    add_custom_command(TARGET mrtrix-core POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:dawn::webgpu_dawn>
            $<TARGET_FILE:slang::slang>
            $<TARGET_FILE_DIR:mrtrix-core>
        COMMENT "Copying imported dependencies to output directory"
    )
endif()
endif()


if(MRTRIX_ENABLE_GPU)
    install(IMPORTED_RUNTIME_ARTIFACTS
        slang::slang
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

install(TARGETS mrtrix-core
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

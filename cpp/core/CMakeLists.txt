include(CheckSymbolExists)
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

file(GLOB_RECURSE HEADLESS_SOURCES *.h *.cpp)

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
    ${FFTW_LIBRARIES}
    mrtrix::common
    mrtrix::version
    Threads::Threads
    nlohmann_json::nlohmann_json
    nifti::nifti
)

# On Windows, the libraries need to be in the same directory as the executables
if(WIN32)
    set_target_properties(mrtrix-core PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )
endif()

install(TARGETS mrtrix-core
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

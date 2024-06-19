include(CheckSymbolExists)

file(GLOB_RECURSE CORE_SRCS *.cpp *.h)

find_package(Eigen3 REQUIRED NO_MODULE)
find_package(ZLIB REQUIRED)
find_package(FFTW REQUIRED)
find_package(Git QUIET)
find_package(Threads REQUIRED)
find_package(PNG QUIET)

# The find modules for Eigen3, and PNG don't log the location
# of the libraries, so we do it manually here
message(STATUS "Found Eigen3: ${EIGEN3_INCLUDE_DIR}")
if(PNG_FOUND)
    message(STATUS "Found PNG: ${PNG_LIBRARIES}")
endif()

add_library(mrtrix-core SHARED ${CORE_SRCS})
add_library(mrtrix::core ALIAS mrtrix-core)

# Check to see if we can use lgamma_r() function in custom Math::betaincreg()
# The function is defined under _REENTRANT on some systems (e.g. MacOS)
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_REENTRANT)
check_symbol_exists(lgamma_r "math.h" MRTRIX_HAVE_LGAMMA_R)
list(REMOVE_AT CMAKE_REQUIRED_DEFINITIONS -1)

if(NOT MRTRIX_HAVE_LGAMMA_R)
    message(STATUS "No lgamma_r() function found; statistical inference may have poorer multi-threading performance")
endif()

# On Windows, the library need to be in the same directory as the executables
if(WIN32)
    set_target_properties(mrtrix-core PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )
endif()

set(CORE_VERSION_CPP ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)

add_custom_target(core-version-target ALL
    COMMAND ${CMAKE_COMMAND}
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -D MRTRIX_BASE_VERSION=${MRTRIX_BASE_VERSION}
        -D DST=${CMAKE_CURRENT_BINARY_DIR}/version.cpp
        -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/version.cpp.in
        -P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/FindVersion.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating version.cpp for core library"
    BYPRODUCTS ${CORE_VERSION_CPP}
    VERBATIM
)

add_library(mrtrix-core-version-lib STATIC ${CORE_VERSION_CPP})
add_library(mrtrix::core-version-lib ALIAS mrtrix-core-version-lib)
add_dependencies(mrtrix-core-version-lib core-version-target)

target_include_directories(mrtrix-core-version-lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MRTRIX_USE_PCH)
    target_precompile_headers(mrtrix-core PRIVATE
        [["exception.h"]]
        <fstream>
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
    mrtrix::core-version-lib
    Threads::Threads
    ${FFTW_LIBRARIES}
)

target_include_directories(mrtrix-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} ${FFTW_INCLUDES}
)

install(TARGETS mrtrix-core
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

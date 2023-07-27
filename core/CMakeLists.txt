file(GLOB_RECURSE CORE_SRCS *.cpp *.h)

find_package(Eigen3 REQUIRED NO_MODULE)
find_package(ZLIB REQUIRED)
find_package(FFTW REQUIRED)
find_package(Git QUIET)
find_package(PNG QUIET)
find_package(TIFF QUIET)

add_library(mrtrix-core SHARED ${CORE_SRCS})
add_library(mrtrix::core ALIAS mrtrix-core)

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

target_compile_definitions(mrtrix-core PUBLIC 
    MRTRIX_BASE_VERSION="${MRTRIX_BASE_VERSION}"
)

if(PNG_FOUND)
    target_compile_definitions(mrtrix-core PUBLIC MRTRIX_PNG_SUPPORT)
    target_link_libraries(mrtrix-core PUBLIC PNG::PNG)
else()
    message(WARNING "libpng not found, disabling PNG support")
endif()

if(TIFF_FOUND)
    target_compile_definitions(mrtrix-core PUBLIC MRTRIX_TIFF_SUPPORT)
    target_link_libraries(mrtrix-core PUBLIC TIFF::TIFF)
else()
    message(WARNING "libtiff not found, disabling TIFF support")
endif()

target_link_libraries(mrtrix-core PUBLIC
    Eigen3::Eigen
    ZLIB::ZLIB
    mrtrix::core-version-lib
    ${FFTW_LIBRARIES}
)

target_include_directories(mrtrix-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${FFTW_INCLUDES})

install(TARGETS mrtrix-core 
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

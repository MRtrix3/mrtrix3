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
list(FILTER HEADLESS_SOURCES EXCLUDE REGEX "gui/.*")

file(GLOB_RECURSE GUI_SOURCES gui/*.h gui/*.cpp)

if(MRTRIX_BUILD_GUI)
    if(MRTRIX_USE_QT5)
        find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL Network REQUIRED)
        qt5_add_resources(RCC_SOURCES ${PROJECT_SOURCE_DIR}/icons/icons.qrc)
    else()
        find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL Network OpenGLWidgets REQUIRED)
        qt6_add_resources(RCC_SOURCES ${PROJECT_SOURCE_DIR}/icons/icons.qrc)
    endif()
    # Explicitly log Qt's location since it's not done automatically by the FindQt module
    message(STATUS "Qt${QT_MAJOR_VERSION}_DIR is set to: ${Qt${QT_MAJOR_VERSION}_DIR}")

    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)
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

add_library(mrtrix-headless ${MRTRIX_LIBRARY_TYPE} ${HEADLESS_SOURCES})
add_library(mrtrix::headless ALIAS mrtrix-headless)

target_include_directories(mrtrix-headless PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)

if(MRTRIX_USE_PCH)
    target_precompile_headers(mrtrix-headless PRIVATE
        [["exception.h"]]
        <fstream>
        <complex>
    )
endif()

target_compile_definitions(mrtrix-headless PUBLIC
    MRTRIX_BASE_VERSION="${MRTRIX_BASE_VERSION}"
    $<$<BOOL:${MRTRIX_HAVE_LGAMMA_R}>:MRTRIX_HAVE_LGAMMA_R>
)

if(APPLE AND MRTRIX_HAVE_LGAMMA_R)
    target_compile_definitions(mrtrix-headless PRIVATE _REENTRANT)
endif()

if(PNG_FOUND)
    target_compile_definitions(mrtrix-headless PUBLIC MRTRIX_PNG_SUPPORT)
    target_link_libraries(mrtrix-headless PUBLIC PNG::PNG)
else()
    message(WARNING "libpng not found, disabling PNG support")
endif()

target_link_libraries(mrtrix-headless PUBLIC
    Eigen3::Eigen
    ZLIB::ZLIB
    ${FFTW_LIBRARIES}
    mrtrix::common
    mrtrix::version
    mrtrix::executable-version
    Threads::Threads
    nlohmann_json::nlohmann_json
    nifti::nifti
    half::half
)

if(MRTRIX_BUILD_GUI)
    add_library(mrtrix-gui ${MRTRIX_LIBRARY_TYPE} ${GUI_SOURCES} ${RCC_SOURCES})
    add_library(mrtrix::gui ALIAS mrtrix-gui)

    set_target_properties(mrtrix-gui PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        LINK_DEPENDS_NO_SHARED ON
    )

    if(MRTRIX_USE_PCH)
        target_precompile_headers(mrtrix-gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gui/gui_pch.h)
    endif()

    target_link_libraries(mrtrix-gui PUBLIC
        mrtrix::headless
        Qt${QT_MAJOR_VERSION}::Core
        Qt${QT_MAJOR_VERSION}::Gui
        Qt${QT_MAJOR_VERSION}::Widgets
        Qt${QT_MAJOR_VERSION}::OpenGL
        Qt${QT_MAJOR_VERSION}::Network
        Threads::Threads
        ${OPENGL_LIBRARIES}
    )
    if(NOT MRTRIX_USE_QT5)
        target_link_libraries(mrtrix-gui PUBLIC Qt6::OpenGLWidgets)
    endif()
endif()

# On Windows, the libraries need to be in the same directory as the executables
if(WIN32)
    set_target_properties(mrtrix-headless PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )
    if(MRTRIX_BUILD_GUI)
        set_target_properties(mrtrix-gui PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
        )
    endif()
endif()

install(TARGETS mrtrix-headless
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(MRTRIX_BUILD_GUI)
    install(TARGETS mrtrix-gui
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

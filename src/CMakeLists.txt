set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE MAIN_SOURCES *.h *.cpp)

find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL Network REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(Git QUIET)

# Create version target and library
set(EXEC_VERSION_CPP ${CMAKE_CURRENT_BINARY_DIR}/exec_version.cpp)

add_custom_target(exec-version-target ALL
    COMMAND ${CMAKE_COMMAND} 
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -D MRTRIX_BASE_VERSION=${MRTRIX_BASE_VERSION}
        -D DST=${EXEC_VERSION_CPP}
        -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/exec_version.cpp.in
        -P ${CMAKE_SOURCE_DIR}/cmake/FindVersion.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating exec_version.cpp for executables"
    BYPRODUCTS ${EXEC_VERSION_CPP}
    VERBATIM
)

add_library(exec-version-lib STATIC ${EXEC_VERSION_CPP})
add_dependencies(exec-version-lib exec-version-target)

target_include_directories(exec-version-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

qt5_add_resources(RCC_SOURCES ${CMAKE_SOURCE_DIR}/icons/icons.qrc)

add_library(mrtrix-main SHARED ${MAIN_SOURCES} ${RCC_SOURCES})
add_library(mrtrix::main ALIAS mrtrix-main)

target_link_libraries(mrtrix-main PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::OpenGL
    Qt5::Network
    mrtrix::core
    Threads::Threads
    ${OPENGL_LIBRARIES}
)

target_include_directories(mrtrix-main PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

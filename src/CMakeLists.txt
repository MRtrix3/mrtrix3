file(GLOB_RECURSE HEADLESS_SOURCES *.h *.cpp)
list(FILTER HEADLESS_SOURCES EXCLUDE REGEX "gui/.*")

file(GLOB_RECURSE GUI_SOURCES gui/*.h gui/*.cpp)

if(BUILD_GUI)
    if(USE_QT6)
        find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL Network OpenGLWidgets REQUIRED)
        qt6_add_resources(RCC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../icons/icons.qrc)
    else()
        find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL Network REQUIRED)
        qt5_add_resources(RCC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../icons/icons.qrc)
    endif()
    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)
endif()

find_package(Git QUIET)

# Create version target and library
set(EXEC_VERSION_CPP ${CMAKE_CURRENT_BINARY_DIR}/exec_version.cpp)

add_custom_target(exec-version-target ALL
    COMMAND ${CMAKE_COMMAND} 
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -D MRTRIX_BASE_VERSION=${MRTRIX_BASE_VERSION}
        -D DST=${EXEC_VERSION_CPP}
        -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/exec_version.cpp.in
        -P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/FindVersion.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating exec_version.cpp for executables"
    BYPRODUCTS ${EXEC_VERSION_CPP}
    VERBATIM
)

add_library(mrtrix-exec-version-lib STATIC ${EXEC_VERSION_CPP})
add_library(mrtrix::exec-version-lib ALIAS mrtrix-exec-version-lib)
add_dependencies(mrtrix-exec-version-lib exec-version-target)

add_library(mrtrix-headless SHARED ${HEADLESS_SOURCES})
add_library(mrtrix::headless ALIAS mrtrix-headless)

# On Windows, the library need to be in the same directory as the executables
if(WIN32)
    set_target_properties(mrtrix-headless PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )
endif()

target_include_directories(mrtrix-exec-version-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(mrtrix-headless PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mrtrix-headless PUBLIC
    mrtrix::core
)

if(BUILD_GUI)
    add_library(mrtrix-gui SHARED ${GUI_SOURCES} ${RCC_SOURCES})
    add_library(mrtrix::gui ALIAS mrtrix-gui)

    set_target_properties(mrtrix-gui PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
    )

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
    if(USE_QT6)
        target_link_libraries(mrtrix-gui PUBLIC Qt6::OpenGLWidgets)
    endif()
endif()

install(TARGETS mrtrix-headless
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(BUILD_GUI)
    install(TARGETS mrtrix-gui
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

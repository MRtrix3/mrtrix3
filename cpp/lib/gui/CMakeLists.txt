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

file(GLOB_RECURSE GUI_SOURCES *.h *.cpp)

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

target_include_directories(mrtrix-gui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mrtrix-gui PUBLIC
    mrtrix::core
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

set_target_properties(mrtrix-gui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

install(TARGETS mrtrix-gui
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
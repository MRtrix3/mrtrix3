#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)

#find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL Network REQUIRED)
#find_package(OpenGL REQUIRED)
#find_package(Threads REQUIRED)

#qt5_add_resources(RCC_SOURCES ${CMAKE_SOURCE_DIR}/icons/icons.qrc)

#add_library(mrtrix-main STATIC ${MAIN_SOURCES} ${RCC_SOURCES})
#add_library(mrtrix::main ALIAS mrtrix-main)

#target_link_libraries(mrtrix-main PUBLIC
#Qt5::Core
#Qt5::Gui
#Qt5::Widgets
#Qt5::OpenGL
#Qt5::Network
#mrtrix::core
#Threads::Threads
#${OPENGL_LIBRARIES}
#)
#

include (objects.cmake)


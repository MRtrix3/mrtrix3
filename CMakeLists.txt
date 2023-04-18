# TODO: Figure what's the minimum CMake we want
cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

if(NOT DEFINED ${CMAKE_TOOLCHAIN_FILE})
    set(CMAKE_TOOLCHAIN_FILE 
        ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
        CACHE STRING "Vcpkg toolchain file")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

project(mrtrix3)

# TODO: Are these required for the commands?
# If not, we shouldn't add them globally

add_compile_definitions(
    MRTRIX_BUILD_TYPE="release" #TODO: Use CMAKE_BUILD_TYPE
    $<$<PLATFORM_ID:Windows>:MRTRIX_WINDOWS>
)

add_subdirectory(core)
add_subdirectory(src)

# TODO: Should commands be built in a separate project or subdirectory?
file(GLOB_RECURSE CMD_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/cmd/*.cpp)

foreach(CMD ${CMD_SRCS})
    # Extract the filename without an extension (NAME_WE)
    get_filename_component(CMD_NAME ${CMD} NAME_WE)
    add_executable(${CMD_NAME} ${CMD})
    target_link_libraries(${CMD_NAME} PRIVATE mrtrix::main)
endforeach(CMD)

# Install the commands in the bin folder
foreach(CMD ${CMD_SRCS})
    get_filename_component(CMD_NAME ${CMD} NAME_WE)
    install(TARGETS ${CMD_NAME} RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/Installation/bin)
endforeach(CMD)

# Copy python files to bin
file(GLOB_RECURSE PYTHON_FILES ${CMAKE_CURRENT_SOURCE_DIR}/Installation/bin)

foreach(PYTHON_CMD ${PYTHON_FILES})
    install(TARGETS ${PYTHON_CMD} RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/Installation/bin)
endforeach(PYTHON_CMD)

set(GUI_CMD_SRCS mrview.cpp shview.cpp)

file(GLOB HEADLESS_CMD_SRCS *.cpp)
foreach(CMD ${GUI_CMD_SRCS})
    list(REMOVE_ITEM HEADLESS_CMD_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/${CMD})
endforeach(CMD)

if(MRTRIX_USE_PCH)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/pch_cmd.cpp "int main(){}")
    add_executable(pch_cmd ${CMAKE_CURRENT_BINARY_DIR}/pch_cmd.cpp)
    target_include_directories(pch_cmd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../core)
    find_package(Eigen3 REQUIRED)
    target_link_libraries(pch_cmd PRIVATE Eigen3::Eigen)
    target_precompile_headers(pch_cmd PRIVATE
        [["app.h"]]
        [["image.h"]]
        <complex>
        <string>
        <vector>
    )
endif()

set(HEAVY_CMDS mrregister mrtransform)

function(add_cmd CMD_SRC IS_GUI)
    # Extract the filename without an extension (NAME_WE)
    get_filename_component(CMD_NAME ${CMD_SRC} NAME_WE)
    add_executable(${CMD_NAME} ${CMD_SRC})
    target_link_libraries(${CMD_NAME} PRIVATE
        $<IF:$<BOOL:${IS_GUI}>,mrtrix::gui,mrtrix::headless>
        mrtrix::exec-version-lib
    )
    set_target_properties(${CMD_NAME} PROPERTIES
        LINK_DEPENDS_NO_SHARED true
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )
    if(MRTRIX_USE_PCH AND NOT ${IS_GUI})
	target_precompile_headers(${CMD_NAME} REUSE_FROM pch_cmd)
    endif()
    install(TARGETS ${CMD_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()


foreach(CMD ${HEADLESS_CMD_SRCS})
    add_cmd(${CMD} FALSE)
endforeach(CMD)

# For the set of commands that take the longest to compile,
# we try to start their compilation as soon as possible by adding them as dependencies
# of a generated dummy file (which Ninja prioritises during compilation).
# This is hacky, but it increases build parallelism.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp "int main(){}")
add_executable(0_dummy ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp)
add_dependencies(0_dummy ${HEAVY_CMDS})

if(MRTRIX_BUILD_GUI)
    foreach(CMD ${GUI_CMD_SRCS})
        add_cmd(${CMD} TRUE)
    endforeach(CMD)
endif()

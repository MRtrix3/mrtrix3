set(GUI_CMD_SRCS mrview.cpp shview.cpp)

file(GLOB HEADLESS_CMD_SRCS *.cpp)
foreach(CMD ${GUI_CMD_SRCS})
    list(REMOVE_ITEM HEADLESS_CMD_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/${CMD})
endforeach(CMD)


function(add_cmd CMD_SRC IS_GUI)
    # Extract the filename without an extension (NAME_WE)
    get_filename_component(CMD_NAME ${CMD} NAME_WE)
    add_executable(${CMD_NAME} ${CMD})
    target_link_libraries(${CMD_NAME} PRIVATE 
        $<IF:$<BOOL:${IS_GUI}>,mrtrix::gui,mrtrix::headless>
        mrtrix::exec-version-lib
    )
    set_target_properties(${CMD_NAME} PROPERTIES 
        LINK_DEPENDS_NO_SHARED true
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )
    install(TARGETS ${CMD_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()


foreach(CMD ${HEADLESS_CMD_SRCS})
    add_cmd(${CMD} FALSE)
endforeach(CMD)

if(BUILD_GUI)
    foreach(CMD ${GUI_CMD_SRCS})
        add_cmd(${CMD} TRUE)
    endforeach(CMD)
endif()
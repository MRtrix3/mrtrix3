file(GLOB CMD_SRCS *.cpp)
file(GLOB BIN_SCRIPTS ${CMAKE_SOURCE_DIR}/bin/*)

foreach(CMD ${CMD_SRCS})
    # Extract the filename without an extension (NAME_WE)
    get_filename_component(CMD_NAME ${CMD} NAME_WE)
    add_executable(${CMD_NAME} ${CMD})
    target_link_libraries(${CMD_NAME} PRIVATE mrtrix::main)
    install(TARGETS ${CMD_NAME} RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/Installation/bin)
endforeach(CMD)


install(FILES ${BIN_SCRIPTS} DESTINATION ${CMAKE_SOURCE_DIR}/Installation/bin)

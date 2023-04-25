file(GLOB CMD_SRCS *.cpp)

foreach(CMD ${CMD_SRCS})
    # Extract the filename without an extension (NAME_WE)
    get_filename_component(CMD_NAME ${CMD} NAME_WE)
    add_executable(${CMD_NAME} ${CMD})
    target_link_libraries(${CMD_NAME} PRIVATE mrtrix::main)
    install(TARGETS ${CMD_NAME} RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/Installation/bin)
endforeach(CMD)

# Copy python files to bin
file(GLOB_RECURSE PYTHON_FILES ${CMAKE_SOURCE_DIR}/bin)

foreach(PYTHON_CMD ${PYTHON_FILES})
    install(TARGETS ${PYTHON_CMD} RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/Installation/bin)
endforeach(PYTHON_CMD)

add_custom_target(copy-share-data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mrtrix3/ ${CMAKE_CURRENT_BINARY_DIR}/mrtrix3
    COMMENT "Copying share data files to build tree"
    VERBATIM
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mrtrix3/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/mrtrix3
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons/hicolor
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/applications/mrview.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
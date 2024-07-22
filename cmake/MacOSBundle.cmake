function(set_bundle_properties executable_name)
    set(icon_files ${CMAKE_CURRENT_SOURCE_DIR}/../icons/macos/${executable_name}.icns)
    if(${executable_name} STREQUAL "mrview")
        list(APPEND icon_files ${CMAKE_CURRENT_SOURCE_DIR}/../icons/macos/mrview_doc.icns)
    endif()

    string(TIMESTAMP CURRENT_YEAR "%Y")
    set(COPYRIGHT_YEAR "2008-${CURRENT_YEAR}" CACHE STRING "Copyright year")

    target_sources(${executable_name} PRIVATE ${icon_files})
    set_target_properties(${executable_name} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../packaging/macos/bundle/${executable_name}.plist.in"
        RESOURCE "${icon_files}"
        INSTALL_RPATH "@executable_path/../../../../lib"
    )
endfunction()

function(install_bundle_wrapper_scripts executable_name)
    set(wrapper_script ${CMAKE_CURRENT_SOURCE_DIR}/../packaging/macos/bundle/wrapper_launcher.sh.in)
    configure_file(${wrapper_script} ${PROJECT_BINARY_DIR}/bin/${executable_name}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        @ONLY
    )

    install(FILES ${PROJECT_BINARY_DIR}/bin/${executable_name}
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            RENAME "${executable_name}"
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

endfunction()

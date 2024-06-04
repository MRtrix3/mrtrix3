function(set_bundle_properties executable_name)
    if(${executable_name} STREQUAL "mrview")
        set(mrtrix_icon_macos ${CMAKE_CURRENT_SOURCE_DIR}/../icons/macos/mrview_doc.icns)
    else()
        set(mrtrix_icon_macos ${CMAKE_CURRENT_SOURCE_DIR}/../icons/macos/${executable_name}.icns)
    endif()

    string(TIMESTAMP CURRENT_YEAR "%Y")
    set(COPYRIGHT_YEAR "2008-${CURRENT_YEAR}" CACHE STRING "Copyright year")

    target_sources(${executable_name} PRIVATE ${mrtrix_icon_macos})
    set_target_properties(${executable_name} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../packaging/macos/bundle/${executable_name}.plist.in"
        RESOURCE ${mrtrix_icon_macos}
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

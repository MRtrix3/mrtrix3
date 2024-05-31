function(set_bundle_properties executable_name)
    if(${executable_name} STREQUAL "mrview")
        set(mrtrix_icon_macos ${CMAKE_CURRENT_SOURCE_DIR}/../icons/macos/mrview_doc.icns)
    else()
        set(mrtrix_icon_macos ${CMAKE_CURRENT_SOURCE_DIR}/../icons/macos/${executable_name}.icns)
    endif()

    target_sources(${executable_name} PRIVATE ${mrtrix_icon_macos})
    set_target_properties(${executable_name} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../packaging/macos/bundle/${executable_name}.plist.in"
        RESOURCE ${mrtrix_icon_macos}
        INSTALL_RPATH "@executable_path/../../../../lib"
    )
endfunction()

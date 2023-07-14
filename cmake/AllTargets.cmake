function(get_all_targets var)
    set(targets)
    get_all_targets_recursive(targets ${CMAKE_SOURCE_DIR})
    set(${var} ${targets} CACHE INTERNAL "")
endfunction()

macro(get_all_targets_recursive targets dir)
    get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
    foreach(subdir ${subdirectories})
        message("${subdir}")
        get_all_targets_recursive(${targets} ${subdir})
    endforeach()


    get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
    list(APPEND ${targets} ${current_targets})
endmacro()

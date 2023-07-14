add_subdirectory(binaries)
add_subdirectory(cmd)
add_subdirectory(src)

include(AllTargets)
get_all_targets(all_targets)

message(STATUS "MRtrix3 project targets : ${all_targets}")

include(FetchDawn)

add_library(mrtrix-gpu-lib STATIC
    gpu.h gpu.cpp
    utils.h utils.cpp
    wgslprocessing.h wgslprocessing.cpp
    match.h
    span.h
)

target_link_libraries(mrtrix-gpu-lib PUBLIC
    dawncpp_headers
    webgpu_dawn
    mrtrix::core
)

target_include_directories(mrtrix-gpu-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MRTRIX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(testing)
endif()


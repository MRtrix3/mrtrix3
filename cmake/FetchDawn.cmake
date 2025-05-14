# Adapted from https://github.com/eliemichel/WebGPU-distribution/blob/dawn/cmake/FetchDawn.cmake

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(
    dawn
    DOWNLOAD_COMMAND
            cd ${FETCHCONTENT_BASE_DIR}/dawn-src &&
            git init &&
            git fetch --depth=1 https://dawn.googlesource.com/dawn chromium/7060 &&
            git reset --hard FETCH_HEAD
)

FetchContent_GetProperties(dawn)
if (NOT dawn_POPULATED)
    FetchContent_Populate(dawn)
    if (APPLE)
        set(USE_VULKAN OFF)
        set(USE_METAL ON)
    else()
        set(USE_VULKAN ON)
        set(USE_METAL OFF)
    endif()

    set(DAWN_FETCH_DEPENDENCIES ON)
    set(DAWN_USE_GLFW OFF)
    set(DAWN_USE_X11 OFF)
    set(DAWN_USE_WAYLAND OFF)
    set(DAWN_ENABLE_D3D11 OFF)
    set(DAWN_ENABLE_D3D12 OFF)
    set(DAWN_ENABLE_METAL ${USE_METAL})
    set(DAWN_ENABLE_NULL OFF)
    set(DAWN_ENABLE_DESKTOP_GL OFF)
    set(DAWN_ENABLE_OPENGLES OFF)
    set(DAWN_ENABLE_VULKAN ${USE_VULKAN})
    set(TINT_BUILD_SPV_READER OFF)
    set(DAWN_BUILD_SAMPLES OFF)
    set(TINT_BUILD_CMD_TOOLS OFF)
    set(TINT_BUILD_TESTS OFF)
    set(TINT_BUILD_IR_BINARY OFF)

    add_subdirectory(${dawn_SOURCE_DIR} ${dawn_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

set(FETCHCONTENT_QUIET ON)

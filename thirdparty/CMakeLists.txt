include(FetchContent)

FetchContent_Declare(json
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(eigen3
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
    # We patch Eigen to remove unnecessary configuration messages
    PATCH_COMMAND git apply --ignore-space-change --ignore-whitespace ${CMAKE_CURRENT_SOURCE_DIR}/eigen.patch
    UPDATE_DISCONNECTED ON # Avoid applying the patch once again
)
FetchContent_MakeAvailable(eigen3)



add_library(nifti INTERFACE)
add_library(nifti::nifti ALIAS nifti)
target_include_directories(nifti INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/nifti)

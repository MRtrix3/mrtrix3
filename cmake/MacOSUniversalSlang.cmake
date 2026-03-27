# CMake module to prepare a universal binary version of the Slang library for macOS.
include(FetchContent)

function(mrtrix_lipo_merge input_x86_64 input_arm64 output_file)
    find_program(MRTRIX_LIPO_EXECUTABLE lipo REQUIRED)

    get_filename_component(output_dir "${output_file}" DIRECTORY)
    file(MAKE_DIRECTORY "${output_dir}")

    execute_process(
        COMMAND "${MRTRIX_LIPO_EXECUTABLE}" -create "${input_x86_64}" "${input_arm64}" -output "${output_file}"
        RESULT_VARIABLE lipo_result
        ERROR_VARIABLE lipo_error
    )

    if(NOT lipo_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to create universal binary '${output_file}' from '${input_x86_64}' and '${input_arm64}':\n"
            "${lipo_error}"
        )
    endif()
endfunction()

function(mrtrix_prepare_universal_slang slang_version out_var)
    set(slang_url_base "https://github.com/shader-slang/slang/releases/download/v${slang_version}")

    FetchContent_Declare(
        slang_x86_64
        DOWNLOAD_NO_PROGRESS 1
        URL "${slang_url_base}/slang-${slang_version}-macos-x86_64.zip"
    )
    FetchContent_Declare(
        slang_aarch64
        DOWNLOAD_NO_PROGRESS 1
        URL "${slang_url_base}/slang-${slang_version}-macos-aarch64.zip"
    )
    FetchContent_MakeAvailable(slang_x86_64 slang_aarch64)

    set(staged_dir "${CMAKE_BINARY_DIR}/_deps/slang-universal")
    file(REMOVE_RECURSE "${staged_dir}")

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${slang_aarch64_SOURCE_DIR}" "${staged_dir}"
        RESULT_VARIABLE copy_result
        ERROR_VARIABLE copy_error
    )

    if(NOT copy_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to stage universal Slang package from '${slang_aarch64_SOURCE_DIR}' to '${staged_dir}':\n"
            "${copy_error}"
        )
    endif()

    set(slang_macho_files
        bin/slangc
        bin/slangd
        bin/slangi
    )
    file(GLOB slang_dylibs RELATIVE "${slang_x86_64_SOURCE_DIR}" "${slang_x86_64_SOURCE_DIR}/lib/*.dylib")
    list(APPEND slang_macho_files ${slang_dylibs})
    list(REMOVE_DUPLICATES slang_macho_files)

    foreach(relpath IN LISTS slang_macho_files)
        set(input_x86_64 "${slang_x86_64_SOURCE_DIR}/${relpath}")
        set(input_arm64 "${slang_aarch64_SOURCE_DIR}/${relpath}")

        if(EXISTS "${input_x86_64}" AND EXISTS "${input_arm64}")
            mrtrix_lipo_merge("${input_x86_64}" "${input_arm64}" "${staged_dir}/${relpath}")
        endif()
    endforeach()

    set(${out_var} "${staged_dir}/lib/cmake/slang" PARENT_SCOPE)
endfunction()
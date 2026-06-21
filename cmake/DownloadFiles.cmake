# Script-mode helper that downloads a fixed subset of files from a common base URL.
#
# Intended to be invoked via "cmake -P" as the DOWNLOAD_COMMAND of an
# ExternalProject so that the files are fetched at build time (only when tests
# are built) rather than at configure time. Required variables, passed on the
# command line with -D:
#   BASE_URL : URL prefix shared by every file (no trailing slash)
#   FILES    : "|"-separated list of file names to append to BASE_URL
#   DEST_DIR : directory into which the files are written (created if absent)
#
# Each file is fetched only when not already present, so a partially-completed
# download step resumes cheaply on the next build without re-fetching whole files.

if(NOT DEFINED BASE_URL OR NOT DEFINED FILES OR NOT DEFINED DEST_DIR)
    message(FATAL_ERROR "DownloadFiles.cmake requires BASE_URL, FILES and DEST_DIR")
endif()

file(MAKE_DIRECTORY ${DEST_DIR})

# FILES is delimited with "|" rather than the CMake list separator ";" so that it
# survives transit as a single command-line argument.
string(REPLACE "|" ";" file_list "${FILES}")

foreach(file_name IN LISTS file_list)
    set(target "${DEST_DIR}/${file_name}")
    if(EXISTS ${target})
        continue()
    endif()
    set(url "${BASE_URL}/${file_name}")
    message(STATUS "Downloading ${url}")
    file(DOWNLOAD ${url} ${target} STATUS download_status)
    list(GET download_status 0 download_code)
    if(NOT download_code EQUAL 0)
        list(GET download_status 1 download_message)
        # Remove the truncated artefact so a re-run does not treat it as complete.
        file(REMOVE ${target})
        message(FATAL_ERROR "Failed to download ${url}: ${download_message}")
    endif()
endforeach()

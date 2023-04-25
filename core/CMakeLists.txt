file(GLOB_RECURSE CORE_SRCS *.cpp *.h)

add_library(mrtrix-core OBJECT ${CORE_SRCS})
add_library(mrtrix::core ALIAS mrtrix-core)

find_package(Eigen3 REQUIRED)
find_package(ZLIB REQUIRED)

if(WIN32)
    find_package(FFTW3 CONFIG REQUIRED)
endif()

target_link_libraries(mrtrix-core PUBLIC
    Eigen3::Eigen
    ZLIB::ZLIB
    $<IF:$<PLATFORM_ID:Windows>,FFTW3::fftw3,fftw3>
)

target_include_directories(mrtrix-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
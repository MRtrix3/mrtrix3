file(GLOB_RECURSE CORE_SRCS *.cpp *.h)

add_library(mrtrix-core SHARED ${CORE_SRCS})
add_library(mrtrix::core ALIAS mrtrix-core)

find_package(Eigen3 REQUIRED NO_MODULE)
find_package(ZLIB REQUIRED)
find_package(FFTW REQUIRED)

target_link_libraries(mrtrix-core PUBLIC
    Eigen3::Eigen
    ZLIB::ZLIB
)

target_include_directories(mrtrix-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS mrtrix-core 
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

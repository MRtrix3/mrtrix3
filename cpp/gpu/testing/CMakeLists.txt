include(FetchContent)
set(GTEST_VERSION 1.16.0)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v${GTEST_VERSION}/googletest-${GTEST_VERSION}.tar.gz
)
FetchContent_MakeAvailable(googletest)

add_executable(mrtrix-gpu-tests
    gputests_common.h
    gputests.cpp
)

target_compile_definitions(mrtrix-gpu-tests PRIVATE
    # To avoid conflicts with MRtrix's FAIL macro in exception.h
    GTEST_DONT_DEFINE_FAIL
)
target_link_libraries(mrtrix-gpu-tests
    GTest::gtest
    mrtrix-gpu-lib
)

set(TEST_SHADER_SOURCE "test_shader.wgsl")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SHADER_SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/${TEST_SHADER_SOURCE} COPYONLY)


include(GoogleTest)
gtest_discover_tests(mrtrix-gpu-tests)

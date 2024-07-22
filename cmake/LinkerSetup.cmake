# On Linux we want to use the LLVM linker if available as it is faster than the default GNU linker.
# Unfortunately, lld is not able to perform LTO when compiling with GCC.

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    set(GCC_LTO TRUE)
else()
    set(GCC_LTO FALSE)
endif()

if(UNIX AND NOT APPLE AND NOT GCC_LTO)
  find_program(LLVM_LINKER NAMES "ld.lld")

  if(LLVM_LINKER)
    message(STATUS "Using LLVM linker: ${LLVM_LINKER}")

    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.29)
      set(CMAKE_LINKER_TYPE "LLD" CACHE STRING "Linker type")
    else()
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
    endif()

  endif()
endif()

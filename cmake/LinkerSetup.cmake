# On Linux we want to use the LLVM linker if available as it is faster than the default GNU linker.

if(UNIX AND NOT APPLE)
  find_program(LLVM_LINKER NAMES "ld.lld")

  if(LLVM_LINKER)
    message(STATUS "Using LLVM linker: ${LLVM_LINKER}")

    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.29)
      set(CMAKE_LINKER_TYPE "LLD" CACHE STRING "Linker type")
    else()
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld-lld")
      set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
    endif()

  endif()
endif()

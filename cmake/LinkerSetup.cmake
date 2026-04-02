# The LLVM linker is faster than the default GNU linker.
# Unfortunately, lld is not able to perform LTO when compiling with GCC.
if(MRTRIX_USE_LLD)
    include(CheckCXXCompilerFlag)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_INTERPROCEDURAL_OPTIMIZATION)
        set(GCC_LTO TRUE)
        message(WARNING "LLD cannot be used with GCC when LTO is enabled. Please another compiler or disable LTO.")
    else()
        set(GCC_LTO FALSE)
    endif()

    if(NOT GCC_LTO)
      set(LINKER_FLAG "-fuse-ld=lld")
      check_cxx_compiler_flag(${LINKER_FLAG} CXX_SUPPORTS_LLVM_LINKER)

      if(CXX_SUPPORTS_LLVM_LINKER)
        find_program(LLVM_LINKER NAMES lld ld.lld)
        message(STATUS "Using LLVM linker: ${LLVM_LINKER}")

        if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.29)
          set(CMAKE_LINKER_TYPE "LLD" CACHE STRING "Linker type")
        else()
          set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
          set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
          set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
        endif()
      else()
        message(WARNING "Compiler does not support LLVM linker. Using default linker.")
      endif()
    endif()
endif()


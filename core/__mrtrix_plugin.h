#ifndef __MRTRIX_MAX_ALIGN_T_DEFINED
#define __MRTRIX_MAX_ALIGN_T_DEFINED

    #ifdef MRTRIX_MAX_ALIGN_T_NEEDS_TO_BE_DEFINED
      #ifndef __mrtrix_max_align_t_defined__
      #define __mrtrix_max_align_t_defined__
     // #if defined(_MSC_VER)
     // typedef double max_align_t;
     // #elif defined(__APPLE__)
     // typedef long double max_align_t;
     // #else
     // Define 'max_align_t' to match the GCC definition.
      typedef struct { NOMEMALIGN(max_align_t)
       long long __clang_max_align_nonce1
           __attribute__((__aligned__(__alignof__(long long))));
       long double __clang_max_align_nonce2
           __attribute__((__aligned__(__alignof__(long double))));
     } max_align_t;
     // #endif
      #endif
    #endif

    #include <stddef.h>

    #ifdef MRTRIX_MAX_ALIGN_T_NOT_DEFINED
      #ifndef __mrtrix_max_align_t_not_defined__
      #define __mrtrix_max_align_t_not_defined__
      using std::max_align_t;
      #endif
    #endif

    #ifdef MRTRIX_STD_MAX_ALIGN_T_NOT_DEFINED
      #ifndef __mrtrix_std_max_align_t_not_defined__
      #define __mrtrix_std_max_align_t_not_defined__
      namespace std { using ::max_align_t; }
      #endif
    #endif
#endif

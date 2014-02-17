#ifndef __mrtrix_types_h__
#define __mrtrix_types_h__

#include <stdint.h>
#include <complex>

/*! \defgroup VLA Variable-length array macros
 *
 * The reason for defining these macros at all is that VLAs are not part of the
 * C++ standard, and not available on all compilers. Regardless of the
 * availability of VLAs, they should be avoided if possible since they run the
 * risk of overrunning the stack if the length of the array is large, or if the
 * function is called recursively. They can be used safely in cases where the
 * size of the array is expected to be small, and the function will not be
 * called recursively, and in these cases may avoid the overhead of allocation
 * that might be incurred by the use of e.g. a std::vector. 
 */

//! \{

/*! \def VLA
 * define a variable-length array (VLA) if supported by the compiler, or a
 * std::vector otherwise. This may have performance implications in the latter
 * case if this forms part of a tight loop.
 * \sa VLA_MAX
 */

/*! \def VLA_MAX
 * define a variable-length array if supported by the compiler, or a
 * fixed-length array of size \a max  otherwise. This may have performance
 * implications in the latter case if this forms part of a tight loop.
 * \note this should not be used in recursive functions, unless the maximum
 * number of calls is known to be small. Large amounts of recursion will run
 * the risk of overrunning the stack.
 * \sa VLA
 */


#ifdef MRTRIX_NO_VLA
# define VLA(name, type, num) \
  std::vector<type> __vla__ ## name(num); \
  type* name = &__vla__ ## name[0]
# define VLA_MAX(name, type, num, max) type name[max]
#else
# define VLA(name, type, num) type name[num]
# define VLA_MAX(name, type, num, max) type name[num]
#endif


/*! \def NON_POD_VLA
 * define a variable-length array of non-POD data if supported by the compiler,
 * or a std::vector otherwise. This may have performance implications in the
 * latter case if this forms part of a tight loop.  
 * \sa VLA_MAX
 */

/*! \def NON_POD_VLA_MAX
 * define a variable-length array of non-POD data if supported by the compiler,
 * or a fixed-length array of size \a max  otherwise. This may have performance
 * implications in the latter case if this forms part of a tight loop.  
 * \note this should not be used in recursive functions, unless the maximum
 * number of calls is known to be small. Large amounts of recursion will run
 * the risk of overrunning the stack.
 * \sa VLA
 */


#ifdef MRTRIX_NO_NON_POD_VLA
# define NON_POD_VLA(name, type, num) \
  std::vector<type> __vla__ ## name(num); \
  type* name = &__vla__ ## name[0]
# define NON_POD_VLA_MAX(name, type, num, max) type name[max]
#else
# define NON_POD_VLA(name, type, num) type name[num]
# define NON_POD_VLA_MAX(name, type, num, max) type name[num]
#endif

//! \}

namespace MR
{

  typedef float  float32;
  typedef double float64;
  typedef std::complex<double> cdouble;
  typedef std::complex<float> cfloat;


}

#endif


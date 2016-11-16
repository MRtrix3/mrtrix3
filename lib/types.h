/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __mrtrix_types_h__
#define __mrtrix_types_h__

#define EIGEN_DONT_PARALLELIZE

#include <stdint.h>
#include <complex>
#include <iostream>
#include <vector>

// These lines are to silence deprecation warnings with Eigen & GCC v5
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <Eigen/Geometry>
#pragma GCC diagnostic pop

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

#ifdef NDEBUG
# define FORCE_INLINE inline __attribute__((always_inline))
#else // don't force inlining in debug mode, so we can get more informative backtraces
# define FORCE_INLINE inline
#endif


#ifndef EIGEN_DEFAULT_ALIGN_BYTES
// Assume 16 byte alignment as hard-coded in Eigen 3.2:
# define EIGEN_DEFAULT_ALIGN_BYTES 16
#endif

template <class T> class __has_custom_new_operator {
    template <typename C> static inline char test (decltype(C::operator new (sizeof(C)))) ;
    template <typename C> static inline long test (...);    
  public:
    enum { value = sizeof(test<T>(nullptr)) == sizeof(char) };
};

inline void* __aligned_alloc (std::size_t size) {
  auto* original = std::malloc (size + EIGEN_DEFAULT_ALIGN_BYTES); 
  if (!original) throw std::bad_alloc(); 
  void *aligned = reinterpret_cast<void*>((reinterpret_cast<std::size_t>(original) & ~(std::size_t(EIGEN_DEFAULT_ALIGN_BYTES-1))) + EIGEN_DEFAULT_ALIGN_BYTES);
  *(reinterpret_cast<void**>(aligned) - 1) = original; 
  return aligned; 
}

inline void __aligned_delete (void* ptr) { if (ptr) std::free (*(reinterpret_cast<void**>(ptr) - 1)); }

/*! \def NO_MEM_ALIGN
 * used to signal that the class does not have special alignment
 * requirements, and so does not need a custom operator new method.
 *
 * The compiler will check whether this is indeed the case, and fail with an
 * appropriate warning if this is not true. In this case, you need to replace
 * NO_MEM_ALIGN with MEM_ALIGN.
 * \sa MEM_ALIGN
 * \sa CHECK_MEM_ALIGN
 */
#define NO_MEM_ALIGN \
  void __check_memalign () { static_assert (alignof(*this) <= MRTRIX_ALLOC_MEM_ALIGN, "please change to MEM_ALIGN for this class"); }

/*! \def MEM_ALIGN
 * used to signal that the class has special alignment requirements, and so
 * needs a custom memory-aligned operator new method.
 *
 * The compiler will check whether this is indeed needed, and fail with an
 * appropriate warning if this is not true. In this case, you need to replace
 * MEM_ALIGN with NO_MEM_ALIGN. While it is technically safe to use an
 * over-aligned allocator in this case, it will impact performance and memory
 * efficiency, and so is to be avoided. 
 * \sa NO_MEM_ALIGN
 * \sa CHECK_MEM_ALIGN
 */
#define MEM_ALIGN \
  void __check_memalign () { static_assert (alignof(*this) > MRTRIX_ALLOC_MEM_ALIGN, "no need to MEM_ALIGN: use NO_MEM_ALIGN here."); }\
  public: \
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

/*! \def CHECK_MEM_ALIGN
 * used to verify that the class is set up approriately for memory alignment
 * when dynamically allocated. This checks whether the class's alignment
 * requirements exceed that of the default allocator, and if so whether it has
 * custom operator new methods defined to deal with this. Conversely, it also
 * checks whether a custom allocator has been defined needlessly, which is to
 * be avoided for performance reasons. 
 *
 * The compiler will check whether this is indeed needed, and fail with an
 * appropriate warning if this is not true. In this case, you need to replace
 * MEM_ALIGN with NO_MEM_ALIGN.
 * \sa NO_MEM_ALIGN
 * \sa MEM_ALIGN
 */
#define CHECK_MEM_ALIGN(classname) \
    static_assert ( (alignof(classname) <= MRTRIX_ALLOC_MEM_ALIGN ) || __has_custom_new_operator<classname>::value, \
        "memory alignment not guaranteed\n\n" \
        "    Memory alignment requirement for this class is larger than that guaranteed\n" \
        "    by default allocator, and no custom operator new has been defined.\n" \
        "    This can cause unexpected runtime errors with Eigen.\n" \
        "    Please replace NO_MEM_ALIGN with MEM_ALIGN for this class.\n"); \
    static_assert ( (alignof(classname) > MRTRIX_ALLOC_MEM_ALIGN ) || !__has_custom_new_operator<classname>::value, \
        "unnecessary use of MEM_ALIGN\n\n" \
        "    Memory alignment requirement for this class is already catered for by default allocator.\n" \
        "    While this program will run fine as-is, using the non-default allocator does incur\n" \
        "    a performance overhead. Please replace MEM_ALIGN with NO_MEM_ALIGN for this class.\n")



namespace MR
{

  typedef float  float32;
  typedef double float64;
  typedef std::complex<double> cdouble;
  typedef std::complex<float> cfloat;

  template <typename T>
    struct container_cast : public T {
      template <typename U>
        container_cast (const U& x) : 
        T (x.begin(), x.end()) { }
    };

  //! the default type used throughout MRtrix
  typedef double default_type;

  constexpr default_type NaN = std::numeric_limits<default_type>::quiet_NaN();
  constexpr default_type Inf = std::numeric_limits<default_type>::infinity();

  //! the type for the affine transform of an image:
  typedef Eigen::Transform<default_type, 3, Eigen::AffineCompact> transform_type;


  //! check whether type is complex:
  template <class ValueType> struct is_complex : std::false_type { };
  template <class ValueType> struct is_complex<std::complex<ValueType>> : std::true_type { };


  //! check whether type is compatible with MRtrix3's file IO backend:
  template <class ValueType> 
    struct is_data_type : 
      std::integral_constant<bool, std::is_arithmetic<ValueType>::value || is_complex<ValueType>::value> { };


}

namespace std 
{
  // these are not defined in the standard, but are needed 
  // for use in generic templates:
  inline uint8_t abs (uint8_t x) { return x; }
  inline uint16_t abs (uint16_t x) { return x; }
  inline uint32_t abs (uint32_t x) { return x; }


  template <class T> inline ostream& operator<< (ostream& stream, const vector<T>& V)
  {
    stream << "[ ";
    for (size_t n = 0; n < V.size(); n++) 
      stream << V[n] << " ";
    stream << "]";
    return stream;
  }

  template <class T, std::size_t N> inline ostream& operator<< (ostream& stream, const array<T,N>& V)
  {
    stream << "[ ";
    for (size_t n = 0; n < N; n++) 
      stream << V[n] << " ";
    stream << "]";
    return stream;
  }

}

namespace Eigen {
  typedef Matrix<MR::default_type,3,1> Vector3;
  typedef Matrix<MR::default_type,4,1> Vector4;
}

#endif






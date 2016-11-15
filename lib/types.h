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


/*! \def MEMALIGN(classname)
 * to be placed immediately after \e every class or struct definition, to
 * ensure the class had appropriate alignment-aware new & delete operators.
 * This is required to ensure correct operation with Eigen >= 3.3 (due to the
 * 32-byte alignment requirement for AVX instructions). 
 *
 * A big problem with this is that \e every class that could contain
 * fixed-sized Eigen types (e.g. Matrix4d) must have these operators defined in
 * case any instances get allocated dynamically. The default new operator
 * provides no alignment guarantees, even for types declared using alignas().
 * It is very difficult to keep track of all such classes to make sure they are
 * all appropriately defined. The approach here will only enforce memory
 * alignment for those class that declare themselves are requirement wider
 * alignment than that provided by the default new operator (typically 16 byte
 * with gcc), using the default allocator otherwise. This means it can be used
 * for \e all classes indiscriminately with no loss of performance or memory
 * use efficiency, and under certain conditions makes it possible to largely
 * automate the process of ensuring compliance, as long as all class or struct
 * definitions all on the same line, for example:
 * \code
 * template <class X> class AlignMe { MEMALIGN(AlignMe) 
 *   ... 
 * };
 * \endcode
 * \warning This will insert a \c public: declaration at the head of the class
 * or struct. While this is the default for struct, special care must be taken
 * with class declarations since methods would otherwise be private by default.
 * If you need to have a private section at the head of your class, make sure
 * you use an explicit \c private: or \c protected: keyword as required. 
 */
#define MEMALIGN(classname) \
  public: \
    inline void* operator new (std::size_t size) { \
      if (alignof(classname) <= MRTRIX_ALLOC_MEMALIGN) return ::operator new (size); \
      std::cerr << __PRETTY_FUNCTION__ << ": new [" << size << "]\n"; \
      auto* original = std::malloc (size + alignof(classname)); \
      if (!original) throw std::bad_alloc(); \
      void *aligned = reinterpret_cast<void*>((reinterpret_cast<std::size_t>(original) & ~(std::size_t(alignof(classname)-1))) + alignof(classname)); \
      *(reinterpret_cast<void**>(aligned) - 1) = original; \
      return aligned; \
    } \
    inline void* operator new[] (std::size_t size) { return classname::operator new (size); } \
    inline void operator delete(void* ptr) { \
      if (alignof(classname) <= MRTRIX_ALLOC_MEMALIGN) ::operator delete (ptr); \
      else { \
        std::cerr << __PRETTY_FUNCTION__ << ": delete\n"; \
        if (ptr) std::free (*(reinterpret_cast<void**>(ptr) - 1)); \
      } \
    } \
    inline void operator delete[](void* ptr, std::size_t sz) { classname::operator delete (ptr); } \
    

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






/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __mrtrix_types_h__
#define __mrtrix_types_h__


#include <stdint.h>
#include <complex>
#include <iostream>
#include <vector>
#include <deque>
#include <cstddef>
#include <memory>

#define NOMEMALIGN

namespace MR {

#ifdef MRTRIX_MAX_ALIGN_T_NOT_DEFINED
# ifdef MRTRIX_STD_MAX_ALIGN_T_NOT_DEFINED
  // needed for clang 3.4:
  using __max_align_t = struct { NOMEMALIGN
    long long __clang_max_align_nonce1
        __attribute__((__aligned__(__alignof__(long long))));
    long double __clang_max_align_nonce2
        __attribute__((__aligned__(__alignof__(long double))));
  };
  constexpr size_t malloc_align = alignof (__max_align_t);
# else
  constexpr size_t malloc_align = alignof (std::max_align_t);
# endif
#else
  constexpr size_t malloc_align = alignof (::max_align_t);
#endif

  namespace Helper {
    template <class ImageType> class ConstRow;
    template <class ImageType> class Row;
  }
}

#ifdef EIGEN_HAS_OPENMP
# undef EIGEN_HAS_OPENMP
#endif

#define EIGEN_DENSEBASE_PLUGIN "eigen_plugins/dense_base.h"
#define EIGEN_MATRIXBASE_PLUGIN "eigen_plugins/dense_base.h"
#define EIGEN_ARRAYBASE_PLUGIN "eigen_plugins/dense_base.h"
#define EIGEN_MATRIX_PLUGIN "eigen_plugins/matrix.h"
#define EIGEN_ARRAY_PLUGIN "eigen_plugins/array.h"

#include <Eigen/Geometry>

/*! \defgroup VLA Variable-length array macros
 *
 * The reason for defining these macros at all is that VLAs are not part of the
 * C++ standard, and not available on all compilers. Regardless of the
 * availability of VLAs, they should be avoided if possible since they run the
 * risk of overrunning the stack if the length of the array is large, or if the
 * function is called recursively. They can be used safely in cases where the
 * size of the array is expected to be small, and the function will not be
 * called recursively, and in these cases may avoid the overhead of allocation
 * that might be incurred by the use of e.g. a vector.
 */

//! \{

/*! \def VLA
 * define a variable-length array (VLA) if supported by the compiler, or a
 * vector otherwise. This may have performance implications in the latter
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
  vector<type> __vla__ ## name(num); \
  type* name = &__vla__ ## name[0]
# define VLA_MAX(name, type, num, max) type name[max]
#else
# define VLA(name, type, num) type name[num]
# define VLA_MAX(name, type, num, max) type name[num]
#endif


/*! \def NON_POD_VLA
 * define a variable-length array of non-POD data if supported by the compiler,
 * or a vector otherwise. This may have performance implications in the
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
  vector<type> __vla__ ## name(num); \
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


template <class T> class __has_custom_new_operator { NOMEMALIGN
    template <typename C> static inline char test (decltype(C::operator new (sizeof(C)))) ;
    template <typename C> static inline long test (...);
  public:
    enum { value = sizeof(test<T>(nullptr)) == sizeof(char) };
};


inline void* __aligned_malloc (std::size_t size) {
  auto* original = std::malloc (size + EIGEN_DEFAULT_ALIGN_BYTES);
  if (!original) throw std::bad_alloc();
  void *aligned = reinterpret_cast<void*>((reinterpret_cast<std::size_t>(original) & ~(std::size_t(EIGEN_DEFAULT_ALIGN_BYTES-1))) + EIGEN_DEFAULT_ALIGN_BYTES);
  *(reinterpret_cast<void**>(aligned) - 1) = original;
  return aligned;
}

inline void __aligned_free (void* ptr) { if (ptr) std::free (*(reinterpret_cast<void**>(ptr) - 1)); }


#define MEMALIGN(...) public: \
  FORCE_INLINE void* operator new (std::size_t size) { return (alignof(__VA_ARGS__)>::MR::malloc_align) ? __aligned_malloc (size) : ::operator new (size); } \
  FORCE_INLINE void* operator new[] (std::size_t size) { return (alignof(__VA_ARGS__)>::MR::malloc_align) ? __aligned_malloc (size) : ::operator new[] (size); } \
  FORCE_INLINE void operator delete (void* ptr) { if (alignof(__VA_ARGS__)>::MR::malloc_align) __aligned_free (ptr); else ::operator delete (ptr); } \
  FORCE_INLINE void operator delete[] (void* ptr) { if (alignof(__VA_ARGS__)>::MR::malloc_align) __aligned_free (ptr); else ::operator delete[] (ptr); }


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
 * MEMALIGN with NOMEMALIGN.
 * \sa NOMEMALIGN
 * \sa MEMALIGN
 */
#define CHECK_MEM_ALIGN(...) \
    static_assert ( (alignof(__VA_ARGS__) <= ::MR::malloc_align) || __has_custom_new_operator<__VA_ARGS__>::value, \
        "class requires over-alignment, but no operator new defined! Please insert MEMALIGN() into class definition.")



namespace MR
{

  using float32 = float;
  using float64 = double;
  using cdouble = std::complex<double>;
  using cfloat  = std::complex<float>;

  template <typename T>
    struct container_cast : public T { MEMALIGN(container_cast<T>)
      template <typename U>
        container_cast (const U& x) :
        T (x.begin(), x.end()) { }
    };

  //! the default type used throughout MRtrix
  using default_type = double;

  constexpr default_type NaN = std::numeric_limits<default_type>::quiet_NaN();
  constexpr default_type Inf = std::numeric_limits<default_type>::infinity();

  //! the type for the affine transform of an image:
  using transform_type = Eigen::Transform<default_type, 3, Eigen::AffineCompact>;


  //! check whether type is complex:
  template <class ValueType> struct is_complex : std::false_type { NOMEMALIGN };
  template <class ValueType> struct is_complex<std::complex<ValueType>> : std::true_type { NOMEMALIGN };


  //! check whether type is compatible with MRtrix3's file IO backend:
  template <class ValueType>
    struct is_data_type :
      std::integral_constant<bool, std::is_arithmetic<ValueType>::value || is_complex<ValueType>::value> { NOMEMALIGN };


  template <typename X, int N=(alignof(X)>::MR::malloc_align)>
    class vector : public ::std::vector<X, Eigen::aligned_allocator<X>> { NOMEMALIGN
      public:
        using ::std::vector<X,Eigen::aligned_allocator<X>>::vector;
        vector() { }
    };

  template <typename X>
    class vector<X,0> : public ::std::vector<X> { NOMEMALIGN
      public:
        using ::std::vector<X>::vector;
        vector() { }
    };


  template <typename X, int N=(alignof(X)>::MR::malloc_align)>
    class deque : public ::std::deque<X, Eigen::aligned_allocator<X>> { NOMEMALIGN
      public:
        using ::std::deque<X,Eigen::aligned_allocator<X>>::deque;
        deque() { }
    };

  template <typename X>
    class deque<X,0> : public ::std::deque<X> { NOMEMALIGN
      public:
        using ::std::deque<X>::deque;
        deque() { }
    };


  template <typename X, typename... Args>
    inline std::shared_ptr<X> make_shared (Args&&... args) {
      return std::shared_ptr<X> (new X (std::forward<Args> (args)...));
    }

  template <typename X, typename... Args>
    inline std::unique_ptr<X> make_unique (Args&&... args) {
      return std::unique_ptr<X> (new X (std::forward<Args> (args)...));
    }


  // required to allow use of abs() call on unsigned integers in template
  // functions, etc, since the standard labels such calls ill-formed:
  // http://en.cppreference.com/w/cpp/numeric/math/abs
  template <typename X>
    inline constexpr typename std::enable_if<std::is_arithmetic<X>::value && std::is_unsigned<X>::value,X>::type abs (X x) { return x; }
  template <typename X>
    inline constexpr typename std::enable_if<std::is_arithmetic<X>::value && !std::is_unsigned<X>::value,X>::type abs (X x) { return std::abs(x); }
}

namespace std
{

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
  using Vector3 = Matrix<MR::default_type, 3, 1>;
  using Vector4 = Matrix<MR::default_type, 4, 1>;
}

#endif






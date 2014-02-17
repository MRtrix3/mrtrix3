#ifndef __math_math_h__
#define __math_math_h__

#include <cmath>
#include <cstdlib>

#include "types.h"
#include "exception.h"

#define DEFINE_ELEMENTARY_FUNCTION(name) \
  inline float name (const float val) throw () { return ::name##f (val); } \
  inline double name (const double val) throw () { return ::name (val); } \
  inline long double name (const long double val) throw () { return ::name##l (val); }

using std::isnan;
using std::isinf;

namespace MR
{
  namespace Math
  {

    /** @defgroup elfun Elementary Functions
      @{ */

    template <typename T> inline T pow2 (const T& v)
    {
      return v*v;
    }
    template <typename T> inline T pow3 (const T& v)
    {
      return pow2 (v) *v;
    }
    template <typename T> inline T pow4 (const T& v)
    {
      return pow2 (pow2 (v));
    }
    template <typename T> inline T pow5 (const T& v)
    {
      return pow4 (v) *v;
    }
    template <typename T> inline T pow6 (const T& v)
    {
      return pow2 (pow3 (v));
    }
    template <typename T> inline T pow7 (const T& v)
    {
      return pow6 (v) *v;
    }
    template <typename T> inline T pow8 (const T& v)
    {
      return pow2 (pow4 (v));
    }
    template <typename T> inline T pow9 (const T& v)
    {
      return pow8 (v) *v;
    }
    template <typename T> inline T pow10 (const T& v)
    {
      return pow8 (v) *pow2 (v);
    }

    DEFINE_ELEMENTARY_FUNCTION (exp);
    DEFINE_ELEMENTARY_FUNCTION (log);
    DEFINE_ELEMENTARY_FUNCTION (log10);
    DEFINE_ELEMENTARY_FUNCTION (sqrt);
    DEFINE_ELEMENTARY_FUNCTION (cos);
    DEFINE_ELEMENTARY_FUNCTION (sin);
    DEFINE_ELEMENTARY_FUNCTION (tan);
    DEFINE_ELEMENTARY_FUNCTION (acos);
    DEFINE_ELEMENTARY_FUNCTION (asin);
    DEFINE_ELEMENTARY_FUNCTION (atan);
    DEFINE_ELEMENTARY_FUNCTION (cosh);
    DEFINE_ELEMENTARY_FUNCTION (sinh);
    DEFINE_ELEMENTARY_FUNCTION (tanh);
    DEFINE_ELEMENTARY_FUNCTION (acosh);
    DEFINE_ELEMENTARY_FUNCTION (asinh);
    DEFINE_ELEMENTARY_FUNCTION (atanh);
    DEFINE_ELEMENTARY_FUNCTION (round);
    DEFINE_ELEMENTARY_FUNCTION (floor);
    DEFINE_ELEMENTARY_FUNCTION (ceil);

    //! template function with cast to different type
    /** example:
     * \code
     * float f = 21.412;
     * int x = round<int> (f);
     * \endcode
     */
    template <typename I, typename T> inline I round (const T x) throw ()
    {
      return static_cast<I> (round (x));
    }
    //! template function with cast to different type
    /** example:
     * \code
     * float f = 21.412;
     * int x = floor<int> (f);
     * \endcode
     */
    template <typename I, typename T> inline I floor (const T x) throw ()
    {
      return static_cast<I> (floor (x));
    }
    //! template function with cast to different type
    /** example:
     * \code
     * float f = 21.412;
     * int x = ceil<int> (f);
     * \endcode
     */
    template <typename I, typename T> inline I ceil (const T x) throw ()
    {
      return static_cast<I> (ceil (x));
    }

    inline int abs (const int val) throw ()
    {
      return ::abs (val);
    }
    inline long int abs (const long int val) throw ()
    {
      return ::labs (val);
    }
    inline long long int abs (const long long int val) throw ()
    {
      return ::llabs (val);
    }
    inline unsigned int abs (const unsigned int val) throw ()
    {
      return ::abs (val);
    }
    inline long unsigned int abs (const long unsigned int val) throw ()
    {
      return ::labs (val);
    }
    inline long long unsigned int abs (const long long unsigned int val) throw ()
    {
      return ::llabs (val);
    }
    inline float abs (const float val) throw ()
    {
      return ::fabsf (val);
    }
    inline double abs (const double val) throw ()
    {
      return ::fabs (val);
    }
    inline long double abs (const long double val) throw ()
    {
      return ::fabsl (val);
    }

    inline float pow (const float val, const float power) throw ()
    {
      return ::powf (val, power);
    }
    inline double pow (const double val, const double power) throw ()
    {
      return ::pow (val, power);
    }
    inline long double pow (const long double val, const long double power) throw ()
    {
      return ::powl (val, power);
    }

    inline float atan2 (const float y, const float x) throw ()
    {
      return ::atan2f (y, x);
    }
    inline double atan2 (const double y, const double x) throw ()
    {
      return ::atan2 (y, x);
    }
    inline long double atan2 (const long double y, const long double x) throw ()
    {
      return ::atan2l (y, x);
    }

    //! swap values in arrays
    /** \param a the first array containing the values to be swapped
     * \param b the second array containing the values to be swapped
     * \param size the number of elements to swap
     * \param stride_a the increment between successive elements in the first array
     * \param stride_b the increment between successive elements in the second array */
    template <typename T> inline void swap (T* a, T* b, const int size, const int stride_a = 1, const int stride_b = 1) throw ()
    {
      T* const end (a + size*stride_a);
      for (; a < end; a += stride_a, b += stride_b) std::swap (*a, *b);
    }

    //! find maximum value in array
    /** \param x the array containing the values to be searched
     * \param size the number of elements to search
     * \param index the index in the array of the result
     * \param stride the increment between successive elements in the array
     * \return the maximum value found in the array */
    template <typename T> inline T max (const T* const x, const int size, int& index, const int stride = 1) throw ()
    {
      T cval = *x, c;
      index = 0;
      for (int i = 1; i < size; i++)
        if ( (c = x[i*stride]) > cval) {
          cval = c;
          index = i;
        }
      return c;
    }

    //! find minimum value in array
    /** \param x the array containing the values to be searched
     * \param size the number of elements to search
     * \param index the index in the array of the result
     * \param stride the increment between successive elements in the array
     * \return the minimum value found in the array */
    template <typename T> inline T min (const T* const x, const int size, int& index, const int stride = 1) throw ()
    {
      T cval = *x, c;
      index = 0;
      for (int i = 1; i < size; i++)
        if ( (c = x[i*stride]) < cval) {
          cval = c;
          index = i;
        }
      return c;
    }

    //! find maximum absolute value in array
    /** \param x the array containing the values to be searched
     * \param size the number of elements to search
     * \param index the index in the array of the result
     * \param stride the increment between successive elements in the array
     * \return the maximum absolute value found in the array */
    template <typename T> inline T absmax (const T* const x, const int size, int& index, const int stride = 1) throw ()
    {
      T cval = abs (*x), c;
      index = 0;
      for (int i = 1; i < size; i++)
        if ( (c = abs (x[i*stride])) > cval) {
          cval = c;
          index = i;
        }
      return c;
    }


    /** @} */
  }
}

#endif




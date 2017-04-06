/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __math_legendre_h__
#define __math_legendre_h__

#include "math/math.h"

namespace MR
{
  namespace Math
  {
    namespace Legendre
    {

      template <typename ValueType>
        inline ValueType factorial (const ValueType n)
        {
          return (n < 2.0 ? 1.0 : n*factorial (n-1.0));
        }

      template <typename ValueType>
        inline ValueType double_factorial (const ValueType n)
        {
          return (n < 2.0 ? 1.0 : n*double_factorial (n-2.0));
        }

      template <typename ValueType>
        inline ValueType Plm (const int l, const int m, const ValueType x)
        {
          if (m && abs (x) >= 1.0) return (0.0);
          ValueType v0 = 1.0;
          if (m > 0) v0 = double_factorial (ValueType (2*m-1)) * pow (1.0-pow2 (x), m/2.0);
          if (m & 1) v0 = -v0; // (-1)^m
          if (l == m) return (v0);

          ValueType v1 = x * (2*m+1) * v0;
          if (l == m+1) return (v1);

          for (int n = m+2; n <= l; n++) {
            ValueType v2 = ( (2.0*n-1.0) *x*v1 - (n+m-1) *v0) / (n-m);
            v0 = v1;
            v1 = v2;
          }

          return (v1);
        }


      namespace
      {
        template <typename ValueType>
          inline ValueType Plm_sph_helper (const ValueType x, const ValueType m)
          {
            return (m < 1.0 ? 1.0 : x * (m-1.0) / m * Plm_sph_helper (x, m-2.0));
          }
      }

      template <typename ValueType>
        inline ValueType Plm_sph (const int l, const int m, const ValueType x)
        {
          ValueType x2 = pow2 (x);
          if (m && x2 >= 1.0) return (0.0);
          ValueType v0 = 0.282094791773878;
          if (m) v0 *= std::sqrt ((2*m+1) * Plm_sph_helper (1.0-x2, 2.0*m));
          if (m & 1) v0 = -v0;
          if (l == m) return (v0);

          ValueType f = std::sqrt (ValueType (2*m+3));
          ValueType v1 = x * f * v0;

          for (int n = m+2; n <= l; n++) {
            ValueType v2 = x*v1 - v0/f;
            f = std::sqrt (ValueType (4*pow2 (n)-1) / ValueType (pow2 (n)-pow2 (m)));
            v0 = v1;
            v1 = f*v2;
          }

          return (v1);
        }




      //* compute array of normalised associated Legendre functions
      /** \note upon completion, the (l,m) value will be stored in \c array[l]. Entries in \a array for l<m will be left undefined. */
      template <typename VectorType> 
        inline void Plm_sph (VectorType& array, const int lmax, const int m, const typename VectorType::Scalar x)
        {
          using value_type = typename VectorType::Scalar;
          value_type x2 = pow2 (x);
          if (m && x2 >= 1.0) {
            for (int n = m; n <= lmax; ++n)
              array[n] = 0.0;
            return;
          }
          array[m] = 0.282094791773878;
          if (m) array[m] *= std::sqrt (value_type (2*m+1) * Plm_sph_helper (1.0-x2, 2.0*m));
          if (m & 1) array[m] = -array[m];
          if (lmax == m) return;

          value_type f = std::sqrt (value_type (2*m+3));
          array[m+1] = x * f * array[m];

          for (int n = m+2; n <= lmax; n++) {
            array[n] = x*array[n-1] - array[n-2]/f;
            f = std::sqrt (value_type (4*pow2 (n)-1) / value_type (pow2 (n)-pow2 (m)));
            array[n] *= f;
          }
        }



      //* compute derivatives of normalised associated Legendre functions
      /** \note this function expects the previously computed array of associated Legendre functions to be stored in \a array,
       * (as computed by Plm_sph (VectorType& array, const int lmax, const int m, const ValueType x))
       * and will overwrite the values in \a array with the derivatives */
      template <typename VectorType> 
        inline void Plm_sph_deriv (VectorType& array, const int lmax, const int m, const typename VectorType::Scalar x)
        {
          using value_type = typename VectorType::Scalar;
          value_type x2 = pow2 (x);
          if (x2 >= 1.0) {
            for (int n = m; n <= lmax; n++) 
              array[n] = NaN;
            return;
          }
          x2 = 1.0 / (x2-1.0);
          for (int n = lmax; n > m; n--)
            array[n] = x2 * (n*x*array[n] - (n+m) * std::sqrt ( (2.0*n+1.0) * (n-m) / ( (2.0*n-1.0) * (n+m))) *array[n-1]);
          array[m] *= x2*m*x;
        }


    }
  }
}

#endif

/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/



/*

   Implementation inspired by:
   - Wikipedia: http://en.wikipedia.org/wiki/Associated_Legendre_functions
   - Mathematical Methods for Physicists (4th edition), George B. Arfken & Hans J. Weber
   - Numerical Recipes (3rd edition), William H. Press, Saul A. Teukolsky, William T. Vetterling, Brian P. Flannery

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

      inline default_type factorial (const default_type n)
      {
        return (n < 2.0 ? 1.0 : n*factorial (n-1.0));
      }

      inline default_type double_factorial (const default_type n)
      {
        return (n < 2.0 ? 1.0 : n*double_factorial (n-2.0));
      }

      inline default_type Plm (const int l, const int m, const default_type x)
      {
        if (m && abs (x) >= 1.0) return (0.0);
        default_type v0 = 1.0;
        if (m > 0) v0 = double_factorial (default_type (2*m-1)) * pow (1.0-pow2 (x), m/2.0);
        if (m & 1) v0 = -v0; // (-1)^m
        if (l == m) return (v0);

        default_type v1 = x * (2*m+1) * v0;
        if (l == m+1) return (v1);

        for (int n = m+2; n <= l; n++) {
          default_type v2 = ( (2.0*n-1.0) *x*v1 - (n+m-1) *v0) / (n-m);
          v0 = v1;
          v1 = v2;
        }

        return (v1);
      }


      namespace
      {
        inline default_type Plm_sph_helper (const default_type x, const default_type m)
        {
          return (m < 1.0 ? 1.0 : x * (m-1.0) / m * Plm_sph_helper (x, m-2.0));
        }
      }

        inline default_type Plm_sph (const int l, const int m, const default_type x)
        {
          default_type x2 = pow2 (x);
          if (m && x2 >= 1.0) return (0.0);
          default_type v0 = 0.282094791773878;
          if (m) v0 *= sqrt ((2*m+1) * Plm_sph_helper (1.0-x2, 2.0*m));
          if (m & 1) v0 = -v0;
          if (l == m) return (v0);

          default_type f = sqrt (default_type (2*m+3));
          default_type v1 = x * f * v0;

          for (int n = m+2; n <= l; n++) {
            default_type v2 = x*v1 - v0/f;
            f = sqrt (default_type (4*pow2 (n)-1) / default_type (pow2 (n)-pow2 (m)));
            v0 = v1;
            v1 = f*v2;
          }

          return (v1);
        }




      //* compute array of normalised associated Legendre functions
      /** \note upon completion, the (l,m) value will be stored in \c array[l]. Entries in \a array for l<m will be left undefined. */
      template <typename VectorType> 
        inline void Plm_sph (VectorType& array, const int lmax, const int m, const default_type x)
        {
          default_type x2 = pow2 (x);
          if (m && x2 >= 1.0) {
            array.segment (m, lmax-m+1).setZero();
            return;
          }
          array[m] = 0.282094791773878;
          if (m) array[m] *= sqrt (default_type (2*m+1) * Plm_sph_helper (1.0-x2, 2.0*m));
          if (m & 1) array[m] = -array[m];
          if (lmax == m) return;

          default_type f = sqrt (default_type (2*m+3));
          array[m+1] = x * f * array[m];

          for (int n = m+2; n <= lmax; n++) {
            array[n] = x*array[n-1] - array[n-2]/f;
            f = sqrt (default_type (4*pow2 (n)-1) / default_type (pow2 (n)-pow2 (m)));
            array[n] *= f;
          }
        }



      //* compute derivatives of normalised associated Legendre functions
      /** \note this function expects the previously computed array of associated Legendre functions to be stored in \a array,
       * (as computed by Plm_sph (VectorType& array, const int lmax, const int m, const default_type x))
       * and will overwrite the values in \a array with the derivatives */
      template <typename VectorType> 
        inline void Plm_sph_deriv (VectorType& array, const int lmax, const int m, const default_type x)
        {
          default_type x2 = pow2 (x);
          if (x2 >= 1.0) {
            for (int n = m; n <= lmax; n++) 
              array[n] = NaN;
            return;
          }
          x2 = 1.0 / (x2-1.0);
          for (int n = lmax; n > m; n--)
            array[n] = x2 * (n*x*array[n] - (n+m) *sqrt ( (2.0*n+1.0) * (n-m) / ( (2.0*n-1.0) * (n+m))) *array[n-1]);
          array[m] *= x2*m*x;
        }


    }
  }
}

#endif

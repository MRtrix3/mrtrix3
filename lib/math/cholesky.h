/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __math_cholesky_h__
#define __math_cholesky_h__

#include <gsl/gsl_linalg.h>
#include "math/matrix.h"

namespace MR
{
  namespace Math
  {
    namespace Cholesky
    {

      //! \cond skip

      // float definitions of GSL functions:
      int gsl_linalg_cholesky_decomp (gsl_matrix_float* A);
      int gsl_linalg_cholesky_solve (const gsl_matrix_float* cholesky, const gsl_vector_float* b, gsl_vector_float* x);
      int gsl_linalg_cholesky_svx (const gsl_matrix_float* cholesky, gsl_vector_float* x);
      int gsl_linalg_cholesky_invert (gsl_matrix_float* cholesky);

      //! \endcond

      /** @addtogroup linalg
        @{ */

      /** @defgroup cholesky Cholesky decomposition
        @{ */


      //! %Cholesky decomposition of \a A into lower triangular matrix
      /** \note the contents of \a A will be overwritten with its %Cholesky decomposition */
      template <typename ValueType> inline Matrix<ValueType>& decomp (Matrix<ValueType>& A)
      {
        gsl_linalg_cholesky_decomp (A.gsl());
        return A;
      }

      //! solve A*x = b given its %Cholesky decomposition \a D
      template <typename ValueType> inline Vector<ValueType>& solve (Vector<ValueType>& x, const Matrix<ValueType>& D, const Vector<ValueType>& b)
      {
        gsl_linalg_cholesky_solve (D.gsl(), b.gsl(), x.gsl());
        return x;
      }

      //! solve A*x = b given its %Cholesky decomposition \a D, in place
      /** \note b is passed in as \a x */
      template <typename ValueType> inline Vector<ValueType>& solve (Vector<ValueType>& x, const Matrix<ValueType>& D)
      {
        gsl_linalg_cholesky_svx (D.gsl(), x.gsl());
        return x;
      }

      //! invert A given its %Cholesky decomposition \a D, in place.
      template <typename ValueType> inline Matrix<ValueType>& inv_from_decomp (Matrix<ValueType>& D)
      {
        gsl_linalg_cholesky_invert (D.gsl());
        return D;
      }

      //! invert \a A using %Cholesky decomposition, in place.
      template <typename ValueType> inline Matrix<ValueType>& inv (Matrix<ValueType>& A)
      {
        return inv_from_decomp (decomp (A));
      }

      /** @} */
      /** @} */


#ifdef __math_complex_h__

      //! \cond skip
      template <> inline Matrix<cdouble>& decomp (Matrix<cdouble>& A)
      {
        gsl_linalg_complex_cholesky_decomp (A.gsl());
        return A;
      }

      template <> inline Vector<cdouble>& solve (Vector<cdouble>& x, const Matrix<cdouble>& D, const Vector<cdouble>& b)
      {
        gsl_linalg_complex_cholesky_solve (D.gsl(), b.gsl(), x.gsl());
        return x;
      }

      template <> inline Vector<cdouble>& solve (Vector<cdouble>& x, const Matrix<cdouble>& D)
      {
        gsl_linalg_complex_cholesky_svx (D.gsl(), x.gsl());
        return x;
      }

      //! \endcond

#endif

    }
  }
}

#endif








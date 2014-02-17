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


#ifndef __math_LU_h__
#define __math_LU_h__

#include <gsl/gsl_linalg.h>
#include "math/matrix.h"
#include "math/permutation.h"

namespace MR
{
  namespace Math
  {
    namespace LU
    {

      //! \cond skip

      // float definitions of GSL functions:
      int gsl_linalg_LU_decomp (gsl_matrix_float* A, gsl_permutation* p, int* signum);
      int gsl_linalg_LU_solve (const gsl_matrix_float* LU, const gsl_permutation* p, const gsl_vector_float* b, gsl_vector_float* x);
      int gsl_linalg_LU_svx (const gsl_matrix_float* LU, const gsl_permutation* p, gsl_vector_float* x);
      int gsl_linalg_LU_refine (const gsl_matrix_float* A, const gsl_matrix_float* LU, const gsl_permutation* p,
                                const gsl_vector_float* b, gsl_vector_float* x, gsl_vector_float* residual);
      int gsl_linalg_LU_invert (const gsl_matrix_float* LU, const gsl_permutation* p, gsl_matrix_float* inverse);
      double gsl_linalg_LU_det (gsl_matrix_float* LU, int signum);
      double gsl_linalg_LU_lndet (gsl_matrix_float* LU);
      int gsl_linalg_LU_sgndet (gsl_matrix_float* lu, int signum);

      //! \endcond

      /** @addtogroup linalg
        @{ */

      /** @defgroup lu LU decomposition
        @{ */


      //! %LU decomposition of A
      /** \note the contents of \a A will be overwritten with its %LU decomposition */
      template <typename T> inline Matrix<T>& decomp (Matrix<T>& A, Permutation& p, int& signum)
      {
        gsl_linalg_LU_decomp (A.gsl(), p.gsl(), &signum);
        return (A);
      }

      //! inverse of A given its %LU decomposition D,p
      template <typename T> inline Matrix<T>& inv (Matrix<T>& I, const Matrix<T>D, const Permutation& p)
      {
        I.allocate (D);
        gsl_linalg_LU_invert (D.gsl(), p.gsl(), I.gsl());
        return (I);
      }

      //! solve A*x = b given %LU decomposition D,p of A
      template <typename T> inline Vector<T>& solve (Vector<T>& x, const Matrix<T>& D, const Permutation& p, const Vector<T>& b)
      {
        x.allocate (D.rows());
        gsl_linalg_LU_solve (D.gsl(), p.gsl(), b.gsl(), x.gsl());
        return (x);
      }

      //! solve A*x = b given %LU decomposition D,p of A, in place (b passed in as x).
      template <typename T> inline Vector<T>& solve (Vector<T>& x, const Matrix<T>& D, const Permutation& p)
      {
        gsl_linalg_LU_svx (D.gsl(), p.gsl(), x.gsl());
        return (x);
      }

      //! inverse of A by %LU decomposition
      template <typename T> inline Matrix<T>& inv (Matrix<T>& I, const Matrix<T>& A)
      {
        I.allocate (A);
        Permutation p (A.rows());
        int signum;
        Matrix<T> D (A);
        decomp (D, p, signum);
        return (inv (I, D, p));
      }


      /** @} */
      /** @} */
    }
  }
}

#endif








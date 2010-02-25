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

#ifndef __math_LU_h__
#define __math_LU_h__

#include <gsl/gsl_linalg.h>
#include "math/matrix.h"
#include "math/permutation.h"

namespace MR {
  namespace Math {
    namespace LU {

      //! \cond skip

      // float definitions of GSL functions:
      int gsl_linalg_LU_decomp (gsl_matrix_float * A, gsl_permutation * p, int *signum);
      int gsl_linalg_LU_solve (const gsl_matrix_float * LU, const gsl_permutation * p, const gsl_vector_float * b, gsl_vector_float * x); 
      int gsl_linalg_LU_svx (const gsl_matrix_float * LU, const gsl_permutation * p, gsl_vector_float * x); 
      int gsl_linalg_LU_refine (const gsl_matrix_float * A, const gsl_matrix_float * LU, const gsl_permutation * p,
          const gsl_vector_float * b, gsl_vector_float * x, gsl_vector_float * residual); 
      int gsl_linalg_LU_invert (const gsl_matrix_float * LU, const gsl_permutation * p, gsl_matrix_float * inverse); 
      double gsl_linalg_LU_det (gsl_matrix_float * LU, int signum);
      double gsl_linalg_LU_lndet (gsl_matrix_float * LU);
      int gsl_linalg_LU_sgndet (gsl_matrix_float * lu, int signum);

      //! \endcond

      /** @addtogroup linalg 
        @{ */

      /** @defgroup lu LU decomposition
        @{ */


      //! %LU decomposition of A
      /** \note the contents of \a A will be overwritten with its %LU decomposition */
      template <typename T> inline Matrix<T>& decomp (Matrix<T>& A, Permutation& p, int& signum) { 
        gsl_linalg_LU_decomp (A.gsl(), p.gsl(), &signum);
        return (A); 
      }

      //! inverse of A given its %LU decomposition D,p
      template <typename T> inline Matrix<T>& inv (Matrix<T>& I, const Matrix<T>D, const Permutation& p) { 
	I.allocate (D); 
        gsl_linalg_LU_invert (D.gsl(), p.gsl(), I.gsl());
        return (I); 
      }

      //! solve A*x = b given %LU decomposition D,p of A
      template <typename T> inline Vector<T>& solve (Vector<T>& x, const Matrix<T>& D, const Permutation& p, const Vector<T>& b) {
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
      template <typename T> inline Matrix<T>& inv (Matrix<T>& I, const Matrix<T>& A) { 
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








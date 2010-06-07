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

#ifndef __math_least_squares_h__
#define __math_least_squares_h__

#include "math/cholesky.h"
#include "math/LU.h"

namespace MR {
  namespace Math {

    /** @addtogroup linalg 
      @{ */

    /** @defgroup ls Least-squares & Moore-Penrose pseudo-inverse
      @{ */


    //! solve least-squares problem Mx = b
    template <typename T> inline Vector<T>& solve_LS (Vector<T>& x, const Matrix<T>& M, const Vector<T>& b, Matrix<T>& work) {
      rankN_update (work, M, CblasTrans);
      Cholesky::decomp (work);
      mult (x, T(1.0), CblasTrans, M, b);
      return (Cholesky::solve (x, work));
    }

    //! compute Moore-Penrose pseudo-inverse of M given its transpose Mt
    template <typename T> inline Matrix<T>& pinv (Matrix<T>& I, const Matrix<T>& Mt, Matrix<T>& work) {
      mult (work, T(0.0), T(1.0), CblasNoTrans, Mt, CblasTrans, Mt);
      Cholesky::inv (work);
      return (mult (I, CblasLeft, T(0.0), T(1.0), CblasUpper, work, Mt));
    }

    //! compute Moore-Penrose pseudo-inverse of M 
    template <typename T> inline Matrix<T>& pinv (Matrix<T>& I, const Matrix<T>& M) {
      I.allocate (M.columns(), M.rows());
      Matrix<T> work (M.columns(), M.columns());
      Matrix<T> Mt (M.columns(), M.rows());
      return (pinv (I, transpose(Mt, M), work));
    }

    /** @} */
    /** @} */

  }
}

#endif









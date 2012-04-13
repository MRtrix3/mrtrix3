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

namespace MR
{
  namespace Math
  {

    namespace {
      template <typename ValueType> inline CBLAS_TRANSPOSE __transpose () { return CblasTrans; } 
#ifdef __math_complex_h__
      template <> inline CBLAS_TRANSPOSE __transpose<cfloat> () { return CblasConjTrans; }
      template <> inline CBLAS_TRANSPOSE __transpose<cdouble> () { return CblasConjTrans; }
#endif
    }

    /** @addtogroup linalg
      @{ */

    /** @defgroup ls Least-squares & Moore-Penrose pseudo-inverse
      @{ */


    //! solve over-determined least-squares problem Mx = b
    template <typename ValueType> 
      inline Vector<ValueType>& solve_LS (Vector<ValueType>& x, const Matrix<ValueType>& M, const Vector<ValueType>& b, Matrix<ValueType>& work)
    {
      work.allocate (M.columns(), M.columns());
      rankN_update (work, M, __transpose<ValueType>(), CblasLower);
      Cholesky::decomp (work);
      mult (x, ValueType (1.0), __transpose<ValueType>(), M, b);
      return Cholesky::solve (x, work);
    }



    //! solve regularised least-squares problem |Mx-b|^2 + r|x|^2 
    template <typename ValueType> 
      inline Vector<ValueType>& solve_LS_reg (
          Vector<ValueType>& x,
          const Matrix<ValueType>& M,
          const Vector<ValueType>& b,
          double reg_weight, 
          Matrix<ValueType>& work)
    {
      work.allocate (M.columns(), M.columns());
      rankN_update (work, M, __transpose<ValueType>(), CblasLower);
      work.diagonal() += ValueType (reg_weight);
      Cholesky::decomp (work);
      mult (x, ValueType (1.0), __transpose<ValueType>(), M, b);
      return Cholesky::solve (x, work);
    }



    //! compute Moore-Penrose pseudo-inverse of M given its transpose Mt
    template <typename ValueType> inline Matrix<ValueType>& pinv (Matrix<ValueType>& I, const Matrix<ValueType>& Mt, Matrix<ValueType>& work)
    {
      mult (work, ValueType (0.0), ValueType (1.0), CblasNoTrans, Mt, CblasTrans, Mt);
      Cholesky::inv (work);
      return mult (I, CblasLeft, ValueType (0.0), ValueType (1.0), CblasUpper, work, Mt);
    }




    //! compute Moore-Penrose pseudo-inverse of M
    template <typename ValueType> inline Matrix<ValueType>& pinv (Matrix<ValueType>& I, const Matrix<ValueType>& M)
    {
      I.allocate (M.columns(), M.rows());
      Matrix<ValueType> work (M.columns(), M.columns());
      Matrix<ValueType> Mt (M.columns(), M.rows());
      return pinv (I, transpose (Mt, M), work);
    }

    /** @} */
    /** @} */








  }
}

#endif









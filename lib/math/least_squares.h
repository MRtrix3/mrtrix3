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

#ifndef __math_least_squares_h__
#define __math_least_squares_h__

#include <Eigen/Cholesky>

#include "types.h"

namespace MR
{
  namespace Math
  {

    /** @addtogroup linalg
      @{ */

    /** @defgroup ls Least-squares & Moore-Penrose pseudo-inverse
      @{ */

/*
    namespace {
      template <typename ValueType> inline CBLAS_TRANSPOSE __transpose () { return CblasTrans; } 
#ifdef __math_complex_h__
      template <> inline CBLAS_TRANSPOSE __transpose<cfloat> () { return CblasConjTrans; }
      template <> inline CBLAS_TRANSPOSE __transpose<cdouble> () { return CblasConjTrans; }
#endif
    }


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

    //! solve regularised least-squares problem |Mx-b|^2 + |diag(w).x|^2 
    template <typename ValueType, typename RealValueType> 
      inline Vector<ValueType>& solve_LS_reg (
          Vector<ValueType>& x,
          const Matrix<ValueType>& M,
          const Vector<ValueType>& b,
          const Vector<RealValueType>& weights,
          Matrix<ValueType>& work)
    {
      work.allocate (M.columns(), M.columns());
      rankN_update (work, M, __transpose<ValueType>(), CblasLower);
      work.diagonal() += weights;
      Cholesky::decomp (work);
      mult (x, ValueType (1.0), __transpose<ValueType>(), M, b);
      return Cholesky::solve (x, work);
    }



*/



    //! return Moore-Penrose pseudo-inverse of M
    template <class MatrixType>
      inline Eigen::Matrix<typename MatrixType::Scalar,Eigen::Dynamic, Eigen::Dynamic> pinv (const MatrixType& M)
      {
        if (M.rows() >= M.cols()) 
         return (M.transpose()*M).ldlt().solve (M.transpose());                                                                                
        else 
          return (M*M.transpose()).ldlt().solve (M).transpose();                                                                               
      }

    template <class MatrixType>
      inline size_t rank (const MatrixType& M)
      {
        Eigen::FullPivLU<MatrixType> lu_decomp (M);
        return lu_decomp.rank();
      }

    /** @} */
    /** @} */








  }
}

#endif









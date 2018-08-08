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


#ifndef __math_least_squares_h__
#define __math_least_squares_h__

#include "types.h"
#include <Eigen/Cholesky>

namespace MR
{
  namespace Math
  {

    /** @addtogroup linalg
      @{ */

    /** @defgroup ls Least-squares & Moore-Penrose pseudo-inverse
      @{ */



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









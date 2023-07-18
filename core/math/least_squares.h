/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
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









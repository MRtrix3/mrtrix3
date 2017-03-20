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


#ifndef __dwi_svr_recon_h__
#define __dwi_svr_recon_h__


#include <vector>

#include "math/SH.h"



namespace MR
{
  namespace DWI
  {

    class ReconMatrix : public Eigen::EigenBase<ReconMatrix>
    {
    public:
      // Required typedefs, constants, and method:
      typedef float Scalar;
      typedef float RealScalar;
      typedef int StorageIndex;
      enum {
        ColsAtCompileTime = Eigen::Dynamic,
        MaxColsAtCompileTime = Eigen::Dynamic,
        IsRowMajor = false
      };
      Index rows() const { return M.rows(); }
      Index cols() const { return M.cols()*Y.cols(); }

      template<typename Rhs>
      Eigen::Product<MatrixReplacement,Rhs,Eigen::AliasFreeProduct> operator*(const Eigen::MatrixBase<Rhs>& x) const {
        return Eigen::Product<MatrixReplacement,Rhs,Eigen::AliasFreeProduct>(*this, x.derived());
      }

      // Custom API:
      ReconMatrix(const Header& in, const Eigen::Matrix& grad, const int lmax)
        : lmax(lmax),
          M(in.size(0)*in.size(1)*in.size(2)*in.size(3), in.size(0)*in.size(1)*in.size(2)),
          Y(in.size(2)*in.size(3), SH::NforL(lmax))
      {
        init_M(in);
        init_Y(in, grad);
      }

    private:
      const int lmax;
      Eigen::SparseMatrix<float> M;
      Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> Y;


      void init_M(const Header& in)
      {
        // Identity matrix for now.
        // TODO: complete with proper voxel weights later
        size_t i = 0;
        size_t j = 0;
        for (size_t x = 0; x < in.size(0); x++)
          for (size_t y = 0; y < in.size(1); y++)
            for (size_t z = 0; z < in.size(2); z++, j++)
              for (size_t v = 0; v < in.size(3); v++, i++)
                M(i,j) = 1.0;
      }

      void init_Y(const Header& in, const Eigen::Matrix& grad)
      {
        assert (in.size(3) == grad.rows());     // one gradient per volume
        Eigen::Vector3f vec;
        for (size_t i = 0; i < in.size(3); i++) {
          vec = {grad(i, 0), grad(i, 1), grad(i, 2)};
          for (size_t j = 0; j < in.size(2); j++) {
            // TODO: rotate vector with motion parameters
            delta(Y.row(j*in.size(3)+i), vec, lmax);
          }
        }
      }



    }




  }
}

#endif


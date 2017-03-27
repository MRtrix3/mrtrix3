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
#include <Eigen/Dense>
#include <Eigen/Sparse>

#include "header.h"
#include "math/SH.h"


namespace MR {
  namespace DWI {
    class ReconMatrix;
    class ReconMatrixAdjoint;
  }
}

namespace Eigen {
  namespace internal {
    // ReconMatrix inherits its traits from SparseMatrix
    template<>
    struct traits<MR::DWI::ReconMatrix> : public Eigen::internal::traits<Eigen::SparseMatrix<float,Eigen::RowMajor> >
    {};

    template<>
    struct traits<MR::DWI::ReconMatrixAdjoint> : public Eigen::internal::traits<Eigen::SparseMatrix<float,Eigen::RowMajor> >
    {};
  }
}


namespace MR
{
  namespace DWI
  {

    class ReconMatrix : public Eigen::EigenBase<ReconMatrix>
    {  MEMALIGN(ReconMatrix);
    public:
      // Required typedefs, constants, and method:
      typedef float Scalar;
      typedef float RealScalar;
      typedef int StorageIndex;
      enum {
        ColsAtCompileTime = Eigen::Dynamic,
        MaxColsAtCompileTime = Eigen::Dynamic,
        IsRowMajor = true
      };
      Eigen::Index rows() const { return M.rows(); }
      Eigen::Index cols() const { return M.cols()*Y.cols(); }


      template<typename Rhs>
      Eigen::Product<ReconMatrix,Rhs,Eigen::AliasFreeProduct> operator*(const Eigen::MatrixBase<Rhs>& x) const {
        return Eigen::Product<ReconMatrix,Rhs,Eigen::AliasFreeProduct>(*this, x.derived());
      }


      typedef Eigen::SparseMatrix<float, Eigen::RowMajor> SparseMat;


      // Custom API:
      ReconMatrix(const Header& in, const Eigen::MatrixXf& rigid, const Eigen::MatrixXf& grad, const int lmax)
        : lmax(lmax),
          nxy (in.size(0)*in.size(1)), nz (in.size(2)), nv (in.size(3)),
          M(nxy*nz*nv, nxy*nz),
          Y(nz*nv, Math::SH::NforL(lmax))
      {
        init_M(in, rigid);
        init_Y(in, rigid, grad);
      }

      const SparseMat& getM() const { return M; }
      const Eigen::MatrixXf& getY() const { return Y; }

      inline const size_t get_grad_idx(const size_t idx) const { return idx / nxy; }

      const ReconMatrixAdjoint adjoint() const;


    private:
      const int lmax;
      const size_t nxy, nz, nv;
      SparseMat M;
      Eigen::MatrixXf Y;


      inline void init_M(const Header& in, const Eigen::MatrixXf& rigid)
      {
        DEBUG("initialise M");
        // Identity matrix for now.
        // TODO: complete with proper voxel weights later
        // Note that this step is highly time and memory critical!
        // Special care must be taken when inserting elements and it is advised to reserve appropriate memory in advance.

        // reserve memory for 1 element along each row (outer strides with row-major order).
        M.reserve(Eigen::VectorXi::Constant(nv*nz*nxy, 1));

        // fill weights
        for (size_t v = 0; v < nv; v++) {
          for (size_t i = 0; i < nxy*nz; i++)
            M.insert(v*nxy*nz+i,i) = 1.0f;
        }

        M.makeCompressed();
      }


      inline void init_Y(const Header& in, const Eigen::MatrixXf& rigid, const Eigen::MatrixXf& grad)
      {
        DEBUG("initialise Y");
        assert (grad.rows() == nv);     // one gradient per volume

        Eigen::Vector3f vec;
        Eigen::Matrix3f rot;
        rot.setIdentity();
        Eigen::VectorXf delta;

        for (size_t i = 0; i < nv; i++) {
          vec = {grad(i, 0), grad(i, 1), grad(i, 2)};
          if (rigid.rows() == nv)
            rot = get_rotation(rigid(i,3), rigid(i,4), rigid(i,5));

          for (size_t j = 0; j < nz; j++) {
            // rotate vector with motion parameters
            if (rigid.rows() == nv*nz)
              rot = get_rotation(rigid(i*nz+j,3), rigid(i*nz+j,4), rigid(i*nz+j,5));

            // evaluate basis functions
            Y.row(i*nz+j) = Math::SH::delta(delta, rot*vec, lmax);
          }

        }

      }


      inline Eigen::Matrix3f get_rotation(float a1, float a2, float a3) const
      {
        Eigen::Matrix3f m;
        m = Eigen::AngleAxisf(a1, Eigen::Vector3f::UnitX())
          * Eigen::AngleAxisf(a2, Eigen::Vector3f::UnitY())
          * Eigen::AngleAxisf(a3, Eigen::Vector3f::UnitZ());
        return m;
      }


    };


    class ReconMatrixAdjoint : public Eigen::EigenBase<ReconMatrixAdjoint>
    {  MEMALIGN(ReconMatrixAdjoint);
    public:
      // Required typedefs, constants, and method:
      typedef float Scalar;
      typedef float RealScalar;
      typedef int StorageIndex;
      enum {
        ColsAtCompileTime = Eigen::Dynamic,
        MaxColsAtCompileTime = Eigen::Dynamic,
        IsRowMajor = true
      };
      Eigen::Index rows() const { return R.cols(); }
      Eigen::Index cols() const { return R.rows(); }

      template<typename Rhs>
      Eigen::Product<ReconMatrixAdjoint,Rhs,Eigen::AliasFreeProduct> operator*(const Eigen::MatrixBase<Rhs>& x) const {
        return Eigen::Product<ReconMatrixAdjoint,Rhs,Eigen::AliasFreeProduct>(*this, x.derived());
      }

      ReconMatrixAdjoint(const ReconMatrix& R)
        : R(R)
      {  }

      const ReconMatrix& R;

    };



  }
}


// Implementation of ReconMatrix * Eigen::DenseVector though a specialization of internal::generic_product_impl:
namespace Eigen {
  namespace internal {

    template<typename Rhs>
    struct generic_product_impl<MR::DWI::ReconMatrix, Rhs, SparseShape, DenseShape, GemvProduct>
      : generic_product_impl_base<MR::DWI::ReconMatrix,Rhs,generic_product_impl<MR::DWI::ReconMatrix,Rhs> >
    {
      typedef typename Product<MR::DWI::ReconMatrix,Rhs>::Scalar Scalar;

      template<typename Dest>
      static void scaleAndAddTo(Dest& dst, const MR::DWI::ReconMatrix& lhs, const Rhs& rhs, const Scalar& alpha)
      {
        // This method should implement "dst += alpha * lhs * rhs" inplace
        assert(alpha==Scalar(1) && "scaling is not implemented");

        TRACE;
        auto Y = lhs.getY();
        size_t nc = Y.cols();
        size_t nxyz = lhs.getM().cols();
        VectorXf r (lhs.rows());

        for (size_t j = 0; j < nc; j++) {
          r = lhs.getM() * rhs.segment(j*nxyz, nxyz);
          for (size_t i = 0; i < lhs.rows(); i++)
            dst[i] += r[i] * Y(lhs.get_grad_idx(i), j);
        }

      }

    };


    template<typename Rhs>
    struct generic_product_impl<MR::DWI::ReconMatrixAdjoint, Rhs, SparseShape, DenseShape, GemvProduct>
      : generic_product_impl_base<MR::DWI::ReconMatrixAdjoint,Rhs,generic_product_impl<MR::DWI::ReconMatrixAdjoint,Rhs> >
    {
      typedef typename Product<MR::DWI::ReconMatrixAdjoint,Rhs>::Scalar Scalar;

      template<typename Dest>
      static void scaleAndAddTo(Dest& dst, const MR::DWI::ReconMatrixAdjoint& lhs, const Rhs& rhs, const Scalar& alpha)
      {
        // This method should implement "dst += alpha * lhs * rhs" inplace
        assert(alpha==Scalar(1) && "scaling is not implemented");

        TRACE;
        auto Y = lhs.R.getY();
        size_t nc = Y.cols();
        size_t nxyz = lhs.R.getM().cols();
        VectorXf r (lhs.cols());

        for (size_t j = 0; j < nc; j++) {
          r = rhs;
          for (size_t i = 0; i < lhs.cols(); i++)
            r[i] *= Y(lhs.R.get_grad_idx(i), j);
          dst.segment(j*nxyz, nxyz) += lhs.R.getM().adjoint() * r;
        }

      }

    };


  }
}


#endif


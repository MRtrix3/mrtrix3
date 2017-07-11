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


#include <Eigen/Dense>
#include <Eigen/Sparse>

#include "types.h"
#include "header.h"
#include "transform.h"
#include "math/SH.h"
#include "dwi/shells.h"
#include "dwi/svr/psf.h"


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
      typedef ssize_t StorageIndex;
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


      typedef Eigen::SparseMatrix<float, Eigen::RowMajor, StorageIndex> SparseMat;


      // Custom API:
      ReconMatrix(const Header& in, const Eigen::MatrixXf& rigid, const Eigen::MatrixXf& grad, const int lmax, const vector<Eigen::MatrixXf>& rf)
        : lmax(lmax),
          nxy (in.size(0)*in.size(1)), nz (in.size(2)), nv (in.size(3)),
          M(nxy*nz*nv, nxy*nz)
      {
        init_M(in, rigid);
        init_Y(in, rigid, grad, rf);
      }

      const SparseMat& getM() const { return M; }
      const Eigen::MatrixXf& getY() const { return Y; }

      inline const size_t get_grad_idx(const size_t idx) const { return idx / nxy; }

      const ReconMatrixAdjoint adjoint() const;


      const Eigen::MatrixXf getY0(const Header& in, const Eigen::MatrixXf& grad, const vector<Eigen::MatrixXf>& rf) const
      {
        DEBUG("initialise Y0");
        assert (grad.rows() == nv);     // one gradient per volume

        Shells shells (grad.template cast<double>());
        vector<size_t> idx (shells.volumecount());
        vector<Eigen::MatrixXf> shellbasis;
        int n = 0;
        if (rf.empty()) {
          n = Math::SH::NforL(lmax);
        } else {
          for (auto& r : rf)
            n += Math::SH::NforL(std::min(2*(int(r.cols())-1), lmax));
        }

        Eigen::MatrixXf Y0 (nv, n);

        for (size_t s = 0; s < shells.count(); s++) {
          for (auto v : shells[s].get_volumes())
            idx[v] = s;

          Eigen::MatrixXf B;
          if (rf.empty()) {
            B.setIdentity(Math::SH::NforL(lmax), Math::SH::NforL(lmax));
          }
          else {
            B.setZero(n, Math::SH::NforL(lmax));
            size_t j = 0;
            for (auto& r : rf) {
              for (size_t l = 0; l < r.cols() and 2*l <= lmax; l++)
                for (size_t i = l*(2*l-1); i < (l+1)*(2*l+1); i++, j++)
                  B(j,i) = r(s,l);
            }
          }
          shellbasis.push_back(B);
        }

        Eigen::Vector3f vec;
        Eigen::VectorXf delta;

        for (size_t i = 0; i < nv; i++) {
          vec = {grad(i, 0), grad(i, 1), grad(i, 2)};

          // evaluate basis functions
          Math::SH::delta(delta, vec, lmax);
          if (rf.empty()) {
            Y0.row(i) = delta;
          }
          else {
            Y0.row(i) = shellbasis[idx[i]]*delta;
          }

        }

        return Y0;
      }


    private:
      const int lmax;
      const size_t nxy, nz, nv;
      SparseMat M;
      Eigen::MatrixXf Y;


      void init_M(const Header& in, const Eigen::MatrixXf& rigid)
      {
        DEBUG("initialise M");
        // Tri-linear interpolation for now.
        // TODO: add point spread function for superresolution

        // Note that this step is highly time and memory critical!
        // Special care must be taken when inserting elements and it is advised to reserve appropriate memory in advance.

        // reserve memory for 8 elements along each row (outer strides with row-major order).

        M.reserve(Eigen::VectorXi::Constant(nv*nz*nxy, 8));

        // set up transform
        Transform T0 (in);          // assume output transform = input transform; needs extending for superresolution

        transform_type T;
        T.setIdentity();

        Eigen::Vector3 p, pp0;
        Eigen::Vector3i p0;
        Eigen::Vector3f w;

        // fill weights
        size_t i = 0;
        for (size_t v = 0; v < nv; v++) {
          if (rigid.rows() == nv)
            T = get_transform(rigid.row(v));

          for (size_t z = 0; z < nz; z++) {
            if (rigid.rows() == nv*nz)
              T = get_transform(rigid.row(v*nz+z));

            for (size_t y = 0; y < in.size(1); y++) {
              for (size_t x = 0; x < in.size(0); x++, i++) {
                p = T0.scanner2voxel * T * T0.voxel2scanner * Eigen::Vector3(x, y, z);
                pp0 = Eigen::Vector3(std::floor(p[0]), std::floor(p[1]), std::floor(p[2]));
                p0 = pp0.template cast<int>();
                w = (p - pp0).template cast<float>();

                if (inbounds(in, p0[0]  , p0[1]  , p0[2]  )) M.insert(i, get_idx(in, p0[0]  , p0[1]  , p0[2]  )) = (1 - w[0]) * (1 - w[1]) * (1 - w[2]);
                if (inbounds(in, p0[0]+1, p0[1]  , p0[2]  )) M.insert(i, get_idx(in, p0[0]+1, p0[1]  , p0[2]  )) =      w[0]  * (1 - w[1]) * (1 - w[2]);
                if (inbounds(in, p0[0]  , p0[1]+1, p0[2]  )) M.insert(i, get_idx(in, p0[0]  , p0[1]+1, p0[2]  )) = (1 - w[0]) *      w[1]  * (1 - w[2]);
                if (inbounds(in, p0[0]+1, p0[1]+1, p0[2]  )) M.insert(i, get_idx(in, p0[0]+1, p0[1]+1, p0[2]  )) =      w[0]  *      w[1]  * (1 - w[2]);
                if (inbounds(in, p0[0]  , p0[1]  , p0[2]+1)) M.insert(i, get_idx(in, p0[0]  , p0[1]  , p0[2]+1)) = (1 - w[0]) * (1 - w[1]) *      w[2] ;
                if (inbounds(in, p0[0]+1, p0[1]  , p0[2]+1)) M.insert(i, get_idx(in, p0[0]+1, p0[1]  , p0[2]+1)) =      w[0]  * (1 - w[1]) *      w[2] ;
                if (inbounds(in, p0[0]  , p0[1]+1, p0[2]+1)) M.insert(i, get_idx(in, p0[0]  , p0[1]+1, p0[2]+1)) = (1 - w[0]) *      w[1]  *      w[2] ;
                if (inbounds(in, p0[0]+1, p0[1]+1, p0[2]+1)) M.insert(i, get_idx(in, p0[0]+1, p0[1]+1, p0[2]+1)) =      w[0]  *      w[1]  *      w[2] ;

              }
            }

          }
        }

        M.makeCompressed();
      }


      void init_Y(const Header& in, const Eigen::MatrixXf& rigid, const Eigen::MatrixXf& grad, const vector<Eigen::MatrixXf>& rf)
      {
        DEBUG("initialise Y");
        assert (grad.rows() == nv);     // one gradient per volume

        Shells shells (grad.template cast<double>());
        vector<size_t> idx (shells.volumecount());
        vector<Eigen::MatrixXf> shellbasis;
        int n = 0;
        if (rf.empty()) {
          n = Math::SH::NforL(lmax);
        } else {
          for (auto& r : rf)
            n += Math::SH::NforL(std::min(2*(int(r.cols())-1), lmax));
        }
        Y.resize(nv*nz, n);
        for (size_t s = 0; s < shells.count(); s++) {
          for (auto v : shells[s].get_volumes())
            idx[v] = s;

          Eigen::MatrixXf B;
          if (rf.empty()) {
            B.setIdentity(Math::SH::NforL(lmax), Math::SH::NforL(lmax));
          }
          else { 
            B.setZero(n, Math::SH::NforL(lmax));
            size_t j = 0;
            for (auto& r : rf) {
              for (size_t l = 0; l < r.cols() and 2*l <= lmax; l++)
                for (size_t i = l*(2*l-1); i < (l+1)*(2*l+1); i++, j++)
                  B(j,i) = r(s,l);
            }
          }
          shellbasis.push_back(B);
        }

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
            Math::SH::delta(delta, rot*vec, lmax);
            if (rf.empty()) {
              Y.row(i*nz+j) = delta;
            }
            else {
              Y.row(i*nz+j) = shellbasis[idx[i]]*delta;
            }
          }

        }

      }


      inline Eigen::Matrix3f get_rotation(const float a1, const float a2, const float a3) const
      {
        Eigen::Matrix3f m;
        m = Eigen::AngleAxisf(a1, Eigen::Vector3f::UnitX())
          * Eigen::AngleAxisf(a2, Eigen::Vector3f::UnitY())
          * Eigen::AngleAxisf(a3, Eigen::Vector3f::UnitZ());
        return m;
      }


      inline transform_type get_transform(const Eigen::VectorXf& p) const
      {
        transform_type T;
        T.translation() = Eigen::Vector3d( double(p[0]), double(p[1]), double(p[2]) );
        T.linear() = get_rotation(p[3], p[4], p[5]).template cast<double>();
        return T;
      }


      inline size_t get_idx(const Header& h, const int x, const int y, const int z) const
      {
        return size_t(z*h.size(1)*h.size(0) + y*h.size(0) + x);
      }


      inline bool inbounds(const Header& h, const int x, const int y, const int z) const
      {
        return (x >= 0) && (x < h.size(0))
            && (y >= 0) && (y < h.size(1))
            && (z >= 0) && (z < h.size(2));
      }


    };


    class ReconMatrixAdjoint : public Eigen::EigenBase<ReconMatrixAdjoint>
    {  MEMALIGN(ReconMatrixAdjoint);
    public:
      // Required typedefs, constants, and method:
      typedef float Scalar;
      typedef float RealScalar;
      typedef ssize_t StorageIndex;
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

        //TRACE;
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

        //TRACE;
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


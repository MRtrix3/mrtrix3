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
        : lmax (lmax),
          nxy (in.size(0)*in.size(1)), nz (in.size(2)), nv (in.size(3)),
          nc (get_ncoefs(rf)),
          M (nxy*nz*nv, nxy*nz)
      {
        init_M(in, rigid);
        init_Y(in, rigid, grad, rf);
      }

      const SparseMat&       getM() const { return M; }
      const Eigen::MatrixXf& getY() const { return Y; }
      const Eigen::MatrixXf& getW() const { return W; }

      void setW (const Eigen::MatrixXf& weights)
      {
        W = weights;
      }

      inline const size_t get_grad_idx(const size_t idx) const { return idx / nxy; }

      const ReconMatrixAdjoint adjoint() const;


      const Eigen::MatrixXf getY0(const Header& in, const Eigen::MatrixXf& grad, const vector<Eigen::MatrixXf>& rf) const
      {
        DEBUG("initialise Y0");
        assert (grad.rows() == nv);     // one gradient per volume

        Shells shells (grad.template cast<double>());
        vector<size_t> idx (shells.volumecount());
        vector<Eigen::MatrixXf> shellbasis;
        Eigen::MatrixXf Y0 (nv, nc);

        for (size_t s = 0; s < shells.count(); s++) {
          for (auto v : shells[s].get_volumes())
            idx[v] = s;

          Eigen::MatrixXf B;
          if (rf.empty()) {
            B.setIdentity(Math::SH::NforL(lmax), Math::SH::NforL(lmax));
          }
          else {
            B.setZero(nc, Math::SH::NforL(lmax));
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
      const size_t nxy, nz, nv, nc;
      SparseMat M;
      Eigen::MatrixXf Y;
      Eigen::MatrixXf W;


      void init_M(const Header& in, const Eigen::MatrixXf& rigid)
      {
        DEBUG("initialise M");
        // Note that this step is highly time and memory critical!
        // Special care must be taken when inserting elements and it is advised to reserve appropriate memory in advance.

        int n = 2;
        SincPSF<float> psf (n);
        SSP<float> ssp {};

        // reserve memory for elements along each row (outer strides with row-major order).
        M.reserve(Eigen::VectorXi::Constant(nv*nz*nxy, (n+1)*8*n*n*n));

        // set up transform
        Transform T0 (in);          // assume output transform = input transform; needs extending for anisotropic voxels

        transform_type Ts2r, Tr2s;

        Eigen::Vector3f ps, pr;
        Eigen::Vector3i p;
        
        // fill weights
        size_t i = 0;
        for (size_t v = 0; v < nv; v++) {   // volumes
          if (rigid.rows() == nv) {
            Ts2r = T0.scanner2voxel * get_transform(rigid.row(v)) * T0.voxel2scanner;
            Tr2s = Ts2r.inverse();
          }

          for (size_t z = 0; z < nz; z++) { // slices
            if (rigid.rows() == nv*nz) {
              Ts2r = T0.scanner2voxel * get_transform(rigid.row(v*nz+z)) * T0.voxel2scanner;
              Tr2s = Ts2r.inverse();
            }

            // in-plane
            for (size_t y = 0; y < in.size(1); y++) {
              for (size_t x = 0; x < in.size(0); x++, i++) {

                for (int s = -n; s <= n; s++) {     // ssp neighbourhood
                  ps = Eigen::Vector3f(x, y, z+s);
                  pr = (Ts2r.cast<float>() * ps);

                  for (int rx = -n; rx < n; rx++) { // sinc interpolator
                    for (int ry = -n; ry < n; ry++) {
                      for (int rz = -n; rz < n; rz++) {
                        p = Eigen::Vector3i(std::ceil(pr[0])+rx, std::ceil(pr[1])+ry, std::ceil(pr[2])+rz);
                        if (inbounds(in, p[0], p[1], p[2])) {
                          M.coeffRef(i, get_idx(in, p[0], p[1], p[2])) += ssp(s) * psf(pr - p.cast<float>());
                        }
                      }
                    }
                  }
                }

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
        Y.resize(nv*nz, nc);

        for (size_t s = 0; s < shells.count(); s++) {
          for (auto v : shells[s].get_volumes())
            idx[v] = s;

          Eigen::MatrixXf B;
          if (rf.empty()) {
            B.setIdentity(Math::SH::NforL(lmax), Math::SH::NforL(lmax));
          }
          else { 
            B.setZero(nc, Math::SH::NforL(lmax));
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


      inline size_t get_ncoefs(rf) const
      {
        size_t n = 0;
        if (rf.empty()) {
          n = Math::SH::NforL(lmax);
        } else {
          for (auto& r : rf)
            n += Math::SH::NforL(std::min(2*(int(r.cols())-1), lmax));
        }
        return n;
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
        Eigen::MatrixXf W = lhs.getW();
        Eigen::Map<VectorXf> w (W.data(),W.size());
        auto Y = w.asDiagonal() * lhs.getY();
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
        Eigen::MatrixXf W = lhs.R.getW();
        Eigen::Map<VectorXf> w (W.data(),W.size());
        auto Y = w.asDiagonal() * lhs.R.getY();
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


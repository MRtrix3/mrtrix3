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
#include "parallel_for.h"
#include "interp/linear.h"
#include "interp/cubic.h"

#include "dwi/svr/param.h"
#include "dwi/svr/psf.h"


namespace MR {
  namespace Interp {
    template <class ImageType>
    class CubicAdjoint : public Cubic <ImageType>
    { MEMALIGN(CubicAdjoint<ImageType>)
      public:
        using typename Cubic<ImageType>::value_type;
        using Cubic<ImageType>::clamp;
        using Cubic<ImageType>::P;
        using Cubic<ImageType>::weights_vec;

        CubicAdjoint (const ImageType& parent, value_type outofbounds = 0)
            : Cubic <ImageType> (parent, outofbounds)
        { }

        //! Add value to local region by interpolation weights.
        void adjoint_add (value_type val) {
          if (Base<ImageType>::out_of_bounds) return;

          ssize_t c[] = { ssize_t (std::floor (P[0])-1), ssize_t (std::floor (P[1])-1), ssize_t (std::floor (P[2])-1) };

          size_t i(0);
          for (ssize_t z = 0; z < 4; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 4; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 4; ++x) {
                ImageType::index(0) = clamp (c[0] + x, ImageType::size (0));
                ImageType::value() += weights_vec[i++] * val;
              }
            }
          }
        }
    };
  }

  namespace DWI {
    namespace SVR {
      class ReconMatrix;
      class ReconMatrixAdjoint;
    }
  }
}


namespace Eigen {
  namespace internal {
    // ReconMatrix inherits its traits from SparseMatrix
    template<>
    struct traits<MR::DWI::SVR::ReconMatrix> : public Eigen::internal::traits<Eigen::SparseMatrix<float,Eigen::RowMajor> >
    {};

    template<>
    struct traits<MR::DWI::SVR::ReconMatrixAdjoint> : public Eigen::internal::traits<Eigen::SparseMatrix<float,Eigen::ColMajor> >
    {};
  }
}


namespace MR
{
  namespace DWI
  {
    namespace SVR
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

      Eigen::Index rows() const { return nxy*nz*nc; }
      Eigen::Index cols() const { return nxy*nz*nc; }

      template<typename Rhs>
      Eigen::Product<ReconMatrix,Rhs,Eigen::AliasFreeProduct> operator*(const Eigen::MatrixBase<Rhs>& x) const {
        return Eigen::Product<ReconMatrix,Rhs,Eigen::AliasFreeProduct>(*this, x.derived());
      }

      typedef Eigen::SparseMatrix<float, Eigen::RowMajor, StorageIndex> SparseMat;
      typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMatrixXf;

      typedef Interp::LinearInterp<Image<float>, Interp::LinearInterpProcessingType::ValueAndDerivative> FieldInterpType;


      // Custom API:
      ReconMatrix(const Header& in, const Eigen::MatrixXf& rigid, const Eigen::MatrixXf& grad,
                  const int lmax, const vector<Eigen::MatrixXf>& rf, const SSP<float,2>& ssp, const float reg)
        : lmax (lmax),
          nx (in.size(0)), ny (in.size(1)), nz (in.size(2)), nv (in.size(3)),
          nxy (nx*ny), nc (get_ncoefs(rf)), ne (rigid.rows() / nv),
          T0 (in),  // Preserve original resolution.
          shellbasis (init_shellbasis(grad, rf)),
          ssp (ssp), motion (rigid), htmp(in)
      {
        INFO("Multiband factor " + str(nz/ne) + " detected.");
        init_Y(grad);
        init_laplacian(nv*reg*reg);
        assert (motion.rows() == nv*ne);

        // set header for temporary images.
        htmp.ndim() = 3;
        htmp.stride(0) = 1;
        htmp.stride(1) = 2;
        htmp.stride(2) = 3;
        htmp.sanitise();
      }

      const RowMatrixXf& getY() const { return Y; }

      const Eigen::MatrixXf& getWeights() const { return W; }

      void setWeights(const Eigen::MatrixXf& weights) { W = weights; }

      const Eigen::MatrixXf& getShellBasis(const int shellidx) const { return shellbasis[shellidx]; }

      void setField(const Image<float>& fieldmap, const size_t fieldidx, const Eigen::MatrixXf& petable)
      {
        field = fieldmap;
        pe = petable;
        Tf = Transform(field).scanner2voxel
            * get_transform(motion.block(fieldidx*ne,0,ne,6).colwise().mean()).inverse()
            * T0.voxel2scanner;
      }

      template <typename VectorType1, typename VectorType2>
      void project_x2y(VectorType1& dst, const VectorType2& rhs) const
      {
        INFO("Forward projection.");
        size_t nxyz = nxy*nz;
        Eigen::Map<const RowMatrixXf> X (rhs.data(), nxyz, nc);
        Thread::parallel_for<size_t>(0, nv*ne,
          [&](size_t idx) {
            size_t v = idx/ne;
            auto tmp = Image<float>::scratch(htmp);
            Eigen::Map<Eigen::VectorXf> q (tmp.address(), nxyz);
            q = X * Y.row(idx).adjoint();
            for (size_t z = idx%ne; z < nz; z += ne) {
              Eigen::Ref<Eigen::VectorXf> r = dst.segment((nz*v+z)*nxy, nxy);
              project_slice_x2y(v, z, r, tmp);
            }
          });
      }

      template <typename VectorType1, typename VectorType2>
      void project_y2x(VectorType1& dst, const VectorType2& rhs) const
      {
        INFO("Transpose projection.");
        size_t nxyz = nxy*nz;
        Eigen::Map<RowMatrixXf> X (dst.data(), nxyz, nc);
        RowMatrixXf zero (nxyz, nc); zero.setZero();
        X = Thread::parallel_sum<RowMatrixXf, size_t>(0, nv*ne,
          [&](size_t idx, RowMatrixXf& T) {
            size_t v = idx/ne;
            auto tmp = Image<float>::scratch(htmp);
            Eigen::Map<Eigen::VectorXf> r (tmp.address(), nxyz);
            r.setZero();
            for (size_t z = idx%ne; z < nz; z += ne) {
              project_slice_y2x(v, z, tmp, W(z,v) * rhs.segment((nz*v+z)*nxy, nxy));
            }
            T.noalias() += r * Y.row(idx);
          }, zero);
      }

      template <typename VectorType1, typename VectorType2>
      void project_x2x(VectorType1& dst, const VectorType2& rhs) const
      {
        INFO("Full projection.");
        size_t nxyz = nxy*nz;
        Eigen::Map<const RowMatrixXf> Xi (rhs.data(), nxyz, nc);
        Eigen::Map<RowMatrixXf> Xo (dst.data(), nxyz, nc);
        RowMatrixXf zero (nxyz, nc); zero.setZero();
        Xo = Thread::parallel_sum<RowMatrixXf, size_t>(0, nv*ne,
          [&](size_t idx, RowMatrixXf& T) {
            size_t v = idx/ne;
            auto tmp1 = Image<float>::scratch(htmp);
            Eigen::Map<Eigen::VectorXf> q (tmp1.address(), nxyz);
            q = Xi * Y.row(idx).adjoint();
            auto tmp2 = Image<float>::scratch(htmp);
            Eigen::Map<Eigen::VectorXf> r (tmp2.address(), nxyz);
            r.setZero();
            // Declare temporary slice
            Eigen::VectorXf tmpslice (nxy);
            for (size_t z = idx%ne; z < nz; z += ne) {
              project_slice_x2x(v, z, tmp2, tmp1, W(z,v));
              //tmpslice.setZero();
              //project_slice_x2y_alt(v, z, tmpslice, tmp1);
              //tmpslice *= W(z,v);
              //project_slice_y2x_alt(v, z, tmp2, tmpslice);
            }
            T.noalias() += r * Y.row(idx);
          }, zero);
        Xo += L.adjoint() * (L * Xi);
        Xo += std::numeric_limits<float>::epsilon() * Xi;
      }


    private:
      const int lmax;
      const size_t nx, ny, nz, nv, nxy, nc, ne;
      const Transform T0;
      const vector<Eigen::MatrixXf> shellbasis;
      const SSP<float,2> ssp;

      RowMatrixXf motion;
      RowMatrixXf Y;
      Eigen::MatrixXf W;
      SparseMat L;

      Header htmp;

      Image<float> field;
      Eigen::MatrixXf pe;
      transform_type Tf;


      vector<Eigen::MatrixXf> init_shellbasis(const Eigen::MatrixXf& grad, const vector<Eigen::MatrixXf>& rf) const
      {
        Shells shells (grad.template cast<double>());
        vector<Eigen::MatrixXf> basis;

        for (size_t s = 0; s < shells.count(); s++) {
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
          basis.push_back(B);
        }

        return basis;
      }

      void init_Y(const Eigen::MatrixXf& grad)
      {
        DEBUG("initialise Y");
        assert (grad.rows() == nv);     // one gradient per volume

        vector<size_t> idx = get_shellidx(grad);
        Y.resize(nv*ne, nc);

        Eigen::Vector3f vec;
        Eigen::Matrix3f rot;
        rot.setIdentity();
        Eigen::VectorXf delta;

        for (size_t v = 0; v < nv; v++) {
          vec = {grad(v, 0), grad(v, 1), grad(v, 2)};
          for (size_t e = 0; e < ne; e++) {
            // rotate vector with motion parameters
            rot = get_rotation(motion.row(v*ne+e));
            // evaluate basis functions
            Math::SH::delta(delta, rot*vec, lmax);
            Y.row(v*ne+e) = shellbasis[idx[v]]*delta;
          }
        }

      }

      inline Eigen::Matrix3f get_rotation(const Eigen::VectorXf& p) const
      {
        Eigen::Matrix3f m = se3exp(p).topLeftCorner<3,3>();
        return m;
      }

      inline transform_type get_transform(const Eigen::VectorXf& p) const
      {
        transform_type T (se3exp(p).cast<double>());
        return T;
      }

      inline transform_type get_Ts2r(const size_t v, const size_t z) const
      {
        transform_type Ts2r = T0.scanner2voxel * get_transform(motion.row(v*ne+z%ne)) * T0.voxel2scanner;
        return Ts2r;
      }

      inline size_t get_idx(const int x, const int y, const int z) const
      {
        return size_t(z*nxy + y*nx + x);
      }

      inline size_t get_ncoefs(const vector<Eigen::MatrixXf>& rf) const
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

      vector<size_t> get_shellidx(const Eigen::MatrixXf& grad) const
      {
        Shells shells (grad.template cast<double>());
        vector<size_t> idx (shells.volumecount());

        for (size_t s = 0; s < shells.count(); s++) {
          for (auto v : shells[s].get_volumes())
            idx[v] = s;
        }
        return idx;
      }

      inline size_t get_grad_idx(const size_t idx) const { return idx / nxy; }
      inline size_t get_grad_idx(const size_t v, const size_t z) const { return v*nz + z; }

      template <typename VectorType1, typename ImageType2>
      void project_slice_x2y(const int v, const int z, VectorType1& dst, const ImageType2& rhs) const
      {
        std::unique_ptr<FieldInterpType> finterp;
        if (field.valid())
          finterp = make_unique<FieldInterpType>(field, 0.0f);

        Eigen::Vector3 ps, pr, peoffset;
        float iJac;
        transform_type Ts2r = get_Ts2r(v, z);
        if (field.valid()) {
          peoffset = pe.block<1,3>(v, 0).transpose().cast<double>();
        }

        Interp::Cubic<ImageType2> source (rhs, 0.0f);

        size_t i = 0;
        for (size_t y = 0; y < ny; y++) {         // in-plane
          ps[1] = y;
          for (size_t x = 0; x < nx; x++, i++) {
            ps[0] = x;
            for (int s = -ssp.size(); s <= ssp.size(); s++) {       // ssp neighbourhood
              ps[2] = z+s;
              // get slice position in recon space
              ps2pr(ps, pr, Ts2r, finterp, peoffset, iJac);
              // update motion matrix
              source.voxel(pr);
              dst[i] += (ssp(s)*iJac) * source.value();
            }
          }
        }
      }

      template <typename ImageType1, typename VectorType2>
      void project_slice_y2x(const int v, const int z, ImageType1& dst, const VectorType2& rhs) const
      {
        std::unique_ptr<FieldInterpType> finterp;
        if (field.valid())
          finterp = make_unique<FieldInterpType>(field, 0.0f);

        Eigen::Vector3 ps, pr, peoffset;
        float iJac;
        transform_type Ts2r = get_Ts2r(v, z);
        if (field.valid()) {
          peoffset = pe.block<1,3>(v, 0).transpose().cast<double>();
        }

        Interp::CubicAdjoint<ImageType1> target (dst, 0.0f);

        size_t i = 0;
        for (size_t y = 0; y < ny; y++) {         // in-plane
          ps[1] = y;
          for (size_t x = 0; x < nx; x++, i++) {
            ps[0] = x;
            for (int s = -ssp.size(); s <= ssp.size(); s++) {       // ssp neighbourhood
              ps[2] = z+s;
              // get slice position in recon space
              ps2pr(ps, pr, Ts2r, finterp, peoffset, iJac);
              // update motion matrix
              target.voxel(pr);
              target.adjoint_add (ssp(s) * iJac * rhs[i]);
            }
          }
        }
      }

      template <typename ImageType1, typename ImageType2>
      void project_slice_x2x(const int v, const int z, ImageType1& dst, const ImageType2& rhs, const float w) const
      {
        std::unique_ptr<FieldInterpType> finterp;
        if (field.valid())
          finterp = make_unique<FieldInterpType>(field, 0.0f);

        Eigen::Vector3 ps, pr, peoffset;
        float t;
        float iJac;
        transform_type Ts2r = get_Ts2r(v, z);
        if (field.valid()) {
          peoffset = pe.block<1,3>(v, 0).transpose().cast<double>();
        }

        Interp::Cubic<ImageType2> source (rhs, 0.0f);
        Interp::CubicAdjoint<ImageType1> target (dst, 0.0f);

        for (size_t y = 0; y < ny; y++) {         // in-plane
          ps[1] = y;
          for (size_t x = 0; x < nx; x++) {
            ps[0] = x;
            t = 0.0f;
            for (int s = -ssp.size(); s <= ssp.size(); s++) {       // ssp neighbourhood
              ps[2] = z+s;
              // get slice position in recon space
              ps2pr(ps, pr, Ts2r, finterp, peoffset, iJac);
              // interpolate source
              source.voxel(pr);
              t += (ssp(s) * iJac) * source.value();
            }
            t *= w;
            for (int s = -ssp.size(); s <= ssp.size(); s++) {
              ps[2] = z+s;
              // get slice position in recon space
              ps2pr(ps, pr, Ts2r, finterp, peoffset, iJac);
              // project to target
              target.voxel(pr);
              target.adjoint_add (ssp(s) * iJac * t);
            }
          }
        }
      }

      inline void ps2pr(const Eigen::Vector3& ps, Eigen::Vector3& pr, const transform_type& Ts2r,
                        std::unique_ptr<FieldInterpType>& field, const Eigen::Vector3 pe, float& invjac) const
      {
        pr = Ts2r * ps;
        // clip pr to edges
        pr[0] = (pr[0] < 0) ? 0 : (pr[0] > nx-1) ? nx-1 : pr[0];
        pr[1] = (pr[1] < 0) ? 0 : (pr[1] > ny-1) ? ny-1 : pr[1];
        pr[2] = (pr[2] < 0) ? 0 : (pr[2] > nz-1) ? nz-1 : pr[2];
        // field mapping
        invjac = 1.0;
        if (field) {
          float B0 = 0.0f;
          Eigen::Vector3 p1 = pr;
          Eigen::RowVector3f dB0;
          // fixed point inversion
          for (int j = 0; j < 30; j++) {
            field->voxel(Tf * pr);
            field->value_and_gradient(B0, dB0);
            pr = Ts2r * (ps - B0 * pe);
            if ((p1 - pr).norm() < 0.1f) break;
            p1 = pr;
          }
          // Jacobian
          dB0 *= (Tf * Ts2r).rotation().cast<float>();
          invjac = 1.0 / std::max(0.1, 1. + 2. * dB0 * pe.cast<float>());
        }
      }

      void init_laplacian(const float lambda)
      {
        DEBUG("Initialising Laplacian matrix.");
        L.resize(nxy*nz, nxy*nz);
        L.reserve(Eigen::VectorXi::Constant(nxy*nz, 7));
        for (size_t z = 0; z < nz; z++) {
          for (size_t y = 0; y < ny; y++) {
            for (size_t x = 0; x < nx; x++) {
              L.coeffRef(get_idx(x, y, z), get_idx(x, y, (z) ? z-1 : 0)) += 1;
              L.coeffRef(get_idx(x, y, z), get_idx(x, (y) ? y-1 : 0, z)) += 1;
              L.coeffRef(get_idx(x, y, z), get_idx((x) ? x-1 : 0, y, z)) += 1;
              L.coeffRef(get_idx(x, y, z), get_idx(x, y, z)) += -6;
              L.coeffRef(get_idx(x, y, z), get_idx((x < nx-1) ? x+1 : nx-1, y, z)) += 1;
              L.coeffRef(get_idx(x, y, z), get_idx(x, (y < ny-1) ? y+1 : ny-1, z)) += 1;
              L.coeffRef(get_idx(x, y, z), get_idx(x, y, (z < nz-1) ? z+1 : nz-1)) += 1;
            }
          }
        }
        L *= std::sqrt(lambda);
        L.makeCompressed();
      }

    };


    }
  }
}


// Implementation of ReconMatrix * Eigen::DenseVector though a specialization of internal::generic_product_impl:
namespace Eigen {
  namespace internal {

    template<typename Rhs>
    struct generic_product_impl<MR::DWI::SVR::ReconMatrix, Rhs, SparseShape, DenseShape, GemvProduct>
      : generic_product_impl_base<MR::DWI::SVR::ReconMatrix,Rhs,generic_product_impl<MR::DWI::SVR::ReconMatrix,Rhs> >
    {
      typedef typename Product<MR::DWI::SVR::ReconMatrix,Rhs>::Scalar Scalar;

      template<typename Dest>
      static void scaleAndAddTo(Dest& dst, const MR::DWI::SVR::ReconMatrix& lhs, const Rhs& rhs, const Scalar& alpha)
      {
        // This method should implement "dst += alpha * lhs * rhs" inplace
        assert(alpha==Scalar(1) && "scaling is not implemented");

        lhs.project_x2x<Dest, Rhs>(dst, rhs);

      }

    };
  }
}


#endif


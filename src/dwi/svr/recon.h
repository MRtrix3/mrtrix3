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
#include "parallel_for.h"
#include "interp/linear.h"


namespace MR {
  namespace DWI {
    class ReconMatrix;
  }
}


namespace Eigen {
  namespace internal {
    // ReconMatrix inherits its traits from SparseMatrix
    template<>
    struct traits<MR::DWI::ReconMatrix> : public Eigen::internal::traits<Eigen::SparseMatrix<float,Eigen::RowMajor> >
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

      Eigen::Index rows() const { return nxy*nz*nc; }
      Eigen::Index cols() const { return nxy*nz*nc; }

      template<typename Rhs>
      Eigen::Product<ReconMatrix,Rhs,Eigen::AliasFreeProduct> operator*(const Eigen::MatrixBase<Rhs>& x) const {
        return Eigen::Product<ReconMatrix,Rhs,Eigen::AliasFreeProduct>(*this, x.derived());
      }

      typedef Eigen::SparseMatrix<float, Eigen::RowMajor, StorageIndex> SparseMat;
      typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMatrixXf;


      // Custom API:
      ReconMatrix(const Header& in, const Eigen::MatrixXf& rigid, const Eigen::MatrixXf& grad,
                  const int lmax, const vector<Eigen::MatrixXf>& rf, const SSP<float,2>& ssp, const float reg)
        : lmax (lmax),
          nx (in.size(0)), ny (in.size(1)), nz (in.size(2)), nv (in.size(3)),
          nxy (nx*ny), nc (get_ncoefs(rf)), ne (rigid.rows() / nv),
          T0 (in),  // Preserve original resolution.
          shellbasis (init_shellbasis(grad, rf)),
          ssp (ssp),
          motion (rigid)
      {
        INFO("Multiband factor " + str(nz/ne) + " detected.");
        init_Y(grad);
        init_laplacian(reg);
        assert (motion.rows() == nv*ne);
      }


      const RowMatrixXf& getY() const { return Y; }

      const Eigen::MatrixXf& getWeights() const { return W; }

      void setWeights(const Eigen::MatrixXf& weights) { W = weights; }

      const Eigen::MatrixXf& getShellBasis(const int shellidx) const { return shellbasis[shellidx]; }

      void setField(const Image<float>& fieldmap, const Eigen::MatrixXf& petable)
      {
        field = fieldmap;
        pe = petable;
        Tf = Transform(field).scanner2voxel * T0.voxel2scanner;
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
            Eigen::VectorXf q = X * Y.row(idx).adjoint();
            for (size_t z = idx%ne; z < nz; z += ne) {
              Eigen::Ref<Eigen::VectorXf> r = dst.segment((nz*v+z)*nxy, nxy);
              project_slice_x2y(v, z, r, q);
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
            Eigen::VectorXf r = Eigen::VectorXf::Zero(nxyz);
            for (size_t z = idx%ne; z < nz; z += ne) {
              project_slice_y2x(v, z, r, W(z,v) * rhs.segment((nz*v+z)*nxy, nxy));
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
            Eigen::VectorXf q = Xi * Y.row(idx).adjoint();
            Eigen::VectorXf r = Eigen::VectorXf::Zero(nxyz);
            for (size_t z = idx%ne; z < nz; z += ne) {
              project_slice_x2x(v, z, r, W(z, v) * q);
            }
            T.noalias() += r * Y.row(idx);
          }, zero);
        Xo += L.adjoint() * (L * Xi);
        Xo += 0.0001f * Xi;
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
            rot = get_rotation(motion(v*ne+e,3), motion(v*ne+e,4), motion(v*ne+e,5));
            // evaluate basis functions
            Math::SH::delta(delta, rot*vec, lmax);
            Y.row(v*ne+e) = shellbasis[idx[v]]*delta;
          }

        }

      }

      inline Eigen::Matrix3f get_rotation(const float a1, const float a2, const float a3) const
      {
        Eigen::Matrix3f m;
        m = Eigen::AngleAxisf(a1, Eigen::Vector3f::UnitZ())
          * Eigen::AngleAxisf(a2, Eigen::Vector3f::UnitY())
          * Eigen::AngleAxisf(a3, Eigen::Vector3f::UnitX());
        return m;
      }

      inline transform_type get_transform(const Eigen::VectorXf& p) const
      {
        transform_type T;
        T.translation() = Eigen::Vector3d( double(p[0]), double(p[1]), double(p[2]) );
        T.linear() = get_rotation(p[3], p[4], p[5]).template cast<double>();
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

      inline bool inbounds(const int x, const int y, const int z) const
      {
        return (x >= 0) && (x < nx)
            && (y >= 0) && (y < ny)
            && (z >= 0) && (z < nz);
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

      template <typename VectorType1, typename VectorType2>
      void project_slice_x2y(const size_t v, const size_t z, VectorType1& dst, const VectorType2& rhs) const
      {
        Eigen::SparseVector<float> m (nxy*nz);
        m.reserve(64);

        std::unique_ptr< Interp::Linear<decltype(field)> > finterp;
        if (field.valid())
          finterp = make_unique< Interp::Linear<decltype(field)> >(field, 0.0f);

        Eigen::Vector3 ps, pr, peoffset;
        float iJac;
        transform_type Ts2r = get_Ts2r(v, z);
        if (field.valid()) {
          peoffset = pe.block<1,3>(v, 0).transpose().cast<double>();
        }

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
              load_sparse_coefs(m, pr.cast<float>());
              dst[i] += (ssp(s)*iJac) * m.dot(rhs);
            }
          }
        }
      }

      template <typename VectorType1, typename VectorType2>
      void project_slice_y2x(const size_t v, const size_t z, VectorType1& dst, const VectorType2& rhs) const
      {
        Eigen::SparseVector<float> m (nxy*nz);
        m.reserve(64);

        std::unique_ptr< Interp::Linear<decltype(field)> > finterp;
        if (field.valid())
          finterp = make_unique< Interp::Linear<decltype(field)> >(field, 0.0f);

        Eigen::Vector3 ps, pr, peoffset;
        float iJac;
        transform_type Ts2r = get_Ts2r(v, z);
        if (field.valid()) {
          peoffset = pe.block<1,3>(v, 0).transpose().cast<double>();
        }

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
              load_sparse_coefs(m, pr.cast<float>());
              dst += (ssp(s) * iJac * rhs[i]) * m;
            }
          }
        }
      }

      template <typename VectorType1, typename VectorType2>
      void project_slice_x2x(const size_t v, const size_t z, VectorType1& dst, const VectorType2& rhs) const
      {
        Eigen::SparseVector<float> m0 (nxy*nz);
        m0.reserve(64);
        std::array<Eigen::SparseVector<float>, 5> m;
        m.fill(m0);

        std::unique_ptr< Interp::Linear<decltype(field)> > finterp;
        if (field.valid())
          finterp = make_unique< Interp::Linear<decltype(field)> >(field, 0.0f);

        Eigen::Vector3 ps, pr, peoffset;
        float t;
        std::array<float, 5> iJac;
        transform_type Ts2r = get_Ts2r(v, z);
        if (field.valid()) {
          peoffset = pe.block<1,3>(v, 0).transpose().cast<double>();
        }

        for (size_t y = 0; y < ny; y++) {         // in-plane
          ps[1] = y;
          for (size_t x = 0; x < nx; x++) {
            ps[0] = x;
            t = 0.0f;
            for (int s = -ssp.size(); s <= ssp.size(); s++) {       // ssp neighbourhood
              ps[2] = z+s;
              // get slice position in recon space
              ps2pr(ps, pr, Ts2r, finterp, peoffset, iJac[2+s]);
              // update motion matrix
              load_sparse_coefs(m[2+s], pr.cast<float>());
              t += (ssp(s) * iJac[2+s]) * m[2+s].dot(rhs);
            }
            for (int s = -2; s <= 2; s++) {
              dst += (ssp(s) * iJac[2+s] * t) * m[2+s];
            }
          }
        }
      }

      template <typename InterpolatorType>
      inline void ps2pr(const Eigen::Vector3& ps, Eigen::Vector3& pr, const transform_type& Ts2r,
                        std::unique_ptr<InterpolatorType>& field, const Eigen::Vector3 pe, float& invjac) const
      {
        pr = Ts2r * ps;
        invjac = 1.0;
        if (field) {
          float b0, b1 = 0.0f;
          // fixed point inversion
          for (int j = 0; j < 100; j++) {
            field->voxel(Tf * pr);
            b0 = field->value();
            pr = Ts2r * (ps - b0 * pe);
            if (std::fabs(b1-b0) < 0.1f) break;
            b1 = b0;
          }
          // approximate jacobian
          field->voxel(Tf * Ts2r * (ps + 0.5 * pe.normalized() - b0 * pe));
          double df = pe.norm() * field->value();
          field->voxel(Tf * Ts2r * (ps - 0.5 * pe.normalized() - b0 * pe));
          df -= pe.norm() * field->value();
          invjac = float(1.0 / (1.0 + df));
        }
      }

      inline void load_sparse_coefs(Eigen::SparseVector<float>& dst, const Eigen::Vector3f& pr) const
      {
        dst.setZero();
        int n = 2;
        Eigen::Vector3f pg = pr.array().ceil();
        std::array<float,4> wx = interpweights<3>(1.0f - (pg[0] - pr[0]));
        std::array<float,4> wy = interpweights<3>(1.0f - (pg[1] - pr[1]));
        std::array<float,4> wz = interpweights<3>(1.0f - (pg[2] - pr[2]));
        int px, py, pz;
        for (int rz = -n; rz < n; rz++) { // local neighbourhood interpolation
          pz = pg[2] + rz;
          if ((pz < 0) || (pz >= nz)) continue;
          for (int ry = -n; ry < n; ry++) {
            py = pg[1] + ry;
            if ((py < 0) || (py >= ny)) continue;
            for (int rx = -n; rx < n; rx++) {
              px = pg[0] + rx;
              if ((px < 0) || (px >= nx)) continue;
              // insert in weight vector.
              dst.insert(get_idx(px, py, pz)) = wx[n+rx] * wy[n+ry] * wz[n+rz];
            }
          }
        }
      }

      void init_laplacian(const float lambda)
      {
        DEBUG("Initialising Laplacian matrix.");
        L.resize(nxy*nz, nxy*nz);
        L.reserve(Eigen::VectorXi::Constant(nxy*nz, 7));
        for (size_t z = 1; z < nz-1; z++) {
          for (size_t y = 1; y < ny-1; y++) {
            for (size_t x = 1; x < nx-1; x++) {
              L.insert(get_idx(x, y, z), get_idx(x, y, z-1)) = -1;
              L.insert(get_idx(x, y, z), get_idx(x, y-1, z)) = -1;
              L.insert(get_idx(x, y, z), get_idx(x-1, y, z)) = -1;
              L.insert(get_idx(x, y, z), get_idx(x, y, z))   =  6;
              L.insert(get_idx(x, y, z), get_idx(x+1, y, z)) = -1;
              L.insert(get_idx(x, y, z), get_idx(x, y+1, z)) = -1;
              L.insert(get_idx(x, y, z), get_idx(x, y, z+1)) = -1;
            }
          }
        }
        L *= std::sqrt(lambda);
        L.makeCompressed();
      }

      inline Eigen::Vector3 wrap_around(const Eigen::Vector3 p) const
      {
        Eigen::Vector3 q;
        q[0] = std::fmod(p[0]+0.5, double(nx)) - 0.5;
        q[1] = std::fmod(p[1]+0.5, double(ny)) - 0.5;
        q[2] = std::fmod(p[2]+0.5, double(nz)) - 0.5;
        return q;
      }

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

        lhs.project_x2x<Dest, Rhs>(dst, rhs);

      }

    };
  }
}


#endif


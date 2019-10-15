/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#ifndef __dwi_svr_recon_h__
#define __dwi_svr_recon_h__


#include <Eigen/Dense>
#include <Eigen/Sparse>

#include "types.h"
#include "header.h"
#include "transform.h"
#include "parallel_for.h"
#include "interp/linear.h"
#include "interp/cubic.h"

#include "dwi/svr/param.h"
#include "dwi/svr/psf.h"
#include "dwi/svr/qspacebasis.h"


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

      Eigen::Index rows() const { return nxy*nz*nv + 2*nxy*nz*nc; }
      Eigen::Index cols() const { return nxy*nz*nc; }

      template<typename Rhs>
      Eigen::Product<ReconMatrix,Rhs,Eigen::AliasFreeProduct> operator*(const Eigen::MatrixBase<Rhs>& x) const {
        return Eigen::Product<ReconMatrix,Rhs,Eigen::AliasFreeProduct>(*this, x.derived());
      }

      typedef Eigen::SparseMatrix<float, Eigen::RowMajor, StorageIndex> SparseMat;
      typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMatrixXf;

      typedef Interp::LinearInterp<Image<float>, Interp::LinearInterpProcessingType::ValueAndDerivative> FieldInterpType;


      // Custom API:
      ReconMatrix(const Header& in, const Eigen::MatrixXf& rigid, const QSpaceBasis& basis,
                  const SSP<float,2>& ssp, const float reg, const float zreg)
        : nx (in.size(0)), ny (in.size(1)), nz (in.size(2)), nv (in.size(3)),
          nxy (nx*ny), nc (basis.get_ncoefs()), ne (rigid.rows() / nv),
          T0 (in), qbasis (basis),
          ssp (ssp), motion (rigid), htmp(in)
      {
        INFO("Multiband factor " + str(nz/ne) + " detected.");

        float scale = std::sqrt(1.0f * nv);
        init_laplacian(scale*reg);
        init_zreg(scale*zreg);
        assert (motion.rows() == nv*ne);

        // set header for temporary images.
        htmp.ndim() = 3;
        htmp.stride(0) = 1;
        htmp.stride(1) = 2;
        htmp.stride(2) = 3;
        htmp.sanitise();
      }

      ReconMatrixAdjoint adjoint() const;

      const Eigen::MatrixXf& getWeights() const        { return W; }
      void setWeights (const Eigen::MatrixXf& weights) { W = weights; }

      void setField(const Image<float>& fieldmap, const size_t fieldidx, const Eigen::MatrixXf& petable)
      {
        field = fieldmap;
        pe = petable;
        Tf = Transform(field).scanner2voxel
            * get_transform(motion.block(fieldidx*ne,0,ne,6).colwise().mean()).inverse()
            * T0.voxel2scanner;
      }

      template <typename VectorType1, typename VectorType2>
      void project(VectorType1& dst, const VectorType2& rhs, bool useweights = true) const
      {
        INFO("Forward projection.");
        size_t nxyz = nxy*nz;
        Eigen::Map<const RowMatrixXf> X (rhs.data(), nxyz, nc);
        Thread::parallel_for<size_t>(0, nv*ne,
          [&](size_t idx) {
            size_t v = idx/ne;
            auto tmp = Image<float>::scratch(htmp);
            Eigen::Map<Eigen::VectorXf> q (tmp.address(), nxyz);
            q.noalias() = X * qbasis.get_projection(idx);
            for (size_t z = idx%ne; z < nz; z += ne) {
              Eigen::Ref<Eigen::VectorXf> r = dst.segment((nz*v+z)*nxy, nxy);
              project_slice_x2y(v, z, r, tmp);
              if (useweights) r *= std::sqrt(W(z,v));
            }
          });
        INFO("Forward projection - regularisers");
        Eigen::Ref<Eigen::VectorXf> ref1 = dst.segment(nxyz*nv, nxyz*nc);
        Eigen::Map<RowMatrixXf> Yreg1 (ref1.data(), nxyz, nc);
        Yreg1.noalias() += L * X;
        Eigen::Ref<Eigen::VectorXf> ref2 = dst.segment(nxyz*(nv+nc), nxyz*nc);
        Eigen::Map<RowMatrixXf> Yreg2 (ref2.data(), nxyz, nc);
        Yreg2.noalias() += Z * X;
      }


    private:
      const size_t nx, ny, nz, nv, nxy, nc, ne;
      const Transform T0;
      const QSpaceBasis qbasis;
      const SSP<float,2> ssp;

      RowMatrixXf motion;
      Eigen::MatrixXf W;
      SparseMat L, Z;

      Header htmp;

      Image<float> field;
      Eigen::MatrixXf pe;
      transform_type Tf;

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
        DEBUG("Initialising Laplacian regularizer.");
        // Regularization convolution filter set as Laplacian filter.
        Eigen::Matrix<Scalar, 2, 1> D;
        D << -6, 1;
        D *= lambda;

        L.resize(nxy*nz, nxy*nz);
        L.reserve(Eigen::VectorXi::Constant(nxy*nz, 7));
        for (size_t z = 0; z < nz; z++) {
          for (size_t y = 0; y < ny; y++) {
            for (size_t x = 0; x < nx; x++) {
              L.coeffRef(get_idx(x, y, z), get_idx(x, y, z)) += D[0];

              L.coeffRef(get_idx(x, y, z), get_idx(x, y, (z) ? z-1 : 0)) += D[1];
              L.coeffRef(get_idx(x, y, z), get_idx(x, (y) ? y-1 : 0, z)) += D[1];
              L.coeffRef(get_idx(x, y, z), get_idx((x) ? x-1 : 0, y, z)) += D[1];
              L.coeffRef(get_idx(x, y, z), get_idx((x < nx-1) ? x+1 : nx-1, y, z)) += D[1];
              L.coeffRef(get_idx(x, y, z), get_idx(x, (y < ny-1) ? y+1 : ny-1, z)) += D[1];
              L.coeffRef(get_idx(x, y, z), get_idx(x, y, (z < nz-1) ? z+1 : nz-1)) += D[1];

            }
          }
        }
        L.makeCompressed();
      }

      void init_zreg(const float lambda)
      {
        DEBUG("Initialising slice regularizer.");
        Eigen::Matrix<Scalar, 5, 1> D;
        D << 70, -56, 28, -8, 1;
        D *= lambda;

        Z.resize(nxy*nz, nxy*nz);
        Z.reserve(Eigen::VectorXi::Constant(nxy*nz, 9));
        for (size_t z = 0; z < nz; z++) {
          for (size_t y = 0; y < ny; y++) {
            for (size_t x = 0; x < nx; x++) {
              Z.coeffRef(get_idx(x, y, z), get_idx(x, y, z)) += D[0];

              Z.coeffRef(get_idx(x, y, z), get_idx(x, y, (z) ? z-1 : 0)) += D[1];
              Z.coeffRef(get_idx(x, y, z), get_idx(x, y, (z < nz-1) ? z+1 : nz-1)) += D[1];

              Z.coeffRef(get_idx(x, y, z), get_idx(x, y, (z > 1) ? z-2 : 0)) += D[2];
              Z.coeffRef(get_idx(x, y, z), get_idx(x, y, (z < nz-2) ? z+2 : nz-1)) += D[2];

              Z.coeffRef(get_idx(x, y, z), get_idx(x, y, (z > 2) ? z-3 : 0)) += D[3];
              Z.coeffRef(get_idx(x, y, z), get_idx(x, y, (z < nz-3) ? z+3 : nz-1)) += D[3];

              Z.coeffRef(get_idx(x, y, z), get_idx(x, y, (z > 3) ? z-4 : 0)) += D[4];
              Z.coeffRef(get_idx(x, y, z), get_idx(x, y, (z < nz-4) ? z+4 : nz-1)) += D[4];
            }
          }
        }
        Z.makeCompressed();
      }

      friend class ReconMatrixAdjoint;
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
        IsRowMajor = false
      };

      Eigen::Index rows() const { return recmat.cols(); }
      Eigen::Index cols() const { return recmat.rows(); }

      template<typename Rhs>
      Eigen::Product<ReconMatrixAdjoint,Rhs,Eigen::AliasFreeProduct> operator*(const Eigen::MatrixBase<Rhs>& x) const {
        return Eigen::Product<ReconMatrixAdjoint,Rhs,Eigen::AliasFreeProduct>(*this, x.derived());
      }

      using RowMatrixXf = ReconMatrix::RowMatrixXf;
      using FieldInterpType = ReconMatrix::FieldInterpType;


      // Custom API:
      ReconMatrixAdjoint(const ReconMatrix& m)
        : recmat (m),
          nx (m.nx), ny (m.ny), nz (m.nz), nv (m.nv), nxy (nx*ny), nc (m.nc), ne (m.ne)
      { }

      const ReconMatrix& adjoint() const
      {
        return recmat;
      }

      template <typename VectorType1, typename VectorType2>
      void project(VectorType1& dst, const VectorType2& rhs, bool useweights = true) const
      {
        INFO("Transpose projection.");
        size_t nxyz = nxy*nz;
        Eigen::Map<RowMatrixXf> X (dst.data(), nxyz, nc);
        RowMatrixXf zero (nxyz, nc); zero.setZero();
        X = Thread::parallel_sum<RowMatrixXf, size_t>(0, nv*ne,
          [&](size_t idx, RowMatrixXf& T) {
            size_t v = idx/ne;
            auto tmp = Image<float>::scratch(recmat.htmp);
            Eigen::Map<Eigen::VectorXf> r (tmp.address(), nxyz);
            r.setZero();
            for (size_t z = idx%ne; z < nz; z += ne) {
              float ww = (useweights) ? std::sqrt(recmat.W(z,v)) : 1.0f;
              project_slice_y2x(v, z, tmp, rhs.segment((nz*v+z)*nxy, nxy), ww);
            }
            Eigen::RowVectorXf qr = recmat.qbasis.get_projection(idx);
            for (size_t j = 0; j < nxyz; j++) {
              if (r[j] != 0.0f) T.row(j) += r[j] * qr;
            }
          }, zero);
        INFO("Transpose projection - regularisers");
        Eigen::Ref<const Eigen::VectorXf> ref1 = rhs.segment(nxyz*nv, nxyz*nc);
        Eigen::Map<const RowMatrixXf> Yreg1 (ref1.data(), nxyz, nc);
        X.noalias() += recmat.L.adjoint() * Yreg1;
        Eigen::Ref<const Eigen::VectorXf> ref2 = rhs.segment(nxyz*(nv+nc), nxyz*nc);
        Eigen::Map<const RowMatrixXf> Yreg2 (ref2.data(), nxyz, nc);
        X.noalias() += recmat.Z.adjoint() * Yreg2;
      }

    private:
      const ReconMatrix& recmat;
      const size_t nx, ny, nz, nv, nxy, nc, ne;


      template <typename ImageType1, typename VectorType2>
      void project_slice_y2x(const int v, const int z, ImageType1& dst, const VectorType2& rhs, const float weight) const
      {
        std::unique_ptr<FieldInterpType> finterp;
        if (recmat.field.valid())
          finterp = make_unique<FieldInterpType>(recmat.field, 0.0f);

        Eigen::Vector3 ps, pr, peoffset;
        float iJac;
        transform_type Ts2r = recmat.get_Ts2r(v, z);
        if (recmat.field.valid()) {
          peoffset = recmat.pe.block<1,3>(v, 0).transpose().cast<double>();
        }

        Interp::CubicAdjoint<ImageType1> target (dst, 0.0f);

        size_t i = 0;
        for (size_t y = 0; y < ny; y++) {         // in-plane
          ps[1] = y;
          for (size_t x = 0; x < nx; x++, i++) {
            ps[0] = x;
            float tmp = weight * rhs[i];
            for (int s = -recmat.ssp.size(); s <= recmat.ssp.size(); s++) {       // ssp neighbourhood
              ps[2] = z+s;
              // get slice position in recon space
              recmat.ps2pr(ps, pr, Ts2r, finterp, peoffset, iJac);
              // update motion matrix
              target.voxel(pr);
              target.adjoint_add (recmat.ssp(s) * iJac * tmp);
            }
          }
        }
      }

    };

    ReconMatrixAdjoint ReconMatrix::adjoint() const
    {
      return ReconMatrixAdjoint(*this);
    }


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

        lhs.project<Dest, Rhs>(dst, rhs);

      }
    };

    template<typename Rhs>
    struct generic_product_impl<MR::DWI::SVR::ReconMatrixAdjoint, Rhs, SparseShape, DenseShape, GemvProduct>
      : generic_product_impl_base<MR::DWI::SVR::ReconMatrixAdjoint,Rhs,generic_product_impl<MR::DWI::SVR::ReconMatrixAdjoint,Rhs> >
    {
      typedef typename Product<MR::DWI::SVR::ReconMatrixAdjoint,Rhs>::Scalar Scalar;

      template<typename Dest>
      static void scaleAndAddTo(Dest& dst, const MR::DWI::SVR::ReconMatrixAdjoint& lhs, const Rhs& rhs, const Scalar& alpha)
      {
        // This method should implement "dst += alpha * lhs * rhs" inplace
        assert(alpha==Scalar(1) && "scaling is not implemented");

        lhs.project<Dest, Rhs>(dst, rhs);

      }
    };

  }
}


#endif


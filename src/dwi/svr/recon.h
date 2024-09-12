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
#include "image.h"

#include "dwi/svr/mapping.h"


namespace MR
{
  template <typename ValueType>
  class ImageView : public ImageBase<ImageView<ValueType>, ValueType>
  {
    public:
      using value_type = ValueType;

      ImageView (const Header& hdr, ValueType* data)
        : templatehdr (hdr), data_pointer (data), x (hdr.ndim(), 0),
          strides (Stride::get(hdr)), data_offset (Stride::offset(hdr))
      {
        DEBUG ("image view \"" + name() + "\" initialised with strides = " + str(strides) + ", start = " + str(data_offset));
      }

      FORCE_INLINE bool valid () const { return data_pointer; }
      FORCE_INLINE bool operator! () const { return !valid(); }

      FORCE_INLINE const std::map<std::string, std::string>& keyval () const { return templatehdr.keyval(); }

      FORCE_INLINE const std::string& name() const { return templatehdr.name(); }
      FORCE_INLINE const transform_type& transform() const { return templatehdr.transform(); }

      FORCE_INLINE size_t  ndim () const { return templatehdr.ndim(); }
      FORCE_INLINE ssize_t size (size_t axis) const { return templatehdr.size (axis); }
      FORCE_INLINE default_type spacing (size_t axis) const { return templatehdr.spacing (axis); }
      FORCE_INLINE ssize_t stride (size_t axis) const { return strides[axis]; }

      FORCE_INLINE size_t offset () const { return data_offset; }

      FORCE_INLINE void reset () {
        for (size_t n = 0; n < ndim(); ++n)
          this->index(n) = 0;
      }

      FORCE_INLINE ssize_t get_index (size_t axis) const { return x[axis]; }
      FORCE_INLINE void move_index (size_t axis, ssize_t increment) { data_offset += stride (axis) * increment; x[axis] += increment; }

      FORCE_INLINE bool is_direct_io () const { return true; }

      FORCE_INLINE ValueType get_value () const { return data_pointer[data_offset]; }
      FORCE_INLINE void set_value (ValueType val) { data_pointer[data_offset] = val; }

      //! return RAM address of current voxel
      FORCE_INLINE ValueType* address () const {
        return static_cast<ValueType*>(data_pointer) + data_offset;
      }

    protected:
      const Header& templatehdr;    // template image header
      value_type* data_pointer;     // pointer to data address
      std::vector<ssize_t> x;
      Stride::List strides;
      size_t data_offset;
  };

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
    {
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

      Eigen::Index rows() const { return map.rows() + 2*map.cols(); }
      Eigen::Index cols() const { return map.cols(); }

      template<typename Rhs>
      Eigen::Product<ReconMatrix,Rhs,Eigen::AliasFreeProduct> operator*(const Eigen::MatrixBase<Rhs>& x) const {
        return Eigen::Product<ReconMatrix,Rhs,Eigen::AliasFreeProduct>(*this, x.derived());
      }

      typedef Eigen::SparseMatrix<float, Eigen::RowMajor, StorageIndex> SparseMat;
      typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMatrixXf;

      // Custom API:
      ReconMatrix(const ReconMapping& map, const float reg, const float zreg)
        : map (map)
      {
        size_t nz = map.yheader().size(2);
        size_t nv = map.yheader().size(3);
        W.resize(nz,nv); W.setOnes();
        Wvox.resize(voxel_count(map.yheader())); Wvox.setOnes();
        float scale = std::sqrt(1.0f * nv);
        init_laplacian(scale*reg);
        init_zreg(scale*zreg);
      }

      ReconMatrixAdjoint adjoint() const;

      const Eigen::MatrixXf& getWeights() const        { return W; }
      void setWeights (const Eigen::MatrixXf& weights) { W = weights; }

      void setVoxelWeights(const Eigen::VectorXf& weights) { Wvox = weights; }

      template <typename VectorType1, typename VectorType2>
      void project(VectorType1& dst, const VectorType2& rhs, bool useweights = true) const
      {
        INFO("Forward projection.");
        Eigen::VectorXf copy = rhs;
        ImageView<float> recon (map.xheader(), copy.data());
        ImageView<float> source (map.yheader(), dst.data());
        map.x2y(recon, source);
        if (useweights) {
          size_t j = 0;
          for (auto l = Loop() (source); l; l++, j++)
            source.value() *= std::sqrt(W((size_t) source.index(2), (size_t) source.index(3)) * Wvox[j]);
        }
        INFO("Forward projection - regularisers");
        size_t nxyz = recon.size(0)*recon.size(1)*recon.size(2);
        size_t nc = recon.size(3);
        Eigen::Map<const RowMatrixXf> X (rhs.data(), nxyz, nc);
        Eigen::Ref<Eigen::VectorXf> ref1 = dst.segment(map.rows(), map.cols());
        Eigen::Map<RowMatrixXf> Yreg1 (ref1.data(), nxyz, nc);
        Yreg1.noalias() += L * X;
        Eigen::Ref<Eigen::VectorXf> ref2 = dst.segment(map.rows()+map.cols(), map.cols());
        Eigen::Map<RowMatrixXf> Yreg2 (ref2.data(), nxyz, nc);
        Yreg2.noalias() += Z * X;
      }


    private:
      const ReconMapping& map;
      Eigen::MatrixXf W;
      Eigen::VectorXf Wvox;
      SparseMat L, Z;

      void init_laplacian(const float lambda)
      {
        DEBUG("Initialising Laplacian regularizer.");
        // Regularization convolution filter set as Laplacian filter.
        Eigen::Matrix<Scalar, 2, 1> D;
        D << -6, 1;
        D *= lambda;

        size_t nx = map.xheader().size(0);
        size_t ny = map.xheader().size(1);
        size_t nz = map.xheader().size(2);
        size_t nxy = nx*ny, nxyz = nxy*nz;
        auto get_idx = [&] (size_t x, size_t y, size_t z) { return z*nxy + y*nx + x; };

        L.resize(nxyz, nxyz);
        L.reserve(Eigen::VectorXi::Constant(nxyz, 7));
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

        size_t nx = map.xheader().size(0);
        size_t ny = map.xheader().size(1);
        size_t nz = map.xheader().size(2);
        size_t nxy = nx*ny, nxyz = nxy*nz;
        auto get_idx = [&] (size_t x, size_t y, size_t z) { return z*nxy + y*nx + x; };

        Z.resize(nxyz, nxyz);
        Z.reserve(Eigen::VectorXi::Constant(nxyz, 9));
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

      Eigen::Index rows() const { return recmat.cols(); }
      Eigen::Index cols() const { return recmat.rows(); }

      template<typename Rhs>
      Eigen::Product<ReconMatrixAdjoint,Rhs,Eigen::AliasFreeProduct> operator*(const Eigen::MatrixBase<Rhs>& x) const {
        return Eigen::Product<ReconMatrixAdjoint,Rhs,Eigen::AliasFreeProduct>(*this, x.derived());
      }

      using RowMatrixXf = ReconMatrix::RowMatrixXf;

      // Custom API:
      ReconMatrixAdjoint(const ReconMatrix& m)
        : recmat (m), map (m.map)
      { }

      const ReconMatrix& adjoint() const { return recmat; }

      template <typename VectorType1, typename VectorType2>
      void project(VectorType1& dst, const VectorType2& rhs, bool useweights = true) const
      {
        INFO("Transpose projection.");
        ImageView<float> recon (map.xheader(), dst.data());
        Eigen::VectorXf copy = rhs;  // temporary for weighted input
        ImageView<float> source (map.yheader(), copy.data());
        if (useweights) {
          size_t j = 0;
          for (auto l = Loop() (source); l; l++, j++)
            source.value() *= std::sqrt(recmat.W((size_t) source.index(2), (size_t) source.index(3)) * recmat.Wvox[j]);
        }
        map.y2x(recon, source);
        INFO("Transpose projection - regularisers");
        size_t nxyz = recon.size(0)*recon.size(1)*recon.size(2);
        size_t nc = recon.size(3);
        Eigen::Map<RowMatrixXf> X (dst.data(), nxyz, nc);
        Eigen::Ref<const Eigen::VectorXf> ref1 = rhs.segment(map.rows(), map.cols());
        Eigen::Map<const RowMatrixXf> Yreg1 (ref1.data(), nxyz, nc);
        X.noalias() += recmat.L.adjoint() * Yreg1;
        Eigen::Ref<const Eigen::VectorXf> ref2 = rhs.segment(map.rows()+map.cols(), map.cols());
        Eigen::Map<const RowMatrixXf> Yreg2 (ref2.data(), nxyz, nc);
        X.noalias() += recmat.Z.adjoint() * Yreg2;
      }

    private:
      const ReconMatrix& recmat;
      const ReconMapping& map;

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


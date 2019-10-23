/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#ifndef __dwi_svr_mapping_h__
#define __dwi_svr_mapping_h__


#include <Eigen/Dense>
#include <Eigen/Sparse>

#include "types.h"
#include "header.h"
#include "transform.h"
#include "adapter/base.h"
#include "math/SH.h"
#include "dwi/shells.h"
#include "interp/linear.h"
#include "interp/cubic.h"
#include "algo/threaded_loop.h"

#include "dwi/svr/param.h"
#include "dwi/svr/psf.h"
#include "dwi/svr/qspacebasis.h"


namespace MR
{
  namespace Interp
  {
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
                ImageType::adjoint_add(weights_vec[i++] * val);
              }
            }
          }
        }
    };
  }

  namespace DWI
  {
    namespace SVR
    {

      template <class ImageType>
      class MotionMapping : public Adapter::Base<MotionMapping<ImageType>, ImageType>
      {
        MEMALIGN (MotionMapping<ImageType>)
        public:
          using base_type = Adapter::Base<MotionMapping<ImageType>, ImageType>;
          using value_type = typename ImageType::value_type;
          using vector_type = typename Eigen::Matrix<value_type, Eigen::Dynamic, 1>;

          using base_type::parent;

          MotionMapping (const ImageType& projection, const Header& source,
                         const Eigen::MatrixXf& rigid)
            : base_type (projection),
              interp (projection, 0.0f), yhdr (source), motion (rigid),
              Tr (projection), Ts (source), Ts2r (Ts.scanner2voxel * Tr.voxel2scanner)
          { }

          // Adapter attributes -----------------------------------------------
          size_t ndim () const { return interp.ndim(); }
          int size (size_t axis) const { return (axis < 3) ? yhdr.size(axis) : interp.size(axis); }
          default_type spacing (size_t axis) const { return (axis < 3) ? yhdr.spacing(axis) : interp.spacing(axis); }
          const transform_type& transform () const { return yhdr.transform(); }
          const std::string& name () const { return yhdr.name(); }

          ssize_t get_index (size_t axis) const {
            return (axis < 3) ? x[axis] : interp.index(axis);
          }
          void move_index (size_t axis, ssize_t increment) {
            if (axis < 3) x[axis] += increment;
            else interp.index(axis) += increment;
          }
          void reset () {
            x[0] = x[1] = x[2] = 0;
            for (size_t n = 3; n < interp.ndim(); ++n)
              interp.index(n) = 0;
          }
          // ------------------------------------------------------------------

          value_type value () {
            Eigen::Vector3 pr = Ts2r * Eigen::Vector3 (x[0], x[1], x[2]);
            for (int k = 0; k < 3; k++) pr[k] = clampdim(pr[k], k);
            interp.voxel (pr);
            return interp.value();
          }

          void adjoint_add (value_type val) {
            Eigen::Vector3 pr = Ts2r * Eigen::Vector3 (x[0], x[1], x[2]);
            for (int k = 0; k < 3; k++) pr[k] = clampdim(pr[k], k);
            interp.voxel (pr);
            interp.adjoint_add(val);
          }

          void set_shotidx (size_t idx) {
            interp.set_shotidx(idx);
            Ts2r = Ts.scanner2voxel * get_transform(motion.row(idx)) * Tr.voxel2scanner;
          }

        private:
          Interp::CubicAdjoint<ImageType> interp;
          const Header& yhdr;
          Eigen::MatrixXf motion;
          ssize_t x[3];
          const Transform Tr, Ts;
          transform_type Ts2r;    // vox-to-vox transform, mapping vectors in source space to recon space

          FORCE_INLINE transform_type get_transform(const Eigen::VectorXf& p) const {
            transform_type T (se3exp(p).cast<double>());
            return T;
          }

          FORCE_INLINE default_type clampdim (default_type r, size_t axis) const {
            return (r < 0) ? 0 : (r > size(axis)-1) ? size(axis)-1 : r;
          }

      };


      class ReconMapping
      {
        MEMALIGN(ReconMapping);
        public:
          ReconMapping(const Header& recon, const Header& source, const QSpaceBasis& basis,
                       const Eigen::MatrixXf& rigid, const SSP<float,2>& ssp)
            : xhdr (recon), yhdr (source), ne (rigid.rows() / source.size(3)),
              outer_axes ({2,3}), slice_axes ({0,1}),
              qbasis (basis), motion (rigid), ssp (ssp)
          {
            INFO("Multiband factor " + str(source.size(2)/ne) + " detected.");
          }

          const Header& xheader() const { return xhdr; }
          const Header& yheader() const { return yhdr; }

          size_t rows() const { return voxel_count(yhdr); }
          size_t cols() const { return voxel_count(xhdr); }


          template <typename ImageType1, typename ImageType2>
          void x2y(const ImageType1& X, ImageType2& Y) const
          {
            // create adapters
            auto qmap = Adapter::makecached<QSpaceMapping> (X, qbasis);
            auto spatialmap = Adapter::make<MotionMapping> (qmap, yhdr, motion);

            // define per-slice mapping
            struct MapSliceX2Y {   MEMALIGN(MapSliceX2Y);
              ImageType2 out;
              decltype(spatialmap) pred;
              size_t ne;
              const vector<size_t>& axouter;
              const vector<size_t>& axslice;
              // define slice-wise operation
              void operator() (Iterator& pos) {
                size_t z = pos.index(2);
                size_t v = pos.index(3);
                if (z < ne) {
                  assign_pos_of (pos, axouter).to (out);
                  pred.set_shotidx(v*ne+z%ne);
                  for (size_t zz = z; zz < out.size(2); zz += ne) {
                    out.index(2) = pred.index(2) = zz;
                    for (auto i = Loop(axslice) (out, pred); i; ++i)
                      out.value() += pred.value();
                  }
                }
              }
            } func = {Y, spatialmap, ne, outer_axes, slice_axes};

            // run across all slices
            ThreadedLoop ("forward projection", Y, outer_axes, slice_axes)
              .run_outer (func);
          }

          template <typename ImageType1, typename ImageType2>
          void y2x(ImageType1& X, const ImageType2& Y) const
          {
            // create adapters
            auto qmap = Adapter::makecached_add<QSpaceMapping> (X, qbasis);
            auto spatialmap = Adapter::make<MotionMapping> (qmap, yhdr, motion);

            // define per-slice mapping
            struct MapSliceY2X {   MEMALIGN(MapSliceY2X);
              ImageType2 in;
              decltype(spatialmap) pred;
              size_t ne;
              const vector<size_t>& axouter;
              const vector<size_t>& axslice;
              // define slice-wise operation
              void operator() (Iterator& pos) {
                size_t z = pos.index(2);
                size_t v = pos.index(3);
                if (z < ne) {
                  assign_pos_of (pos, axouter).to (in);
                  pred.set_shotidx(v*ne+z%ne);
                  for (size_t zz = z; zz < in.size(2); zz += ne) {
                    in.index(2) = pred.index(2) = zz;
                    for (auto i = Loop(axslice) (in, pred); i; ++i)
                      pred.adjoint_add (in.value());
                  }
                  pred.set_shotidx(0); // trigger delayed write back
                }
              }
            } func = {Y, spatialmap, ne, outer_axes, slice_axes};

            // run across all slices
            ThreadedLoop ("transpose projection", Y, outer_axes, slice_axes)
              .run_outer (func);
          }

        private:
          const Header& xhdr, yhdr;
          const size_t ne;
          const vector<size_t> outer_axes;
          const vector<size_t> slice_axes;

          const QSpaceBasis qbasis;
          const Eigen::MatrixXf motion;
          const SSP<float,2> ssp;

      };


//      class ReconMapping
//      {
//        MEMALIGN(ReconMapping);
//        public:
//          typedef Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMatrixXf;
//          typedef Interp::LinearInterp<Image<float>, Interp::LinearInterpProcessingType::ValueAndDerivative> FieldInterpType;

//          ReconMapping(const Header& in, const Eigen::MatrixXf& rigid, const SSP<float,2>& ssp)
//            : nx (in.size(0)), ny (in.size(1)), nz (in.size(2)), nv (in.size(3)),
//              nxy (nx*ny), ne (rigid.rows() / nv),
//              T0 (in),  // Preserve original resolution.
//              ssp (ssp), motion (rigid), htmp(in)
//          {
//            INFO("Multiband factor " + str(nz/ne) + " detected.");

//            // set header for temporary images.
//            htmp.ndim() = 3;
//            htmp.stride(0) = 1;
//            htmp.stride(1) = 2;
//            htmp.stride(2) = 3;
//            htmp.sanitise();
//          }

//          void setField(const Image<float>& fieldmap, const size_t fieldidx, const Eigen::MatrixXf& petable)
//          {
//            field = fieldmap;
//            pe = petable;
//            Tf = Transform(field).scanner2voxel
//                * get_transform(motion.block(fieldidx*ne,0,ne,6).colwise().mean()).inverse()
//                * T0.voxel2scanner;
//          }

//          template <typename ImageType1, typename ImageType2>
//          void x2y(const ImageType1& in, const ImageType2& out) const
//          {
//            adapter_type map_x2y {};
//            struct MapSliceX2Y {   MEMALIGN(MapSliceX2Y);
//              ImageType2 out;
//              adapter_type pred;
//              void operator() (Iterator& pos) {
//                size_t z = pos.index(2);
//                size_t v = pos.index(3);
//                assign_pos_of (pos, {2,3}).to (out, pred);
//                pred.set_idx(z, v);
//                for (auto i = Loop({0,1}) (out, pred); i; ++i) {
//                  out.value() += pred.value();
//                }
//              }
//            } func = {out, map_x2y};

//            ThreadedLoop ("forward projection", in, {2,3}, {0,1})
//              .run_outer (func);
//          }


//          template <typename VectorType1, typename VectorType2>
//          void x2y(VectorType1& dst, const VectorType2& rhs) const
//          {
//            INFO("Forward projection.");
//            size_t nxyz = nxy*nz;
//            Eigen::Map<const RowMatrixXf> X (rhs.data(), nxyz, nc);
//            Thread::parallel_for<size_t>(0, nv*ne,
//              [&](size_t idx) {
//                size_t v = idx/ne;
//                auto tmp = Image<float>::scratch(htmp);
//                Eigen::Map<Eigen::VectorXf> q (tmp.address(), nxyz);
//                q.noalias() = X * Y.row(idx).adjoint();
//                for (size_t z = idx%ne; z < nz; z += ne) {
//                  Eigen::Ref<Eigen::VectorXf> r = dst.segment((nz*v+z)*nxy, nxy);
//                  project_slice_x2y(v, z, r, tmp);
//                }
//              });
//          }

//          template <typename VectorType1, typename VectorType2>
//          void y2x(VectorType1& dst, const VectorType2& rhs) const
//          {
//            INFO("Transpose projection.");
//            size_t nxyz = nxy*nz;
//            Eigen::Map<RowMatrixXf> X (dst.data(), nxyz, nc);
//            RowMatrixXf zero (nxyz, nc); zero.setZero();
//            X = Thread::parallel_sum<RowMatrixXf, size_t>(0, nv*ne,
//              [&](size_t idx, RowMatrixXf& T) {
//                size_t v = idx/ne;
//                auto tmp = Image<float>::scratch(recmat.htmp);
//                Eigen::Map<Eigen::VectorXf> r (tmp.address(), nxyz);
//                r.setZero();
//                for (size_t z = idx%ne; z < nz; z += ne) {
//                  project_slice_y2x(v, z, tmp, rhs.segment((nz*v+z)*nxy, nxy), 1.0f);
//                }
//                Eigen::RowVectorXf qr = recmat.Y.row(idx);
//                for (size_t j = 0; j < nxyz; j++) {
//                  if (r[j] != 0.0f) T.row(j) += r[j] * qr;
//                }
//              }, zero);
//          }


//        private:
//          const size_t nx, ny, nz, nv, nxy, nc, ne;
//          const Transform T0;
//          const SSP<float,2> ssp;

//          RowMatrixXf motion;

//          Header htmp;

//          Image<float> field;
//          Eigen::MatrixXf pe;
//          transform_type Tf;



//          inline transform_type get_transform(const Eigen::VectorXf& p) const
//          {
//            transform_type T (se3exp(p).cast<double>());
//            return T;
//          }

//          inline transform_type get_Ts2r(const size_t v, const size_t z) const
//          {
//            transform_type Ts2r = T0.scanner2voxel * get_transform(motion.row(v*ne+z%ne)) * T0.voxel2scanner;
//            return Ts2r;
//          }

//          inline size_t get_idx(const int x, const int y, const int z) const
//          {
//            return size_t(z*nxy + y*nx + x);
//          }


//          template <typename VectorType1, typename ImageType2>
//          void project_slice_x2y(const int v, const int z, VectorType1& dst, const ImageType2& rhs) const
//          {
//            std::unique_ptr<FieldInterpType> finterp;
//            if (field.valid())
//              finterp = make_unique<FieldInterpType>(field, 0.0f);

//            Eigen::Vector3 ps, pr, peoffset;
//            float iJac;
//            transform_type Ts2r = get_Ts2r(v, z);
//            if (field.valid()) {
//              peoffset = pe.block<1,3>(v, 0).transpose().cast<double>();
//            }

//            Interp::Cubic<ImageType2> source (rhs, 0.0f);

//            size_t i = 0;
//            for (size_t y = 0; y < ny; y++) {         // in-plane
//              ps[1] = y;
//              for (size_t x = 0; x < nx; x++, i++) {
//                ps[0] = x;
//                for (int s = -ssp.size(); s <= ssp.size(); s++) {       // ssp neighbourhood
//                  ps[2] = z+s;
//                  // get slice position in recon space
//                  ps2pr(ps, pr, Ts2r, finterp, peoffset, iJac);
//                  // update motion matrix
//                  source.voxel(pr);
//                  dst[i] += (ssp(s)*iJac) * source.value();
//                }
//              }
//            }
//          }

//          template <typename ImageType1, typename VectorType2>
//          void project_slice_y2x(const int v, const int z, ImageType1& dst, const VectorType2& rhs) const
//          {
//            std::unique_ptr<FieldInterpType> finterp;
//            if (field.valid())
//              finterp = make_unique<FieldInterpType>(field, 0.0f);

//            Eigen::Vector3 ps, pr, peoffset;
//            float iJac;
//            transform_type Ts2r = get_Ts2r(v, z);
//            if (field.valid()) {
//              peoffset = pe.block<1,3>(v, 0).transpose().cast<double>();
//            }

//            Interp::CubicAdjoint<ImageType1> target (dst, 0.0f);

//            size_t i = 0;
//            for (size_t y = 0; y < ny; y++) {         // in-plane
//              ps[1] = y;
//              for (size_t x = 0; x < nx; x++, i++) {
//                ps[0] = x;
//                for (int s = -ssp.size(); s <= ssp.size(); s++) {       // ssp neighbourhood
//                  ps[2] = z+s;
//                  // get slice position in recon space
//                  ps2pr(ps, pr, Ts2r, finterp, peoffset, iJac);
//                  // update motion matrix
//                  target.voxel(pr);
//                  target.adjoint_add (ssp(s) * iJac * rhs[i]);
//                }
//              }
//            }
//          }

//          inline void ps2pr(const Eigen::Vector3& ps, Eigen::Vector3& pr, const transform_type& Ts2r,
//                            std::unique_ptr<FieldInterpType>& field, const Eigen::Vector3 pe, float& invjac) const
//          {
//            pr = Ts2r * ps;
//            // clip pr to edges
//            pr[0] = (pr[0] < 0) ? 0 : (pr[0] > nx-1) ? nx-1 : pr[0];
//            pr[1] = (pr[1] < 0) ? 0 : (pr[1] > ny-1) ? ny-1 : pr[1];
//            pr[2] = (pr[2] < 0) ? 0 : (pr[2] > nz-1) ? nz-1 : pr[2];
//            // field mapping
//            invjac = 1.0;
//            if (field) {
//              float B0 = 0.0f;
//              Eigen::Vector3 p1 = pr;
//              Eigen::RowVector3f dB0;
//              // fixed point inversion
//              for (int j = 0; j < 30; j++) {
//                field->voxel(Tf * pr);
//                field->value_and_gradient(B0, dB0);
//                pr = Ts2r * (ps - B0 * pe);
//                if ((p1 - pr).norm() < 0.1f) break;
//                p1 = pr;
//              }
//              // Jacobian
//              dB0 *= (Tf * Ts2r).rotation().cast<float>();
//              invjac = 1.0 / std::max(0.1, 1. + 2. * dB0 * pe.cast<float>());
//            }
//          }

//      };

    }
  }

}


#endif

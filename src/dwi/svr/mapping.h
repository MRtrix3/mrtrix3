/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#pragma once


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
    class LinearAdjoint : public Linear <ImageType>
    {
      public:
        using typename Linear<ImageType>::value_type;
        using Linear<ImageType>::clamp;
        using Linear<ImageType>::P;
        using Linear<ImageType>::factors;

        LinearAdjoint (const ImageType& parent, value_type outofbounds = 0)
            : Linear <ImageType> (parent, outofbounds)
        { }

        //! Add value to local region by interpolation weights.
        void adjoint_add (value_type val) {
          if (Base<ImageType>::out_of_bounds) return;

          ssize_t c[] = { ssize_t (std::floor (P[0])), ssize_t (std::floor (P[1])), ssize_t (std::floor (P[2])) };

          size_t i(0);
          for (ssize_t z = 0; z < 2; ++z) {
            ImageType::index(2) = clamp (c[2] + z, ImageType::size (2));
            for (ssize_t y = 0; y < 2; ++y) {
              ImageType::index(1) = clamp (c[1] + y, ImageType::size (1));
              for (ssize_t x = 0; x < 2; ++x) {
                ImageType::index(0) = clamp (c[0] + x, ImageType::size (0));
                ImageType::adjoint_add(factors[i++] * val);
              }
            }
          }
        }
    };

    template <class ImageType>
    class CubicAdjoint : public Cubic <ImageType>
    {
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
        public:
          using base_type = Adapter::Base<MotionMapping<ImageType>, ImageType>;
          using value_type = typename ImageType::value_type;
          using vector_type = typename Eigen::Matrix<value_type, Eigen::Dynamic, 1>;

          using base_type::parent;

          MotionMapping (const ImageType& projection, const Header& source,
                         const Eigen::MatrixXf& rigid, const SSP<float>& ssp)
            : base_type (projection),
              interp (projection, 0.0f), yhdr (source), motion (rigid), ssp (ssp),
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
            value_type res = 0;
            for (int z = -ssp.size(); z <= ssp.size(); z++) {
              Eigen::Vector3d pr = Ts2r * Eigen::Vector3d (x[0], x[1], x[2] + z);
              for (int k = 0; k < 3; k++) pr[k] = clampdim(pr[k], k);
              interp.voxel (pr);
              res += ssp(z) * interp.value();
            }
            return res;
          }

          void adjoint_add (value_type val) {
            for (int z = -ssp.size(); z <= ssp.size(); z++) {
              Eigen::Vector3d pr = Ts2r * Eigen::Vector3d (x[0], x[1], x[2] + z);
              for (int k = 0; k < 3; k++) pr[k] = clampdim(pr[k], k);
              interp.voxel (pr);
              interp.adjoint_add(ssp(z) * val);
            }
          }

          void set_shotidx (size_t idx) {
            interp.set_shotidx(idx);
            Ts2r = Tr.scanner2voxel * get_transform(motion.row(idx)) * Ts.voxel2scanner;
          }

        private:
          Interp::CubicAdjoint<ImageType> interp;
          const Header& yhdr;
          Eigen::MatrixXf motion;
          SSP<float> ssp;
          ssize_t x[3];
          const Transform Tr, Ts;
          transform_type Ts2r;    // vox-to-vox transform, mapping vectors in source space to recon space

          FORCE_INLINE transform_type get_transform(const Eigen::VectorXf& p) const {
            transform_type T (se3exp(p).cast<double>());
            return T;
          }

          FORCE_INLINE default_type clampdim (default_type r, size_t axis) const {
            return (r < 0) ? 0 : (r > parent().size(axis)-1) ? parent().size(axis)-1 : r;
          }

      };


      class ReconMapping
      {
        public:
          ReconMapping(const Header& recon, const Header& source, const QSpaceBasis& basis,
                       const Eigen::MatrixXf& rigid, const SSP<float>& ssp)
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
            auto spatialmap = Adapter::make<MotionMapping> (qmap, yhdr, motion, ssp);

            // define per-slice mapping
            struct MapSliceX2Y {
              ImageType2 out;
              decltype(spatialmap) pred;
              size_t ne;
              const std::vector<size_t>& axouter;
              const std::vector<size_t>& axslice;
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
            auto spatialmap = Adapter::make<MotionMapping> (qmap, yhdr, motion, ssp);

            // define per-slice mapping
            struct MapSliceY2X {
              ImageType2 in;
              decltype(spatialmap) pred;
              size_t ne;
              const std::vector<size_t>& axouter;
              const std::vector<size_t>& axslice;
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
          const std::vector<size_t> outer_axes;
          const std::vector<size_t> slice_axes;

          const QSpaceBasis qbasis;
          const Eigen::MatrixXf motion;
          const SSP<float> ssp;

      };

    }
  }

}


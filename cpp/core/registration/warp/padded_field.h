/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#pragma once

#include <array>

#include "exception.h"
#include "header.h"
#include "image.h"
#include "types.h" // provides Eigen with MRtrix's plugins (must precede any direct Eigen include)

#include "algo/loop.h"
#include "registration/warp/extrapolate.h" // make_padded_grid

namespace MR::Registration::Warp {

//! \addtogroup registration
// @{

//! a reusable padded buffer for cubic interpolation of a dense, finite warp field
/*! Non-linear registration repeatedly samples warp fields (displacement or deformation)
 *  that are dense and finite across their entire field of view: they carry no internal
 *  holes, so the validity-mask / dilation / polynomial-halo extrapolation machinery of
 *  \c Interp::Warp is wasted on them. The only requirement for cubic interpolation to be
 *  well-posed is that the four-tap kernel of a sample near a field-of-view edge reads
 *  finite data up to \a margin (two) voxels beyond that edge; the alternative,
 *  \c Interp::Cubic's built-in clamping, would replicate the edge value and so bias the
 *  field there.
 *
 *  \c PaddedField meets that requirement cheaply and persistently. It allocates, once, a
 *  buffer enlarged by a \a margin-voxel border on every side; refresh() copies the
 *  current field into the interior and refills the border shell by separable linear
 *  extrapolation (a fixed two-point stencil per axis that carries the boundary gradient
 *  outward). Because both the buffer and the extrapolation pattern are fixed for a given
 *  grid, a single \c PaddedField is built once per multi-resolution level and refreshed in
 *  place each iteration, so the per-iteration cost collapses to one interior copy plus an
 *  O(surface) extrapolation, with no per-call allocation, finiteness scan, dilation or
 *  polynomial solve. Sample the buffer through \c Interp::DenseWarp, whose acceptance is
 *  the original (pre-padding) field of view.
 *
 *  Linear (rather than the polynomial \c extrapolate_warp_halo()) extrapolation is used
 *  deliberately: on a hole-free field the halo serves only to keep the edge kernel
 *  well-posed, the boundary gradient is the dominant term over a two-voxel reach, and a
 *  fixed stencil avoids both the per-refresh least-squares cost and the per-voxel stencil
 *  storage that a precomputed polynomial scheme would incur on this hot path. */
class PaddedField {
public:
  explicit PaddedField(const Header &field_header, const ssize_t margin = 2) : margin(margin) {
    if (field_header.ndim() != 4 || field_header.size(3) != 3)
      throw Exception("PaddedField requires a 4D warp field with 3 volumes along axis 3");
    for (ssize_t axis = 0; axis != 3; ++axis)
      original_size_[axis] = field_header.size(axis);
    const std::array<ssize_t, 3> pad{margin, margin, margin};
    const PaddedGrid padded = make_padded_grid(field_header, pad, pad);
    shift_ = padded.shift;
    buffer_ = Image<default_type>::scratch(padded.grid, "padded warp field buffer");
  }

  //! copy \a field into the interior of the buffer and refill the extrapolated border shell
  /*! \a field must be a warp field whose spatial grid matches the grid the \c PaddedField was
   *  built for, with three components along axis 3. Reading the three components via row(3)
   *  (rather than looping axis 3) lets \a field be either an \c Image or an \c Adapter::Extract1D
   *  view into a higher-dimensional warp. */
  template <class SourceImageType> void refresh(SourceImageType &field) {
    for (ssize_t axis = 0; axis != 3; ++axis)
      assert(field.size(axis) == original_size_[axis]);
    for (auto l = Loop(field, 0, 3)(field); l; ++l) {
      for (ssize_t axis = 0; axis != 3; ++axis)
        buffer_.index(axis) = field.index(axis) + shift_[axis];
      const Eigen::Vector3d value(field.row(3));
      for (ssize_t component = 0; component != 3; ++component) {
        buffer_.index(3) = component;
        buffer_.value() = value[component];
      }
    }
    extrapolate_halo();
  }

  Image<default_type> &buffer() { return buffer_; }
  const std::array<ssize_t, 3> &shift() const { return shift_; }
  const std::array<ssize_t, 3> &original_size() const { return original_size_; }

private:
  ssize_t margin;
  std::array<ssize_t, 3> shift_;
  std::array<ssize_t, 3> original_size_;
  Image<default_type> buffer_;

  //! fill the margin-voxel border shell by separable linear extrapolation
  /*! Extrapolate along x for the interior of (y,z); then along y for the full x range and
   *  the interior of z; then along z for the full (x,y) range. Processing the axes in this
   *  order populates faces, then edges, then corners, each from already-filled data. */
  void extrapolate_halo() {
    extrapolate_along(0, 1, shift_[1], shift_[1] + original_size_[1], 2, shift_[2], shift_[2] + original_size_[2]);
    extrapolate_along(1, 0, 0, buffer_.size(0), 2, shift_[2], shift_[2] + original_size_[2]);
    extrapolate_along(2, 0, 0, buffer_.size(0), 1, 0, buffer_.size(1));
  }

  //! extrapolate the low and high border slabs along \a axis, over the given ranges of the
  //!   other two axes (\a u_axis in [u_lo,u_hi) and \a v_axis in [v_lo,v_hi))
  void extrapolate_along(const ssize_t axis,
                         const ssize_t u_axis,
                         const ssize_t u_lo,
                         const ssize_t u_hi,
                         const ssize_t v_axis,
                         const ssize_t v_lo,
                         const ssize_t v_hi) {
    const ssize_t lo = shift_[axis];                        // first interior index
    const ssize_t hi = shift_[axis] + original_size_[axis]; // one past last interior index
    // A linear (gradient-carrying) extrapolation needs two interior samples; where the
    //   field is only one voxel thick along this axis, fall back to a constant (slope 0).
    const bool linear = original_size_[axis] >= 2;
    for (ssize_t u = u_lo; u != u_hi; ++u) {
      buffer_.index(u_axis) = u;
      for (ssize_t v = v_lo; v != v_hi; ++v) {
        buffer_.index(v_axis) = v;
        for (ssize_t component = 0; component != 3; ++component) {
          buffer_.index(3) = component;

          buffer_.index(axis) = lo;
          const default_type edge_low = buffer_.value();
          default_type slope_low = 0.0;
          if (linear) {
            buffer_.index(axis) = lo + 1;
            slope_low = edge_low - buffer_.value();
          }
          for (ssize_t j = 1; j <= margin; ++j) {
            buffer_.index(axis) = lo - j;
            buffer_.value() = edge_low + static_cast<default_type>(j) * slope_low;
          }

          buffer_.index(axis) = hi - 1;
          const default_type edge_high = buffer_.value();
          default_type slope_high = 0.0;
          if (linear) {
            buffer_.index(axis) = hi - 2;
            slope_high = edge_high - buffer_.value();
          }
          for (ssize_t j = 1; j <= margin; ++j) {
            buffer_.index(axis) = hi - 1 + j;
            buffer_.value() = edge_high + static_cast<default_type>(j) * slope_high;
          }
        }
      }
    }
  }
};

//! @}

} // namespace MR::Registration::Warp

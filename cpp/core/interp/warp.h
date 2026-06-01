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
#include <cmath>
#include <limits>
#include <utility>

#include "datatype.h"
#include "exception.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "types.h"

#include "algo/loop.h"
#include "filter/dilate.h"
#include "interp/cubic.h"
#include "interp/linear.h"
#include "registration/warp/extrapolate.h"

namespace MR::Interp {

//! \addtogroup interp
// @{

//! policy governing how Interp::Warp decides whether a sampled position is valid
/*! \c Nearest reproduces the original behaviour: the validity bit of the voxel in
 *  which the position resides (nearest-voxel rounding) governs acceptance, giving a
 *  blocky, lattice-quantised accept boundary. \c Interpolated instead trilinearly
 *  interpolates a continuous validity field and accepts at a half threshold, so the
 *  accept boundary is a sub-voxel iso-surface at the true region edge; this removes
 *  the per-sample rounding that otherwise scatters acceptance within one voxel of a
 *  validity transition. \c Nearest is retained so its effect can be assessed. */
enum class ValidityPolicy { Nearest, Interpolated };

//! Cubic interpolation of a non-linear warp field with extrapolation of invalid voxels
/*! This interpolator is dedicated to the application of a non-linear warp field: a
 *  4D image with three volumes along axis 3, holding for each voxel either the
 *  absolute scanner-space position to which it maps (a deformation field) or the
 *  offset from its own position (a displacement field). The same interpolation and
 *  extrapolation machinery serves both forms, since it reads the three raw field
 *  components and never interprets them as absolute positions. The input field may
 *  contain voxels with non-finite values (NaN or Inf), e.g. outside the domain over
 *  which the warp was estimated.
 *
 *  Cubic interpolation reads a 4x4x4 neighbourhood for every sampled position;
 *  were a non-finite value to enter that kernel, it would poison the result even
 *  for nearby valid positions. The 4x4x4 kernel of any sample residing in a
 *  valid voxel reaches at most two voxels away from that voxel along each axis,
 *  so it suffices to guarantee finite data within a two-voxel margin of the
 *  valid region; imputing the remainder of the field would be wasted work.
 *
 *  Construction therefore proceeds as follows:
 *   - A bitwise validity mask is generated, marking a voxel \c true only where
 *     every component of the input warp field is finite.
 *   - If any valid voxel lies within two voxels of a field-of-view edge, a
 *     larger voxel grid is generated that guarantees a two-voxel margin around
 *     the entire valid region (the buffer, the mask and the imputed field all
 *     live on this grid), and the validity mask is shifted onto it.
 *   - The validity mask is dilated by two passes of 26-connectivity (the
 *     \c MR::Filter::Dilate filter), yielding the set of voxels that can be
 *     reached by the cubic kernel of any valid sample.
 *   - The "halo" of voxels added by that dilation (present after dilation but
 *     absent before) is filled by rank-ordered local polynomial extrapolation
 *     (\c MR::Registration::Warp::extrapolate_warp_halo): a warp field is a smooth
 *     coordinate map (or offset map), locally affine or quadratic, so each halo
 *     voxel is filled by a small least-squares fit of a low-order polynomial to
 *     the valid (and already-filled) samples in its vicinity. Halo voxels are
 *     processed outward from the valid region in layers of decreasing
 *     filled-neighbour count, all three components solved together.
 *  Cubic interpolation is then performed against this buffer (empirical warp at
 *  valid voxels, extrapolated values across the halo), so a valid
 *  sample whose kernel overlaps the boundary still yields a clean, finite
 *  result. Voxels beyond the halo are never read by a valid sample and are left
 *  unpopulated.
 *
 *  The validity mask governs the return value of voxel(), image() and scanner():
 *  each returns \c true only if the requested vertex resides inside the valid
 *  region (under \c ValidityPolicy::Interpolated, where the trilinearly-interpolated
 *  finite-input mask is at least one half). When a sample resides outside it the
 *  position is flagged out-of-bounds, so value() and row() return the out-of-bounds
 *  value; the extrapolated halo exists to keep interpolation at valid samples
 *  well-posed, not to fabricate trustworthy data at the holes themselves.
 *
 *  The interpolated warp is read exactly as for any 4D cubic interpolator: position
 *  the interpolator with voxel(), image() or scanner(), then obtain the three
 *  components via row(3) (or via index(3) and value()).
 *
 *  For example:
 *  \code
 *  // open a warp field that may contain non-finite voxels:
 *  Interp::Warp<> warp(Header::open(argument[0]));
 *
 *  // sample the warp at a scanner-space position:
 *  if (warp.scanner(Eigen::Vector3d{10.2, 3.59, 54.1}))
 *    Eigen::Vector3d mapped = warp.row(3);
 *  \endcode
 *
 *  \note The halo is filled by independent per-voxel polynomial fits rather than
 *    a global linear solve, so the cost scales linearly with the number of halo
 *    voxels (themselves bounded by the surface of the valid region rather than by
 *    the total count of non-finite voxels); this remains tractable for large
 *    non-linear fields, where a dense global solve over the halo would not.
 */
template <class ValueType = default_type, Math::SplineProcessingType PType = Math::SplineProcessingType::Value>
class Warp : public SplineInterp<Image<ValueType>, Math::HermiteSpline<ValueType>, PType> {
public:
  using value_type = ValueType;
  using buffer_type = Image<ValueType>;
  using base_type = SplineInterp<buffer_type, Math::HermiteSpline<ValueType>, PType>;

private:
  //! the products of construction: the imputed buffer, the validity bit mask and its
  //!   floating-point counterpart (1 valid / 0 invalid) used for interpolated validity
  struct ScratchData {
    buffer_type buffer;
    Image<bool> mask;
    Image<float> mask_float;
  };

  //! build the extrapolated warp field buffer and the validity mask
  template <class ImageType>
  static ScratchData make_scratch(ImageType field, const Registration::Warp::ExtrapolateDegree degree);

  //! open the warp field image referenced by a Header
  static Image<value_type> open_field(const Header &warp_field) {
    Header header(warp_field);
    if (header.datatype().is_complex())
      throw Exception("Warp field interpolator does not operate on complex image data");
    return header.get_image<value_type>();
  }

  // The per-voxel validity mask shares the spatial voxel grid of the buffer
  //   held by the Cubic base class. The float copy backs the trilinear validity
  //   interpolator used under ValidityPolicy::Interpolated.
  Image<bool> validity;
  Interp::Linear<Image<float>> validity_interp;
  ValidityPolicy validity_policy;

  //! delegating constructor: parent the cubic interpolator on the imputed buffer
  Warp(ScratchData data, const value_type value_when_out_of_bounds, const ValidityPolicy policy)
      : base_type(data.buffer, value_when_out_of_bounds),
        validity(std::move(data.mask)),
        validity_interp(data.mask_float, 0.0f),
        validity_policy(policy) {}

public:
  //! construct from a warp field that may contain non-finite voxels
  /*! \param field             a 4D image-like object with three volumes along axis 3
   *    (a deformation field of absolute positions, or a displacement field of
   *    offsets); either a file-backed or a scratch image is accepted. A single
   *    templated constructor accepts any image type (e.g. \c MR::Image<> or a
   *    reslice source), so no \c MR::Image-specific overload is required.
   *  \param degree            the polynomial degree policy used to extrapolate
   *    the halo around the valid region; defaults to
   *    \c Registration::Warp::ExtrapolateDegree::Adaptive, which fits the richest
   *    model the local support sustains (quadratic, then affine, then constant),
   *    carrying the boundary gradient and curvature into the extrapolated region
   *  \param value_when_out_of_bounds the value returned by value()/row() when the
   *    sample is outside the field of view or resides in an invalid voxel
   *  \param policy            whether validity is decided by nearest-voxel rounding
   *    (\c ValidityPolicy::Nearest) or by trilinear interpolation of the validity
   *    field (\c ValidityPolicy::Interpolated, the default; see \c ValidityPolicy) */
  template <class ImageType>
  Warp(const ImageType &field,
       const Registration::Warp::ExtrapolateDegree degree = Registration::Warp::ExtrapolateDegree::Adaptive,
       const ValidityPolicy policy = ValidityPolicy::Interpolated,
       const value_type value_when_out_of_bounds = base_type::default_out_of_bounds_value())
      : Warp(make_scratch(field, degree), value_when_out_of_bounds, policy) {}

  //! construct from a warp field referenced by a Header
  /*! Equivalent to opening the image and forwarding to the image-based
   *  constructor; see that overload for parameter documentation. This non-templated
   *  overload is preferred by overload resolution for \c Header arguments. */
  Warp(const Header &warp_field,
       const Registration::Warp::ExtrapolateDegree degree = Registration::Warp::ExtrapolateDegree::Adaptive,
       const ValidityPolicy policy = ValidityPolicy::Interpolated,
       const value_type value_when_out_of_bounds = base_type::default_out_of_bounds_value())
      : Warp(make_scratch(open_field(warp_field), degree), value_when_out_of_bounds, policy) {}

  //! Set the current position to <b>voxel space</b> position \a pos
  /*! See file interp/base.h for details.
   *  \return true only if \a pos lies within the field of view and is accepted by the
   *    active validity policy: under \c ValidityPolicy::Nearest the voxel in which \a pos
   *    resides must have been free of non-finite values in the input field; under
   *    \c ValidityPolicy::Interpolated the trilinearly-interpolated validity at \a pos
   *    must be at least one half. */
  template <class VectorType> bool voxel(const VectorType &pos) {
    if (base_type::set_out_of_bounds(pos))
      return false;
    if (validity_policy == ValidityPolicy::Interpolated) {
      // Continuous validity: accept where the trilinearly-interpolated 0/1 field is at
      //   least one half, placing the accept boundary at the true sub-voxel region edge.
      if (!validity_interp.voxel(pos) || validity_interp.value() < 0.5f) {
        base_type::set_out_of_bounds(true);
        return false;
      }
    } else {
      validity.index(0) = static_cast<ssize_t>(std::round(pos[0]));
      validity.index(1) = static_cast<ssize_t>(std::round(pos[1]));
      validity.index(2) = static_cast<ssize_t>(std::round(pos[2]));
      if (!validity.value()) {
        base_type::set_out_of_bounds(true);
        return false;
      }
    }
    return base_type::voxel(pos);
  }

  //! Set the current position to <b>image space</b> position \a pos
  /*! See file interp/base.h for details. */
  template <class VectorType> FORCE_INLINE bool image(const VectorType &pos) {
    return voxel(Transform::voxelsize.inverse() * pos.template cast<default_type>());
  }

  //! Set the current position to the <b>scanner space</b> position \a pos
  /*! See file interp/base.h for details. */
  template <class VectorType> FORCE_INLINE bool scanner(const VectorType &pos) {
    return voxel(Transform::scanner2voxel * pos.template cast<default_type>());
  }
};

template <class ValueType, Math::SplineProcessingType PType>
template <class ImageType>
typename Warp<ValueType, PType>::ScratchData
Warp<ValueType, PType>::make_scratch(ImageType field, const Registration::Warp::ExtrapolateDegree degree) {
  Header header(field);
  if (header.ndim() != 4 || header.size(3) != 3)
    throw Exception("Warp field \"" + header.name() + "\" must be a 4D image with 3 volumes along axis 3");

  // Step 3.1: validity mask on the input grid, true only where every component
  //   of the voxel is finite; simultaneously accumulate the bounding box of the
  //   valid region, needed to size the (possibly padded) working grid.
  constexpr ssize_t margin = 2;
  Header input_mask_header(header);
  input_mask_header.ndim() = 3;
  input_mask_header.datatype() = DataType::Bit;
  Image<bool> input_validity(Image<bool>::scratch(input_mask_header, "warp field validity mask"));
  std::array<ssize_t, 3> bbox_min{header.size(0), header.size(1), header.size(2)};
  std::array<ssize_t, 3> bbox_max{-1, -1, -1};
  for (auto l = Loop(input_validity)(input_validity); l; ++l) {
    assign_pos_of(input_validity, 0, 3).to(field);
    bool valid = true;
    for (ssize_t component = 0; component != 3; ++component) {
      field.index(3) = component;
      if (!std::isfinite(field.value())) {
        valid = false;
        break;
      }
    }
    input_validity.value() = valid;
    if (valid) {
      for (ssize_t axis = 0; axis != 3; ++axis) {
        const ssize_t pos = input_validity.index(axis);
        bbox_min[axis] = std::min(bbox_min[axis], pos);
        bbox_max[axis] = std::max(bbox_max[axis], pos);
      }
    }
  }
  if (bbox_max[0] < 0)
    throw Exception("Warp field \"" + header.name() + "\" contains no finite voxels");

  // Step 3.2: pad the grid where necessary so that every valid voxel lies at
  //   least `margin` voxels from the field-of-view edge, guaranteeing both that
  //   the dilation halo fits and that the cubic kernel of any valid sample stays
  //   in-bounds. `shift` maps an input voxel index onto the (padded) grid.
  std::array<ssize_t, 3> shift{0, 0, 0};
  Header grid(header);
  for (ssize_t axis = 0; axis != 3; ++axis) {
    const ssize_t pad_low = std::max<ssize_t>(0, margin - bbox_min[axis]);
    const ssize_t pad_high = std::max<ssize_t>(0, margin - (header.size(axis) - 1 - bbox_max[axis]));
    shift[axis] = pad_low;
    grid.size(axis) = header.size(axis) + pad_low + pad_high;
    // Shift the transform so that padded voxel `pad_low` along this axis maps to
    //   the scanner-space position of input voxel 0 (origin offset of `-pad_low`).
    for (ssize_t i = 0; i != 3; ++i)
      grid.transform()(i, 3) -= static_cast<default_type>(pad_low) * grid.spacing(axis) * grid.transform()(i, axis);
  }

  // 4D buffer on the working grid: empirical warp where finite, imputed
  //   values across the halo, left unpopulated beyond it.
  Image<value_type> buffer(Image<value_type>::scratch(grid, "imputed warp field"));
  for (auto l = Loop(field)(field); l; ++l) {
    for (ssize_t axis = 0; axis != 3; ++axis)
      buffer.index(axis) = field.index(axis) + shift[axis];
    buffer.index(3) = field.index(3);
    buffer.value() = field.value();
  }

  // Validity mask resampled onto the working grid (a pure index shift).
  Header mask_header(grid);
  mask_header.ndim() = 3;
  mask_header.datatype() = DataType::Bit;
  Image<bool> validity(Image<bool>::scratch(mask_header, "warp field validity mask"));
  for (auto l = Loop(input_validity)(input_validity); l; ++l) {
    for (ssize_t axis = 0; axis != 3; ++axis)
      validity.index(axis) = input_validity.index(axis) + shift[axis];
    validity.value() = input_validity.value();
  }

  // Step 3.3: two passes of 26-connectivity dilation of the validity mask,
  //   producing the set of voxels reachable by the cubic kernel of any valid
  //   sample.
  Image<bool> dilated(Image<bool>::scratch(mask_header, "dilated validity mask"));
  {
    Filter::Dilate dilate(validity);
    dilate.set_26_connectivity(true);
    dilate.set_npass(2);
    dilate(validity, dilated);
  }

  // Step 3.4: the halo is the band added by dilation: present in the dilated
  //   mask but absent from the validity mask.
  Image<bool> halo(Image<bool>::scratch(mask_header, "extrapolation halo mask"));
  for (auto l = Loop(halo)(halo, dilated, validity); l; ++l)
    halo.value() = dilated.value() && !validity.value();

  // Step 3.5: extrapolate the field across the halo by rank-ordered local
  //   polynomial fitting, growing outward from the valid region. Each halo voxel
  //   draws only on valid (and already-filled) samples in its vicinity, so the
  //   unpopulated voxels beyond the halo are never read.
  Registration::Warp::extrapolate_warp_halo(buffer, validity, halo, degree);

  // Floating-point copy of the validity mask (1 valid / 0 invalid) backing the
  //   trilinear validity interpolator used under ValidityPolicy::Interpolated.
  Header mask_float_header(mask_header);
  mask_float_header.datatype() = DataType::Float32;
  mask_float_header.datatype().set_byte_order_native();
  Image<float> mask_float(Image<float>::scratch(mask_float_header, "warp field validity field"));
  for (auto l = Loop(validity)(validity, mask_float); l; ++l)
    mask_float.value() = validity.value() ? 1.0f : 0.0f;

  return ScratchData{buffer, validity, mask_float};
}

//! adapt Interp::Warp to the Filter::reslice / Adapter::Reslice interpolator interface
/*! Those engines expect a single-template-parameter interpolator
 *  (\c template<class ImageType> class) constructed as
 *  <tt>interp(source, value_when_out_of_bounds)</tt>, whereas \c Interp::Warp is
 *  parameterised on its scalar \c ValueType and takes the out-of-bounds value as its
 *  fourth constructor argument. This thin wrapper bridges both: it exposes the
 *  expected one-parameter template form and the two-argument
 *  <tt>(field, value_when_out_of_bounds)</tt> construction, forwarding the warp's
 *  default \c Adaptive extrapolation and \c Interpolated validity policies. A plain
 *  type alias cannot achieve this because the second constructor argument would bind
 *  to \c ExtrapolateDegree rather than the out-of-bounds value. */
template <class ImageType> class WarpReslice : public Warp<typename ImageType::value_type> {
public:
  using value_type = typename ImageType::value_type;
  using base_type = Warp<value_type>;
  WarpReslice(const ImageType &field,
              const value_type value_when_out_of_bounds = base_type::default_out_of_bounds_value())
      : base_type(field,
                  Registration::Warp::ExtrapolateDegree::Adaptive,
                  ValidityPolicy::Interpolated,
                  value_when_out_of_bounds) {}
};

//! @}

} // namespace MR::Interp

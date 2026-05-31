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

#include "algo/impute.h"
#include "algo/loop.h"
#include "filter/dilate.h"
#include "interp/cubic.h"
#include "misc/voxel2vector.h"

namespace MR::Interp {

//! \addtogroup interp
// @{

//! Cubic interpolation of a non-linear deformation field with imputation of invalid voxels
/*! This interpolator is dedicated to the application of a non-linear deformation
 *  field (a 4D image with three volumes along axis 3, holding for each voxel the
 *  scanner-space position to which it maps). The input field may contain voxels
 *  with non-finite values (NaN or Inf), e.g. outside the domain over which the
 *  warp was estimated.
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
 *     every component of the input deformation field is finite.
 *   - If any valid voxel lies within two voxels of a field-of-view edge, a
 *     larger voxel grid is generated that guarantees a two-voxel margin around
 *     the entire valid region (the buffer, the mask and the imputed field all
 *     live on this grid), and the validity mask is shifted onto it.
 *   - The validity mask is dilated by two passes of 26-connectivity (the
 *     \c MR::Filter::Dilate filter), yielding the set of voxels that can be
 *     reached by the cubic kernel of any valid sample.
 *   - The "halo" of voxels added by that dilation (present after dilation but
 *     absent before) is filled by the imputation machinery of \c MR::Impute
 *     (the algorithms exposed by the \c mrimpute command), using the valid
 *     voxels as the only known data and treating the outer edge of the halo as a
 *     natural boundary. Each of the three components is imputed independently.
 *  Cubic interpolation is then performed against this buffer (empirical
 *  deformation at valid voxels, imputed values across the halo), so a valid
 *  sample whose kernel overlaps the boundary still yields a clean, finite
 *  result. Voxels beyond the halo are never read by a valid sample and are left
 *  unpopulated.
 *
 *  The validity mask governs the return value of voxel(), image() and scanner():
 *  each returns \c true only if the voxel in which the requested vertex resides
 *  was free of non-finite values in the input field. When a sample resides in an
 *  invalid voxel the position is flagged out-of-bounds, so value() and row()
 *  return the out-of-bounds value; the imputed halo exists to keep interpolation
 *  at valid samples well-posed, not to fabricate trustworthy data at the holes
 *  themselves.
 *
 *  The interpolated deformation is read exactly as for any 4D cubic
 *  interpolator: position the interpolator with voxel(), image() or scanner(),
 *  then obtain the three components via row(3) (or via index(3) and value()).
 *
 *  For example:
 *  \code
 *  // open a deformation field that may contain non-finite voxels:
 *  Interp::Deform<> deform(Header::open(argument[0]));
 *
 *  // sample the deformation at a scanner-space position:
 *  if (deform.scanner(Eigen::Vector3d{10.2, 3.59, 54.1}))
 *    Eigen::Vector3d mapped_position = deform.row(3);
 *  \endcode
 *
 *  \note The imputation system is dense and is solved once per component during
 *    construction; because only the two-voxel halo around the valid region is
 *    imputed, its size is bounded by the surface of that region rather than by
 *    the total count of non-finite voxels.
 */
template <class ValueType = default_type> class Deform : public Cubic<Image<ValueType>> {
public:
  using value_type = ValueType;
  using buffer_type = Image<ValueType>;
  using base_type = Cubic<buffer_type>;

private:
  //! the products of construction: the imputed buffer and the validity mask
  struct ScratchData {
    buffer_type buffer;
    Image<bool> mask;
  };

  //! build the imputed deformation field buffer and the validity mask
  static ScratchData
  make_scratch(const Header &deformation_field, const Impute::Method method, const Impute::Detrend detrend);

  // The per-voxel validity mask shares the spatial voxel grid of the buffer
  //   held by the Cubic base class.
  Image<bool> validity;

  //! delegating constructor: parent the cubic interpolator on the imputed buffer
  Deform(ScratchData data, const value_type value_when_out_of_bounds)
      : base_type(data.buffer, value_when_out_of_bounds), validity(std::move(data.mask)) {}

public:
  //! construct from a deformation field that may contain non-finite voxels
  /*! \param deformation_field a 4D image with three volumes along axis 3
   *  \param method            the imputation algorithm used to fill non-finite
   *    voxels; defaults to \c Impute::Method::hessian, whose natural boundary
   *    conditions extrapolate the boundary trend without the bias of the
   *    squared-Laplacian (biharmonic) energy
   *  \param detrend           whether to remove and re-add a polynomial trend
   *    during imputation; defaults to \c Impute::Detrend::quadratic, as this
   *    interpolator typically extrapolates the field across a wide non-finite
   *    border, where carrying the boundary gradient and curvature in closed form
   *    continues the field rather than flattening it
   *  \param value_when_out_of_bounds the value returned by value()/row() when the
   *    sample is outside the field of view or resides in an invalid voxel */
  Deform(const Header &deformation_field,
         const Impute::Method method = Impute::Method::hessian,
         const Impute::Detrend detrend = Impute::Detrend::quadratic,
         const value_type value_when_out_of_bounds = base_type::default_out_of_bounds_value())
      : Deform(make_scratch(deformation_field, method, detrend), value_when_out_of_bounds) {}

  //! Set the current position to <b>voxel space</b> position \a pos
  /*! See file interp/base.h for details.
   *  \return true only if \a pos lies within the field of view and the voxel in
   *    which \a pos resides was free of non-finite values in the input field. */
  template <class VectorType> bool voxel(const VectorType &pos) {
    if (base_type::set_out_of_bounds(pos))
      return false;
    validity.index(0) = static_cast<ssize_t>(std::round(pos[0]));
    validity.index(1) = static_cast<ssize_t>(std::round(pos[1]));
    validity.index(2) = static_cast<ssize_t>(std::round(pos[2]));
    if (!validity.value()) {
      base_type::set_out_of_bounds(true);
      return false;
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

template <class ValueType>
typename Deform<ValueType>::ScratchData Deform<ValueType>::make_scratch(const Header &deformation_field,
                                                                        const Impute::Method method,
                                                                        const Impute::Detrend detrend) {
  Header header(deformation_field);
  if (header.ndim() != 4 || header.size(3) != 3)
    throw Exception("Deformation field \"" + header.name() + "\" must be a 4D image with 3 volumes along axis 3");
  if (header.datatype().is_complex())
    throw Exception("Deformation field interpolator does not operate on complex image data");

  Image<value_type> field(header.get_image<value_type>());

  // Step 3.1: validity mask on the input grid, true only where every component
  //   of the voxel is finite; simultaneously accumulate the bounding box of the
  //   valid region, needed to size the (possibly padded) working grid.
  constexpr ssize_t margin = 2;
  Header input_mask_header(header);
  input_mask_header.ndim() = 3;
  input_mask_header.datatype() = DataType::Bit;
  Image<bool> input_validity(Image<bool>::scratch(input_mask_header, "deformation field validity mask"));
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
    throw Exception("Deformation field \"" + header.name() + "\" contains no finite voxels");

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

  // 4D buffer on the working grid: empirical deformation where finite, imputed
  //   values across the halo, left unpopulated beyond it.
  Image<value_type> buffer(Image<value_type>::scratch(grid, "imputed deformation field"));
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
  Image<bool> validity(Image<bool>::scratch(mask_header, "deformation field validity mask"));
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
  Image<bool> halo(Image<bool>::scratch(mask_header, "imputation halo mask"));
  for (auto l = Loop(halo)(halo, dilated, validity); l; ++l)
    halo.value() = dilated.value() && !validity.value();

  // Step 3.5: impute the halo voxels of each component independently. The valid
  //   voxels supply the known data; the imputation field of view is restricted
  //   to the dilated mask so that the outer edge of the halo acts as a natural
  //   boundary rather than reading the unpopulated voxels beyond it.
  const Voxel2Vector v2v(halo, mask_header);
  if (!v2v.empty()) {
    Image<bool> fov_reader(dilated);
    auto in_fov = [fov_reader](const Impute::Position &p) mutable -> bool {
      if (is_out_of_bounds(fov_reader, p, 0, 3))
        return false;
      fov_reader.index(0) = p[0];
      fov_reader.index(1) = p[1];
      fov_reader.index(2) = p[2];
      return fov_reader.value();
    };
    for (ssize_t component = 0; component != 3; ++component) {
      // Independent reader so that index manipulation during the solve does not
      //   disturb the iterators of the surrounding loops.
      Image<value_type> reader(buffer);
      auto value_at = [reader, component](const Impute::Position &p) mutable -> double {
        reader.index(0) = p[0];
        reader.index(1) = p[1];
        reader.index(2) = p[2];
        reader.index(3) = component;
        return static_cast<double>(reader.value());
      };

      const Impute::Vec solution = Impute::make_imputer(method, v2v, value_at, in_fov, detrend)->solve();

      buffer.index(3) = component;
      for (auto l = Loop(halo)(halo); l; ++l) {
        if (!halo.value())
          continue;
        assign_pos_of(halo, 0, 3).to(buffer);
        const Impute::Position p(halo.index(0), halo.index(1), halo.index(2));
        buffer.value() = static_cast<value_type>(solution[v2v(p)]);
      }
    }
  }

  return ScratchData{buffer, validity};
}

//! @}

} // namespace MR::Interp

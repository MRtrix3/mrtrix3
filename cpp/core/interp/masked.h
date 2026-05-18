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

#include "datatype.h"
#include "image.h"
#include "interp/base.h"

namespace MR::Interp {

//! \addtogroup interp
// @{

//! Implicit masking for interpolator class
/*! Wrap an image interpolator in a way that returns false not only if
 * the position is outside of the field of view of the image, but also
 * if the corresponding voxel in the supplied binary mask is false.
 *
 * The mask must share the spatial voxel grid of the parent image
 * (same dimensions, spacing, and scanner-space transform).
 */
template <class InterpType> class Masked : public InterpType {
public:
  using typename InterpType::value_type;

  Masked(
      const typename InterpType::image_type &parent,
      Image<bool> mask,
      const value_type value_when_out_of_bounds = Base<typename InterpType::image_type>::default_out_of_bounds_value())
      : InterpType(parent, value_when_out_of_bounds), voxel_mask(std::move(mask)) {
    check_voxel_grids_match_in_scanner_space(parent, voxel_mask);
  }

  //! Set the current position to <b>voxel space</b> position \a pos
  /*! See file interp/base.h for details. */
  template <class VectorType> bool voxel(const VectorType &pos) {
    if (InterpType::set_out_of_bounds(pos))
      return false;
    voxel_mask.index(0) = static_cast<ssize_t>(std::round(pos[0]));
    voxel_mask.index(1) = static_cast<ssize_t>(std::round(pos[1]));
    voxel_mask.index(2) = static_cast<ssize_t>(std::round(pos[2]));
    if (!voxel_mask.value()) {
      InterpType::set_out_of_bounds(true);
      return true;
    }
    return InterpType::voxel(pos);
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

private:
  Image<bool> voxel_mask;
};

//! @}

} // namespace MR::Interp

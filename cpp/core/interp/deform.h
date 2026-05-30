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

#include <cmath>
#include <utility>

#include "datatype.h"
#include "exception.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "types.h"

#include "algo/impute.h"
#include "algo/loop.h"
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
 *  for nearby valid positions. To avoid this, during construction the adapter
 *  uses the imputation machinery of \c MR::Impute (the algorithms exposed by the
 *  \c mrimpute command) to populate a scratch buffer that holds the empirical
 *  deformation where finite and imputed values where not. Cubic interpolation is
 *  then performed against this gap-filled buffer, so a valid sample whose kernel
 *  overlaps a hole still yields a clean, finite result.
 *
 *  Each of the three components is imputed independently, so that a finite
 *  component of a partially-invalid voxel is preserved as known data for its
 *  own component's solve.
 *
 *  A separate bitwise validity mask is also generated during construction: a
 *  voxel is marked \c true only if every component of the corresponding voxel in
 *  the input deformation field was finite. This mask governs the return value of
 *  voxel(), image() and scanner(): each returns \c true only if the voxel in
 *  which the requested vertex resides is \c true in the mask (i.e. was free of
 *  non-finite values). When a sample resides in an invalid voxel the position is
 *  flagged out-of-bounds, so value() and row() return the out-of-bounds value;
 *  the imputed buffer exists to keep interpolation at valid samples well-posed,
 *  not to fabricate trustworthy data at the holes themselves.
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
 *    construction; this is efficient for typical hole counts, but very large
 *    contiguous invalid regions will produce a large dense system.
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

  // 4D buffer: empirical deformation where finite, imputed values where not.
  Image<value_type> buffer(Image<value_type>::scratch(header, "imputed deformation field"));
  for (auto l = Loop(field)(field, buffer); l; ++l)
    buffer.value() = field.value();

  // 3D validity mask: true only where every component of the voxel is finite.
  Header mask_header(header);
  mask_header.ndim() = 3;
  mask_header.datatype() = DataType::Bit;
  Image<bool> mask(Image<bool>::scratch(mask_header, "deformation field validity mask"));
  for (auto l = Loop(mask)(mask); l; ++l) {
    assign_pos_of(mask, 0, 3).to(field);
    bool valid = true;
    for (ssize_t component = 0; component != 3; ++component) {
      field.index(3) = component;
      if (!std::isfinite(field.value())) {
        valid = false;
        break;
      }
    }
    mask.value() = valid;
  }

  // Impute each component independently against its own set of non-finite
  //   voxels, so that a finite component of a partially-invalid voxel is
  //   retained as known data for that component's solve.
  Header slab_header(header);
  slab_header.ndim() = 3;
  for (ssize_t component = 0; component != 3; ++component) {
    Image<bool> invalid(Image<bool>::scratch(mask_header, "non-finite component voxels"));
    field.index(3) = component;
    for (auto l = Loop(invalid)(invalid); l; ++l) {
      assign_pos_of(invalid, 0, 3).to(field);
      invalid.value() = !std::isfinite(field.value());
    }

    const Voxel2Vector v2v(invalid, slab_header);
    if (v2v.empty())
      continue;

    // Independent reader so that index manipulation during the solve does not
    //   disturb the iterators of the surrounding loops.
    Image<value_type> reader(field);
    auto value_at = [reader, component](const Impute::Position &p) mutable -> double {
      reader.index(0) = p[0];
      reader.index(1) = p[1];
      reader.index(2) = p[2];
      reader.index(3) = component;
      return static_cast<double>(reader.value());
    };
    auto in_fov = [&slab_header](const Impute::Position &p) -> bool { return !is_out_of_bounds(slab_header, p, 0, 3); };

    const Impute::Vec solution = Impute::make_imputer(method, v2v, value_at, in_fov, detrend)->solve();

    buffer.index(3) = component;
    for (auto l = Loop(invalid)(invalid); l; ++l) {
      if (!invalid.value())
        continue;
      assign_pos_of(invalid, 0, 3).to(buffer);
      const Impute::Position p(invalid.index(0), invalid.index(1), invalid.index(2));
      buffer.value() = static_cast<value_type>(solution[v2v(p)]);
    }
  }

  return ScratchData{buffer, mask};
}

//! @}

} // namespace MR::Interp

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

#include <optional>

#include "adapter/reslice.h"
#include "adapter/warp.h"
#include "algo/threaded_copy.h"
#include "algo/threaded_loop.h"
#include "datatype.h"
#include "filter/reslice.h"
#include "interp/warp.h"
#include "transform.h"

namespace MR::Filter {

// TODO if there is a use for this elsewhere then we should have threaded_copy4D convenience functions
class CopyKernel4D {
public:
  template <class InputImageType, class OutputImageType>
  FORCE_INLINE void operator()(InputImageType &in, OutputImageType &out) const {
    out.row(3) = in.row(3);
  }
};

//! Per-voxel kernel writing the analytic Jacobian determinant of a deformation field
/*! Samples the source warp's tri-cubic (Catmull-Rom) spline at each output voxel centre via a
 *  derivative-mode \c Interp::Warp and writes the determinant of the scanner-space Jacobian.
 *  Positions outside the warp's valid region yield a no-op determinant of 1.0; such voxels are
 *  never read downstream, as the resliced warp value is itself out-of-bounds there. A copy of
 *  the interpolator is held per kernel instance, so the kernel parallelises safely under
 *  \c ThreadedLoop. */
template <class WarpDerivInterpType> class WarpJacobianDeterminantKernel {
public:
  WarpJacobianDeterminantKernel(const WarpDerivInterpType &warp_deriv, const transform_type &voxel2scanner)
      : warp_deriv(warp_deriv), voxel2scanner(voxel2scanner) {}

  void operator()(Image<default_type> &out) {
    const Eigen::Vector3d voxel{static_cast<default_type>(out.index(0)),
                                static_cast<default_type>(out.index(1)),
                                static_cast<default_type>(out.index(2))};
    if (warp_deriv.scanner(voxel2scanner * voxel))
      out.value() = warp_deriv.gradient_row_wrt_scanner().template topLeftCorner<3, 3>().determinant();
    else
      out.value() = 1.0;
  }

protected:
  WarpDerivInterpType warp_deriv;
  const transform_type voxel2scanner;
};

//! Jacobian determinant of a deformation field, evaluated analytically on a destination grid
/*! Computes, on the \a destination voxel grid, the determinant of the scanner-space Jacobian of
 *  the deformation field \a source_warp, sampled once per voxel centre from the field's tri-cubic
 *  Catmull-Rom representation. This is the modulation factor required by \c Filter::warp() under
 *  Jacobian intensity modulation when the warp must first be resliced onto the destination grid:
 *  evaluating the determinant from the source spline (rather than finite-differencing the resampled
 *  grid afterward) makes it exact with respect to that spline and independent of the reslice grid.
 *  The derivative interpolator uses the same extrapolation and validity policies as
 *  \c Interp::WarpReslice, so the determinant is valid at exactly the voxels where the resliced
 *  warp value is. */
template <class WarpType, class HeaderType>
Image<default_type> reslice_warp_jacobian_determinant(const WarpType &source_warp, const HeaderType &destination) {
  Header det_header(destination);
  det_header.ndim() = 3;
  det_header.datatype() = DataType::Float64;
  det_header.datatype().set_byte_order_native();
  Image<default_type> jacobian_determinant =
      Image<default_type>::scratch(det_header, "resliced warp Jacobian determinant");

  Interp::Warp<default_type, Math::SplineProcessingType::Derivative> warp_deriv(source_warp);
  const transform_type voxel2scanner = Transform(destination).voxel2scanner;
  WarpJacobianDeterminantKernel<decltype(warp_deriv)> kernel(warp_deriv, voxel2scanner);
  ThreadedLoop(jacobian_determinant).run(kernel, jacobian_determinant);

  return jacobian_determinant;
}

//! convenience function to warp one image onto another
/*! This function resamples (regrids) the Image \a source onto the
 * Image& \a destination, using the templated interpolator class and a supplied deformation field.
 *
 * For example:
 * \code
 * // source and destination data:
 * auto source = Image<float>::open(argument[0]);
 *
 * auto warp = Image<float>::open(argument[1]);
 *
 * auto template = Header::open(argument[2]);
 *
 * auto destination = Image<float>::create (argument[3], template)
 *
 * // regrid source onto destination using linear interpolation:
 * Filter::warp<Image::Interp::Linear> (source, destination, warp);
 * \endcode
 */
template <template <class VoxelType> class Interpolator,
          class ImageTypeDestination,
          class ImageTypeSource,
          class WarpType>
void warp(ImageTypeSource &source,
          ImageTypeDestination &destination,
          WarpType &warp,
          const typename ImageTypeDestination::value_type value_when_out_of_bounds =
              Interpolator<ImageTypeSource>::default_out_of_bounds_value(),
          const std::vector<uint32_t> oversample = Adapter::AutoOverSample,
          const bool jacobian_modulate = false) {

  // reslice warp onto destination grid
  if (warp.transform().matrix() != destination.transform().matrix() || !dimensions_match(warp, destination, 0, 3) ||
      !spacings_match(warp, destination, 0, 3)) {

    Header header(destination);
    header.ndim() = 4;
    header.size(3) = 3;
    Stride::set(header, Stride::contiguous_along_axis(3, header));
    auto warp_resliced = Image<typename WarpType::value_type>::scratch(header);
    // Regrid the warp field through the imputation-aware cubic interpolator so that
    //   non-finite voxels do not poison the cubic kernel and samples beyond the
    //   warp's valid region become NaN rather than fabricated extrapolations.
    reslice<Interp::WarpReslice>(warp, warp_resliced, Adapter::NoTransform, oversample);

    // The warp has been resliced via cubic interpolation, so its Jacobian determinant is computed
    //   analytically from the source spline on the destination grid (exact w.r.t. that spline)
    //   rather than finite-differencing the resampled field; see reslice_warp_jacobian_determinant.
    std::optional<Image<default_type>> jacobian_determinant;
    if (jacobian_modulate)
      jacobian_determinant = reslice_warp_jacobian_determinant(warp, destination);

    Adapter::Warp<Interpolator, ImageTypeSource, Image<typename WarpType::value_type>> interp(
        source, warp_resliced, value_when_out_of_bounds, jacobian_modulate, jacobian_determinant);

    if (destination.ndim() == 4)
      ThreadedLoop("warping \"" + source.name() + "\"" +
                       (jacobian_modulate ? " with Jacobian intensity modulation" : ""),
                   interp,
                   0,
                   3,
                   1)
          .run(CopyKernel4D(), interp, destination);
    else
      threaded_copy_with_progress_message("warping \"" + source.name() + "\"" +
                                              (jacobian_modulate ? " with Jacobian intensity modulation" : ""),
                                          interp,
                                          destination);

    // no need to reslice warp
  } else {
    Adapter::Warp<Interpolator, ImageTypeSource, Image<typename WarpType::value_type>> interp(
        source, warp, value_when_out_of_bounds, jacobian_modulate);
    if (destination.ndim() == 4 && destination.is_direct_io())
      ThreadedLoop("warping \"" + source.name() + "\"" +
                       (jacobian_modulate ? " with Jacobian intensity modulation" : ""),
                   interp,
                   0,
                   3,
                   1)
          .run(CopyKernel4D(), interp, destination);
    else
      threaded_copy_with_progress_message("warping \"" + source.name() + "\"" +
                                              (jacobian_modulate ? " with Jacobian intensity modulation" : ""),
                                          interp,
                                          destination,
                                          0,
                                          destination.ndim(),
                                          2);
  }
}

//! @}
} // namespace MR::Filter

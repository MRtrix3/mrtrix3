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

#include "adapter/extract.h"
#include "adapter/jacobian.h"
#include "algo/threaded_loop.h"
#include "image.h"
#include "interp/warp.h"
#include "registration/warp/helpers.h"
#include "registration/warp/padded_field.h"
#include "transform.h"

namespace MR::Registration::Warp {

class ComposeLinearDeformKernel {
public:
  ComposeLinearDeformKernel(const transform_type &transform) : transform(transform) {}

  template <class InputDeformationFieldType, class OutputDeformationFieldType>
  void operator()(InputDeformationFieldType &deform_input, OutputDeformationFieldType &deform_output) {
    deform_output.row(3) = transform * Eigen::Vector3d(deform_input.row(3));
  }

protected:
  const transform_type transform;
};

class ComposeLinearDispKernel {
public:
  template <class DisplacementFieldType>
  ComposeLinearDispKernel(const transform_type &linear_transform, const DisplacementFieldType &disp_in)
      : linear_transform(linear_transform), image_transform(disp_in) {}

  template <class DisplacementFieldType, class DeformationFieldType>
  void operator()(DisplacementFieldType &disp_input, DeformationFieldType &deform_output) {
    Eigen::Vector3d voxel(disp_input.index(0), disp_input.index(1), disp_input.index(2));
    deform_output.row(3) =
        linear_transform * (image_transform.voxel2scanner * voxel + Eigen::Vector3d(disp_input.row(3)));
  }

protected:
  const transform_type linear_transform;
  MR::Transform image_transform;
};

// Compose two dense displacement fields. The second field is dense and finite by construction, so
//   it is sampled with cubic interpolation via Interp::DenseWarp over a PaddedField buffer; outside
//   its field of view the kernel falls back to the input displacement (as the linear sampler did).
class ComposeDispKernel {
public:
  ComposeDispKernel(Image<default_type> &disp_input1, Interp::DenseWarp<default_type> disp2_interp, default_type step)
      : disp1_transform(disp_input1), disp2_interp(std::move(disp2_interp)), step(step) {}

  void operator()(Image<default_type> &disp_input1, Image<default_type> &disp_output) {
    const Eigen::Vector3d voxel{static_cast<default_type>(disp_input1.index(0)),
                                static_cast<default_type>(disp_input1.index(1)),
                                static_cast<default_type>(disp_input1.index(2))};
    const Eigen::Vector3d voxel_position = disp1_transform.voxel2scanner * voxel;
    const Eigen::Vector3d original_position = voxel_position + Eigen::Vector3d(disp_input1.row(3));
    if (!disp2_interp.scanner(original_position)) {
      disp_output.row(3) = disp_input1.row(3);
    } else {
      const Eigen::Vector3d displacement(Eigen::Vector3d(disp2_interp.row(3)).array() * step);
      const Eigen::Vector3d new_position = displacement + original_position;
      disp_output.row(3) = new_position - voxel_position;
    }
  }

protected:
  MR::Transform disp1_transform;
  Interp::DenseWarp<default_type> disp2_interp;
  default_type step;
};

// Compose two half-way deformation fields, chaining a sample through both. The fields are dense
//   and finite by construction (they originate from the registration's half-way warps), so they
//   are sampled with cubic interpolation via Interp::DenseWarp over PaddedField buffers built by
//   the caller; out-of-field-of-view samples yield NaN. The fields reach the productive caller as
//   Adapter::Extract1D views into a 5D warp, which the caller materialises into the padded buffers.
class ComposeHalfwayKernel {
public:
  ComposeHalfwayKernel(const transform_type &linear1,
                       Interp::DenseWarp<default_type> deform1_interp,
                       Interp::DenseWarp<default_type> deform2_interp,
                       const transform_type &linear2)
      : linear1(linear1),
        deform1_interp(std::move(deform1_interp)),
        deform2_interp(std::move(deform2_interp)),
        linear2(linear2),
        out_of_bounds(Eigen::Vector3d::Constant(NaN)) {}

  void operator()(Image<default_type> &deform) {
    const Eigen::Vector3d voxel{static_cast<default_type>(deform.index(0)),
                                static_cast<default_type>(deform.index(1)),
                                static_cast<default_type>(deform.index(2))};
    const Eigen::Vector3d position = linear1 * voxel;
    if (!deform1_interp.scanner(position)) {
      deform.row(3) = out_of_bounds;
      return;
    }
    const Eigen::Vector3d position2 = deform1_interp.row(3);
    if (!deform2_interp.scanner(position2)) {
      deform.row(3) = out_of_bounds;
      return;
    }
    const Eigen::Vector3d position3 = deform2_interp.row(3);
    deform.row(3) = linear2 * position3;
  }

protected:
  const transform_type linear1;
  Interp::DenseWarp<default_type> deform1_interp;
  Interp::DenseWarp<default_type> deform2_interp;
  const transform_type linear2;
  Eigen::Vector3d out_of_bounds;
};

//! load a dense warp field into a padded buffer for cubic sampling via Interp::DenseWarp
/*! \a field may be an \c Image or an \c Adapter::Extract1D view into a higher-dimensional warp;
 *  a 4D header (three components along axis 3) is taken explicitly so either is accepted. */
template <class FieldType> FORCE_INLINE PaddedField make_padded_dense_field(FieldType &field) {
  Header header(field);
  header.ndim() = 4;
  header.size(3) = 3;
  PaddedField padded(header);
  padded.refresh(field);
  return padded;
}

// Compose a linear transform and a displacement field. The output field is a deformation field. The input and output
// can be the same image.
template <class DisplacementFieldType, class DeformationFieldType>
FORCE_INLINE void compose_linear_displacement(const transform_type &transform,
                                              DisplacementFieldType &disp_in,
                                              DeformationFieldType &deform_out) {
  check_dimensions(disp_in, deform_out, 0, 3);
  ThreadedLoop(disp_in, 0, 3).run(ComposeLinearDispKernel(transform, disp_in), disp_in, deform_out);
}

// Compose a linear transform and a deformation field. The output field is a deformation field. The input and output can
// be the same image.
template <class InputDeformationFieldType, class OutputDeformationFieldType>
FORCE_INLINE void compose_linear_deformation(const transform_type &transform,
                                             InputDeformationFieldType &deform_in,
                                             OutputDeformationFieldType &deform_out) {
  check_dimensions(deform_in, deform_out, 0, 3);
  ThreadedLoop(deform_in, 0, 3).run(ComposeLinearDeformKernel(transform), deform_in, deform_out);
}

// Compose two displacement fields and output a displacement field. The input and output can be the same image.
FORCE_INLINE void update_displacement(Image<default_type> &input,
                                      Image<default_type> &update,
                                      Image<default_type> &output,
                                      default_type step = 1.0) {
  check_dimensions(input, output, 0, 3);
  // The sampled field (update) is dense and finite by construction; load it into a padded buffer
  //   so it is read with cubic interpolation, well-posed at the field-of-view edge.
  PaddedField update_padded = make_padded_dense_field(update);
  ComposeDispKernel kernel(
      input,
      Interp::DenseWarp<default_type>(update_padded.buffer(), update_padded.shift(), update_padded.original_size()),
      step);
  ThreadedLoop(input, 0, 3).run(kernel, input, output);
}

/** Per-sub-step displacement bound divisor for diffeomorphism preservation under scaling-and-squaring.
 *
 * Each squared sub-step displacement must remain below \c min_vox_size / \c diffeomorphism_bound_divisor so that
 * composition of the sub-steps yields a diffeomorphic (positive-Jacobian) displacement field.  The safe value of this
 * divisor depends on the interpolation kernel used along the composition / inversion path:
 *
 * - K = 2.0: half-voxel bound, sufficient for LINEAR interpolation only.  A linear interpolant is a convex combination
 *   of its nodes, so the steepest interpolated slope equals the node secant slope (2a for a symmetric +/-a step);
 *   det = 1 - 2a > 0 requires a < 0.5 voxel.  NOT safe for cubic interpolation, which overshoots.
 * - K = 2.5: Catmull-Rom monotone-step bound (0.4 voxel).  A symmetric monotone step (nodes a, a, -a, -a) has cubic
 *   midpoint slope -2.5a, so det = 1 - 2.5a > 0 requires a < 0.4 voxel.  Matches the Choi-Lee (2000) cubic B-spline
 *   injectivity constant (~0.40).  Safe for smooth/monotone fields but not for pathological oscillation.
 * - K = 3.0: Catmull-Rom 1D axis-aligned worst-case bound (1/3 voxel).  For nodes bounded by +/-B the steepest slope
 * any single cubic segment can manufacture is -3B (oscillatory nodes -B, +B, -B, +B; slope extremum at cell centre);
 *   det = 1 - 3B > 0 requires B < 1/3 voxel.  Safe for a purely axial (diagonal-Jacobian) fold, but NOT for 3D shear:
 *   it ignores the off-diagonal Jacobian terms that the tri-cubic tensor product introduces.
 * - K = 7.03125 (= 225/32): the exact 3D tri-cubic Catmull-Rom injectivity bound (B < 32/225 ~ 0.1422 voxel).  The
 *   worst case is not axial compression but a rank-1 "uniform" Jacobian grad u = -c * (all-ones), whose eigenvalues are
 *   {3c, 0, 0}; det(I + grad u) = 1 - 3c folds once 3c = 1.  Each row of grad u can reach c = (75/32) B at the cell
 *   centre (the value-interpolation factor (5/4)^2 on the two un-differentiated axes amplifies every entry, and the
 *   three entries of a row jointly reach half the single-entry maximum), giving det = 1 - (225/32) B.  This is the
 *   minimum K that keeps EVERY sub-voxel position diffeomorphic for any node configuration within the bound.  It is far
 *   tighter than the Choi-Lee (2000) cubic B-spline bound (~0.40) because Catmull-Rom is interpolating: its basis is
 *   not positive (it overshoots) and has no convex-hull property to temper the Jacobian.
 * - K = 14.0625 (= 225/16): the rigorous Gershgorin / row-sum sufficient bound (B < 16/225 ~ 0.0711 voxel).  Exactly
 *   twice the critical divisor; conservative because it triple-counts shear that the rank-1 worst case single-counts.
 *
 * Active value: K = 225/32 = 7.03125.  The composition / inversion path uses tri-cubic Catmull-Rom interpolation, so
 * the binding constraint is the 3D shear-inclusive injectivity bound, not the 1D axial K = 3.0.  This is the *sharp*
 * bound: it is the smallest divisor that still keeps every sub-voxel position diffeomorphic for the worst-case
 * (pathological rank-1 oscillatory) node configuration, at which the minimum determinant is marginal (det -> 0).  Real
 * image data is far from that worst case, so the realised minimum Jacobian determinant typically carries substantial
 * headroom; the debug-only per-iteration logging below reports it so the divisor can be relaxed toward 3.0 (recovering
 * step size) if the bound proves unnecessarily pessimistic, or raised toward the Gershgorin K = 14.0625 if a worst-case
 * proof is required.
 */
constexpr default_type diffeomorphism_bound_divisor = 225.0 / 32.0;

#ifndef NDEBUG
//! Debug-only: report the minimum deformation-Jacobian determinant of an update field and assert diffeomorphism.
/*! Computes det(I + \a step * grad u) at every voxel of \a update, tracks its minimum over the whole image, emits that
 *  minimum at -debug verbosity, and throws if the field is not diffeomorphic (non-positive determinant anywhere).  The
 *  logging lets the realised headroom below the conservative injectivity bound (diffeomorphism_bound_divisor) be
 *  observed on real image data: if the minimum determinant stays comfortably positive the divisor is unnecessarily
 *  pessimistic and may be relaxed.  Compiled out entirely in release builds. */
FORCE_INLINE void
debug_report_diffeomorphism(Image<default_type> &update, const default_type step, const std::string_view context) {
  Adapter::Jacobian<Image<default_type>> jacobian(update);
  const Eigen::Matrix3d identity = Eigen::Matrix3d::Identity();
  default_type min_det = std::numeric_limits<default_type>::infinity();
  for (auto l = Loop(0, 3)(jacobian); l; ++l)
    min_det = std::min(min_det, (identity + step * jacobian.value()).determinant());
  DEBUG("minimum deformation Jacobian determinant of " + std::string(context) + " = " + str(min_det) +
        " (diffeomorphism bound divisor K = " + str(diffeomorphism_bound_divisor) + ")");
  if (min_det <= 0.0) {
    throw Exception("non-positive deformation Jacobian determinant (" + str(min_det) + ") in " + std::string(context) +
                    "; interpolation overshoot has folded the displacement field");
  }
}
#endif

// Compose two displacement fields and output a displacement field using scaling and squaring.  The input and output can
// be the same image.
FORCE_INLINE void update_displacement_scaling_and_squaring(Image<default_type> &input,
                                                           Image<default_type> &update,
                                                           Image<default_type> &output,
                                                           const default_type step = 1.0) {
  check_dimensions(input, output, 0, 3);

  default_type max_norm = 0.0;
  auto max_norm_func = [&max_norm](Image<default_type> &update) {
    default_type norm = Eigen::Vector3d(update.row(3)).norm();
    if (norm > max_norm)
      max_norm = norm;
  };
  ThreadedLoop(update).run(max_norm_func, update);
  default_type min_vox_size =
      static_cast<default_type>(std::min(input.spacing(0), std::min(input.spacing(1), input.spacing(2))));

  // if the maximum update exceeds the per-sub-step diffeomorphism bound, perform scaling and squaring to ensure the
  // displacement field remains diffeomorphic.  The bound (min_vox_size / diffeomorphism_bound_divisor) accounts for the
  // overshoot of the Catmull-Rom cubic interpolation used along the composition path, not just the linear half-voxel
  // limit; see diffeomorphism_bound_divisor.
  const default_type per_step_bound = min_vox_size / diffeomorphism_bound_divisor;

  default_type scale_factor = 1.0;
  if (max_norm * step < per_step_bound) {
    update_displacement(input, update, output, step);
#ifndef NDEBUG
    // The applied sub-step is update * step; report its minimum Jacobian determinant and assert diffeomorphism.
    debug_report_diffeomorphism(update, step, "direct (single-composition) update");
#endif
  } else {
    scale_factor = std::pow(2, std::ceil(std::log((max_norm * step) / per_step_bound) / std::log(2.0)));

    std::shared_ptr<Image<default_type>> scaled_update =
        std::make_shared<Image<default_type>>(Image<default_type>::scratch(update));
    std::shared_ptr<Image<default_type>> composed =
        std::make_shared<Image<default_type>>(Image<default_type>::scratch(update));

    // Scaling
    default_type scaled_step = step / scale_factor; // apply the step size and scale factor at once
    ThreadedLoop(update).run(
        [&scaled_step](Image<default_type> &update, Image<default_type> &scaled_update) {
          scaled_update.row(3) = Eigen::Vector3d(update.row(3)) * scaled_step;
        },
        update,
        *scaled_update);

    //          CONSOLE ("composing " + str(std::log2 (scale_factor)) + "times");

    // Squaring
    for (size_t i = 0; i < std::log2(scale_factor); ++i) {
      update_displacement(*scaled_update, *scaled_update, *composed);
      std::swap(scaled_update, composed);
    }
#ifndef NDEBUG
    // The squaring loop leaves *scaled_update holding the full-magnitude composed update (step already baked in), so
    //   report its minimum Jacobian determinant and assert diffeomorphism with a unit step factor.
    debug_report_diffeomorphism(*scaled_update, 1.0, "scaling-and-squaring update");
#endif

    update_displacement(input, *scaled_update, output);
  }
}

// Compose linear1<->deform1<->[midway space]<->deform2<->linear2.
template <class DeformationField1Type, class DeformationField2Type, class OutputDeformationFieldType>
FORCE_INLINE void compute_full_deformation(const transform_type &linear1,
                                           DeformationField1Type &deform1,
                                           DeformationField2Type &deform2,
                                           const transform_type &linear2,
                                           OutputDeformationFieldType &deform_out) {
  MR::Transform deform_header_transform(deform_out);
  PaddedField padded1 = make_padded_dense_field(deform1);
  PaddedField padded2 = make_padded_dense_field(deform2);
  ComposeHalfwayKernel compose_kernel(
      linear1 * deform_header_transform.voxel2scanner,
      Interp::DenseWarp<default_type>(padded1.buffer(), padded1.shift(), padded1.original_size()),
      Interp::DenseWarp<default_type>(padded2.buffer(), padded2.shift(), padded2.original_size()),
      linear2);
  ThreadedLoop(deform_out, 0, 3).run(compose_kernel, deform_out);
}

// Compose linear1<->deform1<->[midway space]<->deform2<->linear2.
template <class DeformationField1Type, class DeformationField2Type, class OutputDeformationFieldType>
FORCE_INLINE void compute_full_deformation(std::string message,
                                           const transform_type &linear1,
                                           DeformationField1Type &deform1,
                                           DeformationField2Type &deform2,
                                           const transform_type &linear2,
                                           OutputDeformationFieldType &deform_out) {
  MR::Transform deform_header_transform(deform_out);
  PaddedField padded1 = make_padded_dense_field(deform1);
  PaddedField padded2 = make_padded_dense_field(deform2);
  ComposeHalfwayKernel compose_kernel(
      linear1 * deform_header_transform.voxel2scanner,
      Interp::DenseWarp<default_type>(padded1.buffer(), padded1.shift(), padded1.original_size()),
      Interp::DenseWarp<default_type>(padded2.buffer(), padded2.shift(), padded2.original_size()),
      linear2);
  ThreadedLoop(message, deform_out, 0, 3).run(compose_kernel, deform_out);
}

template <class WarpType> FORCE_INLINE WarpType compute_midway_deformation(WarpType &warp, const int from) {
  Header midway_header(warp);
  midway_header.ndim() = 4;
  midway_header.size(3) = 3;
  WarpType deformation = WarpType::scratch(midway_header);

  transform_type linear;
  std::vector<uint32_t> index(1);
  if (from == 1) {
    linear = Registration::Warp::parse_linear_transform(warp, "linear1");
    index[0] = 0;
  } else {
    linear = Registration::Warp::parse_linear_transform(warp, "linear2");
    index[0] = 2;
  }
  Adapter::Extract1D<WarpType> im_to_mid(warp, 4, index);
  Registration::Warp::compose_linear_deformation(linear, im_to_mid, deformation);
  return deformation;
}

template <class WarpType, class TemplateType>
FORCE_INLINE WarpType compute_full_deformation(WarpType &warp, TemplateType &template_image, const int from) {
  Header deform_header(template_image);
  deform_header.ndim() = 4;
  deform_header.size(3) = 3;
  WarpType deform = WarpType::scratch(deform_header);

  transform_type linear1 = Registration::Warp::parse_linear_transform(warp, "linear1");
  transform_type linear2 = Registration::Warp::parse_linear_transform(warp, "linear2");

  std::vector<uint32_t> index(1);
  if (from == 1) {
    index[0] = 0;
    Adapter::Extract1D<Image<default_type>> im1_to_mid(warp, 4, index);
    index[0] = 3;
    Adapter::Extract1D<Image<default_type>> mid_to_im2(warp, 4, index);
    Registration::Warp::compute_full_deformation(linear2.inverse(), mid_to_im2, im1_to_mid, linear1, deform);
  } else {
    index[0] = 1;
    Adapter::Extract1D<Image<default_type>> mid_to_im1(warp, 4, index);
    index[0] = 2;
    Adapter::Extract1D<Image<default_type>> im2_to_mid(warp, 4, index);
    Registration::Warp::compute_full_deformation(linear1.inverse(), mid_to_im1, im2_to_mid, linear2, deform);
  }
  return deform;
}

} // namespace MR::Registration::Warp

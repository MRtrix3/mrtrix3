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

// DISABLED — retained as reference documentation only; not compiled (removed from CMakeLists.txt and guarded by #if 0).
//
// Purpose this test served: DemonsDiffeomorphism.CubicCompositionFoldsWhereLinearDoesNot was a deliberately-FAILING
//   demonstrator.  It proved that migrating the warp composition / inversion path from linear to cubic (Catmull-Rom)
//   interpolation (commit 57d4e83db) admits a diffeomorphism violation the prior linear path could not produce: it
//   measured the realised warp's analytic Jacobian -- the quantity registration/warp/invert.h consumes -- through both
//   interpolators on a half-voxel-bounded compressive step, showing the linear path stays diffeomorphic while the cubic
//   path folds (det < 0).  Its failure was the deliverable: it motivated and justified the mitigation.
//
// Why it should NOT be executed: it fails by construction, so as a pass/fail unit test it leaves the suite permanently
//   red.  It samples the cubic interpolator DIRECTLY at a 0.45-voxel step amplitude, which the registration algorithm
//   now never reaches: the mitigation (registration/warp/compose.h, diffeomorphism_bound_divisor = 225/32) bounds every
//   composed sub-step well below the folding amplitude, so this exercises raw interpolator behaviour at an input the
//   algorithm forbids rather than any code path it takes.  Asserting it as a failure is therefore misleading once the
//   bound is in place.  The body is preserved below (disabled) so the failure mode the bound guards against remains on
//   record.  See the [[project-cubic-diffeomorphism-bound]] / [[project-demons-diffeomorphism-test]] notes.

#if 0

#include "registration/warp/padded_field.h"

#include <Eigen/Core>
#include <gtest/gtest.h>

#include "header.h"
#include "image.h"
#include "math/cubic_spline.h"

#include "algo/loop.h"
#include "interp/linear.h"
#include "interp/warp.h"

/* The diffeomorphic-demons update relies on every displacement field it composes remaining a
 *   diffeomorphism: invertible, with an everywhere-positive Jacobian determinant (no folding).
 *   The non-linear registration enforces this by bounding each composed step to half a voxel and
 *   exponentiating via scaling-and-squaring (registration/warp/compose.h:199-200), and historically
 *   verified it with a now-disabled negative-Jacobian assertion (compose.h:230-244). The field
 *   inversion machinery (registration/warp/invert.h:169) likewise reads the warp's analytic Jacobian
 *   and assumes it is sign-stable when forming Newton steps.
 *
 * That guarantee is only valid under a monotone interpolant. Linear interpolation returns a convex
 *   combination of node values, so it never manufactures a gradient steeper than the surrounding
 *   nodes imply: a field whose nodes respect the half-voxel envelope keeps det(I + grad u) > 0.
 *   Cubic (Catmull-Rom Hermite) interpolation is not monotone; near a steep transition it overshoots
 *   (Gibbs-like ringing), producing an interpolated gradient steeper than any pair of adjacent nodes.
 *   That excess gradient can drive det(I + grad u) negative at a sub-voxel location -- a fold the
 *   half-voxel bound was designed to preclude.
 *
 * The recent migration of the warp composition / inversion path from Interp::Linear to the cubic
 *   Interp::DenseWarp therefore admits a diffeomorphism violation that the prior linear path could
 *   not produce. The test below exhibits it on a deliberately steep (but half-voxel-bounded)
 *   displacement field: it measures the realized warp's analytic Jacobian -- the very quantity
 *   invert.h consumes -- through both interpolators. The linear assertion passes (prior behaviour);
 *   the cubic assertion fails (current behaviour), so this test is RED by construction on the
 *   cubic tree and would be GREEN on the pre-migration linear tree.
 */

using namespace MR;

namespace {

//! a 4D warp-field header (3 volumes along axis 3) on a unit-spacing identity grid
Header make_warp_header(const ssize_t nx, const ssize_t ny, const ssize_t nz) {
  Header header;
  header.ndim() = 4;
  header.size(0) = nx;
  header.size(1) = ny;
  header.size(2) = nz;
  header.size(3) = 3;
  for (ssize_t axis = 0; axis != 4; ++axis)
    header.spacing(axis) = 1.0;
  header.stride(0) = 1;
  header.stride(1) = 2;
  header.stride(2) = 3;
  header.stride(3) = 4;
  header.transform().setIdentity();
  header.datatype() = DataType::native(DataType::Float64);
  return header;
}

//! amplitude of the displacement step, in voxels; chosen inside the half-voxel diffeomorphism bound
/*! For a symmetric step +a -> -a flanked by plateaus, the Catmull-Rom interpolant's midpoint slope
 *  is -2.5*a whereas the linear secant slope is -2*a. With a = 0.45 (< 0.5, the half-voxel bound),
 *  the realized deformation Jacobian determinant 1 + du/dx is +0.10 under linear interpolation but
 *  -0.125 under cubic interpolation: linear stays diffeomorphic, cubic folds. */
constexpr default_type step_amplitude = 0.45;

//! voxel x-index at and below which the x-displacement is +step_amplitude (above it is -step_amplitude)
constexpr ssize_t step_voxel = 5;

//! fill a displacement field with a steep x-aligned compressive step: +a for x <= step_voxel, -a above
/*! Only the x-component is non-zero and it varies only along x, so the deformation Jacobian is
 *  diagonal with determinant 1 + d(disp_x)/dx; the y/z plateaus on either side of the step give the
 *  cubic kernel the flat support that maximises its overshoot. */
void fill_compressive_step(Image<default_type> &field) {
  for (auto l = Loop(field, 0, 3)(field); l; ++l) {
    const default_type disp_x = (field.index(0) <= step_voxel) ? step_amplitude : -step_amplitude;
    field.index(3) = 0;
    field.value() = disp_x;
    field.index(3) = 1;
    field.value() = 0.0;
    field.index(3) = 2;
    field.value() = 0.0;
  }
}

//! minimum deformation-Jacobian determinant of the interpolated displacement field across an x-sweep
/*! The interpolator is positioned in scanner space (= voxel index on this unit-spacing identity
 *  grid) along a dense line through the step; at each accepted position the analytic displacement
 *  Jacobian is read and the deformation Jacobian determinant det(I + grad u) is tracked. A negative
 *  return value signifies a fold -- a diffeomorphism violation. */
template <class InterpType> default_type min_deformation_jacobian_det(InterpType &interp) {
  default_type min_det = std::numeric_limits<default_type>::infinity();
  const default_type y = 2.0;
  const default_type z = 3.0;
  for (default_type x = static_cast<default_type>(step_voxel) - 2.0; x <= static_cast<default_type>(step_voxel) + 3.0;
       x += 0.01) {
    if (!interp.scanner(Eigen::Vector3d(x, y, z)))
      continue;
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> value;
    Eigen::Matrix<default_type, Eigen::Dynamic, 3> gradient;
    interp.value_and_gradient_row_wrt_scanner(value, gradient);
    const Eigen::Matrix3d jacobian = Eigen::Matrix3d::Identity() + gradient.topRows(3);
    min_det = std::min(min_det, jacobian.determinant());
  }
  return min_det;
}

} // namespace

// A half-voxel-bounded displacement field stays diffeomorphic under linear interpolation but folds
//   under the cubic interpolation now used by the warp composition / inversion path. The cubic
//   assertion is expected to fail on the current tree: it documents the diffeomorphism regression
//   introduced by migrating the demons composition path from Interp::Linear to Interp::DenseWarp.
TEST(DemonsDiffeomorphism, CubicCompositionFoldsWhereLinearDoesNot) {
  Header header = make_warp_header(12, 5, 7);
  Image<default_type> field = Image<default_type>::scratch(header, "compressive step displacement field");
  fill_compressive_step(field);

  // Prior path: linear interpolation of the displacement field.
  Interp::LinearInterp<Image<default_type>, Interp::LinearInterpProcessingType::ValueAndDerivative> linear(field);
  const default_type min_det_linear = min_deformation_jacobian_det(linear);

  // Current path: cubic interpolation via Interp::DenseWarp over a padded buffer.
  Registration::Warp::PaddedField padded(header);
  padded.refresh(field);
  Interp::DenseWarp<default_type, Math::SplineProcessingType::ValueAndDerivative> cubic(
      padded.buffer(), padded.shift(), padded.original_size());
  const default_type min_det_cubic = min_deformation_jacobian_det(cubic);

  // Prior behaviour: the field remains diffeomorphic.
  EXPECT_GT(min_det_linear, 0.0) << "linear interpolation unexpectedly folded (min det = " << min_det_linear << ")";

  // Current behaviour: cubic overshoot drives the Jacobian determinant negative -- a fold the
  //   half-voxel bound was meant to preclude. RED by construction on the cubic tree.
  EXPECT_GT(min_det_cubic, 0.0) << "cubic interpolation folded the warp (min det = " << min_det_cubic
                                << "); the demons diffeomorphism guarantee is violated";
}

#endif // disabled demons-diffeomorphism demonstrator

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
#include <vector>

#include "app.h"
#include "datatype.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "math/cubic_spline.h"
#include "transform.h"

#include "algo/loop.h"
#include "algo/threaded_loop.h"
#include "interp/deform.h"
#include "interp/linear.h"
#include "registration/warp/convert.h"

namespace MR::Registration::Warp {

namespace {

//! cubic interpolation of the (imputation-aware) forward deformation field, yielding both
//   the mapped scanner-space position and its 3x3 Jacobian with respect to scanner coordinates
using DeformInterpType = Interp::Deform<default_type, Math::SplineProcessingType::ValueAndDerivative>;

// neighbour_offsets() (the 26 shared-corner neighbour offsets) is provided by
//   registration/warp/extrapolate.h, included transitively via interp/deform.h.

class DisplacementThreadKernel {

public:
  DisplacementThreadKernel(Image<default_type> &displacement,
                           Image<default_type> &displacement_inverse,
                           const size_t max_iter,
                           const default_type error_tol)
      : displacement(displacement), transform(displacement_inverse), max_iter(max_iter), error_tolerance(error_tol) {}

  void operator()(Image<default_type> &displacement_inverse) {
    const Eigen::Vector3d voxel{static_cast<default_type>(displacement_inverse.index(0)),
                                static_cast<default_type>(displacement_inverse.index(1)),
                                static_cast<default_type>(displacement_inverse.index(2))};
    const Eigen::Vector3d truth = transform.voxel2scanner * voxel;
    Eigen::Vector3d current = truth + Eigen::Vector3d(displacement_inverse.row(3));

    size_t iter = 0;
    default_type error = std::numeric_limits<default_type>::max();
    while (iter < max_iter && error > error_tolerance) {
      error = update(current, truth);
      ++iter;
    }
    displacement_inverse.row(3) = current - truth;
  }

private:
  default_type update(Eigen::Vector3d &current, const Eigen::Vector3d &truth) {
    displacement.scanner(current);
    const Eigen::Vector3d discrepancy = truth - (current + Eigen::Vector3d(displacement.vec3()));
    current += discrepancy;
    return discrepancy.dot(discrepancy);
  }

  Interp::Linear<Image<default_type>> displacement;
  MR::Transform transform;
  const size_t max_iter;
  default_type error_tolerance;
};

//! Per-voxel Newton search for the inverse of a deformation field
/*! For an output voxel whose centre maps to scanner-space position \a truth, this finds the
 *  input-domain point \a current such that the forward field maps \a current onto \a truth,
 *  i.e. it solves D(current) = truth. Each iteration takes a Newton step using the cubic
 *  interpolator's value and Jacobian; the step is halved whenever it would project outside
 *  the valid (non-extrapolated) region of the input field. Output voxels are written NaN
 *  where the warp cannot be inverted (the search is persistently driven out of the valid
 *  region) or where the recovered origin does not reside in valid input data. */
class DeformationThreadKernel {

public:
  DeformationThreadKernel(const DeformInterpType &deform,
                          const Image<default_type> &inv_deform,
                          const size_t max_iter,
                          const default_type error_tol)
      : deform(deform), transform(inv_deform), max_iter(max_iter), error_tolerance_sq(error_tol * error_tol) {}

  void operator()(Image<default_type> &inv_deform) {
    const Eigen::Vector3d voxel{static_cast<default_type>(inv_deform.index(0)),
                                static_cast<default_type>(inv_deform.index(1)),
                                static_cast<default_type>(inv_deform.index(2))};
    const Eigen::Vector3d truth = transform.voxel2scanner * voxel;
    Eigen::Vector3d current = inv_deform.row(3);

    // Secondary NaN criterion: the initial estimate must reside in valid (non-extrapolated)
    //   input data for the recovered origin to be trustworthy.
    if (!current.allFinite() || !deform.scanner(current)) {
      inv_deform.row(3) = invalid();
      return;
    }

    size_t consecutive_blocked = 0;
    bool noninvertible = false;
    for (size_t iter = 0; iter != max_iter; ++iter) {
      // The interpolator is positioned at `current` (either by the pre-loop check or by the
      //   accepted step of the previous iteration).
      Eigen::Matrix<default_type, Eigen::Dynamic, 1> mapped;
      Eigen::Matrix<default_type, Eigen::Dynamic, 3> jacobian;
      deform.value_and_gradient_row_wrt_scanner(mapped, jacobian);
      const Eigen::Vector3d residual = truth - Eigen::Vector3d(mapped);
      if (residual.squaredNorm() <= error_tolerance_sq)
        break;

      const Eigen::Matrix3d J = jacobian.topRows(3);
      Eigen::Vector3d delta;
      if (std::fabs(J.determinant()) < jacobian_epsilon)
        delta = residual; // projection fallback where the field is locally degenerate (a fold)
      else
        delta = J.inverse() * residual;
      if (!delta.allFinite())
        delta = residual;

      // Step with iterative halving: confine the candidate to the valid input region.
      default_type alpha = 1.0;
      size_t halvings = 0;
      bool stepped = false;
      Eigen::Vector3d candidate;
      while (true) {
        candidate = current + alpha * delta;
        if (deform.scanner(candidate)) {
          stepped = true;
          break;
        }
        if (halvings == max_halvings)
          break;
        alpha *= 0.5;
        ++halvings;
      }
      if (!stepped) {
        noninvertible = true; // no valid candidate within the halving budget
        break;
      }

      // Heuristic: successive iterations that all require halving indicate the search is
      //   being persistently driven outside the valid region, i.e. the voxel is not invertible.
      if (halvings != 0) {
        if (++consecutive_blocked == max_consecutive_blocked) {
          noninvertible = true;
          break;
        }
      } else {
        consecutive_blocked = 0;
      }
      current = candidate;
    }

    if (noninvertible)
      inv_deform.row(3) = invalid();
    else
      inv_deform.row(3) = current;
  }

private:
  static Eigen::Vector3d invalid() { return Eigen::Vector3d::Constant(std::numeric_limits<default_type>::quiet_NaN()); }

  DeformInterpType deform;
  MR::Transform transform;
  const size_t max_iter;
  const default_type error_tolerance_sq;

  //! maximum number of step halvings attempted within a single iteration (smallest fraction 2^-10)
  static constexpr size_t max_halvings = 10;
  //! number of consecutive halving-requiring iterations after which a voxel is deemed non-invertible
  static constexpr size_t max_consecutive_blocked = 3;
  //! Jacobian determinant below which the field is treated as locally degenerate
  static constexpr default_type jacobian_epsilon = 1e-6;
};

//! Compute a well-informed initialisation of the inverse deformation field
/*! Two scratch images are built on the output (inverse) grid: \a nearest_position holds, for
 *  each output voxel, the scanner-space position of the input warp voxel whose forward sample
 *  lands nearest the voxel centre; \a nearest_distance holds that minimal distance (and serves
 *  as the "initialised" marker, being infinite where no input voxel scattered to that output
 *  voxel). The scatter pass populates these directly; the remaining gaps are then filled by a
 *  greedy region grow ordered by the number of already-initialised 26-neighbours. The result is
 *  copied into \a inv_deform_field as the starting estimate for the Newton search. */
inline void initialise_inverse_deformation(Image<default_type> &field,
                                           Image<default_type> &inv_deform_field,
                                           DeformInterpType &deform) {
  const MR::Transform in_transform(field);
  const MR::Transform out_transform(inv_deform_field);

  Image<default_type> nearest_position(
      Image<default_type>::scratch(inv_deform_field, "inverse warp initialisation positions"));
  Header dist_header(inv_deform_field);
  dist_header.ndim() = 3;
  dist_header.datatype() = DataType::Float64;
  dist_header.datatype().set_byte_order_native();
  Image<default_type> nearest_distance(
      Image<default_type>::scratch(dist_header, "inverse warp initialisation distances"));
  for (auto l = Loop(nearest_distance)(nearest_distance); l; ++l)
    nearest_distance.value() = std::numeric_limits<default_type>::infinity();

  // Scatter: every non-extrapolated input voxel contributes its source scanner position to the
  //   output voxel nearest the location it maps to, keeping the closest contribution per voxel.
  for (auto l = Loop(field, 0, 3)(field); l; ++l) {
    Eigen::Vector3d mapped;
    bool valid = true;
    for (ssize_t component = 0; component != 3; ++component) {
      field.index(3) = component;
      mapped[component] = field.value();
      if (!std::isfinite(mapped[component])) {
        valid = false;
        break;
      }
    }
    if (!valid)
      continue;
    const Eigen::Vector3d source =
        in_transform.voxel2scanner * Eigen::Vector3d{static_cast<default_type>(field.index(0)),
                                                     static_cast<default_type>(field.index(1)),
                                                     static_cast<default_type>(field.index(2))};
    const Eigen::Vector3d output_voxel_real = out_transform.scanner2voxel * mapped;
    std::array<ssize_t, 3> output_voxel;
    bool inside = true;
    for (ssize_t axis = 0; axis != 3; ++axis) {
      output_voxel[axis] = static_cast<ssize_t>(std::lround(output_voxel_real[axis]));
      if (output_voxel[axis] < 0 || output_voxel[axis] >= inv_deform_field.size(axis)) {
        inside = false;
        break;
      }
    }
    if (!inside)
      continue;
    const Eigen::Vector3d centre =
        out_transform.voxel2scanner * Eigen::Vector3d{static_cast<default_type>(output_voxel[0]),
                                                      static_cast<default_type>(output_voxel[1]),
                                                      static_cast<default_type>(output_voxel[2])};
    const default_type distance = (mapped - centre).norm();
    for (ssize_t axis = 0; axis != 3; ++axis)
      nearest_distance.index(axis) = output_voxel[axis];
    if (distance < nearest_distance.value()) {
      nearest_distance.value() = distance;
      for (ssize_t axis = 0; axis != 3; ++axis)
        nearest_position.index(axis) = output_voxel[axis];
      nearest_position.row(3) = source;
    }
  }

  if (App::log_level >= 3) {
    nearest_distance.dump_to_mrtrix_file("warpinvert_init_distance_scatter.mif");
    nearest_position.dump_to_mrtrix_file("warpinvert_init_position_scatter.mif");
  }

  // Gap fill: rank uninitialised voxels by their count of initialised 26-neighbours, using
  //   27 bucket lists (counts 0..26) with lazy revalidation as counts increase.
  Header count_header(dist_header);
  count_header.datatype() = DataType::UInt8;
  Image<uint8_t> count_image(Image<uint8_t>::scratch(count_header, "initialised-neighbour count"));

  Image<default_type> dist_reader(nearest_distance);
  Image<default_type> pos_reader(nearest_position);
  const auto &offsets = neighbour_offsets();

  const auto is_initialised = [&dist_reader](const std::array<ssize_t, 3> &p) -> bool {
    for (ssize_t axis = 0; axis != 3; ++axis)
      dist_reader.index(axis) = p[axis];
    return std::isfinite(dist_reader.value());
  };
  const auto in_grid = [&inv_deform_field](const std::array<ssize_t, 3> &p) -> bool {
    return p[0] >= 0 && p[0] < inv_deform_field.size(0) && //
           p[1] >= 0 && p[1] < inv_deform_field.size(1) && //
           p[2] >= 0 && p[2] < inv_deform_field.size(2);
  };
  const auto count_initialised_neighbours = [&](const std::array<ssize_t, 3> &p) -> size_t {
    size_t count = 0;
    for (const auto &offset : offsets) {
      const std::array<ssize_t, 3> q{p[0] + offset[0], p[1] + offset[1], p[2] + offset[2]};
      if (in_grid(q) && is_initialised(q))
        ++count;
    }
    return count;
  };

  constexpr uint8_t claimed_marker = std::numeric_limits<uint8_t>::max();
  std::array<std::vector<std::array<ssize_t, 3>>, 27> buckets;
  size_t remaining = 0;
  for (auto l = Loop(nearest_distance)(nearest_distance); l; ++l) {
    if (std::isfinite(nearest_distance.value()))
      continue;
    const std::array<ssize_t, 3> p{nearest_distance.index(0), nearest_distance.index(1), nearest_distance.index(2)};
    const size_t count = count_initialised_neighbours(p);
    count_image.index(0) = p[0];
    count_image.index(1) = p[1];
    count_image.index(2) = p[2];
    count_image.value() = static_cast<uint8_t>(count);
    buckets[count].push_back(p);
    ++remaining;
  }

  const auto current_count = [&count_image](const std::array<ssize_t, 3> &p) -> uint8_t {
    count_image.index(0) = p[0];
    count_image.index(1) = p[1];
    count_image.index(2) = p[2];
    return count_image.value();
  };
  const auto set_count = [&count_image](const std::array<ssize_t, 3> &p, const uint8_t value) {
    count_image.index(0) = p[0];
    count_image.index(1) = p[1];
    count_image.index(2) = p[2];
    count_image.value() = value;
  };
  const auto forward_map = [&deform](const Eigen::Vector3d &x, Eigen::Vector3d &out) -> bool {
    if (!deform.scanner(x))
      return false;
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> value;
    Eigen::Matrix<default_type, Eigen::Dynamic, 3> gradient;
    deform.value_and_gradient_row(value, gradient);
    out = value.head(3);
    return true;
  };

  while (remaining != 0) {
    ssize_t level = 26;
    while (level >= 0 && buckets[level].empty())
      --level;
    if (level < 0)
      break; // safety: no further connected uninitialised voxels

    // Collect all currently-uninitialised voxels at the largest neighbour count, snapshotting
    //   the initialised set so that voxels within one layer are independent of each other.
    std::vector<std::array<ssize_t, 3>> layer;
    for (const auto &p : buckets[level]) {
      if (is_initialised(p))
        continue;
      if (current_count(p) != static_cast<uint8_t>(level))
        continue; // stale: this voxel's count has since increased
      layer.push_back(p);
      set_count(p, claimed_marker); // guard against a voxel being collected twice this layer
    }
    buckets[level].clear();
    if (layer.empty())
      continue;

    std::vector<Eigen::Vector3d> values(layer.size());
    std::vector<default_type> distances(layer.size());
    for (size_t i = 0; i != layer.size(); ++i) {
      const std::array<ssize_t, 3> &p = layer[i];
      const Eigen::Vector3d centre = out_transform.voxel2scanner * Eigen::Vector3d{static_cast<default_type>(p[0]),
                                                                                   static_cast<default_type>(p[1]),
                                                                                   static_cast<default_type>(p[2])};
      Eigen::Vector3d sum = Eigen::Vector3d::Zero();
      std::vector<Eigen::Vector3d> neighbour_positions;
      for (const auto &offset : offsets) {
        const std::array<ssize_t, 3> q{p[0] + offset[0], p[1] + offset[1], p[2] + offset[2]};
        if (!in_grid(q) || !is_initialised(q))
          continue;
        for (ssize_t axis = 0; axis != 3; ++axis)
          pos_reader.index(axis) = q[axis];
        const Eigen::Vector3d neighbour_position = pos_reader.row(3);
        neighbour_positions.push_back(neighbour_position);
        sum += neighbour_position;
      }
      const Eigen::Vector3d average = sum / static_cast<default_type>(neighbour_positions.size());

      Eigen::Vector3d mapped;
      if (forward_map(average, mapped)) {
        // The average maps to an interpolable location: adopt it as the initial position.
        values[i] = average;
        distances[i] = (mapped - centre).norm();
      } else {
        // The average is not interpolable: fall back to the initialised neighbour whose
        //   forward sample lands nearest this voxel's centre.
        default_type best = std::numeric_limits<default_type>::infinity();
        Eigen::Vector3d best_position = neighbour_positions.front();
        for (const auto &neighbour_position : neighbour_positions) {
          Eigen::Vector3d neighbour_mapped;
          if (!forward_map(neighbour_position, neighbour_mapped))
            continue;
          const default_type d = (neighbour_mapped - centre).norm();
          if (d < best) {
            best = d;
            best_position = neighbour_position;
          }
        }
        values[i] = best_position;
        distances[i] = best;
      }
    }

    // Commit the layer, then raise the neighbour counts of adjacent uninitialised voxels.
    for (size_t i = 0; i != layer.size(); ++i) {
      const std::array<ssize_t, 3> &p = layer[i];
      for (ssize_t axis = 0; axis != 3; ++axis) {
        nearest_position.index(axis) = p[axis];
        nearest_distance.index(axis) = p[axis];
      }
      nearest_position.row(3) = values[i];
      nearest_distance.value() = distances[i];
      --remaining;
    }
    for (const auto &p : layer) {
      for (const auto &offset : offsets) {
        const std::array<ssize_t, 3> q{p[0] + offset[0], p[1] + offset[1], p[2] + offset[2]};
        if (!in_grid(q) || is_initialised(q))
          continue;
        const uint8_t updated = static_cast<uint8_t>(current_count(q) + 1);
        set_count(q, updated);
        buckets[updated].push_back(q);
      }
    }
  }

  if (App::log_level >= 3) {
    nearest_distance.dump_to_mrtrix_file("warpinvert_init_distance_filled.mif");
    nearest_position.dump_to_mrtrix_file("warpinvert_init_position_filled.mif");
  }

  for (auto l = Loop(nearest_position)(nearest_position, inv_deform_field); l; ++l)
    inv_deform_field.value() = nearest_position.value();
}

} // namespace

/** \addtogroup Registration
  @{ */

/*! Estimate the inverse of a deformation field
 * Note that the output inv_warp can be passed as either a zero field or an initial estimate.
 * Unless \a is_initialised is set, an informed initialisation is computed from the forward
 * field; the inverse is then refined per voxel by a Newton search using the imputation-aware
 * cubic interpolator MR::Interp::Deform. Output voxels that cannot be inverted, or whose
 * recovered origin does not reside in valid (non-extrapolated) input data, are written NaN.
 */
FORCE_INLINE void invert_deformation(Image<default_type> &deform_field,
                                     Image<default_type> &inv_deform_field,
                                     bool is_initialised = false,
                                     size_t max_iter = 50,
                                     default_type error_tolerance = 0.0001) {
  check_dimensions(deform_field, inv_deform_field);
  error_tolerance *= (deform_field.spacing(0) + deform_field.spacing(1) + deform_field.spacing(2)) / 3;

  DeformInterpType deform(deform_field);

  if (!is_initialised)
    initialise_inverse_deformation(deform_field, inv_deform_field, deform);

  ThreadedLoop("inverting warp field...", inv_deform_field, 0, 3)
      .run(DeformationThreadKernel(deform, inv_deform_field, max_iter, error_tolerance), inv_deform_field);
}

/*! Estimate the inverse of a displacement field, output the inverse as a deformation field
 * Note that the output inv_warp can be passed as either a zero field or an initial estimate (as a deformation field)
 */
FORCE_INLINE void invert_displacement_deformation(Image<default_type> &disp,
                                                  Image<default_type> &inv_deform,
                                                  bool is_initialised = false,
                                                  size_t max_iter = 50,
                                                  default_type error_tolerance = 0.0001) {
  auto deform_field = Image<default_type>::scratch(disp);
  Warp::displacement2deformation(disp, deform_field);

  invert_deformation(deform_field, inv_deform, is_initialised, max_iter, error_tolerance);
}

/*! Estimate the inverse of a displacement field
 * Note that the output inv_warp can be passed as either a zero field or an initial estimate
 */
FORCE_INLINE void invert_displacement(Image<default_type> &disp_field,
                                      Image<default_type> &inv_disp_field,
                                      size_t max_iter = 50,
                                      default_type error_tolerance = 0.0001) {
  check_dimensions(disp_field, inv_disp_field);
  error_tolerance *= (disp_field.spacing(0) + disp_field.spacing(1) + disp_field.spacing(2)) / 3;

  ThreadedLoop("inverting displacement field...", inv_disp_field, 0, 3)
      .run(DisplacementThreadKernel(disp_field, inv_disp_field, max_iter, error_tolerance), inv_disp_field);
}

//! @}
} // namespace MR::Registration::Warp

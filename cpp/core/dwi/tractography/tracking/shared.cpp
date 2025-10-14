/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "dwi/tractography/tracking/shared.h"

namespace MR::DWI::Tractography::Tracking {

SharedBase::SharedBase(const std::string &diff_path, Properties &property_set)
    : source_header(Header::open(diff_path)),
      source(source_header.get_image<float>().with_direct_io(3)),
      properties(property_set),
      init_dir(Eigen::Vector3f::Constant(NaN)),
      min_num_points_preds(0),
      max_num_points_preds(0),
      min_num_points_postds(0),
      max_num_points_postds(0),
      min_dist(NaN),
      max_dist(NaN),
      max_angle_1o(NaN),
      max_angle_ho(NaN),
      cos_max_angle_1o(NaN),
      cos_max_angle_ho(NaN),
      step_size(NaN),
      min_radius(NaN),
      threshold(NaN),
      unidirectional(false),
      rk4(false),
      stop_on_all_include(false),
      implicit_max_num_seeds(properties.find("max_num_seeds") == properties.end()),
      downsampler(1)
#ifdef DEBUG_TERMINATIONS
      ,
      debug_header(Header::open(properties.find("act") == properties.end() ? diff_path : properties["act"])),
      transform(debug_header)
#endif
{
  for (size_t i = 0; i != termination_reason_count; ++i)
    std::atomic_init(&terminations[i], 0);
  for (size_t i = 0; i != rejection_reason_count; ++i)
    std::atomic_init(&rejections[i], 0);

  if (properties.find("max_num_tracks") == properties.end())
    max_num_tracks = (properties.find("max_num_seeds") == properties.end()) ? Defaults::num_selected_tracks : 0;
  properties.set(max_num_tracks, "max_num_tracks");

  properties.set(unidirectional, "unidirectional");
  properties.set(rk4, "rk4");
  properties.set(stop_on_all_include, "stop_on_all_include");

  properties["source"] = source_header.name();

  max_num_seeds = Defaults::seed_to_select_ratio * max_num_tracks;
  properties.set(max_num_seeds, "max_num_seeds");

  assert(properties.seeds.num_seeds());
  max_seed_attempts = properties.seeds[0]->get_max_attempts();
  properties.set(max_seed_attempts, "max_seed_attempts");

  if (properties.find("init_direction") != properties.end()) {
    auto V = parse_floats(properties["init_direction"]);
    if (V.size() != 3)
      throw Exception(std::string("invalid initial direction \"") + properties["init_direction"] + "\"");
    init_dir[0] = V[0];
    init_dir[1] = V[1];
    init_dir[2] = V[2];
    init_dir.normalize();
  }

  if (properties.find("act") != properties.end()) {
    act_shared_additions.reset(new ACT::ACT_Shared_additions(properties["act"], property_set));
    if (act().backtrack() && stop_on_all_include)
      throw Exception("Cannot use -stop option if ACT backtracking is enabled");
  }

  if (properties.find("downsample_factor") != properties.end())
    downsampler.set_ratio(to<int>(properties["downsample_factor"]));

  for (size_t i = 0; i != termination_reason_count; ++i)
    std::atomic_init(&terminations[i], 0);
  for (size_t i = 0; i != rejection_reason_count; ++i)
    std::atomic_init(&rejections[i], 0);
#ifdef DEBUG_TERMINATIONS
  debug_header.ndim() = 3;
  debug_header.datatype() = DataType::UInt32;
  for (const auto &i : termination_info)
    debug_images.emplace_back(
        new Image<uint32_t>(Image<uint32_t>::create("terms_" + i.second.name + ".mif", debug_header)));
#endif
}

SharedBase::~SharedBase() {
  size_t sum_terminations = 0;
  for (const auto &i : terminations)
    sum_terminations += i;
  INFO("Total number of track terminations: " + str(sum_terminations));
  INFO("Termination reason probabilities:");
  for (const auto &i : termination_info) {
    if (termination_relevant(i.first))
      INFO("  " + i.second.description + ": " +
           str(100.0 * terminations[static_cast<ssize_t>(i.first)] / (double)sum_terminations, 3) + "\%");
  }

  INFO("Track rejection counts:");
  for (const auto &i : rejection_strings) {
    if (rejection_relevant(i.first))
      INFO("  " + i.second + ": " + str(rejections[static_cast<ssize_t>(i.first)]));
  }
}

void SharedBase::set_step_and_angle(const float voxel_frac,
                                    const float angle,
                                    const intrinsic_integration_order_t intrinsic_integration_order,
                                    const curvature_constraint_t curvature_constraint_type) {
  step_size = voxel_frac * vox();
  properties.set(step_size, "step_size");
  INFO("step size = " + str(step_size) + " mm");

  max_dist = Defaults::maxlength_voxels * vox();
  properties.set(max_dist, "max_dist");

  min_dist = is_act() ? (Defaults::minlength_voxels_withact * vox()) : (Defaults::minlength_voxels_noact * vox());
  properties.set(min_dist, "min_dist");

  max_angle_1o = angle;
  properties.set(max_angle_1o, "max_angle");
  std::string angle_msg;
  switch (intrinsic_integration_order) {
  case intrinsic_integration_order_t::FIRST:
    angle_msg = "maximum deviation angle per step";
    break;
  case intrinsic_integration_order_t::HIGHER:
    angle_msg = "maximum angular change in fibre orientation per step";
    break;
  }
  INFO(angle_msg + " = " + str(max_angle_1o) + " deg");
  max_angle_1o *= Math::pi / 180.0;
  cos_max_angle_1o = std::cos(max_angle_1o);
  min_radius = step_size / (2.0f * std::sin(0.5f * max_angle_1o));
  INFO("Minimum radius of curvature = " + str(min_radius) + "mm");

  if (intrinsic_integration_order == intrinsic_integration_order_t::HIGHER) {
    max_angle_ho = max_angle_1o;
    cos_max_angle_ho = cos_max_angle_1o;
    // Clear these variables so that the next() function of the underlying method
    //   does not enforce curvature constraints; rely on e.g. RK4 to do it
    max_angle_1o = float(Math::pi);
    cos_max_angle_1o = 0.0f;
  }

  // If the curvature constraint gets applied implicitly during path propagation,
  //   rather than having the orientation determined and then checked against the curvature constraint,
  //   then it is impossible for a streamline to be terminated specifically due to a curvature constraint;
  //   this should therefore be omitted from reporting of termination statistics
  curvature_constraint = curvature_constraint_type;
}

void SharedBase::set_num_points() {
  // Angle around the circle of minimum radius for the given step size
  const float angle_minradius_preds = 2.0f * std::asin(step_size / (2.0f * min_radius));
  // Maximum inter-vertex distance after streamline has been downsampled
  const float max_step_postds = downsampler.get_ratio() * step_size;

  set_num_points(angle_minradius_preds, max_step_postds);
}

void SharedBase::set_num_points(const float angle_minradius_preds, const float max_step_postds) {
  // Maximal angle around this minimum radius traversed after downsampling
  const float angle_minradius_postds = downsampler.get_ratio() * angle_minradius_preds;
  // Minimum chord length after streamline has been downsampled
  const float min_step_postds = (angle_minradius_postds > float(2.0 * Math::pi))
                                    ? 0.0f
                                    : (2.0f * min_radius * std::sin(0.5f * angle_minradius_postds));

  // What we need:
  //   - Before downsampling:
  //     - How many points must be generated in order for it to be feasible that the
  //         streamline may exceed the minimum length after downsampling?
  //       (If a streamline doesn't reach this number of vertices, there's no point in
  //         even attempting any further processing of it; it will always be rejected)
  min_num_points_preds = 1 + std::ceil(min_dist / step_size);
  //     - How many points before it is no longer feasible to become shorter than the
  //         maximum length, even after down-sampling?
  //       (There is no point in continuing streamlines propagation after this point;
  //         it will invariably be either truncated or rejected, no matter what
  //         happens during downsampling)
  max_num_points_preds = min_step_postds ? (3 + std::ceil(downsampler.get_ratio() * max_dist / min_step_postds))
                                         : std::numeric_limits<size_t>::max();
  //   - After downsampling:
  //     - How many vertices must a streamline have (after downsampling) for it to be
  //         guaranteed to exceed the minimum length?
  //       (If a streamline has less than this number of vertices after downsampling, we
  //         need to quantify its length precisely and compare against the minimum)
  min_num_points_postds = 3 + std::ceil(min_dist / min_step_postds);
  //     - How many vertices can a streamline have (after downsampling) for it to be
  //         guaranteed to be shorter than the maximum length?
  //       (If a streamline has more than this number of vertices after downsampling, we
  //         need to quantify its length precisely and compare against the maximum)
  max_num_points_postds = 1 + std::floor(max_dist / max_step_postds);

  DEBUG("For tracking step size " + str(step_size) + "mm, " +
        (std::isfinite(max_angle_ho)
             ? ("max change in fibre orientation angle per step " + str(max_angle_ho * 180.0 / Math::pi, 6) +
                " deg (using RK4)")
             : ("max angle deviation per step " + str(max_angle_1o * 180.0 / Math::pi, 6) + "deg")) +
        ", minimum radius of curvature " + str(min_radius, 6) + "mm, downsampling ratio " +
        str(downsampler.get_ratio()) + ": " + "minimum length of " + str(min_dist) + "mm requires at least " +
        str(min_num_points_preds) + " vertices pre-DS, is tested explicitly for " + str(min_num_points_postds) +
        " vertices or less post-DS; " + "maximum length of " + str(max_dist) + "mm will stop tracking after " +
        str(max_num_points_preds) + " vertices pre-DS, is tested explicitly for " + str(max_num_points_postds) +
        " or more vertices post-DS");
}

void SharedBase::set_cutoff(float cutoff) {
  threshold = cutoff;
  properties.set(threshold, "threshold");
  init_threshold = threshold;
  properties.set(init_threshold, "init_threshold");
}

#ifdef DEBUG_TERMINATIONS
void SharedBase::add_termination(const term_t i, const Eigen::Vector3f &p) const {
  terminations[i].fetch_add(1, std::memory_order_relaxed);
  Image<uint32_t> image(*debug_images[i]);
  const auto pv = transform.scanner2voxel * p.cast<default_type>();
  image.index(0) = ssize_t(std::round(pv[0]));
  image.index(1) = ssize_t(std::round(pv[1]));
  image.index(2) = ssize_t(std::round(pv[2]));
  if (!is_out_of_bounds(image))
    image.value() += 1;
}
#endif

bool SharedBase::termination_relevant(const term_t i) const {
  switch (i) {
  case term_t::CONTINUE:
    return false;
  case term_t::ENTER_CGM:
    return is_act();
  case term_t::CALIBRATOR:
    return true;
  case term_t::EXIT_IMAGE:
    return true;
  case term_t::ENTER_CSF:
    return is_act();
  case term_t::MODEL:
    return true;
  case term_t::HIGH_CURVATURE:
    return curvature_constraint == curvature_constraint_t::POSTHOC_THRESHOLD;
  case term_t::LENGTH_EXCEED:
    return true;
  case term_t::TERM_IN_SGM:
    return is_act();
  case term_t::EXIT_SGM:
    return is_act();
  case term_t::EXIT_MASK:
    return !properties.mask.empty();
  case term_t::ENTER_EXCLUDE:
    return !properties.exclude.empty();
  case term_t::TRAVERSE_ALL_INCLUDE:
    return stop_on_all_include;
  }
  return false;
}

bool SharedBase::rejection_relevant(const reject_t i) const {
  switch (i) {
  case reject_t::INVALID_SEED:
    return true;
  case reject_t::NO_PROPAGATION_FROM_SEED:
    return true;
  case reject_t::TRACK_TOO_SHORT:
    return true;
  case reject_t::TRACK_TOO_LONG:
    return is_act();
  case reject_t::ENTER_EXCLUDE_REGION:
    return !properties.exclude.empty();
  case reject_t::MISSED_INCLUDE_REGION:
    return !properties.include.empty();
  case reject_t::ACT_POOR_TERMINATION:
    return is_act();
  case reject_t::ACT_FAILED_WM_REQUIREMENT:
    return is_act();
  }
  return false;
}

} // namespace MR::DWI::Tractography::Tracking

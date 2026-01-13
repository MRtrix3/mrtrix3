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

#include <atomic>

#include "dwi/tractography/ACT/shared.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/resampling/downsampler.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/tracking/tractography.h"
#include "dwi/tractography/tracking/types.h"
#include "header.h"
#include "image.h"
#include "memory.h"
#include "transform.h"

// If this is enabled, images will be output in the current directory showing the density of streamline terminations due
// to different termination mechanisms throughout the brain
// #define DEBUG_TERMINATIONS

namespace MR::DWI::Tractography::Tracking {

class SharedBase {

public:
  SharedBase(std::string_view diff_path, Properties &property_set);

  virtual ~SharedBase();

  Header source_header;
  Image<float> source;
  Properties &properties;
  Eigen::Vector3f init_dir;
  size_t max_num_tracks, max_num_seeds;
  size_t min_num_points_preds, max_num_points_preds;
  size_t min_num_points_postds, max_num_points_postds;
  float min_dist, max_dist;
  // Different variables for 1st-order integration vs. higher-order integration
  float max_angle_1o, max_angle_ho, cos_max_angle_1o, cos_max_angle_ho;
  float step_size, min_radius, threshold, init_threshold;
  size_t max_seed_attempts;
  bool unidirectional, rk4, stop_on_all_include, implicit_max_num_seeds;
  curvature_constraint_t curvature_constraint;
  DWI::Tractography::Resampling::Downsampler downsampler;

  // Additional members for ACT
  bool is_act() const { return bool(act_shared_additions); }
  const ACT::ACT_Shared_additions &act() const { return *act_shared_additions; }

  float vox() const {
    return std::pow(source_header.spacing(0) * source_header.spacing(1) * source_header.spacing(2), 1.0F / 3.0F);
  }

  void set_step_and_angle(const float voxel_frac,
                          const float angle,
                          const intrinsic_integration_order_t intrinsic_integration_order,
                          const curvature_constraint_t curvature_constraint_type);
  void set_num_points();
  void set_num_points(const float angle_minradius_preds, const float max_step_postds);
  void set_cutoff(float cutoff);

  // This gets overloaded for iFOD2, as each sample is output rather than just each step, and there are
  //   multiple samples per step
  // (Only utilised for Exec::satisfy_wm_requirement())
  virtual float internal_step_size() const { return step_size; }

  void add_termination(const term_t i) const {
    terminations[static_cast<ssize_t>(i)].fetch_add(1, std::memory_order_relaxed);
  }
  void add_rejection(const reject_t i) const {
    rejections[static_cast<ssize_t>(i)].fetch_add(1, std::memory_order_relaxed);
  }

#ifdef DEBUG_TERMINATIONS
  void add_termination(const term_t i, const Eigen::Vector3f &p) const;
#endif

  size_t termination_count(const term_t i) const {
    return terminations[static_cast<ssize_t>(i)].load(std::memory_order_seq_cst);
  }
  size_t rejection_count(const reject_t i) const {
    return rejections[static_cast<ssize_t>(i)].load(std::memory_order_seq_cst);
  }

  bool termination_relevant(const term_t i) const;
  bool rejection_relevant(const reject_t i) const;

private:
  mutable std::array<std::atomic<size_t>, termination_reason_count> terminations;
  mutable std::array<std::atomic<size_t>, rejection_reason_count> rejections;

  std::unique_ptr<ACT::ACT_Shared_additions> act_shared_additions;

#ifdef DEBUG_TERMINATIONS
  Header debug_header;
  std::vector<std::unique_ptr<Image<uint32_t>>> debug_images;
  const Transform transform;
#endif
};

} // namespace MR::DWI::Tractography::Tracking

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

#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/tractography.h"
#include "dwi/tractography/tracking/types.h"
#include "math/SH.h"

namespace MR::DWI::Tractography::Algorithms {

using namespace MR::DWI::Tractography::Tracking;

class SDStream : public MethodBase {
public:
  class Shared : public SharedBase {
  public:
    Shared(std::string_view diff_path, DWI::Tractography::Properties &property_set)
        : SharedBase(diff_path, property_set), lmax(Math::SH::LforN(source.size(3))) {
      try {
        Math::SH::check(source);
      } catch (Exception &e) {
        e.display();
        throw Exception("Algorithm SD_STREAM expects as input a spherical harmonic (SH) image");
      }

      if (is_act() && act().backtrack())
        throw Exception("Backtracking not valid for deterministic algorithms");

      set_step_and_angle(rk4 ? Defaults::stepsize_voxels_rk4 : Defaults::stepsize_voxels_firstorder,
                         Defaults::angle_deterministic,
                         rk4 ? intrinsic_integration_order_t::HIGHER : intrinsic_integration_order_t::FIRST,
                         curvature_constraint_t::POSTHOC_THRESHOLD);
      dot_threshold = std::cos(max_angle_1o);
      set_num_points();
      set_cutoff(Defaults::cutoff_fod * (is_act() ? Defaults::cutoff_act_multiplier : 1.0));

      properties["method"] = "SDStream";

      bool precomputed = true;
      properties.set(precomputed, "sh_precomputed");
      if (precomputed)
        precomputer = new Math::SH::PrecomputedAL<float>(lmax);
    }

    ~Shared() {
      if (precomputer)
        delete precomputer;
    }

    float dot_threshold;
    size_t lmax;
    Math::SH::PrecomputedAL<float> *precomputer;
  };

  SDStream(const Shared &shared) : MethodBase(shared), S(shared), source(S.source) {}

  SDStream(const SDStream &that) : MethodBase(that.S), S(that.S), source(S.source) {}

  ~SDStream() {}

  bool init() override {
    if (!get_data(source))
      return (false);

    if (!S.init_dir.allFinite()) {
      if (!dir.allFinite())
        dir = random_direction();
    } else
      dir = S.init_dir;

    dir.normalize();
    if (!find_peak())
      return false;

    return true;
  }

  term_t next() override {
    if (!get_data(source))
      return term_t::EXIT_IMAGE;

    const Eigen::Vector3f prev_dir(dir);

    if (!find_peak())
      return term_t::MODEL;

    if (prev_dir.dot(dir) < S.dot_threshold)
      return term_t::HIGH_CURVATURE;

    pos += dir * S.step_size;
    return term_t::CONTINUE;
  }

  float get_metric(const Eigen::Vector3f &position, const Eigen::Vector3f &direction) override {
    if (!get_data(source, position))
      return 0.0;
    return FOD(direction);
  }

protected:
  const Shared &S;
  Interpolator<Image<float>>::type source;

  float find_peak() {
    float FOD = Math::SH::get_peak(values, S.lmax, dir, S.precomputer);
    if (!std::isfinite(FOD) || FOD < S.threshold)
      FOD = 0.0;
    return FOD;
  }

  float FOD(const Eigen::Vector3f &d) const {
    return (S.precomputer ? S.precomputer->value(values, d) : Math::SH::value(values, d, S.lmax));
  }
};

} // namespace MR::DWI::Tractography::Algorithms

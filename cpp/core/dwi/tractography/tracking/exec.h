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

#include "dwi/directions/set.h"
#include "dwi/tractography/rng.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tracking/generated_track.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/write_kernel.h"
#include "thread.h"
#include "thread_queue.h"

#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/voxel.h"

#include "dwi/tractography/seeding/dynamic.h"
#include "dwi/tractography/editing/loader.h"
#include "dwi/tractography/editing/receiver.h"
#include "ordered_thread_queue.h"

namespace MR::DWI::Tractography::Tracking {

using namespace MR::DWI::Tractography::Editing;

constexpr ssize_t failed_seed_attempts_to_abort = 100000;
constexpr ssize_t streamline_generation_batch_size = 10;

struct BacktrackConfig {
  float retrack_fraction;
  float terminal_search_length;
  float min_wm_length;
  float truncation_margin_length;
  float min_sgm_length;
};

// TODO Try having ACT as a template boolean; allow compiler to optimise out branch statements

template <class Method> class Exec {

public:
  static void run(std::string_view diff_path, std::string_view destination, DWI::Tractography::Properties &properties) {

    if (properties.find("seed_dynamic") == properties.end()) {

      typename Method::Shared shared(diff_path, properties);
      WriteKernel writer(shared, destination, properties);
      Exec<Method> tracker(shared);
      Thread::run_queue(
          Thread::multi(tracker), Thread::batch(GeneratedTrack(), streamline_generation_batch_size), writer);

    } else {

      const std::string fod_path(properties["seed_dynamic"]);
      const std::string max_num_tracks = properties["max_num_tracks"];
      if (max_num_tracks.empty())
        throw Exception("Dynamic seeding requires setting the desired number of tracks using the -select option");
      const size_t num_tracks = to<size_t>(max_num_tracks);

      using SetDixel = Mapping::SetDixel;
      using TckMapper = Mapping::TrackMapperBase;
      using Writer = Seeding::WriteKernelDynamic;

      DWI::Directions::FastLookupSet dirs(1281);
      auto fod_data = Image<float>::open(fod_path);
      Math::SH::check(fod_data);
      Seeding::Dynamic *seeder = new Seeding::Dynamic(fod_path, fod_data, num_tracks, dirs);
      properties.seeds.add(seeder); // List is responsible for deleting this from memory

      typename Method::Shared shared(diff_path, properties);

      Writer writer(shared, destination, properties);
      Exec<Method> tracker(shared);

      TckMapper mapper(fod_data, dirs);
      mapper.set_upsample_ratio(Mapping::determine_upsample_ratio(fod_data, properties, 0.25));
      mapper.set_use_precise_mapping(true);

      Thread::run_queue(Thread::multi(tracker),
                        Thread::batch(GeneratedTrack(), streamline_generation_batch_size),
                        writer,
                        Thread::batch(Streamline<>(), streamline_generation_batch_size),
                        Thread::multi(mapper),
                        Thread::batch(SetDixel(), streamline_generation_batch_size),
                        *seeder);
    }
  }

  static void run_backtrack(std::string_view in_fod, std::string_view out_tck, std::string_view in_tck,
                            std::string_view in_5tt, DWI::Tractography::Properties &properties,
                            const BacktrackConfig &config) {
    properties["act"] = std::string(in_5tt);
    properties["backtrack"] = "1";
    const size_t number = to<size_t>(properties["max_num_tracks"]);
    const size_t skip = 0;

    // create a seeder
    DWI::Directions::FastLookupSet dirs(1281);
    auto fod_data = Image<float>::open(in_fod);
    Math::SH::check(fod_data);
    Seeding::Dynamic *seeder = new Seeding::Dynamic(std::string(in_fod), fod_data, number, dirs);
    properties.seeds.add(seeder);

    std::vector<std::string> input_file_list;
    input_file_list.push_back(std::string(in_tck));

    Loader loader(input_file_list);
    Receiver receiver(std::string(out_tck), properties, number, skip);
    typename Method::Shared shared(in_fod, properties);

    Exec<Method> tracker(shared);
    tracker.backtrack_config = config;

    Thread::run_ordered_queue(loader, Thread::batch(Streamline<>()), Thread::multi(tracker),
                               Thread::batch(Streamline<>()), receiver);
  }

  Exec(const typename Method::Shared &shared)
      : S(shared),
        method(shared),
        track_excluded(false),
        include_visitation(S.properties.include, S.properties.ordered_include),
        backtrack_config() {}

  Exec(const Exec &that)
      : S(that.S),
        method(that.method),
        track_excluded(false),
        include_visitation(S.properties.include, S.properties.ordered_include),
        backtrack_config(that.backtrack_config) {}

  bool operator()(GeneratedTrack &item) {
    if (!seed_track(item))
      return false;
    if (track_excluded) {
      item.set_status(GeneratedTrack::status_t::SEED_REJECTED);
      S.add_rejection(reject_t::INVALID_SEED);
      return true;
    }
    gen_track(item);
    if (track_rejected(item)) {
      item.clear();
      item.set_status(GeneratedTrack::status_t::TRACK_REJECTED);
    } else if (S.downsampler.get_ratio() > 1 || (S.is_act() && S.act().crop_at_gmwmi())) {
      S.downsampler(item);
      check_downsampled_length(item);
    } else {
      item.set_status(GeneratedTrack::status_t::ACCEPTED);
    }
    return true;
  }

  bool operator()(const Streamline<> &streamline_in, Streamline<> &streamline_out) {
    streamline_out = streamline_in;
    TissuePattern tissue_pattern = get_tissue_pattern(streamline_in);
    size_t front_truncate = 0, back_truncate = 0;
    find_truncation(tissue_pattern, front_truncate, back_truncate);

    if (streamline_out.size() > (front_truncate + back_truncate) && (front_truncate > 0 || back_truncate > 0)) {
      streamline_out.erase(streamline_out.begin(), streamline_out.begin() + front_truncate);
      streamline_out.resize(streamline_out.size() - back_truncate);
    } else {
      front_truncate = back_truncate = 0;
    }

    if (front_truncate > 0 || back_truncate > 0) {
      float native_length = 0.0f;
      for (size_t i = 1; i < streamline_in.size(); ++i) {
        native_length += (streamline_in[i] - streamline_in[i - 1]).norm();
      }
      const float retrack_length_buffer = native_length * backtrack_config.retrack_fraction;

      // Front tracking
      if (front_truncate > 0) {
        const_cast<typename Method::Shared &>(S).max_num_points_preds =
            front_truncate + (retrack_length_buffer / S.step_size);
        track_excluded = false;
        method.pos = streamline_out.front();
        method.dir = -(streamline_out[1] - streamline_out[0]).normalized();
        if (method.check_seed() && method.init()) {
          GeneratedTrack front_track;
          front_track.clear();
          front_track.push_back(method.pos);
          gen_track_unidir(front_track);

          if (front_track.size() > 1) {
            if (S.downsampler.get_ratio() > 1 || (S.is_act() && S.act().crop_at_gmwmi())) {
              S.downsampler(front_track);
              check_downsampled_length(front_track);
            }
            if (front_track.get_status() == GeneratedTrack::status_t::ACCEPTED) {
              Streamline<> final_track;
              final_track.reserve(front_track.size() + streamline_out.size());
              final_track.insert(final_track.end(), front_track.rbegin(), front_track.rend());
              final_track.insert(final_track.end(), streamline_out.begin(), streamline_out.end());
              streamline_out = std::move(final_track);
            }
          }
        }
      }

      // Back tracking
      if (back_truncate > 0) {
        const_cast<typename Method::Shared &>(S).max_num_points_preds =
            back_truncate + (retrack_length_buffer / S.step_size);
        track_excluded = false;
        method.pos = streamline_out.back();
        method.dir =
            (streamline_out[streamline_out.size() - 1] - streamline_out[streamline_out.size() - 2]).normalized();
        if (method.check_seed() && method.init()) {
          GeneratedTrack back_track;
          back_track.clear();
          back_track.push_back(method.pos);
          gen_track_unidir(back_track);

          if (back_track.size() > 1) {
            if (S.downsampler.get_ratio() > 1 || (S.is_act() && S.act().crop_at_gmwmi())) {
              S.downsampler(back_track);
              check_downsampled_length(back_track);
            }
            if (back_track.get_status() == GeneratedTrack::status_t::ACCEPTED) {
              streamline_out.insert(streamline_out.end(), back_track.begin(), back_track.end());
            }
          }
        }
      }
    }
    return true;
  }

private:
  const typename Method::Shared &S;
  Method method;
  bool track_excluded;
  IncludeROIVisitation include_visitation;
  BacktrackConfig backtrack_config;

  struct TissuePattern {
    std::vector<uint8_t> pattern;
    std::vector<float> lengths;
  };

  TissuePattern get_tissue_pattern(const Streamline<> &tck) {
    constexpr uint8_t WM = 0x1;
    constexpr uint8_t CGM = 0x2;
    constexpr uint8_t CSF = 0x4;
    constexpr uint8_t SGM = 0x8;

    TissuePattern tissue_pattern;
    tissue_pattern.pattern.reserve(tck.size());
    tissue_pattern.lengths.reserve(tck.size());

    float cumulative_length = 0.0f;
    for (size_t i = 0; i < tck.size(); ++i) {
      method.act().fetch_tissue_data(tck[i]);
      const auto &tissues = method.act().tissues();
      uint8_t vox = 0x0;
      if (tissues.get_wm() > 0.5)
        vox |= WM;
      if (tissues.get_cgm() > 0.5)
        vox |= CGM;
      if (tissues.get_csf() > 0.5)
        vox |= CSF;
      if (tissues.get_sgm() > 0.5)
        vox |= SGM;

      tissue_pattern.pattern.push_back(vox);
      if (i > 0) {
        cumulative_length += (tck[i] - tck[i - 1]).norm();
      }
      tissue_pattern.lengths.push_back(cumulative_length);
    }
    return tissue_pattern;
  }

  bool check_wm_in_region(const std::vector<uint8_t> &pattern, size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
      if (pattern[i] & 0x1)
        return true;
    }
    return false;
  }

  bool check_onlywm_in_region(const std::vector<uint8_t> &pattern, size_t start, size_t end) {
    for (size_t i = start; i < end; ++i) {
      if (!(pattern[i] & 0x1))
        return false;
    }
    return true;
  }

  bool check_onlysgm_in_region(const std::vector<uint8_t> &pattern, size_t start, size_t end, bool is_front = true) {
    size_t sgm_count = 0;
    if (is_front) {
      for (size_t i = start; i < end; ++i) {
        if (pattern[i] & 0x8)
          sgm_count++;
        else
          break;
      }
    } else {
      for (size_t i = end; i-- > start;) {
        if (pattern[i] & 0x8)
          sgm_count++;
        else
          break;
      }
    }
    const size_t min_sgm_points = std::max (size_t(1), size_t(std::round (backtrack_config.min_sgm_length / S.step_size)));
    return (sgm_count > min_sgm_points);
  }

  size_t findIndexBeyondThreshold(const std::vector<float> &lengths, float threshold) {
    size_t idx = 0;
    while (idx < lengths.size() && lengths[idx] < threshold) {
      idx++;
    }
    return idx;
  }

  size_t findIndexBeforeThreshold(const std::vector<float> &lengths, float total_length, float threshold) {
    size_t idx = lengths.size() - 1;
    while (idx > 0 && (total_length - lengths[idx - 1]) < threshold) {
      idx--;
    }
    return idx;
  }

  struct WMRegion {
    size_t start;
    size_t length;
  };

  WMRegion findLongestWMRegion(const std::vector<uint8_t> &pattern, float min_length_mm, uint8_t wm_flag) {
    WMRegion result = {0, 0};
    size_t current_start = 0;
    size_t current_length = 0;
    const size_t min_length = std::max (size_t(1), size_t(std::round (min_length_mm / S.step_size)));
    for (size_t i = 0; i < pattern.size(); ++i) {
      if ((pattern[i] & wm_flag)) {
        if (current_length == 0) {
          current_start = i;
        }
        current_length++;
      } else {
        if (current_length > result.length && current_length >= min_length) {
          result.start = current_start;
          result.length = current_length;
        }
        current_length = 0;
      }
    }
    if (current_length > result.length && current_length >= min_length) {
      result.start = current_start;
      result.length = current_length;
    }
    return result;
  }

  size_t processFrontTerminal(const std::vector<uint8_t> &pattern, size_t front_terminal_idx, bool has_wm, bool has_sgm,
                              bool has_onlywm, uint8_t wm_flag, uint8_t sgm_flag, size_t safety_margin) {
    if (has_wm && !has_sgm) {
      if (has_onlywm) {
        return safety_margin;
      } else {
        for (size_t i = front_terminal_idx; i > 0; --i) {
          if ((pattern[i - 1] & wm_flag)) {
            size_t k = i - 1;
            while (k > 0 && (pattern[k - 1] & wm_flag)) {
              k--;
            }
            size_t wm_count = 0;
            while (k < pattern.size() - 1 && wm_count < safety_margin) {
              if (!(pattern[k - 1] & wm_flag))
                break;
              k++;
              wm_count++;
            }
            return k;
          }
        }
      }
    } else if (has_sgm) {
      for (size_t i = 0; i < front_terminal_idx; ++i) {
        if (!(pattern[i] & sgm_flag)) {
          size_t k = i - 1;
          size_t sgm_count = 0;
          while (k > 0 && sgm_count < safety_margin) {
            if (!(pattern[k - 1] & sgm_flag))
              break;
            k--;
            sgm_count++;
          }
          return k;
        }
      }
      return safety_margin;
    }
    return 0;
  }

  size_t processBackTerminal(const std::vector<uint8_t> &pattern, size_t back_terminal_idx, bool has_wm, bool has_sgm,
                             bool has_onlywm, uint8_t wm_flag, uint8_t sgm_flag, size_t safety_margin) {
    if (has_wm && !has_sgm) {
      if (has_onlywm) {
        return safety_margin;
      } else {
        for (size_t i = back_terminal_idx; i < pattern.size(); ++i) {
          if ((pattern[i] & wm_flag)) {
            size_t k = i;
            while (k < pattern.size() - 1 && (pattern[k + 1] & wm_flag)) {
              k++;
            }
            size_t wm_count = 0;
            while (k > 0 && wm_count < safety_margin) {
              if (!(pattern[k - 1] & wm_flag))
                break;
              k--;
              wm_count++;
            }
            return pattern.size() - k - 1;
          }
        }
      }
    } else if (has_sgm) {
      for (size_t i = pattern.size() - 1; i > back_terminal_idx; --i) {
        if (!(pattern[i] & sgm_flag)) {
          size_t k = i + 1;
          size_t sgm_count = 0;
          while (k < pattern.size() && sgm_count < safety_margin) {
            if (!(pattern[k] & sgm_flag))
              break;
            k++;
            sgm_count++;
          }
          return pattern.size() - k;
        }
      }
      return safety_margin;
    }
    return 0;
  }

  void find_truncation(const TissuePattern &tissue_pattern, size_t &front_truncate, size_t &back_truncate) {
    front_truncate = back_truncate = 0;
    const std::vector<uint8_t> &pattern = tissue_pattern.pattern;
    const std::vector<float> &lengths = tissue_pattern.lengths;
    if (pattern.empty() || lengths.empty()) {
      return;
    }
    const float terminal_search_length = backtrack_config.terminal_search_length;
    const float min_wm_length = backtrack_config.min_wm_length;
    const float truncation_margin_length = backtrack_config.truncation_margin_length;
    const size_t truncation_margin = std::max (size_t(1), size_t(std::round (truncation_margin_length / S.step_size)));
    const size_t min_wm_points = std::max (size_t(1), size_t(std::round (min_wm_length / S.step_size)));

    const uint8_t WM_FLAG = 0x1;
    const uint8_t SGM_FLAG = 0x8;

    WMRegion wm_region = findLongestWMRegion(pattern, min_wm_length, WM_FLAG);
    if (wm_region.length < min_wm_points) {
      return;
    }

    size_t front_terminal_idx, back_terminal_idx;
    float total_length = lengths.back();
    if (total_length >= 2 * terminal_search_length) {
      front_terminal_idx = findIndexBeyondThreshold(lengths, terminal_search_length);
      back_terminal_idx = findIndexBeforeThreshold(lengths, total_length, terminal_search_length);
    } else {
      front_terminal_idx = wm_region.start + truncation_margin;
      back_terminal_idx = wm_region.start + wm_region.length - truncation_margin;
    }
    bool front_has_wm = check_wm_in_region(pattern, 0, front_terminal_idx);
    bool back_has_wm = check_wm_in_region(pattern, back_terminal_idx, pattern.size());
    bool front_has_sgm = check_onlysgm_in_region(pattern, 0, front_terminal_idx, true);
    bool back_has_sgm = check_onlysgm_in_region(pattern, back_terminal_idx, pattern.size(), false);
    bool front_has_onlywm = check_onlywm_in_region(pattern, 0, front_terminal_idx);
    bool back_has_onlywm = check_onlywm_in_region(pattern, back_terminal_idx, pattern.size());
    front_truncate = processFrontTerminal(pattern, front_terminal_idx, front_has_wm, front_has_sgm, front_has_onlywm,
                                           WM_FLAG, SGM_FLAG, truncation_margin);
    back_truncate = processBackTerminal(pattern, back_terminal_idx, back_has_wm, back_has_sgm, back_has_onlywm,
                                         WM_FLAG, SGM_FLAG, truncation_margin);
    if (front_truncate + back_truncate >= pattern.size() - 1) {
      front_truncate = back_truncate = 0;
    }
  }

  term_t iterate() {
    const term_t method_term = (S.rk4 ? next_rk4() : method.next());

    if (method_term != term_t::CONTINUE)
      return (S.is_act() && method.act().sgm_depth) ? term_t::TERM_IN_SGM : method_term;

    if (S.is_act()) {
      const term_t structural_term = method.act().check_structural(method.pos);
      if (structural_term != term_t::CONTINUE)
        return structural_term;
    }

    if (!S.properties.mask.empty() && !S.properties.mask.contains(method.pos))
      return term_t::EXIT_MASK;

    if (S.properties.exclude.contains(method.pos))
      return term_t::ENTER_EXCLUDE;

    // If backtracking is not enabled, add streamline to include regions as it is generated
    // If it is enabled, this check can only be performed after the streamline is completed
    if (!(S.is_act() && S.act().backtrack()))
      include_visitation(method.pos);

    if (S.stop_on_all_include && bool(include_visitation))
      return term_t::TRAVERSE_ALL_INCLUDE;

    return term_t::CONTINUE;
  }

  bool seed_track(GeneratedTrack &tck) {
    tck.clear();
    track_excluded = false;
    include_visitation.reset();
    method.dir.setConstant(NaNF);

    if (S.properties.seeds.is_finite()) {

      if (!S.properties.seeds.get_seed(method.pos, method.dir))
        return false;
      if (!method.check_seed() || !method.init()) {
        track_excluded = true;
        tck.set_status(GeneratedTrack::status_t::SEED_REJECTED);
      }
      return true;

    } else {

      for (size_t num_attempts = 0; num_attempts != failed_seed_attempts_to_abort; ++num_attempts) {
        if (S.properties.seeds.get_seed(method.pos, method.dir)) {
          if (!(method.check_seed() && method.init())) {
            track_excluded = true;
            tck.set_status(GeneratedTrack::status_t::SEED_REJECTED);
          }
          return true;
        }
      }
      FAIL("Failed to find suitable seed point after " + str(failed_seed_attempts_to_abort) + " attempts - aborting");
      return false;
    }
  }

  bool gen_track(GeneratedTrack &tck) {
    bool unidirectional = S.unidirectional;
    if (S.is_act() && !unidirectional)
      unidirectional = method.act().seed_is_unidirectional(method.pos, method.dir);

    include_visitation(method.pos);

    const Eigen::Vector3f seed_dir(method.dir);
    tck.push_back(method.pos);

    gen_track_unidir(tck);

    if (!track_excluded && !unidirectional) {
      tck.reverse();
      method.pos = tck.back();
      method.dir = -seed_dir;
      method.reverse_track();
      gen_track_unidir(tck);
    }

    return true;
  }

  void gen_track_unidir(GeneratedTrack &tck) {
    term_t termination = term_t::CONTINUE;

    if (S.is_act() && S.act().backtrack()) {

      size_t revert_step = 1;
      size_t max_size_at_backtrack = tck.size();
      unsigned int revert_count = 0;

      do {
        termination = iterate();
        if (termination_info.at(termination).add_term_to_tck)
          tck.push_back(method.pos);
        if (termination != term_t::CONTINUE) {
          apply_priors(termination);
          if (track_excluded && termination != term_t::ENTER_EXCLUDE) {
            if (tck.size() > max_size_at_backtrack) {
              max_size_at_backtrack = tck.size();
              revert_step = 1;
              revert_count = 1;
            } else {
              if (revert_count++ == ACT::backtrack_attempts) {
                revert_count = 1;
                ++revert_step;
              }
            }
            method.truncate_track(tck, max_size_at_backtrack, revert_step);
            if (method.pos.allFinite()) {
              track_excluded = false;
              termination = term_t::CONTINUE;
            }
          }
        } else if (tck.size() >= S.max_num_points_preds) {
          termination = term_t::LENGTH_EXCEED;
        }
      } while (termination == term_t::CONTINUE);

    } else {

      do {
        termination = iterate();
        if (termination_info.at(termination).add_term_to_tck)
          tck.push_back(method.pos);
        if (termination == term_t::CONTINUE && tck.size() >= S.max_num_points_preds)
          termination = term_t::LENGTH_EXCEED;
      } while (termination == term_t::CONTINUE);
    }

    apply_priors(termination);

    if (termination == term_t::EXIT_SGM) {
      truncate_exit_sgm(tck);
      method.pos = tck.back();
    }

    if (track_excluded) {
      switch (termination) {
      case term_t::CALIBRATOR:
      case term_t::ENTER_CSF:
      case term_t::MODEL:
      case term_t::HIGH_CURVATURE:
        S.add_rejection(reject_t::ACT_POOR_TERMINATION);
        break;
      case term_t::LENGTH_EXCEED:
        S.add_rejection(reject_t::TRACK_TOO_LONG);
        break;
      case term_t::ENTER_EXCLUDE:
        S.add_rejection(reject_t::ENTER_EXCLUDE_REGION);
        break;
      default:
        throw Exception("\nFIXME: Unidirectional track excluded but termination is good!\n");
      }
    }

    if (S.is_act() && (termination == term_t::ENTER_CGM) && S.act().crop_at_gmwmi())
      S.act().crop_at_gmwmi(tck);

#ifdef DEBUG_TERMINATIONS
    S.add_termination(termination, method.pos);
#else
    S.add_termination(termination);
#endif
  }

  void apply_priors(term_t &termination) {

    if (S.is_act()) {

      switch (termination) {

      case term_t::CONTINUE:
        throw Exception("\nFIXME: undefined termination of track in apply_priors()\n");

      case term_t::ENTER_CGM:
      case term_t::EXIT_IMAGE:
      case term_t::EXIT_MASK:
      case term_t::EXIT_SGM:
      case term_t::TERM_IN_SGM:
      case term_t::TRAVERSE_ALL_INCLUDE:
        break;

      case term_t::ENTER_CSF:
      case term_t::LENGTH_EXCEED:
      case term_t::ENTER_EXCLUDE:
        track_excluded = true;
        break;

      case term_t::CALIBRATOR:
      case term_t::MODEL:
      case term_t::HIGH_CURVATURE:
        if (method.act().sgm_depth)
          termination = term_t::TERM_IN_SGM;
        else if (!method.act().in_pathology())
          track_excluded = true;
        break;
      }

    } else {

      switch (termination) {

      case term_t::CONTINUE:
        throw Exception("\nFIXME: undefined termination of track in apply_priors()\n");

      case term_t::ENTER_CGM:
      case term_t::ENTER_CSF:
      case term_t::EXIT_SGM:
      case term_t::TERM_IN_SGM:
        throw Exception("\nFIXME: Have received ACT-based termination for non-ACT tracking in apply_priors()\n");

      case term_t::EXIT_IMAGE:
      case term_t::EXIT_MASK:
      case term_t::LENGTH_EXCEED:
      case term_t::CALIBRATOR:
      case term_t::MODEL:
      case term_t::HIGH_CURVATURE:
      case term_t::TRAVERSE_ALL_INCLUDE:
        break;

      case term_t::ENTER_EXCLUDE:
        track_excluded = true;
        break;
      }
    }
  }

  bool track_rejected(const GeneratedTrack &tck) {

    if (track_excluded)
      return true;

    // seedtest algorithm uses min_num_points_preds = 1; should be 2 or more for all other algorithms
    if (tck.size() == 1 && S.min_num_points_preds > 1) {
      S.add_rejection(reject_t::NO_PROPAGATION_FROM_SEED);
      return true;
    }

    if (tck.size() < S.min_num_points_preds) {
      S.add_rejection(reject_t::TRACK_TOO_SHORT);
      return true;
    }

    if (S.is_act()) {

      if (!satisfy_wm_requirement(tck)) {
        S.add_rejection(reject_t::ACT_FAILED_WM_REQUIREMENT);
        return true;
      }

      if (S.act().backtrack()) {
        for (const auto &i : tck)
          include_visitation(i);
      }
    }

    if (!bool(include_visitation)) {
      S.add_rejection(reject_t::MISSED_INCLUDE_REGION);
      return true;
    }

    return false;
  }

  bool satisfy_wm_requirement(const std::vector<Eigen::Vector3f> &tck) {
    // If using the Seed_test algorithm (indicated by max_num_points == 2), don't want to execute this check
    if (S.max_num_points_preds == 2)
      return true;
    // If the seed was in SGM, need to confirm that one side of the track actually made it to WM
    if (method.act().seed_in_sgm && !method.act().sgm_seed_to_wm)
      return false;
    // Used these in the ACT paper, but wasn't entirely happy with the method;
    //   can change these constexprs to re-enable
    // ACT instead now defaults to a 2-voxel minimum length
    if (ACT::wm_pathintegral_threshold == 0.0F && ACT::wm_maxabs_threshold == 0.0F)
      return true;
    float integral = 0.0, max_value = 0.0;
    for (const auto &i : tck) {
      if (method.act().fetch_tissue_data(i)) {
        const float wm = method.act().tissues().get_wm();
        max_value = std::max(max_value, wm);
        if (((integral += (Math::pow2(wm) * S.internal_step_size())) > ACT::wm_pathintegral_threshold) &&
            (max_value > ACT::wm_maxabs_threshold))
          return true;
      }
    }
    return false;
  }

  void truncate_exit_sgm(std::vector<Eigen::Vector3f> &tck) {
    const size_t sgm_start = tck.size() - method.act().sgm_depth;
    assert(sgm_start >= 0 && sgm_start < tck.size());
    size_t best_termination = tck.size() - 1;
    float min_value = std::numeric_limits<float>::infinity();
    for (size_t i = sgm_start; i != tck.size(); ++i) {
      const Eigen::Vector3f direction = (tck[i] - tck[i - 1]).normalized();
      const float this_value = method.get_metric(tck[i], direction);
      if (this_value < min_value) {
        min_value = this_value;
        best_termination = i;
      }
    }
    tck.erase(tck.begin() + best_termination + 1, tck.end());
  }

  void check_downsampled_length(GeneratedTrack &tck) {
    // Don't quantify the precise streamline length if we don't have to
    //   (i.e. we know for sure that even in the presence of downsampling,
    //   we're not going to break either of these two criteria)
    if (tck.size() > S.min_num_points_postds && tck.size() < S.max_num_points_postds) {
      tck.set_status(GeneratedTrack::status_t::ACCEPTED);
      return;
    }
    const float length = Tractography::length(tck);
    if (length < S.min_dist) {
      tck.clear();
      tck.set_status(GeneratedTrack::status_t::TRACK_REJECTED);
      S.add_rejection(reject_t::TRACK_TOO_SHORT);
    } else if (length > S.max_dist) {
      if (S.is_act()) {
        tck.clear();
        tck.set_status(GeneratedTrack::status_t::TRACK_REJECTED);
        S.add_rejection(reject_t::TRACK_TOO_LONG);
      } else {
        truncate_maxlength(tck);
        tck.set_status(GeneratedTrack::status_t::ACCEPTED);
      }
    } else {
      tck.set_status(GeneratedTrack::status_t::ACCEPTED);
    }
  }

  void truncate_maxlength(GeneratedTrack &tck) {
    // Chop the track short so that the length is precisely the maximum length
    // If the truncation would result in removing the seed point, truncate all
    //   the way back to the seed point, reverse the streamline, and then
    //   truncate off the end (which used to be the start)
    float length_sum = 0.0f;
    size_t index;
    for (index = 1; index != tck.size(); ++index) {
      const float seg_length = (tck[index] - tck[index - 1]).norm();
      if (length_sum + seg_length > S.max_dist)
        break;
      length_sum += seg_length;
    }

    // If we don't exceed the maximum length, this function should never have been called!
    assert(index != tck.size());
    // But nevertheless; if we happen to somehow get here, let's just allow processing to continue
    if (index == tck.size())
      return;

    // Would truncation at this vertex (plus including a new small segment at the end) result in
    //   discarding the seed point? If so:
    //   - Truncate so that the seed point is the last point on the streamline
    //   - Reverse the order of the vertices
    //   - Re-run this function recursively in order to truncate from the opposite end of the streamline
    if (tck.get_seed_index() >= index) {
      tck.resize(tck.get_seed_index() + 1);
      tck.reverse();
      truncate_maxlength(tck);
      return;
    }

    // We want to determine a new vertex in between "index" and the prior vertex, which
    //   is of the appropriate distance away from "index" in order to make the streamline
    //   precisely the maximum length
    tck.resize(index + 1);
    tck[index] = tck[index - 1] + ((tck[index] - tck[index - 1]).normalized() * (S.max_dist - length_sum));
  }

  term_t next_rk4() {
    term_t termination = term_t::CONTINUE;
    const Eigen::Vector3f init_pos(method.pos);
    const Eigen::Vector3f init_dir(method.dir);
    if ((termination = method.next()) != term_t::CONTINUE)
      return termination;
    const Eigen::Vector3f dir_rk1(method.dir);
    method.pos = init_pos + (dir_rk1 * (0.5 * S.step_size));
    method.dir = init_dir;
    if ((termination = method.next()) != term_t::CONTINUE)
      return termination;
    const Eigen::Vector3f dir_rk2(method.dir);
    method.pos = init_pos + (dir_rk2 * (0.5 * S.step_size));
    method.dir = init_dir;
    if ((termination = method.next()) != term_t::CONTINUE)
      return termination;
    const Eigen::Vector3f dir_rk3(method.dir);
    method.pos = init_pos + (dir_rk3 * S.step_size);
    method.dir = (dir_rk2 + dir_rk3).normalized();
    if ((termination = method.next()) != term_t::CONTINUE)
      return termination;
    const Eigen::Vector3f dir_rk4(method.dir);
    method.dir = (dir_rk1 + (dir_rk2 * 2.0) + (dir_rk3 * 2.0) + dir_rk4).normalized();
    method.pos = init_pos + (method.dir * S.step_size);
    const Eigen::Vector3f final_pos(method.pos);
    const Eigen::Vector3f final_dir(method.dir);
    if ((termination = method.next()) != term_t::CONTINUE)
      return termination;
    if (dir_rk1.dot(method.dir) < S.cos_max_angle_ho)
      return term_t::HIGH_CURVATURE;
    method.pos = final_pos;
    method.dir = final_dir;
    return term_t::CONTINUE;
  }
};

} // namespace MR::DWI::Tractography::Tracking

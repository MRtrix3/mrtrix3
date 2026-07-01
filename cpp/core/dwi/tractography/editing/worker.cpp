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

#include "dwi/tractography/editing/worker.h"

#include "eigen_plugins/eigen_plugins.h"
#include <Eigen/Dense>
#include <cassert>
#include <cstddef>
#include <variant>

#include "exception.h"
#include "match_variant.h"

namespace MR::DWI::Tractography::Editing {

namespace {

//! \brief contiguous span of original vertices kept by one crop fragment.
struct FragmentRange {
  size_t start;
  size_t length;
};

//! \brief slice each dpv field to a fragment's contiguous vertex row-block.
/*! For a fragment spanning original vertex indices [range.start, range.start +
 * range.length), every input dpv field is sliced as field.middleRows(start,
 * length) (preserving the VectorOrMatrix<T> variant alternative). Each sliced
 * field's row count must equal the fragment vertex count. */
std::vector<DPVValue> slice_dpv(const std::vector<DPVValue> &in_dpv, const FragmentRange &range) {
  std::vector<DPVValue> out_dpv;
  out_dpv.reserve(in_dpv.size());
  for (const auto &field : in_dpv) {
    DPVValue sliced = MR::match_v(field, [&range](const auto &matrix) -> DPVValue {
      using FieldType = std::decay_t<decltype(matrix)>;
      FieldType block(matrix.middleRows(range.start, range.length));
      return DPVValue(std::move(block));
    });
    // Sanity check: sliced rows must match the fragment vertex count.
    const auto rows = MR::match_v(sliced, [](const auto &matrix) -> Eigen::Index { return matrix.rows(); });
    assert(static_cast<size_t>(rows) == range.length);
    (void)rows;
    out_dpv.push_back(std::move(sliced));
  }
  return out_dpv;
}

} // namespace

bool Worker::dps_filters_pass(const std::vector<DPSValue> &dps) const {
  for (const auto &filter : dps_filters) {
    assert(filter.ordinal < dps.size());
    const float value = dps_scalar_to_float(dps[filter.ordinal]);
    const bool ok = (filter.bound == Bound::Min) ? (value >= filter.value) : (value <= filter.value);
    if (!ok)
      return false;
  }
  return true;
}

bool Worker::dpv_filters_pass(const std::vector<DPVValue> &dpv, const size_t row) const {
  for (const auto &filter : dpv_filters) {
    assert(filter.ordinal < dpv.size());
    const float value = dpv_scalar_to_float(dpv[filter.ordinal], row);
    const bool ok = (filter.bound == Bound::Min) ? (value >= filter.value) : (value <= filter.value);
    if (!ok)
      return false;
  }
  return true;
}

bool Worker::keep(const TractogramItem<> &item) const {

  const Streamline<> &in = item.streamline;

  // Need to track exclusion separately, since we may still need to apply mask
  //   (or, more accurately, their inverse) afterwards if -inverse is specified.
  // A per-streamline (dps) field threshold is a whole-streamline criterion,
  //   applied here alongside the length / weight thresholds.
  bool exclude = !thresholds(in) || !dps_filters_pass(item.dps);

  if (!exclude) {
    // If no thresholds are specified, and no include / exclude ROIs are defined, then
    //   it's still possible that one or more masks have been provided;
    //   if this is the case, then we want to continue processing this streamline,
    //   regardless of whether or not -inverse has been specified
    if (include_visitation.empty() && properties.exclude.empty() && inverse) {

      exclude = true;

    } else if (!include_visitation.empty() || !properties.exclude.empty()) {

      // Assign to ROIs
      include_visitation.reset();

      if (ends_only) {
        for (size_t i = 0; i != 2; ++i) {
          const Eigen::Vector3f &p(i ? in.back() : in.front());
          include_visitation(p);
          if (properties.exclude.contains(p)) {
            exclude = true;
            break;
          }
        }
      } else {
        for (const auto &p : in) {
          include_visitation(p);
          if (properties.exclude.contains(p)) {
            exclude = true;
            break;
          }
        }
      }

      // Make sure all of the include regions were visited
      if (!include_visitation)
        exclude = true;
    }
  }

  // In default usage, the track is kept if it is not excluded.
  // If inverse selection is sought, the track is kept if it did not fail any criteria.
  return (exclude == inverse);
}

bool Worker::operator()(TractogramItem<> &in, TractogramItem<> &out) const {

  out.clear();

  // No-mask path: filtering only. Excluded streamlines yield an empty item.
  if (!keep(in))
    return true;

  out = std::move(in);
  return true;
}

bool Worker::operator()(TractogramItem<> &in, std::vector<TractogramItem<>> &out) const {

  out.clear();

  if (!keep(in))
    return true;

  // Split the streamline into contiguous retained fragments, recording each kept
  //   fragment's vertex span so dpv rows can be sliced in lockstep. A vertex is
  //   retained iff it lies within the mask (when one is provided) AND satisfies
  //   every per-vertex (dpv) field threshold; either criterion alone can fragment
  //   one input streamline into several outputs.
  const bool have_mask = !properties.mask.empty();
  std::vector<FragmentRange> fragments;
  size_t run_start = 0;
  size_t run_length = 0;
  for (size_t i = 0; i != in.streamline.size(); ++i) {
    const bool in_mask = !have_mask || properties.mask.contains(in.streamline[i]);
    const bool retain = in_mask && dpv_filters_pass(in.dpv, i);
    // "Inverse" applies to per-vertex retention in addition to selection criteria
    if (retain == inverse) {
      if (run_length >= 2)
        fragments.push_back({run_start, run_length});
      run_length = 0;
    } else {
      if (run_length == 0)
        run_start = i;
      ++run_length;
    }
  }
  if (run_length >= 2)
    fragments.push_back({run_start, run_length});

  out.reserve(fragments.size());
  for (const auto &range : fragments) {
    TractogramItem<> fragment;
    fragment.streamline.resize(range.length);
    for (size_t i = 0; i != range.length; ++i)
      fragment.streamline[i] = in.streamline[range.start + i];
    // dps, weight and index are shared verbatim across all fragments of one input.
    fragment.streamline.weight = in.streamline.weight;
    fragment.set_index(in.get_index());
    fragment.dps = in.dps;
    fragment.groups = in.groups;
    // dpv is split per fragment, mirroring the vertex split.
    fragment.dpv = slice_dpv(in.dpv, range);
    out.push_back(std::move(fragment));
  }

  return true;
}

Worker::Thresholds::Thresholds(Tractography::Properties &properties)
    : max_length(std::numeric_limits<float>::infinity()),
      min_length(0.0F),
      max_weight(std::numeric_limits<float>::infinity()),
      min_weight(0.0F),
      step_size(properties.get_stepsize()) {
  if (properties.find("max_dist") != properties.end()) {
    try {
      max_length = to<float>(properties["max_dist"]);
    } catch (Exception &) {
      WARN("Ignoring corrupt key-value \"max_dist\"");
    }
  }
  if (properties.find("min_dist") != properties.end()) {
    try {
      min_length = to<float>(properties["min_dist"]);
    } catch (Exception &) {
      WARN("Ignoring corrupt key-value \"min_dist\"");
    }
  }

  if (std::isfinite(step_size)) {
    // User may set these values to a precise value, which may then fail due to floating-point
    //   calculation of streamline length
    // Therefore throw a bit of error margin in here
    float error_margin = 0.1;
    if (properties.find("downsample_factor") != properties.end())
      error_margin = 0.5 / to<float>(properties["downsample_factor"]);
    max_length += error_margin * step_size;
    min_length -= error_margin * step_size;
  }

  if (properties.find("max_weight") != properties.end())
    max_weight = to<float>(properties["max_weight"]);

  if (properties.find("min_weight") != properties.end())
    min_weight = to<float>(properties["min_weight"]);
}

Worker::Thresholds::Thresholds(const Worker::Thresholds &that)
    : max_length(that.max_length),
      min_length(that.min_length),
      max_weight(that.max_weight),
      min_weight(that.min_weight),
      step_size(that.step_size) {}

bool Worker::Thresholds::operator()(const Streamline<> &in) const {
  const float length = Tractography::length(in);
  return ((length <= max_length) && (length >= min_length) && (in.weight <= max_weight) && (in.weight >= min_weight));
}

} // namespace MR::DWI::Tractography::Editing

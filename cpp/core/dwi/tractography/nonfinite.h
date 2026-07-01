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
#include <cstddef>
#include <limits>
#include <type_traits>
#include <variant>
#include <vector>

#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram_item.h"
#include "exception.h"
#include "match_variant.h"

//! Shared utilities for handling non-finite (NaN / infinite) tractography data.
/*! These helpers underpin the non-finite tolerance broadcast by each format
 * handler (Formats::Capabilities, §2.6): the writer backends use them to enforce
 * their advertised tolerance (throwing on detection of data they cannot
 * represent), and the producing commands use them to poll the selected handler
 * and warn / throw before committing such data. */
namespace MR::DWI::Tractography {

//! \brief whether a single value is acceptable under a non-finite tolerance.
/*! A finite value is always acceptable; a NaN is acceptable unless the tolerance
 * is Forbidden; an infinity is acceptable only under NonFinite::Any (so no
 * vertex channel, whose tolerance is never Any, ever accepts an infinity). */
inline bool nonfinite_permitted(const Formats::NonFinite tolerance, const double value) {
  if (std::isfinite(value))
    return true;
  if (std::isnan(value))
    return tolerance != Formats::NonFinite::Forbidden;
  return tolerance == Formats::NonFinite::Any; // infinity
}

//! \brief What non-finite content a scan found in a data channel.
struct NonFiniteContent {
  bool nan = false;
  bool inf = false;
  //! \brief whether any non-finite value was found
  bool any() const { return nan || inf; }
};

//! \brief whether scanned content is permitted under a non-finite tolerance.
/*! The aggregate companion to the single-value nonfinite_permitted(): used by a
 * command to poll a format's tolerance against the content it is about to write. */
inline bool nonfinite_permitted(const Formats::NonFinite tolerance, const NonFiniteContent &content) {
  if (content.inf && tolerance != Formats::NonFinite::Any)
    return false;
  if (content.nan && tolerance == Formats::NonFinite::Forbidden)
    return false;
  return true;
}

//! \brief scan a streamline's vertex coordinates for non-finite values.
template <class ValueType> NonFiniteContent scan_vertices(const Streamline<ValueType> &tck) {
  NonFiniteContent content;
  for (const auto &vertex : tck) {
    for (Eigen::Index axis = 0; axis != 3; ++axis) {
      const ValueType value = vertex[axis];
      if (std::isnan(value))
        content.nan = true;
      else if (std::isinf(value))
        content.inf = true;
    }
  }
  return content;
}

//! \brief scan a per-vertex track scalar (.tsf) sequence for non-finite values.
template <class ValueType> NonFiniteContent scan_scalars(const TrackScalar<ValueType> &scalars) {
  NonFiniteContent content;
  for (const ValueType value : scalars) {
    if (std::isnan(value))
      content.nan = true;
    else if (std::isinf(value))
      content.inf = true;
  }
  return content;
}

//! \brief throw if any of \a tck's vertices violate the tolerance \a tolerance.
/*! The writer-backend backstop (§2.6): a handler intolerant of the data it is
 * being asked to write rejects it here rather than corrupting the on-disk
 * stream. The caller (the writer) is expected to chain this exception with the
 * format description for a hierarchical, user-interpretable message. */
template <class ValueType> void enforce_vertices(const Streamline<ValueType> &tck, const Formats::NonFinite tolerance) {
  const NonFiniteContent content = scan_vertices(tck);
  if (!content.any())
    return;
  const bool nan_violation = content.nan && !nonfinite_permitted(tolerance, std::nan(""));
  const bool inf_violation = content.inf && !nonfinite_permitted(tolerance, std::numeric_limits<double>::infinity());
  if (!nan_violation && !inf_violation)
    return;
  const std::string kind = inf_violation ? "infinite" : "NaN";
  throw Exception("streamline " + str(tck.get_index()) + " contains " + kind + " vertex coordinate data");
}

//! \brief throw if any of \a scalars violate the tolerance \a tolerance.
/*! The per-vertex (.tsf) analogue of enforce_vertices(). */
template <class ValueType>
void enforce_scalars(const TrackScalar<ValueType> &scalars, const Formats::NonFinite tolerance) {
  const NonFiniteContent content = scan_scalars(scalars);
  if (!content.any())
    return;
  const bool nan_violation = content.nan && !nonfinite_permitted(tolerance, std::nan(""));
  const bool inf_violation = content.inf && !nonfinite_permitted(tolerance, std::numeric_limits<double>::infinity());
  if (!nan_violation && !inf_violation)
    return;
  const std::string kind = inf_violation ? "infinite" : "NaN";
  throw Exception("streamline " + str(scalars.get_index()) + " carries " + kind + " per-vertex scalar data");
}

//! \brief retain only the listed vertex rows of every per-vertex (dpv) field.
/*! When a command culls vertices from a streamline (e.g. tcktransform dropping
 * vertices that the output format cannot represent), each per-vertex sidecar
 * field must be sub-sampled to exactly the retained vertices to stay aligned.
 * \a kept lists the original vertex ordinals to retain, in output order. Each
 * dpv field is an n_vertices×M matrix (sidecar_value.h); this rebuilds it with
 * the kept rows for every variant element type. Per-streamline (dps) fields are
 * unaffected by vertex culling and are left untouched. */
template <class ValueType> void select_dpv_vertices(TractogramItem<ValueType> &item, const std::vector<size_t> &kept) {
  for (DPVValue &field : item.dpv) {
    MR::match_v(field, [&kept](auto &matrix) {
      using Wrapper = std::decay_t<decltype(matrix)>;
      using Element = typename Wrapper::element_type;
      const Eigen::Index cols = matrix.cols();
      Eigen::Matrix<Element, Eigen::Dynamic, Eigen::Dynamic> selected(static_cast<Eigen::Index>(kept.size()), cols);
      for (size_t row = 0; row != kept.size(); ++row)
        selected.row(static_cast<Eigen::Index>(row)) = matrix.row(static_cast<Eigen::Index>(kept[row]));
      matrix = selected;
    });
  }
}

} // namespace MR::DWI::Tractography

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

#pragma once

#include "image.h"
#include "interp/linear.h"
#include "interp/masked.h"

namespace MR::DWI::Tractography::Tracking {

enum class term_t {
  CONTINUE,
  ENTER_CGM,
  CALIBRATOR,
  EXIT_IMAGE,
  ENTER_CSF,
  MODEL,
  HIGH_CURVATURE,
  LENGTH_EXCEED,
  TERM_IN_SGM,
  EXIT_SGM,
  EXIT_MASK,
  ENTER_EXCLUDE,
  TRAVERSE_ALL_INCLUDE
};
struct term_info {
  std::string name;
  std::string description;
  bool add_term_to_tck;
};
const std::map<term_t, term_info> termination_info{
    {term_t::CONTINUE, {"continue", "Continue", true}},
    {term_t::ENTER_CGM, {"enter_cgm", "Entered cortical grey matter", true}},
    {term_t::CALIBRATOR, {"calibrator", "Calibrator sub-threshold", false}},
    {term_t::EXIT_IMAGE, {"exit_image", "Exited image", false}},
    {term_t::ENTER_CSF, {"enter_csf", "Entered CSF", true}},
    {term_t::MODEL, {"model", "Diffusion model sub-threshold", false}},
    {term_t::HIGH_CURVATURE, {"curvature", "Excessive curvature", false}},
    {term_t::LENGTH_EXCEED, {"max_length", "Max length exceeded", true}},
    {term_t::TERM_IN_SGM, {"term_in_sgm", "Terminated in subcortex", false}},
    {term_t::EXIT_SGM, {"exit_sgm", "Exiting sub-cortical GM", false}},
    {term_t::EXIT_MASK, {"exit_mask", "Exited mask", false}},
    {term_t::ENTER_EXCLUDE, {"enter_exclude", "Entered exclusion region", true}},
    {term_t::TRAVERSE_ALL_INCLUDE, {"all_include", "Traversed all include regions", true}}};
constexpr ssize_t termination_type_count = 13;

enum class reject_t {
  INVALID_SEED,
  NO_PROPAGATION_FROM_SEED,
  TRACK_TOO_SHORT,
  TRACK_TOO_LONG,
  ENTER_EXCLUDE_REGION,
  MISSED_INCLUDE_REGION,
  ACT_POOR_TERMINATION,
  ACT_FAILED_WM_REQUIREMENT
};
const std::map<reject_t, std::string> rejection_strings{
    {reject_t::INVALID_SEED, "Invalid seed point"},
    {reject_t::NO_PROPAGATION_FROM_SEED, "No propagation from seed"},
    {reject_t::TRACK_TOO_SHORT, "Shorter than minimum length"},
    {reject_t::TRACK_TOO_LONG, "Longer than maximum length"},
    {reject_t::ENTER_EXCLUDE_REGION, "Entered exclusion region"},
    {reject_t::MISSED_INCLUDE_REGION, "Missed inclusion region"},
    {reject_t::ACT_POOR_TERMINATION, "Poor structural termination"},
    {reject_t::ACT_FAILED_WM_REQUIREMENT, "Failed to traverse white matter"}};
constexpr ssize_t rejection_type_count = 8;

enum class intrinsic_integration_order_t { FIRST, HIGHER };

enum class curvature_constraint_t { LIMITED_SEARCH, POSTHOC_THRESHOLD };

template <class ImageType> class Interpolator {
public:
  using type = Interp::Masked<Interp::Linear<ImageType>>;
};

} // namespace MR::DWI::Tractography::Tracking

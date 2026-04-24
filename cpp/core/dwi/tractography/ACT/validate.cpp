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

#include "dwi/tractography/ACT/validate.h"

#include <cmath>
#include <string>

#include "algo/loop.h"
#include "app.h"
#include "dwi/tractography/ACT/act.h"
#include "exception.h"
#include "header.h"
#include "image.h"
#include "mrtrix.h"

namespace MR::DWI::Tractography::ACT {

void validate_5TT_header(const Header &H) {
  try {
    if (!H.datatype().is_floating_point())
      throw Exception("Not of floating-point datatype");
    if (H.ndim() != 4 || H.size(3) != 5)
      throw Exception("Should be 4D image with 5 volumes");
    try {
      check_3D_nonunity(H);
    } catch (Exception &e) {
      throw Exception(e, "Needs to be non-unity in all three spatial dimensions");
    }
  } catch (Exception &e) {
    throw Exception(e, "Image " + std::string(H.name()) + " is not a valid ACT 5TT image");
  }
}

const FiveTTValidation validate_5TT_image(const Image<float> &in) {
  // ---------------------------------------------------------------
  // Check 1: structural validity.
  // Delegates to the existing lightweight check:
  // 4-dimensional, exactly 5 volumes.  Throws on failure.
  // Note that check for floating-point datatype will always be satisfied here
  // as MR::Image<> class has already been constructed.
  // ---------------------------------------------------------------
  validate_5TT_header(in);

  // ---------------------------------------------------------------
  // Checks 2 & 3: per-voxel content validation.
  //
  // For every brain voxel (identified by a non-zero partial-volume sum):
  //   - Each individual tissue PVF must lie within [0.0, 1.0].
  //   - The sum of all five tissue PVFs must equal 1.0 within
  //     max_sum_deviation.
  //
  // Background voxels (sum == 0) are silently skipped.
  // ---------------------------------------------------------------
  Image<float> img(in);

  FiveTTValidation result;

  for (auto outer = Loop("Validating 5TT image", img, 0, 3)(img); outer; ++outer) {
    default_type sum = 0.0;
    bool abs_error = false;
    for (auto inner = Loop(3)(img); inner; ++inner) {
      const float v = img.value();
      sum += v;
      if (v < 0.0F || v > 1.0F)
        abs_error = true;
    }
    if (sum == 0.0)
      continue; // background voxel — skip

    if (std::fabs(sum - 1.0) > max_sum_deviation)
      ++result.n_voxels_sum_error;
    if (abs_error)
      ++result.n_voxels_abs_error;
  }

  return result;
}

void debug_validate_5TT_image(const Image<float> &in) {
  try {
    validate_5TT_header(in);
  } catch (const Exception &e) {
    throw Exception(e, "5TT image \"" + in.name() + "\" validation failed");
  }
  if (App::log_level < 3)
    return;
  const FiveTTValidation v = validate_5TT_image(in);
  if (v.n_voxels_abs_error == size_t(0) && v.n_voxels_sum_error == size_t(0)) {
    DEBUG("5TT image \"" + in.name() + "\": validation OK");
    return;
  }
  if (v.n_voxels_abs_error > size_t(0))
    WARN("5TT image \"" + in.name() + "\": " + str(v.n_voxels_abs_error) +
         " brain voxel(s) with a non-physical partial volume fraction");
  if (v.n_voxels_sum_error > size_t(0))
    WARN("5TT image \"" + in.name() + "\": " + str(v.n_voxels_sum_error) +
         " brain voxel(s) with a non-unity partial volume sum");
}

} // namespace MR::DWI::Tractography::ACT

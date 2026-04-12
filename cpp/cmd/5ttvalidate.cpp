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

#include "app.h"
#include "command.h"
#include "datatype.h"
#include "image.h"
#include "image_helpers.h"
#include "mrtrix.h"

#include "algo/copy.h"
#include "algo/loop.h"
#include "file/path.h"
#include "formats/list.h"

#include "dwi/tractography/ACT/validate.h"

using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography::ACT;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Validate that one or more images conform to the expected"
             " ACT five-tissue-type (5TT) format";

  DESCRIPTION
  + "A 5TT image encodes the partial volume fractions of five tissue types"
    " in every voxel: cortical grey matter, sub-cortical grey matter,"
    " white matter, CSF, and pathological tissue."
    " Each tissue partial volume fraction (PVF) must be a value in [0.0, 1.0],"
    " and for any brain voxel the five PVFs must sum to 1.0."

  + "The following checks are performed on each input image:"

  + "1. The image must be of floating-point type,"
    " 4-dimensional, and contain exactly 5 volumes."
    " Failure causes the image to be immediately rejected as structurally invalid."

  + "2. For every brain voxel (identified by a non-zero partial-volume sum),"
    " each of the five tissue PVFs must lie within [0.0, 1.0]."
    " Voxels that violate this constraint contain non-physical values"
    " and the image is rejected as a hard error."

  + "3. For every brain voxel, the sum of all five tissue PVFs must equal 1.0"
    " to within a tolerance of " + str(max_sum_deviation) + "."
    " Voxels that violate this constraint are reported as a soft warning:"
    " the image may still be usable for ACT but does not perfectly conform"
    " to the format.";

  ARGUMENTS
  + Argument ("input", "the 5TT image(s) to be validated").type_image_in().allow_multiple();

  OPTIONS
  + Option ("voxels", "output mask images highlighting voxels"
                      " where the input does not conform to 5TT requirements")
    + Argument ("prefix").type_text();
}
// clang-format on

void run() {
  const std::string voxels_prefix = get_option_value<std::string>("voxels", "");

  size_t major_error_count = 0;
  size_t minor_error_count = 0;

  for (size_t i = 0; i != argument.size(); ++i) {

    // ---------------------------------------------------------------
    // Phase 1: validate the header (structural check).
    // Only structural violations throw — content violations are returned.
    // ---------------------------------------------------------------
    Header H = Header::open(argument[i]);
    validate_5TT_header(H);

    // ---------------------------------------------------------------
    // Phase 2: validate the image (content scan).
    // Only structural violations throw — content violations are returned.
    // ---------------------------------------------------------------
    auto in = H.get_image<float>();
    FiveTTValidation result;
    try {
      result = validate_5TT_image(in);
    } catch (Exception &e) {
      e.display();
      WARN("Image \"" + std::string(argument[i]) + "\" does not conform to fundamental 5TT format requirements");
      ++major_error_count;
      continue;
    }

    // ---------------------------------------------------------------
    // Phase 3: optionally produce a voxel-error mask.
    // A second pass is performed only when -voxels is requested and at
    // least one content violation was detected in phase 1.
    // ---------------------------------------------------------------
    if (!voxels_prefix.empty() && (result.n_voxels_abs_error > 0 || result.n_voxels_sum_error > 0)) {
      Header H_out(in);
      H_out.ndim() = 3;
      H_out.datatype() = DataType::Bit;
      H_out.name() = voxels_prefix;
      if (argument.size() > 1) {
        H_out.name() += Path::basename(argument[i]);
      } else {
        bool has_extension = false;
        for (const auto &p : MR::Formats::known_extensions) {
          if (Path::has_suffix(H_out.name(), p)) {
            has_extension = true;
            break;
          }
        }
        if (!has_extension)
          H_out.name() += ".mif";
      }
      auto voxels = Image<bool>::create(H_out.name(), H_out);

      for (auto outer = Loop(in, 0, 3)(in); outer; ++outer) {
        default_type sum = 0.0;
        bool abs_error = false;
        for (auto inner = Loop(3)(in); inner; ++inner) {
          const float v = in.value();
          sum += v;
          if (v < 0.0F || v > 1.0F)
            abs_error = true;
        }
        if (sum == 0.0)
          continue;
        const bool voxel_error = abs_error || (std::fabs(sum - 1.0) > max_sum_deviation);
        if (voxel_error) {
          assign_pos_of(in, 0, 3).to(voxels);
          voxels.value() = true;
        }
      }
    }

    // ---------------------------------------------------------------
    // Phase 4: report findings and accumulate error counts.
    // ---------------------------------------------------------------
    if (result.n_voxels_sum_error > 1) {
      WARN("Image \"" + std::string(argument[i]) + "\" contains " + //
           (result.n_voxels_sum_error > 1                           //
                ? str(result.n_voxels_sum_error) + " brain voxels"  //
                : "one isolated voxel") +                           //
           " with non-unity sum of partial volume fractions");      //
    }
    if (result.n_voxels_abs_error == 0 && result.n_voxels_sum_error == 0) {
      INFO("Image \"" + std::string(argument[i]) + "\" conforms to 5TT format");
    }
    if (result.n_voxels_sum_error > 0 && result.n_voxels_abs_error == 0)
      ++minor_error_count;
    if (result.n_voxels_abs_error > 0) {
      WARN("Image \"" + std::string(argument[i]) + "\" contains " + str(result.n_voxels_abs_error) +
           " brain voxels with a non-physical partial volume fraction");
      ++major_error_count;
    }
  }

  const std::string vox_option_suggestion =
      voxels_prefix.empty() ? " (suggest re-running using the -voxels option"
                              " to see voxels with non-conformant tissue fractions)"
                            : (" (suggest checking " +
                               std::string(major_error_count > 1 ? "outputs from" : "output of") + " -voxels option)");

  if (major_error_count) {
    throw Exception((argument.size() > 1
                         ? (str(major_error_count) + " input image" + (major_error_count > 1 ? "s do" : " does"))
                         : "Input image does") +
                    " not conform to 5TT format");
  } else if (minor_error_count > size_t(0)) {
    WARN((argument.size() > 1
              ? (str(minor_error_count) + " input image" + (minor_error_count > 1 ? "s do" : " does")) //
              : "Input image does") +                                                                  //
         " not perfectly conform to 5TT format, but may still be applicable" +                         //
         vox_option_suggestion);
  } else {
    CONSOLE(std::string(argument.size() > 1 ? "All images" : "Input image") + " checked OK");
  }
}

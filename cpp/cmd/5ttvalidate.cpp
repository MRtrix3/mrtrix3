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

#include <filesystem>
#include <fmt/std.h>
#include <optional>

#include "app.h"
#include "command.h"
#include "datatype.h"
#include "file/path.h"
#include "formats/list.h"
#include "image.h"
#include "image_helpers.h"
#include "mrtrix.h"

#include "algo/copy.h"
#include "algo/loop.h"

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

  + fmt::format("3. For every brain voxel, the sum of all five tissue PVFs must equal 1.0"
               " to within a tolerance of {}."
               " Voxels that violate this constraint are reported as a soft warning:"
               " the image may still be usable for ACT but does not perfectly conform"
               " to the format.", max_sum_deviation)

  + "The path to be provided to the -voxels option"
    " depends on the image(s) provided as input to the command."
    " if ony a single input image is provided,"
    " then the path provided to the -voxels option"
    " should be the path to an output image file that will be created if necessary."
    " If however there are multiple input files provided to the command,"
    " then the path provided to the -voxels option"
    " should instead be a path to a directory that will be created"
    " and populated with an individual image per problematic input image.";

  ARGUMENTS
  + Argument ("input", "the 5TT image(s) to be validated").type_image_in().allow_multiple();

  OPTIONS
  + Option ("voxels", "output path for mask image(s) highlighting voxels"
                      " where the input(s) violate 5TT requirements"
                      " (see Description)")
    + Argument ("image_or_dir").type_image_out().type_directory_out(DirOutMode::MayExist);
}
// clang-format on

void run() {
  const auto opt_voxels = get_options("voxels");
  const bool single_input = (argument.size() == 1);
  std::optional<std::filesystem::path> voxels_path;

  if (!opt_voxels.empty()) {
    voxels_path.emplace(opt_voxels[0][0]);
    if (single_input) {
      if (!Path::has_suffix(voxels_path.value(), MR::Formats::known_extensions))
        WARN("\"-voxels\" argument \"{}\""
             " does not have a recognised image file extension;"
             " with a single input image, \"-voxels\" expects an output image path",
             voxels_path.value());
      check_overwrite(voxels_path.value());
    } else {
      if (Path::has_suffix(voxels_path.value(), MR::Formats::known_extensions))
        WARN("\"-voxels\" argument \"{}\""
             " has a recognised image file extension;"
             " with multiple input images, \"-voxels\" expects an output directory path",
             voxels_path.value());
      check_overwrite(voxels_path.value());
      std::filesystem::create_directories(voxels_path.value());
    }
  }

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
      WARN("Image \"{}\" does not conform to fundamental 5TT format requirements", argument[i]);
      ++major_error_count;
      continue;
    }

    // ---------------------------------------------------------------
    // Phase 3: optionally produce a voxel-error mask.
    // A second pass is performed only when -voxels is requested and at
    // least one content violation was detected in phase 1.
    // ---------------------------------------------------------------
    if (voxels_path.has_value() && (result.n_voxels_abs_error > 0 || result.n_voxels_sum_error > 0)) {
      Header H_out(in);
      H_out.ndim() = 3;
      H_out.datatype() = DataType::Bit;
      const std::filesystem::path voxels_out =
          single_input ? voxels_path.value()
                       : voxels_path.value() / static_cast<std::filesystem::path>(argument[i]).filename();
      auto voxels = Image<bool>::create(voxels_out, H_out);

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
      WARN("Image \"{}\" contains {}{}",
           argument[i],                                                     //
           (result.n_voxels_sum_error > 1                                   //
                ? fmt::format("{} brain voxels", result.n_voxels_sum_error) //
                : "one isolated voxel"),                                    //
           " with non-unity sum of partial volume fractions");              //
    }
    if (result.n_voxels_abs_error == 0 && result.n_voxels_sum_error == 0) {
      INFO("Image \"{}\" conforms to 5TT format", argument[i]);
    }
    if (result.n_voxels_sum_error > 0 && result.n_voxels_abs_error == 0)
      ++minor_error_count;
    if (result.n_voxels_abs_error > 0) {
      WARN("Image \"{}\" contains {} brain voxels with a non-physical partial volume fraction",
           argument[i],
           result.n_voxels_abs_error);
      ++major_error_count;
    }
  }

  const std::string vox_option_suggestion =
      voxels_path.has_value()
          ? fmt::format(" (suggest checking {} -voxels option)", major_error_count > 1 ? "outputs from" : "output of")
          : " (suggest re-running using the -voxels option"
            " to see voxels with non-conformant tissue fractions)";

  if (major_error_count) {
    const std::string subject =
        argument.size() > 1 ? fmt::format("{} input image{}", major_error_count, major_error_count > 1 ? "s" : "")
                            : "Input image";
    throw Exception("{} {} not conform to 5TT format",
                    subject,
                    argument.size() > 1 ? (major_error_count > 1 ? "do" : "does") : "does");
  } else if (minor_error_count > size_t(0)) {
    const std::string subject =
        argument.size() > 1 ? fmt::format("{} input image{}", minor_error_count, minor_error_count > 1 ? "s" : "")
                            : "Input image";
    WARN("{} {} not perfectly conform to 5TT format, but may still be applicable{}",
         subject,
         argument.size() > 1 ? (minor_error_count > 1 ? "do" : "does") : "does",
         vox_option_suggestion);
  } else {
    CONSOLE(argument.size() > 1 ? "All images checked OK" : "Input image checked OK");
  }
}

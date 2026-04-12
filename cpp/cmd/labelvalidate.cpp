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

#include "command.h"
#include "header.h"
#include "image.h"
#include "mrtrix.h"

#include "connectome/connectome.h"
#include "connectome/validate.h"

using namespace MR;
using namespace App;
using MR::Connectome::node_t;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Validate a hard segmentation (label) image";

  DESCRIPTION
  + "This command checks that an image conforms to the requirements"
    " of a hard segmentation image."
    " Specifically, it verifies that the image is 3-dimensional"
    " (or 4-dimensional with a singleton 4th axis),"
    " and contains only non-negative integer values"
    " (either stored as an unsigned integer datatype,"
    " or as a floating-point or signed integer datatype"
    " with values that satisfy these constraints)."

  + "The command then reports two further properties of the image."
    " First, whether the label indices are contiguous:"
    " i.e. whether all integer values in the range"
    " [1, max_label] are present in the image."
    " Second, for each unique non-background label,"
    " whether the set of voxels carrying that label"
    " forms a single spatially connected region"
    " or comprises multiple disconnected components,"
    " assessed using 26-nearest-neighbour voxel connectivity."

  + "Label index 0 is treated as background and excluded from all reports.";

  ARGUMENTS
  + Argument ("image", "the input label image").type_image_in();
}
// clang-format on

void run() {
  Header H = Header::open(argument[0]);
  // Validate basic format and analyse contiguity.
  // Throws Exception if the image does not meet hard segmentation requirements.
  Connectome::validate_label_header(H);

  // ---------------------------------------------------------------
  // Report datatype
  // ---------------------------------------------------------------
  if (H.datatype().is_integer() && !H.datatype().is_signed()) {
    CONSOLE("Datatype: " + H.datatype().description() + " - image conforms to hard segmentation requirements");
  } else {
    CONSOLE("Datatype: " + H.datatype().description() + " - image values verified to be non-negative integers");
  }

  // Deeper analysis of image contents
  auto image = H.get_image<node_t>();
  const Connectome::LabelValidation result = Connectome::validate_label_image(image);

  // ---------------------------------------------------------------
  // Report label count
  // ---------------------------------------------------------------
  if (result.labels.empty())
    throw Exception("No non-background labels found (image contains only zeros)");

  const node_t max_label = *result.labels.rbegin();
  CONSOLE(str(result.labels.size()) + std::string(" unique non-background label(s) found;") + //
          " index range: 1 to " + str(max_label));                                            //

  // ---------------------------------------------------------------
  // Report index contiguity
  // ---------------------------------------------------------------
  if (result.indices_contiguous) {
    CONSOLE("Label indices: contiguous (all values 1 through " + str(max_label) + " are present)");
  } else {
    const size_t ngaps = result.missing_indices.size();
    WARN(std::string("Label indices: non-contiguous") +                                        //
         " (" + str(ngaps) + " value(s) missing from the range [1, " + str(max_label) + "])"); //
    // List the missing indices, abbreviated if there are many.
    constexpr size_t max_listed = 20;
    std::string missing_str;
    for (size_t i = 0; i < std::min(ngaps, max_listed); ++i) {
      if (i)
        missing_str += ", ";
      missing_str += str(result.missing_indices[i]);
    }
    if (ngaps > max_listed)
      missing_str += ", ... (and " + str(ngaps - max_listed) + " more)";
    CONSOLE("  Missing indices: " + missing_str);
  }

  // ---------------------------------------------------------------
  // Report number of labels without spatial contiguity
  // ---------------------------------------------------------------
  if (result.disconnected_components == 0) {
    CONSOLE("All " + str(result.labels.size()) + " labels are spatially contiguous");
  } else {
    std::string msg(str(result.disconnected_components) + " of " + str(result.labels.size()) +
                    " labels are spatially disconnected: ");
    std::vector<node_t> disconnected_labels;
    for (auto label : result.labels) {
      if (result.component_counts.at(label) > 1)
        disconnected_labels.push_back(label);
    }
    WARN(msg + str(disconnected_labels));
  }
}

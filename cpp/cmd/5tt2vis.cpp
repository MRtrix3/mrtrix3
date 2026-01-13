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

#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/ACT/tissues.h"

using namespace MR;
using namespace App;

constexpr default_type default_value_background = 0.0;
constexpr default_type default_value_cgm = 0.5;
constexpr default_type default_value_sgm = 0.75;
constexpr default_type default_value_wm = 1.0;
constexpr default_type default_value_csf = 0.15;
constexpr default_type default_value_pathology = 2.0;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Generate an image for visualisation purposes"
             " from an ACT 5TT segmented anatomical image";

  ARGUMENTS
  + Argument ("input",  "the input 4D tissue-segmented image").type_image_in()
  + Argument ("output", "the output 3D image for visualisation").type_image_out();

  OPTIONS

  + Option ("bg", "image intensity of background"
                  " (default: " + str(default_value_background, 2) + ")")
    + Argument ("value").type_float (0.0, 1.0)

  + Option ("cgm", "image intensity of cortical grey matter"
                   " (default: " + str(default_value_cgm, 2) + ")")
    + Argument ("value").type_float (0.0, 1.0)

  + Option ("sgm", "image intensity of sub-cortical grey matter"
                   " (default: " + str(default_value_sgm, 2) + ")")
    + Argument ("value").type_float (0.0, 1.0)

  + Option ("wm", "image intensity of white matter"
                  " (default: " + str(default_value_wm, 2) + ")")
    + Argument ("value").type_float (0.0, 1.0)

  + Option ("csf", "image intensity of CSF"
                   " (default: " + str(default_value_csf, 2) + ")")
    + Argument ("value").type_float (0.0, 1.0)

  + Option ("path", "image intensity of pathological tissue"
                    " (default: " + str(default_value_pathology, 2) + ")")
    + Argument ("value").type_float (0.0, 10.0);

}
// clang-format on

void run() {

  auto input = Image<float>::open(argument[0]);
  DWI::Tractography::ACT::verify_5TT_image(input);

  Header H(input);
  H.ndim() = 3;

  const float bg_multiplier = get_option_value("bg", default_value_background);
  const float cgm_multiplier = get_option_value("cgm", default_value_cgm);
  const float sgm_multiplier = get_option_value("sgm", default_value_sgm);
  const float wm_multiplier = get_option_value("wm", default_value_wm);
  const float csf_multiplier = get_option_value("csf", default_value_csf);
  const float path_multiplier = get_option_value("path", default_value_pathology);

  auto output = Image<float>::create(argument[1], H);

  auto f = [&](decltype(input) &in, decltype(output) &out) {
    const DWI::Tractography::ACT::Tissues t(in);
    const float bg = 1.0 - (t.get_cgm() + t.get_sgm() + t.get_wm() + t.get_csf() + t.get_path());
    out.value() = (bg_multiplier * bg) + (cgm_multiplier * t.get_cgm()) + (sgm_multiplier * t.get_sgm()) +
                  (wm_multiplier * t.get_wm()) + (csf_multiplier * t.get_csf()) + (path_multiplier * t.get_path());
  };
  ThreadedLoop(output).run(f, input, output);
}

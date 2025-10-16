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

#include "command.h"
#include "image.h"
#include "types.h"

#include "math/SH.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/SIFT/proc_mask.h"
#include "dwi/tractography/SIFT/sift.h"
#include "dwi/tractography/SIFT/sifter.h"

using namespace MR;
using namespace App;

using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Mapping;
using namespace MR::DWI::Tractography::SIFT;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Filter a whole-brain fibre-tracking data set"
             " such that the streamline densities match the FOD lobe integrals";

  REFERENCES
    + "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "SIFT: Spherical-deconvolution informed filtering of tractograms. "
    "NeuroImage, 2013, 67, 298-312";

  ARGUMENTS
  + Argument ("in_tracks", "the input track file").type_tracks_in()
  + Argument ("in_fod", "input image containing the spherical harmonics"
                        " of the fibre orientation distributions").type_image_in()
  + Argument ("out_tracks", "the output filtered tracks file").type_tracks_out();

  OPTIONS

  + Option ("nofilter", "do NOT perform track filtering;"
                        " just construct the model in order to provide output debugging images")

  + Option ("output_at_counts", "output filtered track files"
                                " (and optionally debugging images if -output_debug is specified)"
                                " at specific numbers of remaining streamlines;"
                                " provide as comma-separated list of integers")
    + Argument ("counts").type_sequence_int()

  + SIFTModelProcMaskOption
  + SIFTModelOption
  + SIFTOutputOption

  + Option ("out_selection", "output a text file containing the binary selection of streamlines")
    + Argument ("path").type_file_out()

  + SIFTTermOption;

}
// clang-format on

void run() {

  const std::string debug_path = get_option_value<std::string>("output_debug", "");

  auto in_dwi = Image<float>::open(argument[1]);
  Math::SH::check(in_dwi);
  DWI::Directions::FastLookupSet dirs(1281);

  SIFTer sifter(in_dwi, dirs);

  if (!debug_path.empty()) {
    sifter.initialise_debug_image_output(debug_path);
    sifter.output_proc_mask(Path::join(debug_path, "proc_mask.mif"));
    if (!get_options("act").empty())
      sifter.output_5tt_image(Path::join(debug_path, "5tt.mif"));
  }

  sifter.perform_FOD_segmentation(in_dwi);
  sifter.scale_FDs_by_GM();

  sifter.map_streamlines(argument[0]);

  if (!debug_path.empty())
    sifter.output_all_debug_images(debug_path, "before");

  sifter.remove_excluded_fixels();

  if (get_options("nofilter").empty()) {

    auto opt = get_options("term_number");
    if (!opt.empty())
      sifter.set_term_number(static_cast<MR::App::ParsedArgument::IntType>(opt[0][0]));
    opt = get_options("term_ratio");
    if (!opt.empty())
      sifter.set_term_ratio(static_cast<float>(opt[0][0]));
    opt = get_options("term_mu");
    if (!opt.empty())
      sifter.set_term_mu(static_cast<float>(opt[0][0]));
    opt = get_options("csv");
    if (!opt.empty())
      sifter.set_csv_path(opt[0][0]);
    opt = get_options("output_at_counts");
    if (!opt.empty()) {
      std::vector<uint32_t> counts = parse_ints<uint32_t>(opt[0][0]);
      sifter.set_regular_outputs(counts, debug_path);
    }

    sifter.perform_filtering();

    if (!debug_path.empty())
      sifter.output_all_debug_images(debug_path, "after");

    sifter.output_filtered_tracks(argument[0], argument[2]);

    opt = get_options("out_selection");
    if (!opt.empty())
      sifter.output_selection(opt[0][0]);
  }

  auto opt = get_options("out_mu");
  if (!opt.empty()) {
    File::OFStream out_mu(opt[0][0]);
    out_mu << sifter.mu();
  }
}

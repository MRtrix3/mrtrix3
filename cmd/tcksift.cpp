/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "math/sphere/SH.h"
#include "math/sphere/set/adjacency.h"
#include "math/sphere/set/predefined.h"

#include "dwi/tractography/SIFT/proc_mask.h"
#include "dwi/tractography/SIFT/sift.h"
#include "dwi/tractography/SIFT/sifter.h"




using namespace MR;
using namespace App;

using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Mapping;
using namespace MR::DWI::Tractography::SIFT;




void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Filter a whole-brain fibre-tracking data set such that the streamline densities match fixel-wise fibre densities";

  REFERENCES
    + "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "SIFT: Spherical-deconvolution informed filtering of tractograms. "
    "NeuroImage, 2013, 67, 298-312";

  ARGUMENTS
  + Argument ("in_tracks",  "the input track file").type_tracks_in()
  + Argument ("in_fd",      "input fixel data file containing fibre densities").type_image_in()
  + Argument ("out_tracks", "the output filtered tracks file").type_tracks_out();

  OPTIONS

  + Option ("nofilter", "do NOT perform track filtering - just construct the model in order to provide output debugging images")

  + Option ("output_at_counts", "output filtered track files (and optionally debugging images if -output_debug is specified) at specific "
                                "numbers of remaining streamlines; provide as comma-separated list of integers")
    + Argument ("counts").type_sequence_int()

  + SIFTModelWeightsOption
  + SIFTModelOption
  + SIFTOutputOption

  + Option ("out_selection", "output a text file containing the binary selection of streamlines")
    + Argument ("path").type_file_out()

  + SIFTTermOption;

}



void run ()
{

  const std::string debug_path = get_option_value<std::string> ("output_debug", "");

  SIFTer sifter (argument[1]);

  if (debug_path.size()) {
    sifter.initialise_debug_image_output (debug_path);
    //sifter.output_proc_mask (Path::join (debug_path, "proc_mask.mif"));
    if (get_options("act").size())
      sifter.output_5tt_image (Path::join (debug_path, "5tt.mif"));
  }

  if (App::get_options("fd_scale_gm").size())
    sifter.scale_FDs_by_GM();

  sifter.map_streamlines (argument[0]);

  if (debug_path.size())
    sifter.output_all_debug_images (debug_path, "before");

  sifter.exclude_fixels ();

  if (!get_options ("nofilter").size()) {

    auto opt = get_options ("term_number");
    if (opt.size())
      sifter.set_term_number (int(opt[0][0]));
    opt = get_options ("term_ratio");
    if (opt.size())
      sifter.set_term_ratio (float(opt[0][0]));
    opt = get_options ("term_mu");
    if (opt.size())
      sifter.set_term_mu (float(opt[0][0]));
    opt = get_options ("csv");
    if (opt.size())
      sifter.set_csv_path (opt[0][0]);
    opt = get_options ("output_at_counts");
    if (opt.size()) {
      vector<uint32_t> counts = parse_ints<uint32_t> (opt[0][0]);
      sifter.set_regular_outputs (counts, debug_path);
    }

    sifter.perform_filtering();

    if (debug_path.size())
      sifter.output_all_debug_images (debug_path, "after");

    sifter.output_filtered_tracks (argument[0], argument[2]);

    opt = get_options ("out_selection");
    if (opt.size())
      sifter.output_selection (opt[0][0]);

  }

  auto opt = get_options ("out_mu");
  if (opt.size()) {
    File::OFStream out_mu (opt[0][0]);
    out_mu << sifter.mu();
  }

}





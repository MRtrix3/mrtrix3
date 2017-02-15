/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include <vector>

#include "command.h"

#include "image.h"

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




void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Filter a whole-brain fibre-tracking data set such that the streamline densities match the FOD lobe integrals";

  REFERENCES 
    + "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "SIFT: Spherical-deconvolution informed filtering of tractograms. "
    "NeuroImage, 2013, 67, 298-312";

  ARGUMENTS
  + Argument ("in_tracks",  "the input track file").type_tracks_in()
  + Argument ("in_fod",     "input image containing the spherical harmonics of the fibre orientation distributions").type_image_in()
  + Argument ("out_tracks", "the output filtered tracks file").type_tracks_out();

  OPTIONS

  + Option ("nofilter", "do NOT perform track filtering - just construct the model in order to provide output debugging images")

  + Option ("output_at_counts", "output filtered track files (and optionally debugging images if -output_debug is specified) at specific "
                                "numbers of remaining streamlines; provide as comma-separated list of integers")
    + Argument ("counts").type_sequence_int()

  + SIFTModelProcMaskOption
  + SIFTModelOption
  + SIFTOutputOption

  + Option ("out_selection", "output a text file containing the binary selection of streamlines")
    + Argument ("path").type_file_out()

  + SIFTTermOption;

}



void run ()
{

  auto opt = get_options ("output_debug");
  const bool out_debug = opt.size();

  auto in_dwi = Image<float>::open (argument[1]);
  Math::SH::check (in_dwi);
  DWI::Directions::FastLookupSet dirs (1281);

  SIFTer sifter (in_dwi, dirs);

  if (out_debug) {
    sifter.output_proc_mask ("proc_mask.mif");
    if (get_options("act").size())
      sifter.output_5tt_image ("5tt.mif");
  }

  sifter.perform_FOD_segmentation (in_dwi);
  sifter.scale_FDs_by_GM();

  sifter.map_streamlines (argument[0]);

  if (out_debug)
    sifter.output_all_debug_images ("before");

  sifter.remove_excluded_fixels ();

  opt = get_options ("nofilter");
  if (!opt.size()) {

    opt = get_options ("term_number");
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
      vector<int> counts = parse_ints (opt[0][0]);
      sifter.set_regular_outputs (counts, out_debug);
    }

    sifter.perform_filtering();

    if (out_debug)
      sifter.output_all_debug_images ("after");

    sifter.output_filtered_tracks (argument[0], argument[2]);

    opt = get_options ("out_selection");
    if (opt.size())
      sifter.output_selection (opt[0][0]);

  }

  opt = get_options ("out_mu");
  if (opt.size()) {
    File::OFStream out_mu (opt[0][0]);
    out_mu << sifter.mu();
  }

}





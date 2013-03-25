/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2011.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */




#include "app.h"
#include "point.h"
#include "progressbar.h"

#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/header.h"
#include "image/loop.h"
#include "image/voxel.h"

#include "image/interp/linear.h"

#include "thread/exec.h"
#include "thread/queue.h"

#include "dwi/tractography/file.h"

#include "dwi/tractography/mapping/loader.h"

#include "dwi/tractography/ACT/tissues.h"

#include "dwi/tractography/SIFT/multithread.h"
#include "dwi/tractography/SIFT/sifter.h"



MRTRIX_APPLICATION

using namespace MR;
using namespace App;

using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Mapping;
using namespace MR::DWI::Tractography::SIFT;




#define TERMINATION_RATIO_DEFAULT 0.0
#define TERMINATION_NUMBER_DEFAULT 1
#define TERMINATION_MU_DEFAULT 0.0





void usage ()
{

  AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "filter a whole-brain fibre-tracking data set such that the streamline densities match the FOD lobe integrals.";

  ARGUMENTS
  + Argument ("in_tracks",  "the input track file.").type_file()
  + Argument ("in_fod",     "input image containing the spherical harmonics of the fibre orientation distributions").type_image_in()
  + Argument ("out_tracks", "the output filtered tracks file").type_file();

  OPTIONS

  + Option ("proc_mask", "provide an image containing the processing mask weights for the SIFT model. \n"
                         "Image spatial dimensions must match the FOD image.")
    + Argument ("image").type_image_in()

  + Option ("act", "use an ACT four-tissue-type segmented anatomical image to derive the processing mask for SIFT. \n")
    + Argument ("image").type_image_in()

  + Option ("no_dilate_lut", "do NOT dilate FOD lobe lookup tables; only map streamlines to FOD lobes if the precise tangent lies within the angular spread of that lobe")

  + Option ("make_null_lobes", "add an additional FOD lobe to each voxel, with zero integral, that covers all directions with zero / negative FOD amplitudes")

  + Option ("remove_untracked", "remove FOD lobes that do not have any streamline density attributed to them; "
                                "this improves filtering slightly, at the expense of longer computation time "
                                "(and you can no longer do quantitative comparisons between reconstructions if this is enabled)")

  + Option ("FOD_int_thresh", "FOD integral threshold; exclude an FOD lobe from filtering processing if its integral is less than this amount "
                              "(streamlines will still be mapped to it, but it will not contribute to the cost function or the filtering)")
    + Argument ("value").type_float (0.0, 0.0, 2.0 * M_PI)

  + Option ("nofilter", "do NOT perform track filtering - just get the initial output images")

  + Option ("term_number", "number of streamlines - continue filtering until this number of streamlines remain")
    + Argument ("value").type_integer (1, TERMINATION_NUMBER_DEFAULT, std::numeric_limits<int>::max())

  + Option ("term_ratio", "termination ratio - defined as the ratio between reduction in cost function, and reduction in density of streamlines.\n"
                          "Smaller values result in more streamlines being filtered out.")
    + Argument ("value").type_float (0.0, TERMINATION_RATIO_DEFAULT, 10.0)

  + Option ("term_mu", "terminate filtering once the SIFT proportionality coefficient reaches a given value")
    + Argument ("value").type_float (0.0, TERMINATION_MU_DEFAULT, std::numeric_limits<float>::max())

  + Option ("csv", "output statistics of execution per iteration to a .csv file")
    + Argument ("file").type_file()

  + Option ("output_at_counts", "output filtered track files (and optionally debugging images if -output_debug is specified) at certain "
                                "numbers of remaining streamlines; provide as comma-separated list of integers")
    + Argument ("counts").type_sequence_int()

  + Option ("output_debug", "provide various output images for assessing & debugging filtering performace etc.");

};



void run ()
{

  Options opt = get_options ("output_debug");
  const bool out_debug = opt.size();

  Image::Buffer<float> in_dwi (argument[1]);
  DWI::Directions::FastLookupSet dirs (1281);

  SIFTer sifter (in_dwi, dirs);

  if (out_debug)
    sifter.output_proc_mask ("proc_mask.mif");

  opt = get_options ("no_dilate_lut");
  bool dilate_lut = !opt.size();
  if (dilate_lut)
    sifter.FMLS_set_dilate_lookup_table();
  opt = get_options ("make_null_lobes");
  if (opt.size()) {
    if (dilate_lut)
      throw Exception ("Option -make_null_lobes only works if lookup tables are NOT dilated (i.e. also provide option -no_dilate_lut)");
    sifter.FMLS_set_create_null_lobe();
  }

  sifter.perform_FOD_segmentation ();
  sifter.map_streamlines (argument[0]);

  if (out_debug)
    sifter.output_all_debug_images ("before");

  opt = get_options ("remove_untracked");
  if (opt.size())
    sifter.set_remove_untracked_lobes (true);
  opt = get_options ("FOD_int_thresh");
  if (opt.size())
    sifter.set_min_FOD_integral (opt[0][0]);

  sifter.remove_excluded_lobes();

  opt = get_options ("nofilter");
  if (!opt.size()) {

    opt = get_options ("term_number");
    if (opt.size())
      sifter.set_term_number (int(opt[0][0]));
    opt = get_options ("term_ratio");
    if (opt.size())
      sifter.set_term_ratio (opt[0][0]);
    opt = get_options ("term_mu");
    if (opt.size())
      sifter.set_term_mu (opt[0][0]);
    opt = get_options ("csv");
    if (opt.size())
      sifter.set_csv_path (opt[0][0]);
    opt = get_options ("output_at_counts");
    if (opt.size()) {
      std::vector<int> counts = parse_ints (opt[0][0]);
      sifter.set_regular_outputs (counts, out_debug);
    }

    sifter.perform_filtering();

    if (out_debug)
      sifter.output_all_debug_images ("after");

    sifter.output_filtered_tracks (argument[0], argument[2]);

  }

}





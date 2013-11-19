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

#include "image/buffer.h"

#include "dwi/tractography/SIFT/sift.h"
#include "dwi/tractography/SIFT/sifter.h"



MRTRIX_APPLICATION

using namespace MR;
using namespace App;

using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Mapping;
using namespace MR::DWI::Tractography::SIFT;




void usage ()
{

  AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "filter a whole-brain fibre-tracking data set such that the streamline densities match the FOD lobe integrals.";

  ARGUMENTS
  + Argument ("in_tracks",  "the input track file").type_file()
  + Argument ("in_fod",     "input image containing the spherical harmonics of the fibre orientation distributions").type_image_in()
  + Argument ("out_tracks", "the output filtered tracks file").type_file();

  OPTIONS

  + Option ("nofilter", "do NOT perform track filtering - just construct the model in order to provide output debugging images")

  + Option ("output_at_counts", "output filtered track files (and optionally debugging images if -output_debug is specified) at specific "
                                "numbers of remaining streamlines; provide as comma-separated list of integers")
    + Argument ("counts").type_sequence_int()

  + FixelMapProcMaskOption

  + SIFTModelOption
  + SIFTOutputOption
  + SIFTTermOption;

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

  if (sifter.have_act_data()) {
    opt = get_options ("no_fod_scaling");
    if (!opt.size())
      sifter.scale_FODs_by_GM();
  }

  sifter.map_streamlines (argument[0]);

  if (out_debug)
    sifter.output_all_debug_images ("before");

  opt = get_options ("remove_untracked");
  if (opt.size())
    sifter.set_remove_untracked_fixels (true);
  opt = get_options ("FOD_int_thresh");
  if (opt.size())
    sifter.set_min_FOD_integral (float(opt[0][0]));

  sifter.remove_excluded_fixels();

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
      std::vector<int> counts = parse_ints (opt[0][0]);
      sifter.set_regular_outputs (counts, out_debug);
    }

    sifter.perform_filtering();

    if (out_debug)
      sifter.output_all_debug_images ("after");

    sifter.output_filtered_tracks (argument[0], argument[2]);

  }

}





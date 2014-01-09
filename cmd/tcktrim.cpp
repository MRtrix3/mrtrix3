/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by David Vaughan 13/8/13.
   Much code copied from mrstats.cpp, written by J-Donald Tournier, 27/06/08.

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

#include <fstream>
#include <iomanip>

#include "command.h"
#include "point.h"
#include "bitset.h"
#include "progressbar.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"
#include "math/rng.h"



using namespace MR;
using namespace MR::DWI;
using namespace App;

void usage ()
{

  AUTHOR = "David Vaughan (d.vaughan@brain.org.au)";

  DESCRIPTION
    + "trim tracks to be present only within a mask region. "
    "If a track leaves and re-enters the mask, multiple short tracks are created. " ;

  ARGUMENTS
    + Argument ("in_tracks", "the input track file.").type_file()
    + Argument ("mask", "the mask file.").type_image_in()
    + Argument ("out_tracks", "the output track file.").type_file() ;

  OPTIONS
    + Option ( "minlength", "minimum track length that will be written out in mm (default=0).")
    + Argument ("value").type_float(0.0,0.0,INFINITY)

    + Tractography::TrackWeightsInOption
    + Tractography::TrackWeightsOutOption;

}


typedef float value_type;
typedef cfloat complex_type;
typedef Tractography::Streamline<float> Track;


void run ()
{

  Tractography::Properties properties;
  Tractography::Reader<float> file (argument[0], properties);
  const size_t count = properties["count"].empty() ? 0 : to<int> (properties["count"]);

  Image::Buffer<bool> data ( argument[1] );
  Image::Buffer<bool>::voxel_type vox ( data );
  Image::Transform transform (data);

  Tractography::Writer<float> writer (argument[2], properties);

  // set the minimum length of trimmed tracks
  float min_length=0;
  Options opt = get_options ("minlength");
  if (opt.size()) {
    min_length = opt[0][0];
  }

  // Loop through all of the tracks
  ProgressBar progress ("analysing tracks...", count);
  Track in_track, out_track;
  size_t out_count=0;

  while (file (in_track) ) {

    // loop through the points on the in_track
    bool within_mask = false;
    out_track.clear();
    out_track.weight = in_track.weight;

    for ( Track::iterator pt = in_track.begin() ; pt != in_track.end() ; ++pt ) {

      // is this point in the mask ?
      Point<float> p = transform.scanner2voxel( *pt );
      for (size_t i=0; i<3; i++)
        vox[i] = Math::round (p[i]);

      if (Image::Nav::within_bounds (vox)) {

        if (! within_mask ) {
          // while we are outside the mask, either wait, or start a new track
          if ( vox.value() ) {
            within_mask = true;
            out_track.clear();
            out_track.push_back(*pt);
          }

        } else {
          // while we are within the mask, keep writing out this track, or end
          if ( vox.value() ) {
            // continue with track
            out_track.push_back(*pt);
          } else {
            // now outside the mask, so write the track and start another
            within_mask=false;

            // check length before writing it out
            float len = 0;
            for (size_t i = 0; i < out_track.size() -1; ++i ) {
              len += dist( out_track[i], out_track[i+1]);
              if (len> min_length) break;
            }
            if (len > min_length) {
              writer (out_track);
              out_count++;
            }
          }

        }
      }
    }
    // write out any remaining track fragment
    if (out_track.size()) {
      // check length before writing it out
      float len = 0;
      for (size_t i = 0; i < out_track.size() -1; ++i ) {
        len += dist( out_track[i], out_track[i+1]);
        if (len> min_length) break;
      }
      if (len > min_length) {
        writer (out_track);
        out_count++;
      }
    }

    progress++;
  }

  writer.total_count = std::max (writer.total_count, out_count);

}

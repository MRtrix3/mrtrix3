  /*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt

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

#include "command.h"
#include "progressbar.h"
#include "image/buffer.h"
#include "image/buffer_sparse.h"
#include "image/loop.h"
#include "image/voxel.h"
#include "image/sparse/fixel_metric.h"
#include "image/sparse/voxel.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/scalar_file.h"
#include "math/rng.h"


using namespace MR;
using namespace App;


using Image::Sparse::FixelMetric;


void usage ()
{

  DESCRIPTION
  + "generates fixel ROI maps based on tracks";

  ARGUMENTS
  + Argument ("fixel_in", "the input sparse fixel image.").type_image_in ()

  + Argument ("tracks", "the input tract of interest").type_file ()

  + Argument ("signal", "desired signal").type_float ()

  + Argument ("noise", "desired noise (stdev)").type_float ()

  + Argument ("fixel_out", "the output sparse fixel image.").type_image_out ();

}


#define ANGULAR_THRESHOLD 30


void run ()
{
  Image::Header input_header (argument[0]);
  Image::BufferSparse<FixelMetric> input_data (input_header);
  Image::BufferSparse<FixelMetric>::voxel_type input_fixel (input_data);

  Image::Header output_header (input_header);
  Image::BufferSparse<FixelMetric> output_data (argument[4], output_header);
  Image::BufferSparse<FixelMetric>::voxel_type output_fixel (output_data);

  Image::LoopInOrder loop (output_fixel);

  // create output fixel file and intialise with zeros
  for (loop.start (input_fixel, output_fixel); loop.ok(); loop.next (input_fixel, output_fixel)) {
    output_fixel.value().set_size (input_fixel.value().size());
    for (size_t f = 0; f != input_fixel.value().size(); ++f) {
      output_fixel.value()[f] = input_fixel.value()[f];
      output_fixel.value()[f].value = 0.0;
    }
  }

  float signal = argument[2];
  float noise = argument[3];

  float angular_threshold_dp = cos (ANGULAR_THRESHOLD * (M_PI/180.0));

  // loop through tracks and give corresponding fixels a fake signal
  DWI::Tractography::Properties tck_properties;
  DWI::Tractography::Reader<float> tck_reader (argument[1], tck_properties);
  const size_t num_tracks = tck_properties["count"].empty() ? 0 : to<int> (tck_properties["count"]);
  if (!num_tracks)
    throw Exception ("no tracks found in input file");

  Image::Transform transform (input_fixel);
  DWI::Tractography::Streamline<float> tck;

  {
    ProgressBar progress ("Generating fake test statistic fixel image...", num_tracks);
    while (tck_reader (tck)) {
      progress++;
      Point<float> tangent;
      std::vector<float> scalars;

      for (size_t p = 0; p < tck.size(); ++p) {
        if (p == 0)
          tangent = tck[p+1] - tck[p];
        else if (p == tck.size() - 1)
          tangent = tck[p] - tck[p-1];
        else
          tangent = tck[p+1] - tck[p-1];
        tangent.normalise();

        Point<float> voxel_pos = transform.scanner2voxel (tck[p]);
        voxel_pos[0] = Math::round (voxel_pos[0]);
        voxel_pos[1] = Math::round (voxel_pos[1]);
        voxel_pos[2] = Math::round (voxel_pos[2]);
        Image::Nav::set_pos (output_fixel, voxel_pos);
        float largest_dp = 0.0;
        int closest_fixel_index = -1;
        for (size_t f = 0; f < output_fixel.value().size(); ++f ) {
          float dp = Math::abs (tangent.dot (output_fixel.value()[f].dir));
          if (dp > largest_dp) {
            largest_dp = dp;
            closest_fixel_index = f;
          }
        }
        if (largest_dp > angular_threshold_dp)
          output_fixel.value()[closest_fixel_index].value += 1.0;
      }
    }
  }

  // Only label 1 fixel per voxel as belonging to this tract
  for (loop.start (output_fixel); loop.ok(); loop.next (output_fixel)) {
    float largest = 0.0;
    bool contains_tracks = false;
    size_t highest_fixel = 0;
    for (size_t f = 0; f < output_fixel.value().size(); ++f) {
      if (output_fixel.value()[f].value > largest) {
        largest = output_fixel.value()[f].value;
        highest_fixel = f;
        contains_tracks = true;
      }
    }
    if (contains_tracks) {
      for (size_t f = 0; f < output_fixel.value().size(); ++f) {
        if (f == highest_fixel) {
          output_fixel.value()[f].value = signal;
        } else {
          output_fixel.value()[f].value = 0.0;
        }
      }
    }

  }


  Math::RNG rng;
  // Add gaussian noise with zero mean and unit variance
  Image::LoopInOrder loop2 (output_fixel, "adding Gaussian noise to output fixel image...");
  for (loop2.start (output_fixel); loop2.ok(); loop2.next (output_fixel)) {
    for (size_t f = 0; f != output_fixel.value().size(); ++f)
      output_fixel.value()[f].value = output_fixel.value()[f].value + rng.normal(noise);
  }

}


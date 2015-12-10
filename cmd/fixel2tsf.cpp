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
#include "algo/loop.h"

#include "image.h"
#include "sparse/fixel_metric.h"
#include "sparse/keys.h"
#include "sparse/image.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/scalar_file.h"

#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/loader.h"


using namespace MR;
using namespace App;



using Sparse::FixelMetric;



void usage ()
{

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Map fixel values to a track scalar file based on an input tractogram. "
    "This is useful for visualising the output from fixelcfestats in 3D.";

  ARGUMENTS
  + Argument ("fixel_in", "the input fixel image").type_image_in ()
  + Argument ("tracks",   "the input track file ").type_tracks_in ()
  + Argument ("tsf",      "the output track scalar file").type_file_out ();


  OPTIONS
  + Option ("angle", "the max anglular threshold for computing correspondence between a fixel direction and track tangent")
  + Argument ("value").type_float (0.001, 30, 90);

}

typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;


void run ()
{
  DWI::Tractography::Properties properties;

  auto input_header = Header::open (argument[0]);
  Sparse::Image<FixelMetric> input_fixel (argument[0]);

  DWI::Tractography::Reader<float> reader (argument[1], properties);
  properties.comments.push_back ("Created using fixel2tsf");
  properties.comments.push_back ("Source fixel image: " + Path::basename (argument[0]));
  properties.comments.push_back ("Source track file: " + Path::basename (argument[1]));

  DWI::Tractography::ScalarWriter<float> tsf_writer (argument[2], properties);

  float angular_threshold = get_option_value("angle", 30.0);
  const float angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));

  const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);

  DWI::Tractography::Mapping::TrackMapperBase mapper (input_header);
  mapper.set_use_precise_mapping (true);

  ProgressBar progress ("mapping fixel values to streamline points", num_tracks);
  DWI::Tractography::Streamline<float> tck;

  Transform transform (input_fixel);
  Eigen::Vector3 voxel_pos;

  while (reader (tck)) {
    SetVoxelDir dixels;
    mapper (tck, dixels);
    std::vector<float> scalars (tck.size(), 0.0);
    for (size_t p = 0; p < tck.size(); ++p) {
      voxel_pos = transform.scanner2voxel * tck[p].cast<default_type> ();
      for (SetVoxelDir::const_iterator d = dixels.begin(); d != dixels.end(); ++d) {
        if ((int)round(voxel_pos[0]) == (*d)[0] && (int)round(voxel_pos[1]) == (*d)[1] && (int)round(voxel_pos[2]) == (*d)[2]) {
          assign_pos_of (*d).to (input_fixel);
          Eigen::Vector3f dir = d->get_dir();
          dir.normalize();
          float largest_dp = 0.0;
          int32_t closest_fixel_index = -1;
          for (size_t f = 0; f != input_fixel.value().size(); ++f) {
            float dp = std::abs (dir.dot (input_fixel.value()[f].dir));
            if (dp > largest_dp) {
              largest_dp = dp;
              closest_fixel_index = f;
            }
          }
          if (largest_dp > angular_threshold_dp)
            scalars[p] = input_fixel.value()[closest_fixel_index].value;
          else
            scalars[p] = 0.0;
          break;
        }
      }
    }
    tsf_writer (scalars);
    progress++;
  }
}


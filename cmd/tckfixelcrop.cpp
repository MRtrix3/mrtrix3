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

#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/loader.h"


using namespace MR;
using namespace App;



using Image::Sparse::FixelMetric;



void usage ()
{

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Crop tractogram based on a fixel image. This can be useful for displaying "
    "fixel images in 3D using a tractogram. The output tracks can also be coloured "
    "by outputting a track scalar file derived from the fixel values.";

  ARGUMENTS
  + Argument ("tracks",   "the input track file ").type_file ()
  + Argument ("fixel_in", "the input fixel image").type_image_in ()
  + Argument ("tracks",   "the output track file ").type_file ();


  OPTIONS
  + Option ("tsf", "output an accompanying track scalar file containing the fixel values")
  + Argument ("path").type_image_out ()

  + Option ("angle", "the max anglular threshold for computing correspondence between a fixel direction and track tangent")
  + Argument ("value").type_float (0.001, 30, 90)

  + Option ("threshold", "don't include fixels below the specified threshold")
  + Argument ("value").type_float (std::numeric_limits<float>::min(), 0.0, std::numeric_limits<float>::max());


}

typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;


void run ()
{
  DWI::Tractography::Properties properties;

  DWI::Tractography::Reader<float> reader (argument[0], properties);
  properties.comments.push_back ("Created using tckfixelcrop");
  properties.comments.push_back ("Source track file: " + Path::basename (argument[0]));
  properties.comments.push_back ("Source fixel image: " + Path::basename (argument[1]));

  Image::Header input_header (argument[1]);
  Image::BufferSparse<FixelMetric> input_data (input_header);
  Image::BufferSparse<FixelMetric>::voxel_type input_fixel (input_data);

  DWI::Tractography::Writer<float> tck_writer (argument[2], properties);

  Ptr< DWI::Tractography::ScalarWriter<float> > tsf_writer;
  Options opt = get_options ("tsf");
  if (opt.size())
    tsf_writer = new DWI::Tractography::ScalarWriter<float> (opt[0][0], properties);

  float angular_threshold = 30.0;
  opt = get_options ("angle");
  if (opt.size())
    angular_threshold = opt[0][0];
  const float angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));

  float fixel_threshold = 0.0;
  opt = get_options ("threshold");
  if (opt.size())
    fixel_threshold = opt[0][0];

  const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);

  DWI::Tractography::Mapping::TrackMapperBase<SetVoxelDir> mapper (input_header);

  ProgressBar progress ("cropping tracks by fixels...", num_tracks);
  DWI::Tractography::Streamline<float> tck;

  Image::Transform transform (input_fixel);
  Point<float> voxel_pos;

  while (reader (tck)) {
    SetVoxelDir dixels;
    mapper (tck, dixels);
    DWI::Tractography::Streamline<float> temp_tck;
    std::vector<float> temp_scalars;
    for (size_t p = 0; p < tck.size(); ++p) {
      transform.scanner2voxel (tck[p], voxel_pos);
      for (SetVoxelDir::const_iterator d = dixels.begin(); d != dixels.end(); ++d) {
        if ((int)round(voxel_pos[0]) == (*d)[0] && (int)round(voxel_pos[1]) == (*d)[1] && (int)round(voxel_pos[2]) == (*d)[2]) {
          Image::Nav::set_pos (input_fixel, (*d));
          Point<float> dir (d->get_dir());
          dir.normalise();
          float largest_dp = 0.0;
          int32_t closest_fixel_index = -1;
          for (size_t f = 0; f != input_fixel.value().size(); ++f) {
            float dp = Math::abs (dir.dot (input_fixel.value()[f].dir));
            if (dp > largest_dp) {
              largest_dp = dp;
              closest_fixel_index = f;
            }
          }
          if (largest_dp > angular_threshold_dp) {
            if (input_fixel.value()[closest_fixel_index].value > fixel_threshold) {
              temp_tck.push_back(tck[p]);
              if (tsf_writer)
                temp_scalars.push_back (input_fixel.value()[closest_fixel_index].value);
            } else if (temp_tck.size()) {
              tck_writer (temp_tck);
              temp_tck.clear();
              if (tsf_writer) {
                (*tsf_writer) (temp_scalars);
                temp_scalars.clear();
              }
            }
          }
        }
      }
    }
    progress++;
  }
}


/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "command.h"
#include "progressbar.h"
#include "algo/loop.h"
#include "image.h"

#include "fixel/helpers.h"
#include "fixel/keys.h"
#include "fixel/loop.h"

#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/writer.h"


using namespace MR;
using namespace App;

#define DEFAULT_ANGLE_THRESHOLD 45.0


class TrackProcessor { MEMALIGN (TrackProcessor)

 public:

   using SetVoxelDir = DWI::Tractography::Mapping::SetVoxelDir;

   TrackProcessor (Image<uint32_t>& fixel_indexer,
                   const vector<Eigen::Vector3>& fixel_directions,
                   vector<uint16_t>& fixel_TDI,
                   const float angular_threshold):
     fixel_indexer (fixel_indexer) ,
     fixel_directions (fixel_directions),
     fixel_TDI (fixel_TDI),
     angular_threshold_dp (std::cos (angular_threshold * (Math::pi/180.0))) { }


   bool operator () (const SetVoxelDir& in)  {
     // For each voxel tract tangent, assign to a fixel
     vector<int32_t> tract_fixel_indices;
     for (SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
       assign_pos_of (*i).to (fixel_indexer);
       fixel_indexer.index(3) = 0;
       uint32_t num_fibres = fixel_indexer.value();
       if (num_fibres > 0) {
         fixel_indexer.index(3) = 1;
         uint32_t first_index = fixel_indexer.value();
         uint32_t last_index = first_index + num_fibres;
         uint32_t closest_fixel_index = 0;
         float largest_dp = 0.0;
         const Eigen::Vector3 dir (i->get_dir().normalized());
         for (uint32_t j = first_index; j < last_index; ++j) {
           const float dp = abs (dir.dot (fixel_directions[j]));
           if (dp > largest_dp) {
             largest_dp = dp;
             closest_fixel_index = j;
           }
         }
         if (largest_dp > angular_threshold_dp) {
           tract_fixel_indices.push_back (closest_fixel_index);
           fixel_TDI[closest_fixel_index]++;
         }
       }
     }
     return true;
   }


 private:
   Image<uint32_t> fixel_indexer;
   const vector<Eigen::Vector3>& fixel_directions;
   vector<uint16_t>& fixel_TDI;
   const float angular_threshold_dp;
};


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Compute a fixel TDI map from a tractogram";

  ARGUMENTS
  + Argument ("tracks",  "the input tracks.").type_tracks_in()
  + Argument ("fixel_folder_in", "the input fixel folder. Used to define the fixels and their directions").type_directory_in()
  + Argument ("fixel_folder_out", "the fixel folder to which the output will be written. This can be the same as the input folder if desired").type_text()
  + Argument ("fixel_data_out", "the name of the fixel data image.").type_image_out();

  OPTIONS
  + Option ("angle", "the max angle threshold for assigning streamline tangents to fixels (Default: " + str(DEFAULT_ANGLE_THRESHOLD, 2) + " degrees)")
  + Argument ("value").type_float (0.0, 90.0);
}



template <class VectorType>
void write_fixel_output (const std::string& filename,
                         const VectorType& data,
                         const Header& header)
{
  auto output = Image<float>::create (filename, header);
  for (uint32_t i = 0; i < data.size(); ++i) {
    output.index(0) = i;
    output.value() = data[i];
  }
}



void run ()
{
  const std::string input_fixel_folder = argument[1];
  Header index_header = Fixel::find_index_header (input_fixel_folder);
  auto index_image = index_header.get_image<uint32_t>();

  const uint32_t num_fixels = Fixel::get_number_of_fixels (index_header);

  const float angular_threshold = get_option_value ("angle", DEFAULT_ANGLE_THRESHOLD);

  vector<Eigen::Vector3> positions (num_fixels);
  vector<Eigen::Vector3> directions (num_fixels);

  const std::string output_fixel_folder = argument[2];
  Fixel::copy_index_and_directions_file (input_fixel_folder, output_fixel_folder);

  {
    auto directions_data = Fixel::find_directions_header (input_fixel_folder).get_image<default_type>().with_direct_io();
    // Load template fixel directions
    Transform image_transform (index_image);
    for (auto i = Loop ("loading template fixel directions and positions", index_image, 0, 3)(index_image); i; ++i) {
      const Eigen::Vector3 vox ((default_type)index_image.index(0), (default_type)index_image.index(1), (default_type)index_image.index(2));
      index_image.index(3) = 1;
      uint32_t offset = index_image.value();
      size_t fixel_index = 0;
      for (auto f = Fixel::Loop (index_image) (directions_data); f; ++f, ++fixel_index) {
        directions[offset + fixel_index] = directions_data.row(1);
        positions[offset + fixel_index] = image_transform.voxel2scanner * vox;
      }
    }
  }

  vector<uint16_t> fixel_TDI (num_fixels, 0.0);
  const std::string track_filename = argument[0];
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<float> track_file (track_filename, properties);
  // Read in tracts, and compute whole-brain fixel-fixel connectivity
  const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
  if (!num_tracks)
    throw Exception ("no tracks found in input file");

  {
    using SetVoxelDir = DWI::Tractography::Mapping::SetVoxelDir;
    DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "mapping tracks to fixels");
    DWI::Tractography::Mapping::TrackMapperBase mapper (index_image);
    mapper.set_upsample_ratio (DWI::Tractography::Mapping::determine_upsample_ratio (index_header, properties, 0.333f));
    mapper.set_use_precise_mapping (true);
    TrackProcessor tract_processor (index_image, directions, fixel_TDI, angular_threshold);
    Thread::run_queue (
        loader,
        Thread::batch (DWI::Tractography::Streamline<float>()),
        mapper,
        Thread::batch (SetVoxelDir()),
        tract_processor);
  }
  track_file.close();

  Header output_header (Fixel::data_header_from_index (index_image));

  write_fixel_output (Path::join (output_fixel_folder, argument[3]), fixel_TDI, output_header);

}

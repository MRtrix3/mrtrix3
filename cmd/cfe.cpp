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
#include "math/rng.h"
#include "stats/cfe.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/writer.h"


using namespace MR;
using namespace App;


using Image::Sparse::FixelMetric;




void usage ()
{

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "perform connectivity-based fixel enhancement";

  ARGUMENTS
  + Argument ("fixel_in", "the input sparse fixel image.").type_image_in ()

  + Argument ("tracks", "the tractogram used to derive fixel-fixel connectivity").type_file_in ()

  + Argument ("fixel_out", "the output sparse fixel image.").type_image_out ();

  OPTIONS

  + Option ("dh", "the height increment used in the CFE integration (default = 0.1)")
  + Argument ("value").type_float (0.001, 0.1, 100000)

  + Option ("cfe_e", "cfe height parameter (default = 1.0)")
  + Argument ("value").type_float (0.0, 0.5, 100000)

  + Option ("cfe_h", "cfe extent parameter (default = 2.0)")
  + Argument ("value").type_float (0.0, 2.0, 100000)

  + Option ("cfe_c", "cfe connectivity parameter (default = 0.5)")
  + Argument ("value").type_float (0.0, 0.5, 100000)

  + Option ("angle", "the max angle threshold for assigning track tangents to fixels")
  + Argument ("value").type_float (0.001, 30, 90)

  + Option ("connectivity", "a threshold to define the required fraction of shared connections to be included in the neighbourhood (default: 1%)")
  + Argument ("threshold").type_float (0.001, 0.01, 1.0)

  + Option ("smooth", "perform connectivity-based smoothing using a Gaussian kernel with the supplied FWHM (default: 5mm)")
  + Argument ("FWHM").type_float (0.0, 5.0, 200.0)

  + Option ("smoothonly", "don't perform cfe smooth only");

}


#define ANGULAR_THRESHOLD 45

typedef float value_type;

class FixelIndex
{
  public:
  FixelIndex (const Point<float>& dir, uint32_t index) :
      dir (dir),
      index (index) { }
  FixelIndex () :
      dir (),
      index (0) { }
    Point<float> dir;
    uint32_t index;
};



void run ()
{

  Options opt = get_options ("dh");
  value_type dh = 0.1;
  if (opt.size())
   dh = opt[0][0];

  opt = get_options ("cfe_h");
  value_type cfe_H = 2.0;
  if (opt.size())
   cfe_H = opt[0][0];

  opt = get_options ("cfe_e");
  value_type cfe_E = 1.0;
  if (opt.size())
   cfe_E = opt[0][0];

  opt = get_options ("cfe_c");
  value_type cfe_C = 0.5;
  if (opt.size())
   cfe_C = opt[0][0];


  value_type angular_threshold = ANGULAR_THRESHOLD;
  opt = get_options ("angle");
  if (opt.size())
   angular_threshold = opt[0][0];

  value_type connectivity_threshold = 0.01;
  opt = get_options ("connectivity");
  if (opt.size())
   connectivity_threshold = opt[0][0];

  value_type smooth_std_dev = 5.0 / 2.3548;
  opt = get_options ("smooth");
  if (opt.size())
   smooth_std_dev = value_type (opt[0][0]) / 2.3548;

  std::vector<Point<value_type> > fixel_directions;
  DWI::Directions::Set dirs (1281);
  Image::Header index_header (argument[0]);
  index_header.set_ndim(4);
  index_header.dim(3) = 2;
  index_header.datatype() = DataType::Int32;
  Image::BufferScratch<int32_t> indexer (index_header);
  Image::BufferScratch<int32_t>::voxel_type indexer_vox (indexer);
  Image::LoopInOrder loop4D (indexer_vox);
  for (loop4D.start (indexer_vox); loop4D.ok(); loop4D.next (indexer_vox))
    indexer_vox.value() = -1;

  std::vector<Point<value_type> > fixel_positions;
  std::vector<value_type> test_statistic;
  int32_t num_fixels = 0;

  Image::BufferSparse<FixelMetric> input_data (argument[0]);
  Image::BufferSparse<FixelMetric>::voxel_type input_fixel (input_data);
  Image::Transform transform (input_fixel);
  Image::LoopInOrder loop (input_fixel);
  float max_stat = 0.0;
  for (loop.start (input_fixel, indexer_vox); loop.ok(); loop.next (input_fixel, indexer_vox)) {
    indexer_vox[3] = 0;
    indexer_vox.value() = num_fixels;
    size_t f = 0;
    for (; f != input_fixel.value().size(); ++f) {
      num_fixels++;
      if (input_fixel.value()[f].value > max_stat)
        max_stat = input_fixel.value()[f].value;
      test_statistic.push_back (input_fixel.value()[f].value);
      fixel_directions.push_back (input_fixel.value()[f].dir);
      fixel_positions.push_back (transform.voxel2scanner (input_fixel));
    }
    indexer_vox[3] = 1;
    indexer_vox.value() = f;
  }

  std::vector<std::map<int32_t, Stats::CFE::connectivity> > fixel_connectivity (num_fixels);
  std::vector<uint16_t> fixel_TDI (num_fixels, 0);

  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> track_file (argument[1], properties);
  const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
  if (!num_tracks)
    throw Exception ("no tracks found in input file");

  {
    typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;
    DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "pre-computing fixel-fixel connectivity...");
    DWI::Tractography::Mapping::TrackMapperBase mapper (index_header);
    mapper.set_upsample_ratio (DWI::Tractography::Mapping::determine_upsample_ratio (index_header, properties, 0.333f));
    mapper.set_use_precise_mapping (true);
    Stats::CFE::TrackProcessor tract_processor (indexer, fixel_directions, fixel_TDI, fixel_connectivity, angular_threshold );
    Thread::run_queue (loader, DWI::Tractography::Streamline<float>(), mapper, SetVoxelDir(), tract_processor);
  }


  // Normalise connectivity matrix and threshold, pre-compute fixel-fixel weights for smoothing.
  std::vector<std::map<int32_t, value_type> > fixel_smoothing_weights (num_fixels);
  bool do_smoothing = false;
  const value_type gaussian_const2 = 2.0 * smooth_std_dev * smooth_std_dev;
  value_type gaussian_const1 = 1.0;
  if (smooth_std_dev > 0.0) {
   do_smoothing = true;
   gaussian_const1 = 1.0 / (smooth_std_dev *  std::sqrt (2.0 * M_PI));
  }
  {
    ProgressBar progress ("normalising and thresholding fixel-fixel connectivity matrix...", num_fixels);
    for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
      auto it = fixel_connectivity[fixel].begin();
      while (it != fixel_connectivity[fixel].end()) {
       value_type connectivity = it->second.value / value_type (fixel_TDI[fixel]);
       if (connectivity < connectivity_threshold)  {
         fixel_connectivity[fixel].erase (it++);
       } else {
         if (do_smoothing) {
           value_type distance = std::sqrt (std::pow (fixel_positions[fixel][0] - fixel_positions[it->first][0], 2) +
                                            std::pow (fixel_positions[fixel][1] - fixel_positions[it->first][1], 2) +
                                            std::pow (fixel_positions[fixel][2] - fixel_positions[it->first][2], 2));
           value_type weight = connectivity * gaussian_const1 * std::exp (-std::pow (distance, 2) / gaussian_const2);
           if (weight > connectivity_threshold)
             fixel_smoothing_weights[fixel].insert (std::pair<int32_t, value_type> (it->first, weight));
         }
         it->second.value = std::pow (connectivity, cfe_C);
         ++it;
       }
      }
      // Make sure the fixel is fully connected to itself giving it a smoothing weight of 1
      Stats::CFE::connectivity self_connectivity;
      self_connectivity.value = 1.0;
      fixel_connectivity[fixel].insert (std::pair<int32_t, Stats::CFE::connectivity> (fixel, self_connectivity));
      fixel_smoothing_weights[fixel].insert (std::pair<int32_t, value_type> (fixel, gaussian_const1));
      progress++;
    }
  }

  std::vector<value_type> smoothed_test_statistic (num_fixels, 0.0);
  if (smooth_std_dev > 0.0) {
    max_stat = 0.0;
    // Normalise smoothing weights
    {
      ProgressBar progress ("normalising fixel smoothing weights",  num_fixels);
      for (int32_t i = 0; i < num_fixels; ++i) {
        progress++;
        value_type sum = 0.0;
        for (auto it = fixel_smoothing_weights[i].begin(); it != fixel_smoothing_weights[i].end(); ++it)
          sum += it->second;
        value_type norm_factor = 1.0 / sum;
        for (auto it = fixel_smoothing_weights[i].begin(); it != fixel_smoothing_weights[i].end(); ++it)
          it->second *= norm_factor;
      }
    }

    // Smooth the data based on connectivity

    {
      ProgressBar progress ("smoothing test statistic",  num_fixels);
      for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
        progress++;
        std::map<int32_t, value_type>::const_iterator it = fixel_smoothing_weights[fixel].begin();
        for (; it != fixel_smoothing_weights[fixel].end(); ++it)
          smoothed_test_statistic[fixel] += test_statistic[it->first] * it->second;
        if (smoothed_test_statistic[fixel] > max_stat)
          max_stat = smoothed_test_statistic[fixel];
      }
    }
  }

  // Write output
  Image::Header output_header (argument[0]);
  Image::BufferSparse<FixelMetric> output_data (argument[2], output_header);
  Image::BufferSparse<FixelMetric>::voxel_type output_fixel (output_data);


  std::vector<value_type> cfe_statistics;

  if (!get_options("smoothonly").size()) {
    MR::Stats::CFE::Enhancer cfe (fixel_connectivity, dh, cfe_E, cfe_H);
    CONSOLE("performing connectivity-based fixel enhancement...");
    if (smooth_std_dev > 0.0)
      cfe (max_stat, smoothed_test_statistic, cfe_statistics);
    else
      cfe (max_stat, test_statistic, cfe_statistics);
  } else {
    if (smooth_std_dev > 0.0)
      cfe_statistics = smoothed_test_statistic;
    else
      cfe_statistics = test_statistic;
  }

  for (loop.start (input_fixel, indexer_vox, output_fixel); loop.ok(); loop.next (input_fixel, indexer_vox, output_fixel)) {
    output_fixel.value().set_size (input_fixel.value().size());
    indexer_vox[3] = 0;
    int32_t index = indexer_vox.value();
    for (size_t f = 0; f != input_fixel.value().size(); ++f, ++index) {
      output_fixel.value()[f] = input_fixel.value()[f];
      output_fixel.value()[f].value = cfe_statistics[index];
    }
  }
}

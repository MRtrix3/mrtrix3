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
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/writer.h"
#include "thread_queue.h"
#include "stats/cfe.h"

#include <sys/stat.h>


using namespace MR;
using namespace App;


using Image::Sparse::FixelMetric;

void usage ()
{
  DESCRIPTION
  + "output the fixel connectivity maps for all fixels in a desired voxel";

    ARGUMENTS
    + Argument ("fixel_in", "the input fake signal fixel image.").type_image_in ()
    + Argument ("tracks", "the tractogram used to derive fixel-fixel connectivity").type_file_in ()
    + Argument ("output", "the output prefix").type_image_out ();

    OPTIONS
    + Option ("smooth", "output the smoothing kernel")
    + Argument ("fwhm", "in mm").type_float (0.0,10.0,1000.0);
}

#define ANGULAR_THRESHOLD 30

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


void write_fixel_output (const std::string filename,
                         const std::vector<value_type> data,
                         const Image::Header& header,
                         Image::BufferSparse<FixelMetric>::voxel_type& mask_vox,
                         Image::BufferScratch<int32_t>::voxel_type& indexer_vox) {
  Image::BufferSparse<FixelMetric> output_buffer (filename, header);
  auto output_voxel = output_buffer.voxel();
  Image::LoopInOrder loop (mask_vox);
  for (loop.start (mask_vox, indexer_vox, output_voxel); loop.ok(); loop.next (mask_vox, indexer_vox, output_voxel)) {
    output_voxel.value().set_size (mask_vox.value().size());
    indexer_vox[3] = 0;
    int32_t index = indexer_vox.value();
    for (size_t f = 0; f != mask_vox.value().size(); ++f, ++index) {
      output_voxel.value()[f] = mask_vox.value()[f];
      output_voxel.value()[f].value = data[index];
    }
  }
}





void run ()
{
  const float angular_threshold = 30;
  const float connectivity_threshold = 0.01;

    Options opt = get_options("smooth");
    float fwhm = opt.size() ? opt[0][0] : 0.0;

    std::vector<uint32_t> fixel_coord (4);
    fixel_coord[0] = 43;
    fixel_coord[1] = 62;
    fixel_coord[2] = 47;
    fixel_coord[3] = 0;

    // Segment the fixels
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
    std::vector<value_type> truth_statistic;
    int32_t num_fixels = 0;
    int32_t actual_positives = 0;

    Image::Header input_header (argument[0]);
    Image::BufferSparse<FixelMetric> input_data (input_header);
    Image::BufferSparse<FixelMetric>::voxel_type input_fixel (input_data);

    Image::Transform transform (input_fixel);
    Image::LoopInOrder loop (input_fixel);

    int32_t fixel_index = 0;

    for (loop.start (input_fixel, indexer_vox); loop.ok(); loop.next (input_fixel, indexer_vox)) {
      indexer_vox[3] = 0;
      indexer_vox.value() = num_fixels;
      size_t f = 0;

      if ( input_fixel[0] == fixel_coord[0] && input_fixel[1] == fixel_coord[1] && input_fixel[2] == fixel_coord[2]) {
        if (fixel_coord[3] >= input_fixel.value().size())
          throw Exception ("not enough fixels in voxel for index provided");
        fixel_index = num_fixels + fixel_coord[3];
      }

      for (; f != input_fixel.value().size(); ++f) {
        num_fixels++;
        if (input_fixel.value()[f].value >= 1.0)
          actual_positives++;
        truth_statistic.push_back (input_fixel.value()[f].value);
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
      mapper.set_upsample_ratio (DWI::Tractography::Mapping::determine_upsample_ratio (input_header, properties, 0.333f));
      mapper.set_use_precise_mapping (true);
      Stats::CFE::TrackProcessor tract_processor (indexer, fixel_directions, fixel_TDI, fixel_connectivity, angular_threshold );
      Thread::run_queue (loader, DWI::Tractography::Streamline<float>(), mapper, SetVoxelDir(), tract_processor);
    }

    // Normalise connectivity matrix and threshold
    {
      ProgressBar progress ("normalising and thresholding fixel-fixel connectivity matrix...", num_fixels);
      for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
        std::map<int32_t, Stats::CFE::connectivity>::iterator it = fixel_connectivity[fixel].begin();
        while (it != fixel_connectivity[fixel].end()) {
         value_type connectivity = it->second.value / value_type (fixel_TDI[fixel]);
         if (connectivity < connectivity_threshold)  {
           fixel_connectivity[fixel].erase (it++);
         } else {
           it->second.value = connectivity;
           ++it;
         }
        }
        // Make sure the fixel is fully connected to itself giving it a smoothing weight of 1
        Stats::CFE::connectivity self_connectivity;
        self_connectivity.value = 1.0;
        fixel_connectivity[fixel].insert (std::pair<int32_t, Stats::CFE::connectivity> (fixel, self_connectivity));
        progress++;
      }
    }


    std::vector<std::map<int32_t, Stats::CFE::connectivity> > weighted_fixel_connectivity (num_fixels);
      for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
      std::map<int32_t, Stats::CFE::connectivity>::iterator it = fixel_connectivity[fixel].begin();
      while (it != fixel_connectivity[fixel].end()) {
        Stats::CFE::connectivity weighted_connectivity;
        float C = 0.75;
        weighted_connectivity.value = std::pow (it->second.value , C);
        weighted_fixel_connectivity[fixel].insert (std::pair<int32_t, Stats::CFE::connectivity> (it->first, weighted_connectivity));
        ++it;
      }
    }

    if (fwhm > 0.0) {
      CONSOLE ("computing smoothing weights...");
      std::vector<std::map<int32_t, value_type> > fixel_smoothing_weights (num_fixels);
      float stdev = fwhm / 2.3548;
      const value_type gaussian_const2 = 2.0 * stdev * stdev;
      value_type gaussian_const1 = 1.0;

        gaussian_const1 = 1.0 / (stdev *  std::sqrt (2.0 * M_PI));
        for (int32_t f = 0; f < num_fixels; ++f) {
          std::map<int32_t, Stats::CFE::connectivity>::iterator it = fixel_connectivity[f].begin();
          while (it != fixel_connectivity[f].end()) {
            value_type connectivity = it->second.value;
            value_type distance = std::sqrt (Math::pow2 (fixel_positions[f][0] - fixel_positions[it->first][0]) +
                                              Math::pow2 (fixel_positions[f][1] - fixel_positions[it->first][1]) +
                                              Math::pow2 (fixel_positions[f][2] - fixel_positions[it->first][2]));
            value_type weight = connectivity * gaussian_const1 * std::exp (-Math::pow2 (distance) / gaussian_const2);
            if (weight > connectivity_threshold)
              fixel_smoothing_weights[f].insert (std::pair<int32_t, value_type> (it->first, weight));
             ++it;
           }
         }
      // add self smoothing weight
      for (int32_t f = 0; f < num_fixels; ++f)
        fixel_smoothing_weights[f].insert (std::pair<int32_t, value_type> (f, gaussian_const1));

      // Normalise smoothing weights
        for (int32_t i = 0; i < num_fixels; ++i) {
        value_type sum = 0.0;
        for (std::map<int32_t, value_type>::iterator it = fixel_smoothing_weights[i].begin(); it != fixel_smoothing_weights[i].end(); ++it)
          sum += it->second;
        value_type norm_factor = 1.5 / sum;
        for (std::map<int32_t, value_type>::iterator it = fixel_smoothing_weights[i].begin(); it != fixel_smoothing_weights[i].end(); ++it)
          it->second *= norm_factor;
      }

        std::vector<float> output_fixel_smoothing_weights (num_fixels, 0.0);
        for (std::map<int32_t, value_type>::iterator it = fixel_smoothing_weights[fixel_index].begin(); it != fixel_smoothing_weights[fixel_index].end(); ++it) {
          output_fixel_smoothing_weights[it->first] = it->second;
        }
        write_fixel_output(str(argument[2]), output_fixel_smoothing_weights, input_header, input_fixel, indexer_vox);

    } else {
      std::vector<float> fixel_connectivities (num_fixels, 0.0);
      std::map<int32_t, Stats::CFE::connectivity>::iterator it = weighted_fixel_connectivity[fixel_index].begin();
      while (it != weighted_fixel_connectivity[fixel_index].end()) {
        fixel_connectivities[it->first] = it->second.value;
        it++;
      }
      write_fixel_output(str(argument[2]), fixel_connectivities, input_header, input_fixel, indexer_vox);
    }


}


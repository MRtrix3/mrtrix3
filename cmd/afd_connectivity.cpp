/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 2013

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
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/fmls.h"
#include "thread/queue.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/loader.h"


using namespace MR;
using namespace MR::DWI;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "sum the Apparent Fibre Density (AFD) for all fixels belonging to a fibre bundle, "
    "and normalise by the mean track length."

  + "Use -quiet to suppress progress messages and output AFD value only."

  + "For valid comparisons of AFD connectivity across scans, images MUST be intensity "
    "normalised and bias field corrected."

  + "Note that the sum of the AFD is normalised by the mean track length to "
    "account for subject differences in fibre bundle length. This normalisation results in a measure "
    "that is more related to the cross-sectional volume of the tract (and therefore 'connectivity'). "
    "Note that SIFT-ed tract count is a superior measure because it is unaffected by tangental yet unrelated "
    "fibres. However, AFD connectivity can be used as a substitute when Anatomically Constrained Tractography and SIFT is not"
    "possible due to uncorrectable EPI distortions.";



  ARGUMENTS
  + Argument ("image", "the input FOD image.").type_image_in()

  + Argument ("tracks", "the input track file defining the bundle of interest.").type_file();

  OPTIONS
  + Option ("afd", "output a 3D image containing the AFD estimated for each voxel. "
                   "If the input tracks are tangent to multiple fibres in a voxel (fixels), "
                   "then the output AFD is the sum of the AFD for each fixel")
  + Argument ("image").type_image_out();

}


typedef float value_type;
typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;



/**
 * Process each track (represented as a set of dixels). For each track dixel, identify the closest FOD fixel
 */
class TrackProcessor {

  public:
    TrackProcessor (Image::Buffer<float>& fod_buf,
                    Image::BufferScratch<int32_t>& FOD_fixel_indexer,
                    std::vector<Point<value_type> >& FOD_fixel_directions,
                    std::vector<value_type>& fixel_AFD,
                    std::vector<int>& fixel_TDI,
                    DWI::FMLS::Segmenter& fmls):
                    fod_vox (fod_buf),
                    fixel_indexer (FOD_fixel_indexer),
                    fixel_directions (FOD_fixel_directions),
                    fixel_AFD (fixel_AFD),
                    fixel_TDI (fixel_TDI),
                    fmls (fmls) {}

    bool operator () (SetVoxelDir& dixels)
    {
      // For each fixel in the tract
      std::vector<int32_t> tract_fixel_indices;
      for (SetVoxelDir::const_iterator d = dixels.begin(); d != dixels.end(); ++d) {
        Image::Nav::set_pos (fixel_indexer, *d);
        fixel_indexer[3] = 0;
        int32_t first_index = fixel_indexer.value();

        // We have not yet visited this voxel so segment the FOD and store fixel details
        if (first_index < 0) {
          Image::Nav::set_pos (fod_vox, *d);
          DWI::FMLS::SH_coefs fod;
          DWI::FMLS::FOD_lobes lobes;
          fod.vox[0] = fod_vox[0]; fod.vox[1] = fod_vox[1]; fod.vox[2] = fod_vox[2];
          fod.allocate (fod_vox.dim (3));
          for (fod_vox[3] = 0; fod_vox[3] < fod_vox.dim (3); ++fod_vox[3])
            fod[fod_vox[3]] = fod_vox.value();
          fmls (fod, lobes);
          if (lobes.size() == 0)
            return true;
          fixel_indexer.value() = fixel_directions.size();
          first_index = fixel_directions.size();
          int32_t fixel_count = 0;
          for (DWI::FMLS::FOD_lobes::const_iterator l = lobes.begin(); l != lobes.end(); ++l, ++fixel_count) {
            fixel_directions.push_back (l->get_peak_dir());
            fixel_AFD.push_back (l->get_integral());
            fixel_TDI.push_back (0);
          }
          fixel_indexer[3] = 1;
          fixel_indexer.value() = fixel_count;
        }

        fixel_indexer[3] = 1;
        int32_t last_index = first_index + fixel_indexer.value();
        int32_t closest_fixel_index = -1;
        value_type largest_dp = -1.0;
        Point<value_type> dir (d->get_dir());
        dir.normalise();

        // find the fixel with the closest direction to track tangent
        for (int32_t j = first_index; j < last_index; ++j) {
          value_type dp = Math::abs (dir.dot (fixel_directions[j]));
          if (dp >= largest_dp) {
            largest_dp = dp;
            closest_fixel_index = j;
          }
        }
        fixel_TDI[closest_fixel_index]++;
      }

      return true;
    }

  private:
    Image::Buffer<float>::voxel_type fod_vox;
    Image::BufferScratch<int32_t>::voxel_type fixel_indexer;
    std::vector<Point<value_type> >& fixel_directions;
    std::vector<value_type>& fixel_AFD;
    std::vector<int>& fixel_TDI;
    DWI::FMLS::Segmenter& fmls;
};



void run ()
{

  Tractography::Properties properties;
  Tractography::Reader<value_type> track_file (argument[1], properties);

  const size_t track_count = properties["count"].empty() ? 0 : to<int> (properties["count"]);
  if (track_count == 0)
    throw Exception ("no tracks found in the input track file");

  const float step_size = properties["step_size"].empty() ? 0 : to<float> (properties["step_size"]);
  if (step_size == 0.0)
    throw Exception ("track file step size is equal to zero");

  std::vector<Point<value_type> > fixel_directions;
  std::vector<value_type> fixel_AFD;
  DWI::Directions::Set dirs (1281);
  Image::Header index_header (argument[0]);
  index_header.dim(3) = 2;
  Image::BufferScratch<int32_t> fixel_indexer (index_header);
  Image::BufferScratch<int32_t>::voxel_type fixel_indexer_vox (fixel_indexer);
  Image::LoopInOrder loop4D (fixel_indexer_vox);
  for (loop4D.start (fixel_indexer_vox); loop4D.ok(); loop4D.next (fixel_indexer_vox))
    fixel_indexer_vox.value() = -1;

  DWI::Tractography::Mapping::TrackLoader loader (track_file, track_count, "summing apparent fibre density within track...");
  Image::Header header (argument[0]);
  DWI::Tractography::Mapping::TrackMapperBase<SetVoxelDir> mapper (header);
  std::vector<int> fixel_TDI;
  Image::Buffer<value_type> fod_buffer (argument[0]);
  DWI::FMLS::Segmenter fmls (dirs, Math::SH::LforN (fod_buffer.dim(3)));
  TrackProcessor tract_processor (fod_buffer, fixel_indexer, fixel_directions, fixel_AFD, fixel_TDI, fmls);
  Thread::run_queue_custom_threading (loader, 1, DWI::Tractography::TrackData<float>(), mapper, 1, SetVoxelDir(), tract_processor, 1);

  double total_AFD = 0.0;
  Image::Loop loop (0, 3);

  for (loop.start (fixel_indexer_vox); loop.ok(); loop.next (fixel_indexer_vox)) {
    fixel_indexer_vox[3] = 0;
    int32_t fixel_index = fixel_indexer_vox.value();
    if (fixel_index >= 0) {
      fixel_indexer_vox[3] = 1;
      int32_t last_index = fixel_index + fixel_indexer_vox.value();
      int32_t largest_TDI_index = std::numeric_limits<int32_t>::max();
      int largest_TDI = 0;
      for (;fixel_index < last_index; ++fixel_index) {
        if (fixel_TDI[fixel_index] > largest_TDI) {
          largest_TDI = fixel_TDI[fixel_index];
          largest_TDI_index = fixel_index;
        }
      }
      if (largest_TDI_index != std::numeric_limits<int32_t>::max())
        total_AFD += fixel_AFD [largest_TDI_index];
    }
  }

  Tractography::Reader<value_type> tck_file (argument[1], properties);
  std::vector<Point<value_type> > tck;
  double total_track_length = 0.0;
  {
    ProgressBar progress ("normalising apparent fibre density by mean track length...", track_count);
    while (tck_file.next (tck)) {
      total_track_length += static_cast<value_type>(tck.size() - 1) * step_size;
      progress++;
    }
  }

  // output the AFD sum using std::cout. This enables output to be redirected to a file without the console output.
  std::cout << total_AFD / (total_track_length / static_cast<value_type>(track_count)) << std::endl;

  header.set_ndim(3);

  Options opt = get_options ("afd");
  if (opt.size()) {
    Image::Buffer<value_type> AFD_buf (opt[0][0], header);
    Image::Buffer<value_type>::voxel_type AFD_vox (AFD_buf);
    for (loop.start (fixel_indexer_vox, AFD_vox); loop.ok(); loop.next (fixel_indexer_vox, AFD_vox)) {
        fixel_indexer_vox[3] = 0;
        int32_t fixel_index = fixel_indexer_vox.value();
        if (fixel_index >= 0) {
          fixel_indexer_vox[3] = 1;
          int32_t last_index = fixel_index + fixel_indexer_vox.value();
          int32_t largest_TDI_index = std::numeric_limits<int32_t>::max();
          int largest_TDI = 0;
          for (;fixel_index < last_index; ++fixel_index) {
            if (fixel_TDI[fixel_index] > largest_TDI) {
              largest_TDI = fixel_TDI[fixel_index];
              largest_TDI_index = fixel_index;
            }
          }
          if (largest_TDI_index != std::numeric_limits<int32_t>::max())
            AFD_vox.value() += fixel_AFD [largest_TDI_index];
        }
      }
  }
}

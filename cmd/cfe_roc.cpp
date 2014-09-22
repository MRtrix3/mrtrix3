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
#include "stats/tfce.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/writer.h"
#include "thread/queue.h"

#include <sys/stat.h>


using namespace MR;
using namespace App;


using Image::Sparse::FixelMetric;



void usage ()
{

  DESCRIPTION
  + "perform connectivity-based fixel enhancement ROC experiments";

  ARGUMENTS
  + Argument ("fixel_in", "the input fake signal fixel image.").type_image_in ()

  + Argument ("tracks", "the tractogram used to derive fixel-fixel connectivity").type_file ()

  + Argument ("output", "the output prefix").type_file();

  OPTIONS
  + Option ("snr", "the snr of the test statistic")
  + Argument ("value").type_float (1.0, 1.0, 100).type_sequence_float()

  + Option ("smooth", "the smoothing applied to the test statistic")
  + Argument ("fwhm").type_float (1.0, 1.0, 100).type_sequence_float()

  + Option ("extent", "the extent weight")
  + Argument ("E").type_float (1.0, 1.0, 100).type_sequence_float()

  + Option ("height", "the height weight")
  + Argument ("H").type_float (1.0, 1.0, 100).type_sequence_float()

  + Option ("connectivity", "the connectivity weight")
  + Argument ("C").type_float (1.0, 1.0, 100).type_sequence_float()

  + Option ("realisations", "the number of noise realisations")
  + Argument ("num").type_integer(1, 1000, 10000)

  + Option ("roc", "the number of thresholds for ROC curve generation")
  + Argument ("num").type_integer (1, 1000, 10000);


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
  Image::BufferSparse<FixelMetric>::voxel_type output_voxel (output_buffer);
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


/**
 * Process each track (represented as a set of fixels). For each track tangent dixel, identify the closest fixel, build connectivity matrix
 */
class TrackProcessor {

  public:
    TrackProcessor (Image::BufferScratch<int32_t>& FOD_fixel_indexer,
                    const std::vector<Point<value_type> >& FOD_fixel_directions,
                    std::vector<uint16_t>& fixel_TDI,
                    std::vector<std::map<int32_t, Stats::TFCE::connectivity> >& fixel_connectivity,
                    value_type angular_threshold):
                    fixel_indexer (FOD_fixel_indexer) ,
                    fixel_directions (FOD_fixel_directions),
                    fixel_TDI (fixel_TDI),
                    fixel_connectivity (fixel_connectivity) {
      angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));
    }

    bool operator () (MR::DWI::Tractography::Mapping::SetVoxelDir& in)
    {
      // For each voxel tract tangent, assign to a fixel
      std::vector<int32_t> tract_fixel_indices;
      for (MR::DWI::Tractography::Mapping::SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
        Image::Nav::set_pos (fixel_indexer, *i);
        fixel_indexer[3] = 0;
        int32_t first_index = fixel_indexer.value();
        if (first_index >= 0) {
          fixel_indexer[3] = 1;
          int32_t last_index = first_index + fixel_indexer.value();
          int32_t closest_fixel_index = -1;
          value_type largest_dp = 0.0;
          Point<value_type> dir (i->get_dir());
          dir.normalise();
          for (int32_t j = first_index; j < last_index; ++j) {
            value_type dp = Math::abs (dir.dot (fixel_directions[j]));
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

      for (size_t i = 0; i < tract_fixel_indices.size(); i++) {
        for (size_t j = i + 1; j < tract_fixel_indices.size(); j++) {
          fixel_connectivity[tract_fixel_indices[i]][tract_fixel_indices[j]].value++;
          fixel_connectivity[tract_fixel_indices[j]][tract_fixel_indices[i]].value++;
        }
      }
      return true;
    }

  private:
    Image::BufferScratch<int32_t>::voxel_type fixel_indexer;
    const std::vector<Point<value_type> >& fixel_directions;
    std::vector<uint16_t>& fixel_TDI;
    std::vector<std::map<int32_t, Stats::TFCE::connectivity> >& fixel_connectivity;
    value_type angular_threshold_dp;
};



class Stack {
  public:
  Stack (size_t num_noise_realisation) :
    num_noise_realisation (num_noise_realisation),
      progress ("running " + str(num_noise_realisation) + " noise realisations...", num_noise_realisation),
      index (0) {}

    size_t next () {
      Thread::Mutex::Lock lock (permutation_mutex);
      if (index < num_noise_realisation)
        ++progress;
      return index++;
    }

    const size_t num_noise_realisation;

  protected:
    ProgressBar progress;
    size_t index;
    Thread::Mutex permutation_mutex;
};



class Processor {
  public:
    Processor (Stack& stack,
               const int32_t actual_positives,
               const int32_t num_ROC_samples,
               const std::vector<value_type>& truth_statistic,
               const std::vector<std::map<int32_t, Stats::TFCE::connectivity> >& fixel_connectivity,
               std::vector<std::vector<uint32_t> >& global_TPRates,
               std::vector<int32_t>& global_num_noise_instances_with_a_false_positive,
               const value_type dh,
               const value_type e,
               const value_type h,
               const std::vector<std::vector<value_type> >& smoothed_test_statistic,
               const std::vector<std::vector<value_type> >& smoothed_noise,
               const std::vector<value_type>& max_statistics,
               Image::Header& input_header,
               Image::BufferSparse<FixelMetric>::voxel_type& template_vox,
               const Image::BufferSparse<FixelMetric>& template_data,
               Image::BufferScratch<int32_t>::voxel_type& indexer_vox,
               const Image::BufferScratch<int32_t>& indexer_data):
                 stack (stack),
                 actual_positives (actual_positives),
                 num_ROC_samples (num_ROC_samples),
                 truth_statistic (truth_statistic),
                 fixel_smoothing_weights (fixel_smoothing_weights),
                 fixel_connectivity (fixel_connectivity),
                 global_TPRates (global_TPRates),
                 global_num_noise_instances_with_a_false_positive (global_num_noise_instances_with_a_false_positive),
                 thread_TPRates (num_ROC_samples, 0.0),
                 thread_num_noise_instances_with_a_false_positive (num_ROC_samples, 0),
                 smoothed_test_statistic (smoothed_test_statistic),
                 smoothed_noise (smoothed_noise),
                 max_statistics (max_statistics),
                 cfe (fixel_connectivity, dh, e, h),
                 input_header (input_header),
                 template_vox (template_vox),
                 template_data (template_data),
                 indexer_vox (indexer_vox),
                 indexer_data (indexer_data){

    }

    ~Processor () {
      for (size_t t = 0; t < num_ROC_samples; ++t)
        global_num_noise_instances_with_a_false_positive[t] += thread_num_noise_instances_with_a_false_positive[t];
    }


    void execute () {
      size_t index = 0;
      while (( index = stack.next() ) < stack.num_noise_realisation)
        process_noise_realisation (index);
    }

  private:


    void write_fixel_output (const std::string filename,
                             const std::vector<value_type>& data) {
      Image::BufferSparse<FixelMetric> output_buffer (filename, input_header);
      Image::BufferSparse<FixelMetric>::voxel_type output_voxel (output_buffer);
      Image::LoopInOrder loop (template_vox);
      Image::check_dimensions (output_voxel, template_vox);
      for (loop.start (template_vox, indexer_vox, output_voxel); loop.ok(); loop.next (template_vox, indexer_vox, output_voxel)) {
        output_voxel.value().set_size (template_vox.value().size());
        indexer_vox[3] = 0;
        int32_t index = indexer_vox.value();
        for (size_t f = 0; f != template_vox.value().size(); ++f, ++index) {
         output_voxel.value()[f] = template_vox.value()[f];
         output_voxel.value()[f].value = data[index];
        }
      }
    }


    void process_noise_realisation (size_t index) {

      std::vector<value_type> cfe_test_statistic;
      std::vector<value_type> cfe_noise;

      value_type max_cfe_statistic = cfe (max_statistics[index], smoothed_test_statistic[index], &cfe_test_statistic);
      cfe (max_statistics[index], smoothed_noise[index], &cfe_noise);

      for (size_t t = 0; t < num_ROC_samples; ++t) {
        value_type threshold = ((value_type) t / ((value_type) num_ROC_samples - 1.0)) * max_cfe_statistic;
        bool contains_false_positive = false;
        for (uint32_t f = 0; f < truth_statistic.size(); ++f) {
          if (truth_statistic[f] >= 1.0) {
            if (cfe_test_statistic[f] > threshold)
              global_TPRates [t][index]++;
          }
          if (cfe_noise[f] > threshold)
             contains_false_positive = true;
        }

        if (contains_false_positive)
          thread_num_noise_instances_with_a_false_positive[t]++;

      }
    }



    Stack& stack;
    const int32_t actual_positives;
    const size_t num_ROC_samples;
    const std::vector<value_type>& truth_statistic;
    const std::vector<std::map<int32_t, value_type> >& fixel_smoothing_weights;
    const std::vector<std::map<int32_t, Stats::TFCE::connectivity> >& fixel_connectivity;
    std::vector<std::vector<uint32_t> >& global_TPRates;
    std::vector<int32_t>& global_num_noise_instances_with_a_false_positive;
    std::vector<value_type> thread_TPRates;
    std::vector<int32_t> thread_num_noise_instances_with_a_false_positive;
    const std::vector<std::vector<value_type> >& smoothed_test_statistic;
    const std::vector<std::vector<value_type> >& smoothed_noise;
    const std::vector<value_type>& max_statistics;
    MR::Stats::TFCE::Connectivity cfe;
    Image::Header input_header;
    Image::BufferSparse<FixelMetric>::voxel_type template_vox;
    const Image::BufferSparse<FixelMetric>& template_data;
    Image::BufferScratch<int32_t>::voxel_type indexer_vox;
    const Image::BufferScratch<int32_t>& indexer_data;
};


void run ()
{
  const value_type angular_threshold = 30;
  const value_type dh = 0.1;
  const value_type connectivity_threshold = 0.01;


  Options opt = get_options ("roc");
  size_t num_ROC_samples = 1000;
  if (opt.size())
    num_ROC_samples = opt[0][0];

  size_t num_noise_realisations = 1000;
  opt = get_options("realisations");
  if (opt.size())
    num_noise_realisations = opt[0][0];


  std::vector<value_type> SNR (1);
  SNR[0] = 1.0;
  opt = get_options("snr");
  if (opt.size())
    SNR = opt[0][0];

  std::vector<value_type> H (1);
  H[0] = 2.0;
  opt = get_options("height");
  if (opt.size())
    H = opt[0][0];

  std::vector<value_type> E (1);
  E[0] = 1.0;
  opt = get_options("extent");
  if (opt.size())
    E = opt[0][0];

  std::vector<value_type> C (1);
  C[0] = 0.5;
  opt = get_options("connectivity");
  if (opt.size())
    C = opt[0][0];

  std::vector<value_type> smooth (1);
  smooth[0] = 10.0;
  opt = get_options("smooth");
  if (opt.size())
    smooth = opt[0][0];


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


  for (loop.start (input_fixel, indexer_vox); loop.ok(); loop.next (input_fixel, indexer_vox)) {
    indexer_vox[3] = 0;
    indexer_vox.value() = num_fixels;
    size_t f = 0;
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

  std::vector<std::map<int32_t, Stats::TFCE::connectivity> > fixel_connectivity (num_fixels);
  std::vector<uint16_t> fixel_TDI (num_fixels, 0);

  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> track_file (argument[1], properties);
  const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
  if (!num_tracks)
    throw Exception ("no tracks found in input file");

  {
    typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;
    DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "pre-computing fixel-fixel connectivity...");
    DWI::Tractography::Mapping::TrackMapperBase<SetVoxelDir> mapper (index_header);
    TrackProcessor tract_processor (indexer, fixel_directions, fixel_TDI, fixel_connectivity, angular_threshold );
    Thread::run_queue (loader, DWI::Tractography::Streamline<float>(), mapper, SetVoxelDir(), tract_processor);
  }

  // Normalise connectivity matrix and threshold
  {
    ProgressBar progress ("normalising and thresholding fixel-fixel connectivity matrix...", num_fixels);
    for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
      std::map<int32_t, Stats::TFCE::connectivity>::iterator it = fixel_connectivity[fixel].begin();
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
      Stats::TFCE::connectivity self_connectivity;
      self_connectivity.value = 1.0;
      fixel_connectivity[fixel].insert (std::pair<int32_t, Stats::TFCE::connectivity> (fixel, self_connectivity));
      progress++;
    }
  }


  for (size_t s = 0; s < smooth.size(); ++s) {

    CONSOLE ("computing smoothing weights...");
    std::vector<std::map<int32_t, value_type> > fixel_smoothing_weights (num_fixels);
    float stdev = smooth[s] / 2.3548;
    const value_type gaussian_const2 = 2.0 * stdev * stdev;
    value_type gaussian_const1 = 1.0;
    if (smooth[s] > 0.0) {
      gaussian_const1 = 1.0 / (stdev *  Math::sqrt (2.0 * M_PI));
      for (int32_t f = 0; f < num_fixels; ++f) {
        std::map<int32_t, Stats::TFCE::connectivity>::iterator it = fixel_connectivity[f].begin();
        while (it != fixel_connectivity[f].end()) {
          value_type connectivity = it->second.value;
          value_type distance = Math::sqrt (Math::pow2 (fixel_positions[f][0] - fixel_positions[it->first][0]) +
                                            Math::pow2 (fixel_positions[f][1] - fixel_positions[it->first][1]) +
                                            Math::pow2 (fixel_positions[f][2] - fixel_positions[it->first][2]));
          value_type weight = connectivity * gaussian_const1 * Math::exp (-Math::pow2 (distance) / gaussian_const2);
          if (weight > connectivity_threshold)
            fixel_smoothing_weights[f].insert (std::pair<int32_t, value_type> (it->first, weight));
           ++it;
         }
       }
    }
    // add self smoothing weight
    for (int32_t f = 0; f < num_fixels; ++f)
      fixel_smoothing_weights[f].insert (std::pair<int32_t, value_type> (f, gaussian_const1));

    // Normalise smoothing weights
    if (smooth[s] > 0.0) {
      for (int32_t i = 0; i < num_fixels; ++i) {
        value_type sum = 0.0;
        for (std::map<int32_t, value_type>::iterator it = fixel_smoothing_weights[i].begin(); it != fixel_smoothing_weights[i].end(); ++it)
          sum += it->second;
        value_type norm_factor = 1.0 / sum;
        for (std::map<int32_t, value_type>::iterator it = fixel_smoothing_weights[i].begin(); it != fixel_smoothing_weights[i].end(); ++it)
          it->second *= norm_factor;
      }
    }


    for (size_t snr = 0; snr < SNR.size(); ++snr) {

      std::vector<std::vector<value_type> > smoothed_test_statistic (num_noise_realisations, std::vector<value_type> (num_fixels, 0.0));
      std::vector<std::vector<value_type> > smoothed_noise (num_noise_realisations, std::vector<value_type> (num_fixels, 0.0));
      std::vector<value_type> max_statistics (num_noise_realisations, 0.0);

      {
        ProgressBar progress ("generating noise realisations", num_noise_realisations);
        for (size_t r = 0; r < num_noise_realisations; ++r) {

          std::vector<value_type> noisy_test_statistic (num_fixels, 0.0);
          std::vector<value_type> noise_only (num_fixels, 0.0);

          // Add noise
          Math::RNG rng;
          for (int32_t f = 0; f < num_fixels; ++f) {
            value_type the_noise = rng.normal();
            noisy_test_statistic[f] = truth_statistic[f] * SNR[snr] + the_noise;
            noise_only[f] = the_noise;
          }

          // Smooth
          double sum_squares = 0.0;
          for (int32_t f = 0; f < num_fixels; ++f) {
            std::map<int32_t, value_type>::const_iterator it = fixel_smoothing_weights[f].begin();
            for (; it != fixel_smoothing_weights[f].end(); ++it) {
              smoothed_test_statistic[r][f] += noisy_test_statistic[it->first] * it->second;
              smoothed_noise[r][f] += noise_only[it->first] * it->second;
            }
            sum_squares += Math::pow2 (smoothed_noise[r][f]);
          }

          // Normalise so noise std is 1.0 after smoothing
          double scale_factor = 1.0 / Math::sqrt(sum_squares / (double)num_fixels);
          for (int32_t f = 0; f < num_fixels; ++f) {
            smoothed_test_statistic[r][f] *= scale_factor;
            smoothed_noise[r][f] *= scale_factor;
            if (smoothed_test_statistic[r][f] > max_statistics[r])
              max_statistics[r] = smoothed_test_statistic[r][f];
          }
          progress++;
        }
      }


      for (size_t c = 0; c < C.size(); ++c) {

        // Here we pre-exponentiate each connectivity value to speed up the CFE
        std::vector<std::map<int32_t, Stats::TFCE::connectivity> > weighted_fixel_connectivity (num_fixels);
        for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
          std::map<int32_t, Stats::TFCE::connectivity>::iterator it = fixel_connectivity[fixel].begin();
          while (it != fixel_connectivity[fixel].end()) {
            Stats::TFCE::connectivity weighted_connectivity;
            weighted_connectivity.value = Math::pow (it->second.value , C[c]);
            weighted_fixel_connectivity[fixel].insert (std::pair<int32_t, Stats::TFCE::connectivity> (it->first, weighted_connectivity));
            ++it;
          }
        }

        for (size_t h = 0; h < H.size(); ++h) {
          for (size_t e = 0; e < E.size(); ++e) {
            CONSOLE ("starting test: smoothing = " + str(smooth[s]) + ", snr = " + str(SNR[snr]) + ", h = " + str(H[h]) + ", e = " + str(E[e]) + ", c = " + str(C[c]));

            MR::Timer timer;

            std::string filename (argument[2]);
            filename.append ("_s" + str(smooth[s]) + "_snr" + str(SNR[snr]) + "_h" + str(H[h]) + "_e" + str(E[e]) + "_c" + str (C[c]));

            if (Path::exists (filename)) {
              CONSOLE ("Already done!");
            } else {
              std::vector<std::vector<uint32_t> > TPRates (num_ROC_samples, std::vector<uint32_t> (num_noise_realisations, 0));
              std::vector<int32_t> num_noise_instances_with_a_false_positive (num_ROC_samples, 0);

              {
                Stack stack (num_noise_realisations);
                Processor processor (stack, actual_positives, num_ROC_samples, truth_statistic,
                                     weighted_fixel_connectivity, TPRates, num_noise_instances_with_a_false_positive, dh, E[e], H[h],
                                     smoothed_test_statistic, smoothed_noise, max_statistics,
                                     input_header, input_fixel, input_data, indexer_vox, indexer);
                Thread::Array< Processor > thread_list (processor);
                Thread::Exec threads (thread_list, "threads");
              }

              // output all noise instance TPR values for variance calculations
              std::string filename_all_TPR (filename);
              filename_all_TPR.append("_all_tpr");

              std::ofstream output_all;
              output_all.open (filename_all_TPR.c_str());
              for (size_t t = 0; t < num_ROC_samples; ++t) {
                for (size_t p = 0; p < num_noise_realisations; ++p) {
                  output_all << (value_type) TPRates [t][p] / (value_type) actual_positives << " ";
                }
                output_all << std::endl;
              }
              output_all.close();

              std::ofstream output;
              output.open (filename.c_str());
              for (size_t t = 0; t < num_ROC_samples; ++t) {
                // average TPR across all realisations
                u_int32_t sum = 0.0;
                for (size_t p = 0; p < num_noise_realisations; ++p) {
                  sum += TPRates [t][p];
                }
                output << (value_type) sum / ((value_type) actual_positives * (value_type) num_noise_realisations)   << " ";
                // FPR is defined as the fraction of realisations with a false positive
                output << (value_type) num_noise_instances_with_a_false_positive[t] / (value_type) num_noise_realisations << std::endl;
              }
              output.close();
            }
            std::cout << "Minutes: " << timer.elapsed() / 60.0 << std::endl;
          }
        }
      }
    }
  }

}


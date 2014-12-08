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
#include "stats/cfe.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/writer.h"
#include "math/stats/permutation.h"
#include "math/stats/glm.h"
#include "thread_queue.h"


#include <sys/stat.h>


using namespace MR;
using namespace App;


using Image::Sparse::FixelMetric;



void usage ()
{

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "perform connectivity-based fixel enhancement ROC experiments";

  ARGUMENTS
  + Argument ("input", "a text file listing the file names of the input fixel images").type_file_in ()

  + Argument ("fixel_in", "the template fixel image including the fake pathology ROI.").type_image_in ()

  + Argument ("tracks", "the tractogram used to derive fixel-fixel connectivity").type_file_in ()

  + Argument ("design", "the design matrix").type_file_in()

  + Argument ("contrast", "the contrast matrix").type_file_in()

  + Argument ("permutations", "the set of indices for all permutations").type_file_in()

  + Argument ("tpr", "the output tpr prefix").type_text()

  + Argument ("fpr", "the output fpr prefix").type_text();

  OPTIONS
  + Option ("effect", "the percentage decrease applied to simulate pathology")
  + Argument ("value").type_float (0.0, 1.0, 100).type_sequence_float()

  + Option ("smooth", "the smoothing applied to the test statistic")
  + Argument ("fwhm").type_float (0.0, 1.0, 100).type_sequence_float()

  + Option ("extent", "the extent weight")
  + Argument ("E").type_float (1.0, 1.0, 100).type_sequence_float()

  + Option ("height", "the height weight")
  + Argument ("H").type_float (1.0, 1.0, 100).type_sequence_float()

  + Option ("connectivity", "the connectivity weight")
  + Argument ("C").type_float (1.0, 1.0, 100).type_sequence_float()

  + Option ("roc", "the number of thresholds for ROC curve generation")
  + Argument ("num").type_integer (1, 1000, 10000);


}


#define ANGULAR_THRESHOLD 30

typedef float value_type;


template <class VectorType>
void write_fixel_output (const std::string& filename,
                         const VectorType data,
                         const Image::Header& header,
                         Image::BufferSparse<FixelMetric>::voxel_type& mask_vox,
                         Image::BufferScratch<int32_t>::voxel_type& indexer_vox) {
  Image::BufferSparse<FixelMetric> output (filename, header);
  auto output_voxel = output.voxel();
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

class PermutationStack {
  public:
    PermutationStack (const Math::Matrix<float>& permutations_matrix) :
      num_permutations (permutations_matrix.rows()),
      current_permutation (0),
      progress ("running " + str(permutations_matrix.rows()) + " permutations...", permutations_matrix.rows()),
      permutations (permutations_matrix.rows())
    {
      for (size_t p = 0; p < permutations_matrix.rows(); ++p){
        permutations[p].resize(permutations_matrix.columns());
        for (size_t c = 0; c < permutations_matrix.columns(); ++c)
          permutations[p][c] = static_cast<size_t>(permutations_matrix(p,c));
      }
    }

    size_t next () {
      std::lock_guard<std::mutex> lock (permutation_mutex);
      size_t index = current_permutation++;
      if (index < permutations.size())
        ++progress;
      return index;
    }
    const std::vector<size_t>& permutation (size_t index) const {
      return permutations[index];
    }

    const size_t num_permutations;

  protected:
    size_t current_permutation;
    ProgressBar progress;
    std::vector <std::vector<size_t> > permutations;
    std::mutex permutation_mutex;
};


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


class Stack {
  public:
  Stack (size_t num_noise_realisation) :
    num_noise_realisation (num_noise_realisation),
      progress ("running " + str(num_noise_realisation) + " noise realisations...", num_noise_realisation),
      index (0) {}

    size_t next () {
      std::lock_guard<std::mutex> lock (permutation_mutex);
      if (index < num_noise_realisation)
        ++progress;
      return index++;
    }

    const size_t num_noise_realisation;

  protected:
    ProgressBar progress;
    size_t index;
    std::mutex permutation_mutex;
};


class ROCThresholdKernel {
  public:
    ROCThresholdKernel (Stack& perm_stack,
                        const std::vector<std::vector<value_type> > & control_cfe_statistics,
                        const std::vector<std::vector<value_type> > & path_cfe_statistics,
                        const std::vector<value_type>& ROC_thresholds,
                        const std::vector<value_type>& truth_statistic,
                        std::vector<std::vector<uint32_t> >& global_TPRates,
                        std::vector<int32_t>& global_num_permutations_with_a_false_positive):
                         perm_stack (perm_stack),
                         control_cfe_statistics (control_cfe_statistics),
                         path_cfe_statistics (path_cfe_statistics),
                         ROC_thresholds (ROC_thresholds),
                         truth_statistic (truth_statistic),
                         global_TPRates (global_TPRates),
                         global_num_permutations_with_a_false_positive (global_num_permutations_with_a_false_positive),
                         thread_num_permutations_with_a_false_positive (ROC_thresholds.size(), 0) {
    }

    ~ROCThresholdKernel () {
      for (size_t t = 0; t < ROC_thresholds.size(); ++t)
        global_num_permutations_with_a_false_positive[t] += thread_num_permutations_with_a_false_positive[t];
    }

    void execute () {
      size_t index = 0;
      while (( index = perm_stack.next() ) < perm_stack.num_noise_realisation)
        process_permutation (index);
    }

  private:
    void process_permutation (int perm) {
      for (size_t t = 0; t < ROC_thresholds.size(); ++t) {
        bool contains_false_positive = false;
        for (uint32_t f = 0; f < truth_statistic.size(); ++f) {
          if (truth_statistic[f] >= 1.0) {
            if (path_cfe_statistics[perm][f] > ROC_thresholds[t])
              global_TPRates[t][perm]++;
          }
          if (control_cfe_statistics[perm][f] > ROC_thresholds[t])
             contains_false_positive = true;
        }
        if (contains_false_positive)
          thread_num_permutations_with_a_false_positive[t]++;
      }
    }

    Stack& perm_stack;
    const std::vector<std::vector<value_type> >& control_cfe_statistics;
    const std::vector<std::vector<value_type> >& path_cfe_statistics;
    const std::vector<value_type>& ROC_thresholds;
    const std::vector<value_type>& truth_statistic;
    std::vector<std::vector<uint32_t> >& global_TPRates;
    std::vector<int32_t>& global_num_permutations_with_a_false_positive;
    std::vector<int32_t> thread_num_permutations_with_a_false_positive;
};





class EnhancerKernel {
  public:
    EnhancerKernel (Stack& perm_stack,
                    const std::vector<std::vector<value_type> >& control_test_statistics,
                    const std::vector<std::vector<value_type> >& path_test_statistics,
                    const std::vector<value_type>& max_statistics,
                    const MR::Stats::CFE::Enhancer& cfe,
                    std::vector<value_type>& max_cfe_statistics,
                    std::vector<std::vector<value_type> > & control_cfe_statistics,
                    std::vector<std::vector<value_type> > & path_cfe_statistics):
                      perm_stack (perm_stack),
                      control_test_statistics (control_test_statistics),
                      path_test_statistics (path_test_statistics),
                      max_statistics (max_statistics),
                      cfe (cfe),
                      max_cfe_statistics (max_cfe_statistics),
                      control_cfe_statistics (control_cfe_statistics),
                      path_cfe_statistics (path_cfe_statistics) {
    }

    void execute () {
      size_t index = 0;
      while (( index = perm_stack.next() ) < perm_stack.num_noise_realisation)
        process_permutation (index);
    }

  private:
    void process_permutation (int perm) {
      max_cfe_statistics[perm] = cfe (max_statistics[perm], path_test_statistics[perm], path_cfe_statistics[perm]);
      cfe (max_statistics[perm], control_test_statistics[perm], control_cfe_statistics[perm]);
    }

    Stack& perm_stack;
    const std::vector<std::vector<value_type> >& control_test_statistics;
    const std::vector<std::vector<value_type> >& path_test_statistics;
    const std::vector<value_type>& max_statistics;
    const MR::Stats::CFE::Enhancer cfe;
    std::vector<value_type>& max_cfe_statistics;
    std::vector<std::vector<value_type> > & control_cfe_statistics;
    std::vector<std::vector<value_type> > & path_cfe_statistics;
};




void run ()
{
  const value_type angular_threshold_dp = cos (ANGULAR_THRESHOLD * (M_PI/180.0));
  const value_type dh = 0.1;
  const value_type connectivity_threshold = 0.01;

  Options opt = get_options("roc");
  size_t num_ROC_samples = 2000;
  if (opt.size())
    num_ROC_samples = opt[0][0];

  std::vector<value_type> effect(1);
  effect[0] = 0.2;
  opt = get_options("effect");
  if (opt.size())
    effect = opt[0][0];

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

  // Read filenames
  std::vector<std::string> filenames;
  {
    std::string folder = Path::dirname (argument[0]);
    std::ifstream ifs (argument[0].c_str());
    std::string temp;
    while (getline (ifs, temp))
      filenames.push_back (Path::join (folder, temp));
  }
  const size_t num_subjects = filenames.size();

  // Load design matrix:
  Math::Matrix<value_type> design;
  design.load (argument[3]);
  if (design.rows() != filenames.size())
    throw Exception ("number of subjects does not match number of rows in design matrix");

  // Load contrast matrix:
  Math::Matrix<value_type> contrast;
  contrast.load (argument[4]);

  // Load permutation matrix:
  Math::Matrix<value_type> permutations;
  permutations.load (argument[5]);

  size_t num_permutations = permutations.rows();

  if (contrast.columns() > design.columns())
    throw Exception ("too many contrasts for design matrix");
  contrast.resize (contrast.rows(), design.columns());

  Image::Header input_header (argument[1]);
  Image::BufferSparse<FixelMetric> mask (input_header);
  auto mask_vox = mask.voxel();

  // Create a image to store fixel indices of a 1D vector
  Image::Header index_header (input_header);
  index_header.set_ndim(4);
  index_header.dim(3) = 2;
  index_header.datatype() = DataType::Int32;
  Image::BufferScratch<int32_t> indexer (index_header);
  auto indexer_vox = indexer.voxel();
  Image::LoopInOrder loop4D (indexer_vox);
  for (loop4D.start (indexer_vox); loop4D.ok(); loop4D.next (indexer_vox))
    indexer_vox.value() = -1;

  // vectors to hold fixel data
  std::vector<Point<value_type> > fixel_positions;
  std::vector<Point<value_type> > fixel_directions;
  std::vector<value_type> pathology_mask;

  int32_t num_fixels = 0;
  int32_t actual_positives = 0;

  Image::BufferSparse<FixelMetric> template_buffer (argument[1]);
  auto template_vox = template_buffer.voxel();

  Image::Transform transform (template_vox);
  Image::LoopInOrder loop (template_vox);

  // Loop over the fixel image, build the indexer image and store fixel data in vectors
  for (loop.start (template_vox, indexer_vox); loop.ok(); loop.next (template_vox, indexer_vox)) {
    indexer_vox[3] = 0;
    indexer_vox.value() = num_fixels;
    size_t f = 0;
    for (; f != template_vox.value().size(); ++f) {
      num_fixels++;
      if (template_vox.value()[f].value >= 1.0)
        actual_positives++;
      pathology_mask.push_back (template_vox.value()[f].value);
      fixel_directions.push_back (template_vox.value()[f].dir);
      fixel_positions.push_back (transform.voxel2scanner (template_vox));
    }
    indexer_vox[3] = 1;
    indexer_vox.value() = f;
  }


  // fixel-fixel connectivity matrix
  std::vector<std::map<int32_t, Stats::CFE::connectivity> > fixel_connectivity (num_fixels);
  std::vector<uint16_t> fixel_TDI (num_fixels, 0);

  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> track_file (argument[2], properties);
  const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
  if (!num_tracks)
    throw Exception ("no tracks found in input file");

  { // Process each track. Build up fixel-fixel connectivity and fixel TDI
    typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;
    DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "pre-computing fixel-fixel connectivity...");
    DWI::Tractography::Mapping::TrackMapperBase mapper (index_header);
    mapper.set_upsample_ratio (DWI::Tractography::Mapping::determine_upsample_ratio (input_header, properties, 0.333f));
    mapper.set_use_precise_mapping (true);
    Stats::CFE::TrackProcessor tract_processor (indexer, fixel_directions, fixel_TDI, fixel_connectivity, angular_threshold_dp);
    Thread::run_queue (loader, DWI::Tractography::Streamline<float>(), mapper, SetVoxelDir(), tract_processor);
  }

  { // Normalise connectivity matrix and threshold
    ProgressBar progress ("normalising and thresholding fixel-fixel connectivity matrix...", num_fixels);
    for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
      auto it = fixel_connectivity[fixel].begin();
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


  // load each fixel image, identify fixel correspondence and save AFD in 2D vector
  Math::Matrix<value_type> control_data (num_fixels, num_subjects);
  {
    ProgressBar progress ("loading input images...", num_subjects);
    for (size_t subject = 0; subject < num_subjects; subject++) {
      Image::BufferSparse<FixelMetric> fixel (filenames[subject]);
      auto fixel_vox = fixel.voxel();
      Image::check_dimensions (fixel, template_vox, 0, 3);

      for (loop.start (fixel_vox, indexer_vox); loop.ok(); loop.next (fixel_vox, indexer_vox)) {
         indexer_vox[3] = 0;
         int32_t index = indexer_vox.value();
         indexer_vox[3] = 1;
         int32_t number_fixels = indexer_vox.value();

         // for each fixel in the template, find the corresponding fixel in this subject voxel
         for (int32_t i = index; i < index + number_fixels; ++i) {
           value_type largest_dp = 0.0;
           int index_of_closest_fixel = -1;
           for (size_t f = 0; f != fixel_vox.value().size(); ++f) {
             value_type dp = std::abs (fixel_directions[i].dot(fixel_vox.value()[f].dir));
             if (dp > largest_dp) {
               largest_dp = dp;
               index_of_closest_fixel = f;
             }
           }
           if (largest_dp > angular_threshold_dp)
             control_data (i, subject) = fixel_vox.value()[index_of_closest_fixel].value;
         }
       }

      progress++;
    }
  }

  for (size_t effect_size = 0; effect_size < effect.size(); ++effect_size) {
    // generate images effected by pathology for all images
    Math::Matrix<value_type> path_data (control_data);
    for (size_t subject = 0; subject < num_subjects; ++subject) {
      for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
        if (pathology_mask[fixel] > 0.0)
          path_data (fixel, subject) = control_data (fixel, subject) - (effect[effect_size] * control_data (fixel, subject));
      }
    }

    // write_fixel_output ("path.msf", path_data.column(4), input_header, template_vox, indexer_vox);
    for (size_t s = 0; s < smooth.size(); ++s) {
      Math::Matrix<value_type> input_control_data (num_fixels, num_subjects);
      Math::Matrix<value_type> input_path_data (num_fixels, num_subjects);
      if (smooth[s] > 0.0) {
        std::vector<std::map<int32_t, value_type> > fixel_smoothing_weights (num_fixels);
        float stdev = smooth[s] / 2.3548;
        const value_type gaussian_const2 = 2.0 * stdev * stdev;
        value_type gaussian_const1 = 1.0 / (stdev *  std::sqrt (2.0 * M_PI));
        for (int32_t f = 0; f < num_fixels; ++f) {
          auto it = fixel_connectivity[f].begin();
          while (it != fixel_connectivity[f].end()) {
            value_type connectivity = it->second.value;
            value_type distance = std::sqrt (std::pow (fixel_positions[f][0] - fixel_positions[it->first][0], 2) +
                                             std::pow (fixel_positions[f][1] - fixel_positions[it->first][1], 2) +
                                             std::pow (fixel_positions[f][2] - fixel_positions[it->first][2], 2));
            value_type weight = connectivity * gaussian_const1 * std::exp (-std::pow (distance, 2) / gaussian_const2);
            if (weight > connectivity_threshold)
              fixel_smoothing_weights[f].insert (std::pair<int32_t, value_type> (it->first, weight));
            ++it;
          }
        }
        // Normalise smoothing weights
        if (smooth[s] > 0.0) {
          for (int32_t i = 0; i < num_fixels; ++i) {
            value_type sum = 0.0;
            for (auto it = fixel_smoothing_weights[i].begin();
                 it != fixel_smoothing_weights[i].end(); ++it)
              sum += it->second;
            value_type norm_factor = 1.0 / sum;
            for (auto it = fixel_smoothing_weights[i].begin();
                   it != fixel_smoothing_weights[i].end(); ++it)
              it->second *= norm_factor;
          }
        }

        // Smooth healthy and pathology data for each subject
        for (size_t subject = 0; subject < num_subjects; ++subject) {
          for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
            auto it = fixel_smoothing_weights[fixel].begin();
            for (; it != fixel_smoothing_weights[fixel].end(); ++it) {
              input_control_data (fixel, subject) += control_data (it->first, subject) * it->second;
              input_path_data (fixel, subject) += path_data (it->first, subject) * it->second;
            }
          }
        }

      // no smoothing, just copy the data across
      } else {
        input_control_data = control_data;
        input_path_data = path_data;
      }
      //write_fixel_output ("path_smoothed.msf", input_path_data.column(4), input_header, template_vox, indexer_vox);


      // Here we pre-compute the t-statistic images for all permutations
      // Create 2 groups of subjects  - pathology vs uneffected controls.
      //    Compute t-statistic image (this is our signal + noise image)
      // Here we use the random permutation to select which patients belong in each group, placing the
      // control data in the LHS of the Y transposed matrix, and path data in the RHS.
      // We then perform the t-test using the default permutation of the design matrix.
      // This code assumes the number of subjects in each group is equal

      std::vector<std::vector<value_type> > control_test_statistics;
      std::vector<std::vector<value_type> > path_test_statistics;
      std::vector<value_type> max_statistics;
      double average_max_t = 0.0;
      double average_t = 0.0;
      {
        PermutationStack perm_stack (permutations);
        ProgressBar progress ("precomputing tstats...", num_permutations);
        for (size_t perm = 0; perm < num_permutations; ++perm) {
          Math::Matrix<value_type> path_v_control_data (input_path_data);
          Math::Matrix<value_type> control_v_control_data (input_control_data);
          for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
            for (size_t subj = 0; subj < num_subjects; ++subj) {
              if (subj < num_subjects / 2)
                path_v_control_data (fixel, subj) = input_control_data (fixel, perm_stack.permutation (perm)[subj]);
              else
                path_v_control_data (fixel, subj) = input_path_data (fixel, perm_stack.permutation (perm)[subj]);
              control_v_control_data (fixel, subj) = input_control_data (fixel, perm_stack.permutation (perm)[subj]);
            }
          }

          std::vector<value_type> path_statistic;
          std::vector<value_type> control_statistic;

          // Perform t-test and enhance
          value_type max_stat = 0.0, min_stat = 0.0;
          Math::Stats::GLMTTest ttest_path (path_v_control_data, design, contrast);
          ttest_path (perm_stack.permutation (0), path_statistic, max_stat, min_stat);
          path_test_statistics.push_back (path_statistic);
          max_statistics.push_back (max_stat);
          average_max_t += max_stat;

          // Here we test control vs control population (ie null test statistic).
          Math::Stats::GLMTTest ttest_control (control_v_control_data, design, contrast);
          ttest_control (perm_stack.permutation (0), control_statistic, max_stat, min_stat);
          control_test_statistics.push_back (control_statistic);


          uint32_t num_tp = 0;
          float sum_max_t = 0.0;
          for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
            if (pathology_mask[fixel] > 0.0) {
              sum_max_t += path_statistic[fixel];
              num_tp++;
            }
          }
          average_t += sum_max_t / (float) num_tp;
//                  std::string path_file("path_tvalue" + str(perm) + ".msf");
//                  std::string control_file("control_tvalue" + str(perm) + ".msf");
//                  write_fixel_output (path_file, path_statistic, input_header, template_vox, indexer_vox);
//                  write_fixel_output (control_file, control_statistic, input_header, template_vox, indexer_vox);
          progress++;
        }
      }
      std::cout << average_t / (float) num_permutations << std::endl;
      std::cout << average_max_t / (float) num_permutations << std::endl;


      for (size_t c = 0; c < C.size(); ++c) {

        // Here we pre-exponentiate each connectivity value to speed up the CFE
        std::vector<std::map<int32_t, Stats::CFE::connectivity> > weighted_fixel_connectivity (num_fixels);
        for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
          auto it = fixel_connectivity[fixel].begin();
          while (it != fixel_connectivity[fixel].end()) {
            Stats::CFE::connectivity weighted_connectivity;
            weighted_connectivity.value = std::pow (it->second.value , C[c]);
            weighted_fixel_connectivity[fixel].insert (std::pair<int32_t, Stats::CFE::connectivity> (it->first, weighted_connectivity));
            ++it;
          }
        }


        for (size_t h = 0; h < H.size(); ++h) {

          for (size_t e = 0; e < E.size(); ++e) {

            CONSOLE ("starting test: effect = " + str(effect[effect_size]) + ", smoothing = " + str(smooth[s]) +
                     ", c = " + str(C[c]) +
                     ", h = " + str(H[h]) +
                     ", e = " + str(E[e]));

            std::string filenameTPR (argument[6]);
            filenameTPR.append ("effect" + str(effect[effect_size]) + "_s" + str(smooth[s]) +
                                "_c" + str (C[c]) + "_h" + str(H[h]) + "_e" + str(E[e]));

            if (MR::Path::exists(filenameTPR)) {
              CONSOLE ("Already done!");
            } else {

              MR::Timer timer;
              std::vector<value_type> max_cfe_statistics (num_permutations);
              std::vector<std::vector<value_type> > control_cfe_statistics (num_permutations, std::vector<value_type> (num_fixels, 0.0));
              std::vector<std::vector<value_type> > path_cfe_statistics (num_permutations, std::vector<value_type> (num_fixels, 0.0));
              {
                MR::Stats::CFE::Enhancer cfe (fixel_connectivity, dh, e, h);
                Stack stack (num_permutations);
                EnhancerKernel processor (stack, control_test_statistics, path_test_statistics, max_statistics, cfe,
                                          max_cfe_statistics, control_cfe_statistics, path_cfe_statistics);
                auto threads = Thread::run (Thread::multi (processor), "threads");
              }

              value_type max_cfe_statistic = *std::max_element (std::begin (max_cfe_statistics), std::end (max_cfe_statistics));
              std::vector<value_type> ROC_thresholds (num_ROC_samples);
              for (size_t t = 0; t < ROC_thresholds.size(); ++t)
                ROC_thresholds[t] = ((value_type) t / ((value_type) num_ROC_samples - 1.0)) * max_cfe_statistic;

              std::vector<std::vector<uint32_t> > TPRates (num_ROC_samples, std::vector<uint32_t> (num_permutations, 0));
              std::vector<int32_t> num_permutations_with_a_false_positive (num_ROC_samples, 0);

              {
                Stack stack (num_permutations);
                ROCThresholdKernel processor (stack, control_cfe_statistics, path_cfe_statistics, ROC_thresholds, pathology_mask,
                                              TPRates, num_permutations_with_a_false_positive);
                auto threads = Thread::run (Thread::multi (processor), "threads");
              }


              // output all noise instance TPR values for variance calculations
              std::ofstream output_all;
              output_all.open (filenameTPR.c_str());
              for (size_t t = 0; t < num_ROC_samples; ++t) {
                for (size_t p = 0; p < num_permutations; ++p) {
                  output_all << (value_type) TPRates [t][p] / (value_type) actual_positives << " ";
                }
                output_all << std::endl;
              }
              output_all.close();


              std::string filenameFPR (argument[7]);
              filenameFPR.append ("effect" + str(effect[effect_size]) + "_s" + str(smooth[s]) +
                                  "_c" + str (C[c]) + "_h" + str(H[h]) + "_e" + str(E[e]));

              std::ofstream output;
              output.open (filenameFPR.c_str());
              for (size_t t = 0; t < num_ROC_samples; ++t) {
                // average TPR across all permutations
                u_int32_t sum = 0.0;
                for (size_t p = 0; p < num_permutations; ++p) {
                  sum += TPRates [t][p];
                }
                output << (value_type) sum / ((value_type) actual_positives * (value_type) num_permutations)   << " ";
                // FPR is defined as the fraction of realisations with a false positive
                output << (value_type) num_permutations_with_a_false_positive[t] / (value_type) num_permutations << std::endl;
              }
              output.close();

              std::cout << "Minutes: " << timer.elapsed() / 60.0 << std::endl;
            }
          }
        }
      }
    }
  }
}


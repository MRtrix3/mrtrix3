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
  + "compute SNR of a ROI";

  ARGUMENTS
  + Argument ("input", "a text file listing the file names of the input fixel images").type_file_in ()

  + Argument ("fixel_in", "the template fixel image including the fake pathology ROI.").type_image_in ()

  + Argument ("design", "the design matrix").type_file_in()

  + Argument ("contrast", "the contrast matrix").type_file_in()

  + Argument ("permutations", "the set of indices for all permutations").type_file_in();

  OPTIONS
  + Option ("effect", "the percentage decrease applied to simulate pathology")
  + Argument ("value").type_float (0.0, 1.0, 100).type_sequence_float();

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

class PermutationStack {
  public:
    PermutationStack (const Math::Matrix<float>& permutations_matrix) :
      num_permutations (permutations_matrix.rows()),
      current_permutation (0),
      progress ("message"),
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


void run ()
{

  const value_type angular_threshold_dp = cos (ANGULAR_THRESHOLD * (M_PI/180.0));

  std::vector<value_type> effect(1);
  effect[0] = 0.2;
  Options opt = get_options("effect");
  if (opt.size())
    effect = opt[0][0];


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
  design.load (argument[2]);
  if (design.rows() != filenames.size())
    throw Exception ("number of subjects does not match number of rows in design matrix");

  // Load contrast matrix:
  Math::Matrix<value_type> contrast;
  contrast.load (argument[3]);

  // Load permutation matrix:
  Math::Matrix<value_type> permutations;
  permutations.load (argument[4]);

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

    // Here we pre-compute the t-statistic images for all permutations
    // Create 2 groups of subjects  - pathology vs uneffected controls.
    //    Compute t-statistic image (this is our signal + noise image)
    // Here we use the random permutation to select which patients belong in each group, placing the
    // control data in the LHS of the Y transposed matrix, and path data in the RHS.
    // We then perform the t-test using the default permutation of the design matrix.
    // This code assumes the number of subjects in each group is equal
    std::vector<std::vector<value_type> > control_test_statistics;
    std::vector<std::vector<value_type> > path_test_statistics;
    double path_sum = 0.0;
    double stdev_sum = 0.0;
    {
      PermutationStack perm_stack (permutations);
      ProgressBar progress ("precomputing tstats...", num_permutations);
      for (size_t perm = 0; perm < num_permutations; ++perm) {
        Math::Matrix<value_type> path_v_control_data (path_data);
        Math::Matrix<value_type> control_v_control_data (control_data);
        for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
          for (size_t subj = 0; subj < num_subjects; ++subj) {
            if (subj < num_subjects / 2)
              path_v_control_data (fixel, subj) = control_data (fixel, perm_stack.permutation (perm)[subj]);
            else
              path_v_control_data (fixel, subj) = path_data (fixel, perm_stack.permutation (perm)[subj]);
            control_v_control_data (fixel, subj) = control_data (fixel, perm_stack.permutation (perm)[subj]);
          }
        }
        std::vector<value_type> path_statistic;
        std::vector<value_type> control_statistic;

        // Perform t-test
        value_type max_stat = 0.0, min_stat = 0.0;
        Math::Stats::GLMTTest ttest_path (path_v_control_data, design, contrast);
        ttest_path (perm_stack.permutation (0), path_statistic, max_stat, min_stat);
        path_test_statistics.push_back (path_statistic);

        // Here we test control vs control population (ie null test statistic).
        Math::Stats::GLMTTest ttest_control (control_v_control_data, design, contrast);
        ttest_control (perm_stack.permutation (0), control_statistic, max_stat, min_stat);
        control_test_statistics.push_back (control_statistic);

        uint32_t num_tp = 0;
        double control_sum_squares = 0.0;
        double path_sum_perm = 0.0;
        for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
          if (pathology_mask[fixel] > 0.0) {
            path_sum_perm += path_statistic[fixel];
            num_tp++;
          }
          control_sum_squares += std::pow (control_statistic[fixel], 2);
        }
        path_sum += path_sum_perm / (double) num_tp;

//        std::cout << "average " << path_sum_perm / (double) num_tp << std::endl;
//        std::cout << "stdev " << std::sqrt (control_sum_squares / ((float)num_fixels - 1)) << std::endl;

        stdev_sum += std::sqrt (control_sum_squares / ((float)num_fixels - 1));
        progress++;
      }
    }
    double stdev = stdev_sum / ((float) num_permutations );
    double path_average = path_sum / (double) num_permutations;
    std::cout << path_average / stdev << std::endl;
  }
}




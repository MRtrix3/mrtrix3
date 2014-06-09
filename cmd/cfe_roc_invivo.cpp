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
#include "math/stats/permutation.h"
#include "math/stats/glm.h"
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
  + Argument ("input", "a text file listing the file names of the input fixel images").type_file ()

  + Argument ("fixel_in", "the template fixel image including the fake pathology ROI.").type_image_in ()

  + Argument ("tracks", "the tractogram used to derive fixel-fixel connectivity").type_file ()

  + Argument ("design", "the design matrix").type_file()

  + Argument ("contrast", "the contrast matrix").type_file()

  + Argument ("output", "the output prefix").type_file();

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

  + Option ("permutations", "the number of permutations")
  + Argument ("num").type_integer(1, 1000, 10000)

  + Option ("roc", "the number of thresholds for ROC curve generation")
  + Argument ("num").type_integer (1, 1000, 10000);


}


#define ANGULAR_THRESHOLD 30

typedef float value_type;


void write_fixel_output (const std::string filename,
                         const Math::Vector<value_type> data,
                         const Image::Header& header,
                         Image::BufferSparse<FixelMetric>::voxel_type& mask_vox,
                         Image::BufferScratch<int32_t>::voxel_type& indexer_vox) {
  Image::BufferSparse<FixelMetric> output_buffer (filename, header);
  Image::BufferSparse<FixelMetric>::voxel_type output_voxel (output_buffer);
  Image::LoopInOrder loop (mask_vox);
  for (loop.start (mask_vox, indexer_vox, output_voxel); loop.ok(); loop.next (mask_vox, indexer_vox, output_voxel)) {
    output_voxel.value().zero();
    output_voxel.value().set_size (mask_vox.value().size());
    indexer_vox[3] = 0;
    int32_t index = indexer_vox.value();
    for (size_t f = 0; f != mask_vox.value().size(); ++f, ++index) {
      output_voxel.value()[f] = mask_vox.value()[f];
      output_voxel.value()[f].value = data[index];
    }
  }
}

void write_fixel_output (const std::string filename,
                         const std::vector<value_type> data,
                         const Image::Header& header,
                         Image::BufferSparse<FixelMetric>::voxel_type& mask_vox,
                         Image::BufferScratch<int32_t>::voxel_type& indexer_vox) {
  Image::BufferSparse<FixelMetric> output_buffer (filename, header);
  Image::BufferSparse<FixelMetric>::voxel_type output_voxel (output_buffer);
  Image::LoopInOrder loop (mask_vox);
  for (loop.start (mask_vox, indexer_vox, output_voxel); loop.ok(); loop.next (mask_vox, indexer_vox, output_voxel)) {
    output_voxel.value().zero();
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


/**
 * Process each track. For each track tangent dixel, identify the closest fixel
 */
class TrackProcessor {

  public:
    TrackProcessor (Image::BufferScratch<int32_t>& FOD_fixel_indexer,
                    const std::vector<Point<value_type> >& FOD_fixel_directions,
                    std::vector<uint16_t>& fixel_TDI,
                    std::vector<std::map<int32_t, Stats::TFCE::connectivity> >& fixel_connectivity,
                    value_type angular_threshold_dp):
                    fixel_indexer (FOD_fixel_indexer) ,
                    fixel_directions (FOD_fixel_directions),
                    fixel_TDI (fixel_TDI),
                    fixel_connectivity (fixel_connectivity),
                    angular_threshold_dp (angular_threshold_dp) { }

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



class Processor {
  public:
    Processor (Stats::TFCE::PermutationStack& perm_stack,
               const Math::Matrix<value_type>& control_data,
               const Math::Matrix<value_type>& path_data,
               const Math::Matrix<value_type>& design,
               const Math::Matrix<value_type>& contrast,
               const Math::Stats::GLMTTest& ttest_controls,
               const int32_t num_fixels,
               const int32_t actual_positives,
               const int32_t num_ROC_samples,
               const std::vector<value_type>& truth_statistic,
               const std::vector<std::map<int32_t, Stats::TFCE::connectivity> >& fixel_connectivity,
               Math::Matrix<value_type>& global_TPRates,
               std::vector<int32_t>& global_num_permutations_with_false_positive,
               const value_type dh,
               const value_type e,
               const value_type h,
               const value_type c,
               Image::Header& input_header,
               Image::BufferSparse<FixelMetric>::voxel_type& template_vox,
               const Image::BufferSparse<FixelMetric>& template_data,
               Image::BufferScratch<int32_t>::voxel_type& indexer_vox,
               const Image::BufferScratch<int32_t>& indexer_data):
                 perm_stack (perm_stack),
                 control_data (control_data),
                 path_data (path_data),
                 design (design),
                 contrast (contrast),
                 ttest_controls (ttest_controls),
                 num_fixels (num_fixels),
                 actual_positives (actual_positives),
                 num_ROC_samples (num_ROC_samples),
                 truth_statistic (truth_statistic),
                 global_TPRates (global_TPRates),
                 global_num_permutations_with_false_positive (global_num_permutations_with_false_positive),
                 num_permutations_with_a_false_positive (num_ROC_samples, 0),
                 dh (dh), e (e), h (h), c (c),
                 control_test_statistic (num_fixels, 0.0),
                 path_test_statistic (num_fixels, 0.0),
                 cfe_control_test_statistic (num_fixels, 0.0),
                 cfe_path_test_statistic (num_fixels, 0.0),
                 cfe (fixel_connectivity, dh, e, h),
                 input_header (input_header),
                 template_vox (template_vox),
                 template_data (template_data),
                 indexer_vox (indexer_vox),
                 indexer_data (indexer_data){
    }

    ~Processor () {
      for (size_t t = 0; t < num_ROC_samples; ++t)
        global_num_permutations_with_false_positive[t] += num_permutations_with_a_false_positive[t];
    }

    void execute () {
      size_t index = 0;
      while (( index = perm_stack.next() ) < perm_stack.num_permutations)
        process_permutation (index);
    }

  private:

    void write_fixel_output (const std::string filename,
                             const std::vector<value_type>& data) {
      Image::BufferSparse<FixelMetric> output_buffer (filename, input_header);
      Image::BufferSparse<FixelMetric>::voxel_type output_voxel (output_buffer);
      Image::LoopInOrder loop (template_vox);
      Image::check_dimensions (output_voxel, template_vox);
      for (loop.start (template_vox, indexer_vox, output_voxel); loop.ok(); loop.next (template_vox, indexer_vox, output_voxel)) {
        output_voxel.value().zero();
        output_voxel.value().set_size (template_vox.value().size());
        indexer_vox[3] = 0;
        int32_t index = indexer_vox.value();
        for (size_t f = 0; f != template_vox.value().size(); ++f, ++index) {
         output_voxel.value()[f] = template_vox.value()[f];
         output_voxel.value()[f].value = data[index];
        }
      }
    }


    void process_permutation (int param_index) {

      // Create 2 groups of subjects  - pathology vs uneffected controls.
      //    Compute t-statistic image (this is our signal + noise image)
      // Here we use the random permutation to select which patients belong in each group, placing the
      // control data in the LHS of the Y transposed matrix, and path data in the RHS.
      // We then perform the t-test using the default permutation of the design matrix.
      // This code assumes the number of subjects in each group is equal
      Math::Matrix<value_type> path_v_control_data (control_data);
      for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
        for (size_t row = 0; row < perm_stack.permutation (param_index).size(); ++row) {
          if (row < perm_stack.permutation (param_index).size() / 2)
            path_v_control_data (fixel, row) = control_data (fixel, perm_stack.permutation (param_index)[row]);
          else
            path_v_control_data (fixel, row) = path_data (fixel, perm_stack.permutation (param_index)[row]);
        }
      }


      // Perform t-test and enhance
      value_type max_stat = 0.0, min_stat = 0.0;
      Math::Stats::GLMTTest ttest_path (path_v_control_data, design, contrast);
      ttest_path (perm_stack.permutation (0), path_test_statistic, max_stat, min_stat);

//      write_fixel_output ("path_tvalue.msf", path_test_statistic);

      value_type max_cfe_statistic = cfe (max_stat, path_test_statistic, &cfe_path_test_statistic, c);

//      write_fixel_output ("path_tfce.msf", cfe_path_test_statistic);

      // Here we test control vs control population (ie null test statistic).
      ttest_controls (perm_stack.permutation (param_index), control_test_statistic, max_stat, min_stat);

//      write_fixel_output ("control_tvalue.msf", control_test_statistic);

      cfe (max_stat, control_test_statistic, &cfe_control_test_statistic, c);
//      write_fixel_output ("control_tfce.msf", cfe_control_test_statistic);

      std::vector<value_type> num_true_positives (num_ROC_samples);


      for (size_t t = 0; t < num_ROC_samples; ++t) {
        value_type threshold = ((value_type) t / ((value_type) num_ROC_samples - 1.0)) * max_cfe_statistic;
        bool contains_false_positive = false;
        for (int32_t f = 0; f < num_fixels; ++f) {
          if (truth_statistic[f] >= 1.0) {
            if (cfe_path_test_statistic[f] > threshold)
              num_true_positives[t]++;
          } else {
            if (cfe_control_test_statistic[f] > threshold)
              contains_false_positive = true;
          }
        }
        if (contains_false_positive)
          num_permutations_with_a_false_positive[t]++;
        global_TPRates(t, param_index) = (float) num_true_positives[t] / (float) actual_positives;
      }

    }

    Stats::TFCE::PermutationStack& perm_stack;
    const Math::Matrix<value_type>& control_data;
    const Math::Matrix<value_type>& path_data;
    const Math::Matrix<value_type>& design;
    const Math::Matrix<value_type>& contrast;
    Math::Stats::GLMTTest ttest_controls;
    const int32_t num_fixels;
    const int32_t actual_positives;
    const size_t num_ROC_samples;
    const std::vector<value_type>& truth_statistic;
    Math::Matrix<value_type>& global_TPRates;
    std::vector<int32_t>& global_num_permutations_with_false_positive;
    std::vector<int32_t> num_permutations_with_a_false_positive;
    const value_type dh, e, h, c;
    std::vector<value_type> control_test_statistic;
    std::vector<value_type> path_test_statistic;
    std::vector<value_type> cfe_control_test_statistic;
    std::vector<value_type> cfe_path_test_statistic;
    MR::Stats::TFCE::Connectivity cfe;
    Image::Header input_header;
    Image::BufferSparse<FixelMetric>::voxel_type template_vox;
    const Image::BufferSparse<FixelMetric>& template_data;
    Image::BufferScratch<int32_t>::voxel_type indexer_vox;
    const Image::BufferScratch<int32_t>& indexer_data;
};







bool file_exists (const std::string& filename)
{
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1)
      return true;
    return false;
}







void run ()
{
  const value_type angular_threshold_dp = cos (ANGULAR_THRESHOLD * (M_PI/180.0));
  const value_type dh = 0.1;
  const value_type connectivity_threshold = 0.01;

  Options opt = get_options("roc");
  int num_ROC_samples = 2000;
  if (opt.size())
    num_ROC_samples = opt[0][0];

  int num_permutations = 1000;
  opt = get_options("permutations");
  if (opt.size())
    num_permutations = opt[0][0];

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

  if (contrast.columns() > design.columns())
    throw Exception ("too many contrasts for design matrix");
  contrast.resize (contrast.rows(), design.columns());

  Image::Header input_header (argument[1]);
  Image::BufferSparse<FixelMetric> mask (input_header);
  Image::BufferSparse<FixelMetric>::voxel_type mask_vox (mask);

  // Create a image to store fixel indices of a 1D vector
  Image::Header index_header (input_header);
  index_header.set_ndim(4);
  index_header.dim(3) = 2;
  index_header.datatype() = DataType::Int32;
  Image::BufferScratch<int32_t> indexer (index_header);
  Image::BufferScratch<int32_t>::voxel_type indexer_vox (indexer);
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
  Image::BufferSparse<FixelMetric>::voxel_type template_vox (template_buffer);

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
      Image::BufferSparse<FixelMetric>::voxel_type fixel_vox (fixel);
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
             value_type dp = Math::abs (fixel_directions[i].dot(fixel_vox.value()[f].dir));
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


  //  write_fixel_output ("before.msf", control_data.column(4), input_header, template_vox, indexer_vox);

  // fixel-fixel connectivity matrix (sparse)
  std::vector<std::map<int32_t, Stats::TFCE::connectivity> > fixel_connectivity (num_fixels);
  std::vector<uint16_t> fixel_TDI (num_fixels, 0);

  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> track_file (argument[2], properties);
  const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
  if (!num_tracks)
    throw Exception ("no tracks found in input file");

  { // Process each track. Build up fixel-fixel connectivity and fixel TDI
    typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;
    DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "pre-computing fixel-fixel connectivity...");
    DWI::Tractography::Mapping::TrackMapperBase<SetVoxelDir> mapper (index_header);
    TrackProcessor tract_processor (indexer, fixel_directions, fixel_TDI, fixel_connectivity, angular_threshold_dp);
    Thread::run_queue (loader, DWI::Tractography::Streamline<float>(), mapper, SetVoxelDir(), tract_processor);
  }

  { // Normalise connectivity matrix and threshold
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



  for (size_t effect_size = 0; effect_size < effect.size(); ++effect_size) {
    // generate images effected by pathology for all images
    Math::Matrix<value_type> path_data (control_data);
    for (size_t subject = 0; subject < num_subjects; ++subject) {
      for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
        if (pathology_mask[fixel] > 0.0)
          path_data (fixel, subject) = control_data (fixel, subject) - (effect[effect_size] * control_data (fixel, subject));
      }
    }

//    write_fixel_output ("path.msf", path_data.column(4), input_header, template_vox, indexer_vox);
    for (size_t s = 0; s < smooth.size(); ++s) {
      Math::Matrix<value_type> input_data (num_fixels, num_subjects);
      Math::Matrix<value_type> input_path_data (num_fixels, num_subjects);
      if (smooth[s] > 0.0) {
        std::vector<std::map<int32_t, value_type> > fixel_smoothing_weights (num_fixels);
        float stdev = smooth[s] / 2.3548;
        const value_type gaussian_const2 = 2.0 * stdev * stdev;
        value_type gaussian_const1 = 1.0 / (stdev *  Math::sqrt (2.0 * M_PI));
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
        // Normalise smoothing weights
        if (smooth[s] > 0.0) {
          for (int32_t i = 0; i < num_fixels; ++i) {
            value_type sum = 0.0;
            for (std::map<int32_t, value_type>::iterator it = fixel_smoothing_weights[i].begin();
                 it != fixel_smoothing_weights[i].end(); ++it)
              sum += it->second;
            value_type norm_factor = 1.0 / sum;
            for (std::map<int32_t, value_type>::iterator it = fixel_smoothing_weights[i].begin();
                   it != fixel_smoothing_weights[i].end(); ++it)
              it->second *= norm_factor;
          }
        }

        // Smooth healthy and pathology data for each subject
        for (size_t subject = 0; subject < num_subjects; ++subject) {
          for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
            std::map<int32_t, value_type>::const_iterator it = fixel_smoothing_weights[fixel].begin();
            for (; it != fixel_smoothing_weights[fixel].end(); ++it) {
              input_data (fixel, subject) += control_data (it->first, subject) * it->second;
              input_path_data (fixel, subject) += path_data (it->first, subject) * it->second;
            }
          }
        }


      // no smoothing, just copy the data across
      } else {
        input_data = control_data;
        input_path_data = path_data;
      }
//      write_fixel_output ("path_smoothed.msf", input_path_data.column(4), input_header, template_vox, indexer_vox);

      for (size_t h = 0; h < H.size(); ++h) {
        for (size_t e = 0; e < E.size(); ++e) {
          for (size_t c = 0; c < C.size(); ++c) {

            CONSOLE ("starting test: smoothing = " + str(smooth[s]) +
                     ", effect = " + str(effect[effect_size]) + ", h = " + str(H[h]) +
                     ", e = " + str(E[e]) + ", c = " + str(C[c]));

            std::string filename (argument[5]);
            filename.append ("_s" + str(smooth[s]) + "_effect" + str(effect[effect_size]) +
                             "_h" + str(H[h]) + "_e" + str(E[e]) +
                             "_c" + str (C[c]));

            if (file_exists (filename)) {
              CONSOLE ("Already done!");
            } else {

              MR::Timer timer;

              Math::Matrix<value_type> TPRates (num_ROC_samples, num_permutations);
              TPRates.zero();
              std::vector<int32_t> num_permutations_with_a_false_positive (num_ROC_samples, 0);
              {

                Stats::TFCE::PermutationStack perm_stack (num_permutations, num_subjects);
                Math::Stats::GLMTTest ttest_control (control_data, design, contrast);

                int index = 0;

                Math::Matrix<value_type> path_v_control_data (control_data);
                for (int32_t fixel = 0; fixel < num_fixels; ++fixel) {
                  for (size_t row = 0; row < perm_stack.permutation (index).size(); ++row) {
                    if (row < perm_stack.permutation (index).size() / 2)
                      path_v_control_data (fixel, row) = control_data (fixel, perm_stack.permutation (index)[row]);
                    else
                      path_v_control_data (fixel, row) = path_data (fixel, perm_stack.permutation (index)[row]);
                  }
                }

                Processor processor (perm_stack, control_data, path_data, design, contrast, ttest_control, num_fixels, actual_positives,
                                     num_ROC_samples, pathology_mask, fixel_connectivity, TPRates,
                                     num_permutations_with_a_false_positive, dh, E[e], H[h], C[c],
                                     input_header, template_vox, template_buffer, indexer_vox, indexer);

                Thread::Array< Processor > thread_list (processor);
                Thread::Exec threads (thread_list, "threads");
              }


              std::string filename_all_TPR (filename);
              filename_all_TPR.append("_all_tpr");
              TPRates.save(filename_all_TPR);

              std::ofstream output;
              output.open (filename.c_str());

              for (int t = 0; t < num_ROC_samples; ++t) {
                // average TPR across all permutations realisations
                double sum = 0.0;
                for (int p = 0; p < num_permutations; ++p)
                  sum += TPRates (t,p);
                output << sum / (value_type) num_permutations << " ";
                // FPR is defined as the fraction of permutations realisations with a false positive
                output << num_permutations_with_a_false_positive[t] / (value_type) num_permutations << std::endl;
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


/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "file/path.h"
#include "algo/loop.h"
#include "image.h"
#include "math/SH.h"
#include "dwi/directions/predefined.h"
#include "timer.h"
#include "math/stats/glm.h"
#include "math/stats/permutation.h"
#include "math/stats/typedefs.h"
#include "stats/cluster.h"
#include "stats/enhance.h"
#include "stats/permtest.h"
#include "stats/tfce.h"


using namespace MR;
using namespace App;
using namespace MR::Math::Stats;


#define DEFAULT_TFCE_DH 0.1
#define DEFAULT_TFCE_H 2.0
#define DEFAULT_TFCE_E 0.5


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Voxel-based analysis using permutation testing and threshold-free cluster enhancement";

  REFERENCES
   + "* If not using the -threshold command-line option:\n"
   "Smith, S. M. & Nichols, T. E. "
   "Threshold-free cluster enhancement: Addressing problems of smoothing, threshold dependence and localisation in cluster inference. "
   "NeuroImage, 2009, 44, 83-98"

   + "* If using the -nonstationary option:"
   "Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. Adjusting the effect of nonstationarity in cluster-based and TFCE inference. \n"
   "Neuroimage, 2011, 54(3), 2006-19\n";


  ARGUMENTS
  + Argument ("input", "a text file containing the file names of the input images, one file per line").type_file_in()

  + Argument ("design", "the design matrix, rows should correspond with images in the input image text file").type_file_in()

  + Argument ("contrast", "the contrast matrix, only specify one contrast as it will automatically compute the opposite contrast.").type_file_in()

  + Argument ("mask", "a mask used to define voxels included in the analysis.").type_image_in()

  + Argument ("output", "the filename prefix for all output.").type_text();


  OPTIONS
  + Stats::PermTest::Options (true)

  + Stats::TFCE::Options (DEFAULT_TFCE_DH, DEFAULT_TFCE_E, DEFAULT_TFCE_H)

  + OptionGroup ("Additional options for mrclusterstats")

    + Option ("negative", "automatically test the negative (opposite) contrast. By computing the opposite contrast simultaneously "
                          "the computation time is reduced.")

    + Option ("threshold", "the cluster-forming threshold to use for a standard cluster-based analysis. "
                           "This disables TFCE, which is the default otherwise.")
    + Argument ("value").type_float (1.0e-6)

    + Option ("connectivity", "use 26-voxel-neighbourhood connectivity (Default: 6)");

}



template <class VectorType, class ImageType>
void write_output (const VectorType& data,
                   const vector<vector<int> >& mask_indices,
                   ImageType& image) {
  for (size_t i = 0; i < mask_indices.size(); i++) {
    for (size_t dim = 0; dim < image.ndim(); dim++)
      image.index(dim) = mask_indices[i][dim];
    image.value() = data[i];
  }
}



using value_type = Stats::TFCE::value_type;



void run() {

  const value_type cluster_forming_threshold = get_option_value ("threshold", NaN);
  const value_type tfce_dh = get_option_value ("tfce_dh", DEFAULT_TFCE_DH);
  const value_type tfce_H = get_option_value ("tfce_h", DEFAULT_TFCE_H);
  const value_type tfce_E = get_option_value ("tfce_e", DEFAULT_TFCE_E);
  const bool use_tfce = !std::isfinite (cluster_forming_threshold);
  int num_perms = get_option_value ("nperms", DEFAULT_NUMBER_PERMUTATIONS);
  int nperms_nonstationary = get_option_value ("nperms_nonstationary", DEFAULT_NUMBER_PERMUTATIONS_NONSTATIONARITY);

  const bool do_26_connectivity = get_options("connectivity").size();
  const bool do_nonstationary_adjustment = get_options ("nonstationary").size();

  // Read filenames
  vector<std::string> subjects;
  {
    std::string folder = Path::dirname (argument[0]);
    std::ifstream ifs (argument[0].c_str());
    std::string temp;
    while (getline (ifs, temp))
      subjects.push_back (Path::join (folder, temp));
  }

  // Load design matrix
  const matrix_type design = load_matrix<value_type> (argument[1]);
  if (design.rows() != (ssize_t)subjects.size())
    throw Exception ("number of input files does not match number of rows in design matrix");

  // Load permutations file if supplied
  auto opt = get_options("permutations");
  vector<vector<size_t> > permutations;
  if (opt.size()) {
    permutations = Math::Stats::Permutation::load_permutations_file (opt[0][0]);
    num_perms = permutations.size();
    if (permutations[0].size() != (size_t)design.rows())
       throw Exception ("number of rows in the permutations file (" + str(opt[0][0]) + ") does not match number of rows in design matrix");
  }

  // Load non-stationary correction permutations file if supplied
  opt = get_options("permutations_nonstationary");
  vector<vector<size_t> > permutations_nonstationary;
  if (opt.size()) {
    permutations_nonstationary = Math::Stats::Permutation::load_permutations_file (opt[0][0]);
    nperms_nonstationary = permutations.size();
    if (permutations_nonstationary[0].size() != (size_t)design.rows())
      throw Exception ("number of rows in the nonstationary permutations file (" + str(opt[0][0]) + ") does not match number of rows in design matrix");
  }

  // Load contrast matrix
  const matrix_type contrast = load_matrix<value_type> (argument[2]);
  if (contrast.cols() != design.cols())
    throw Exception ("the number of contrasts does not equal the number of columns in the design matrix");

  auto mask_header = Header::open (argument[3]);
  // Load Mask and compute adjacency
  auto mask_image = mask_header.get_image<value_type>();
  Filter::Connector connector (do_26_connectivity);
  vector<vector<int> > mask_indices = connector.precompute_adjacency (mask_image);
  const size_t num_vox = mask_indices.size();

  matrix_type data (num_vox, subjects.size());

  {
    // Load images
    ProgressBar progress("loading images", subjects.size());
    for (size_t subject = 0; subject < subjects.size(); subject++) {
      LogLevelLatch log_level (0);
      auto input_image = Image<float>::open (subjects[subject]); //.with_direct_io (3); <- Should be inputting 3D images?
      check_dimensions (input_image, mask_image, 0, 3);
      int index = 0;
      vector<vector<int> >::iterator it;
      for (it = mask_indices.begin(); it != mask_indices.end(); ++it) {
        input_image.index(0) = (*it)[0];
        input_image.index(1) = (*it)[1];
        input_image.index(2) = (*it)[2];
        data (index++, subject) = input_image.value();
      }
      progress++;
    }
  }
  if (!data.allFinite())
    WARN ("input data contains non-finite value(s)");

  Header output_header (mask_header);
  output_header.datatype() = DataType::Float32;
  output_header.keyval()["num permutations"] = str(num_perms);
  output_header.keyval()["26 connectivity"] = str(do_26_connectivity);
  output_header.keyval()["nonstationary adjustment"] = str(do_nonstationary_adjustment);
  if (use_tfce) {
    output_header.keyval()["tfce_dh"] = str(tfce_dh);
    output_header.keyval()["tfce_e"] = str(tfce_E);
    output_header.keyval()["tfce_h"] = str(tfce_H);
  } else {
    output_header.keyval()["threshold"] = str(cluster_forming_threshold);
  }

  const std::string prefix (argument[4]);
  bool compute_negative_contrast = get_options("negative").size();

  vector_type default_cluster_output (num_vox);
  std::shared_ptr<vector_type> default_cluster_output_neg;
  vector_type tvalue_output (num_vox);
  vector_type empirical_enhanced_statistic;
  if (compute_negative_contrast)
    default_cluster_output_neg.reset (new vector_type (num_vox));

  Math::Stats::GLMTTest glm (data, design, contrast);

  std::shared_ptr<Stats::EnhancerBase> enhancer;
  if (use_tfce) {
    std::shared_ptr<Stats::TFCE::EnhancerBase> base (new Stats::Cluster::ClusterSize (connector, cluster_forming_threshold));
    enhancer.reset (new Stats::TFCE::Wrapper (base, tfce_dh, tfce_E, tfce_H));
  } else {
    enhancer.reset (new Stats::Cluster::ClusterSize (connector, cluster_forming_threshold));
  }

  if (do_nonstationary_adjustment) {
    if (!use_tfce)
      throw Exception ("nonstationary adjustment is not currently implemented for threshold-based cluster analysis");
    empirical_enhanced_statistic = vector_type::Zero (num_vox);
    if (permutations_nonstationary.size()) {
      Stats::PermTest::PermutationStack permutations (permutations_nonstationary, "precomputing empirical statistic for non-stationarity adjustment...");
      Stats::PermTest::precompute_empirical_stat (glm, enhancer, permutations, empirical_enhanced_statistic);
    } else {
      Stats::PermTest::PermutationStack permutations (nperms_nonstationary, design.rows(), "precomputing empirical statistic for non-stationarity adjustment...", false);
      Stats::PermTest::precompute_empirical_stat (glm, enhancer, permutations, empirical_enhanced_statistic);
    }

    save_matrix (empirical_enhanced_statistic, prefix + "empirical.txt");
  }

  Stats::PermTest::precompute_default_permutation (glm, enhancer, empirical_enhanced_statistic,
                                                   default_cluster_output, default_cluster_output_neg, tvalue_output);

  {
    ProgressBar progress ("generating pre-permutation output", (compute_negative_contrast ? 3 : 2) + contrast.cols() + 3);
    {
      auto tvalue_image = Image<float>::create (prefix + "tvalue.mif", output_header);
      write_output (tvalue_output, mask_indices, tvalue_image);
    }
    ++progress;
    {
      auto cluster_image = Image<float>::create (prefix + (use_tfce ? "tfce.mif" : "cluster_sizes.mif"), output_header);
      write_output (default_cluster_output, mask_indices, cluster_image);
    }
    ++progress;
    if (compute_negative_contrast) {
      assert (default_cluster_output_neg);
      auto cluster_image_neg = Image<float>::create (prefix + (use_tfce ? "tfce_neg.mif" : "cluster_sizes_neg.mif"), output_header);
      write_output (*default_cluster_output_neg, mask_indices, cluster_image_neg);
      ++progress;
    }
    auto temp = Math::Stats::GLM::solve_betas (data, design);
    for (ssize_t i = 0; i < contrast.cols(); ++i) {
      auto beta_image = Image<float>::create (prefix + "beta" + str(i) + ".mif", output_header);
      write_output (temp.row(i), mask_indices, beta_image);
      ++progress;
    }
    {
      const auto temp = Math::Stats::GLM::abs_effect_size (data, design, contrast);
      auto abs_effect_image = Image<float>::create (prefix + "abs_effect.mif", output_header);
      write_output (temp.row(0), mask_indices, abs_effect_image);
    }
    ++progress;
    {
      const auto temp = Math::Stats::GLM::std_effect_size (data, design, contrast);
      auto std_effect_image = Image<float>::create (prefix + "std_effect.mif", output_header);
      write_output (temp.row(0), mask_indices, std_effect_image);
    }
    ++progress;
    {
      const auto temp = Math::Stats::GLM::stdev (data, design);
      auto std_dev_image = Image<float>::create (prefix + "std_dev.mif", output_header);
      write_output (temp.row(0), mask_indices, std_dev_image);
    }
  }

  if (!get_options ("notest").size()) {

    vector_type perm_distribution (num_perms);
    std::shared_ptr<vector_type> perm_distribution_neg;
    vector_type uncorrected_pvalue (num_vox);
    std::shared_ptr<vector_type> uncorrected_pvalue_neg;

    if (compute_negative_contrast) {
      perm_distribution_neg.reset (new vector_type (num_perms));
      uncorrected_pvalue_neg.reset (new vector_type (num_vox));
    }

    if (permutations.size()) {
      Stats::PermTest::run_permutations (permutations, glm, enhancer, empirical_enhanced_statistic,
                                         default_cluster_output, default_cluster_output_neg,
                                         perm_distribution, perm_distribution_neg,
                                         uncorrected_pvalue, uncorrected_pvalue_neg);
    } else {
      Stats::PermTest::run_permutations (num_perms, glm, enhancer, empirical_enhanced_statistic,
                                         default_cluster_output, default_cluster_output_neg,
                                         perm_distribution, perm_distribution_neg,
                                         uncorrected_pvalue, uncorrected_pvalue_neg);
    }

    save_matrix (perm_distribution, prefix + "perm_dist.txt");
    if (compute_negative_contrast) {
      assert (perm_distribution_neg);
      save_matrix (*perm_distribution_neg, prefix + "perm_dist_neg.txt");
    }

    ProgressBar progress ("generating output", compute_negative_contrast ? 4 : 2);
    {
      auto uncorrected_pvalue_image = Image<float>::create (prefix + "uncorrected_pvalue.mif", output_header);
      write_output (uncorrected_pvalue, mask_indices, uncorrected_pvalue_image);
    }
    ++progress;
    {
      vector_type fwe_pvalue_output (num_vox);
      Math::Stats::Permutation::statistic2pvalue (perm_distribution, default_cluster_output, fwe_pvalue_output);
      auto fwe_pvalue_image = Image<float>::create (prefix + "fwe_pvalue.mif", output_header);
      write_output (fwe_pvalue_output, mask_indices, fwe_pvalue_image);
    }
    ++progress;
    if (compute_negative_contrast) {
      assert (uncorrected_pvalue_neg);
      assert (perm_distribution_neg);
      auto uncorrected_pvalue_image_neg = Image<float>::create (prefix + "uncorrected_pvalue_neg.mif", output_header);
      write_output (*uncorrected_pvalue_neg, mask_indices, uncorrected_pvalue_image_neg);
      ++progress;
      vector_type fwe_pvalue_output_neg (num_vox);
      Math::Stats::Permutation::statistic2pvalue (*perm_distribution_neg, *default_cluster_output_neg, fwe_pvalue_output_neg);
      auto fwe_pvalue_image_neg = Image<float>::create (prefix + "fwe_pvalue_neg.mif", output_header);
      write_output (fwe_pvalue_output_neg, mask_indices, fwe_pvalue_image_neg);
    }
  }

}

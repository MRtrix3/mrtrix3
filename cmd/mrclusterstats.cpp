/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
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


#define DEFAULT_PERMUTATIONS 5000
#define DEFAULT_TFCE_DH 0.1
#define DEFAULT_TFCE_H 2.0
#define DEFAULT_TFCE_E 0.5
#define DEFAULT_PERMUTATIONS_NONSTATIONARITY 5000


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Voxel-based analysis using permutation testing and threshold-free cluster enhancement.";

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
  + Option ("negative", "automatically test the negative (opposite) contrast. By computing the opposite contrast simultaneously "
                        "the computation time is reduced.")

  + Option ("nperms", "the number of permutations (default = " + str(DEFAULT_PERMUTATIONS) + ").")
  +   Argument ("num").type_integer (1)

  + Option ("threshold", "the cluster-forming threshold to use for a standard cluster-based analysis. "
      "This disables TFCE, which is the default otherwise.")
  +   Argument ("value").type_float (1.0e-6)

  + Option ("tfce_dh", "the height increment used in the TFCE integration (default = " + str(DEFAULT_TFCE_DH, 2) + ")")
  +   Argument ("value").type_float (0.001, 1.0)

  + Option ("tfce_e", "TFCE extent parameter (default = " + str(DEFAULT_TFCE_E, 2) + ")")
  +   Argument ("value").type_float (0.001, 100.0)

  + Option ("tfce_h", "TFCE height parameter (default = " + str(DEFAULT_TFCE_H, 2) + ")")
  +   Argument ("value").type_float (0.001, 100.0)

  + Option ("connectivity", "use 26-voxel-neighbourhood connectivity (Default: 6)")

  + Option ("nonstationary", "perform non-stationarity correction (currently only implemented with tfce)")

  + Option ("nperms_nonstationary", "the number of permutations used when precomputing the empirical statistic image for nonstationary correction (Default: " + str(DEFAULT_PERMUTATIONS_NONSTATIONARITY) + ")")
  +   Argument ("num").type_integer (1);

}



void run() {

  const value_type cluster_forming_threshold = get_option_value ("threshold", NaN);
  const value_type tfce_dh = get_option_value ("tfce_dh", DEFAULT_TFCE_DH);
  const value_type tfce_H = get_option_value ("tfce_h", DEFAULT_TFCE_H);
  const value_type tfce_E = get_option_value ("tfce_e", DEFAULT_TFCE_E);
  const bool use_tfce = !std::isfinite (cluster_forming_threshold);
  const int num_perms = get_option_value ("nperms", DEFAULT_PERMUTATIONS);
  const int nperms_nonstationary = get_option_value ("nperms_nonstationary", DEFAULT_PERMUTATIONS_NONSTATIONARITY);
  
  const bool do_26_connectivity = get_options("connectivity").size();
  const bool do_nonstationary_adjustment = get_options ("nonstationary").size();

  // Read filenames
  std::vector<std::string> subjects;
  {
    std::string folder = Path::dirname (argument[0]);
    std::ifstream ifs (argument[0].c_str());
    std::string temp;
    while (getline (ifs, temp))
      subjects.push_back (Path::join (folder, temp));
  }

  // Load design matrix:
  const matrix_type design = load_matrix (argument[1]);
  if (design.rows() != (ssize_t)subjects.size())
    throw Exception ("number of input files does not match number of rows in design matrix");

  // Load contrast matrix:
  const matrix_type contrast = load_matrix (argument[2]);
  if (contrast.cols() != design.cols())
    throw Exception ("the number of contrasts does not equal the number of columns in the design matrix");

  // Load Mask
  auto header = Header::open (argument[3]);
  auto mask_image = header.get_image<bool>();

  Filter::Connector connector (do_26_connectivity);
  std::vector<std::vector<int> > mask_indices = connector.precompute_adjacency (mask_image);

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
      std::vector<std::vector<int> >::iterator it;
      for (it = mask_indices.begin(); it != mask_indices.end(); ++it) {
        input_image.index(0) = (*it)[0];
        input_image.index(1) = (*it)[1];
        input_image.index(2) = (*it)[2];
        data (index++, subject) = input_image.value();
      }
      progress++;
    }
  }

  Header output_header (header);
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

  auto cluster_image = Image<float>::create (prefix + (use_tfce ? "tfce.mif" : "cluster_sizes.mif"), output_header);
  auto tvalue_image = Image<float>::create (prefix + "tvalue.mif", output_header);
  auto fwe_pvalue_image = Image<float>::create (prefix + "fwe_pvalue.mif", output_header);
  auto uncorrected_pvalue_image = Image<float>::create (prefix + "uncorrected_pvalue.mif", output_header);
  Image<float> cluster_image_neg;
  Image<float> fwe_pvalue_image_neg;
  Image<float> uncorrected_pvalue_image_neg;

  vector_type perm_distribution (num_perms);
  std::shared_ptr<vector_type> perm_distribution_neg;
  vector_type default_cluster_output (num_vox);
  std::shared_ptr<vector_type> default_cluster_output_neg;
  vector_type tvalue_output (num_vox);
  vector_type empirical_enhanced_statistic;
  vector_type uncorrected_pvalue (num_vox);
  std::shared_ptr<vector_type> uncorrected_pvalue_neg;


  const bool compute_negative_contrast = get_options("negative").size() ? true : false;
  if (compute_negative_contrast) {
    cluster_image_neg = Image<float>::create (prefix + (use_tfce ? "tfce_neg.mif" : "cluster_sizes_neg.mif"), output_header);
    perm_distribution_neg.reset (new vector_type (num_perms));
    default_cluster_output_neg.reset (new vector_type (num_vox));
    fwe_pvalue_image_neg = Image<float>::create (prefix + "fwe_pvalue_neg.mif", output_header);
    uncorrected_pvalue_neg.reset (new vector_type (num_vox));
    uncorrected_pvalue_image_neg = Image<float>::create (prefix + "uncorrected_pvalue_neg.mif", output_header);
  }

  std::shared_ptr<Stats::EnhancerBase> enhancer;
  if (use_tfce)
    enhancer.reset (new Stats::TFCE::Enhancer (connector, tfce_dh, tfce_E, tfce_H));
  else
    enhancer.reset (new Stats::Cluster::ClusterSize (connector, cluster_forming_threshold));

  { // Do permutation testing:
    Math::Stats::GLMTTest glm (data, design, contrast);

    if (do_nonstationary_adjustment) {
      if (!use_tfce)
        throw Exception ("nonstationary adjustment is not currently implemented for threshold-based cluster analysis");
      Stats::PermTest::precompute_empirical_stat (glm, enhancer, nperms_nonstationary, empirical_enhanced_statistic);
    }

    Stats::PermTest::precompute_default_permutation (glm, enhancer, empirical_enhanced_statistic,
                                                     default_cluster_output, default_cluster_output_neg, tvalue_output);

    Stats::PermTest::run_permutations (glm, enhancer, num_perms, empirical_enhanced_statistic,
                                       default_cluster_output, default_cluster_output_neg,
                                       perm_distribution, perm_distribution_neg,
                                       uncorrected_pvalue, uncorrected_pvalue_neg);
  }

  save_matrix (perm_distribution, prefix + "perm_dist.txt");

  vector_type pvalue_output (num_vox);
  Math::Stats::Permutation::statistic2pvalue (perm_distribution, default_cluster_output, pvalue_output);
  {
    ProgressBar progress ("generating output", num_vox);
    for (size_t i = 0; i < num_vox; i++) {
      for (size_t dim = 0; dim < cluster_image.ndim(); dim++)
        tvalue_image.index(dim) = cluster_image.index(dim) = fwe_pvalue_image.index(dim) = uncorrected_pvalue_image.index(dim) = mask_indices[i][dim];
      tvalue_image.value() = tvalue_output[i];
      cluster_image.value() = default_cluster_output[i];
      fwe_pvalue_image.value() = pvalue_output[i];
      uncorrected_pvalue_image.value() = uncorrected_pvalue[i];
      ++progress;
    }
  }

  if (compute_negative_contrast) {
    save_matrix (*perm_distribution_neg, prefix + "perm_dist_neg.txt");
    vector_type pvalue_output_neg (num_vox);
    Math::Stats::Permutation::statistic2pvalue (*perm_distribution_neg, *default_cluster_output_neg, pvalue_output_neg);

    ProgressBar progress ("generating negative contrast output", num_vox);
    for (size_t i = 0; i < num_vox; i++) {
      for (size_t dim = 0; dim < cluster_image.ndim(); dim++)
        cluster_image_neg.index(dim) = fwe_pvalue_image_neg.index(dim) = uncorrected_pvalue_image_neg.index(dim) = mask_indices[i][dim];
      cluster_image_neg.value() = (*default_cluster_output_neg)[i];
      fwe_pvalue_image_neg.value() = pvalue_output_neg[i];
      uncorrected_pvalue_image_neg.value() = (*uncorrected_pvalue_neg)[i];
      ++progress;
    }
  }

}

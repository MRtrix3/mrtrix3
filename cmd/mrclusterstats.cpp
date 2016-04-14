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
#include "math/stats/permutation.h"
#include "math/stats/glm.h"
#include "stats/tfce.h"
#include "stats/cluster.h"
#include "stats/permtest.h"


using namespace MR;
using namespace App;


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


typedef Stats::TFCE::value_type value_type;


void run() {

  value_type cluster_forming_threshold = get_option_value ("threshold", NAN);
  value_type tfce_dh = get_option_value ("tfce_dh", DEFAULT_TFCE_DH);
  value_type tfce_H = get_option_value ("tfce_h", DEFAULT_TFCE_H);
  value_type tfce_E = get_option_value ("tfce_e", DEFAULT_TFCE_E);
  int num_perms = get_option_value ("nperms", DEFAULT_PERMUTATIONS);
  int nperms_nonstationary = get_option_value ("nperms_nonstationary", DEFAULT_PERMUTATIONS_NONSTATIONARITY);
  
  bool do_26_connectivity = get_options("connectivity").size();
  bool do_nonstationary_adjustment = get_options ("nonstationary").size();

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
  Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> design = load_matrix<value_type> (argument[1]);
  if (design.rows() != (ssize_t)subjects.size())
    throw Exception ("number of input files does not match number of rows in design matrix");

  // Load contrast matrix:
  Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> contrast = load_matrix<value_type> (argument[2]);
  if (contrast.cols() != design.cols())
    throw Exception ("the number of contrasts does not equal the number of columns in the design matrix");

  // Load Mask
  auto header = Header::open (argument[3]);
  auto mask_image = header.get_image<value_type>();

  Filter::Connector connector (do_26_connectivity);
  std::vector<std::vector<int> > mask_indices = connector.precompute_adjacency (mask_image);

  const size_t num_vox = mask_indices.size();
  Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> data (num_vox, subjects.size());


  {
    // Load images
    ProgressBar progress("loading images", subjects.size());
    for (size_t subject = 0; subject < subjects.size(); subject++) {
      LogLevelLatch log_level (0);
      auto input_image = Image<float>::open(subjects[subject]).with_direct_io (3);
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
  if (std::isfinite (cluster_forming_threshold)) {
    output_header.keyval()["threshold"] = str(cluster_forming_threshold);
  } else {
    output_header.keyval()["tfce_dh"] = str(tfce_dh);
    output_header.keyval()["tfce_e"] = str(tfce_E);
    output_header.keyval()["tfce_h"] = str(tfce_H);
  }

  std::string prefix (argument[4]);

  std::string cluster_name (prefix);
  if (std::isfinite (cluster_forming_threshold))
     cluster_name.append ("cluster_sizes.mif");
  else
    cluster_name.append ("tfce.mif");

  auto cluster_image = Image<value_type>::create (cluster_name, output_header);
  auto tvalue_image = Image<value_type>::create (prefix + "tvalue.mif", output_header);
  auto fwe_pvalue_image = Image<value_type>::create (prefix + "fwe_pvalue.mif", output_header);
  auto uncorrected_pvalue_image = Image<value_type>::create (prefix + "uncorrected_pvalue.mif", output_header);
  Image<value_type> cluster_image_neg;
  Image<value_type> fwe_pvalue_image_neg;
  Image<value_type> uncorrected_pvalue_image_neg;

  Eigen::Matrix<value_type, Eigen::Dynamic, 1> perm_distribution (num_perms);
  std::shared_ptr<Eigen::Matrix<value_type, Eigen::Dynamic, 1> > perm_distribution_neg;
  std::vector<value_type> default_cluster_output (num_vox, 0.0);
  std::shared_ptr<std::vector<value_type> > default_cluster_output_neg;
  std::vector<value_type> tvalue_output (num_vox, 0.0);
  std::shared_ptr<std::vector<double> > empirical_tfce_statistic;
  std::vector<value_type> uncorrected_pvalue (num_vox, 0.0);
  std::shared_ptr<std::vector<value_type> > uncorrected_pvalue_neg;


  bool compute_negative_contrast = get_options("negative").size() ? true : false;
  if (compute_negative_contrast) {
    std::string cluster_neg_name (prefix);
    if (std::isfinite (cluster_forming_threshold))
       cluster_neg_name.append ("cluster_sizes_neg.mif");
    else
      cluster_neg_name.append ("tfce_neg.mif");
    cluster_image_neg = Image<value_type>::create (cluster_neg_name, output_header);
    perm_distribution_neg.reset (new Eigen::Matrix<value_type, Eigen::Dynamic, 1> (num_perms));
    default_cluster_output_neg.reset (new std::vector<value_type> (num_vox, 0.0));
    fwe_pvalue_image_neg = Image<value_type>::create (prefix + "fwe_pvalue_neg.mif", output_header);
    uncorrected_pvalue_neg.reset (new std::vector<value_type> (num_vox, 0.0));
    uncorrected_pvalue_image_neg = Image<value_type>::create (prefix + "uncorrected_pvalue_neg.mif", output_header);
  }

  { // Do permutation testing:
    Math::Stats::GLMTTest glm (data, design, contrast);

    // Suprathreshold clustering
    if (std::isfinite (cluster_forming_threshold)) {
      if (do_nonstationary_adjustment)
        throw Exception ("nonstationary adjustment is not currently implemented for threshold-based cluster analysis");
      Stats::Cluster::ClusterSize cluster_size_test (connector, cluster_forming_threshold);

      Stats::PermTest::precompute_default_permutation (glm, cluster_size_test, empirical_tfce_statistic,
                                                       default_cluster_output, default_cluster_output_neg, tvalue_output);



      Stats::PermTest::run_permutations (glm, cluster_size_test, num_perms, empirical_tfce_statistic,
                                         default_cluster_output, default_cluster_output_neg,
                                         perm_distribution, perm_distribution_neg,
                                         uncorrected_pvalue, uncorrected_pvalue_neg);
    // TFCE
    } else {
      Stats::TFCE::Enhancer tfce_integrator (connector, tfce_dh, tfce_E, tfce_H);
      if (do_nonstationary_adjustment) {
        empirical_tfce_statistic.reset (new std::vector<double> (num_vox, 0.0));
        Stats::PermTest::precompute_empirical_stat (glm, tfce_integrator, nperms_nonstationary, *empirical_tfce_statistic);
      }

      Stats::PermTest::precompute_default_permutation (glm, tfce_integrator, empirical_tfce_statistic,
                                                       default_cluster_output, default_cluster_output_neg, tvalue_output);

      Stats::PermTest::run_permutations (glm, tfce_integrator, num_perms, empirical_tfce_statistic,
                                         default_cluster_output, default_cluster_output_neg,
                                         perm_distribution, perm_distribution_neg,
                                         uncorrected_pvalue, uncorrected_pvalue_neg);
    }
  }

  save_matrix (perm_distribution, prefix + "perm_dist.txt");

  std::vector<value_type> pvalue_output (num_vox, 0.0);
  Math::Stats::statistic2pvalue (perm_distribution, default_cluster_output, pvalue_output);
  {
    ProgressBar progress ("generating output");
    for (size_t i = 0; i < num_vox; i++) {
      for (size_t dim = 0; dim < cluster_image.ndim(); dim++)
        tvalue_image.index(dim) = cluster_image.index(dim) = fwe_pvalue_image.index(dim) = uncorrected_pvalue_image.index(dim) = mask_indices[i][dim];
      tvalue_image.value() = tvalue_output[i];
      cluster_image.value() = default_cluster_output[i];
      fwe_pvalue_image.value() = pvalue_output[i];
      uncorrected_pvalue_image.value() = uncorrected_pvalue[i];
    }
  }
  {
    if (compute_negative_contrast) {
      save_matrix (*perm_distribution_neg, prefix + "perm_dist_neg.txt");
      std::vector<value_type> pvalue_output_neg (num_vox, 0.0);
      Math::Stats::statistic2pvalue (*perm_distribution_neg, *default_cluster_output_neg, pvalue_output_neg);

      ProgressBar progress ("generating negative contrast output");
      for (size_t i = 0; i < num_vox; i++) {
        for (size_t dim = 0; dim < cluster_image.ndim(); dim++)
          cluster_image_neg.index(dim) = fwe_pvalue_image_neg.index(dim) = uncorrected_pvalue_image_neg.index(dim) = mask_indices[i][dim];
        cluster_image_neg.value() = (*default_cluster_output_neg)[i];
        fwe_pvalue_image_neg.value() = pvalue_output_neg[i];
        uncorrected_pvalue_image_neg.value() = (*uncorrected_pvalue_neg)[i];
      }
    }

  }
}

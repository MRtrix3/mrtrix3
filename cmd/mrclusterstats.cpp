/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 07/11/11.

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
#include "file/path.h"
#include "image/loop.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
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


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Voxel-based analysis using permutation testing and threshold-free cluster enhancement.";

  REFERENCES = "If not using the -threshold command-line option: \n"
               "Smith, S. M. & Nichols, T. E. "
               "Threshold-free cluster enhancement: Addressing problems of smoothing, threshold dependence and localisation in cluster inference. "
               "NeuroImage, 2009, 44, 83-98 \n\n"
               "If using the -nonstationary option: \n"
               "Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. Adjusting the effect of nonstationarity in cluster-based and TFCE inference. \n"
               "Neuroimage, 2011, 54(3), 2006-19\n" ;


  ARGUMENTS
  + Argument ("input", "a text file containing the file names of the input images, one file per line").type_file_in()

  + Argument ("design", "the design matrix, rows should correspond with images in the input image text file").type_file_in()

  + Argument ("contrast", "the contrast matrix, only specify one contrast as it will automatically compute the opposite contrast.").type_file_in()

  + Argument ("mask", "a mask used to define voxels included in the analysis.").type_image_in()

  + Argument ("output", "the filename prefix for all output.").type_text();


  OPTIONS
  + Option ("negative", "automatically test the negative (opposite) contrast. By computing the opposite contrast simultaneously "
                        "the computation time is reduced.")

  + Option ("nperms", "the number of permutations (default = 5000).")
  +   Argument ("num").type_integer (1, 5000, 100000)

  + Option ("threshold", "the cluster-forming threshold to use for a standard cluster-based analysis. "
      "This disables TFCE, which is the default otherwise.")
  +   Argument ("value").type_float (1.0e-6, 3.0, 1.0e6)

  + Option ("tfce_dh", "the height increment used in the TFCE integration (default = 0.1)")
  +   Argument ("value").type_float (0.001, 0.1, 100000)

  + Option ("tfce_e", "TFCE height parameter (default = 2)")
  +   Argument ("value").type_float (0.001, 2.0, 100000)

  + Option ("tfce_h", "TFCE extent parameter (default = 0.5)")
  +   Argument ("value").type_float (0.001, 0.5, 100000)

  + Option ("connectivity", "use 26 neighbourhood connectivity (Default: 6)")

  + Option ("nonstationary", "perform non-stationarity correction (currently only implemented with tfce)")

  + Option ("nperms_nonstationary", "the number of permutations used when precomputing the empirical statistic image for nonstationary correction (Default: 5000)")
  +   Argument ("num").type_integer (1, 5000, 100000);

}


typedef Stats::TFCE::value_type value_type;


void run() {

  Options opt = get_options ("threshold");
  value_type cluster_forming_threshold = NAN;
  if (opt.size())
    cluster_forming_threshold = opt[0][0];

  opt = get_options ("tfce_dh");
  value_type tfce_dh = 0.1;
  if (opt.size())
    tfce_dh = opt[0][0];

  opt = get_options ("tfce_h");
  value_type tfce_H = 2.0;
  if (opt.size())
    tfce_H = opt[0][0];

  opt = get_options ("tfce_e");
  value_type tfce_E = 0.5;
  if (opt.size())
    tfce_E = opt[0][0];

  opt = get_options ("nperms");
  int num_perms = 5000;
  if (opt.size())
    num_perms = opt[0][0];

  opt = get_options ("nperms_nonstationary");
  int nperms_nonstationary = 5000;
  if (opt.size())
    num_perms = opt[0][0];


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
  Math::Matrix<value_type> design;
  design.load (argument[1]);

  if (design.rows() != subjects.size())
    throw Exception ("number of subjects does not match number of rows in design matrix");

  // Load contrast matrix:
  Math::Matrix<value_type> contrast;
  contrast.load (argument[2]);

  if (contrast.columns() > design.columns())
    throw Exception ("too many contrasts for design matrix");
  contrast.resize (contrast.rows(), design.columns());

  // Load Mask
  Image::Header header (argument[3]);
  Image::Buffer<value_type> mask_data (header);
  auto mask_vox = mask_data.voxel();

  Image::Filter::Connector connector (do_26_connectivity);
  std::vector<std::vector<int> > mask_indices = connector.precompute_adjacency (mask_vox);

  const size_t num_vox = mask_indices.size();
  Math::Matrix<value_type> data (num_vox, subjects.size());


  {
    // Load images
    ProgressBar progress("loading images...", subjects.size());
    for (size_t subject = 0; subject < subjects.size(); subject++) {
      LogLevelLatch log_level (0);
      Image::BufferPreload<value_type> input_buffer (subjects[subject], Image::Stride::contiguous_along_axis (3));
      Image::check_dimensions (input_buffer, mask_vox, 0, 3);
      auto input_vox = input_buffer.voxel();
      int index = 0;
      std::vector<std::vector<int> >::iterator it;
      for (it = mask_indices.begin(); it != mask_indices.end(); ++it) {
        input_vox[0] = (*it)[0];
        input_vox[1] = (*it)[1];
        input_vox[2] = (*it)[2];
        data (index++, subject) = input_vox.value();
      }
      progress++;
    }
  }

  header.datatype() = DataType::Float32;
  Image::Header output_header (header);
  output_header.comments().push_back("num permutations = " + str(num_perms));
  output_header.comments().push_back("tfce_dh = " + str(tfce_dh));
  output_header.comments().push_back("tfce_e = " + str(tfce_E));
  output_header.comments().push_back("tfce_h = " + str(tfce_H));
  output_header.comments().push_back("26 connectivity = " + str(do_26_connectivity));
  output_header.comments().push_back("nonstationary adjustment = " + str(do_nonstationary_adjustment));

  std::string prefix (argument[4]);

  std::string cluster_name (prefix);
  if (std::isfinite (cluster_forming_threshold))
     cluster_name.append ("clusters.mif");
  else
    cluster_name.append ("tfce.mif");

  Image::Buffer<value_type> cluster_data (cluster_name, output_header);
  Image::Buffer<value_type> tvalue_data (prefix + "tvalue.mif", output_header);
  Image::Buffer<value_type> fwe_pvalue_data (prefix + "fwe_pvalue.mif", output_header);
  Image::Buffer<value_type> uncorrected_pvalue_data (prefix + "uncorrected_pvalue.mif", output_header);
  RefPtr<Image::Buffer<value_type> > cluster_data_neg;
  RefPtr<Image::Buffer<value_type> > fwe_pvalue_data_neg;
  RefPtr<Image::Buffer<value_type> > uncorrected_pvalue_data_neg;


  Math::Vector<value_type> perm_distribution (num_perms);
  RefPtr<Math::Vector<value_type> > perm_distribution_neg;
  std::vector<value_type> default_cluster_output (num_vox, 0.0);
  RefPtr<std::vector<value_type> > default_cluster_output_neg;
  std::vector<value_type> tvalue_output (num_vox, 0.0);
  RefPtr<std::vector<double> > empirical_tfce_statistic;
  std::vector<value_type> uncorrected_pvalue (num_vox, 0.0);
  RefPtr<std::vector<value_type> > uncorrected_pvalue_neg;


  bool compute_negative_contrast = get_options("negative").size() ? true : false;
  if (compute_negative_contrast) {
    std::string cluster_neg_name (prefix);
    if (std::isfinite (cluster_forming_threshold))
       cluster_neg_name.append ("clusters_neg.mif");
    else
      cluster_neg_name.append ("tfce_neg.mif");
    cluster_data_neg = new Image::Buffer<value_type> (cluster_neg_name, output_header);
    perm_distribution_neg = new Math::Vector<value_type> (num_perms);
    default_cluster_output_neg = new std::vector<value_type> (num_vox, 0.0);
    fwe_pvalue_data_neg = new Image::Buffer<value_type> (prefix + "fwe_pvalue_neg.mif", output_header);
    uncorrected_pvalue_neg = new std::vector<value_type> (num_vox, 0.0);
    uncorrected_pvalue_data_neg = new Image::Buffer<value_type> (prefix + "uncorrected_pvalue_neg.mif", output_header);
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
        empirical_tfce_statistic = new std::vector<double> (num_vox, 0.0);
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

  perm_distribution.save (prefix + "perm_dist.txt");

  std::vector<value_type> pvalue_output (num_vox, 0.0);
  Math::Stats::statistic2pvalue (perm_distribution, default_cluster_output, pvalue_output);
  Image::Buffer<value_type>::voxel_type cluster_voxel (cluster_data);
  Image::Buffer<value_type>::voxel_type tvalue_voxel (tvalue_data);
  Image::Buffer<value_type>::voxel_type fwe_pvalue_voxel (fwe_pvalue_data);
  Image::Buffer<value_type>::voxel_type uncorrected_pvalue_voxel (uncorrected_pvalue_data);
  {
    ProgressBar progress ("generating output...");
    for (size_t i = 0; i < num_vox; i++) {
      for (size_t dim = 0; dim < cluster_voxel.ndim(); dim++)
        tvalue_voxel[dim] = cluster_voxel[dim] = fwe_pvalue_voxel[dim] = uncorrected_pvalue_voxel[dim] = mask_indices[i][dim];
      tvalue_voxel.value() = tvalue_output[i];
      cluster_voxel.value() = default_cluster_output[i];
      fwe_pvalue_voxel.value() = pvalue_output[i];
      uncorrected_pvalue_voxel.value() = uncorrected_pvalue[i];
    }
  }
  {
    if (compute_negative_contrast) {
      (*perm_distribution_neg).save (prefix + "perm_dist_neg.txt");
      std::vector<value_type> pvalue_output_neg (num_vox, 0.0);
      Math::Stats::statistic2pvalue (*perm_distribution_neg, *default_cluster_output_neg, pvalue_output_neg);
      Image::Buffer<value_type>::voxel_type cluster_voxel_neg (*cluster_data_neg);
      Image::Buffer<value_type>::voxel_type fwe_pvalue_voxel_neg (*fwe_pvalue_data_neg);
      Image::Buffer<value_type>::voxel_type uncorrected_pvalue_voxel_neg (*uncorrected_pvalue_data_neg);

      ProgressBar progress ("generating negative contrast output...");
      for (size_t i = 0; i < num_vox; i++) {
        for (size_t dim = 0; dim < cluster_voxel.ndim(); dim++)
          cluster_voxel_neg[dim] = fwe_pvalue_voxel_neg[dim] = uncorrected_pvalue_voxel_neg[dim] = mask_indices[i][dim];
        cluster_voxel_neg.value() = (*default_cluster_output_neg)[i];
        fwe_pvalue_voxel_neg.value() = pvalue_output_neg[i];
        uncorrected_pvalue_voxel_neg.value() = (*uncorrected_pvalue_neg)[i];

      }
    }

  }
}

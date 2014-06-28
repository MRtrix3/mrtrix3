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


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Voxel-based analysis using permutation testing and threshold-free cluster enhancement.";

  REFERENCES = "If not using the -threshold command-line option: \n"
               "Smith, S. M. & Nichols, T. E. "
               "Threshold-free cluster enhancement: Addressing problems of smoothing, threshold dependence and localisation in cluster inference. "
               "NeuroImage, 2009, 44, 83-98 \n";


  ARGUMENTS
  + Argument ("input", "a text file containing the file names of the input images, one file per line").type_file()

  + Argument ("design", "the design matrix, rows should correspond with images in the input image text file").type_file()

  + Argument ("contrast", "the contrast matrix, only specify one contrast as it will automatically compute the opposite contrast.").type_file()

  + Argument ("mask", "a mask used to define voxels included in the analysis.").type_image_in()

  + Argument ("output", "the filename prefix for all output.").type_text();


  OPTIONS
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

  + Option ("stationary", "do not perform non-stationarity correction");
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

  bool do_26_connectivity = get_options("connectivity").size();

  bool do_nonstationary_adjustment = !get_options ("nonstationary").size();

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
  Image::Buffer<value_type>::voxel_type mask_vox (mask_data);

  Image::Filter::Connector connector (do_26_connectivity);
  std::vector<std::vector<int> > mask_indices = connector.precompute_adjacency (mask_vox);

  const size_t num_vox = mask_indices.size();
  Math::Matrix<value_type> data (num_vox, subjects.size());


  // Load images
  ProgressBar progress("loading images...", subjects.size());
  for (size_t subject = 0; subject < subjects.size(); subject++) {
    LogLevelLatch log_level (0);
    Image::BufferPreload<value_type> fod_data (subjects[subject], Image::Stride::contiguous_along_axis (3));
    Image::check_dimensions (fod_data, mask_vox, 0, 3);
    Image::BufferPreload<value_type>::voxel_type input_vox (fod_data);
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

  header.datatype() = DataType::Float32;
  std::string prefix (argument[4]);

  std::string cluster_pos (prefix);
  std::string cluster_neg (prefix);
  if (std::isfinite (cluster_forming_threshold)) {
     cluster_pos.append ("_clusters_pos.mif");
     cluster_neg.append ("_clusters_neg.mif");
  } else {
    cluster_pos.append ("_tfce_pos.mif");
    cluster_neg.append ("_tfce_neg.mif");
  }

  Image::Header output_header (header);
  output_header.comments().push_back("num permutations = " + str(num_perms));
  output_header.comments().push_back("tfce_dh = " + str(dh));
  output_header.comments().push_back("tfce_e = " + str(cfe_E));
  output_header.comments().push_back("tfce_h = " + str(cfe_H));
  output_header.comments().push_back("26 connectivity = " + str(do_26_connectivity));
  output_header.comments().push_back("nonstationary adjustment = " + str(do_nonstationary_adjustment));

  Image::Buffer<value_type> cluster_data_pos (cluster_pos, output_header);
  Image::Buffer<value_type> cluster_data_neg (cluster_neg, output_header);
  Image::Buffer<value_type> tvalue_data (prefix + "_tvalue.mif", output_header);
  Image::Buffer<value_type> pvalue_data_pos (prefix + "_pvalue_pos.mif", output_header);
  Image::Buffer<value_type> pvalue_data_neg (prefix + "_pvalue_neg.mif", output_header);

  Math::Vector<value_type> perm_distribution_pos (num_perms - 1);
  Math::Vector<value_type> perm_distribution_neg (num_perms - 1);
  std::vector<value_type> cluster_output_pos (num_vox, 0.0);
  std::vector<value_type> cluster_output_neg (num_vox, 0.0);
  std::vector<value_type> pvalue_output_pos (num_vox, 0.0);
  std::vector<value_type> pvalue_output_neg (num_vox, 0.0);
  std::vector<value_type> tvalue_output (num_vox, 0.0);

  std::vector<value_type> empirical_statistic (num_vox, 0.0);

  { // Do permutation testing:
    Math::Stats::GLMTTest glm (data, design, contrast);
    if (std::isfinite (cluster_forming_threshold)) {
      Stats::TFCE::ClusterSize cluster_size_test (connector, cluster_forming_threshold);
      Stats::TFCE::run (glm, cluster_size_test, num_perms, do_nonstationary_adjustment, empirical_statistic,
                        perm_distribution_pos, perm_distribution_neg,
                        cluster_output_pos, cluster_output_neg, tvalue_output);
    }
    else { // TFCE

      Stats::TFCE::Spatial tfce_integrator (connector, tfce_dh, tfce_E, tfce_H);
      Stats::TFCE::run (glm, tfce_integrator, num_perms, do_nonstationary_adjustment, empirical_statistic,
                        perm_distribution_pos, perm_distribution_neg,
                        cluster_output_pos, cluster_output_neg, tvalue_output);
    }
  }

  perm_distribution_pos.save (prefix + "_perm_dist_pos.txt");
  perm_distribution_neg.save (prefix + "_perm_dist_neg.txt");
  Math::Stats::statistic2pvalue (perm_distribution_pos, cluster_output_pos, pvalue_output_pos);
  Math::Stats::statistic2pvalue (perm_distribution_neg, cluster_output_neg, pvalue_output_neg);

  Image::Buffer<value_type>::voxel_type cluster_voxel_pos (cluster_data_pos);
  Image::Buffer<value_type>::voxel_type cluster_voxel_neg (cluster_data_neg);
  Image::Buffer<value_type>::voxel_type tvalue_voxel (tvalue_data);
  Image::Buffer<value_type>::voxel_type pvalue_voxel_pos (pvalue_data_pos);
  Image::Buffer<value_type>::voxel_type pvalue_voxel_neg (pvalue_data_neg);


  {
    ProgressBar progress ("generating output...");
    for (size_t i = 0; i < num_vox; i++) {
      for (size_t dim = 0; dim < cluster_voxel_pos.ndim(); dim++)
        tvalue_voxel[dim] = cluster_voxel_pos[dim] = cluster_voxel_neg[dim] = pvalue_voxel_pos[dim] = pvalue_voxel_neg[dim] = mask_indices[i][dim];
      tvalue_voxel.value() = tvalue_output[i];
      cluster_voxel_pos.value() = cluster_output_pos[i];
      cluster_voxel_neg.value() = cluster_output_neg[i];
      pvalue_voxel_pos.value() = pvalue_output_pos[i];
      pvalue_voxel_neg.value() = pvalue_output_neg[i];
    }
  }
}

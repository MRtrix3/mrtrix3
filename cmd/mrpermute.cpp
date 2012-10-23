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

#include "app.h"
#include "image/loop.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "math/SH.h"
#include "math/hemisphere/directions.h"
#include "timer.h"
#include "stats/permute.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

class PermutationItem
{
  public:
    std::vector<size_t> labelling;
    size_t index;
};

typedef Thread::Queue<PermutationItem> Queue;
typedef float value_type;

class PermutationQueueLoader
{
 public:
   PermutationQueueLoader (size_t num_perms, size_t num_subjects) :
     num_perms (num_perms),
     num_subjects (num_subjects),
     current_perm (0),
     progress ("running " + str(num_perms) + " permutations...", num_perms) { }

   bool operator() (PermutationItem& item) {
     if (current_perm >= num_perms)
       return false;

     item.index = current_perm;
     for (size_t i = 0; i < num_subjects; ++i)
       item.labelling.push_back(i);

     if (current_perm) {
       do {
         std::random_shuffle (item.labelling.begin(), item.labelling.end());
         // TODO: check that permutation is distinct from original
       } while (MR::Stats::is_duplicate_permutation (item.labelling, previous_perms));
       // TODO: check whether we need to check for duplicate
       // i.e. can we sample with replacement?
     }
     previous_perms.push_back (item.labelling);
     ++current_perm;
     ++progress;

     return true;
   }

 private:

   std::vector <std::vector<size_t> > previous_perms;
   size_t num_perms, num_subjects, current_perm;
   ProgressBar progress;
};

class TFCEProcessorSpatialCluster : public MR::Stats::TFCEProcessorBase
{
  public:
    TFCEProcessorSpatialCluster (Ptr<Image::Filter::Connector<Image::Buffer<value_type>::voxel_type> > connector,
                                 Math::Vector<value_type>& perm_distribution_pos, Math::Vector<value_type>& perm_distribution_neg,
                                 const Math::Matrix<value_type>& afd,
                                 const Math::Matrix<value_type>& design_matrix, const Math::Matrix<value_type>& contrast_matrix,
                                 value_type dh, value_type E, value_type H,
                                 std::vector<value_type>& tfce_output_pos, std::vector<value_type>& tfce_output_neg,
                                 std::vector<value_type>& tvalue_output) :
                                   TFCEProcessorBase (perm_distribution_pos, perm_distribution_neg, afd, design_matrix, contrast_matrix,
                                                      dh, E, H, tfce_output_pos, tfce_output_neg, tvalue_output),
                                                      connector (connector) {}

    bool operator() (const PermutationItem& item)
    {
      std::vector<value_type> stats (afd.rows(), 0.0);
      std::vector<value_type> tfce_stats (afd.rows(), 0.0);
      value_type max_stat = 0.0, min_stat = 0.0;

      compute_tstatistics (item.labelling, stats, max_stat, min_stat);
      if (item.index == 0)
        tvalue_output = stats;

      value_type max_tfce_stat = tfce_integration (max_stat, stats, tfce_stats);
      if (item.index == 0)
        tfce_output_pos = tfce_stats;
      else
        perm_distribution_pos[item.index - 1] = max_tfce_stat;

      for (size_t i = 0; i < afd.rows(); ++i) {
        stats[i] = -stats[i];
        tfce_stats[i] = 0.0;
      }
      max_tfce_stat = tfce_integration (-min_stat, stats, tfce_stats);
      if (item.index == 0)
        tfce_output_neg = tfce_stats;
      else
        perm_distribution_neg[item.index - 1] = max_tfce_stat;

      return true;
    }

  private:

    value_type tfce_integration (const value_type max_stat, const std::vector<value_type>& stats, std::vector<value_type>& tfce_stats)
    {
      for (value_type threshold = dh; threshold < max_stat; threshold += dh) {
        std::vector<Image::Filter::cluster> clusters;
        std::vector<uint32_t> labels (afd.rows(), 0);
        connector->run (clusters, labels, stats, threshold);

        for (size_t i = 0; i < afd.rows(); ++i)
          if (labels[i])
            tfce_stats[i] += pow (clusters[labels[i]-1].size, E) * pow (threshold, H);
      }

      value_type max_tfce_stat = 0.0;
      for (size_t i = 0; i < afd.rows(); i++)
        if (tfce_stats[i] > max_tfce_stat)
          max_tfce_stat = tfce_stats[i];

      return max_tfce_stat;
    }

    Ptr<Image::Filter::Connector<Image::Buffer<value_type>::voxel_type> > connector;
};



void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Voxel-based analysis using permutation testing and threshold-free cluster enhancement.";


  ARGUMENTS
  + Argument ("input", "a text file containing the file names of the input images").type_file()

  + Argument ("design", "the design matrix").type_file()

  + Argument ("contrast", "the contrast matrix").type_file()

  + Argument ("mask", "a mask used to define voxels included in the analysis. "
                      "Note that a 4D mask must be supplied for AFD analysis to "
                      "also define orientations of interest.").type_image_in()

  + Argument ("output", "the root directory and filename prefix "
              "for all output.").type_text();


  OPTIONS
  + Option ("afd", "assume input images are FOD images (i.e. perform AFD voxel-based analysis.")

  + Option ("nperms", "the number of permutations (default = 5000).")
  +   Argument ("num").type_integer (1, 5000, 100000)

  + Option ("dh", "the height increment used in the TFCE integration (default = 0.1)")
  +   Argument ("value").type_float (0.001, 0.1, 100000)

  + Option ("tfce_e", "TFCE height parameter (default = 2)")
  +   Argument ("value").type_float (0.001, 2.0, 100000)

  + Option ("tfce_h", "TFCE extent parameter (default = 0.5)")
  +   Argument ("value").type_float (0.001, 0.5, 100000)

  + Option ("directions", "the directions (corresponding to the input mask) used to sample AFD. "
                          "By default this option is not required providing the direction set can "
                          "be found within the mask image header.")
  +   Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file()

  + Option ("angle", "the angular threshold used to define neighbouring orientations (in degrees)")
  +   Argument ("value").type_float (0.001, 12, 90)

  + Option ("connectivity", "use 26 neighbourhood connectivity (Default: 6)");
}


typedef Stats::value_type value_type;


void run() {

  Options opt = get_options ("dh");
  value_type dh = 0.1;
  if (opt.size())
    dh = opt[0][0];

  opt = get_options ("tfce_h");
  value_type H = 2.0;
  if (opt.size())
    H = opt[0][0];

  opt = get_options ("tfce_e");
  value_type E = 0.5;
  if (opt.size())
    E = opt[0][0];

  opt = get_options ("nperms");
  int num_perms = 5000;
  if (opt.size())
    num_perms = opt[0][0];

  bool do_26_connectivity = get_options("connectivity").size();
  bool do_afd = get_options ("afd").size();

  // Read filenames
  std::vector<std::string> subjects;
  {
    std::string filename = argument[0];
    std::ifstream ifs (filename.c_str());
    std::string temp;
    while (getline (ifs, temp))
      subjects.push_back (temp);
  }

  // Load design matrix:
  Math::Matrix<Stats::value_type> design;
  design.load (argument[1]);

  if (design.rows() != subjects.size())
    throw Exception ("number of subjects does not match number of rows in design matrix");

  // Load contrast matrix:
  Math::Matrix<Stats::value_type> contrast;
  contrast.load (argument[2]);

  if (contrast.columns() > design.columns())
    throw Exception ("too many contrasts for design matrix");
  contrast.resize (contrast.rows(), design.columns());

  // Load Mask
  Image::Header header (argument[3]);
  Image::Buffer<value_type> mask_data (header);
  Image::Buffer<value_type>::voxel_type mask_vox (mask_data);

  Math::Matrix<value_type> directions;
  value_type angular_threshold = 12;

  if (do_afd) {
    opt = get_options ("directions");
    if (opt.size())
      directions.load(opt[0][0]);
    else {
      if (!header["directions"].size())
        throw Exception ("no mask directions have been specified.");
      std::vector<value_type> dir_vector;
      std::vector<std::string> lines = split (header["directions"], "\n", true);
      for (size_t l = 0; l < lines.size(); l++) {
        std::vector<float> v = parse_floats (lines[l]);
        dir_vector.insert (dir_vector.end(), v.begin(), v.end());
      }
      directions.resize (dir_vector.size() / 2, 2);
      for (size_t i = 0; i < dir_vector.size(); i += 2) {
        directions(i/2, 0) = dir_vector[i];
        directions(i/2, 1) = dir_vector[i+1];
      }
    }
    if (int(directions.rows()) != mask_data.dim(3))
      throw Exception ("the number of directions is not equal to the number of 3D volumes within the mask.");

    opt = get_options ("angle");
    if (opt.size())
      angular_threshold = opt[0][0];
  }


  print ("Precomputing voxel adjacency from mask...");
  Ptr<Image::Filter::Connector<Image::Buffer<value_type>::voxel_type> > connector (new Image::Filter::Connector<Image::Buffer<value_type>::voxel_type> (mask_vox, do_26_connectivity));
  if (do_afd)
    connector->set_directions (directions, angular_threshold);
  std::vector<std::vector<int> > mask_indices = connector->precompute_adjacency ();
  print (" done\n");

  const size_t num_vox = mask_indices.size();
  Math::Matrix<Stats::value_type> data (num_vox, subjects.size());


  // Load images
  if (do_afd) {

    Math::Matrix<value_type> SHT;
    Image::Header first_header (subjects[0]);
    Image::check_dimensions(header, first_header, 0, 3);
    Math::SH::init_transform (SHT, directions, Math::SH::LforN (first_header.dim(3)));
    {
      ProgressBar progress("loading FOD images and computing AFD...", subjects.size());
      for (size_t subject = 0; subject < subjects.size(); subject++) {
        LogLevelLatch log_level (0);
        Image::Stride::List strides(4, 0);
        strides[3] = 1;
        Image::BufferPreload<value_type> fod_data (subjects[subject], strides);
        Image::BufferPreload<value_type>::voxel_type fod_voxel (fod_data);
        int index = 0;
        std::vector<std::vector<int> >::iterator it;
        Math::Vector<value_type> fod (fod_voxel.dim(3));
        for (it = mask_indices.begin(); it != mask_indices.end(); ++it) {
          if (fod_voxel[0] != (*it)[0] || fod_voxel[1] != (*it)[1] || fod_voxel[2] != (*it)[2]) {
            fod_voxel[0] = (*it)[0];
            fod_voxel[1] = (*it)[1];
            fod_voxel[2] = (*it)[2];
            for (int sh = 0; sh < fod_voxel.dim(3); sh++) {
              fod_voxel[3] = sh;
              fod[sh] = fod_voxel.value();
            }
          }
          data(index++, subject) = Math::dot (SHT.row ((*it)[3]), fod);
        }
        progress++;
      }
    }

  }
  else {

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
        data(index++, subject) = input_vox.value();
      }
      progress++;
    }
  }

  Math::Vector<value_type> perm_distribution_pos (num_perms - 1);
  Math::Vector<value_type> perm_distribution_neg (num_perms - 1);
  std::vector<value_type> tfce_output_pos (num_vox, 0.0);
  std::vector<value_type> tfce_output_neg (num_vox, 0.0);
  std::vector<value_type> tvalue_output (num_vox, 0.0);


  {
    PermutationQueueLoader permute (num_perms, subjects.size());
    TFCEProcessorSpatialCluster processor (connector, perm_distribution_pos, perm_distribution_neg, data, design, contrast, dh, E, H,
                                                  tfce_output_pos, tfce_output_neg, tvalue_output);
    Thread::run_queue (permute, 1, PermutationItem(), processor, 0);
  }


  header.datatype() = DataType::Float32;
  std::string prefix (argument[4]);

  std::string tfce_filename_pos = prefix + "_tfce_pos.mif";
  Image::Buffer<value_type> tfce_data_pos (tfce_filename_pos, header);
  Image::Buffer<value_type>::voxel_type tfce_voxel_pos (tfce_data_pos);
  std::string tfce_filename_neg = prefix + "_tfce_neg.mif";
  Image::Buffer<value_type> tfce_data_neg (tfce_filename_neg, header);
  Image::Buffer<value_type>::voxel_type tfce_voxel_neg (tfce_data_neg);
  std::string tvalue_filename = prefix + "_tvalue.mif";
  Image::Buffer<value_type> tvalue_data (tvalue_filename, header);
  Image::Buffer<value_type>::voxel_type tvalue_voxel (tvalue_data);

  {
    ProgressBar progress ("generating output...");
    for (size_t i = 0; i < num_vox; i++) {
      for (size_t dim = 0; dim < tfce_voxel_pos.ndim(); dim++)
        tvalue_voxel[dim] = tfce_voxel_pos[dim] = tfce_voxel_neg[dim] = mask_indices[i][dim];
      tvalue_voxel.value() = tvalue_output[i];
      tfce_voxel_pos.value() = tfce_output_pos[i];
      tfce_voxel_neg.value() = tfce_output_neg[i];
    }
    std::string perm_filename_pos = prefix + "_permutation_pos.txt";
    std::string perm_filename_neg = prefix + "_permutation_neg.txt";
    perm_distribution_pos.save (perm_filename_pos);
    perm_distribution_neg.save (perm_filename_neg);

    std::string pvalue_filename_pos = prefix + "_pvalue_pos.mif";
    Image::Buffer<value_type> pvalue_data_pos (pvalue_filename_pos, header);
    Image::Buffer<value_type>::voxel_type pvalue_voxel_pos (pvalue_data_pos);
    Stats::statistic2pvalue (perm_distribution_pos, tfce_voxel_pos, pvalue_voxel_pos);

    std::string pvalue_filename_neg = prefix + "_pvalue_neg.mif";
    Image::Buffer<value_type> pvalue_data_neg (pvalue_filename_neg, header);
    Image::Buffer<value_type>::voxel_type pvalue_voxel_neg (pvalue_data_neg);
    Stats::statistic2pvalue (perm_distribution_neg, tfce_voxel_neg, pvalue_voxel_neg);
  }
}

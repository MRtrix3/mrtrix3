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
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "image/loop.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/filter/connected_components.h"
#include "math/SH.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/hemisphere/directions.h"
#include "timer.h"
#include "stats/permute.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;
using namespace std;


void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Voxel-based analysis using permutation testing and threshold-free cluster enhancement.";


  ARGUMENTS
  + Argument ("input", "a text file containing the file names of the input images").type_file()

  + Argument ("num1", "the number of subjects in group 1").type_integer(0, 10, 10000)

  + Argument ("num2", "the number of subjects in group 2").type_integer(0, 10, 10000)

  + Argument ("mask", "a mask used to define voxels included in the analysis. "
                      "Note that a 4D mask must be supplied for AFD analysis to "
                      "also define orientations of interest.").type_image_in()

  + Argument ("output", "the root directory and filename prefix "
              "for all output.").type_text();


  OPTIONS
  + Option ("afd", "assume input images are FOD images (i.e. perform AFD voxel-based analysis.")

  + Option ("nperms", "the number of permutations (default = 5000).")
  + Argument ("num").type_integer (1, 5000, 100000)

  + Option ("dh", "the height increment used in the TFCE integration (default = 0.1)")
  + Argument ("value").type_float (0.001, 0.1, 100000)

  + Option ("tfce_e", "TFCE height parameter (default = 2)")
  + Argument ("value").type_float (0.001, 2.0, 100000)

  + Option ("tfce_h", "TFCE extent parameter (default = 0.5)")
  + Argument ("value").type_float (0.001, 0.5, 100000)

  + Option ("directions", "the directions (corresponding to the input mask) used to sample AFD. "
                          "By default this option is not required providing the direction set can "
                          "be found within the mask image header.")
  + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file()

  + Option ("angle", "the angular threshold used to define neighbouring orientations (in degrees)")
  + Argument ("value").type_float (0.001, 12, 90)

  + Option ("connectivity", "use 26 neighbourhood connectivity (Default: 6)");
}



void run() {

  int num1 = argument[1];
  int num2 = argument[2];

  Options opt = get_options ("dh");
  float dh = 0.1;
  if (opt.size())
    dh = opt[0][0];

  opt = get_options ("tfce_h");
  float H = 2.0;
  if (opt.size())
    H = opt[0][0];

  opt = get_options ("tfce_e");
  float E = 0.5;
  if (opt.size())
    E = opt[0][0];

  opt = get_options ("nperms");
  int num_perms = 5000;
  if (opt.size())
    num_perms = opt[0][0];

  bool do_26_connectivity = false;
  opt = get_options ("connectivity");
  if (opt.size())
    do_26_connectivity = true;

  bool do_afd = false;
  opt = get_options ("afd");
  if (opt.size())
    do_afd = true;

  // Read filenames
  string filename = argument[0];
  ifstream ifs (filename.c_str());
  vector<string> input_files;
  string temp;
  int num_subjects = 0;
  while (getline (ifs, temp)) {
    input_files.push_back (temp);
    num_subjects++;
  }

  if (num_subjects != num1 + num2)
    throw Exception ("the number of input files does not correspond to the total number of subjects");

  // Load Mask
  Image::Header header (argument[3]);
  Image::Buffer<float> mask_data (header);
  Image::Buffer<float>::voxel_type mask_vox (mask_data);

  Math::Matrix<float> directions;
  float angular_threshold = 12;

  if (do_afd) {
    opt = get_options ("directions");
    if (opt.size()) {
      directions.load(opt[0][0]);
    } else {
      if (!header["directions"].size())
        throw Exception ("no mask directions have been specified.");
      vector<float> dir_vector;
      vector<string > lines = split (header["directions"], "\n", true);
      for (size_t l = 0; l < lines.size(); l++) {
        vector<float> v (parse_floats(lines[l]));
        dir_vector.insert (dir_vector.end(), v.begin(), v.end());
      }
      directions.resize(dir_vector.size() / 2, 2);
      for (size_t i = 0; i < dir_vector.size(); i += 2) {
        directions(i / 2, 0) = dir_vector[i];
        directions(i / 2, 1) = dir_vector[i + 1];
      }
    }
    if (int(directions.rows()) != mask_data.dim(3))
      throw Exception ("the number of directions is not equal to the number of 3D volumes within the mask.");

    opt = get_options ("angle");
    if (opt.size())
      angular_threshold = opt[0][0];
  }


  print ("Precomputing voxel adjacency from mask...");
  Ptr<Image::Filter::Connector<Image::Buffer<float>::voxel_type> > connector (new Image::Filter::Connector<Image::Buffer<float>::voxel_type> (mask_vox, do_26_connectivity));
  if (do_afd)
    connector->set_directions (directions, angular_threshold);
  vector<vector<int> > mask_indices = connector->precompute_adjacency ();
  print (" Done\n");

  size_t num_vox = mask_indices.size();
  vector<vector<float> > data (num_vox, vector<float> (num_subjects));


  // Load images
  if (do_afd) {
    Math::Matrix<float> SHT;
    Image::Header first_header (input_files[0]);
    Image::check_dimensions(header, first_header, 0, 3);
    Math::SH::init_transform (SHT, directions, Math::SH::LforN (first_header.dim(3)));
    {
      ProgressBar progress("loading FOD images and computing AFD...", input_files.size());
      for (uint subject = 0; subject < input_files.size(); subject++) {
        LogLevelLatch log_level (0);
        Image::Stride::List strides(4, 0);
        strides[3] = 1;
        Image::BufferPreload<float> fod_data (input_files[subject], strides);
        Image::BufferPreload<float>::voxel_type fod_voxel (fod_data);
        int index = 0;
        vector<vector<int> >::iterator it;
        Math::Vector<float> fod (fod_voxel.dim(3));
        for (it = mask_indices.begin(); it != mask_indices.end(); ++it) {
          if (fod_voxel[0] != (*it)[0] || fod_voxel[1] != (*it)[1] || fod_voxel[2] != (*it)[2]) {
            fod_voxel[0] = (*it)[0];
            fod_voxel[1] = (*it)[1];
            fod_voxel[2] = (*it)[2];
            for (uint sh = 0; sh < fod_voxel.dim(3); sh++) {
              fod_voxel[3] = sh;
              fod[sh] = fod_voxel.value();
            }
          }
          data[index++][subject] = Math::dot (SHT.row ((*it)[3]), fod);
        }
        progress++;
      }
    }
  } else {
    ProgressBar progress("loading images...", input_files.size());
    for (uint subject = 0; subject < input_files.size(); subject++) {
      LogLevelLatch log_level (0);
      Image::Stride::List strides(4, 0);
      strides[3] = 1;
      Image::BufferPreload<float> fod_data (input_files[subject], strides);
      Image::BufferPreload<float>::voxel_type input_vox (fod_data);
      Image::check_dimensions(input_vox, mask_vox, 0, 3);
      int index = 0;
      vector<vector<int> >::iterator it;
      for (it = mask_indices.begin(); it != mask_indices.end(); ++it) {
        input_vox[0] = (*it)[0];
        input_vox[1] = (*it)[1];
        input_vox[2] = (*it)[2];
        data[index++][subject] = input_vox.value();
      }
      progress++;
    }
  }

  Math::Vector<float> perm_distribution_1 (num_perms - 1);
  Math::Vector<float> perm_distribution_2 (num_perms - 1);
  vector<float> tfce_output_1 (num_vox, 0.0);
  vector<float> tfce_output_2 (num_vox, 0.0);

  {
    ProgressBar progress ("running " + str(num_perms) + " permutations...", num_perms);
    Stats::DataLoader loader (num_perms, num_subjects);
    Stats::Processor processor (connector, perm_distribution_1, perm_distribution_2, data, tfce_output_1, tfce_output_2, num1, dh, E, H, progress);
    Thread::run_queue (loader, 1, MR::Stats::Item(), processor, 0);
  }

  cout << "Generating output..." << flush;

  header.datatype() = DataType::Float32;
  string prefix(argument[4]);

  string tvalue_filename_1 = prefix + "_tfce_1.mif";
  Image::Buffer<float> tfce_data_1 (tvalue_filename_1, header);
  Image::Buffer<float>::voxel_type tfce_voxel_1 (tfce_data_1);
  string tvalue_filename_2 = prefix + "_tfce_2.mif";
  Image::Buffer<float> tfce_data_2 (tvalue_filename_2, header);
  Image::Buffer<float>::voxel_type tfce_voxel_2 (tfce_data_2);

  Image::LoopInOrder loop(tfce_voxel_1);
  for (loop.start(tfce_voxel_1, tfce_voxel_2); loop.ok(); loop.next(tfce_voxel_1, tfce_voxel_2)) {
    tfce_voxel_1.value() = 0.0;
    tfce_voxel_2.value() = 0.0;
  }
  for (uint32_t i = 0; i < num_vox; i++) {
    for (size_t dim = 0; dim < tfce_voxel_1.ndim(); dim++) {
      tfce_voxel_2[dim] = mask_indices[i][dim];
      tfce_voxel_1[dim] = mask_indices[i][dim];
    }
    tfce_voxel_1.value() = tfce_output_1[i];
    tfce_voxel_2.value() = tfce_output_2[i];
  }
  string perm_filename_1 = prefix + "_permutation_1.txt";
  string perm_filename_2 = prefix + "_permutation_2.txt";
  perm_distribution_1.save(perm_filename_1);
  perm_distribution_2.save(perm_filename_2);

  string pvalue_filename_1 = prefix + "_pvalue_1.mif";
  Image::Buffer<float> pvalue_data_1 (pvalue_filename_1, header);
  Image::Buffer<float>::voxel_type pvalue_voxel_1 (pvalue_data_1);
  Stats::statistic2pvalue (perm_distribution_1, tfce_voxel_1, pvalue_voxel_1);
  string pvalue_filename_2 = prefix + "_pvalue_2.mif";
  Image::Buffer<float> pvalue_data_2 (pvalue_filename_2, header);
  Image::Buffer<float>::voxel_type pvalue_voxel_2 (pvalue_data_2);
  Stats::statistic2pvalue (perm_distribution_2, tfce_voxel_2, pvalue_voxel_2);

  cout << " Done" << endl;
}

/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 01/10/2012.

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
#include "image/transform.h"
#include "image/filter/connected_components.h"
#include "math/SH.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/hemisphere/directions.h"
#include "timer.h"
#include "math/stats/permutation.h"
#include "math/stats/glm.h"
#include "stats/tfce.h"
#include "dwi/fmls.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/loader.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;
using namespace std;
using namespace MR::DWI::Tractography::Mapping;

typedef float value_type;

// TODO combine this with Rob's SIFT code once committed into main repository
template <class FODImageType, class MaskImageType>
class SHQueueWriter
{
  public:
    SHQueueWriter (FODImageType& fod_image, MaskImageType& mask_data) :
      fod_image_ (fod_image),
      mask_ (mask_data),
      loop_ (0, 3),
      progress_ ("computing FOD lobe integrals... ", 1)
    {
      size_t count = 0;
      Image::Loop count_loop;
      for (count_loop.start (mask_); count_loop.ok(); count_loop.next (mask_)) {
        if (mask_.value())
          ++count;
      }
      progress_.set_max (count);
      loop_.start (fod_image_, mask_);
    }


    bool operator () (DWI::SH_coefs& out) {
      while (loop_.ok() && !mask_.value())
        loop_.next (fod_image_, mask_);
      if (!loop_.ok()) {
        return false;
      }
      out.vox[0] = fod_image_[0]; out.vox[1] = fod_image_[1]; out.vox[2] = fod_image_[2];
      out.allocate (fod_image_.dim (3));
      for (fod_image_[3] = 0; fod_image_[3] != fod_image_.dim (3); ++fod_image_[3])
        out[fod_image_[3]] = fod_image_.value();
      ++progress_;
      loop_.next (fod_image_, mask_);
      return true;
    }


  private:
    typename FODImageType::voxel_type fod_image_;
    typename MaskImageType::voxel_type mask_;
    Image::Loop loop_;
    ProgressBar progress_;

};



class GroupAvLobeProcessor
{
  public:
    GroupAvLobeProcessor (Image::BufferScratch<int32_t>& FOD_lobe_indexer,
                          vector<Point<float> >& FOD_lobe_directions,
                          vector<Point<float> >& index2scanner_pos) :
                          FOD_lobe_indexer (FOD_lobe_indexer) ,
                          FOD_lobe_directions (FOD_lobe_directions),
                          index2scanner_pos (index2scanner_pos),
                          image_transform (FOD_lobe_indexer){}

    bool operator () (DWI::FOD_lobes& in) {
      if (in.empty())
         return true;
      FOD_lobe_indexer[0] = in.vox[0];
      FOD_lobe_indexer[1] = in.vox[1];
      FOD_lobe_indexer[2] = in.vox[2];
      FOD_lobe_indexer[3] = 0;
      FOD_lobe_indexer.value() = FOD_lobe_directions.size();
      int32_t lobe_count = 0;
      for (vector<DWI::FOD_lobe>::const_iterator i = in.begin(); i != in.end(); ++i, ++lobe_count) {
        FOD_lobe_directions.push_back (i->get_peak_dir());
        Point<float> pos;
        image_transform.voxel2scanner (FOD_lobe_indexer, pos);
        index2scanner_pos.push_back (pos);
      }
      FOD_lobe_indexer[3] = 1;
      FOD_lobe_indexer.value() = lobe_count;
      return true;
    }


  private:
    Image::BufferScratch<int32_t>::voxel_type FOD_lobe_indexer;
    vector<Point<float> >& FOD_lobe_directions;
    vector<Point<float> >& index2scanner_pos;
    Image::Transform image_transform;
};


class SubjectLobeProcessor {

  public:
    SubjectLobeProcessor (Image::BufferScratch<int32_t>& FOD_lobe_indexer,
                          const vector<Point<float> >& FOD_lobe_directions,
                          vector<float>& subject_lobe_integrals,
                          float angular_threshold) :
                          FOD_lobe_indexer (FOD_lobe_indexer),
                          average_lobe_directions (FOD_lobe_directions),
                          subject_lobe_integrals (subject_lobe_integrals)
    {
      angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));
    }

    bool operator () (DWI::FOD_lobes& in)
    {
      if (in.empty())
        return true;

      FOD_lobe_indexer[0] = in.vox[0];
      FOD_lobe_indexer[1] = in.vox[1];
      FOD_lobe_indexer[2] = in.vox[2];
      FOD_lobe_indexer[3] = 0;
      int32_t voxel_index = FOD_lobe_indexer.value();
      FOD_lobe_indexer[3] = 1;
      int32_t number_lobes = FOD_lobe_indexer.value();

      // for each lobe in the average, find the corresponding lobe in this subject voxel
      for (int32_t i = voxel_index; i < voxel_index + number_lobes; ++i) {
        float largest_dp = 0.0;
        int largest_index = -1;
        for (size_t j = 0; j < in.size(); ++j) {
          float dp = Math::abs (average_lobe_directions[i].dot(in[j].get_peak_dir()));
          if (dp > largest_dp) {
            largest_dp = dp;
            largest_index = j;
          }
        }
        if (largest_dp > angular_threshold_dp) {
          subject_lobe_integrals[i] = in[largest_index].get_integral();
        }
      }

      return true;
    }


  private:
    Image::BufferScratch<int32_t>::voxel_type FOD_lobe_indexer;
    const vector<Point<float> >& average_lobe_directions;
    vector<float>& subject_lobe_integrals;
    float angular_threshold_dp;
};



class TractProcessor {

  public:
    TractProcessor (Image::BufferScratch<int32_t>& FOD_lobe_indexer,
                    vector<Point<float> >& FOD_lobe_directions,
                    vector<uint16_t>& lobe_TDI,
                    vector<map<int32_t, Stats::TFCE::connectivity> >& lobe_connectivity,
                    float angular_threshold):
                    FOD_lobe_indexer (FOD_lobe_indexer) ,
                    FOD_lobe_directions (FOD_lobe_directions),
                    lobe_TDI (lobe_TDI),
                    lobe_connectivity (lobe_connectivity) {
      angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));
    }

    bool operator () (SetVoxelDir& in)
    {
      // For each voxel tract tangent, assign to a lobe
      vector<int32_t> tract_lobe_indices;
      for (SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
        Image::Nav::set_pos (FOD_lobe_indexer, *i);
        FOD_lobe_indexer[3] = 0;
        int32_t first_index = FOD_lobe_indexer.value();

        if (first_index >= 0) {
          FOD_lobe_indexer[3] = 1;
          int32_t last_index = first_index + FOD_lobe_indexer.value();
          int32_t closest_lobe_index = -1;
          float largest_dp = 0.0;
          Point<float> dir (i->get_dir());
          dir.normalise();
          for (int32_t j = first_index; j < last_index; j++) {
            float dp = Math::abs (dir.dot (FOD_lobe_directions[j]));
            if (dp > largest_dp) {
              largest_dp = dp;
              closest_lobe_index = j;
            }
          }
          if (largest_dp > angular_threshold_dp) {
            tract_lobe_indices.push_back (closest_lobe_index);
            lobe_TDI[closest_lobe_index]++;
          }
        }
      }

      for (size_t i = 0; i < tract_lobe_indices.size(); i++) {
        for (size_t j = i + 1; j < tract_lobe_indices.size(); j++) {
          lobe_connectivity[tract_lobe_indices[i]][tract_lobe_indices[j]].value++;
          lobe_connectivity[tract_lobe_indices[j]][tract_lobe_indices[i]].value++;
        }
      }

      return true;
    }


  private:
    Image::BufferScratch<int32_t>::voxel_type FOD_lobe_indexer;
    vector<Point<float> >& FOD_lobe_directions;
    vector<uint16_t>& lobe_TDI;
    vector<map<int32_t, Stats::TFCE::connectivity> >& lobe_connectivity;
    float angular_threshold_dp;
};



void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Statistical analysis of bundle-specific DWI indices using threshold-free cluster "
    "enhancement and a whole-brain tractogram for defining a probabilistic neighbourhood";


  ARGUMENTS
  + Argument ("input", "a text file containing the file names of the input images").type_file()

  + Argument ("design", "the design matrix").type_file()

  + Argument ("contrast", "the contrast matrix").type_file()

  + Argument ("group", "the group average FOD image (ideally at lmax=8).").type_image_in()

  + Argument ("mask", "a 3D mask to define which voxels to include in the analysis").type_image_in()

  + Argument ("tracks", "the tracks used to define orientations of "
                        "interest and spatial neighbourhoods.").type_file()

  + Argument ("output", "the root directory and filename prefix "
              "for all output.").type_text();


  OPTIONS
  + Option ("nperms", "the number of permutations (default = 5000).")
  + Argument ("num").type_integer (1, 5000, 100000)

  + Option ("dh", "the height increment used in the TFCE integration (default = 0.1)")
  + Argument ("value").type_float (0.001, 0.1, 100000)

  + Option ("tfce_e", "TFCE height parameter (default = 2)")
  + Argument ("value").type_float (0.001, 2.0, 100000)

  + Option ("tfce_h", "TFCE extent parameter (default = 0.5)")
  + Argument ("value").type_float (0.001, 0.5, 100000)

  + Option ("angle", "the max angle threshold for computing inter-subject FOD peak correspondence")
  + Argument ("value").type_float (0.001, 30, 90)

  + Option ("connectivity", "a threshold to define the required fraction of shared connections to be included in the neighbourhood (default: 1%)")
  + Argument ("threshold").type_float (0.001, 0.01, 1.0)

  + Option ("smooth", "smooth the AFD integral along the fibre tracts using a Gaussian kernel with the supplied FWHM (default: 5mm)")
  + Argument ("FWHM").type_float (0.0, 5.0, 200.0);

}


#define GROUP_AVERAGE_FOD_THRESHOLD 0.15
#define SUBJECT_FOD_THRESHOLD 0.15

typedef Stats::TFCE::value_type value_type;

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

  value_type angular_threshold = 20.0;
  opt = get_options ("angle");
  if (opt.size())
    angular_threshold = opt[0][0];

  value_type connectivity_threshold = 0.01;
  opt = get_options ("connectivity");
  if (opt.size())
    connectivity_threshold = opt[0][0];

  value_type std_dev = 5.0 / 2.3548;
  opt = get_options ("smooth");
  if (opt.size())
    std_dev = value_type(opt[0][0]) / 2.3548;

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

  // Compute FOD lobes on average FOD image
  vector<Point<value_type> > FOD_lobe_directions;
  Math::Hemisphere::Directions dirs (1281);
  Image::Header index_header (argument[3]);
  index_header.dim(3) = 2;
  Image::BufferScratch<int32_t> FOD_lobe_indexer (index_header);
  Image::BufferScratch<int32_t>::voxel_type lobe_indexer_vox (FOD_lobe_indexer);
  vector<Point<value_type> > lobe_positions;
  Image::LoopInOrder loop4D (lobe_indexer_vox);
  for (loop4D.start (lobe_indexer_vox); loop4D.ok(); loop4D.next (lobe_indexer_vox))
    lobe_indexer_vox.value() = -1;

  {
    Image::Buffer<value_type> av_fod_buffer (argument[3]);
    Image::Buffer<bool> brain_mask_buffer (argument[4]);
    Image::check_dimensions (av_fod_buffer, brain_mask_buffer, 0, 3);
    SHQueueWriter<Image::Buffer<value_type>, Image::Buffer<bool> > writer (av_fod_buffer, brain_mask_buffer);
    DWI::FOD_FMLS fmls (dirs, Math::SH::LforN (av_fod_buffer.dim(3)));
    fmls.set_peak_value_threshold (GROUP_AVERAGE_FOD_THRESHOLD);
    GroupAvLobeProcessor lobe_processor (FOD_lobe_indexer, FOD_lobe_directions, lobe_positions);
    Thread::run_queue (writer, 1, DWI::SH_coefs(), fmls, 0, DWI::FOD_lobes(), lobe_processor, 1);
  }

  // Compute 3D analysis mask based on lobes in average FOD image
  uint32_t num_lobes = FOD_lobe_directions.size();
  CONSOLE ("Number of lobes: " + str(num_lobes));
  Image::Header header (argument[3]);
  Image::BufferScratch<bool> lobe_mask (header);
  Image::BufferScratch<bool>::voxel_type lobe_mask_vox (lobe_mask);
  Image::Loop loop (0, 3);
  for (loop.start (lobe_indexer_vox, lobe_mask_vox); loop.ok(); loop.next (lobe_indexer_vox, lobe_mask_vox)) {
    lobe_indexer_vox[3] = 0;
    if (lobe_indexer_vox.value() >= 0) {
      lobe_mask_vox.value() = 1;
      lobe_indexer_vox[3] = 1;
    } else {
      lobe_mask_vox.value() = 0;
    }
  }

  // Read in tracts, and computer whole-brain lobe-lobe connectivity
  DWI::Tractography::Reader<value_type> file;
  DWI::Tractography::Properties properties;
  file.open (argument[5], properties);

  const int num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
  if (!num_tracks)
    throw Exception ("Error with track count in input file");

  typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;
  Math::Matrix<value_type> dummy_matrix;
  DWI::Tractography::Mapping::TrackLoader loader (file, num_tracks);
  DWI::Tractography::Mapping::TrackMapperBase<SetVoxelDir> mapper (header, dummy_matrix);
  vector<uint16_t> lobe_TDI (num_lobes, 0.0);
  vector<map<int32_t, Stats::TFCE::connectivity> > lobe_connectivity (num_lobes);
  TractProcessor tract_processor (FOD_lobe_indexer, FOD_lobe_directions, lobe_TDI, lobe_connectivity, 30);
  Thread::run_queue (loader, 1, DWI::Tractography::Mapping::TrackAndIndex(), mapper, 1, SetVoxelDir(), tract_processor, 1);

  // Normalise connectivity matrix and threshold, pre-compute lobe-lobe weights for smoothing.
  vector<map<int32_t, value_type> > lobe_smoothing_weights (num_lobes);
  bool do_smoothing;
  const value_type gaussian_const2 = 2.0 * std_dev * std_dev;
  value_type gaussian_const1 = 1.0;
  if (std_dev > 0.0) {
    do_smoothing = true;
    gaussian_const1 = 1.0 / (std_dev *  Math::sqrt (2.0 * M_PI));
  }
  for (unsigned int lobe = 0; lobe < num_lobes; ++lobe) {
    map<int32_t, Stats::TFCE::connectivity>::iterator it = lobe_connectivity[lobe].begin();
    while (it != lobe_connectivity[lobe].end()) {
      value_type connectivity = it->second.value / value_type (lobe_TDI[lobe]);
      if (connectivity < connectivity_threshold)  {
        lobe_connectivity[lobe].erase (it++);
      } else {
        if (do_smoothing) {
          value_type distance = Math::sqrt (Math::pow2 (lobe_positions[lobe][0] - lobe_positions[it->first][0]) +
                                            Math::pow2 (lobe_positions[lobe][1] - lobe_positions[it->first][1]) +
                                            Math::pow2 (lobe_positions[lobe][2] - lobe_positions[it->first][2]));
          value_type weight = connectivity * gaussian_const1 * Math::exp (-Math::pow2 (distance) / gaussian_const2);
          if (weight > 0.005)
            lobe_smoothing_weights[lobe].insert (pair<int32_t, value_type> (it->first, weight));
        }
        it->second.value = connectivity;
        ++it;
      }
    }
    // Make sure the lobe is fully connected to itself and give it a smoothing weight
    Stats::TFCE::connectivity self_connectivity;
    self_connectivity.value = 1.0;
    lobe_connectivity[lobe].insert (pair<int32_t, Stats::TFCE::connectivity> (lobe, self_connectivity));
    lobe_smoothing_weights[lobe].insert (pair<int32_t, value_type> (lobe, gaussian_const1));
  }

  // Normalise smoothing weights
  for (size_t i = 0; i < num_lobes; ++i) {
    value_type sum = 0.0;
    for (map<int32_t, value_type>::iterator it = lobe_smoothing_weights[i].begin(); it != lobe_smoothing_weights[i].end(); ++it)
      sum += it->second;
    value_type norm_factor = 1.0 / sum;
    for (map<int32_t, value_type>::iterator it = lobe_smoothing_weights[i].begin(); it != lobe_smoothing_weights[i].end(); ++it)
      it->second *= norm_factor;
  }

  Math::Matrix<value_type> subject_FOD_lobe_integrals;
  subject_FOD_lobe_integrals.resize (subjects.size(), num_lobes, 0.0);
  for (size_t subject = 0; subject < subjects.size(); subject++) {
    Image::Buffer<value_type> fod_buffer (subjects[subject]);
    SHQueueWriter<Image::Buffer<value_type>, Image::BufferScratch<bool> > writer (fod_buffer, lobe_mask);
    DWI::FOD_FMLS fmls (dirs, Math::SH::LforN (fod_buffer.dim(3)));
    fmls.set_peak_value_threshold (SUBJECT_FOD_THRESHOLD);
    vector<value_type> temp_lobe_integrals (num_lobes, 0.0);
    SubjectLobeProcessor lobe_processor (FOD_lobe_indexer, FOD_lobe_directions, temp_lobe_integrals, angular_threshold);
    Thread::run_queue (writer, 1, DWI::SH_coefs(), fmls, 0, DWI::FOD_lobes(), lobe_processor, 1);

    // Smooth the data based on connectivity
    for (size_t lobe = 0; lobe < num_lobes; ++lobe) {
      value_type value = 0.0;
      map<int32_t, value_type>::iterator it = lobe_smoothing_weights[lobe].begin();
      for (; it != lobe_smoothing_weights[lobe].end(); ++it)
        value += temp_lobe_integrals[it->first] * it->second;
      subject_FOD_lobe_integrals (subject, lobe) = value;
    }
  }

  // Run permutation test
  Math::Vector<value_type> perm_distribution_pos (num_perms - 1);
  Math::Vector<value_type> perm_distribution_neg (num_perms - 1);
  std::vector<value_type> tfce_output_pos (num_lobes, 0.0);
  std::vector<value_type> tfce_output_neg (num_lobes, 0.0);
  std::vector<value_type> tvalue_output (num_lobes, 0.0);

  {
    Math::Stats::GLMTTest glm (subject_FOD_lobe_integrals, design, contrast);
    Stats::TFCE::QueueLoader loader (num_perms, subjects.size());
    Stats::TFCE::TFCEConnectivity tfce_integrator (lobe_connectivity, dh, E, H);
    Stats::TFCE::ThreadKernel<Math::Stats::GLMTTest,
                              Stats::TFCE::TFCEConnectivity> processor (glm, tfce_integrator,
                                                                        perm_distribution_pos, perm_distribution_neg,
                                                                        tfce_output_pos, tfce_output_neg, tvalue_output);
    Thread::run_queue (loader, 1, Stats::TFCE::PermutationItem(), processor, 0);
  }


}

  // Output data
//  header.datatype() = DataType::Float32;
//  std::string prefix (argument[6]);
//
//  std::string tfce_filename_pos = prefix + "_tfce_pos.mif";
//  Image::Buffer<value_type> tfce_data_pos (tfce_filename_pos, header);
//  Image::Buffer<value_type>::voxel_type tfce_voxel_pos (tfce_data_pos);
//  std::string tfce_filename_neg = prefix + "_tfce_neg.mif";
//  Image::Buffer<value_type> tfce_data_neg (tfce_filename_neg, header);
//  Image::Buffer<value_type>::voxel_type tfce_voxel_neg (tfce_data_neg);
//  std::string tvalue_filename = prefix + "_tvalue.mif";
//  Image::Buffer<value_type> tvalue_data (tvalue_filename, header);
//  Image::Buffer<value_type>::voxel_type tvalue_voxel (tvalue_data);
//
//  {
//    ProgressBar progress ("generating output...");
//    for (size_t i = 0; i < num_lobes; i++) {
//      for (size_t dim = 0; dim < tfce_voxel_pos.ndim(); dim++)
//        tvalue_voxel[dim] = tfce_voxel_pos[dim] = tfce_voxel_neg[dim] = mask_indices[i][dim];
//      tvalue_voxel.value() = tvalue_output[i];
//      tfce_voxel_pos.value() = tfce_output_pos[i];
//      tfce_voxel_neg.value() = tfce_output_neg[i];
//    }
//    std::string perm_filename_pos = prefix + "_permutation_pos.txt";
//    std::string perm_filename_neg = prefix + "_permutation_neg.txt";
//    perm_distribution_pos.save (perm_filename_pos);
//    perm_distribution_neg.save (perm_filename_neg);
//
//    std::string pvalue_filename_pos = prefix + "_pvalue_pos.mif";
//    Image::Buffer<value_type> pvalue_data_pos (pvalue_filename_pos, header);
//    Image::Buffer<value_type>::voxel_type pvalue_voxel_pos (pvalue_data_pos);
//    Stats::statistic2pvalue (perm_distribution_pos, tfce_voxel_pos, pvalue_voxel_pos);
//
//    std::string pvalue_filename_neg = prefix + "_pvalue_neg.mif";
//    Image::Buffer<value_type> pvalue_data_neg (pvalue_filename_neg, header);
//    Image::Buffer<value_type>::voxel_type pvalue_voxel_neg (pvalue_data_neg);
//    Stats::statistic2pvalue (perm_distribution_neg, tfce_voxel_neg, pvalue_voxel_neg);
//  }



  /**
//   * load average FOD image.
//   * Compute FOD lobes on average image using low FOD threshold -> default set
     * Load tracks
     * Allocate lobe tract count array
     * For each tract
     *   get set of dixels
     *   for each dixel
     *     assign to lobe with angular threshold
     *   for each pair of lobes increment map
     *   increment lobe tract count
     *
     * normalise connectivity matrix and threshold at 1%
     * pre-compute smoothing weights for each lobe
     * For each subject
     *   load FOD image
     *   compute lobes
     *   for each voxel
     *     assign to default set using angular threshold
     *   smooth data
     *
     * For all lobes in default set
     *     for each permutation
     *       compute t-values
     *       keep max_tfce
     *     assign to permutation distribution
     *
     *
     *
     *
     */


  /****************************************************************
  * test lobe connectivity for single lobe
  *****************************************************************/
//  FOD_lobe_indexer_vox[0] = 44;
//  FOD_lobe_indexer_vox[1] = 55;
//  FOD_lobe_indexer_vox[2] = 39;
//  FOD_lobe_indexer_vox[3] = 0;
//  int32_t lobe_index = FOD_lobe_indexer_vox.value()  ;
//  VAR(lobe_index);
//cout << lobe_connectivity[lobe_index].size() << endl;

//  cout << FOD_lobe_directions[lobe_index] << endl;
//  cout << lobe_TDI[lobe_index] << endl;

//  Image::Header temp_header (argument[4]);
//  temp_header.set_ndim (3);
//  Image::Buffer<float> temp ("tractTDI_norm.mif", temp_header);
//  Image::Buffer<float>::voxel_type temp_vox (temp);
//  Image::Loop loop2 (0, 3);
//
//  map<int32_t, shared_tract_count>::iterator it;
//  for (loop2.start (FOD_lobe_indexer_vox, temp_vox); loop2.ok(); loop2.next (FOD_lobe_indexer_vox, temp_vox)) {
//    FOD_lobe_indexer_vox[3] = 0;
//    int32_t index = FOD_lobe_indexer_vox.value();
//    if (index >= 0) {
//      FOD_lobe_indexer_vox[3] = 1;
//      int32_t num_lobes = FOD_lobe_indexer_vox.value();
//      float connectivity = 0.0;
//      for (int l = index; l < num_lobes + index; ++l) {
//        it = lobe_connectivity[lobe_index].find(l);
//        if (it != lobe_connectivity[lobe_index].end())
//          connectivity += it->second.count;
//      }
//      temp_vox.value() = connectivity ; /// float(lobe_TDI[lobe_index]);
//    } else {
//      temp_vox.value() = 0.0;
//    }
//  }
//  cout << lobe_connectivity[lobe_index].size() << endl;


  /****************************************************************
  * Check subject lobe fibre count
  *****************************************************************/
//  Image::BufferScratch<int> sub_fibre_count_buffer (header);
//  Image::BufferScratch<int>::voxel_type sub_fibre_count_vox (sub_fibre_count_buffer);
//  Image::Loop subject_loop(0, 3);
//  for (subject_loop.start (FOD_lobe_indexer_vox, sub_fibre_count_vox); subject_loop.ok(); subject_loop.next (FOD_lobe_indexer_vox, sub_fibre_count_vox)) {
//    FOD_lobe_indexer_vox[3] = 0;
//    int32_t lobe_index = FOD_lobe_indexer_vox.value();
//    if (lobe_index >= 0) {
//      FOD_lobe_indexer_vox[3] = 1;
//      int num_lobes =  FOD_lobe_indexer_vox.value();
//      int num_sub_lobes = 0;
//      for (int i = lobe_index; i < lobe_index + num_lobes; i++) {
//        if (subject_FOD_lobe_integrals[subject][i] > 0.0)
//          num_sub_lobes++;
//      }
//      sub_fibre_count_vox.value() = num_sub_lobes;
//    }
//  }
//  Image::Buffer<int> fibre_count ("fibre_count" + str(subject) + ".mif", header);
//  Image::Buffer<int>::voxel_type fibre_vox (fibre_count);
//  Image::copy (sub_fibre_count_vox, fibre_vox);


  //  Image::Header temp_header (argument[4]);
  //  temp_header.set_ndim (3);
  //  Image::BufferScratch<float> temp (temp_header);
  //  Image::BufferScratch<float>::voxel_type temp_vox (temp);
  //  Image::Loop loop2 (0, 3);
  //  FOD_lobe_indexer_vox[3] = 1;
  //
  //  for (loop2.start (FOD_lobe_indexer_vox, temp_vox); loop2.ok(); loop2.next (lobe_indexer_vox, temp_vox)) {
  //    FOD_lobe_indexer_vox[3] = 0;
  //    float average = 0.0;
  //    int32_t index = FOD_lobe_indexer_vox.value();
  //    if (index >= 0) {
  //      lobe_indexer_vox[3] = 1;
  //      int32_t num_lobes = FOD_lobe_indexer_vox.value();
  //      for (int l = index; l < num_lobes + index; ++l) {
  //        for (int s = 0; s < num_subjects; ++s) {
  //          if (subject_FOD_lobe_integrals[s][l] > 0) {
  //            average += 1.0;
  //          }
  //        }
  //      }
  //
  //      temp_vox.value() = average / (float(num_subjects) * float(num_lobes));
  //    } else {
  //      temp_vox.value() = 0.0;
  //    }
  //  }

//
//   /****************************
//   * check smoothing weights
//   *****************************/
//     FOD_lobe_indexer_vox[0] = 44;
//     FOD_lobe_indexer_vox[1] = 55;
//     FOD_lobe_indexer_vox[2] = 39;
//     FOD_lobe_indexer_vox[3] = 0;
//     int32_t lobe_index = FOD_lobe_indexer_vox.value()  ;
//     VAR(lobe_index);
//
//     Image::Header temp_header (argument[4]);
//     temp_header.set_ndim (3);
//     Image::Buffer<float> temp ("testSmoothing.mif", temp_header);
//     Image::Buffer<float>::voxel_type temp_vox (temp);
//     Image::Loop loop2 (0, 3);
//
//     map<int32_t, float>::iterator it;
//     for (loop2.start (FOD_lobe_indexer_vox, temp_vox); loop2.ok(); loop2.next (FOD_lobe_indexer_vox, temp_vox)) {
//       FOD_lobe_indexer_vox[3] = 0;
//       int32_t index = FOD_lobe_indexer_vox.value();
//       if (index >= 0) {
//         FOD_lobe_indexer_vox[3] = 1;
//         int32_t num_lobes = FOD_lobe_indexer_vox.value();
//         float weight = 0.0;
//         for (int l = index; l < num_lobes + index; ++l) {
//           it = lobe_smoothing_weights[lobe_index].find(l);
//           if (it != lobe_smoothing_weights[lobe_index].end())
//             weight += it->second;
//         }
//         temp_vox.value() = weight ; /// float(lobe_TDI[lobe_index]);
//       } else {
//         temp_vox.value() = 0.0;
//       }
//     }

  /***********************************
   * test smoothing
   ***********************************/
//   FOD_lobe_indexer_vox[0] = 44;
//   FOD_lobe_indexer_vox[1] = 55;
//   FOD_lobe_indexer_vox[2] = 39;
//   FOD_lobe_indexer_vox[3] = 0;
//   int32_t lobe_index = FOD_lobe_indexer_vox.value()  ;
//
//   Image::Header temp_header (argument[4]);
//   temp_header.set_ndim (3);
//   Image::Buffer<float> temp ("afdSmoothed.mif", temp_header);
//   Image::Buffer<float>::voxel_type temp_vox (temp);
//   Image::Loop loop2 (0, 3);
//
//   map<int32_t, shared_tract_count>::iterator it;
//   for (loop2.start (FOD_lobe_indexer_vox, temp_vox); loop2.ok(); loop2.next (FOD_lobe_indexer_vox, temp_vox)) {
//     FOD_lobe_indexer_vox[3] = 0;
//     int32_t index = FOD_lobe_indexer_vox.value();
//     if (index >= 0) {
//       FOD_lobe_indexer_vox[3] = 1;
//       int32_t num_lobes = FOD_lobe_indexer_vox.value();
//       float afd = 0.0;
//       for (int l = index; l < num_lobes + index; ++l) {
////         it = lobe_connectivity[lobe_index].find(l);
////         if (it != lobe_connectivity[lobe_index].end())
//         afd += subject_FOD_lobe_integrals[0][l];
//       }
//       temp_vox.value() = afd ; /// float(lobe_TDI[lobe_index]);
//     } else {
//       temp_vox.value() = 0.0;
//     }
//   }


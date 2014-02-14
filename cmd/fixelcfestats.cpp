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
#include "command.h"
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "image/loop.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/transform.h"
#include "image/filter/connected_components.h"
#include "image/interp/nearest.h"
#include "math/SH.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/stats/permutation.h"
#include "math/stats/glm.h"
#include "timer.h"
#include "stats/tfce.h"
#include "dwi/fmls.h"
#include "dwi/directions/set.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/writer.h"


using namespace MR;
using namespace App;
using namespace std;
using namespace MR::DWI::Tractography::Mapping;

#define GROUP_AVERAGE_FOD_THRESHOLD 0.12
#define SUBJECT_FOD_THRESHOLD 0.04

typedef Stats::TFCE::value_type value_type;


void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Statistical analysis of bundle-specific DWI indices using threshold-free cluster "
    "enhancement and a whole-brain tractogram for defining a probabilistic neighbourhood";


  ARGUMENTS
  + Argument ("fods", "a text file listing the file names of the input FOD images").type_file()

  + Argument ("modfods", "a text file listing the file names of the input MODULATED FOD images").type_file()

  + Argument ("design", "the design matrix").type_file()

  + Argument ("contrast", "the contrast matrix").type_file()

  + Argument ("group", "the group average FOD image.").type_image_in()

  + Argument ("mask", "a 3D mask to define which voxels to include in the analysis").type_image_in()

  + Argument ("tracks", "the tracks used to define orientations of "
                        "interest and spatial neighbourhoods.").type_file()

  + Argument ("output", "the filename prefix for all output.").type_text();


  OPTIONS

  + Option ("notest", "don't perform permutation testing and only output population statistics (effect size, stdev etc)")

  + Option ("nperms", "the number of permutations (default = 5000).")
  + Argument ("num").type_integer (1, 5000, 100000)

  + Option ("dh", "the height increment used in the TFCE integration (default = 0.1)")
  + Argument ("value").type_float (0.001, 0.1, 100000)

  + Option ("tfce_e", "TFCE height parameter (default = 1.0)")
  + Argument ("value").type_float (0.001, 0.5, 100000)

  + Option ("tfce_h", "TFCE extent parameter (default = 2.0)")
  + Argument ("value").type_float (0.001, 2.0, 100000)

  + Option ("tfce_c", "TFCE connectivity parameter (default = 0.5)")
  + Argument ("value").type_float (0.001, 0.5, 100000)

  + Option ("angle", "the max angle threshold for computing inter-subject FOD peak correspondence")
  + Argument ("value").type_float (0.001, 30, 90)

  + Option ("connectivity", "a threshold to define the required fraction of shared connections to be included in the neighbourhood (default: 1%)")
  + Argument ("threshold").type_float (0.001, 0.01, 1.0)

  + Option ("smooth", "smooth the AFD integral along the fibre tracts using a Gaussian kernel with the supplied FWHM (default: 5mm)")
  + Argument ("FWHM").type_float (0.0, 5.0, 200.0)

  + Option ("num_vis_tracks", "the number of tracks to use when generating output for visualisation. "
                              "These tracts are obtained by truncating the input tracks (default: 200000")
  + Argument ("num").type_integer (1, 200000, INT_MAX)

  + Option ("check", "output a 3D image to check the number of fixels per voxel identified in the template")
  + Argument ("image").type_image_out ();
}


/**
 * For each fixel in the group average image, save the first fixel index and number of
 * voxel fixels into the corresponding voxel of the FOD_fixel_indexer. Store the
 * fixel direction and the scanner position.
 */
class GroupAvFixelProcessor
{
  public:
    GroupAvFixelProcessor (Image::BufferScratch<int32_t>& fixel_indexer,
                           vector<Point<value_type> >& fixel_directions,
                           vector<Point<value_type> >& fixel_scanner_pos) :
                           fixel_indexer (fixel_indexer) ,
                           fixel_directions (fixel_directions),
                           fixel_scanner_positions (fixel_scanner_pos),
                           image_transform (fixel_indexer) {}

    bool operator () (DWI::FMLS::FOD_lobes& in) {
      if (in.empty())
         return true;
      fixel_indexer[0] = in.vox[0];
      fixel_indexer[1] = in.vox[1];
      fixel_indexer[2] = in.vox[2];
      fixel_indexer[3] = 0;
      fixel_indexer.value() = fixel_directions.size();
      int32_t fixel_count = 0;
      for (vector<DWI::FMLS::FOD_lobe>::const_iterator i = in.begin(); i != in.end(); ++i, ++fixel_count) {
        fixel_directions.push_back (i->get_peak_dir());
        Point<value_type> pos;
        image_transform.voxel2scanner (fixel_indexer, pos);
        fixel_scanner_positions.push_back (pos);
      }
      fixel_indexer[3] = 1;
      fixel_indexer.value() = fixel_count;
      return true;
    }


  private:
    Image::BufferScratch<int32_t>::voxel_type fixel_indexer;
    vector<Point<value_type> >& fixel_directions;
    vector<Point<value_type> >& fixel_scanner_positions;
    Image::Transform image_transform;
};



class SubjectFixelProcessor {

  public:
    SubjectFixelProcessor (Image::BufferScratch<int32_t>& FOD_fixel_indexer,
                           const vector<Point<value_type> >& FOD_fixel_directions,
                           vector<value_type>& subject_fixel_integrals,
                           value_type angular_threshold) :
                           FOD_fixel_indexer (FOD_fixel_indexer),
                           average_fixel_directions (FOD_fixel_directions),
                           subject_fixel_integrals (subject_fixel_integrals)
    {
      angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));
    }

    bool operator () (DWI::FMLS::FOD_lobes& in)
    {
      if (in.empty())
        return true;

      FOD_fixel_indexer[0] = in.vox[0];
      FOD_fixel_indexer[1] = in.vox[1];
      FOD_fixel_indexer[2] = in.vox[2];
      FOD_fixel_indexer[3] = 0;
      int32_t voxel_index = FOD_fixel_indexer.value();
      FOD_fixel_indexer[3] = 1;
      int32_t number_fixels = FOD_fixel_indexer.value();

      // for each fixel in the average, find the corresponding fixel in this subject voxel
      for (int32_t i = voxel_index; i < voxel_index + number_fixels; ++i) {
        value_type largest_dp = 0.0;
        int largest_index = -1;
        for (size_t j = 0; j < in.size(); ++j) {
          value_type dp = Math::abs (average_fixel_directions[i].dot(in[j].get_peak_dir()));
          if (dp > largest_dp) {
            largest_dp = dp;
            largest_index = j;
          }
        }
        if (largest_dp > angular_threshold_dp)
          subject_fixel_integrals[i] = in[largest_index].get_integral();
      }

      return true;
    }


  private:
    Image::BufferScratch<int32_t>::voxel_type FOD_fixel_indexer;
    const vector<Point<value_type> >& average_fixel_directions;
    vector<value_type>& subject_fixel_integrals;
    value_type angular_threshold_dp;
};


/**
 * Process each track (represented as a set of fixels). For each track tangent fixel, identify the closest group average fixel
 */
class TrackProcessor {

  public:
    TrackProcessor (Image::BufferScratch<int32_t>& FOD_fixel_indexer,
                    const vector<Point<value_type> >& FOD_fixel_directions,
                    vector<uint16_t>& fixel_TDI,
                    vector<map<int32_t, Stats::TFCE::connectivity> >& fixel_connectivity,
                    value_type angular_threshold):
                    fixel_indexer (FOD_fixel_indexer) ,
                    fixel_directions (FOD_fixel_directions),
                    fixel_TDI (fixel_TDI),
                    fixel_connectivity (fixel_connectivity) {
      angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));
    }

    bool operator () (SetVoxelDir& in)
    {
      // For each voxel tract tangent, assign to a fixel
      vector<int32_t> tract_fixel_indices;
      for (SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
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
    const vector<Point<value_type> >& fixel_directions;
    vector<uint16_t>& fixel_TDI;
    vector<map<int32_t, Stats::TFCE::connectivity> >& fixel_connectivity;
    value_type angular_threshold_dp;
};



/**
 * Processes each track in the tractogram used for output (most likely a subset
 * of all tracks since we need less tracks for display than for estimating fixel-fixel connectivity)
 * and computes the FOD fixel indexes for each track point. Indices are stored for later
 * use when outputting various track-point statistics. This function also writes out the tractogram subset.
 */
double compute_track_indices (const string& input_track_filename,
                              Image::BufferScratch<int32_t>& fixel_indexer,
                              const vector<Point<value_type> >& fixel_directions,
                              const value_type angular_threshold,
                              const int num_vis_tracks,
                              const string& output_track_filename,
                              vector<vector<int32_t> >& track_indices) {

  Image::Interp::Nearest<Image::BufferScratch<int32_t>::voxel_type> interp (fixel_indexer);
  float angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));
  DWI::Tractography::Properties tck_properties;
  DWI::Tractography::Reader<value_type> tck_reader (input_track_filename, tck_properties);
  double tck_timestamp = tck_properties.timestamp;
  DWI::Tractography::Writer<value_type> tck_writer (output_track_filename, tck_properties);
  DWI::Tractography::Streamline<value_type> tck;
  int counter = 0;

  while (tck_reader (tck) && counter < num_vis_tracks) {
    tck_writer (tck);
    vector<int32_t> indices (tck.size(), std::numeric_limits<int32_t>::max());
    Point<value_type> tangent;
    for (size_t p = 0; p < tck.size(); ++p) {
      interp.scanner (tck[p]);
      interp[3] = 0;
      int32_t first_index = interp.value();
      if (first_index >= 0) {
        interp[3] = 1;
        int32_t last_index = first_index + interp.value();
        int32_t closest_fixel_index = -1;
        value_type largest_dp = 0.0;
        if (p == 0)
          tangent = tck[p+1] - tck[p];
        else if (p == tck.size() - 1)
          tangent = tck[p] - tck[p-1];
        else
          tangent = tck[p+1] - tck[p-1];
        tangent.normalise();
        for (int32_t j = first_index; j < last_index; j++) {
          value_type dp = Math::abs (tangent.dot (fixel_directions[j]));
          if (dp > largest_dp) {
            largest_dp = dp;
            closest_fixel_index = j;
          }
        }
        if (largest_dp > angular_threshold_dp)
          indices[p] = closest_fixel_index;
      }
    }
    track_indices.push_back (indices);
    counter++;
  }
  tck_reader.close();
  return tck_timestamp;
}


void write_track_stats (const string& filename,
                        const vector<value_type>& data,
                        const vector<vector<int32_t> >& track_point_indices,
                        double tckfile_timestamp) {

  DWI::Tractography::Properties tck_properties;
  tck_properties.timestamp = tckfile_timestamp;
  DWI::Tractography::ScalarWriter<value_type> writer (filename, tck_properties);

  for (size_t t = 0; t < track_point_indices.size(); ++t) {
    vector<value_type > scalars (track_point_indices[t].size());
    for (size_t p = 0; p < track_point_indices[t].size(); ++p) {
      if (track_point_indices[t][p] == std::numeric_limits<int32_t>::max())
        scalars[p] = 0.0;
      else
        scalars[p] = data [track_point_indices[t][p]];
    }
    writer (scalars);
  }
}


void write_track_stats (const string& filename,
                        const Math::Matrix<value_type>& data,
                        const vector<vector<int32_t> >& track_point_indices,
                        double tckfile_timestamp) {
  vector<value_type> vec (data.rows());
  for (size_t i = 0; i < data.rows(); ++i)
    vec[i] = data(i, 0);
  write_track_stats (filename, vec, track_point_indices, tckfile_timestamp);
}

void write_track_stats (const string& filename,
                        const Math::Vector<value_type>& data,
                        const vector<vector<int32_t> >& track_point_indices,
                        double tckfile_timestamp) {
  vector<value_type> vec (data.size());
  for (size_t i = 0; i < data.size(); ++i)
    vec[i] = data[i];
  write_track_stats (filename, vec, track_point_indices, tckfile_timestamp);
}



/**
 * Run permutation test and output results
 */
void do_glm_and_output (const Math::Matrix<value_type>& data,
                        const Math::Matrix<value_type>& design,
                        const Math::Matrix<value_type>& contrast,
                        const value_type dh,
                        const value_type E,
                        const value_type H,
                        const int num_perms,
                        const vector<map<int32_t, Stats::TFCE::connectivity> >& fixel_connectivity,
                        Image::BufferScratch<int32_t>& fixel_indexer,
                        const vector<Point<value_type> >& fixel_directions,
                        const vector<vector<int32_t> >& track_point_indices,
                        string output_prefix,
                        double tckfile_timestamp) {

  int num_fixels = fixel_directions.size();
  Math::Vector<value_type> perm_distribution_pos (num_perms - 1);
  Math::Vector<value_type> perm_distribution_neg (num_perms - 1);
  vector<value_type> tfce_output_pos (num_fixels, 0.0);
  vector<value_type> tfce_output_neg (num_fixels, 0.0);
  vector<value_type> tvalue_output (num_fixels, 0.0);
  vector<value_type> pvalue_output_pos (num_fixels, 0.0);
  vector<value_type> pvalue_output_neg (num_fixels, 0.0);

  Math::Stats::GLMTTest glm (data, design, contrast);
  {
    Stats::TFCE::Connectivity tfce_integrator (fixel_connectivity, dh, E, H);
    Stats::TFCE::run (glm, tfce_integrator, num_perms,
                      perm_distribution_pos, perm_distribution_neg,
                      tfce_output_pos, tfce_output_neg, tvalue_output);
  }

  // Generate output
  perm_distribution_pos.save (output_prefix + "_perm_dist_pos.txt");
  perm_distribution_neg.save (output_prefix + "_perm_dist_neg.txt");
  Math::Stats::statistic2pvalue (perm_distribution_pos, tfce_output_pos, pvalue_output_pos);
  Math::Stats::statistic2pvalue (perm_distribution_neg, tfce_output_neg, pvalue_output_neg);

  write_track_stats (output_prefix + "_tfce_pos.tsf", tfce_output_pos, track_point_indices, tckfile_timestamp);
  write_track_stats (output_prefix + "_tfce_neg.tsf", tfce_output_neg, track_point_indices, tckfile_timestamp);
  write_track_stats (output_prefix + "_tvalue.tsf", tvalue_output, track_point_indices, tckfile_timestamp);
  write_track_stats (output_prefix + "_pvalue_pos.tsf", pvalue_output_pos, track_point_indices, tckfile_timestamp);
  write_track_stats (output_prefix + "_pvalue_neg.tsf", pvalue_output_neg, track_point_indices, tckfile_timestamp);
}



void load_data_and_compute_integrals (const vector<string>& filename_list,
                                      Image::BufferScratch<bool>& fixel_mask,
                                      Image::BufferScratch<int32_t>& fixel_indexer,
                                      const vector<Point<value_type> >& fixel_directions,
                                      const value_type angular_threshold,
                                      const vector<map<int32_t, value_type> >& fixel_smoothing_weights,
                                      Math::Matrix<value_type>& fod_fixel_integrals) {

  fod_fixel_integrals.resize (fixel_directions.size(), filename_list.size(), 0.0);
  ProgressBar progress ("loading FOD images and computing integrals...", filename_list.size());
  DWI::Directions::Set dirs (1281);

  for (size_t subject = 0; subject < filename_list.size(); subject++) {
    LogLevelLatch log_level (0);
    Image::Buffer<value_type> fod_buffer (filename_list[subject]);
    Image::check_dimensions (fod_buffer, fixel_mask, 0, 3);
    DWI::FMLS::FODQueueWriter<Image::Buffer<value_type>, Image::BufferScratch<bool> > writer2 (fod_buffer, fixel_mask);
    DWI::FMLS::Segmenter fmls (dirs, Math::SH::LforN (fod_buffer.dim(3)));
    fmls.set_peak_value_threshold (SUBJECT_FOD_THRESHOLD);
    vector<value_type> temp_fixel_integrals (fixel_directions.size(), 0.0);
    SubjectFixelProcessor fixel_processor (fixel_indexer, fixel_directions, temp_fixel_integrals, angular_threshold);
    Thread::run_queue (writer2, DWI::FMLS::SH_coefs(), Thread::multi (fmls), DWI::FMLS::FOD_lobes(), fixel_processor);

    // Smooth the data based on connectivity
    for (size_t fixel = 0; fixel < fixel_directions.size(); ++fixel) {
      value_type value = 0.0;
      map<int32_t, value_type>::const_iterator it = fixel_smoothing_weights[fixel].begin();
      for (; it != fixel_smoothing_weights[fixel].end(); ++it)
        value += temp_fixel_integrals[it->first] * it->second;
      fod_fixel_integrals (fixel, subject) = value;
    }
    progress++;
  }
}



void run() {

  Options opt = get_options ("dh");
  value_type dh = 0.1;
  if (opt.size())
    dh = opt[0][0];

  opt = get_options ("tfce_h");
  value_type tfce_H = 2.0;
  if (opt.size())
    tfce_H = opt[0][0];

  opt = get_options ("tfce_e");
  value_type tfce_E = 1.0;
  if (opt.size())
    tfce_E = opt[0][0];

  opt = get_options ("tfce_c");
  value_type tfce_C = 0.5;
  if (opt.size())
    tfce_C = opt[0][0];

  opt = get_options ("nperms");
  int num_perms = 5000;
  if (opt.size())
    num_perms = opt[0][0];

  value_type angular_threshold = 30.0;
  opt = get_options ("angle");
  if (opt.size())
    angular_threshold = opt[0][0];

  value_type connectivity_threshold = 0.01;
  opt = get_options ("connectivity");
  if (opt.size())
    connectivity_threshold = opt[0][0];

  value_type smooth_std_dev = 5.0 / 2.3548;
  opt = get_options ("smooth");
  if (opt.size())
    smooth_std_dev = value_type(opt[0][0]) / 2.3548;

  opt = get_options ("num_vis_tracks");
  size_t num_vis_tracks = 200000;
  if (opt.size())
    num_vis_tracks = opt[0][0];

  // Read filenames
  vector<string> fod_filenames;
  {
    string folder = Path::dirname (argument[0]);
    ifstream ifs (argument[0].c_str());
    string temp;
    while (getline (ifs, temp))
      fod_filenames.push_back (Path::join (folder, temp));
  }
  vector<string> mod_fod_filenames;
  {
    string folder = Path::dirname (argument[1]);
    ifstream ifs (argument[1].c_str());
    string temp;
    while (getline (ifs, temp))
      mod_fod_filenames.push_back (Path::join (folder, temp));
  }

  if (mod_fod_filenames.size() != fod_filenames.size())
    throw Exception ("the number of fod and modulated fod images in the file lists is not the same");

  // Load design matrix:
  Math::Matrix<value_type> design;
  design.load (argument[2]);
  if (design.rows() != fod_filenames.size())
    throw Exception ("number of subjects does not match number of rows in design matrix");

  // Load contrast matrix:
  Math::Matrix<value_type> contrast;
  contrast.load (argument[3]);

  if (contrast.columns() > design.columns())
    throw Exception ("too many contrasts for design matrix");
  contrast.resize (contrast.rows(), design.columns());

  // Identify fixels on group average FOD image
  vector<Point<value_type> > fixel_directions;
  DWI::Directions::Set dirs (1281);
  Image::Header index_header (argument[4]);
  index_header.dim(3) = 2;
  Image::BufferScratch<int32_t> fixel_indexer (index_header);
  Image::BufferScratch<int32_t>::voxel_type fixel_indexer_vox (fixel_indexer);
  vector<Point<value_type> > fixel_positions;
  Image::LoopInOrder loop4D (fixel_indexer_vox);
  for (loop4D.start (fixel_indexer_vox); loop4D.ok(); loop4D.next (fixel_indexer_vox))
    fixel_indexer_vox.value() = -1;

  {
    Image::Buffer<value_type> av_fod_buffer (argument[4]);
    Image::Buffer<bool> brain_mask_buffer (argument[5]);
    Image::check_dimensions (av_fod_buffer, brain_mask_buffer, 0, 3);
    DWI::FMLS::FODQueueWriter <Image::Buffer<value_type>, Image::Buffer<bool> > writer (av_fod_buffer, brain_mask_buffer);
    DWI::FMLS::Segmenter fmls (dirs, Math::SH::LforN (av_fod_buffer.dim(3)));
    fmls.set_peak_value_threshold (GROUP_AVERAGE_FOD_THRESHOLD);
    GroupAvFixelProcessor fixel_processor (fixel_indexer, fixel_directions, fixel_positions);
    Thread::run_queue (writer, DWI::FMLS::SH_coefs(), Thread::multi (fmls), DWI::FMLS::FOD_lobes(), fixel_processor);
  }

  uint32_t num_fixels = fixel_directions.size();
  CONSOLE ("number of fixels: " + str(num_fixels));

  // Compute 3D analysis mask based on fixels in average FOD image
  Image::Header header3D (index_header);
  header3D.set_ndim(3);
  Image::BufferScratch<bool> fixel_mask (header3D);
  Image::BufferScratch<bool>::voxel_type fixel_mask_vox (fixel_mask);
  Image::Loop loop (0, 3);
  for (loop.start (fixel_indexer_vox, fixel_mask_vox); loop.ok(); loop.next (fixel_indexer_vox, fixel_mask_vox)) {
    fixel_indexer_vox[3] = 0;
    if (fixel_indexer_vox.value() >= 0) {
      fixel_mask_vox.value() = 1;
      fixel_indexer_vox[3] = 1;
    } else {
      fixel_mask_vox.value() = 0;
    }
  }


  // Check number of fixels per voxel
  opt = get_options ("check");
  if (opt.size()) {
    Image::Buffer<value_type> fibre_count_buffer (opt[0][0], header3D);
    Image::Buffer<value_type>::voxel_type fibre_count_buffer_vox (fibre_count_buffer);
    Image::Loop loop (0, 3);
    for (loop.start (fixel_indexer_vox, fibre_count_buffer_vox); loop.ok(); loop.next (fixel_indexer_vox, fibre_count_buffer_vox)) {
      fixel_indexer_vox[3] = 0;
      int32_t fixel_index = fixel_indexer_vox.value();
      if (fixel_index >= 0) {
        fixel_indexer_vox[3] = 1;
        fibre_count_buffer_vox.value() = fixel_indexer_vox.value();
      }
    }
  }

  vector<map<int32_t, Stats::TFCE::connectivity> > fixel_connectivity (num_fixels);
  vector<uint16_t> fixel_TDI (num_fixels, 0.0);
  vector<vector<int32_t> > track_point_indices;
  string track_filename = argument[6];
  string output_prefix = argument[7];
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> track_file (track_filename, properties);
  double tckfile_timestamp;
  {
    // Read in tracts, and compute whole-brain fixel-fixel connectivity
    const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
    if (!num_tracks)
      throw Exception ("error with track count in input file");
    if (num_vis_tracks > num_tracks) {
      WARN ("the number of visualisation tracts is larger than the total available. Setting -num_vis_tracks to " + str(num_tracks));
      num_vis_tracks = num_tracks;
    }

    tckfile_timestamp = compute_track_indices (track_filename, fixel_indexer, fixel_directions, angular_threshold, num_vis_tracks, output_prefix + "_tracks.tck", track_point_indices);

    typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;
    DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "pre-computing fixel-fixel connectivity...");
    Image::Header header (argument[4]);
    DWI::Tractography::Mapping::TrackMapperBase<SetVoxelDir> mapper (header);
    TrackProcessor tract_processor (fixel_indexer, fixel_directions, fixel_TDI, fixel_connectivity, angular_threshold);
    Thread::run_queue (
        loader, 
        Thread::batch (DWI::Tractography::Streamline<float>()), 
        mapper, 
        Thread::batch (SetVoxelDir()), 
        tract_processor);
  }
  track_file.close();


  // Normalise connectivity matrix and threshold, pre-compute fixel-fixel weights for smoothing.
  vector<map<int32_t, value_type> > fixel_smoothing_weights (num_fixels);
  bool do_smoothing;
  const value_type gaussian_const2 = 2.0 * smooth_std_dev * smooth_std_dev;
  value_type gaussian_const1 = 1.0;
  if (smooth_std_dev > 0.0) {
    do_smoothing = true;
    gaussian_const1 = 1.0 / (smooth_std_dev *  Math::sqrt (2.0 * M_PI));
  }
  {
    ProgressBar progress ("normalising and thresholding fixel-fixel connectivity matrix...", num_fixels);
    for (unsigned int fixel = 0; fixel < num_fixels; ++fixel) {
      map<int32_t, Stats::TFCE::connectivity>::iterator it = fixel_connectivity[fixel].begin();
      while (it != fixel_connectivity[fixel].end()) {
        value_type connectivity = it->second.value / value_type (fixel_TDI[fixel]);
        if (connectivity < connectivity_threshold)  {
          fixel_connectivity[fixel].erase (it++);
        } else {
          if (do_smoothing) {
            value_type distance = Math::sqrt (Math::pow2 (fixel_positions[fixel][0] - fixel_positions[it->first][0]) +
                                              Math::pow2 (fixel_positions[fixel][1] - fixel_positions[it->first][1]) +
                                              Math::pow2 (fixel_positions[fixel][2] - fixel_positions[it->first][2]));
            value_type weight = connectivity * gaussian_const1 * Math::exp (-Math::pow2 (distance) / gaussian_const2);
            if (weight > connectivity_threshold)
              fixel_smoothing_weights[fixel].insert (pair<int32_t, value_type> (it->first, weight));
          }
          it->second.value = Math::pow (connectivity, tfce_C);
          ++it;
        }
      }
      // Make sure the fixel is fully connected to itself giving it a smoothing weight of 1
      Stats::TFCE::connectivity self_connectivity;
      self_connectivity.value = 1.0;
      fixel_connectivity[fixel].insert (pair<int32_t, Stats::TFCE::connectivity> (fixel, self_connectivity));
      fixel_smoothing_weights[fixel].insert (pair<int32_t, value_type> (fixel, gaussian_const1));
      progress++;
    }
  }

  // Normalise smoothing weights
  for (size_t i = 0; i < num_fixels; ++i) {
    value_type sum = 0.0;
    for (map<int32_t, value_type>::iterator it = fixel_smoothing_weights[i].begin(); it != fixel_smoothing_weights[i].end(); ++it)
      sum += it->second;
    value_type norm_factor = 1.0 / sum;
    for (map<int32_t, value_type>::iterator it = fixel_smoothing_weights[i].begin(); it != fixel_smoothing_weights[i].end(); ++it)
      it->second *= norm_factor;
  }

  // Load subject data and compute FOD integrals within the mask
  Math::Matrix<value_type> fod_fixel_integrals;
  Math::Matrix<value_type> mod_fod_fixel_integrals;
  load_data_and_compute_integrals (fod_filenames, fixel_mask, fixel_indexer, fixel_directions, angular_threshold, fixel_smoothing_weights, fod_fixel_integrals);
  load_data_and_compute_integrals (mod_fod_filenames, fixel_mask, fixel_indexer, fixel_directions, angular_threshold, fixel_smoothing_weights, mod_fod_fixel_integrals);

  CONSOLE ("outputting beta coefficients, effect size and standard deviation");
  Math::Matrix<float> temp;

  Math::Stats::GLM::solve_betas (fod_fixel_integrals, design, temp);
  for (size_t i = 0; i < contrast.columns(); ++i)
    write_track_stats (output_prefix + "_fod_beta" + str(i) + ".tsf", temp.column (i), track_point_indices, tckfile_timestamp);
  Math::Stats::GLM::solve_betas (mod_fod_fixel_integrals, design, temp);
  for (size_t i = 0; i < contrast.columns(); ++i)
    write_track_stats (output_prefix + "_mod_fod_beta" + str(i) + ".tsf", temp.column (i), track_point_indices, tckfile_timestamp);

  Math::Stats::GLM::abs_effect_size (fod_fixel_integrals, design, contrast, temp);
  write_track_stats (output_prefix + "_fod_abs_effect_size.tsf", temp, track_point_indices, tckfile_timestamp);
  Math::Stats::GLM::std_effect_size (fod_fixel_integrals, design, contrast, temp);
  write_track_stats (output_prefix + "_fod_std_effect_size.tsf", temp, track_point_indices, tckfile_timestamp);
  Math::Stats::GLM::stdev (fod_fixel_integrals, design, temp);
  write_track_stats (output_prefix + "_fod_std_dev.tsf", temp, track_point_indices, tckfile_timestamp);
  Math::Stats::GLM::abs_effect_size (mod_fod_fixel_integrals, design, contrast, temp);
  write_track_stats (output_prefix + "_mod_fod_abs_effect_size.tsf", temp, track_point_indices, tckfile_timestamp);
  Math::Stats::GLM::std_effect_size (mod_fod_fixel_integrals, design, contrast, temp);
  write_track_stats (output_prefix + "_mod_fod_std_effect_size.tsf", temp, track_point_indices, tckfile_timestamp);
  Math::Stats::GLM::stdev (mod_fod_fixel_integrals, design, temp);
  write_track_stats (output_prefix + "_mod_fod_std_dev.tsf", temp, track_point_indices, tckfile_timestamp);


  // Perform permutation testing
  opt = get_options("notest");
  if (!opt.size()) {
    // Modulated FODs
    do_glm_and_output (mod_fod_fixel_integrals, design, contrast, dh, tfce_E, tfce_H, num_perms, fixel_connectivity, fixel_indexer, fixel_directions, track_point_indices, output_prefix + "_mod_fod", tckfile_timestamp);
    // FOD information only
    do_glm_and_output (fod_fixel_integrals, design, contrast, dh, tfce_E, tfce_H, num_perms, fixel_connectivity, fixel_indexer, fixel_directions, track_point_indices, output_prefix + "_fod", tckfile_timestamp);
  }

}

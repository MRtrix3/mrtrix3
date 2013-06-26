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
#include "dwi/tractography/SIFT/multithread.h"

MRTRIX_APPLICATION

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
                              "These tracts are obtained by truncating the input tracks (default: 100000")
  + Argument ("num").type_integer (1, 100000, INT_MAX)

  + Option ("check", "output a 3D image to check the number of dixels per voxel identified in the template")
  + Argument ("image").type_image_out ();
}


/**
 * For each dixel in the group average image, save the first dixel index and number of
 * voxel dixels into the corresponding voxel of the FOD_dixel_indexer. Store the
 * dixel direction and the scanner position.
 */
class GroupAvDixelProcessor
{
  public:
    GroupAvDixelProcessor (Image::BufferScratch<int32_t>& FOD_dixel_indexer,
                              vector<Point<value_type> >& FOD_dixel_directions,
                              vector<Point<value_type> >& index2scanner_pos) :
                              FOD_dixel_indexer (FOD_dixel_indexer) ,
                              FOD_dixel_directions (FOD_dixel_directions),
                              index2scanner_pos (index2scanner_pos),
                              image_transform (FOD_dixel_indexer) {}

    bool operator () (DWI::FMLS::FOD_lobes& in) {
      if (in.empty())
         return true;
      FOD_dixel_indexer[0] = in.vox[0];
      FOD_dixel_indexer[1] = in.vox[1];
      FOD_dixel_indexer[2] = in.vox[2];
      FOD_dixel_indexer[3] = 0;
      FOD_dixel_indexer.value() = FOD_dixel_directions.size();
      int32_t dixel_count = 0;
      for (vector<DWI::FMLS::FOD_lobe>::const_iterator i = in.begin(); i != in.end(); ++i, ++dixel_count) {
        FOD_dixel_directions.push_back (i->get_peak_dir());
        Point<value_type> pos;
        image_transform.voxel2scanner (FOD_dixel_indexer, pos);
        index2scanner_pos.push_back (pos);
      }
      FOD_dixel_indexer[3] = 1;
      FOD_dixel_indexer.value() = dixel_count;
      return true;
    }


  private:
    Image::BufferScratch<int32_t>::voxel_type FOD_dixel_indexer;
    vector<Point<value_type> >& FOD_dixel_directions;
    vector<Point<value_type> >& index2scanner_pos;
    Image::Transform image_transform;
};



class SubjectDixelProcessor {

  public:
    SubjectDixelProcessor (Image::BufferScratch<int32_t>& FOD_dixel_indexer,
                              const vector<Point<value_type> >& FOD_dixel_directions,
                              vector<value_type>& subject_dixel_integrals,
                              value_type angular_threshold) :
                              FOD_dixel_indexer (FOD_dixel_indexer),
                              average_dixel_directions (FOD_dixel_directions),
                              subject_dixel_integrals (subject_dixel_integrals)
    {
      angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));
    }

    bool operator () (DWI::FMLS::FOD_lobes& in)
    {
      if (in.empty())
        return true;

      FOD_dixel_indexer[0] = in.vox[0];
      FOD_dixel_indexer[1] = in.vox[1];
      FOD_dixel_indexer[2] = in.vox[2];
      FOD_dixel_indexer[3] = 0;
      int32_t voxel_index = FOD_dixel_indexer.value();
      FOD_dixel_indexer[3] = 1;
      int32_t number_dixels = FOD_dixel_indexer.value();

      // for each dixel in the average, find the corresponding dixel in this subject voxel
      for (int32_t i = voxel_index; i < voxel_index + number_dixels; ++i) {
        value_type largest_dp = 0.0;
        int largest_index = -1;
        for (size_t j = 0; j < in.size(); ++j) {
          value_type dp = Math::abs (average_dixel_directions[i].dot(in[j].get_peak_dir()));
          if (dp > largest_dp) {
            largest_dp = dp;
            largest_index = j;
          }
        }
        if (largest_dp > angular_threshold_dp) {
          subject_dixel_integrals[i] = in[largest_index].get_integral();
        }
      }

      return true;
    }


  private:
    Image::BufferScratch<int32_t>::voxel_type FOD_dixel_indexer;
    const vector<Point<value_type> >& average_dixel_directions;
    vector<value_type>& subject_dixel_integrals;
    value_type angular_threshold_dp;
};


/**
 * Process each track (represented as a set of dixels). For each track tangent dixel, identify the closest group average dixel
 */
class TrackProcessor {

  public:
    TrackProcessor (Image::BufferScratch<int32_t>& FOD_dixel_indexer,
                      const vector<Point<value_type> >& FOD_dixel_directions,
                      vector<uint16_t>& dixel_TDI,
                      vector<map<int32_t, Stats::TFCE::connectivity> >& dixel_connectivity,
                      value_type angular_threshold):
                      dixel_indexer (FOD_dixel_indexer) ,
                      dixel_directions (FOD_dixel_directions),
                      dixel_TDI (dixel_TDI),
                      dixel_connectivity (dixel_connectivity) {
      angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));
    }

    bool operator () (SetVoxelDir& in)
    {
      // For each voxel tract tangent, assign to a dixel
      vector<int32_t> tract_dixel_indices;
      for (SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
        Image::Nav::set_pos (dixel_indexer, *i);
        dixel_indexer[3] = 0;
        int32_t first_index = dixel_indexer.value();
        if (first_index >= 0) {
          dixel_indexer[3] = 1;
          int32_t last_index = first_index + dixel_indexer.value();
          int32_t closest_dixel_index = -1;
          value_type largest_dp = 0.0;
          Point<value_type> dir (i->get_dir());
          dir.normalise();
          for (int32_t j = first_index; j < last_index; ++j) {
            value_type dp = Math::abs (dir.dot (dixel_directions[j]));
            if (dp > largest_dp) {
              largest_dp = dp;
              closest_dixel_index = j;
            }
          }
          if (largest_dp > angular_threshold_dp) {
            tract_dixel_indices.push_back (closest_dixel_index);
            dixel_TDI[closest_dixel_index]++;
          }
        }
      }

      for (size_t i = 0; i < tract_dixel_indices.size(); i++) {
        for (size_t j = i + 1; j < tract_dixel_indices.size(); j++) {
          dixel_connectivity[tract_dixel_indices[i]][tract_dixel_indices[j]].value++;
          dixel_connectivity[tract_dixel_indices[j]][tract_dixel_indices[i]].value++;
        }
      }
      return true;
    }

  private:
    Image::BufferScratch<int32_t>::voxel_type dixel_indexer;
    const vector<Point<value_type> >& dixel_directions;
    vector<uint16_t>& dixel_TDI;
    vector<map<int32_t, Stats::TFCE::connectivity> >& dixel_connectivity;
    value_type angular_threshold_dp;
};



/**
 * Processes each track in the tractogram used for output display (most likely a subset
 * of all tracks since we need less tracks for display than for estimating dixel-dixel connectivity)
 * and computes the FOD dixel indexes for each track point. Indices are stored for later
 * use when outputting various track-point statistics. This function also writes out the tractogram subset.
 */
double compute_track_indices (const string& input_track_filename,
                                  Image::BufferScratch<int32_t>& dixel_indexer,
                                  const vector<Point<value_type> >& dixel_directions,
                                  const value_type angular_threshold,
                                  const int num_vis_tracks,
                                  const string& output_track_filename,
                                  vector<vector<int32_t> >& track_indices) {

  Image::Interp::Nearest<Image::BufferScratch<int32_t>::voxel_type> interp (dixel_indexer);
  float angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));
  DWI::Tractography::Properties tck_properties;
  DWI::Tractography::Reader<value_type> tck_reader (input_track_filename, tck_properties);
  double tck_timestamp = tck_properties.timestamp;
  DWI::Tractography::Writer<value_type> tck_writer (output_track_filename, tck_properties);
  vector<Point<value_type> > tck;
  int counter = 0;

  while (tck_reader.next (tck) && counter < num_vis_tracks) {
    tck_writer.append (tck);
    vector<int32_t> indices (tck.size(), std::numeric_limits<int32_t>::max());
    Point<value_type> tangent;
    for (size_t p = 0; p < tck.size(); ++p) {
      interp.scanner (tck[p]);
      interp[3] = 0;
      int32_t first_index = interp.value();
      if (first_index >= 0) {
        interp[3] = 1;
        int32_t last_index = first_index + interp.value();
        int32_t closest_dixel_index = -1;
        value_type largest_dp = 0.0;
        if (p == 0)
          tangent = tck[p+1] - tck[p];
        else if (p == tck.size() - 1)
          tangent = tck[p] - tck[p-1];
        else
          tangent = tck[p+1] - tck[p-1];
        tangent.normalise();
        for (int32_t j = first_index; j < last_index; j++) {
          value_type dp = Math::abs (tangent.dot (dixel_directions[j]));
          if (dp > largest_dp) {
            largest_dp = dp;
            closest_dixel_index = j;
          }
        }
        if (largest_dp > angular_threshold_dp)
          indices[p] = closest_dixel_index;
      }
    }
    track_indices.push_back (indices);
    counter++;
  }
  tck_reader.close();
  return tck_timestamp;
}


/**
 * Write out scalar values associated with a track file. Currently scalars are stored
 * within a tractography file, however a specific reader and writer will be implemented in the future
 */
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
    writer.append (scalars);
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
                        const vector<map<int32_t, Stats::TFCE::connectivity> >& dixel_connectivity,
                        Image::BufferScratch<int32_t>& dixel_indexer,
                        const vector<Point<value_type> >& dixel_directions,
                        const vector<vector<int32_t> >& track_point_indices,
                        string output_prefix,
                        double tckfile_timestamp) {

  int num_dixels = dixel_directions.size();
  Math::Vector<value_type> perm_distribution_pos (num_perms - 1);
  Math::Vector<value_type> perm_distribution_neg (num_perms - 1);
  vector<value_type> tfce_output_pos (num_dixels, 0.0);
  vector<value_type> tfce_output_neg (num_dixels, 0.0);
  vector<value_type> tvalue_output (num_dixels, 0.0);
  vector<value_type> pvalue_output_pos (num_dixels, 0.0);
  vector<value_type> pvalue_output_neg (num_dixels, 0.0);

  Math::Stats::GLMTTest glm (data, design, contrast);
  {
    Stats::TFCE::Connectivity tfce_integrator (dixel_connectivity, dh, E, H);
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
                                      Image::BufferScratch<bool>& dixel_mask,
                                      Image::BufferScratch<int32_t>& dixel_indexer,
                                      const vector<Point<value_type> >& dixel_directions,
                                      const value_type angular_threshold,
                                      const vector<map<int32_t, value_type> >& dixel_smoothing_weights,
                                      Math::Matrix<value_type>& fod_dixel_integrals) {

  fod_dixel_integrals.resize (dixel_directions.size(), filename_list.size(), 0.0);
  ProgressBar progress ("loading FOD images and computing integrals...", filename_list.size());
  DWI::Directions::Set dirs (1281);

  for (size_t subject = 0; subject < filename_list.size(); subject++) {

    LogLevelLatch log_level (0);
    Image::Buffer<value_type> fod_buffer (filename_list[subject]);
    Image::check_dimensions (fod_buffer, dixel_mask, 0, 3);
    DWI::Tractography::SIFT::FODQueueWriter<Image::Buffer<value_type>, Image::BufferScratch<bool> > writer2 (fod_buffer, dixel_mask);
    DWI::FMLS::Segmenter fmls (dirs, Math::SH::LforN (fod_buffer.dim(3)));
    fmls.set_peak_value_threshold (SUBJECT_FOD_THRESHOLD);
    vector<value_type> temp_dixel_integrals (dixel_directions.size(), 0.0);
    SubjectDixelProcessor dixel_processor (dixel_indexer, dixel_directions, temp_dixel_integrals, angular_threshold);
    Thread::run_queue_threaded_pipe (writer2, DWI::FMLS::SH_coefs(), fmls, DWI::FMLS::FOD_lobes(), dixel_processor);

    // Smooth the data based on connectivity
    for (size_t dixel = 0; dixel < dixel_directions.size(); ++dixel) {
      value_type value = 0.0;
      map<int32_t, value_type>::const_iterator it = dixel_smoothing_weights[dixel].begin();
      for (; it != dixel_smoothing_weights[dixel].end(); ++it)
        value += temp_dixel_integrals[it->first] * it->second;
      fod_dixel_integrals (dixel, subject) = value;
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
  size_t num_vis_tracks = 100000;
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

  // Compute FOD dixels on average FOD image
  vector<Point<value_type> > dixel_directions;
  DWI::Directions::Set dirs (1281);
  Image::Header index_header (argument[4]);
  index_header.dim(3) = 2;
  Image::BufferScratch<int32_t> dixel_indexer (index_header);
  Image::BufferScratch<int32_t>::voxel_type dixel_indexer_vox (dixel_indexer);
  vector<Point<value_type> > dixel_positions;
  Image::LoopInOrder loop4D (dixel_indexer_vox);
  for (loop4D.start (dixel_indexer_vox); loop4D.ok(); loop4D.next (dixel_indexer_vox))
    dixel_indexer_vox.value() = -1;

  {
    Image::Buffer<value_type> av_fod_buffer (argument[4]);
    Image::Buffer<bool> brain_mask_buffer (argument[5]);
    Image::check_dimensions (av_fod_buffer, brain_mask_buffer, 0, 3);
    DWI::Tractography::SIFT::FODQueueWriter <Image::Buffer<value_type>, Image::Buffer<bool> > writer (av_fod_buffer, brain_mask_buffer);
    DWI::FMLS::Segmenter fmls (dirs, Math::SH::LforN (av_fod_buffer.dim(3)));
    fmls.set_peak_value_threshold (GROUP_AVERAGE_FOD_THRESHOLD);
    GroupAvDixelProcessor dixel_processor (dixel_indexer, dixel_directions, dixel_positions);
    Thread::run_queue_threaded_pipe (writer, DWI::FMLS::SH_coefs(), fmls, DWI::FMLS::FOD_lobes(), dixel_processor);
  }

  uint32_t num_dixels = dixel_directions.size();
  CONSOLE ("number of dixels: " + str(num_dixels));

  // Compute 3D analysis mask based on dixels in average FOD image
  Image::Header header3D (index_header);
  header3D.set_ndim(3);
  Image::BufferScratch<bool> dixel_mask (header3D);
  Image::BufferScratch<bool>::voxel_type dixel_mask_vox (dixel_mask);
  Image::Loop loop (0, 3);
  for (loop.start (dixel_indexer_vox, dixel_mask_vox); loop.ok(); loop.next (dixel_indexer_vox, dixel_mask_vox)) {
    dixel_indexer_vox[3] = 0;
    if (dixel_indexer_vox.value() >= 0) {
      dixel_mask_vox.value() = 1;
      dixel_indexer_vox[3] = 1;
    } else {
      dixel_mask_vox.value() = 0;
    }
  }


  // Check number of dixels per voxel
  opt = get_options ("check");
  if (opt.size()) {
    Image::Buffer<value_type> fibre_count_buffer (opt[0][0], header3D);
    Image::Buffer<value_type>::voxel_type fibre_count_buffer_vox (fibre_count_buffer);
    Image::Loop loop (0, 3);
    for (loop.start (dixel_indexer_vox, fibre_count_buffer_vox); loop.ok(); loop.next (dixel_indexer_vox, fibre_count_buffer_vox)) {
      dixel_indexer_vox[3] = 0;
      int32_t dixel_index = dixel_indexer_vox.value();
      if (dixel_index >= 0) {
        dixel_indexer_vox[3] = 1;
        fibre_count_buffer_vox.value() = dixel_indexer_vox.value();
      }
    }
  }

  vector<map<int32_t, Stats::TFCE::connectivity> > dixel_connectivity (num_dixels);
  vector<uint16_t> dixel_TDI (num_dixels, 0.0);
  vector<vector<int32_t> > track_point_indices;
  string track_filename = argument[6];
  string output_prefix = argument[7];
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<value_type> track_file (track_filename, properties);
  double tckfile_timestamp;
  {
    // Read in tracts, and computer whole-brain dixel-dixel connectivity
    const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
    if (!num_tracks)
      throw Exception ("error with track count in input file");
    if (num_vis_tracks > num_tracks) {
      WARN("the number of visualisation tracts is larger than the total available. Setting num_vis_tracks to " + str(num_tracks));
      num_vis_tracks = num_tracks;
    }

    tckfile_timestamp = compute_track_indices (track_filename, dixel_indexer, dixel_directions, angular_threshold, num_vis_tracks, output_prefix + "_tracks.tck", track_point_indices);

    typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;
    DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "pre-computing dixel-dixel connectivity...");
    Image::Header header (argument[4]);
    DWI::Tractography::Mapping::TrackMapperBase<SetVoxelDir> mapper (header);
    TrackProcessor tract_processor (dixel_indexer, dixel_directions, dixel_TDI, dixel_connectivity, angular_threshold);
    Thread::run_queue_custom_threading (loader, 1, DWI::Tractography::Mapping::TrackAndIndex(), mapper, 1, SetVoxelDir(), tract_processor, 1);
  }
  track_file.close();


  // Normalise connectivity matrix and threshold, pre-compute dixel-dixel weights for smoothing.
  vector<map<int32_t, value_type> > dixel_smoothing_weights (num_dixels);
  bool do_smoothing;
  const value_type gaussian_const2 = 2.0 * smooth_std_dev * smooth_std_dev;
  value_type gaussian_const1 = 1.0;
  if (smooth_std_dev > 0.0) {
    do_smoothing = true;
    gaussian_const1 = 1.0 / (smooth_std_dev *  Math::sqrt (2.0 * M_PI));
  }
  for (unsigned int dixel = 0; dixel < num_dixels; ++dixel) {
    map<int32_t, Stats::TFCE::connectivity>::iterator it = dixel_connectivity[dixel].begin();
    while (it != dixel_connectivity[dixel].end()) {
      value_type connectivity = it->second.value / value_type (dixel_TDI[dixel]);
      if (connectivity < connectivity_threshold)  {
        dixel_connectivity[dixel].erase (it++);
      } else {
        if (do_smoothing) {
          value_type distance = Math::sqrt (Math::pow2 (dixel_positions[dixel][0] - dixel_positions[it->first][0]) +
                                            Math::pow2 (dixel_positions[dixel][1] - dixel_positions[it->first][1]) +
                                            Math::pow2 (dixel_positions[dixel][2] - dixel_positions[it->first][2]));
          value_type weight = connectivity * gaussian_const1 * Math::exp (-Math::pow2 (distance) / gaussian_const2);
          if (weight > 0.005)
            dixel_smoothing_weights[dixel].insert (pair<int32_t, value_type> (it->first, weight));
        }
        it->second.value = Math::pow (connectivity, tfce_C);
        ++it;
      }
    }
    // Make sure the dixel is fully connected to itself and give it a smoothing weight
    Stats::TFCE::connectivity self_connectivity;
    self_connectivity.value = 1.0;
    dixel_connectivity[dixel].insert (pair<int32_t, Stats::TFCE::connectivity> (dixel, self_connectivity));
    dixel_smoothing_weights[dixel].insert (pair<int32_t, value_type> (dixel, gaussian_const1));
  }

  // Normalise smoothing weights
  for (size_t i = 0; i < num_dixels; ++i) {
    value_type sum = 0.0;
    for (map<int32_t, value_type>::iterator it = dixel_smoothing_weights[i].begin(); it != dixel_smoothing_weights[i].end(); ++it)
      sum += it->second;
    value_type norm_factor = 1.0 / sum;
    for (map<int32_t, value_type>::iterator it = dixel_smoothing_weights[i].begin(); it != dixel_smoothing_weights[i].end(); ++it)
      it->second *= norm_factor;
  }

  // Load subject data and compute FOD integrals within the mask
  Math::Matrix<value_type> fod_dixel_integrals;
  Math::Matrix<value_type> mod_fod_dixel_integrals;
  load_data_and_compute_integrals (fod_filenames, dixel_mask, dixel_indexer, dixel_directions, angular_threshold, dixel_smoothing_weights, fod_dixel_integrals);
  load_data_and_compute_integrals (mod_fod_filenames, dixel_mask, dixel_indexer, dixel_directions, angular_threshold, dixel_smoothing_weights, mod_fod_dixel_integrals);
  Math::Matrix<value_type> log_mod_scale_factor (num_dixels, fod_filenames.size());


  // Compute and output effect size and std_deviation
  Math::Matrix<float> abs_effect_size, std_effect_size, std_dev, beta;
  Math::Stats::GLM::abs_effect_size (fod_dixel_integrals, design, contrast, abs_effect_size);
  write_track_stats (output_prefix + "_fod_abs_effect_size.tsf", abs_effect_size, track_point_indices, tckfile_timestamp);
  Math::Stats::GLM::std_effect_size (fod_dixel_integrals, design, contrast, std_effect_size);
  write_track_stats (output_prefix + "_fod_std_effect_size.tsf", std_effect_size, track_point_indices, tckfile_timestamp);
  Math::Stats::GLM::stdev (fod_dixel_integrals, design, std_dev);
  write_track_stats (output_prefix + "_fod_std_dev.tsf", std_dev, track_point_indices, tckfile_timestamp);
  Math::Stats::GLM::abs_effect_size (mod_fod_dixel_integrals, design, contrast, abs_effect_size);
  write_track_stats (output_prefix + "_mod_fod_abs_effect_size.tsf", abs_effect_size, track_point_indices, tckfile_timestamp);
  Math::Stats::GLM::std_effect_size (mod_fod_dixel_integrals, design, contrast, std_effect_size);
  write_track_stats (output_prefix + "_mod_fod_std_effect_size.tsf", std_effect_size, track_point_indices, tckfile_timestamp);
  Math::Stats::GLM::stdev (mod_fod_dixel_integrals, design, std_dev);
  write_track_stats (output_prefix + "_mod_fod_std_dev.tsf", std_dev, track_point_indices, tckfile_timestamp);

  Math::Stats::GLM::solve_betas (fod_dixel_integrals, design, beta);
  std::cout << beta.columns() << " " << beta.rows() << std::endl;
  std::cout << contrast << std::endl;
  for (size_t i = 0; i < contrast.columns(); ++i)
    write_track_stats (output_prefix + "_fod_beta" + str(i) + ".tsf", beta.column (i), track_point_indices, tckfile_timestamp);
  Math::Stats::GLM::solve_betas (mod_fod_dixel_integrals, design, beta);
  for (size_t i = 0; i < contrast.columns(); ++i)
    write_track_stats (output_prefix + "_mod_fod_beta" + str(i) + ".tsf", beta.column (i), track_point_indices, tckfile_timestamp);

//  // Extract the amount of AFD contributed by modulation
//  for (size_t l = 0; l < num_dixels; ++l)
//    for (size_t s = 0; s < fod_filenames.size(); ++s)
//      log_mod_scale_factor(l,s) = Math::log (mod_fod_dixel_integrals(l,s) / fod_dixel_integrals(l,s));
//  Math::Stats::GLM::abs_effect_size (log_mod_scale_factor, design, contrast, abs_effect_size);
//  write_track_stats (output_prefix + "_mod_abs_effect_size.tck", abs_effect_size, track_point_indices);
//  Math::Stats::GLM::std_effect_size (log_mod_scale_factor, design, contrast, std_effect_size);
//  write_track_stats (output_prefix + "_mod_std_effect_size.tck", std_effect_size, track_point_indices);
//  Math::Stats::GLM::stdev (log_mod_scale_factor, design, std_dev);
//  write_track_stats (output_prefix + "_mod_std_dev.tck", std_dev, track_point_indices);

  // Perform permutation testing
  opt = get_options("notest");
  if (!opt.size()) {
    // Modulated FODs
    do_glm_and_output (mod_fod_dixel_integrals, design, contrast, dh, tfce_E, tfce_H, num_perms, dixel_connectivity, dixel_indexer, dixel_directions, track_point_indices, output_prefix + "_mod_fod", tckfile_timestamp);
    // FOD information only
    do_glm_and_output (fod_dixel_integrals, design, contrast, dh, tfce_E, tfce_H, num_perms, dixel_connectivity, dixel_indexer, dixel_directions, track_point_indices, output_prefix + "_fod", tckfile_timestamp);
    // Modulation information only
//    do_glm_and_output (log_mod_scale_factor, design, contrast, dh, tfce_E, tfce_H, num_perms, dixel_connectivity, dixel_indexer, dixel_directions, track_point_indices, output_prefix + "_mod");
  }
}

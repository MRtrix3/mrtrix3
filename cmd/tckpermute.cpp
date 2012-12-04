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
#include "math/hemisphere/directions.h"
#include "timer.h"
#include "math/stats/permutation.h"
#include "math/stats/glm.h"
#include "stats/tfce.h"
#include "dwi/fmls.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/writer.h"

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

  + Argument ("group", "the group average FOD image (ideally at lmax=8).").type_image_in()

  + Argument ("mask", "a 3D mask to define which voxels to include in the analysis").type_image_in()

  + Argument ("tracks", "the tracks used to define orientations of "
                        "interest and spatial neighbourhoods.").type_file()

  + Argument ("output", "the filename prefix for all output.").type_text();


  OPTIONS

  + Option ("only", "")

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

  + Option ("check", "output an image to check the number of lobes per voxel identified in the template")
  + Argument ("image").type_image_out ();
}



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
                    lobe_indexer (FOD_lobe_indexer) ,
                    lobe_directions (FOD_lobe_directions),
                    lobe_TDI (lobe_TDI),
                    lobe_connectivity (lobe_connectivity) {
      angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));
    }

    bool operator () (SetVoxelDir& in)
    {
      // For each voxel tract tangent, assign to a lobe
      vector<int32_t> tract_lobe_indices;
      for (SetVoxelDir::const_iterator i = in.begin(); i != in.end(); ++i) {
        Image::Nav::set_pos (lobe_indexer, *i);
        lobe_indexer[3] = 0;
        int32_t first_index = lobe_indexer.value();

        if (first_index >= 0) {
          lobe_indexer[3] = 1;
          int32_t last_index = first_index + lobe_indexer.value();
          int32_t closest_lobe_index = -1;
          float largest_dp = 0.0;
          Point<float> dir (i->get_dir());
          dir.normalise();
          for (int32_t j = first_index; j < last_index; j++) {
            float dp = Math::abs (dir.dot (lobe_directions[j]));
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
    Image::BufferScratch<int32_t>::voxel_type lobe_indexer;
    vector<Point<float> >& lobe_directions;
    vector<uint16_t>& lobe_TDI;
    vector<map<int32_t, Stats::TFCE::connectivity> >& lobe_connectivity;
    float angular_threshold_dp;
};


class TrackStatItem {
  public:
    vector<Point<float> > tck;
    vector<float> tvalue;
    vector<float> tfce_pos;
    vector<float> tfce_neg;
    vector<float> pvalue_pos;
    vector<float> pvalue_neg;
};


class Track2StatProcessor {

  public:
    Track2StatProcessor (Image::BufferScratch<int32_t>& lobe_indexer,
                         const vector<Point<float> >& lobe_directions,
                         const float angular_threshold,
                         const vector<float>& tvalue,
                         const vector<float>& tfce_pos,
                         const vector<float>& tfce_neg,
                         const vector<float>& pvalue_pos,
                         const vector<float>& pvalue_neg) :
                           lobe_indexer_vox (lobe_indexer),
                           lobe_directions (lobe_directions),
                           tvalue (tvalue),
                           tfce_pos (tfce_pos),
                           tfce_neg (tfce_neg),
                           pvalue_pos (pvalue_pos),
                           pvalue_neg (pvalue_neg),
                           interp (lobe_indexer_vox) {
      angular_threshold_dp = cos (angular_threshold * (M_PI/180.0));
    }

  bool operator () (DWI::Tractography::Mapping::TrackAndIndex& input, TrackStatItem& output)
  {
    output.tck = input.tck;
    output.tvalue.resize (input.tck.size());
    output.tfce_pos.resize (input.tck.size());
    output.tfce_neg.resize (input.tck.size());
    output.pvalue_pos.resize (input.tck.size());
    output.pvalue_neg.resize (input.tck.size());
    Point<float> tangent;
    for (size_t p = 0; p < input.tck.size(); ++p) {
      interp.scanner (input.tck[p]);
      interp[3] = 0;
      int32_t first_index = interp.value();

      if (first_index >= 0) {
        interp[3] = 1;
        int32_t last_index = first_index + interp.value();
        int32_t closest_lobe_index = -1;
        float largest_dp = 0.0;
        if (p == 0)
          tangent = input.tck[p+1] - input.tck[p];
        else if (p == input.tck.size() - 1)
          tangent = input.tck[p] - input.tck[p-1];
        else
          tangent = input.tck[p+1] - input.tck[p-1];
        tangent.normalise();
        for (int32_t j = first_index; j < last_index; j++) {
          float dp = Math::abs (tangent.dot (lobe_directions[j]));
          if (dp > largest_dp) {
            largest_dp = dp;
            closest_lobe_index = j;
          }
        }
        if (largest_dp > angular_threshold_dp) {
          output.tvalue[p] = tvalue[closest_lobe_index];
          output.tfce_pos[p] = tfce_pos[closest_lobe_index];
          output.tfce_neg[p] = tfce_neg[closest_lobe_index];
          output.pvalue_pos[p] = pvalue_pos[closest_lobe_index];
          output.pvalue_neg[p] = pvalue_neg[closest_lobe_index];
        } else {
          output.tvalue[p] = 0.0;
          output.tfce_pos[p] = 0.0;
          output.tfce_neg[p] = 0.0;
          output.pvalue_pos[p] = 0.0;
          output.pvalue_neg[p] = 0.0;
        }
      } else {
        output.tvalue[p] = 0.0;
        output.tfce_pos[p] = 0.0;
        output.tfce_neg[p] = 0.0;
        output.pvalue_pos[p] = 0.0;
        output.pvalue_neg[p] = 0.0;
      }
    }
    return true;
  }


  private:
    Image::BufferScratch<int32_t>::voxel_type lobe_indexer_vox;
    const vector<Point<float> >& lobe_directions;
    float angular_threshold_dp;
    const vector<float>& tvalue;
    const vector<float>& tfce_pos;
    const vector<float>& tfce_neg;
    const vector<float>& pvalue_pos;
    const vector<float>& pvalue_neg;
    Image::Interp::Nearest<Image::BufferScratch<int32_t>::voxel_type> interp;
};



class Track2StatWriter {
  public:
    Track2StatWriter (DWI::Tractography::Writer<value_type>& tck_writer,
                      DWI::Tractography::Writer<value_type>& tvalue_writer,
                      DWI::Tractography::Writer<value_type>& tfce_pos_writer,
                      DWI::Tractography::Writer<value_type>& tfce_neg_writer,
                      DWI::Tractography::Writer<value_type>& pvalue_pos_writer,
                      DWI::Tractography::Writer<value_type>& pvalue_neg_writer) :
                      tck_writer (tck_writer),
                      tvalue_writer (tvalue_writer),
                      tfce_pos_writer (tfce_pos_writer),
                      tfce_neg_writer (tfce_neg_writer),
                      pvalue_pos_writer (pvalue_pos_writer),
                      pvalue_neg_writer (pvalue_neg_writer) { }

    bool operator() (TrackStatItem& output) {
      if (output.tck.empty())
        return true;
      tck_writer.append (output.tck);
      write_scalars (output.tvalue, tvalue_writer);
      write_scalars (output.tfce_pos, tfce_pos_writer);
      write_scalars (output.tfce_neg, tfce_neg_writer);
      write_scalars (output.pvalue_pos, pvalue_pos_writer);
      write_scalars (output.pvalue_neg, pvalue_neg_writer);
      return true;
    }

  private:

    void write_scalars (vector<value_type>& values, DWI::Tractography::Writer<value_type>& writer) {
      vector<Point<float> > scalars;
      for (size_t i = 0; i < values.size(); i += 3) {
        Point<float> point;
        point[0] = values[i];
        if (i + 1 < values.size())
          point[1] = values[i + 1];
        else
          point[1] = NAN;
        if (i + 2 < values.size())
          point[2] = values[i + 2];
        else
          point[2] = NAN;
        scalars.push_back(point);
      }
      writer.append (scalars);
    }

    DWI::Tractography::Writer<value_type>& tck_writer;
    DWI::Tractography::Writer<value_type>& tvalue_writer;
    DWI::Tractography::Writer<value_type>& tfce_pos_writer;
    DWI::Tractography::Writer<value_type>& tfce_neg_writer;
    DWI::Tractography::Writer<value_type>& pvalue_pos_writer;
    DWI::Tractography::Writer<value_type>& pvalue_neg_writer;
};


// Run permutation test and output results
void do_glm_and_output (const Math::Matrix<value_type>& data,
                        const Math::Matrix<value_type>& design,
                        const Math::Matrix<value_type>& contrast,
                        const value_type dh,
                        const value_type E,
                        const value_type H,
                        const int num_perms,
                        const vector<map<int32_t, Stats::TFCE::connectivity> >& lobe_connectivity,
                        const string track_filename,
                        Image::BufferScratch<int32_t>& lobe_indexer,
                        const vector<Point<value_type> >& lobe_directions,
                        const float angular_threshold,
                        const int num_vis_tracks,
                        string output_prefix) {

  int num_lobes = lobe_directions.size();
  Math::Vector<value_type> perm_distribution_pos (num_perms - 1);
  Math::Vector<value_type> perm_distribution_neg (num_perms - 1);
  vector<value_type> tfce_output_pos (num_lobes, 0.0);
  vector<value_type> tfce_output_neg (num_lobes, 0.0);
  vector<value_type> tvalue_output (num_lobes, 0.0);
  vector<value_type> pvalue_output_pos (num_lobes, 0.0);
  vector<value_type> pvalue_output_neg (num_lobes, 0.0);

    Math::Stats::GLMTTest glm (data, design, contrast);
  {
    Stats::TFCE::Connectivity tfce_integrator (lobe_connectivity, dh, E, H);
    Stats::TFCE::run (glm, tfce_integrator, num_perms,
                      perm_distribution_pos, perm_distribution_neg,
                      tfce_output_pos, tfce_output_neg, tvalue_output);
  }

  perm_distribution_pos.save (output_prefix + "_perm_dist_pos.txt");
  perm_distribution_neg.save (output_prefix + "_perm_dist_neg.txt");
  Math::Stats::statistic2pvalue (perm_distribution_pos, tfce_output_pos, pvalue_output_pos);
  Math::Stats::statistic2pvalue (perm_distribution_neg, tfce_output_neg, pvalue_output_neg);

  // Generate output
  DWI::Tractography::Properties tck_properties;
  DWI::Tractography::Reader<value_type> tracks_file;
  tracks_file.open (track_filename, tck_properties);
  DWI::Tractography::Writer<value_type> tck_writer (output_prefix + "_tck.tck", tck_properties);
  DWI::Tractography::Writer<value_type> tvalue_writer (output_prefix + "_tck.tval", tck_properties);
  DWI::Tractography::Writer<value_type> tfce_pos_writer (output_prefix + "_tck.tfcep", tck_properties);
  DWI::Tractography::Writer<value_type> tfce_neg_writer (output_prefix + "_tck.tfcen", tck_properties);
  DWI::Tractography::Writer<value_type> pvalue_pos_writer (output_prefix + "_tck.pvalp", tck_properties);
  DWI::Tractography::Writer<value_type> pvalue_neg_writer (output_prefix + "_tck.pvaln", tck_properties);
  DWI::Tractography::Mapping::TrackLoader loader (tracks_file, num_vis_tracks, "generating output tracks and associated statistics...");
  Track2StatProcessor processor (lobe_indexer, lobe_directions, angular_threshold, tvalue_output, tfce_output_pos, tfce_output_neg, pvalue_output_pos, pvalue_output_neg);
  Track2StatWriter writer (tck_writer, tvalue_writer, tfce_pos_writer, tfce_neg_writer, pvalue_pos_writer, pvalue_neg_writer);
  Thread::run_queue (loader, 1, DWI::Tractography::Mapping::TrackAndIndex(), processor, 0, TrackStatItem(), writer, 1);
  tracks_file.close();
  tck_writer.close();
  tvalue_writer.close();
  tfce_pos_writer.close();
  tfce_neg_writer.close();
  pvalue_pos_writer.close();
  pvalue_neg_writer.close();
}


void load_data_and_compute_integrals (const vector<string>& filename_list,
                                      Image::BufferScratch<bool>& lobe_mask,
                                      Image::BufferScratch<int32_t>& lobe_indexer,
                                      const vector<Point<value_type> >& lobe_directions,
                                      const value_type angular_threshold,
                                      const vector<map<int32_t, value_type> >& lobe_smoothing_weights,
                                      Math::Matrix<value_type>& fod_lobe_integrals) {

  fod_lobe_integrals.resize (lobe_directions.size(), filename_list.size(), 0.0);
  ProgressBar progress ("loading FOD images and computing integrals...", filename_list.size());
  Math::Hemisphere::Directions dirs (1281);
  for (size_t subject = 0; subject < filename_list.size(); subject++) {
    LogLevelLatch log_level (0);
    Image::Buffer<value_type> fod_buffer (filename_list[subject]);
    Image::check_dimensions (fod_buffer, lobe_mask, 0, 3);
    SHQueueWriter<Image::Buffer<value_type>, Image::BufferScratch<bool> > writer2 (fod_buffer, lobe_mask);
    DWI::FOD_FMLS fmls (dirs, Math::SH::LforN (fod_buffer.dim(3)));
    fmls.set_peak_value_threshold (SUBJECT_FOD_THRESHOLD);
    vector<value_type> temp_lobe_integrals (lobe_directions.size(), 0.0);
    SubjectLobeProcessor lobe_processor (lobe_indexer, lobe_directions, temp_lobe_integrals, angular_threshold);
    Thread::run_queue (writer2, 1, DWI::SH_coefs(), fmls, 0, DWI::FOD_lobes(), lobe_processor, 1);

    // Smooth the data based on connectivity
    for (size_t lobe = 0; lobe < lobe_directions.size(); ++lobe) {
      value_type value = 0.0;
      map<int32_t, value_type>::const_iterator it = lobe_smoothing_weights[lobe].begin();
      for (; it != lobe_smoothing_weights[lobe].end(); ++it)
        value += temp_lobe_integrals[it->first] * it->second;
      fod_lobe_integrals (lobe, subject) = value;
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

  value_type std_dev = 5.0 / 2.3548;
  opt = get_options ("smooth");
  if (opt.size())
    std_dev = value_type(opt[0][0]) / 2.3548;

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

  DWI::Tractography::Reader<value_type> track_file;
  DWI::Tractography::Properties properties;
  track_file.open (argument[6], properties);

  // Compute FOD lobes on average FOD image
  vector<Point<value_type> > lobe_directions;
  Math::Hemisphere::Directions dirs (1281);
  Image::Header index_header (argument[4]);
  index_header.dim(3) = 2;
  Image::BufferScratch<int32_t> lobe_indexer (index_header);
  Image::BufferScratch<int32_t>::voxel_type lobe_indexer_vox (lobe_indexer);
  vector<Point<value_type> > lobe_positions;
  Image::LoopInOrder loop4D (lobe_indexer_vox);
  for (loop4D.start (lobe_indexer_vox); loop4D.ok(); loop4D.next (lobe_indexer_vox))
    lobe_indexer_vox.value() = -1;

  {
    Image::Buffer<value_type> av_fod_buffer (argument[4]);
    Image::Buffer<bool> brain_mask_buffer (argument[5]);
    Image::check_dimensions (av_fod_buffer, brain_mask_buffer, 0, 3);
    SHQueueWriter<Image::Buffer<value_type>, Image::Buffer<bool> > writer (av_fod_buffer, brain_mask_buffer);
    DWI::FOD_FMLS fmls (dirs, Math::SH::LforN (av_fod_buffer.dim(3)));
    fmls.set_peak_value_threshold (GROUP_AVERAGE_FOD_THRESHOLD);
    GroupAvLobeProcessor lobe_processor (lobe_indexer, lobe_directions, lobe_positions);
    Thread::run_queue (writer, 1, DWI::SH_coefs(), fmls, 0, DWI::FOD_lobes(), lobe_processor, 1);
  }


  // Compute 3D analysis mask based on lobes in average FOD image
  uint32_t num_lobes = lobe_directions.size();
  CONSOLE ("number of lobes: " + str(num_lobes));
  Image::Header header3D (argument[4]);
  header3D.set_ndim(3);
  Image::BufferScratch<bool> lobe_mask (header3D);
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


  // Check number of lobes per voxel
  opt = get_options ("check");
  if (opt.size()) {
    Image::Buffer<float> fibre_count_buffer (opt[0][0], header3D);
    Image::Buffer<float>::voxel_type fibre_count_buffer_vox (fibre_count_buffer);
    Image::Loop loop(0, 3);
    for (loop.start (lobe_indexer_vox, fibre_count_buffer_vox); loop.ok(); loop.next (lobe_indexer_vox, fibre_count_buffer_vox)) {
      lobe_indexer_vox[3] = 0;
      int32_t lobe_index = lobe_indexer_vox.value();
      if (lobe_index >= 0) {
        lobe_indexer_vox[3] = 1;
        fibre_count_buffer_vox.value() = lobe_indexer_vox.value();
      }
    }
  }

  vector<map<int32_t, Stats::TFCE::connectivity> > lobe_connectivity (num_lobes);
  vector<uint16_t> lobe_TDI (num_lobes, 0.0);
  {
    // Read in tracts, and computer whole-brain lobe-lobe connectivity
    const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
    if (!num_tracks)
      throw Exception ("error with track count in input file");
    if (num_vis_tracks > num_tracks) {
      WARN("the number of visualisation tracts is larger than the total available. Setting num_vis_tracks to " + str(num_tracks));
      num_vis_tracks = num_tracks;
    }

    typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;
    Math::Matrix<value_type> dummy_matrix;
    DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "pre-computing lobe-lobe connectivity...");
    Image::Header header (argument[4]);
    DWI::Tractography::Mapping::TrackMapperBase<SetVoxelDir> mapper (header, dummy_matrix);
    TractProcessor tract_processor (lobe_indexer, lobe_directions, lobe_TDI, lobe_connectivity, 30);
    Thread::run_queue (loader, 1, DWI::Tractography::Mapping::TrackAndIndex(), mapper, 1, SetVoxelDir(), tract_processor, 1);
  }
  track_file.close();


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
        it->second.value = Math::pow (connectivity, tfce_C);
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

  // Load subject data and compute FOD integrals within the mask
  Math::Matrix<value_type> fod_lobe_integrals;
  Math::Matrix<value_type> mod_fod_lobe_integrals;
  load_data_and_compute_integrals (fod_filenames, lobe_mask, lobe_indexer, lobe_directions, angular_threshold, lobe_smoothing_weights, fod_lobe_integrals);
  load_data_and_compute_integrals (mod_fod_filenames, lobe_mask, lobe_indexer, lobe_directions, angular_threshold, lobe_smoothing_weights, mod_fod_lobe_integrals);
  Math::Matrix<value_type> modulation_only (num_lobes, fod_filenames.size());

  // Extract the amount of AFD contributed by modulation
  for (size_t l = 0; l < num_lobes; ++l)
    for (size_t s = 0; s < fod_filenames.size(); ++s)
      modulation_only(l,s) = mod_fod_lobe_integrals(l,s) - fod_lobe_integrals (l,s);

  string output_prefix = argument[7];
  string track_filename = argument[6];

  // FOD information only
  do_glm_and_output (fod_lobe_integrals, design, contrast, dh, tfce_E, tfce_H, num_perms, lobe_connectivity, track_filename, lobe_indexer, lobe_directions, angular_threshold, num_vis_tracks, output_prefix + "_fod");
  // Modulated FODs
  do_glm_and_output (mod_fod_lobe_integrals, design, contrast, dh, tfce_E, tfce_H, num_perms, lobe_connectivity, track_filename, lobe_indexer, lobe_directions, angular_threshold, num_vis_tracks, output_prefix + "_fod_mod");
  // Modulation information only
  do_glm_and_output (modulation_only, design, contrast, dh, tfce_E, tfce_H, num_perms, lobe_connectivity, track_filename, lobe_indexer, lobe_directions, angular_threshold, num_vis_tracks, output_prefix + "_mod");

}

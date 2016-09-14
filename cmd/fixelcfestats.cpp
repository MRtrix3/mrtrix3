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
#include "progressbar.h"
#include "thread_queue.h"
#include "algo/loop.h"
#include "transform.h"
#include "image.h"
#include "fixel_format/helpers.h"
#include "fixel_format/keys.h"
#include "fixel_format/loop.h"
#include "math/stats/glm.h"
#include "math/stats/permutation.h"
#include "math/stats/typedefs.h"
#include "stats/cfe.h"
#include "stats/enhance.h"
#include "stats/permtest.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/writer.h"


using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography::Mapping;
using namespace MR::Math::Stats;
using Stats::CFE::direction_type;
using Stats::CFE::connectivity_value_type;

#define DEFAULT_CFE_DH 0.1
#define DEFAULT_CFE_E 2.0
#define DEFAULT_CFE_H 3.0
#define DEFAULT_CFE_C 0.5
#define DEFAULT_ANGLE_THRESHOLD 45.0
#define DEFAULT_CONNECTIVITY_THRESHOLD 0.01
#define DEFAULT_SMOOTHING_STD 10.0

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "Fixel-based analysis using connectivity-based fixel enhancement and non-parametric permutation testing.";

  REFERENCES
  + "Raffelt, D.; Smith, RE.; Ridgway, GR.; Tournier, JD.; Vaughan, DN.; Rose, S.; Henderson, R.; Connelly, A." // Internal
    "Connectivity-based fixel enhancement: Whole-brain statistical analysis of diffusion MRI measures in the presence of crossing fibres. \n"
    "Neuroimage, 2015, 15(117):40-55\n"

  + "* If using the -nonstationary option: \n"
    "Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. \n"
    "Adjusting the effect of nonstationarity in cluster-based and TFCE inference. \n"
    "NeuroImage, 2011, 54(3), 2006-19\n" ;

  ARGUMENTS
  + Argument ("in_fixel_folder", "the fixel folder containing the data files for each subject (after obtaining fixel correspondence").type_file_in ()

  + Argument ("subjects", "a text file listing the subject identifiers (one per line). This should correspond with the filenames "
                          "in the fixel folder (including the file extension), and be listed in the same order as the rows of the design matrix.").type_image_in ()

  + Argument ("design", "the design matrix. Note that a column of 1's will need to be added for correlations.").type_file_in ()

  + Argument ("contrast", "the contrast vector, specified as a single row of weights").type_file_in ()

  + Argument ("tracks", "the tracks used to determine fixel-fixel connectivity").type_tracks_in ()

  + Argument ("out_fixel_folder", "the output folder where results will be saved. Will be created if it does not exist").type_text();


  OPTIONS

  + Stats::PermTest::Options (true)

  + OptionGroup ("Parameters for the Connectivity-based Fixel Enhancement algorithm")

  + Option ("cfe_dh", "the height increment used in the cfe integration (default: " + str(DEFAULT_CFE_DH, 2) + ")")
  + Argument ("value").type_float (0.001, 1.0)

  + Option ("cfe_e", "cfe extent exponent (default: " + str(DEFAULT_CFE_E, 2) + ")")
  + Argument ("value").type_float (0.0, 100.0)

  + Option ("cfe_h", "cfe height exponent (default: " + str(DEFAULT_CFE_H, 2) + ")")
  + Argument ("value").type_float (0.0, 100.0)

  + Option ("cfe_c", "cfe connectivity exponent (default: " + str(DEFAULT_CFE_C, 2) + ")")
  + Argument ("value").type_float (0.0, 100.0)

  + OptionGroup ("Additional options for fixelcfestats")

  + Option ("negative", "automatically test the negative (opposite) contrast. By computing the opposite contrast simultaneously "
                        "the computation time is reduced.")

  + Option ("smooth", "smooth the fixel value along the fibre tracts using a Gaussian kernel with the supplied FWHM (default: " + str(DEFAULT_SMOOTHING_STD, 2) + "mm)")
  + Argument ("FWHM").type_float (0.0, 200.0)

  + Option ("connectivity", "a threshold to define the required fraction of shared connections to be included in the neighbourhood (default: " + str(DEFAULT_CONNECTIVITY_THRESHOLD, 2) + ")")
  + Argument ("threshold").type_float (0.0, 1.0)

  + Option ("angle", "the max angle threshold for assigning streamline tangents to fixels (Default: " + str(DEFAULT_ANGLE_THRESHOLD, 2) + " degrees)")
  + Argument ("value").type_float (0.0, 90.0);

}



template <class VectorType>
void write_fixel_output (const std::string& filename,
                         const VectorType& data,
                         const Header& header)
{
  assert (data.size() == header.size(2));
  auto output = Image<float>::create (filename, header);
  for (uint32_t i = 0; i < data.size(); ++i) {
    output.index(2) = i;
    output.value() = data[i];
  }
}



void run() {

  auto opt = get_options ("negative");
  bool compute_negative_contrast = opt.size() ? true : false;
  const value_type cfe_dh = get_option_value ("cfe_dh", DEFAULT_CFE_DH);
  const value_type cfe_h = get_option_value ("cfe_h", DEFAULT_CFE_H);
  const value_type cfe_e = get_option_value ("cfe_e", DEFAULT_CFE_E);
  const value_type cfe_c = get_option_value ("cfe_c", DEFAULT_CFE_C);
  const int num_perms = get_option_value ("nperms", DEFAULT_NUMBER_PERMUTATIONS);
  const value_type smooth_std_dev = get_option_value ("smooth", DEFAULT_SMOOTHING_STD) / 2.3548;
  const value_type connectivity_threshold = get_option_value ("connectivity", DEFAULT_CONNECTIVITY_THRESHOLD);
  const bool do_nonstationary_adjustment = get_options ("nonstationary").size();
  const int nperms_nonstationary = get_option_value ("nperms_nonstationary", DEFAULT_NUMBER_PERMUTATIONS_NONSTATIONARITY);
  const value_type angular_threshold = get_option_value ("angle", DEFAULT_ANGLE_THRESHOLD);

  const std::string input_fixel_folder = argument[0];
  Header index_header = FixelFormat::find_index_header (input_fixel_folder);
  auto index_image = index_header.get_image<uint32_t>();
  uint32_t num_fixels = std::stoul (index_image.keyval().at(FixelFormat::n_fixels_key));

  std::vector<Eigen::Vector3> positions (num_fixels);
  std::vector<direction_type> directions (num_fixels);

  {
    auto directions_data = FixelFormat::find_directions_header (input_fixel_folder, index_image).get_image<float>().with_direct_io();

    Transform image_transform (index_image);
    for (auto i = Loop (index_image)(index_image); i; ++i) {
      const Eigen::Vector3 vox ((default_type)index_image.index(0), (default_type)index_image.index(1), (default_type)index_image.index(2));
      index_image.index(3) = 1;
      uint32_t offset = index_image.value();
      size_t fixel_index = 0;
      for (auto f = FixelFormat::FixelLoop (index_image) (directions_data); f; ++f, ++fixel_index) {
        directions[offset + fixel_index] = directions_data.row(1).cast<default_type>();
        positions[offset + fixel_index] = image_transform.voxel2scanner * vox;
      }
    }
  }
  CONSOLE ("number of fixels: " + str(num_fixels));


  // Read identifiers and check files exist
  std::vector<std::string> identifiers;
  Header header;
  {
    std::ifstream ifs (argument[1].c_str());
    std::string temp;
    while (getline (ifs, temp)) {
      std::string filename (Path::join (input_fixel_folder, temp));
      size_t p = filename.find_last_not_of(" \t");
      if (std::string::npos != p)
        filename.erase(p+1);
      if (!MR::Path::exists (filename))
        throw Exception ("input fixel image not found: " + filename);
      header = Header::open (filename);
      FixelFormat::fixels_match (index_header, header);
      identifiers.push_back (filename);
    }
  }

  // Load design matrix:
  const matrix_type design = load_matrix (argument[2]);
  if (design.rows() != (ssize_t)identifiers.size())
    throw Exception ("number of input files does not match number of rows in design matrix");


  // Load contrast matrix:
  const matrix_type contrast = load_matrix (argument[3]);

  if (contrast.cols() != design.cols())
    throw Exception ("the number of contrasts does not equal the number of columns in the design matrix");
  if (contrast.rows() > 1)
    throw Exception ("only a single contrast vector (defined as a row) is currently supported");

  // Copy index and directions over to output folder
  const std::string output_fixel_folder = argument[5];
  FixelFormat::copy_index_file (input_fixel_folder, output_fixel_folder);
  FixelFormat::copy_directions_file (input_fixel_folder, output_fixel_folder);


  // Compute fixel-fixel connectivity
  std::vector<std::map<uint32_t, Stats::CFE::connectivity> > connectivity_matrix (num_fixels);
  std::vector<uint16_t> fixel_TDI (num_fixels, 0.0);
  const std::string track_filename = argument[4];
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<> track_file (track_filename, properties);
  // Read in tracts, and compute whole-brain fixel-fixel connectivity
  const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
  if (!num_tracks)
    throw Exception ("no tracks found in input file");
  if (num_tracks < 1000000)
    WARN ("more than 1 million tracks should be used to ensure robust fixel-fixel connectivity");
  {
    typedef DWI::Tractography::Mapping::SetVoxelDir SetVoxelDir;
    DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "pre-computing fixel-fixel connectivity");
    DWI::Tractography::Mapping::TrackMapperBase mapper (index_image);
    mapper.set_upsample_ratio (DWI::Tractography::Mapping::determine_upsample_ratio (index_header, properties, 0.333f));
    mapper.set_use_precise_mapping (true);
    Stats::CFE::TrackProcessor tract_processor (index_image, directions, fixel_TDI, connectivity_matrix, angular_threshold);
    Thread::run_queue (
        loader,
        Thread::batch (DWI::Tractography::Streamline<float>()),
        mapper,
        Thread::batch (SetVoxelDir()),
        tract_processor);
  }
  track_file.close();

  // Normalise connectivity matrix and threshold, pre-compute fixel-fixel weights for smoothing.
  std::vector<std::map<uint32_t, connectivity_value_type> > smoothing_weights (num_fixels);
  bool do_smoothing = false;

  const float gaussian_const2 = 2.0 * smooth_std_dev * smooth_std_dev;
  float gaussian_const1 = 1.0;
  if (smooth_std_dev > 0.0) {
    do_smoothing = true;
    gaussian_const1 = 1.0 / (smooth_std_dev *  std::sqrt (2.0 * Math::pi));
  }

  {
    ProgressBar progress ("normalising and thresholding fixel-fixel connectivity matrix", num_fixels);
    for (uint32_t fixel = 0; fixel < num_fixels; ++fixel) {

      auto it = connectivity_matrix[fixel].begin();
      while (it != connectivity_matrix[fixel].end()) {
        const value_type connectivity = it->second.value / value_type (fixel_TDI[fixel]);
        if (connectivity < connectivity_threshold)  {
          connectivity_matrix[fixel].erase (it++);
        } else {
          if (do_smoothing) {
            const value_type distance = std::sqrt (Math::pow2 (positions[fixel][0] - positions[it->first][0]) +
                                                   Math::pow2 (positions[fixel][1] - positions[it->first][1]) +
                                                   Math::pow2 (positions[fixel][2] - positions[it->first][2]));
            const float smoothing_weight = connectivity * gaussian_const1 * std::exp (-std::pow (distance, 2) / gaussian_const2);
            smoothing_weights[fixel].insert (std::pair<uint32_t, float> (it->first, smoothing_weight));
          }
          // Here we pre-exponentiate each connectivity value by C
          it->second.value = std::pow (connectivity, cfe_c);
          ++it;
        }
      }
      // Make sure the fixel is fully connected to itself
      connectivity_matrix[fixel].insert (std::pair<uint32_t, Stats::CFE::connectivity> (fixel, Stats::CFE::connectivity (1.0)));
      smoothing_weights[fixel].insert (std::pair<uint32_t, float> (fixel, gaussian_const1));

      // Normalise smoothing weights
      value_type sum = 0.0;
      for (auto it = smoothing_weights[fixel].begin(); it != smoothing_weights[fixel].end(); ++it)
        sum += it->second;
      value_type norm_factor = 1.0 / sum;
      for (auto it = smoothing_weights[fixel].begin(); it != smoothing_weights[fixel].end(); ++it) {
        it->second *= norm_factor;
        if (it->second < 0.005)
          smoothing_weights[fixel].erase (it++);
      }
      progress++;
    }
  }

  // Load input data
  matrix_type data (num_fixels, identifiers.size());
  {
    ProgressBar progress ("loading input images", identifiers.size());
    for (size_t subject = 0; subject < identifiers.size(); subject++) {
      LogLevelLatch log_level (0);

      auto subject_data = Image<float>::open (identifiers[subject]).with_direct_io();
      std::vector<value_type> subject_data_vector (num_fixels, 0.0);
      for (auto i = Loop (index_image)(index_image); i; ++i) {
        index_image.index(3) = 1;
        uint32_t offset = index_image.value();
        uint32_t fixel_index = 0;
        for (auto f = FixelFormat::FixelLoop (index_image) (subject_data); f; ++f, ++fixel_index)
          subject_data_vector[offset + fixel_index] = subject_data.value();
      }

      // Smooth the data
      for (size_t fixel = 0; fixel < num_fixels; ++fixel) {
        value_type value = 0.0;
        std::map<uint32_t, float>::const_iterator it = smoothing_weights[fixel].begin();
        for (; it != smoothing_weights[fixel].end(); ++it)
          value += subject_data_vector[it->first] * it->second;
        data (fixel, subject) = value;
      }
      progress++;
    }
  }

  if (!data.allFinite())
    throw Exception ("input data contains non-finite value(s)");


  Header output_header (header);
  output_header.keyval()["num permutations"] = str(num_perms);
  output_header.keyval()["dh"] = str(cfe_dh);
  output_header.keyval()["cfe_e"] = str(cfe_e);
  output_header.keyval()["cfe_h"] = str(cfe_h);
  output_header.keyval()["cfe_c"] = str(cfe_c);
  output_header.keyval()["angular threshold"] = str(angular_threshold);
  output_header.keyval()["connectivity threshold"] = str(connectivity_threshold);
  output_header.keyval()["smoothing FWHM"] = str(smooth_std_dev * 2.3548);

  {
    ProgressBar progress ("outputting beta coefficients, effect size and standard deviation");
    auto temp = Math::Stats::GLM::solve_betas (data, design);
    for (ssize_t i = 0; i < contrast.cols(); ++i) {
      write_fixel_output (Path::join (output_fixel_folder, "beta" + str(i) + ".mif"), temp.row(i), output_header);
      ++progress;
    }
    temp = Math::Stats::GLM::abs_effect_size (data, design, contrast); ++progress;
    write_fixel_output (Path::join (output_fixel_folder, "abs_effect.mif"), temp.row(0), output_header); ++progress;
    temp = Math::Stats::GLM::std_effect_size (data, design, contrast); ++progress;
    write_fixel_output (Path::join (output_fixel_folder, "std_effect.mif"), temp.row(0), output_header); ++progress;
    temp = Math::Stats::GLM::stdev (data, design); ++progress;
    write_fixel_output (Path::join (output_fixel_folder, "std_dev.mif"), temp.row(0), output_header);
  }

  Math::Stats::GLMTTest glm_ttest (data, design, contrast);
  std::shared_ptr<Stats::EnhancerBase> cfe_integrator;
  cfe_integrator.reset (new Stats::CFE::Enhancer (connectivity_matrix, cfe_dh, cfe_e, cfe_h));
  vector_type empirical_cfe_statistic;


  // If performing non-stationarity adjustment we need to pre-compute the empirical CFE statistic
  if (do_nonstationary_adjustment) {
    Stats::PermTest::precompute_empirical_stat (glm_ttest, cfe_integrator, nperms_nonstationary, empirical_cfe_statistic);
    output_header.keyval()["nonstationary adjustment"] = str(true);
    write_fixel_output (Path::join (output_fixel_folder, "cfe_empirical.mif"), empirical_cfe_statistic, output_header);
  } else {
    output_header.keyval()["nonstationary adjustment"] = str(false);
  }

  // Precompute default statistic and CFE statistic
  vector_type cfe_output (num_fixels);
  std::shared_ptr<vector_type> cfe_output_neg;
  vector_type tvalue_output (num_fixels);
  if (compute_negative_contrast)
    cfe_output_neg.reset (new vector_type (num_fixels));

  Stats::PermTest::precompute_default_permutation (glm_ttest, cfe_integrator, empirical_cfe_statistic, cfe_output, cfe_output_neg, tvalue_output);

  write_fixel_output (Path::join (output_fixel_folder, "cfe.mif"), cfe_output, output_header);
  write_fixel_output (Path::join (output_fixel_folder, "tvalue.mif"), tvalue_output, output_header);
  if (compute_negative_contrast)
    write_fixel_output (Path::join (output_fixel_folder, "cfe_neg.mif"), *cfe_output_neg, output_header);

  // Perform permutation testing
  opt = get_options ("notest");
  if (!opt.size()) {
    vector_type perm_distribution (num_perms);
    std::shared_ptr<vector_type> perm_distribution_neg;
    vector_type uncorrected_pvalues (num_fixels);
    std::shared_ptr<vector_type> uncorrected_pvalues_neg;

    if (compute_negative_contrast) {
      perm_distribution_neg.reset (new vector_type (num_perms));
      uncorrected_pvalues_neg.reset (new vector_type (num_fixels));
    }

    Stats::PermTest::run_permutations (glm_ttest, cfe_integrator, num_perms, empirical_cfe_statistic,
                                       cfe_output, cfe_output_neg,
                                       perm_distribution, perm_distribution_neg,
                                       uncorrected_pvalues, uncorrected_pvalues_neg);

    ProgressBar progress ("outputting final results");
    save_matrix (perm_distribution, Path::join (output_fixel_folder, "perm_dist.txt")); ++progress;

    vector_type pvalue_output (num_fixels);
    Math::Stats::Permutation::statistic2pvalue (perm_distribution, cfe_output, pvalue_output); ++progress;
    write_fixel_output (Path::join (output_fixel_folder, "fwe_pvalue.mif"), pvalue_output, output_header); ++progress;
    write_fixel_output (Path::join (output_fixel_folder, "uncorrected_pvalue.mif"), uncorrected_pvalues, output_header); ++progress;

    if (compute_negative_contrast) {
      save_matrix (*perm_distribution_neg, Path::join (output_fixel_folder, "perm_dist_neg.txt")); ++progress;
      vector_type pvalue_output_neg (num_fixels);
      Math::Stats::Permutation::statistic2pvalue (*perm_distribution_neg, *cfe_output_neg, pvalue_output_neg); ++progress;
      write_fixel_output (Path::join (output_fixel_folder, "fwe_pvalue_neg.mif"), pvalue_output_neg, output_header); ++progress;
      write_fixel_output (Path::join (output_fixel_folder, "uncorrected_pvalue_neg.mif"), *uncorrected_pvalues_neg, output_header);
    }
  }
}

/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "progressbar.h"
#include "thread_queue.h"
#include "algo/loop.h"
#include "transform.h"
#include "image.h"
#include "fixel/helpers.h"
#include "fixel/keys.h"
#include "fixel/loop.h"
#include "math/stats/glm.h"
#include "math/stats/permutation.h"
#include "math/stats/typedefs.h"
#include "stats/cfe.h"
#include "stats/enhance.h"
#include "stats/permtest.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/writer.h"


using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography::Mapping;
using namespace MR::Math::Stats;
using Stats::CFE::direction_type;
using Stats::CFE::connectivity_value_type;
using Stats::CFE::index_type;

#define DEFAULT_CFE_DH 0.1
#define DEFAULT_CFE_E 2.0
#define DEFAULT_CFE_H 3.0
#define DEFAULT_CFE_C 0.5
#define DEFAULT_ANGLE_THRESHOLD 45.0
#define DEFAULT_CONNECTIVITY_THRESHOLD 0.01
#define DEFAULT_SMOOTHING_STD 10.0

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Fixel-based analysis using connectivity-based fixel enhancement and non-parametric permutation testing";

  DESCRIPTION
  + "Note that if the -mask option is used, the output fixel directory will still contain the same set of fixels as that "
    "present in the input fixel template, in order to retain fixel correspondence. However a consequence of this is that "
    "all fixels in the template will be initialy visible when the output fixel directory is loaded in mrview. Those fixels "
    "outside the processing mask will immediately disappear from view as soon as any data-file-based fixel colouring or "
    "thresholding is applied.";

  REFERENCES
  + "Raffelt, D.; Smith, RE.; Ridgway, GR.; Tournier, JD.; Vaughan, DN.; Rose, S.; Henderson, R.; Connelly, A." // Internal
    "Connectivity-based fixel enhancement: Whole-brain statistical analysis of diffusion MRI measures in the presence of crossing fibres. \n"
    "Neuroimage, 2015, 15(117):40-55\n"

  + "* If using the -nonstationary option: \n"
    "Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. \n"
    "Adjusting the effect of nonstationarity in cluster-based and TFCE inference. \n"
    "NeuroImage, 2011, 54(3), 2006-19\n" ;

  ARGUMENTS
  + Argument ("in_fixel_directory", "the fixel directory containing the data files for each subject (after obtaining fixel correspondence").type_file_in ()

  + Argument ("subjects", "a text file listing the subject identifiers (one per line). This should correspond with the filenames "
                          "in the fixel directory (including the file extension), and be listed in the same order as the rows of the design matrix.").type_image_in ()

  + Argument ("design", "the design matrix. Note that a column of 1's will need to be added for correlations.").type_file_in ()

  + Argument ("contrast", "the contrast vector, specified as a single row of weights").type_file_in ()

  + Argument ("tracks", "the tracks used to determine fixel-fixel connectivity").type_tracks_in ()

  + Argument ("out_fixel_directory", "the output directory where results will be saved. Will be created if it does not exist").type_text();


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
  + Argument ("value").type_float (0.0, 90.0)

  + Option ("mask", "provide a fixel data file containing a mask of those fixels to be used during processing")
  + Argument ("file").type_image_in();

}



template <class VectorType>
void write_fixel_output (const std::string& filename,
                         const VectorType& data,
                         const vector<int32_t>& fixel2row,
                         const Header& header)
{
  assert (size_t(header.size(0)) == fixel2row.size());
  auto output = Image<float>::create (filename, header);
  for (size_t i = 0; i != fixel2row.size(); ++i) {
    output.index(0) = i;
    output.value() = (fixel2row[i] >= 0) ? data[fixel2row[i]] : NaN;
  }
}



void run() {

  auto opt = get_options ("negative");
  bool compute_negative_contrast = opt.size() ? true : false;
  const value_type cfe_dh = get_option_value ("cfe_dh", DEFAULT_CFE_DH);
  const value_type cfe_h = get_option_value ("cfe_h", DEFAULT_CFE_H);
  const value_type cfe_e = get_option_value ("cfe_e", DEFAULT_CFE_E);
  const value_type cfe_c = get_option_value ("cfe_c", DEFAULT_CFE_C);
  int num_perms = get_option_value ("nperms", DEFAULT_NUMBER_PERMUTATIONS);
  const value_type smooth_std_dev = get_option_value ("smooth", DEFAULT_SMOOTHING_STD) / 2.3548;
  const value_type connectivity_threshold = get_option_value ("connectivity", DEFAULT_CONNECTIVITY_THRESHOLD);
  const bool do_nonstationary_adjustment = get_options ("nonstationary").size();
  int nperms_nonstationary = get_option_value ("nperms_nonstationary", DEFAULT_NUMBER_PERMUTATIONS_NONSTATIONARITY);
  const value_type angular_threshold = get_option_value ("angle", DEFAULT_ANGLE_THRESHOLD);


  const std::string input_fixel_directory = argument[0];
  Header index_header = Fixel::find_index_header (input_fixel_directory);
  auto index_image = index_header.get_image<index_type>();

  const index_type num_fixels = Fixel::get_number_of_fixels (index_header);
  CONSOLE ("number of fixels: " + str(num_fixels));

  Image<bool> mask;
  index_type mask_fixels;
  // Lookup table that maps from input fixel index to row number
  // Note that if a fixel is masked out, it will have a value of -1
  //   in this array; hence require a signed integer type
  vector<int32_t> fixel2row (num_fixels);
  opt = get_options ("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
    Fixel::check_data_file (mask);
    if (!Fixel::fixels_match (index_header, mask))
      throw Exception ("Mask image provided using -mask option does not match fixel template");
    mask_fixels = 0;
    for (mask.index(0) = 0; mask.index(0) != num_fixels; ++mask.index(0))
      fixel2row[mask.index(0)] = mask.value() ? mask_fixels++ : -1;
    CONSOLE ("Fixel mask contains " + str(mask_fixels) + " fixels");
  } else {
    Header data_header;
    data_header.ndim() = 3;
    data_header.size(0) = num_fixels;
    data_header.size(1) = 1;
    data_header.size(2) = 1;
    data_header.spacing(0) = data_header.spacing(1) = data_header.spacing(2) = NaN;
    data_header.stride(0) = 1; data_header.stride(1) = 2; data_header.stride(2) = 3;
    mask = Image<bool>::scratch (data_header, "scratch fixel mask");
    for (index_type f = 0; f != num_fixels; ++f) {
      mask.index(0) = f;
      mask.value() = true;
    }
    mask_fixels = num_fixels;
    for (index_type f = 0; f != num_fixels; ++f)
      fixel2row[f] = f;
  }

  vector<Eigen::Vector3> positions (num_fixels);
  vector<direction_type> directions (num_fixels);

  const std::string output_fixel_directory = argument[5];
  Fixel::copy_index_and_directions_file (input_fixel_directory, output_fixel_directory);

  {
    auto directions_data = Fixel::find_directions_header (input_fixel_directory).get_image<default_type>().with_direct_io ({+2,+1});
    // Load template fixel directions
    Transform image_transform (index_image);
    for (auto i = Loop ("loading template fixel directions and positions", index_image, 0, 3)(index_image); i; ++i) {
      Eigen::Vector3 vox ((default_type)index_image.index(0), (default_type)index_image.index(1), (default_type)index_image.index(2));
      vox = image_transform.voxel2scanner * vox;
      index_image.index(3) = 1;
      const index_type offset = index_image.value();
      size_t fixel_index = 0;
      for (auto f = Fixel::Loop (index_image) (directions_data); f; ++f, ++fixel_index) {
        directions[offset + fixel_index] = directions_data.row(1);
        positions[offset + fixel_index] = vox;
      }
    }
  }
  // Read identifiers and check files exist
  vector<std::string> identifiers;
  Header header;
  {
    ProgressBar progress ("validating input files");
    std::ifstream ifs (argument[1].c_str());
    std::string temp;
    while (getline (ifs, temp)) {
      std::string filename = strip (Path::join (input_fixel_directory, temp));
      if (filename.empty())
        continue;
      if (!MR::Path::exists (filename))
        throw Exception ("input fixel image not found: " + filename);
      header = Header::open (filename);
      Fixel::fixels_match (index_header, header);
      identifiers.push_back (filename);
      progress++;
    }
  }

  // Load design matrix:
  const matrix_type design = load_matrix (argument[2]);
  if (design.rows() != (ssize_t)identifiers.size())
    throw Exception ("number of input files does not match number of rows in design matrix");

  // Load permutations file if supplied
  opt = get_options("permutations");
  vector<vector<size_t> > permutations;
  if (opt.size()) {
    permutations = Math::Stats::Permutation::load_permutations_file (opt[0][0]);
    num_perms = permutations.size();
    if (permutations[0].size() != (size_t)design.rows())
      throw Exception ("number of rows in the permutations file (" + str(opt[0][0]) + ") does not match number of rows in design matrix");
  }

  // Load non-stationary correction permutations file if supplied
  opt = get_options("permutations_nonstationary");
  vector<vector<size_t> > permutations_nonstationary;
  if (opt.size()) {
    permutations_nonstationary = Math::Stats::Permutation::load_permutations_file (opt[0][0]);
    nperms_nonstationary = permutations_nonstationary.size();
    if (permutations_nonstationary[0].size() != (size_t)design.rows())
      throw Exception ("number of rows in the nonstationary permutations file (" + str(opt[0][0]) + ") does not match number of rows in design matrix");
  }

  // Load contrast matrix:
  const matrix_type contrast = load_matrix (argument[3]);

  if (contrast.cols() != design.cols())
    throw Exception ("the number of columns per contrast does not equal the number of columns in the design matrix");
  if (contrast.rows() > 1)
    throw Exception ("only a single contrast vector (defined as a row) is currently supported");

  // Compute fixel-fixel connectivity
  Stats::CFE::init_connectivity_matrix_type connectivity_matrix (num_fixels);
  vector<uint16_t> fixel_TDI (num_fixels, 0);
  const std::string track_filename = argument[4];
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<float> track_file (track_filename, properties);
  // Read in tracts, and compute whole-brain fixel-fixel connectivity
  const size_t num_tracks = properties["count"].empty() ? 0 : to<size_t> (properties["count"]);
  if (!num_tracks)
    throw Exception ("no tracks found in input file");
  if (num_tracks < 1000000)
    WARN ("more than 1 million tracks should be used to ensure robust fixel-fixel connectivity");
  {
    DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "pre-computing fixel-fixel connectivity");
    DWI::Tractography::Mapping::TrackMapperBase mapper (index_image);
    mapper.set_upsample_ratio (DWI::Tractography::Mapping::determine_upsample_ratio (index_header, properties, 0.333f));
    mapper.set_use_precise_mapping (true);
    Stats::CFE::TrackProcessor tract_processor (index_image, directions, mask, fixel_TDI, connectivity_matrix, angular_threshold);
    Thread::run_queue (
        loader,
        Thread::batch (DWI::Tractography::Streamline<float>()),
        mapper,
        Thread::batch (DWI::Tractography::Mapping::SetVoxelDir()),
        tract_processor);
  }
  track_file.close();

  // Normalise connectivity matrix, threshold, and put in a more efficient format
  Stats::CFE::norm_connectivity_matrix_type norm_connectivity_matrix (mask_fixels);
  // Also pre-compute fixel-fixel weights for smoothing.
  Stats::CFE::norm_connectivity_matrix_type smoothing_weights (mask_fixels);
  bool do_smoothing = false;

  const float gaussian_const2 = 2.0 * smooth_std_dev * smooth_std_dev;
  float gaussian_const1 = 1.0;
  if (smooth_std_dev > 0.0) {
    do_smoothing = true;
    gaussian_const1 = 1.0 / (smooth_std_dev *  std::sqrt (2.0 * Math::pi));
  }

  {
    ProgressBar progress ("normalising and thresholding fixel-fixel connectivity matrix", num_fixels);
    for (index_type fixel = 0; fixel < num_fixels; ++fixel) {
      mask.index(0) = fixel;
      const int32_t row = fixel2row[fixel];

      if (mask.value()) {

        // Here, the connectivity matrix needs to be modified to reflect the
        //   fact that fixel indices in the template fixel image may not
        //   correspond to rows in the statistical analysis
        connectivity_value_type sum_weights = 0.0;

        for (auto& it : connectivity_matrix[fixel]) {
#ifndef NDEBUG
          // Even if this fixel is within the mask, it should still not
          //   connect to any fixel that is outside the mask
          mask.index(0) = it.first;
          assert (mask.value());
#endif
          const connectivity_value_type connectivity = it.second.value / connectivity_value_type (fixel_TDI[fixel]);
          if (connectivity >= connectivity_threshold) {
            if (do_smoothing) {
              const value_type distance = std::sqrt (Math::pow2 (positions[fixel][0] - positions[it.first][0]) +
                                                     Math::pow2 (positions[fixel][1] - positions[it.first][1]) +
                                                     Math::pow2 (positions[fixel][2] - positions[it.first][2]));
              const connectivity_value_type smoothing_weight = connectivity * gaussian_const1 * std::exp (-Math::pow2 (distance) / gaussian_const2);
              if (smoothing_weight >= connectivity_threshold) {
                smoothing_weights[row].push_back (Stats::CFE::NormMatrixElement (fixel2row[it.first], smoothing_weight));
                sum_weights += smoothing_weight;
              }
            }
            // Here we pre-exponentiate each connectivity value by C
            norm_connectivity_matrix[row].push_back (Stats::CFE::NormMatrixElement (fixel2row[it.first], std::pow (connectivity, cfe_c)));
          }
        }

        // Make sure the fixel is fully connected to itself
        norm_connectivity_matrix[row].push_back (Stats::CFE::NormMatrixElement (uint32_t(row), connectivity_value_type(1.0)));
        smoothing_weights[row].push_back (Stats::CFE::NormMatrixElement (uint32_t(row), connectivity_value_type(gaussian_const1)));
        sum_weights += connectivity_value_type(gaussian_const1);

        // Normalise smoothing weights
        const connectivity_value_type norm_factor = connectivity_value_type(1.0) / sum_weights;
        for (auto i : smoothing_weights[row])
          i.normalise (norm_factor);

        // Force deallocation of memory used for this fixel in the original matrix
        std::map<uint32_t, Stats::CFE::connectivity>().swap (connectivity_matrix[fixel]);

      } else {

        // If fixel is not in the mask, tract_processor should never assign
        //   any connections to it
        assert (connectivity_matrix[fixel].empty());

      }

      progress++;
    }
  }

  // The connectivity matrix is now in vector rather than matrix form;
  //   throw out the structure holding the original data
  // (Note however that all entries in the original structure should
  //   have been deleted during the prior loop)
  Stats::CFE::init_connectivity_matrix_type().swap (connectivity_matrix);


  Header output_header (header);
  output_header.keyval()["num permutations"] = str(num_perms);
  output_header.keyval()["dh"] = str(cfe_dh);
  output_header.keyval()["cfe_e"] = str(cfe_e);
  output_header.keyval()["cfe_h"] = str(cfe_h);
  output_header.keyval()["cfe_c"] = str(cfe_c);
  output_header.keyval()["angular threshold"] = str(angular_threshold);
  output_header.keyval()["connectivity threshold"] = str(connectivity_threshold);
  output_header.keyval()["smoothing FWHM"] = str(smooth_std_dev * 2.3548);


  // Load input data
  matrix_type data (mask_fixels, identifiers.size());
  data.setZero();
  {
    ProgressBar progress (std::string ("loading input images") + (do_smoothing ? " and smoothing" : ""), identifiers.size());
    for (size_t subject = 0; subject < identifiers.size(); subject++) {
      LogLevelLatch log_level (0);

      auto subject_data = Image<value_type>::open (identifiers[subject]).with_direct_io();
      vector<value_type> subject_data_vector (mask_fixels, 0.0);
      for (auto i = Loop (index_image, 0, 3)(index_image); i; ++i) {
        index_image.index(3) = 1;
        uint32_t offset = index_image.value();
        uint32_t fixel_index = 0;
        for (auto f = Fixel::Loop (index_image) (subject_data); f; ++f, ++fixel_index) {
          if (!std::isfinite(static_cast<value_type>(subject_data.value())))
            throw Exception ("subject data file " + identifiers[subject] + " contains non-finite value: " + str(subject_data.value()));
          // Note that immediately on import, data are re-arranged according to fixel mask
          const int32_t row = fixel2row[offset+fixel_index];
          if (row >= 0)
            subject_data_vector[row] = subject_data.value();
        }
      }

      // Smooth the data
      if (do_smoothing) {
        for (size_t fixel = 0; fixel < mask_fixels; ++fixel) {
          value_type value = 0.0;
          for (auto i : smoothing_weights[fixel])
            value += subject_data_vector[i.index()] * i.value();
          data (fixel, subject) = value;
        }
      } else {
        data.col (subject) = Eigen::Map<Eigen::Matrix<value_type, Eigen::Dynamic, 1> > (subject_data_vector.data(), mask_fixels);
      }
      progress++;
    }
  }

  // Free the memory occupied by the data smoothing filter; no longer required
  Stats::CFE::norm_connectivity_matrix_type().swap (smoothing_weights);

  if (!data.allFinite())
    throw Exception ("input data contains non-finite value(s)");
  {
    ProgressBar progress ("outputting beta coefficients, effect size and standard deviation");
    auto temp = Math::Stats::GLM::solve_betas (data, design);

    for (ssize_t i = 0; i < contrast.cols(); ++i) {
      write_fixel_output (Path::join (output_fixel_directory, "beta" + str(i) + ".mif"), temp.row(i), fixel2row, output_header);
      ++progress;
    }
    temp = Math::Stats::GLM::abs_effect_size (data, design, contrast); ++progress;
    write_fixel_output (Path::join (output_fixel_directory, "abs_effect.mif"), temp.row(0), fixel2row, output_header); ++progress;
    temp = Math::Stats::GLM::std_effect_size (data, design, contrast); ++progress;
    write_fixel_output (Path::join (output_fixel_directory, "std_effect.mif"), temp.row(0), fixel2row, output_header); ++progress;
    temp = Math::Stats::GLM::stdev (data, design); ++progress;
    write_fixel_output (Path::join (output_fixel_directory, "std_dev.mif"), temp.row(0), fixel2row, output_header);
  }

  Math::Stats::GLMTTest glm_ttest (data, design, contrast);
  std::shared_ptr<Stats::EnhancerBase> cfe_integrator;
  cfe_integrator.reset (new Stats::CFE::Enhancer (norm_connectivity_matrix, cfe_dh, cfe_e, cfe_h));
  vector_type empirical_cfe_statistic;

  // If performing non-stationarity adjustment we need to pre-compute the empirical CFE statistic
  if (do_nonstationary_adjustment) {

    if (permutations_nonstationary.size()) {
      Stats::PermTest::PermutationStack permutations (permutations_nonstationary, "precomputing empirical statistic for non-stationarity adjustment");
      Stats::PermTest::precompute_empirical_stat (glm_ttest, cfe_integrator, permutations, empirical_cfe_statistic);
    } else {
      Stats::PermTest::PermutationStack permutations (nperms_nonstationary, design.rows(), "precomputing empirical statistic for non-stationarity adjustment", false);
      Stats::PermTest::precompute_empirical_stat (glm_ttest, cfe_integrator, permutations, empirical_cfe_statistic);
    }
    output_header.keyval()["nonstationary adjustment"] = str(true);
    write_fixel_output (Path::join (output_fixel_directory, "cfe_empirical.mif"), empirical_cfe_statistic, fixel2row, output_header);
  } else {
    output_header.keyval()["nonstationary adjustment"] = str(false);
  }

  // Precompute default statistic and CFE statistic
  vector_type cfe_output (mask_fixels);
  std::shared_ptr<vector_type> cfe_output_neg;
  vector_type tvalue_output (mask_fixels);
  if (compute_negative_contrast)
    cfe_output_neg.reset (new vector_type (mask_fixels));

  Stats::PermTest::precompute_default_permutation (glm_ttest, cfe_integrator, empirical_cfe_statistic, cfe_output, cfe_output_neg, tvalue_output);

  write_fixel_output (Path::join (output_fixel_directory, "cfe.mif"), cfe_output, fixel2row, output_header);
  write_fixel_output (Path::join (output_fixel_directory, "tvalue.mif"), tvalue_output, fixel2row, output_header);
  if (compute_negative_contrast)
    write_fixel_output (Path::join (output_fixel_directory, "cfe_neg.mif"), *cfe_output_neg, fixel2row, output_header);

  // Perform permutation testing
  if (!get_options ("notest").size()) {
    vector_type perm_distribution (num_perms);
    std::shared_ptr<vector_type> perm_distribution_neg;
    vector_type uncorrected_pvalues (mask_fixels);
    std::shared_ptr<vector_type> uncorrected_pvalues_neg;

    if (compute_negative_contrast) {
      perm_distribution_neg.reset (new vector_type (num_perms));
      uncorrected_pvalues_neg.reset (new vector_type (mask_fixels));
    }

    // FIXME fixelcfestats is hanging here for some reason...
    //   Even when no mask is supplied

    if (permutations.size()) {
      Stats::PermTest::run_permutations (permutations, glm_ttest, cfe_integrator, empirical_cfe_statistic,
                                         cfe_output, cfe_output_neg,
                                         perm_distribution, perm_distribution_neg,
                                         uncorrected_pvalues, uncorrected_pvalues_neg);
    } else {
      Stats::PermTest::run_permutations (num_perms, glm_ttest, cfe_integrator, empirical_cfe_statistic,
                                         cfe_output, cfe_output_neg,
                                         perm_distribution, perm_distribution_neg,
                                         uncorrected_pvalues, uncorrected_pvalues_neg);
    }

    ProgressBar progress ("outputting final results");
    save_matrix (perm_distribution, Path::join (output_fixel_directory, "perm_dist.txt")); ++progress;

    vector_type pvalue_output (mask_fixels);
    Math::Stats::Permutation::statistic2pvalue (perm_distribution, cfe_output, pvalue_output); ++progress;
    write_fixel_output (Path::join (output_fixel_directory, "fwe_pvalue.mif"), pvalue_output, fixel2row, output_header); ++progress;
    write_fixel_output (Path::join (output_fixel_directory, "uncorrected_pvalue.mif"), uncorrected_pvalues, fixel2row, output_header); ++progress;

    if (compute_negative_contrast) {
      save_matrix (*perm_distribution_neg, Path::join (output_fixel_directory, "perm_dist_neg.txt")); ++progress;
      vector_type pvalue_output_neg (mask_fixels);
      Math::Stats::Permutation::statistic2pvalue (*perm_distribution_neg, *cfe_output_neg, pvalue_output_neg); ++progress;
      write_fixel_output (Path::join (output_fixel_directory, "fwe_pvalue_neg.mif"), pvalue_output_neg, fixel2row, output_header); ++progress;
      write_fixel_output (Path::join (output_fixel_directory, "uncorrected_pvalue_neg.mif"), *uncorrected_pvalues_neg, fixel2row, output_header);
    }
  }
}

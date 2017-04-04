/* Copyright (c) 2008-2017 the MRtrix3 contributors
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

  SYNOPSIS = "Fixel-based analysis using connectivity-based fixel enhancement and non-parametric permutation testing";

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

  + Option ("column", "add a column to the design matrix corresponding to subject fixel-wise values "
                      "(the contrast vector length must include columns for these additions)").allow_multiple()
  + Argument ("path").type_file_in()

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
  auto output = Image<float>::create (filename, header);
  for (uint32_t i = 0; i < data.size(); ++i) {
    output.index(0) = i;
    output.value() = data[i];
  }
}



// Define data importer class that willl obtain fixel data for a
//   specific subject based on the string path to the image file for
//   that subject
class SubjectFixelImport : public SubjectDataImportBase
{ NOMEMALIGN
  public:
    SubjectFixelImport (const std::string& path) :
        SubjectDataImportBase (path),
        H (Header::open (path)),
        data (H.get_image<float>())
    {
      for (size_t axis = 1; axis < data.ndim(); ++axis) {
        if (data.size(axis) > 1)
          throw Exception ("Image file \"" + path + "\" does not contain fixel data (wrong dimensions)");
      }
    }

    void operator() (matrix_type::ColXpr column) const override
    {
      assert (column.rows() == data.size(0));
      Image<float> temp (data); // For thread-safety
      column = temp.row(0);
    }

    default_type operator[] (const size_t index) const override
    {
      assert (index < size_t(data.size(0)));
      Image<float> temp (data); // For thread-safety
      temp.index(0) = index;
      return default_type(temp.value());
    }

    const Header& header() const { return H; }

  private:
    Header H;
    const Image<float> data;

};



void run()
{

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
  auto index_image = index_header.get_image<uint32_t>();

  const uint32_t num_fixels = Fixel::get_number_of_fixels (index_header);
  CONSOLE ("number of fixels: " + str(num_fixels));

  vector<Eigen::Vector3> positions (num_fixels);
  vector<direction_type> directions (num_fixels);

  const std::string output_fixel_directory = argument[5];
  Fixel::copy_index_and_directions_file (input_fixel_directory, output_fixel_directory);

  {
    auto directions_data = Fixel::find_directions_header (input_fixel_directory).get_image<default_type>().with_direct_io ({+2,+1});
    // Load template fixel directions
    Transform image_transform (index_image);
    for (auto i = Loop ("loading template fixel directions and positions", index_image, 0, 3)(index_image); i; ++i) {
      const Eigen::Vector3 vox ((default_type)index_image.index(0), (default_type)index_image.index(1), (default_type)index_image.index(2));
      index_image.index(3) = 1;
      uint32_t offset = index_image.value();
      size_t fixel_index = 0;
      for (auto f = Fixel::Loop (index_image) (directions_data); f; ++f, ++fixel_index) {
        directions[offset + fixel_index] = directions_data.row(1);
        positions[offset + fixel_index] = image_transform.voxel2scanner * vox;
      }
    }
  }

  // Read identifiers and check files exist
  CohortDataImport importer;
  importer.initialise<SubjectFixelImport> (argument[1]);
  for (size_t i = 0; i != importer.size(); ++i) {
    if (!Fixel::fixels_match (index_header, dynamic_cast<SubjectFixelImport*>(importer[i].get())->header()))
      throw Exception ("Fixel data file \"" + importer[i]->name() + "\" does not match template fixel image");
  }
  if (importer.size()) {
    CONSOLE ("number of element-wise design matrix columns: " + str(importer.size()));
  }

  // Load design matrix:
  const matrix_type design = load_matrix (argument[2]);
  CONSOLE ("design matrix dimensions: " + str(design.rows()) + " x " + str(design.cols()));
  if (design.rows() != (ssize_t)importer.size())
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
    if (do_nonstationary_adjustment) {
      permutations_nonstationary = Math::Stats::Permutation::load_permutations_file (opt[0][0]);
      nperms_nonstationary = permutations_nonstationary.size();
      if (permutations_nonstationary[0].size() != (size_t)design.rows())
        throw Exception ("number of rows in the nonstationary permutations file (" + str(opt[0][0]) + ") does not match number of rows in design matrix");
    } else {
      WARN ("-permutations_nonstationary option ignored: nonstationarity correction is not being performed (-nonstationary option)");
    }
  }

  // Load contrast matrix
  const matrix_type contrast = load_matrix (argument[3]);
  CONSOLE ("Contrast matrix dimensions: " + str(contrast.rows()) + " x " + str(contrast.cols()));

  // Before validating the contrast matrix, we first need to see if there are any
  //   additional design matrix columns coming from fixel-wise subject data
  vector<CohortDataImport> extra_columns;
  opt = get_options ("column");
  for (size_t i = 0; i != opt.size(); ++i) {
    extra_columns.push_back (CohortDataImport());
    extra_columns[i].initialise<SubjectFixelImport> (opt[i][0]);
  }

  if (contrast.cols() != design.cols() + ssize_t(extra_columns.size()))
    throw Exception ("the number of columns per contrast (" + str(contrast.cols()) + ")"
                     + " does not equal the number of columns in the design matrix (" + str(design.cols()) + ")"
                     + (extra_columns.size() ? " (taking into account the " + str(extra_columns.size()) + " uses of -column)" : ""));
  if (contrast.rows() > 1)
    throw Exception ("only a single contrast vector (defined as a row) is currently supported");

  // Compute fixel-fixel connectivity
  vector<std::map<uint32_t, Stats::CFE::connectivity> > connectivity_matrix (num_fixels);
  vector<uint16_t> fixel_TDI (num_fixels, 0.0);
  const std::string track_filename = argument[4];
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<float> track_file (track_filename, properties);
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
  vector<std::map<uint32_t, connectivity_value_type> > smoothing_weights (num_fixels);
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
        const connectivity_value_type connectivity = it->second.value / connectivity_value_type (fixel_TDI[fixel]);
        if (connectivity < connectivity_threshold)  {
          connectivity_matrix[fixel].erase (it++);
        } else {
          if (do_smoothing) {
            const value_type distance = std::sqrt (Math::pow2 (positions[fixel][0] - positions[it->first][0]) +
                                                   Math::pow2 (positions[fixel][1] - positions[it->first][1]) +
                                                   Math::pow2 (positions[fixel][2] - positions[it->first][2]));
            const connectivity_value_type smoothing_weight = connectivity * gaussian_const1 * std::exp (-std::pow (distance, 2) / gaussian_const2);
            if (smoothing_weight > 0.01)
              smoothing_weights[fixel].insert (std::pair<uint32_t, connectivity_value_type> (it->first, smoothing_weight));
          }
          // Here we pre-exponentiate each connectivity value by C
          it->second.value = std::pow (connectivity, cfe_c);
          ++it;
        }
      }
      // Make sure the fixel is fully connected to itself
      connectivity_matrix[fixel].insert (std::pair<uint32_t, Stats::CFE::connectivity> (fixel, Stats::CFE::connectivity (1.0)));
      smoothing_weights[fixel].insert (std::pair<uint32_t, connectivity_value_type> (fixel, gaussian_const1));

      // Normalise smoothing weights
      value_type sum = 0.0;
      for (auto smooth_it = smoothing_weights[fixel].begin(); smooth_it != smoothing_weights[fixel].end(); ++smooth_it) {
        sum += smooth_it->second;
      }
      value_type norm_factor = 1.0 / sum;
      for (auto smooth_it = smoothing_weights[fixel].begin(); smooth_it != smoothing_weights[fixel].end(); ++smooth_it) {
        smooth_it->second *= norm_factor;
      }
      progress++;
    }
  }

  Header output_header (dynamic_cast<SubjectFixelImport*>(importer[0].get())->header());
  output_header.keyval()["num permutations"] = str(num_perms);
  output_header.keyval()["dh"] = str(cfe_dh);
  output_header.keyval()["cfe_e"] = str(cfe_e);
  output_header.keyval()["cfe_h"] = str(cfe_h);
  output_header.keyval()["cfe_c"] = str(cfe_c);
  output_header.keyval()["angular threshold"] = str(angular_threshold);
  output_header.keyval()["connectivity threshold"] = str(connectivity_threshold);
  output_header.keyval()["smoothing FWHM"] = str(smooth_std_dev * 2.3548);


  // Load input data
  matrix_type data (num_fixels, importer.size());
  data.setZero();

  {
    ProgressBar progress ("loading input images", importer.size());
    for (size_t subject = 0; subject < importer.size(); subject++) {
      (*importer[subject]) (data.col (subject));
      // Smooth the data
      vector_type smoothed_data (vector_type::Zero (num_fixels));
      for (size_t fixel = 0; fixel < num_fixels; ++fixel) {
        value_type value = 0.0;
        std::map<uint32_t, connectivity_value_type>::const_iterator it = smoothing_weights[fixel].begin();
        for (; it != smoothing_weights[fixel].end(); ++it)
          value += data (it->first, subject) * it->second;
        smoothed_data (fixel) = value;
      }
      if (!smoothed_data.allFinite())
        throw Exception ("Input fixel data \"" + importer[subject]->name() + "\" contains at least one non-finite value");
      data.col (subject) = smoothed_data;
    }
    progress++;
  }

  if (extra_columns.size()) {
    WARN ("Beta coefficients, effect size and standard deviation outputs not yet implemented for fixel-wise extra columns");
    // TODO
  } else {
    ProgressBar progress ("outputting beta coefficients, effect size and standard deviation");
    auto temp = Math::Stats::GLM::solve_betas (data, design);
    for (ssize_t i = 0; i < contrast.cols(); ++i) {
      write_fixel_output (Path::join (output_fixel_directory, "beta" + str(i) + ".mif"), temp.row(i), output_header);
      ++progress;
    }
    temp = Math::Stats::GLM::abs_effect_size (data, design, contrast); ++progress;
    write_fixel_output (Path::join (output_fixel_directory, "abs_effect.mif"), temp.row(0), output_header); ++progress;
    temp = Math::Stats::GLM::std_effect_size (data, design, contrast); ++progress;
    write_fixel_output (Path::join (output_fixel_directory, "std_effect.mif"), temp.row(0), output_header); ++progress;
    temp = Math::Stats::GLM::stdev (data, design); ++progress;
    write_fixel_output (Path::join (output_fixel_directory, "std_dev.mif"), temp.row(0), output_header);
  }

  // Construct the class for performing the initial statistical tests
  std::shared_ptr<GLMTestBase> glm_test;
  if (extra_columns.size()) {
    glm_test.reset (new GLMTTestVariable (extra_columns, data, design, contrast));
  } else {
    glm_test.reset (new GLMTTestFixed (data, design, contrast));
  }

  // Construct the class for performing fixel-based statistical enhancement
  std::shared_ptr<Stats::EnhancerBase> cfe_integrator (new Stats::CFE::Enhancer (connectivity_matrix, cfe_dh, cfe_e, cfe_h));

  // If performing non-stationarity adjustment we need to pre-compute the empirical CFE statistic
  vector_type empirical_cfe_statistic;
  if (do_nonstationary_adjustment) {

    if (permutations_nonstationary.size()) {
      Stats::PermTest::PermutationStack permutations (permutations_nonstationary, "precomputing empirical statistic for non-stationarity adjustment");
      Stats::PermTest::precompute_empirical_stat (glm_test, cfe_integrator, permutations, empirical_cfe_statistic);
    } else {
      Stats::PermTest::PermutationStack permutations (nperms_nonstationary, design.rows(), "precomputing empirical statistic for non-stationarity adjustment", false);
      Stats::PermTest::precompute_empirical_stat (glm_test, cfe_integrator, permutations, empirical_cfe_statistic);
    }
    output_header.keyval()["nonstationary adjustment"] = str(true);
    write_fixel_output (Path::join (output_fixel_directory, "cfe_empirical.mif"), empirical_cfe_statistic, output_header);
  } else {
    output_header.keyval()["nonstationary adjustment"] = str(false);
  }

  // Precompute default statistic and CFE statistic
  vector_type cfe_output (num_fixels);
  std::shared_ptr<vector_type> cfe_output_neg;
  vector_type tvalue_output (num_fixels);
  if (compute_negative_contrast)
    cfe_output_neg.reset (new vector_type (num_fixels));

  Stats::PermTest::precompute_default_permutation (glm_test, cfe_integrator, empirical_cfe_statistic, cfe_output, cfe_output_neg, tvalue_output);

  write_fixel_output (Path::join (output_fixel_directory, "cfe.mif"), cfe_output, output_header);
  write_fixel_output (Path::join (output_fixel_directory, "tvalue.mif"), tvalue_output, output_header);
  if (compute_negative_contrast)
    write_fixel_output (Path::join (output_fixel_directory, "cfe_neg.mif"), *cfe_output_neg, output_header);

  // Perform permutation testing
  if (!get_options ("notest").size()) {
    vector_type perm_distribution (num_perms);
    std::shared_ptr<vector_type> perm_distribution_neg;
    vector_type uncorrected_pvalues (num_fixels);
    std::shared_ptr<vector_type> uncorrected_pvalues_neg;

    if (compute_negative_contrast) {
      perm_distribution_neg.reset (new vector_type (num_perms));
      uncorrected_pvalues_neg.reset (new vector_type (num_fixels));
    }

    if (permutations.size()) {
      Stats::PermTest::run_permutations (permutations, glm_test, cfe_integrator, empirical_cfe_statistic,
                                         cfe_output, cfe_output_neg,
                                         perm_distribution, perm_distribution_neg,
                                         uncorrected_pvalues, uncorrected_pvalues_neg);
    } else {
      Stats::PermTest::run_permutations (num_perms, glm_test, cfe_integrator, empirical_cfe_statistic,
                                         cfe_output, cfe_output_neg,
                                         perm_distribution, perm_distribution_neg,
                                         uncorrected_pvalues, uncorrected_pvalues_neg);
    }

    ProgressBar progress ("outputting final results");
    save_matrix (perm_distribution, Path::join (output_fixel_directory, "perm_dist.txt")); ++progress;

    vector_type pvalue_output (num_fixels);
    Math::Stats::Permutation::statistic2pvalue (perm_distribution, cfe_output, pvalue_output); ++progress;
    write_fixel_output (Path::join (output_fixel_directory, "fwe_pvalue.mif"), pvalue_output, output_header); ++progress;
    write_fixel_output (Path::join (output_fixel_directory, "uncorrected_pvalue.mif"), uncorrected_pvalues, output_header); ++progress;

    if (compute_negative_contrast) {
      save_matrix (*perm_distribution_neg, Path::join (output_fixel_directory, "perm_dist_neg.txt")); ++progress;
      vector_type pvalue_output_neg (num_fixels);
      Math::Stats::Permutation::statistic2pvalue (*perm_distribution_neg, *cfe_output_neg, pvalue_output_neg); ++progress;
      write_fixel_output (Path::join (output_fixel_directory, "fwe_pvalue_neg.mif"), pvalue_output_neg, output_header); ++progress;
      write_fixel_output (Path::join (output_fixel_directory, "uncorrected_pvalue_neg.mif"), *uncorrected_pvalues_neg, output_header);
    }
  }
}

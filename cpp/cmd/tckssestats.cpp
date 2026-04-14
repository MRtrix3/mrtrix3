/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "command.h"
#include "image.h"
#include "progressbar.h"
#include "thread_queue.h"
#include "transform.h"
#include "algo/loop.h"
#include "fixel/helpers.h"
#include "fixel/index_remapper.h"
#include "fixel/loop.h"
#include "file/matrix.h"
#include "track/matrix.h"
#include "math/stats/fwe.h"
#include "math/stats/glm.h"
#include "math/stats/import.h"
#include "math/stats/shuffle.h"
#include "math/stats/typedefs.h"
#include "stats/sse.h"
#include "stats/enhance.h"
#include "stats/permtest.h"


using namespace MR;
using namespace App;

using Fixel::index_type;
using Math::Stats::matrix_type;  // Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic>;
using Math::Stats::value_type;
using Math::Stats::vector_type;
using Stats::PermTest::count_matrix_type;

constexpr double DEFAULT_SSE_DH = 0.1;
constexpr double DEFAULT_SSE_E = 2.0;
constexpr double DEFAULT_SSE_H = 3.0;
constexpr double DEFAULT_SSE_M = 0.25;
constexpr double DEFAULT_EMPIRICAL_SKEW = 1.0;  // TODO Update from experience

void usage ()
{
  AUTHOR = "Simone Zanoni (simone.zanoni@sydney.edu.au)";

  SYNOPSIS = "Streamline-based analysis using Similarity-informed Streamline Enhancement (SSE) and non-parametric permutation testing";

  DESCRIPTION

  + "For Similarity-informed Streamline Enhancement,"
    " use -nonstationarity_intrinsic to enable the intrinsic non-stationarity correction "
    "and/or -nonstationarity for empirical non-stationarity correction."

  + MR::Stats::PermTest::mask_posthoc_description

  + Math::Stats::GLM::column_ones_description;

  ARGUMENTS
  + Argument ("in_streamline_directory", "the streamline directory containing the data for each subject").type_directory_in ()

  + Argument ("subjects", "a text file listing the subject identifiers (one per line). "
              "This should correspond with the filenames in the image directory, and be listed in the same order as the rows of the design matrix.").type_file_in ()

  + Argument ("design", "the design matrix").type_file_in ()

  + Argument ("similarity", "the streamline similarity matrix").type_directory_in ()

  + Argument ("out_streamline_directory", "the output directory where results will be saved. Will be created if it does not exist").type_directory_out ();


  OPTIONS

  + Option ("mask", "provide a text file containing a mask of those streamlines to be used during processing")
    + Argument ("file").type_file_in()

  + Option ("posthoc", "provide a text file containing a mask of those streamlines to contribute to statistical inference")
    + Argument ("file").type_file_in()

  +Option ("tck_weights_in", "txt file containing the streamline-wise weights").required()
    + Argument ("path").type_file_in()

  + Math::Stats::shuffle_options (true, DEFAULT_EMPIRICAL_SKEW)

  + OptionGroup ("Parameters for the Similarity-based Streamline Enhancement algorithm")

  + Option ("sse_dh", "the height increment used in the sse integration (default: " + str(DEFAULT_SSE_DH, 2) + ")")
  + Argument ("value").type_float (0.001, 1.0)

  + Option ("sse_e", "sse extent exponent (default: " + str(DEFAULT_SSE_E, 2) + ")")
  + Argument ("value").type_float (0.0, 100.0)

  + Option ("sse_h", "sse height exponent (default: " + str(DEFAULT_SSE_H, 2) + ")")
  + Argument ("value").type_float (0.0, 100.0)

  + Option ("sse_m", "sse similarity exponent (default: " + str(DEFAULT_SSE_M, 2) + ")")
  + Argument ("value").type_float (0.0, 100.0)

  + Option ("nonstationarity_intrinsic", "use the intrinsic non-stationarity correction (normalised version of SSE)")

  + Math::Stats::GLM::glm_options ("streamline");

  REFERENCES
  + "Zanoni, S.; Lv, J.; Smith, R. E. & Calamante, F. "
    "Streamline-Based Analysis: "
    "A novel framework for tractogram-driven streamline-wise statistical analysis. "
    "Proceedings of the International Society for Magnetic Resonance in Medicine, 2025, 4781"

  + "* If using the -nonstationary option: \n"
    "Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. "
    "Adjusting the effect of nonstationarity in cluster-based and TFCE inference. "
    "NeuroImage, 2011, 54(3), 2006-19";

}


using ind_type = int;


template <class VectorType>
void write_streamline_output (std::string_view filename, const VectorType& data) {
  Eigen::VectorXf temp (Eigen::VectorXf::Zero (data.size()));
  for (ssize_t j = 0; j < data.size(); j++) {
    temp[j] = data[j];
  }
  File::Matrix::save_vector(temp, filename);
}


class SubjectTxtImport : public Math::Stats::SubjectDataImportBase {
public:
  SubjectTxtImport(std::string_view path) : 
    Math::Stats::SubjectDataImportBase(path) {
    data = File::Matrix::load_vector<float> (path);

    // TODO Error handling
  }

  // write data into a matrix
  void operator() (Math::Stats::measurements_matrix_type::RowXpr row) const override {
    for (size_t i = 0; i < static_cast<size_t>(data.size()); ++i) {
      row(i) = data[i];
    }
  }

  // access data
  Math::Stats::measurements_value_type operator[](const Math::Stats::index_type index) const override {
    assert(index < data.size());
    return data[index];
  }

  Math::Stats::index_type size() const override {
    return data.size();  // Number of rows in the .txt file
  }

private:
  Eigen::Matrix<float, Eigen::Dynamic, 1> data;
};





void run()
{
  // Import relevant values
  const value_type sse_dh = get_option_value ("sse_dh", DEFAULT_SSE_DH);
  const value_type sse_h = get_option_value ("sse_h", DEFAULT_SSE_H);
  const value_type sse_e = get_option_value ("sse_e", DEFAULT_SSE_E);
  const value_type sse_m = get_option_value ("sse_m", DEFAULT_SSE_M);

  const bool normalise = get_options ("nonstationarity_intrinsic").size();
 
  auto opt = get_options ("tck_weights_in");
  Eigen::Matrix<float, Eigen::Dynamic, 1> weights = File::Matrix::load_vector<float> (opt[0][0]);

  const bool do_nonstationarity_adjustment = get_options ("nonstationarity").size();
  const default_type empirical_skew = get_option_value ("skew_nonstationarity", DEFAULT_EMPIRICAL_SKEW);

  // Directory containing each txt file of the subject cohort
  const std::string input_streamline_directory = argument[0];


  // TODO: txt files don't need to be in the same folder if absolute path is provided
  Math::Stats::CohortDataImport importer;
  importer.initialise<SubjectTxtImport> (argument[1], input_streamline_directory);

  // Check that all the files have the same number of streamlines
  std::shared_ptr<SubjectTxtImport> first_subject = std::dynamic_pointer_cast<SubjectTxtImport>(importer[0]);
  size_t num_streamline = first_subject -> size();

  for (size_t i = 1; i != importer.size(); ++i) {
    std::shared_ptr<SubjectTxtImport> curr_subject = std::dynamic_pointer_cast<SubjectTxtImport>(importer[i]);
    if (num_streamline != (curr_subject -> size()))
      throw Exception ("Streamline data files \"" + importer[i]->name() + "\" have different number of streamlines");
  }

  CONSOLE ("Number of inputs: " + str(importer.size()));
  CONSOLE("Number of streamlines: " + str(num_streamline));

  Math::Stats::element_mask_type mask_processing (num_streamline);
  mask_processing.setConstant (true);
  opt = get_options ("mask");
  if (opt.size()) {
    auto data = File::Matrix::load_vector<int> (opt[0][0]);
    if (data.size() != static_cast<ssize_t>(num_streamline))
      throw Exception ("Processing mask file \"" + std::string(opt[0][0]) + "\" does not match number of streamlines");
    for (size_t i = 0; i != num_streamline; ++i)
      mask_processing[i] = (data[i] != 0);
  }

  Math::Stats::element_mask_type mask_inference (num_streamline);
  opt = get_options ("posthoc");
  if (opt.size()) {
    auto data = File::Matrix::load_vector<int> (opt[0][0]);
    if (data.size() != static_cast<ssize_t>(num_streamline))
      throw Exception ("Post-hoc mask file \"" + std::string(opt[0][0]) + "\" does not match number of streamlines");
    for (size_t i = 0; i != num_streamline; ++i)
      mask_inference[i] = (data[i] != 0);
  } else {
    mask_inference = mask_processing;
  }

  const matrix_type design = File::Matrix::load_matrix (argument[2]);
  if (design.rows() != static_cast<ssize_t>(importer.size()))
    throw Exception ("Number of input files does not match number of rows in design matrix");


  // DEALING WITH EXTRA COLUMN
  // See if there is any additional design matrix columns coming from subject data
  bool nans_in_data = false;

  std::vector<Math::Stats::CohortDataImport> extra_columns;
  bool nans_in_columns = false;

  opt = get_options ("column");
  for (size_t i = 0; i != opt.size(); ++i) {
    extra_columns.emplace_back (Math::Stats::CohortDataImport());
    extra_columns[i].initialise<SubjectTxtImport> (opt[i][0]);
    // Check for non-finite values
    if (!nans_in_columns) {
      Math::Stats::measurements_matrix_type column_data (importer.size(), num_streamline);
      for (size_t j = 0; j != importer.size(); ++j)
        (*extra_columns[i][j]) (column_data.row (j));
      for (size_t j = 0; j != num_streamline; ++j) {
        if (mask_processing[j] && !column_data.col(j).allFinite()) {
          nans_in_columns = true;
          break;
        }
      }
    }
  }

  const ssize_t num_factors = design.cols() + extra_columns.size();
  CONSOLE ("Number of factors: " + str(num_factors));
  if (extra_columns.size()) {
    CONSOLE ("Number of element-wise design matrix columns: " + str(extra_columns.size()));
    if (nans_in_columns)
      CONSOLE ("Non-finite values detected in streamline-wise design matrix columns; individual rows will be removed from streamline-wise design matrices accordingly");
  }

  // DESIGN
  Math::Stats::GLM::check_design (design, extra_columns.size());

  // VARIANCE
  auto variance_groups = Math::Stats::GLM::load_variance_groups (design.rows());
  const size_t num_vgs = variance_groups.size() ? variance_groups.maxCoeff()+1 : 1;
  if (num_vgs > 1)
    CONSOLE ("Number of variance groups: " + str(num_vgs));

  // Load hypotheses
  const std::vector<Math::Stats::GLM::Hypothesis> hypotheses = Math::Stats::GLM::load_hypotheses (num_factors);
  const size_t num_hypotheses = hypotheses.size();
  CONSOLE ("Number of hypotheses: " + str(num_hypotheses));

  // SIMILARITY
  Track::Matrix::Reader sim_matrix (argument[3]);
  CONSOLE("Matrix size: " + str(sim_matrix.size()));

  const std::string output_streamline_directory = argument[4];
    if (Path::exists (output_streamline_directory)) {
        if (!Path::is_dir (output_streamline_directory)) {
            if (App::overwrite_files) {
              File::remove (output_streamline_directory);
            } else {
              throw Exception ("Cannot create stats folder \"" + output_streamline_directory + "\": Already exists as file");
            }
        }
    } else {
        File::mkdir (output_streamline_directory);
    }



  // TO DO
  // Inform the used about the presence of streamlines with no similar streamlines





  Math::Stats::measurements_matrix_type data = Math::Stats::measurements_matrix_type::Zero (importer.size(), num_streamline);
  {
    ProgressBar progress (std::string ("Loading streamline data into matrix data format"), importer.size());
    for (size_t subject = 0; subject != importer.size(); subject++) {
      (*importer[subject]) (data.row (subject));
      progress++;
    }
  }

  for (size_t i = 0; i != num_streamline; ++i) {
    if (mask_processing[i]) {
      if (!data.col(i).allFinite())
        nans_in_data = true;
    } else {
      data.col(i).fill (std::numeric_limits<Math::Stats::value_type>::quiet_NaN());
    }
  }

  if (nans_in_data) {
    CONSOLE ("Non-finite values present in data; rows will be removed from streamline-wise design matrices accordingly");
    if (!extra_columns.size()) {
      CONSOLE ("(Note that this will result in slower execution than if such values were not present)");
    }
  }


  // Only add contrast matrix row number to image outputs if there's more than one hypothesis
  auto postfix = [&] (const size_t i) -> std::string { return (num_hypotheses > 1) ? ("_" + hypotheses[i].name()) : ""; };

  {
    matrix_type empirical_sse_statistic (num_hypotheses, num_streamline);
    matrix_type betas (num_factors, num_streamline);
    matrix_type abs_effect_size (num_streamline, num_hypotheses);
    matrix_type std_effect_size (num_streamline, num_hypotheses);
    matrix_type stdev (num_vgs, num_streamline);
    vector_type cond (num_streamline);

    Math::Stats::GLM::all_stats (data, design, extra_columns, hypotheses, variance_groups,
                                 cond, betas, abs_effect_size, std_effect_size, stdev);

    ProgressBar progress ("Outputting beta coefficients, effect size and standard deviation", num_factors + (2 * num_hypotheses) + num_vgs + (nans_in_data || extra_columns.size() ? 1 : 0));

    for (ssize_t i = 0; i != num_factors; ++i) {
      write_streamline_output(Path::join (output_streamline_directory, "beta" + str(i) + ".txt"), betas.row(i));
      ++progress;
    }

    for (size_t i = 0; i != num_hypotheses; ++i) {
      if (!hypotheses[i].is_F()) {
        write_streamline_output (Path::join (output_streamline_directory, "abs_effect" + postfix(i) + ".txt"), abs_effect_size.col(i));
        ++progress;
        if (num_vgs == 1)
          write_streamline_output (Path::join (output_streamline_directory, "std_effect" + postfix(i) + ".txt"), std_effect_size.col(i));
      } else {
        ++progress;
      }
      ++progress;
    }

    if (nans_in_data || extra_columns.size()) {
      write_streamline_output (Path::join (output_streamline_directory, "cond.txt"), cond);
      ++progress;
    }

    if (num_vgs == 1) {
      write_streamline_output (Path::join (output_streamline_directory, "std_dev.txt"), stdev.row (0));
    } else {
      for (size_t i = 0; i != num_vgs; ++i) {
        write_streamline_output (Path::join (output_streamline_directory, "std_dev" + str(i) + ".txt"), stdev.row (i));
        ++progress;
      }
    }
  }


  // Construct the class for performing the initial statistical tests
  std::unique_ptr<Math::Stats::GLM::TestBase> glm_test;
  if (extra_columns.size() || nans_in_data) {
    if (variance_groups.size())
      glm_test.reset (new Math::Stats::GLM::TestVariableHeteroscedastic (data, design, hypotheses, variance_groups, extra_columns, nans_in_data, nans_in_columns));
    else
      glm_test.reset (new Math::Stats::GLM::TestVariableHomoscedastic (data, design, hypotheses, extra_columns, nans_in_data, nans_in_columns));
  } else {
    if (variance_groups.size())
      glm_test.reset (new Math::Stats::GLM::TestFixedHeteroscedastic (data, design, hypotheses, variance_groups));
    else
      glm_test.reset (new Math::Stats::GLM::TestFixedHomoscedastic (data, design, hypotheses));
  }

  matrix_type statistics (num_hypotheses, num_streamline);
  {
    matrix_type zstatistics (num_hypotheses, num_streamline);
    (*glm_test) (Math::Stats::shuffle_matrix_type::Identity (glm_test->num_inputs(), glm_test->num_inputs()), statistics, zstatistics);
  }

  // Construct the class for performing streamline-based statistical enhancement
  std::shared_ptr<Stats::EnhancerBase> sse_integrator (new Stats::SSE (sim_matrix, weights, sse_dh, sse_e, sse_h, sse_m, normalise));

  // If performing non-stationarity adjustment we need to pre-compute the empirical SSE statistic
  matrix_type empirical_sse_statistic;
  if (do_nonstationarity_adjustment) {
    Stats::PermTest::precompute_empirical_stat (glm_test, sse_integrator, empirical_skew, empirical_sse_statistic);
    for (size_t i = 0; i != num_hypotheses; ++i)
      write_streamline_output (Path::join (output_streamline_directory, "sse_empirical" + postfix(i) + ".txt"), empirical_sse_statistic.col(i));
  }
  CONSOLE ("Non-stationarity - empirical: " + str(do_nonstationarity_adjustment));
  CONSOLE ("Non-stationarity - intrinsic: " + str(normalise));






  // Precompute default statistic and SSE statistic
  matrix_type default_statistic, default_zstat, default_enhanced;


  Stats::PermTest::precompute_default_permutation (glm_test, sse_integrator, empirical_sse_statistic, default_statistic, default_zstat, default_enhanced);
  for (size_t i = 0; i != num_hypotheses; ++i) {
    write_streamline_output (Path::join (output_streamline_directory, (hypotheses[i].is_F() ? std::string("F") : std::string("t")) + "value" + postfix(i) + ".txt"), default_statistic.col(i));
    write_streamline_output (Path::join (output_streamline_directory, "Zstat" + postfix(i) + ".txt"), default_zstat.col(i));
    write_streamline_output (Path::join (output_streamline_directory, "sse" + postfix(i) + ".txt"), default_enhanced.col(i));
  }



  // Perform permutation testing
  if (!get_options ("notest").size()) {

    // for multiple hypothesis
    const bool fwe_strong = get_options("strong").size();
    if (fwe_strong && num_hypotheses == 1) {
      WARN("Option -strong has no effect when testing a single hypothesis only");
    }

    matrix_type null_distribution, uncorrected_pvalues;
    count_matrix_type null_contributions;
    Stats::PermTest::run_permutations (glm_test, sse_integrator, empirical_sse_statistic, default_enhanced, fwe_strong,
                                       mask_inference, null_distribution, null_contributions, uncorrected_pvalues);

    ProgressBar progress ("Outputting final results", (fwe_strong ? 1 : num_hypotheses) + 1 + 3*num_hypotheses);

    // This is to output the empirical null distribution as the result of the permutation testing
    if (fwe_strong) {
      File::Matrix::save_vector (null_distribution.col(0), Path::join (output_streamline_directory, "null_dist.txt"));
      ++progress;
    } else {
      for (size_t i = 0; i != num_hypotheses; ++i) {
        File::Matrix::save_vector (null_distribution.col(i), Path::join (output_streamline_directory, "null_dist" + postfix(i) + ".txt"));
        ++progress;
      }
    }

    const matrix_type pvalue_output = MR::Math::Stats::fwe_pvalue (null_distribution, default_enhanced, mask_inference);
    ++progress;
    

    for (size_t i = 0; i != num_hypotheses; ++i) {
      write_streamline_output (Path::join (output_streamline_directory, "fwe_1mpvalue" + postfix(i) + ".txt"), pvalue_output.col(i));
      ++progress;
      write_streamline_output (Path::join (output_streamline_directory, "uncorrected_pvalue" + postfix(i) + ".txt"), uncorrected_pvalues.col(i));
      ++progress;
      write_streamline_output (Path::join (output_streamline_directory, "null_contributions" + postfix(i) + ".txt"), null_contributions.col(i));
      ++progress;
    }
  }
}

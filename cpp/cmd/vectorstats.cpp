/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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
#include "progressbar.h"
#include "types.h"

#include "file/matrix.h"
#include "file/path.h"
#include "math/stats/fwe.h"
#include "math/stats/glm.h"
#include "math/stats/import.h"
#include "math/stats/shuffle.h"
#include "math/stats/typedefs.h"

#include "stats/permtest.h"

using namespace MR;
using namespace App;
using namespace MR::Math::Stats;
using namespace MR::Math::Stats::GLM;

using MR::Math::Stats::index_type;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Statistical testing of vector data using non-parametric permutation testing";

  DESCRIPTION
  + "This command can be used to perform permutation testing of any form of data."
    " The data for each input subject must be stored in a text file,"
    " with one value per row."
    " The data for each row across subjects will be tested independently,"
    " i.e. there is no statistical enhancement that occurs between the data;"
    " however family-wise error control will be used."

  + Math::Stats::GLM::column_ones_description;

  ARGUMENTS
  + Argument ("input", "a text file listing the file names of the input subject data").type_file_in()
  + Argument ("design", "the design matrix").type_file_in()
  + Argument ("contrast", "the contrast matrix").type_file_in()
  + Argument ("output", "the filename prefix for all output").type_text();

  OPTIONS
  + Math::Stats::shuffle_options(false)
  + Math::Stats::GLM::glm_options("element");

}
// clang-format on

using Math::Stats::matrix_type;
using Math::Stats::vector_type;
using Stats::PermTest::count_matrix_type;

// Define data importer class that willl obtain data for a
//   specific subject based on the string path to the data file for
//   that subject
//
// This is far more simple than the equivalent functionality in other
//   MRtrix3 statistical inference commands, since the data are
//   already in a vectorised form.

class SubjectVectorImport : public SubjectDataImportBase {
public:
  SubjectVectorImport(const std::string &path) : SubjectDataImportBase(path), data(File::Matrix::load_vector(path)) {}

  void operator()(matrix_type::RowXpr row) const override {
    assert(index_type(row.size()) == size());
    row = data;
  }

  default_type operator[](const index_type index) const override {
    assert(index < size());
    return data[index];
  }

  index_type size() const override { return data.size(); }

private:
  const vector_type data;
};

void run() {

  // Unlike other statistical inference commands, don't delay actual
  //   loading of input data: feasible for the input itself to be
  //   a text file containing raw numerical matrix data, rather than
  //   a list of files
  CohortDataImport importer;
  matrix_type data;
  index_type num_inputs = 0, num_elements = 0;
  try {
    importer.initialise<SubjectVectorImport>(argument[0]);
    num_inputs = importer.size();
    num_elements = importer[0]->size();
    for (index_type i = 0; i != importer.size(); ++i) {
      if (importer[i]->size() != num_elements)
        throw Exception("Subject file \"" + importer[i]->name() + "\" contains incorrect number of elements (" +
                        str(importer[i]) + "; expected " + str(num_elements) + ")");
    }
    data.resize(num_inputs, num_elements);
    for (index_type subject = 0; subject != num_inputs; subject++)
      (*importer[subject])(data.row(subject));
  } catch (Exception &e_asfilelist) {
    try {
      data = File::Matrix::load_matrix(argument[0]);
      num_inputs = data.rows();
      num_elements = data.cols();
    } catch (Exception &e_asmatrix) {
      Exception e("Unable to load input data from file \"" + argument[0] + '"');
      e.push_back("Error when interpreted as containing list of file names: ");
      e.push_back(e_asfilelist);
      e.push_back("Error when interpreted as numerical matrix data: ");
      e.push_back(e_asmatrix);
      throw e;
    }
  }
  CONSOLE("Number of subjects: " + str(num_inputs));
  CONSOLE("Number of elements: " + str(num_elements));

  // Load design matrix
  const matrix_type design = File::Matrix::load_matrix(argument[1]);
  if (index_type(design.rows()) != num_inputs)
    throw Exception("Number of subjects (" + str(num_inputs) + ") does not match number of rows in design matrix (" +
                    str(design.rows()) + ")");

  // Before validating the contrast matrix, we first need to see if there are any
  //   additional design matrix columns coming from element-wise subject data
  std::vector<CohortDataImport> extra_columns;
  bool nans_in_columns = false;
  auto opt = get_options("column");
  for (size_t i = 0; i != opt.size(); ++i) {
    extra_columns.push_back(CohortDataImport());
    extra_columns[i].initialise<SubjectVectorImport>(opt[i][0]);
    if (!extra_columns[i].allFinite())
      nans_in_columns = true;
  }
  const bool have_extra_columns = !extra_columns.empty();
  const index_type num_factors = design.cols() + extra_columns.size();
  CONSOLE("Number of factors: " + str(num_factors));
  if (have_extra_columns) {
    CONSOLE("Number of element-wise design matrix columns: " + str(extra_columns.size()));
    if (nans_in_columns)
      CONSOLE("Non-finite values detected in element-wise design matrix columns;"
              " individual rows will be removed from voxel-wise design matrices accordingly");
  }
  check_design(design, have_extra_columns);

  // Load variance groups
  auto variance_groups = GLM::load_variance_groups(num_inputs);
  const index_type num_vgs = variance_groups.size() ? variance_groups.maxCoeff() + 1 : 1;
  if (num_vgs > 1)
    CONSOLE("Number of variance groups: " + str(num_vgs));

  // Load hypotheses
  const std::vector<Hypothesis> hypotheses = Math::Stats::GLM::load_hypotheses(argument[2]);
  const index_type num_hypotheses = hypotheses.size();
  if (hypotheses[0].cols() != num_factors)
    throw Exception(
        "The number of columns in the contrast matrix (" + str(hypotheses[0].cols()) + ")" +
        " does not equal the number of columns in the design matrix (" + str(design.cols()) + ")" +
        (have_extra_columns ? " (taking into account the " + str(extra_columns.size()) + " uses of -column)" : ""));
  CONSOLE("Number of hypotheses: " + str(num_hypotheses));

  const std::string output_prefix = argument[3];

  const bool nans_in_data = !data.allFinite();
  if (nans_in_data) {
    INFO("Non-finite values present in data; rows will be removed from element-wise design matrices accordingly");
    if (!have_extra_columns) {
      INFO("(Note that this will result in slower execution than if such values were not present)");
    }
  }
  const bool variable_design_matrix = nans_in_data || have_extra_columns;

  // Only add contrast matrix row number to image outputs if there's more than one hypothesis
  auto postfix = [&](const index_type i) { return (num_hypotheses > 1) ? ("_" + hypotheses[i].name()) : ""; };

  {
    matrix_type betas(num_factors, num_elements);
    matrix_type abs_effect_size(num_elements, num_hypotheses);
    matrix_type std_effect_size(num_elements, num_hypotheses);
    matrix_type stdev(num_vgs, num_elements);
    vector_type cond(num_elements);

    Math::Stats::GLM::all_stats(
        data, design, extra_columns, hypotheses, variance_groups, cond, betas, abs_effect_size, std_effect_size, stdev);

    ProgressBar progress("Outputting beta coefficients, effect size and standard deviation",
                         2 + (2 * num_hypotheses) + (variable_design_matrix ? 1 : 0));
    File::Matrix::save_matrix(betas, output_prefix + "betas.csv");
    ++progress;
    for (index_type i = 0; i != num_hypotheses; ++i) {
      if (!hypotheses[i].is_F()) {
        File::Matrix::save_vector(abs_effect_size.col(i), output_prefix + "abs_effect" + postfix(i) + ".csv");
        ++progress;
        if (num_vgs == 1)
          File::Matrix::save_vector(std_effect_size.col(i), output_prefix + "std_effect" + postfix(i) + ".csv");
      } else {
        ++progress;
      }
      ++progress;
    }
    if (variable_design_matrix) {
      File::Matrix::save_vector(cond, output_prefix + "cond.csv");
      ++progress;
    }
    if (num_vgs == 1)
      File::Matrix::save_vector(stdev.row(0), output_prefix + "std_dev.csv");
    else
      File::Matrix::save_matrix(stdev, output_prefix + "std_dev.csv");
  }

  // Construct the class for performing the initial statistical tests
  std::shared_ptr<GLM::TestBase> glm_test;
  if (variable_design_matrix) {
    if (variance_groups.size())
      glm_test.reset(new GLM::TestVariableHeteroscedastic(
          extra_columns, data, design, hypotheses, variance_groups, nans_in_data, nans_in_columns));
    else
      glm_test.reset(
          new GLM::TestVariableHomoscedastic(extra_columns, data, design, hypotheses, nans_in_data, nans_in_columns));
  } else {
    if (variance_groups.size())
      glm_test.reset(new GLM::TestFixedHeteroscedastic(data, design, hypotheses, variance_groups));
    else
      glm_test.reset(new GLM::TestFixedHomoscedastic(data, design, hypotheses));
  }

  // Precompute default statistic
  // Don't use convenience function: No enhancer!
  // Manually construct default shuffling matrix
  // TODO Change to use convenience function; we make an empty enhancer later anyway
  const matrix_type default_shuffle(matrix_type::Identity(num_inputs, num_inputs));
  matrix_type default_statistic, default_zstat;
  (*glm_test)(default_shuffle, default_statistic, default_zstat);
  for (index_type i = 0; i != num_hypotheses; ++i) {
    File::Matrix::save_matrix(default_statistic.col(i),
                              output_prefix + (hypotheses[i].is_F() ? "F" : "t") + "value" + postfix(i) + ".csv");
    File::Matrix::save_matrix(default_zstat.col(i), output_prefix + "Zstat" + postfix(i) + ".csv");
  }

  // Perform permutation testing
  if (get_options("notest").empty()) {

    const bool fwe_strong = !get_options("strong").empty();
    if (fwe_strong && num_hypotheses == 1) {
      WARN("Option -strong has no effect when testing a single hypothesis only");
    }

    std::shared_ptr<Stats::EnhancerBase> enhancer;
    matrix_type null_distribution, uncorrected_pvalues;
    count_matrix_type null_contributions;
    matrix_type empirical_distribution; // unused
    Stats::PermTest::run_permutations(glm_test,
                                      enhancer,
                                      empirical_distribution,
                                      default_zstat,
                                      fwe_strong,
                                      null_distribution,
                                      null_contributions,
                                      uncorrected_pvalues);
    if (fwe_strong) {
      File::Matrix::save_vector(null_distribution.col(0), output_prefix + "null_dist.csv");
    } else {
      for (index_type i = 0; i != num_hypotheses; ++i)
        File::Matrix::save_vector(null_distribution.col(i), output_prefix + "null_dist" + postfix(i) + ".csv");
    }
    const matrix_type fwe_pvalues = MR::Math::Stats::fwe_pvalue(null_distribution, default_zstat);
    for (index_type i = 0; i != num_hypotheses; ++i) {
      File::Matrix::save_vector(fwe_pvalues.col(i), output_prefix + "fwe_1mpvalue" + postfix(i) + ".csv");
      File::Matrix::save_vector(uncorrected_pvalues.col(i),
                                output_prefix + "uncorrected_1mpvalue" + postfix(i) + ".csv");
      File::Matrix::save_vector(null_contributions.col(i), output_prefix + "null_contributions" + postfix(i) + ".csv");
    }
  }
}

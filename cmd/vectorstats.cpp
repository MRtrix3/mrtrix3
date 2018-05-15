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
#include "types.h"

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



void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Statistical testing of vector data using non-parametric permutation testing";

  DESCRIPTION
  + "This command can be used to perform permutation testing of any form of data. "
    "The data for each input subject must be stored in a text file, with one value per row. "
    "The data for each row across subjects will be tested independently, i.e. there is no "
    "statistical enhancement that occurs between the data; however family-wise error control "
    "will be used."

  + Math::Stats::GLM::column_ones_description;


  ARGUMENTS
  + Argument ("input", "a text file listing the file names of the input subject data").type_file_in ()

  + Argument ("design", "the design matrix").type_file_in ()

  + Argument ("contrast", "the contrast matrix").type_file_in ()

  + Argument ("output", "the filename prefix for all output").type_text();


  OPTIONS
  + Math::Stats::shuffle_options (false)

  + Math::Stats::GLM::glm_options ("element");

}



using Math::Stats::matrix_type;
using Math::Stats::vector_type;



// Define data importer class that willl obtain data for a
//   specific subject based on the string path to the data file for
//   that subject
//
// This is far more simple than the equivalent functionality in other
//   MRtrix3 statistical inference commands, since the data are
//   already in a vectorised form.

class SubjectVectorImport : public SubjectDataImportBase
{ MEMALIGN(SubjectVectorImport)
  public:
    SubjectVectorImport (const std::string& path) :
        SubjectDataImportBase (path),
        data (load_vector (path)) { }

    void operator() (matrix_type::RowXpr row) const override
    {
      assert (size_t(row.size()) == size());
      row = data;
    }

    default_type operator[] (const size_t index) const override
    {
      assert (index < size());
      return data[index];
    }

    size_t size() const override { return data.size(); }

  private:
    const vector_type data;

};



void run()
{

  CohortDataImport importer;
  importer.initialise<SubjectVectorImport> (argument[0]);
  const size_t num_subjects = importer.size();
  CONSOLE ("Number of subjects: " + str(num_subjects));
  const size_t num_elements = importer[0]->size();
  CONSOLE ("Number of elements: " + str(num_elements));
  for (size_t i = 0; i != importer.size(); ++i) {
    if (importer[i]->size() != num_elements)
      throw Exception ("Subject file \"" + importer[i]->name() + "\" contains incorrect number of elements (" + str(importer[i]) + "; expected " + str(num_elements) + ")");
  }

  // Load design matrix
  const matrix_type design = load_matrix (argument[1]);
  if (size_t(design.rows()) != num_subjects)
    throw Exception ("Number of subjects (" + str(num_subjects) + ") does not match number of rows in design matrix (" + str(design.rows()) + ")");

  // Load contrasts
  const vector<Contrast> contrasts = Math::Stats::GLM::load_contrasts (argument[2]);
  const size_t num_contrasts = contrasts.size();
  CONSOLE ("Number of contrasts: " + str(num_contrasts));

  // Before validating the contrasts, we first need to see if there are any
  //   additional design matrix columns coming from element-wise subject data
  vector<CohortDataImport> extra_columns;
  bool nans_in_columns = false;
  auto opt = get_options ("column");
  for (size_t i = 0; i != opt.size(); ++i) {
    extra_columns.push_back (CohortDataImport());
    extra_columns[i].initialise<SubjectVectorImport> (opt[i][0]);
    if (!extra_columns[i].allFinite())
      nans_in_columns = true;
  }
  if (extra_columns.size()) {
    CONSOLE ("Number of element-wise design matrix columns: " + str(extra_columns.size()));
    if (nans_in_columns)
      INFO ("Non-finite values detected in element-wise design matrix columns; individual rows will be removed from voxel-wise design matrices accordingly");
  }

  const ssize_t num_factors = design.cols() + extra_columns.size();
  if (contrasts[0].cols() != num_factors)
    throw Exception ("The number of columns per contrast (" + str(contrasts[0].cols()) + ")"
                     + " does not equal the number of columns in the design matrix (" + str(design.cols()) + ")"
                     + (extra_columns.size() ? " (taking into account the " + str(extra_columns.size()) + " uses of -column)" : ""));
  CONSOLE ("Number of factors: " + str(num_factors));

  const std::string output_prefix = argument[3];

  // Load input data
  matrix_type data (num_subjects, num_elements);
  for (size_t subject = 0; subject != num_subjects; subject++)
    (*importer[subject]) (data.row(subject));

  const bool nans_in_data = !data.allFinite();
  if (nans_in_data) {
    INFO ("Non-finite values present in data; rows will be removed from element-wise design matrices accordingly");
    if (!extra_columns.size()) {
      INFO ("(Note that this will result in slower execution than if such values were not present)");
    }
  }

  // Only add contrast row number to image outputs if there's more than one contrast
  auto postfix = [&] (const size_t i) { return (num_contrasts > 1) ? ("_" + contrasts[i].name()) : ""; };

  {
    matrix_type betas (num_factors, num_elements);
    matrix_type abs_effect_size (num_elements, num_contrasts), std_effect_size (num_elements, num_contrasts);
    vector_type cond (num_elements), stdev (num_elements);

    Math::Stats::GLM::all_stats (data, design, extra_columns, contrasts,
                                 cond, betas, abs_effect_size, std_effect_size, stdev);

    ProgressBar progress ("Outputting beta coefficients, effect size and standard deviation", 2 + (2 * num_contrasts) + (nans_in_data || extra_columns.size() ? 1 : 0));
    save_matrix (betas, output_prefix + "betas.csv"); ++progress;
    for (size_t i = 0; i != num_contrasts; ++i) {
      if (!contrasts[i].is_F()) {
        save_vector (abs_effect_size.col(i), output_prefix + "abs_effect" + postfix(i) + ".csv");
        ++progress;
        save_vector (std_effect_size.col(i), output_prefix + "std_effect" + postfix(i) + ".csv");
        ++progress;
      }
    }
    if (nans_in_data || extra_columns.size()) {
      save_vector (cond, output_prefix + "cond.csv");
      ++progress;
    }
    save_vector (stdev, output_prefix + "std_dev.csv");
  }

  // Construct the class for performing the initial statistical tests
  std::shared_ptr<GLM::TestBase> glm_test;
  if (extra_columns.size() || nans_in_data) {
    glm_test.reset (new GLM::TestVariable (extra_columns, data, design, contrasts, nans_in_data, nans_in_columns));
  } else {
    glm_test.reset (new GLM::TestFixed (data, design, contrasts));
  }

  // Precompute default statistic
  // Don't use convenience function: No enhancer!
  // Manually construct default shuffling matrix
  // TODO Change to use convenience function; we make an empty enhancer later anyway
  const matrix_type default_shuffle (matrix_type::Identity (num_subjects, num_subjects));
  matrix_type default_tvalues;
  (*glm_test) (default_shuffle, default_tvalues);
  for (size_t i = 0; i != num_contrasts; ++i)
    save_matrix (default_tvalues.col(i), output_prefix + (contrasts[i].is_F() ? "F" : "t") + "value" + postfix(i) + ".csv");

  // Perform permutation testing
  if (!get_options ("notest").size()) {

    std::shared_ptr<Stats::EnhancerBase> enhancer;
    matrix_type null_distribution, uncorrected_pvalues;
    matrix_type empirical_distribution; // unused
    Stats::PermTest::run_permutations (glm_test, enhancer, empirical_distribution,
                                       default_tvalues, null_distribution, uncorrected_pvalues);

    const matrix_type fwe_pvalues = MR::Math::Stats::fwe_pvalue (null_distribution, default_tvalues);
    for (size_t i = 0; i != num_contrasts; ++i) {
      save_vector (fwe_pvalues.col(i), output_prefix + "fwe_pvalue" + postfix(i) + ".csv");
      save_vector (uncorrected_pvalues.col(i), output_prefix + "uncorrected_pvalue" + postfix(i) + ".csv");
    }

  }
}


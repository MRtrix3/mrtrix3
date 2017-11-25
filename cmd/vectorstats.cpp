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


#include <vector>

#include "command.h"
#include "progressbar.h"

#include "file/path.h"
#include "math/stats/glm.h"
#include "math/stats/import.h"
#include "math/stats/permutation.h"
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
  + Stats::PermTest::Options (false)

  + OptionGroup ("Additional options for vectorstats")

    + Option ("column", "add a column to the design matrix corresponding to subject element-wise values "
                        "(the contrast vector length must include columns for these additions)").allow_multiple()
      + Argument ("path").type_file_in();

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

    void operator() (matrix_type::ColXpr column) const override
    {
      assert (column.rows() == size());
      column = data;
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

  size_t num_perms = get_option_value ("nperms", DEFAULT_NUMBER_PERMUTATIONS);

  // Load design matrix
  const matrix_type design = load_matrix (argument[1]);
  if (size_t(design.rows()) != num_subjects)
    throw Exception ("Number of subjects (" + str(num_subjects) + ") does not match number of rows in design matrix (" + str(design.rows()) + ")");

  // Load permutations file if supplied
  auto opt = get_options("permutations");
  vector<vector<size_t> > permutations;
  if (opt.size()) {
    permutations = Math::Stats::Permutation::load_permutations_file (opt[0][0]);
    num_perms = permutations.size();
    if (permutations[0].size() != (size_t)design.rows())
      throw Exception ("number of rows in the permutations file (" + str(opt[0][0]) + ") does not match number of rows in design matrix");
  }

  // Load contrast matrix
  vector<Contrast> contrasts;
  {
    const matrix_type contrast_matrix = load_matrix (argument[2]);
    for (ssize_t row = 0; row != contrast_matrix.rows(); ++row)
      contrasts.emplace_back (Contrast (contrast_matrix.row (row)));
  }
  const size_t num_contrasts = contrasts.size();
  CONSOLE ("Number of contrasts: " + str(num_contrasts));

  // Before validating the contrast matrix, we first need to see if there are any
  //   additional design matrix columns coming from voxel-wise subject data
  vector<CohortDataImport> extra_columns;
  bool nans_in_columns = false;
  opt = get_options ("column");
  for (size_t i = 0; i != opt.size(); ++i) {
    extra_columns.push_back (CohortDataImport());
    extra_columns[i].initialise<SubjectVectorImport> (opt[i][0]);
    if (!extra_columns[i].allFinite())
      nans_in_columns = true;
  }
  if (extra_columns.size()) {
    CONSOLE ("number of element-wise design matrix columns: " + str(extra_columns.size()));
    if (nans_in_columns)
      INFO ("Non-finite values detected in element-wise design matrix columns; individual rows will be removed from voxel-wise design matrices accordingly");
  }

  const ssize_t num_factors = design.cols() + extra_columns.size();
  if (contrasts[0].cols() != num_factors)
    throw Exception ("the number of columns per contrast (" + str(contrasts[0].cols()) + ")"
                     + " does not equal the number of columns in the design matrix (" + str(design.cols()) + ")"
                     + (extra_columns.size() ? " (taking into account the " + str(extra_columns.size()) + " uses of -column)" : ""));
  CONSOLE ("Number of factors: " + str(num_factors));

  const std::string output_prefix = argument[3];

  // Load input data
  matrix_type data (num_elements, num_subjects);
  for (size_t subject = 0; subject != num_subjects; subject++)
    (*importer[subject]) (data.col(subject));

  const bool nans_in_data = !data.allFinite();
  if (nans_in_data) {
    INFO ("Non-finite values present in data; rows will be removed from element-wise design matrices accordingly");
    if (!extra_columns.size()) {
      INFO ("(Note that this will result in slower execution than if such values were not present)");
    }
  }

  // Construct the class for performing the initial statistical tests
  std::shared_ptr<GLM::TestBase> glm_test;
  if (extra_columns.size() || nans_in_data) {
    glm_test.reset (new GLM::TestVariable (extra_columns, data, design, contrasts, nans_in_data, nans_in_columns));
  } else {
    glm_test.reset (new GLM::TestFixed (data, design, contrasts));
  }


  // Only add contrast row number to image outputs if there's more than one contrast
  auto postfix = [&] (const size_t i) { return (num_contrasts > 1) ? ("_" + str(i)) : ""; };

  {
    matrix_type betas (num_factors, num_elements);
    matrix_type abs_effect_size (num_contrasts, num_elements), std_effect_size (num_contrasts, num_elements);
    vector_type stdev (num_elements);

    if (extra_columns.size()) {

      // For each variable of interest (e.g. beta coefficients, effect size etc.) need to:
      //   Construct the output data vector, with size = num_voxels
      //   For each voxel:
      //     Use glm_test to obtain the design matrix for the default permutation for that voxel
      //     Use the relevant Math::Stats::GLM function to get the value of interest for just that voxel
      //       (will still however need to come out as a matrix_type)
      //     Write that value to data vector
      //   Finally, use write_output() function to write to an image file
      class Source
      { NOMEMALIGN
        public:
          Source (const size_t num_elements) :
              num_elements (num_elements),
              counter (0),
              progress (new ProgressBar ("calculating basic properties of default permutation", num_elements)) { }

          bool operator() (size_t& index)
          {
            index = counter++;
            if (counter >= num_elements) {
              progress.reset();
              return false;
            }
            assert (progress);
            ++(*progress);
            return true;
          }

        private:
          const size_t num_elements;
          size_t counter;
          std::unique_ptr<ProgressBar> progress;
      };

      class Functor
      { MEMALIGN(Functor)
        public:
          Functor (const matrix_type& data, std::shared_ptr<GLM::TestBase> glm_test, const vector<Contrast>& contrasts,
                   matrix_type& betas, matrix_type& abs_effect_size, matrix_type& std_effect_size, vector_type& stdev) :
              data (data),
              glm_test (glm_test),
              contrasts (contrasts),
              global_betas (betas),
              global_abs_effect_size (abs_effect_size),
              global_std_effect_size (std_effect_size),
              global_stdev (stdev) { }

          bool operator() (const size_t& index)
          {
            const matrix_type data_element = data.row (index);
            const matrix_type design_element = dynamic_cast<const GLM::TestVariable* const>(glm_test.get())->default_design (index);
            Math::Stats::GLM::all_stats (data_element, design_element, contrasts,
                                         local_betas, local_abs_effect_size, local_std_effect_size, local_stdev);
            global_betas.col (index) = local_betas;
            global_abs_effect_size.col(index) = local_abs_effect_size.col(0);
            global_std_effect_size.col(index) = local_std_effect_size.col(0);
            global_stdev[index] = local_stdev[0];
            return true;
          }

        private:
          const matrix_type& data;
          const std::shared_ptr<GLM::TestBase> glm_test;
          const vector<Contrast>& contrasts;
          matrix_type& global_betas;
          matrix_type& global_abs_effect_size;
          matrix_type& global_std_effect_size;
          vector_type& global_stdev;
          matrix_type local_betas, local_abs_effect_size, local_std_effect_size;
          vector_type local_stdev;
      };

      Source source (num_elements);
      Functor functor (data, glm_test, contrasts,
                       betas, abs_effect_size, std_effect_size, stdev);
      Thread::run_queue (source, Thread::batch (size_t()), Thread::multi (functor));

    } else {

      ProgressBar progress ("calculating basic properties of default permutation");
      Math::Stats::GLM::all_stats (data, design, contrasts,
                                   betas, abs_effect_size, std_effect_size, stdev);
    }

    ProgressBar progress ("outputting beta coefficients, effect size and standard deviation", 2 + (2 * num_contrasts));
    save_matrix (betas, output_prefix + "betas.csv"); ++progress;
    for (size_t i = 0; i != num_contrasts; ++i) {
      save_vector (abs_effect_size.col(i), output_prefix + "abs_effect" + postfix(i) + ".csv"); ++progress;
      save_vector (std_effect_size.col(i), output_prefix + "std_effect" + postfix(i) + ".csv"); ++progress;
    }
    save_vector (stdev, output_prefix + "std_dev.csv");
  }


  // Precompute default statistic
  // Don't use convenience function: No enhancer!
  // Manually construct default permutation
  vector<size_t> default_permutation (num_subjects);
  for (size_t i = 0; i != num_subjects; ++i)
    default_permutation[i] = i;
  matrix_type default_tvalues;
  (*glm_test) (default_permutation, default_tvalues);
  for (size_t i = 0; i != num_contrasts; ++i)
    save_matrix (default_tvalues.col(i), output_prefix + "tvalue" + postfix(i) + ".csv");

  // Perform permutation testing
  if (!get_options ("notest").size()) {

    std::shared_ptr<Stats::EnhancerBase> enhancer;
    matrix_type null_distribution (num_perms, num_contrasts);
    matrix_type uncorrected_pvalues (num_elements, num_contrasts);
    matrix_type empirical_distribution;

    if (permutations.size()) {
      Stats::PermTest::run_permutations (permutations, glm_test, enhancer, empirical_distribution,
                                         default_tvalues, null_distribution, uncorrected_pvalues);
    } else {
      Stats::PermTest::run_permutations (num_perms, glm_test, enhancer, empirical_distribution,
                                         default_tvalues, null_distribution, uncorrected_pvalues);
    }

    matrix_type default_pvalues (num_elements, num_contrasts);
    Math::Stats::Permutation::statistic2pvalue (null_distribution, default_tvalues, default_pvalues);
    for (size_t i = 0; i != num_contrasts; ++i) {
      save_vector (default_pvalues.col(i), output_prefix + "fwe_pvalue" + postfix(i) + ".csv");
      save_vector (uncorrected_pvalues.col(i), output_prefix + "uncorrected_pvalue" + postfix(i) + ".csv");
    }

  }
}


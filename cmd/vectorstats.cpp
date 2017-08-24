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



void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Statistical testing of vector data using non-parametric permutation testing";

  DESCRIPTION
      + Math::Stats::glm_column_ones_description;


  ARGUMENTS
  + Argument ("input", "a text file listing the file names of the input subject data").type_file_in ()

  + Argument ("design", "the design matrix").type_file_in ()

  + Argument ("contrast", "the contrast matrix").type_file_in ()

  + Argument ("output", "the filename prefix for all output").type_text();


  OPTIONS
  + Stats::PermTest::Options (false);

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
  const matrix_type contrast = load_matrix (argument[2]);
  const size_t num_contrasts = contrast.rows();
  if (contrast.cols() != design.cols())
    throw Exception ("number of columns in contrast matrix (" + str(contrast.cols()) + ") does not match number of columns in design matrix (" + str(design.cols()) + ")");

  const std::string output_prefix = argument[3];

  // Load input data
  matrix_type data (num_elements, num_subjects);
  for (size_t subject = 0; subject != num_subjects; subject++)
    (*importer[subject]) (data.col(subject));

  {
    matrix_type betas, abs_effects, std_effects, stdevs;
    Math::Stats::GLM::all_stats (data, design, contrast, betas, abs_effects, std_effects, stdevs);
    save_matrix (betas, output_prefix + "betas.csv");
    save_matrix (abs_effects, output_prefix + "abs_effect.csv");
    save_matrix (std_effects, output_prefix + "std_effect.csv");
    save_matrix (stdevs, output_prefix + "std_dev.csv");
  }

  std::shared_ptr<Math::Stats::GLMTestBase> glm_ttest (new Math::Stats::GLMTTestFixed (data, design, contrast));

  // Precompute default statistic
  // Don't use convenience function: No enhancer!
  // Manually construct default permutation
  vector<size_t> default_permutation (num_subjects);
  for (size_t i = 0; i != num_subjects; ++i)
    default_permutation[i] = i;
  matrix_type default_tvalues;
  (*glm_ttest) (default_permutation, default_tvalues);
  save_matrix (default_tvalues, output_prefix + "tvalue.csv");

  // Perform permutation testing
  if (!get_options ("notest").size()) {

    std::shared_ptr<Stats::EnhancerBase> enhancer;
    matrix_type null_distribution (num_perms, num_contrasts);
    matrix_type uncorrected_pvalues (num_elements, num_contrasts);
    matrix_type empirical_distribution;

    if (permutations.size()) {
      Stats::PermTest::run_permutations (permutations, glm_ttest, enhancer, empirical_distribution,
                                         default_tvalues, null_distribution, uncorrected_pvalues);
    } else {
      Stats::PermTest::run_permutations (num_perms, glm_ttest, enhancer, empirical_distribution,
                                         default_tvalues, null_distribution, uncorrected_pvalues);
    }

    matrix_type default_pvalues (num_elements, num_contrasts);
    Math::Stats::Permutation::statistic2pvalue (null_distribution, default_tvalues, default_pvalues);
    save_matrix (default_pvalues, output_prefix + "fwe_pvalue.csv");
    save_matrix (uncorrected_pvalues, output_prefix + "uncorrected_pvalue.csv");

  }
}


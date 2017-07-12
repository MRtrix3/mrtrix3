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
#include "math/stats/permutation.h"
#include "math/stats/typedefs.h"

#include "stats/permtest.h"


using namespace MR;
using namespace App;



void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Statistical testing of vector data using non-parametric permutation testing";


  ARGUMENTS
  + Argument ("input", "a text file listing the file names of the input subject data").type_file_in ()

  + Argument ("design", "the design matrix. Note that a column of 1's will need to be added for correlations.").type_file_in ()

  + Argument ("contrast", "the contrast vector, specified as a single row of weights").type_file_in ()

  + Argument ("output", "the filename prefix for all output.").type_text();


  OPTIONS
  + Stats::PermTest::Options (false);

}



using Math::Stats::matrix_type;
using Math::Stats::vector_type;



void run()
{

  // Read filenames
  vector<std::string> filenames;
  {
    std::string folder = Path::dirname (argument[0]);
    std::ifstream ifs (argument[0].c_str());
    std::string temp;
    while (getline (ifs, temp)) {
      std::string filename (Path::join (folder, temp));
      size_t p = filename.find_last_not_of(" \t");
      if (std::string::npos != p)
        filename.erase(p+1);
      if (filename.size()) {
        if (!MR::Path::exists (filename))
          throw Exception ("Input data vector file not found: \"" + filename + "\"");
        filenames.push_back (filename);
      }
    }
  }

  const vector_type example_data = load_vector (filenames.front());
  const size_t num_elements = example_data.size();

  size_t num_perms = get_option_value ("nperms", DEFAULT_NUMBER_PERMUTATIONS);

  // Load design matrix
  const matrix_type design = load_matrix (argument[1]);
  if (size_t(design.rows()) != filenames.size())
    throw Exception ("number of subjects does not match number of rows in design matrix");

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
  matrix_type data (num_elements, filenames.size());
  {
    ProgressBar progress ("Loading input vector data", filenames.size());
    for (size_t subject = 0; subject < filenames.size(); subject++) {

      const std::string& path (filenames[subject]);
      vector_type subject_data;
      try {
        subject_data = load_vector (path);
      } catch (Exception& e) {
        throw Exception (e, "Error loading vector data for subject #" + str(subject) + " (file \"" + path + "\")");
      }

      if (size_t(subject_data.size()) != num_elements)
        throw Exception ("Vector data for subject #" + str(subject) + " (file \"" + path + "\") is wrong length (" + str(subject_data.size()) + " , expected " + str(num_elements) + ")");

      data.col(subject) = subject_data;

      ++progress;
    }
  }

  {
    const matrix_type betas = Math::Stats::GLM::solve_betas (data, design);
    save_matrix (betas, output_prefix + "betas.csv");

    const matrix_type abs_effects = Math::Stats::GLM::abs_effect_size (data, design, contrast);
    save_matrix (abs_effects, output_prefix + "abs_effect.csv");

    const matrix_type std_effects = Math::Stats::GLM::std_effect_size (data, design, contrast);
    save_matrix (std_effects, output_prefix + "std_effect.csv");

    const matrix_type stdevs = Math::Stats::GLM::stdev (data, design);
    save_matrix (stdevs, output_prefix + "std_dev.csv");
  }

  std::shared_ptr<Math::Stats::GLMTestBase> glm_ttest (new Math::Stats::GLMTTestFixed (data, design, contrast));

  // Precompute default statistic
  // Don't use convenience function: No enhancer!
  // Manually construct default permutation
  vector<size_t> default_permutation (filenames.size());
  for (size_t i = 0; i != filenames.size(); ++i)
    default_permutation[i] = i;
  matrix_type default_tvalues;
  (*glm_ttest) (default_permutation, default_tvalues);
  save_matrix (default_tvalues, output_prefix + "tvalue.csv");

  // Perform permutation testing
  if (!get_options ("notest").size()) {

    std::shared_ptr<Stats::EnhancerBase> enhancer;
    matrix_type null_distribution (num_perms, num_contrasts), uncorrected_pvalues (num_perms, num_contrasts);
    matrix_type empirical_distribution;

    if (permutations.size()) {
      Stats::PermTest::run_permutations (permutations, glm_ttest, enhancer, empirical_distribution,
                                         default_tvalues, null_distribution, uncorrected_pvalues);
    } else {
      Stats::PermTest::run_permutations (num_perms, glm_ttest, enhancer, empirical_distribution,
                                         default_tvalues, null_distribution, uncorrected_pvalues);
    }

    matrix_type default_pvalues (num_contrasts, num_elements);
    Math::Stats::Permutation::statistic2pvalue (null_distribution, default_tvalues, default_pvalues);
    save_matrix (default_pvalues, output_prefix + "fwe_pvalue.csv");
    save_matrix (uncorrected_pvalues, output_prefix + "uncorrected_pvalue.csv");

  }
}

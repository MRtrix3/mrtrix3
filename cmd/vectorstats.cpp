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


#define NPERMS_DEFAULT 5000


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "Statistical testing of vector data using non-parametric permutation testing.";


  ARGUMENTS
  + Argument ("input", "a text file listing the file names of the input subject data").type_file_in ()

  + Argument ("design", "the design matrix. Note that a column of 1's will need to be added for correlations.").type_file_in ()

  + Argument ("contrast", "the contrast vector, specified as a single row of weights").type_file_in ()

  + Argument ("output", "the filename prefix for all output.").type_text();


  OPTIONS

  + Option ("notest", "don't perform permutation testing and only output population statistics (effect size, stdev etc)")

  + Option ("nperms", "the number of permutations (default: " + str(NPERMS_DEFAULT) + ").")
  + Argument ("num").type_integer (1);

}



using Math::Stats::matrix_type;
using Math::Stats::vector_type;



void run()
{

  // Read filenames
  std::vector<std::string> filenames;
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

  const size_t num_perms = get_option_value ("nperms", NPERMS_DEFAULT);

  // Load design matrix
  const matrix_type design = load_matrix (argument[2]);
  if (size_t(design.rows()) != filenames.size())
    throw Exception ("number of subjects does not match number of rows in design matrix");

  // Load contrast matrix
  matrix_type contrast = load_matrix (argument[3]);
  if (contrast.cols() > design.cols())
    throw Exception ("too many contrasts for design matrix");
  contrast.conservativeResize (contrast.rows(), design.cols());

  const std::string output_prefix = argument[4];

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
        throw Exception (e, "Error loading vector data for subject #" + str(subject) + " (file \"" + path + "\"");
      }

      if (size_t(subject_data.size()) != num_elements)
        throw Exception ("Vector data for subject #" + str(subject) + " (file \"" + path + "\") is wrong length (" + str(subject_data.size()) + " , expected " + str(num_elements) + ")");

      data.col(subject) = subject_data;

      ++progress;
    }
  }

  {
    ProgressBar progress ("outputting beta coefficients, effect size and standard deviation...", contrast.cols() + 3);

    const matrix_type betas = Math::Stats::GLM::solve_betas (data, design);
    for (size_t i = 0; i < size_t(contrast.cols()); ++i) {
      save_vector (betas.col(i), output_prefix + "_beta_" + str(i) + ".csv");
      ++progress;
    }

    const matrix_type abs_effects = Math::Stats::GLM::abs_effect_size (data, design, contrast);
    save_vector (abs_effects.col(0), output_prefix + "_abs_effect.csv");
    ++progress;

    const matrix_type std_effects = Math::Stats::GLM::std_effect_size (data, design, contrast);
    vector_type first_std_effect = std_effects.col(0);
    for (size_t i = 0; i != num_elements; ++i) {
      if (!std::isfinite (first_std_effect[i]))
        first_std_effect[i] = 0.0;
    }
    save_vector (first_std_effect, output_prefix + "_std_effect.csv");
    ++progress;

    const matrix_type stdevs = Math::Stats::GLM::stdev (data, design);
    save_vector (stdevs.col(0), output_prefix + "_std_dev.csv");
  }

  Math::Stats::GLMTTest glm_ttest (data, design, contrast);

  // Precompute default statistic
  // Don't use convenience function: No enhancer!
  // Manually construct default permutation
  std::vector<size_t> default_permutation (filenames.size());
  for (size_t i = 0; i != filenames.size(); ++i)
    default_permutation[i] = i;
  vector_type default_tvalues;
  default_type max_stat, min_stat;
  glm_ttest (default_permutation, default_tvalues, max_stat, min_stat);
  save_vector (default_tvalues, output_prefix + "_tvalue.csv");

  // Perform permutation testing
  if (!get_options ("notest").size()) {

    vector_type null_distribution (num_perms), uncorrected_pvalues (num_perms);
    vector_type empirical_distribution;

    // TODO Need the float_type_changes updates in order to bypass enhancement
    Stats::PermTest::run_permutations (glm_ttest, nullptr, num_perms, empirical_distribution,
                                       default_tvalues, std::shared_ptr<vector_type>(),
                                       null_distribution, std::shared_ptr<vector_type>(),
                                       uncorrected_pvalues, std::shared_ptr<vector_type>());

    vector_type default_pvalues (num_elements);
    Math::Stats::Permutation::statistic2pvalue (null_distribution, default_tvalues, default_pvalues);
    save_vector (default_pvalues,     output_prefix + "_fwe_pvalue.csv");
    save_vector (uncorrected_pvalues, output_prefix + "_uncorrected_pvalue.csv");

  }

}

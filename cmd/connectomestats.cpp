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

#include "connectome/enhance.h"
#include "connectome/mat2vec.h"

#include "stats/permtest.h"


using namespace MR;
using namespace App;


const char* algorithms[] = { "nbs", "nbse", "none", nullptr };



// TODO Eventually these will move to some kind of TFCE header
#define TFCE_DH_DEFAULT 0.1
#define TFCE_E_DEFAULT 0.4
#define TFCE_H_DEFAULT 3.0



void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Connectome group-wise statistics at the edge level using non-parametric permutation testing";


  ARGUMENTS
  + Argument ("input", "a text file listing the file names of the input connectomes").type_file_in ()

  + Argument ("algorithm", "the algorithm to use in network-based clustering/enhancement. "
                           "Options are: " + join(algorithms, ", ")).type_choice (algorithms)

  + Argument ("design", "the design matrix. Note that a column of 1's will need to be added for correlations.").type_file_in ()

  + Argument ("contrast", "the contrast vector, specified as a single row of weights").type_file_in ()

  + Argument ("output", "the filename prefix for all output.").type_text();


  OPTIONS

  + Stats::PermTest::Options (true)

  // TODO OptionGroup these, and provide a generic loader function
  + Stats::TFCE::Options (TFCE_DH_DEFAULT, TFCE_E_DEFAULT, TFCE_H_DEFAULT)

  + OptionGroup ("Additional options for connectomestats")

  + Option ("threshold", "the t-statistic value to use in threshold-based clustering algorithms")
  + Argument ("value").type_float (0.0);


  REFERENCES + "* If using the NBS algorithm: \n"
               "Zalesky, A.; Fornito, A. & Bullmore, E. T. Network-based statistic: Identifying differences in brain networks. \n"
               "NeuroImage, 2010, 53, 1197-1207"

             + "* If using the NBSE algorithm: \n"
               "Vinokur, L.; Zalesky, A.; Raffelt, D.; Smith, R.E. & Connelly, A. A Novel Threshold-Free Network-Based Statistics Method: Demonstration using Simulated Pathology. \n"
               "OHBM, 2015, 4144"

             + "* If using the -nonstationary option: \n"
               "Salimi-Khorshidi, G.; Smith, S.M. & Nichols, T.E. Adjusting the effect of nonstationarity in cluster-based and TFCE inference. \n"
               "Neuroimage, 2011, 54(3), 2006-19";

}



using Math::Stats::matrix_type;
using Math::Stats::vector_type;



void load_tfce_parameters (Stats::TFCE::Wrapper& enhancer)
{
  const default_type dH = get_option_value ("tfce_dh", TFCE_DH_DEFAULT);
  const default_type E  = get_option_value ("tfce_e", TFCE_E_DEFAULT);
  const default_type H  = get_option_value ("tfce_h", TFCE_H_DEFAULT);
  enhancer.set_tfce_parameters (dH, E, H);
}



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
          throw Exception ("Input connectome file not found: \"" + filename + "\"");
        filenames.push_back (filename);
      }
    }
  }

  const MR::Connectome::matrix_type example_connectome = load_matrix (filenames.front());
  if (example_connectome.rows() != example_connectome.cols())
    throw Exception ("Connectome of first subject is not square (" + str(example_connectome.rows()) + " x " + str(example_connectome.cols()) + ")");
  const MR::Connectome::node_t num_nodes = example_connectome.rows();

  // Initialise enhancement algorithm
  std::shared_ptr<Stats::EnhancerBase> enhancer;
  switch (int(argument[1])) {
    case 0: {
      auto opt = get_options ("threshold");
      if (!opt.size())
        throw Exception ("For NBS algorithm, -threshold option must be provided");
      enhancer.reset (new MR::Connectome::Enhance::NBS (num_nodes, opt[0][0]));
      }
      break;
    case 1: {
      std::shared_ptr<Stats::TFCE::EnhancerBase> base (new MR::Connectome::Enhance::NBS (num_nodes));
      enhancer.reset (new Stats::TFCE::Wrapper (base));
      load_tfce_parameters (*(dynamic_cast<Stats::TFCE::Wrapper*>(enhancer.get())));
      if (get_options ("threshold").size())
        WARN (std::string (argument[1]) + " is a threshold-free algorithm; -threshold option ignored");
      }
      break;
    case 2: {
      enhancer.reset (new MR::Connectome::Enhance::PassThrough());
      if (get_options ("threshold").size())
        WARN ("No enhancement algorithm being used; -threshold option ignored");
      }
      break;
    default:
      throw Exception ("Unknown enhancement algorithm");
  }

  size_t num_perms = get_option_value ("nperms", DEFAULT_NUMBER_PERMUTATIONS);
  const bool do_nonstationary_adjustment = get_options ("nonstationary").size();
  size_t nperms_nonstationary = get_option_value ("nperms_nonstationarity", DEFAULT_NUMBER_PERMUTATIONS_NONSTATIONARITY);

  // Load design matrix
  const matrix_type design = load_matrix (argument[2]);
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

  // Load non-stationary correction permutations file if supplied
  opt = get_options("permutations_nonstationary");
  vector<vector<size_t> > permutations_nonstationary;
  if (opt.size()) {
    permutations_nonstationary = Math::Stats::Permutation::load_permutations_file (opt[0][0]);
    nperms_nonstationary = permutations.size();
    if (permutations_nonstationary[0].size() != (size_t)design.rows())
      throw Exception ("number of rows in the nonstationary permutations file (" + str(opt[0][0]) + ") does not match number of rows in design matrix");
  }


  // Load contrast matrix
  matrix_type contrast = load_matrix (argument[3]);
  if (contrast.cols() > design.cols())
    throw Exception ("too many contrasts for design matrix");
  contrast.conservativeResize (contrast.rows(), design.cols());

  const std::string output_prefix = argument[4];

  // Load input data
  // For compatibility with existing statistics code, symmetric matrix data is adjusted
  //   into vector form - one row per edge in the symmetric connectome. The Mat2Vec class
  //   deals with the re-ordering of matrix data into this form.
  MR::Connectome::Mat2Vec mat2vec (num_nodes);
  const size_t num_edges = mat2vec.vec_size();
  matrix_type data (num_edges, filenames.size());
  {
    ProgressBar progress ("Loading input connectome data", filenames.size());
    for (size_t subject = 0; subject < filenames.size(); subject++) {

      const std::string& path (filenames[subject]);
      MR::Connectome::matrix_type subject_data;
      try {
        subject_data = load_matrix (path);
      } catch (Exception& e) {
        throw Exception (e, "Error loading connectome data for subject #" + str(subject) + " (file \"" + path + "\"");
      }

      try {
        MR::Connectome::to_upper (subject_data);
        if (size_t(subject_data.rows()) != num_nodes)
          throw Exception ("Connectome matrix is not the correct size (" + str(subject_data.rows()) + ", should be " + str(num_nodes) + ")");
      } catch (Exception& e) {
        throw Exception (e, "Connectome for subject #" + str(subject) + " (file \"" + path + "\") invalid");
      }

      for (size_t i = 0; i != num_edges; ++i)
        data(i, subject) = subject_data (mat2vec(i).first, mat2vec(i).second);

      ++progress;
    }
  }

  {
    ProgressBar progress ("outputting beta coefficients, effect size and standard deviation...", contrast.cols() + 3);

    const matrix_type betas = Math::Stats::GLM::solve_betas (data, design);
    for (size_t i = 0; i < size_t(contrast.cols()); ++i) {
      save_matrix (mat2vec.V2M (betas.col(i)), output_prefix + "_beta_" + str(i) + ".csv");
      ++progress;
    }

    const matrix_type abs_effects = Math::Stats::GLM::abs_effect_size (data, design, contrast);
    save_matrix (mat2vec.V2M (abs_effects.col(0)), output_prefix + "_abs_effect.csv");
    ++progress;

    const matrix_type std_effects = Math::Stats::GLM::std_effect_size (data, design, contrast);
    matrix_type first_std_effect = mat2vec.V2M (std_effects.col (0));
    for (MR::Connectome::node_t i = 0; i != num_nodes; ++i) {
      for (MR::Connectome::node_t j = 0; j != num_nodes; ++j) {
        if (!std::isfinite (first_std_effect (i, j)))
          first_std_effect (i, j) = 0.0;
      }
    }
    save_matrix (first_std_effect, output_prefix + "_std_effect.csv");
    ++progress;

    const matrix_type stdevs = Math::Stats::GLM::stdev (data, design);
    save_vector (stdevs.col(0), output_prefix + "_std_dev.csv");
  }

  Math::Stats::GLMTTest glm_ttest (data, design, contrast);

  // If performing non-stationarity adjustment we need to pre-compute the empirical statistic
  vector_type empirical_statistic;
  if (do_nonstationary_adjustment) {
    if (permutations_nonstationary.size()) {
      Stats::PermTest::PermutationStack perm_stack (permutations_nonstationary, "precomputing empirical statistic for non-stationarity adjustment...");
      Stats::PermTest::precompute_empirical_stat (glm_ttest, enhancer, perm_stack, empirical_statistic);
    } else {
      Stats::PermTest::PermutationStack perm_stack (nperms_nonstationary, design.rows(), "precomputing empirical statistic for non-stationarity adjustment...", true);
      Stats::PermTest::precompute_empirical_stat (glm_ttest, enhancer, perm_stack, empirical_statistic);
    }
    save_matrix (mat2vec.V2M (empirical_statistic), output_prefix + "_empirical.csv");
  }

  // Precompute default statistic and enhanced statistic
  vector_type tvalue_output   (num_edges);
  vector_type enhanced_output (num_edges);

  Stats::PermTest::precompute_default_permutation (glm_ttest, enhancer, empirical_statistic, enhanced_output, std::shared_ptr<vector_type>(), tvalue_output);

  save_matrix (mat2vec.V2M (tvalue_output),   output_prefix + "_tvalue.csv");
  save_matrix (mat2vec.V2M (enhanced_output), output_prefix + "_enhanced.csv");

  // Perform permutation testing
  if (!get_options ("notest").size()) {

    // FIXME Getting NANs in the null distribution
    // Check: was result of pre-nulled subject data
    vector_type null_distribution (num_perms);
    vector_type uncorrected_pvalues (num_edges);

    if (permutations.size()) {
      Stats::PermTest::run_permutations (permutations, glm_ttest, enhancer, empirical_statistic,
                                         enhanced_output, std::shared_ptr<vector_type>(),
                                         null_distribution, std::shared_ptr<vector_type>(),
                                         uncorrected_pvalues, std::shared_ptr<vector_type>());
    } else {
      Stats::PermTest::run_permutations (num_perms, glm_ttest, enhancer, empirical_statistic,
                                         enhanced_output, std::shared_ptr<vector_type>(),
                                         null_distribution, std::shared_ptr<vector_type>(),
                                         uncorrected_pvalues, std::shared_ptr<vector_type>());
    }

    save_vector (null_distribution, output_prefix + "_null_dist.txt");
    vector_type pvalue_output (num_edges);
    Math::Stats::Permutation::statistic2pvalue (null_distribution, enhanced_output, pvalue_output);
    save_matrix (mat2vec.V2M (pvalue_output),       output_prefix + "_fwe_pvalue.csv");
    save_matrix (mat2vec.V2M (uncorrected_pvalues), output_prefix + "_uncorrected_pvalue.csv");

  }

}

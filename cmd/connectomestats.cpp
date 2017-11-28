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
#include "math/stats/shuffle.h"
#include "math/stats/typedefs.h"

#include "connectome/enhance.h"
#include "connectome/mat2vec.h"

#include "stats/permtest.h"


using namespace MR;
using namespace App;
using namespace MR::Math::Stats;
using namespace MR::Math::Stats::GLM;

using Math::Stats::matrix_type;
using Math::Stats::vector_type;


const char* algorithms[] = { "nbs", "nbse", "none", nullptr };



// TODO Eventually these will move to some kind of TFCE header
#define TFCE_DH_DEFAULT 0.1
#define TFCE_E_DEFAULT 0.4
#define TFCE_H_DEFAULT 3.0



void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Connectome group-wise statistics at the edge level using non-parametric permutation testing";

  DESCRIPTION
      + Math::Stats::GLM::column_ones_description;


  ARGUMENTS
  + Argument ("input", "a text file listing the file names of the input connectomes").type_file_in ()

  + Argument ("algorithm", "the algorithm to use in network-based clustering/enhancement. "
                           "Options are: " + join(algorithms, ", ")).type_choice (algorithms)

  + Argument ("design", "the design matrix").type_file_in ()

  + Argument ("contrast", "the contrast vector, specified as a single row of weights").type_file_in ()

  + Argument ("output", "the filename prefix for all output.").type_text();


  OPTIONS

  + Math::Stats::shuffle_options (true)

  // TODO OptionGroup these, and provide a generic loader function
  + Stats::TFCE::Options (TFCE_DH_DEFAULT, TFCE_E_DEFAULT, TFCE_H_DEFAULT)

  + OptionGroup ("Additional options for connectomestats")

  + Option ("threshold", "the t-statistic value to use in threshold-based clustering algorithms")
  + Argument ("value").type_float (0.0)

  // TODO Generalise this across commands
  + Option ("column", "add a column to the design matrix corresponding to subject edge-wise values "
                      "(the contrast vector length must include columns for these additions)").allow_multiple()
  + Argument ("path").type_file_in();


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




void load_tfce_parameters (Stats::TFCE::Wrapper& enhancer)
{
  const default_type dH = get_option_value ("tfce_dh", TFCE_DH_DEFAULT);
  const default_type E  = get_option_value ("tfce_e",  TFCE_E_DEFAULT);
  const default_type H  = get_option_value ("tfce_h",  TFCE_H_DEFAULT);
  enhancer.set_tfce_parameters (dH, E, H);
}



// Define data importer class that willl obtain connectome data for a
//   specific subject based on the string path to the image file for
//   that subject
class SubjectConnectomeImport : public SubjectDataImportBase
{ MEMALIGN(SubjectConnectomeImport)
  public:
    SubjectConnectomeImport (const std::string& path) :
        SubjectDataImportBase (path)
    {
      auto M = load_matrix (path);
      Connectome::check (M);
      if (Connectome::is_directed (M))
        throw Exception ("Connectome from file \"" + Path::basename (path) + "\" is a directed matrix");
      Connectome::to_upper (M);
      Connectome::Mat2Vec mat2vec (M.rows());
      mat2vec.M2V (M, data);
    }

    void operator() (matrix_type::ColXpr column) const override
    {
      assert (column.rows() == data.size());
      column = data;
    }

    default_type operator[] (const size_t index) const override
    {
      assert (index < size_t(data.size()));
      return (data[index]);
    }

    size_t size() const override { return data.size(); }

  private:
    vector_type data;

};



void run()
{

  // Read file names and check files exist
  CohortDataImport importer;
  importer.initialise<SubjectConnectomeImport> (argument[0]);
  CONSOLE ("Number of subjects: " + str(importer.size()));
  const size_t num_edges = importer[0]->size();

  for (size_t i = 1; i < importer.size(); ++i) {
    if (importer[i]->size() != importer[0]->size())
      throw Exception ("Size of connectome for subject " + str(i) + " (file \"" + importer[i]->name() + "\" does not match that of first subject");
  }

  // TODO Could determine this from the vector length with the right equation
  const MR::Connectome::matrix_type example_connectome = load_matrix (importer[0]->name());
  const MR::Connectome::node_t num_nodes = example_connectome.rows();
  Connectome::Mat2Vec mat2vec (num_nodes);

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

  const bool do_nonstationary_adjustment = get_options ("nonstationary").size();

  // Load design matrix
  const matrix_type design = load_matrix (argument[2]);
  if (size_t(design.rows()) != importer.size())
    throw Exception ("number of subjects (" + str(importer.size()) + ") does not match number of rows in design matrix (" + str(design.rows()) + ")");

  // Load contrast matrix
  // TODO Eventually this should be functionalised, and include F-tests
  // TODO Eventually will want ability to disable t-test output, and output F-tests only
  vector<Contrast> contrasts;
  {
    const matrix_type contrast_matrix = load_matrix (argument[3]);
    for (ssize_t row = 0; row != contrast_matrix.rows(); ++row)
      contrasts.emplace_back (Contrast (contrast_matrix.row (row)));
  }
  const size_t num_contrasts = contrasts.size();

  // Before validating the contrast matrix, we first need to see if there are any
  //   additional design matrix columns coming from fixel-wise subject data
  vector<CohortDataImport> extra_columns;
  bool nans_in_columns = false;
  auto opt = get_options ("column");
  for (size_t i = 0; i != opt.size(); ++i) {
    extra_columns.push_back (CohortDataImport());
    extra_columns[i].initialise<SubjectConnectomeImport> (opt[i][0]);
    if (!extra_columns[i].allFinite())
      nans_in_columns = true;
  }
  if (extra_columns.size()) {
    CONSOLE ("number of element-wise design matrix columns: " + str(extra_columns.size()));
    if (nans_in_columns)
      INFO ("Non-finite values detected in element-wise design matrix columns; individual rows will be removed from edge-wise design matrices accordingly");
  }

  // Now we can check the contrast matrix
  const ssize_t num_factors = design.cols() + extra_columns.size();
  if (contrasts[0].cols() != num_factors)
    // TODO Re-word this error message
    throw Exception ("the number of columns per contrast (" + str(contrasts[0].cols()) + ")"
                     + " does not equal the number of columns in the design matrix (" + str(design.cols()) + ")"
                     + (extra_columns.size() ? " (taking into account the " + str(extra_columns.size()) + " uses of -column)" : ""));


  const std::string output_prefix = argument[4];

  // Load input data
  // For compatibility with existing statistics code, symmetric matrix data is adjusted
  //   into vector form - one row per edge in the symmetric connectome. This has already
  //   been performed when the CohortDataImport class is initialised.
  matrix_type data (num_edges, importer.size());
  {
    ProgressBar progress ("Agglomerating input connectome data", importer.size());
    for (size_t subject = 0; subject < importer.size(); subject++) {
      (*importer[subject]) (data.col (subject));
      ++progress;
    }
  }
  const bool nans_in_data = data.allFinite();

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
    matrix_type betas (num_factors, num_edges);
    // TODO Pretty sure these are transposed with respect to what I'd prefer them to be
    matrix_type abs_effect_size (num_contrasts, num_edges), std_effect_size (num_contrasts, num_edges);
    vector_type stdev (num_edges);

    if (extra_columns.size()) {

      // For each variable of interest (e.g. beta coefficients, effect size etc.) need to:
      //   Construct the output data vector, with size = num_edges
      //   For each edge:
      //     Use glm_test to obtain the design matrix for the default permutation for that edge
      //     Use the relevant Math::Stats::GLM function to get the value of interest for just that edge
      //       (will still however need to come out as a matrix_type)
      //     Write that value to data vector
      //   Finally, write results to connectome files
      class Source
      { NOMEMALIGN
        public:
          Source (const size_t num_edges) :
              num_edges (num_edges),
              counter (0),
              progress (new ProgressBar ("calculating basic properties of of default permutation", num_edges)) { }

          bool operator() (size_t& edge_index)
          {
            edge_index = counter++;
            if (counter >= num_edges) {
              progress.reset();
              return false;
            }
            assert (progress);
            ++(*progress);
            return true;
          }

        private:
          const size_t num_edges;
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

          bool operator() (const size_t& edge_index)
          {
            const matrix_type data_edge = data.row (edge_index);
            const matrix_type design_edge = dynamic_cast<const GLM::TestVariable* const>(glm_test.get())->default_design (edge_index);
            Math::Stats::GLM::all_stats (data_edge, design_edge, contrasts,
                                         local_betas, local_abs_effect_size, local_std_effect_size, local_stdev);
            global_betas.col (edge_index) = local_betas;
            global_abs_effect_size.col(edge_index) = local_abs_effect_size.col(0);
            global_std_effect_size.col(edge_index) = local_std_effect_size.col(0);
            global_stdev[edge_index] = local_stdev[0];
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

      Source source (num_edges);
      Functor functor (data, glm_test, contrasts,
                       betas, abs_effect_size, std_effect_size, stdev);
      Thread::run_queue (source, Thread::batch (size_t()), Thread::multi (functor));


    } else {

      ProgressBar progress ("calculating basic properties of default permutation");
      Math::Stats::GLM::all_stats (data, design, contrasts,
                                   betas, abs_effect_size, std_effect_size, stdev);
    }

    // TODO Contrasts should be somehow named, in order to differentiate between t-tests and F-tests

    ProgressBar progress ("outputting beta coefficients, effect size and standard deviation", num_factors + (2 * num_contrasts) + 1);
    for (ssize_t i = 0; i != num_factors; ++i) {
      save_matrix (mat2vec.V2M (betas.row(i)), "beta" + str(i) + ".csv");
      ++progress;
    }
    for (size_t i = 0; i != num_contrasts; ++i) {
      save_matrix (mat2vec.V2M (abs_effect_size.row(i)), "abs_effect" + postfix(i) + ".csv"); ++progress;
      save_matrix (mat2vec.V2M (std_effect_size.row(i)), "std_effect" + postfix(i) + ".csv"); ++progress;
    }
    save_matrix (mat2vec.V2M (stdev), "std_dev.csv");

  }


  // If performing non-stationarity adjustment we need to pre-compute the empirical statistic
  matrix_type empirical_statistic;
  if (do_nonstationary_adjustment) {
    Stats::PermTest::precompute_empirical_stat (glm_test, enhancer, empirical_statistic);
    for (size_t i = 0; i != num_contrasts; ++i)
      save_matrix (mat2vec.V2M (empirical_statistic.row(i)), output_prefix + "_empirical" + postfix(i) + ".csv");
  }

  // Precompute default statistic and enhanced statistic
  matrix_type tvalue_output   (num_contrasts, num_edges);
  matrix_type enhanced_output (num_contrasts, num_edges);

  Stats::PermTest::precompute_default_permutation (glm_test, enhancer, empirical_statistic, enhanced_output, tvalue_output);

  for (size_t i = 0; i != num_contrasts; ++i) {
    save_matrix (mat2vec.V2M (tvalue_output.row(i)),   output_prefix + "_tvalue" + postfix(i) + ".csv");
    save_matrix (mat2vec.V2M (enhanced_output.row(i)), output_prefix + "_enhanced" + postfix(i) + ".csv");
  }

  // Perform permutation testing
  if (!get_options ("notest").size()) {

    matrix_type null_distribution, uncorrected_pvalues;

    Stats::PermTest::run_permutations (glm_test, enhancer, empirical_statistic,
                                       enhanced_output, null_distribution, uncorrected_pvalues);

    for (size_t i = 0; i != num_contrasts; ++i)
      save_vector (null_distribution.row(i), output_prefix + "_null_dist" + postfix(i) + ".txt");

    matrix_type pvalue_output (num_contrasts, num_edges);
    Math::Stats::statistic2pvalue (null_distribution, enhanced_output, pvalue_output);
    for (size_t i = 0; i != num_contrasts; ++i) {
      save_matrix (mat2vec.V2M (pvalue_output.row(i)),       output_prefix + "_fwe_pvalue" + postfix(i) + ".csv");
      save_matrix (mat2vec.V2M (uncorrected_pvalues.row(i)), output_prefix + "_uncorrected_pvalue" + postfix(i) + ".csv");
    }

  }

}

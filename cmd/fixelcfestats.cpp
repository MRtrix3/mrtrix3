/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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
#include "fixel/keys.h"
#include "fixel/loop.h"
#include "fixel/types.h"
#include "fixel/filter/smooth.h"
#include "math/stats/fwe.h"
#include "math/stats/glm.h"
#include "math/stats/import.h"
#include "math/stats/shuffle.h"
#include "math/stats/typedefs.h"
#include "stats/cfe.h"
#include "stats/enhance.h"
#include "stats/permtest.h"


using namespace MR;
using namespace App;

using Fixel::index_type;
using Math::Stats::matrix_type;
using Math::Stats::value_type;
using Math::Stats::vector_type;
using Stats::PermTest::count_matrix_type;

#define DEFAULT_ANGLE_THRESHOLD 45.0
#define DEFAULT_CONNECTIVITY_THRESHOLD 0.01
#define DEFAULT_SMOOTHING_FWHM 10.0

#define DEFAULT_CFE_DH 0.1
#define DEFAULT_CFE_E 2.0
#define DEFAULT_CFE_H 3.0
#define DEFAULT_CFE_C 0.5
#define DEFAULT_EMPIRICAL_SKEW 1.0 // TODO Update from experience

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Fixel-based analysis using connectivity-based fixel enhancement and non-parametric permutation testing";

  DESCRIPTION
  + "Unlike previous versions of this command, where a whole-brain tractogram file would be provided as input "
    "in order to generate the fixel-fixel connectivity matrix and smooth fixel data, this version expects to be "
    "provided with the directory path to a pre-calculated fixel-fixel connectivity matrix (likely generated using "
    "the MRtrix3 command fixelconnectivity), and for the input fixel data to have already been smoothed (likely "
    "using the MRtrix3 command fixelfilter)."

  + "Note that if the -mask option is used, the output fixel directory will still contain the same set of fixels as that "
    "present in the input fixel template, in order to retain fixel correspondence. However a consequence of this is that "
    "all fixels in the template will be initialy visible when the output fixel directory is loaded in mrview. Those fixels "
    "outside the processing mask will immediately disappear from view as soon as any data-file-based fixel colouring or "
    "thresholding is applied."

  + Math::Stats::GLM::column_ones_description;

  REFERENCES
  + "Raffelt, D.; Smith, RE.; Ridgway, GR.; Tournier, JD.; Vaughan, DN.; Rose, S.; Henderson, R.; Connelly, A. " // Internal
    "Connectivity-based fixel enhancement: Whole-brain statistical analysis of diffusion MRI measures in the presence of crossing fibres."
    "Neuroimage, 2015, 15(117):40-55"

  + "* If not using the -cfe_legacy option: \n"
    "Smith, RE.; Dimond, D; Vaughan, D.; Parker, D.; Dhollander, T.; Jackson, G.; Connelly, A. "
    "Intrinsic non-stationarity correction for Fixel-Based Analysis. "
    "In Proc OHBM 2019 M789"

  + "* If using the -nonstationary option: \n"
    "Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. "
    "Adjusting the effect of nonstationarity in cluster-based and TFCE inference. "
    "NeuroImage, 2011, 54(3), 2006-19";

  ARGUMENTS
  + Argument ("in_fixel_directory", "the fixel directory containing the data files for each subject (after obtaining fixel correspondence").type_directory_in()

  + Argument ("subjects", "a text file listing the subject identifiers (one per line). This should correspond with the filenames "
                          "in the fixel directory (including the file extension), and be listed in the same order as the rows of the design matrix.").type_image_in ()

  + Argument ("design", "the design matrix").type_file_in ()

  + Argument ("contrast", "the contrast matrix, specified as rows of weights").type_file_in ()

  // .type_various() rather than .type_directory_in() to catch people trying to
  //   pass a track file, and give a more informative error message
  + Argument ("connectivity", "the fixel-fixel connectivity matrix").type_various ()

  + Argument ("out_fixel_directory", "the output directory where results will be saved. Will be created if it does not exist").type_text();


  OPTIONS

  + Option ("mask", "provide a fixel data file containing a mask of those fixels to be used during processing")
  + Argument ("file").type_image_in()

  + Math::Stats::shuffle_options (true, DEFAULT_EMPIRICAL_SKEW)

  + OptionGroup ("Parameters for the Connectivity-based Fixel Enhancement algorithm")

  + Option ("cfe_dh", "the height increment used in the cfe integration (default: " + str(DEFAULT_CFE_DH, 2) + ")")
  + Argument ("value").type_float (0.001, 1.0)

  + Option ("cfe_e", "cfe extent exponent (default: " + str(DEFAULT_CFE_E, 2) + ")")
  + Argument ("value").type_float (0.0, 100.0)

  + Option ("cfe_h", "cfe height exponent (default: " + str(DEFAULT_CFE_H, 2) + ")")
  + Argument ("value").type_float (0.0, 100.0)

  + Option ("cfe_c", "cfe connectivity exponent (default: " + str(DEFAULT_CFE_C, 2) + ")")
  + Argument ("value").type_float (0.0, 100.0)

  + Option ("cfe_legacy", "use the legacy (non-normalised) form of the cfe equation")

  + Math::Stats::GLM::glm_options ("fixel");

}



template <class VectorType>
void write_fixel_output (const std::string& filename,
                         const VectorType& data,
                         Image<bool>& mask,
                         const Header& header)
{
  auto output = Image<float>::create (filename, header);
  for (auto l = Loop(0) (output, mask); l; ++l)
    output.value() = mask.value() ? data[output.index(0)] : NaN;
}



// Define data importer class that will obtain fixel data for a
//   specific subject based on the string path to the image file for
//   that subject
class SubjectFixelImport : public Math::Stats::SubjectDataImportBase
{ MEMALIGN(SubjectFixelImport)
  public:
    SubjectFixelImport (const std::string& path) :
        Math::Stats::SubjectDataImportBase (path),
        H (Header::open (path)),
        data (H.get_image<float>())
    {
      for (size_t axis = 1; axis < data.ndim(); ++axis) {
        if (data.size(axis) > 1)
          throw Exception ("Image file \"" + path + "\" does not contain fixel data (wrong dimensions)");
      }
    }

    void operator() (matrix_type::RowXpr row) const override
    {
      Image<float> temp (data); // For thread-safety
      for (temp.index(0) = 0; temp.index(0) != temp.size(0); ++temp.index(0))
        row [temp.index(0)] = temp.value();
    }

    default_type operator[] (const size_t index) const override
    {
      Image<float> temp (data); // For thread-safety
      temp.index(0) = index;
      assert (!is_out_of_bounds (temp));
      return default_type(temp.value());
    }

    size_t size() const override { return data.size(0); }

    const Header& header() const { return H; }



  private:
    Header H;
    Image<float> data; // May be mapped input file, or scratch smoothed data

};




void run()
{
  if (Path::has_suffix (argument[4], ".tck"))
    throw Exception ("This version of fixelcfestats requires as input not a track file, but a "
                     "pre-calculated fixel-fixel connectivity matrix; in addition, input fixel "
                     "data must be pre-smoothed. Please check command / pipeline documentation "
                     "specific to this software version.");

  const value_type cfe_dh = get_option_value ("cfe_dh", DEFAULT_CFE_DH);
  const value_type cfe_h = get_option_value ("cfe_h", DEFAULT_CFE_H);
  const value_type cfe_e = get_option_value ("cfe_e", DEFAULT_CFE_E);
  const value_type cfe_c = get_option_value ("cfe_c", DEFAULT_CFE_C);
  const bool cfe_legacy = get_options ("cfe_legacy").size();

  const bool do_nonstationarity_adjustment = get_options ("nonstationarity").size();
  const default_type empirical_skew = get_option_value ("skew_nonstationarity", DEFAULT_EMPIRICAL_SKEW);

  const std::string input_fixel_directory = argument[0];
  Header index_header = Fixel::find_index_header (input_fixel_directory);
  auto index_image = index_header.get_image<index_type>();

  const index_type num_fixels = Fixel::get_number_of_fixels (index_header);
  CONSOLE ("Number of fixels in template: " + str(num_fixels));

  Image<bool> mask;
  auto opt = get_options ("mask");
  index_type mask_fixels = 0;
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
    Fixel::check_data_file (mask);
    if (!Fixel::fixels_match (index_header, mask))
      throw Exception ("Mask image provided using -mask option does not match fixel template");
    for (auto l = Loop(0) (mask); l; ++l) {
      if (mask.value())
        ++mask_fixels;
    }
    CONSOLE ("Number of fixels in mask: " + str(mask_fixels));
  } else {
    Header fixel_mask_header = Fixel::data_header_from_index (index_header);
    fixel_mask_header.datatype() = DataType::Bit;
    mask = Image<bool>::scratch (fixel_mask_header, "true-filled scratch fixel mask");
    for (auto l = Loop(0) (mask); l; ++l)
      mask.value() = true;
    mask_fixels = num_fixels;
  }

  // Read file names and check files exist
  // Preference for finding files relative to input template fixel directory
  Math::Stats::CohortDataImport importer;
  importer.initialise<SubjectFixelImport> (argument[1], input_fixel_directory);
  for (size_t i = 0; i != importer.size(); ++i) {
    if (!Fixel::fixels_match (index_header, dynamic_cast<SubjectFixelImport*>(importer[i].get())->header()))
      throw Exception ("Fixel data file \"" + importer[i]->name() + "\" does not match template fixel image");
  }
  CONSOLE ("Number of inputs: " + str(importer.size()));

  // Load design matrix:
  const matrix_type design = load_matrix (argument[2]);
  if (design.rows() != (ssize_t)importer.size())
    throw Exception ("Number of input files does not match number of rows in design matrix");

  // Before validating the contrast matrix, we first need to see if there are any
  //   additional design matrix columns coming from fixel-wise subject data
  vector<Math::Stats::CohortDataImport> extra_columns;
  bool nans_in_columns = false;
  opt = get_options ("column");
  for (size_t i = 0; i != opt.size(); ++i) {
    extra_columns.push_back (Math::Stats::CohortDataImport());
    extra_columns[i].initialise<SubjectFixelImport> (opt[i][0]);
    // Check for non-finite values in mask fixels only
    // Can't use generic allFinite() function; need to populate matrix data
    if (!nans_in_columns) {
      matrix_type column_data (importer.size(), num_fixels);
      for (size_t j = 0; j != importer.size(); ++j)
        (*extra_columns[i][j]) (column_data.row (j));
      if (mask_fixels == num_fixels) {
        nans_in_columns = !column_data.allFinite();
      } else {
        for (auto l = Loop(0) (mask); l; ++l) {
          if (mask.value() && !column_data.col (mask.index(0)).allFinite()) {
            nans_in_columns = true;
            break;
          }
        }
      }
    }
  }
  const ssize_t num_factors = design.cols() + extra_columns.size();
  CONSOLE ("Number of factors: " + str(num_factors));
  if (extra_columns.size()) {
    CONSOLE ("Number of element-wise design matrix columns: " + str(extra_columns.size()));
    if (nans_in_columns)
      CONSOLE ("Non-finite values detected in element-wise design matrix columns; individual rows will be removed from fixel-wise design matrices accordingly");
  }
  Math::Stats::GLM::check_design (design, extra_columns.size());

  // Load variance groups
  auto variance_groups = Math::Stats::GLM::load_variance_groups (design.rows());
  const size_t num_vgs = variance_groups.size() ? variance_groups.maxCoeff()+1 : 1;
  if (num_vgs > 1)
    CONSOLE ("Number of variance groups: " + str(num_vgs));

  // Load hypotheses
  const vector<Math::Stats::GLM::Hypothesis> hypotheses = Math::Stats::GLM::load_hypotheses (argument[3]);
  const size_t num_hypotheses = hypotheses.size();
  if (hypotheses[0].cols() != num_factors)
    throw Exception ("The number of columns in the contrast matrix (" + str(hypotheses[0].cols()) + ")"
                     + (extra_columns.size() ? " (in addition to the " + str(extra_columns.size()) + " uses of -column)" : "")
                     + " does not equal the number of columns in the design matrix (" + str(design.cols()) + ")");
  CONSOLE ("Number of hypotheses: " + str(num_hypotheses));

  // Load fixel-fixel connectivity matrix
  Fixel::Matrix::Reader matrix (argument[4], mask);

  const std::string output_fixel_directory = argument[5];
  Fixel::copy_index_and_directions_file (input_fixel_directory, output_fixel_directory);

  // Do we still want to check whether or not there are any disconnected fixels?
  // With the current derivation, disconnected fixels will not possess any self-connectivity,
  //   and therefore will receive a value of 0 according to the CFE expression. So these
  //   should actually not interfere at all with the intrinsic normalisation / empirical
  //   non-stationarity correction.
  // It may nevertheless be informative to know whether there are fixels that are included
  //   in the mask but don't have any connectivity; warn the user that these will be
  //   zeroed by the enhancement process
  index_type num_unconnected_fixels = 0;
  for (index_type f = 0; f != num_fixels; ++f) {
    mask.index (0) = f;
    if (mask.value() && !matrix.size (f))
      ++num_unconnected_fixels;
  }
  if (num_unconnected_fixels) {
    WARN ("A total of " + str(num_unconnected_fixels) + " fixels " +
          (mask_fixels == num_fixels ? "" : "in the provided mask ") +
          "do not possess any streamlines-based connectivity; "
          "these will not be enhanced by CFE, and hence cannot be "
          "tested for statistical significance");
  }

  Header output_header (dynamic_cast<SubjectFixelImport*>(importer[0].get())->header());
  output_header.keyval()["cfe_dh"] = str(cfe_dh);
  output_header.keyval()["cfe_e"] = str(cfe_e);
  output_header.keyval()["cfe_h"] = str(cfe_h);
  output_header.keyval()["cfe_c"] = str(cfe_c);
  output_header.keyval()["cfe_legacy"] = str(cfe_legacy);

  matrix_type data = matrix_type::Zero (importer.size(), num_fixels);
  {
    ProgressBar progress (std::string ("Loading fixel data (no smoothing)"), importer.size());
    for (size_t subject = 0; subject != importer.size(); subject++) {
      (*importer[subject]) (data.row (subject));
      progress++;
    }
  }
  // Detect non-finite values in mask fixels only; NaN-fill other fixels
  bool nans_in_data = false;
  for (auto l = Loop(0) (mask); l; ++l) {
    if (mask.value()) {
      if (!data.col (mask.index(0)).allFinite())
        nans_in_data = true;
    } else {
      data.col (mask.index (0)).fill (NaN);
    }
  }
  if (nans_in_data) {
    CONSOLE ("Non-finite values present in data; rows will be removed from fixel-wise design matrices accordingly");
    if (!extra_columns.size()) {
      CONSOLE ("(Note that this will result in slower execution than if such values were not present)");
    }
  }

  // Only add contrast matrix row number to image outputs if there's more than one hypothesis
  auto postfix = [&] (const size_t i) -> std::string { return (num_hypotheses > 1) ? ("_" + hypotheses[i].name()) : ""; };

  {
    matrix_type betas (num_factors, num_fixels);
    matrix_type abs_effect_size (num_fixels, num_hypotheses);
    matrix_type std_effect_size (num_fixels, num_hypotheses);
    matrix_type stdev (num_vgs, num_fixels);
    vector_type cond (num_fixels);

    Math::Stats::GLM::all_stats (data, design, extra_columns, hypotheses, variance_groups,
                                 cond, betas, abs_effect_size, std_effect_size, stdev);

    ProgressBar progress ("Outputting beta coefficients, effect size and standard deviation", num_factors + (2 * num_hypotheses) + num_vgs + (nans_in_data || extra_columns.size() ? 1 : 0));

    for (ssize_t i = 0; i != num_factors; ++i) {
      write_fixel_output (Path::join (output_fixel_directory, "beta" + str(i) + ".mif"), betas.row(i), mask, output_header);
      ++progress;
    }
    for (size_t i = 0; i != num_hypotheses; ++i) {
      if (!hypotheses[i].is_F()) {
        write_fixel_output (Path::join (output_fixel_directory, "abs_effect" + postfix(i) + ".mif"), abs_effect_size.col(i), mask, output_header);
        ++progress;
        if (num_vgs == 1)
          write_fixel_output (Path::join (output_fixel_directory, "std_effect" + postfix(i) + ".mif"), std_effect_size.col(i), mask, output_header);
      } else {
        ++progress;
      }
      ++progress;
    }
    if (nans_in_data || extra_columns.size()) {
      write_fixel_output (Path::join (output_fixel_directory, "cond.mif"), cond, mask, output_header);
      ++progress;
    }
    if (num_vgs == 1) {
      write_fixel_output (Path::join (output_fixel_directory, "std_dev.mif"), stdev.row (0), mask, output_header);
    } else {
      for (size_t i = 0; i != num_vgs; ++i) {
        write_fixel_output (Path::join (output_fixel_directory, "std_dev" + str(i) + ".mif"), stdev.row (i), mask, output_header);
        ++progress;
      }
    }
  }

  // Construct the class for performing the initial statistical tests
  std::shared_ptr<Math::Stats::GLM::TestBase> glm_test;
  if (extra_columns.size() || nans_in_data) {
    if (variance_groups.size())
      glm_test.reset (new Math::Stats::GLM::TestVariableHeteroscedastic (extra_columns, data, design, hypotheses, variance_groups, nans_in_data, nans_in_columns));
    else
      glm_test.reset (new Math::Stats::GLM::TestVariableHomoscedastic (extra_columns, data, design, hypotheses, nans_in_data, nans_in_columns));
  } else {
    if (variance_groups.size())
      glm_test.reset (new Math::Stats::GLM::TestFixedHeteroscedastic (data, design, hypotheses, variance_groups));
    else
      glm_test.reset (new Math::Stats::GLM::TestFixedHomoscedastic (data, design, hypotheses));
  }

  // Construct the class for performing fixel-based statistical enhancement
  std::shared_ptr<Stats::EnhancerBase> cfe_integrator (new Stats::CFE (matrix, cfe_dh, cfe_e, cfe_h, cfe_c, !cfe_legacy));

  // If performing non-stationarity adjustment we need to pre-compute the empirical CFE statistic
  matrix_type empirical_cfe_statistic;
  if (do_nonstationarity_adjustment) {
    Stats::PermTest::precompute_empirical_stat (glm_test, cfe_integrator, empirical_skew, empirical_cfe_statistic);
    output_header.keyval()["nonstationarity_adjustment"] = str(true);
    for (size_t i = 0; i != num_hypotheses; ++i)
      write_fixel_output (Path::join (output_fixel_directory, "cfe_empirical" + postfix(i) + ".mif"), empirical_cfe_statistic.col(i), mask, output_header);
  } else {
    output_header.keyval()["nonstationarity_adjustment"] = str(false);
  }

  // Precompute default statistic and CFE statistic
  matrix_type default_statistic, default_zstat, default_enhanced;
  Stats::PermTest::precompute_default_permutation (glm_test, cfe_integrator, empirical_cfe_statistic, default_statistic, default_zstat, default_enhanced);
  for (size_t i = 0; i != num_hypotheses; ++i) {
    write_fixel_output (Path::join (output_fixel_directory, (hypotheses[i].is_F() ? std::string("F") : std::string("t")) + "value" + postfix(i) + ".mif"), default_statistic.col(i), mask, output_header);
    write_fixel_output (Path::join (output_fixel_directory, "Zstat" + postfix(i) + ".mif"), default_zstat.col(i), mask, output_header);
    write_fixel_output (Path::join (output_fixel_directory, "cfe" + postfix(i) + ".mif"), default_enhanced.col(i), mask, output_header);
  }

  // Perform permutation testing
  if (!get_options ("notest").size()) {

    const bool fwe_strong = get_options("strong").size();
    if (fwe_strong && num_hypotheses == 1) {
      WARN("Option -strong has no effect when testing a single hypothesis only");
    }

    matrix_type null_distribution, uncorrected_pvalues;
    count_matrix_type null_contributions;
    Stats::PermTest::run_permutations (glm_test, cfe_integrator, empirical_cfe_statistic, default_enhanced, fwe_strong,
                                       null_distribution, null_contributions, uncorrected_pvalues);

    ProgressBar progress ("Outputting final results", (fwe_strong ? 1 : num_hypotheses) + 1 + 3*num_hypotheses);

    if (fwe_strong) {
      save_vector (null_distribution.col(0), Path::join (output_fixel_directory, "null_dist.txt"));
      ++progress;
    } else {
      for (size_t i = 0; i != num_hypotheses; ++i) {
        save_vector (null_distribution.col(i), Path::join (output_fixel_directory, "null_dist" + postfix(i) + ".txt"));
        ++progress;
      }
    }

    const matrix_type pvalue_output = MR::Math::Stats::fwe_pvalue (null_distribution, default_enhanced);
    ++progress;
    for (size_t i = 0; i != num_hypotheses; ++i) {
      write_fixel_output (Path::join (output_fixel_directory, "fwe_1mpvalue" + postfix(i) + ".mif"), pvalue_output.col(i), mask, output_header);
      ++progress;
      write_fixel_output (Path::join (output_fixel_directory, "uncorrected_pvalue" + postfix(i) + ".mif"), uncorrected_pvalues.col(i), mask, output_header);
      ++progress;
      write_fixel_output (Path::join (output_fixel_directory, "null_contributions" + postfix(i) + ".mif"), null_contributions.col(i), mask, output_header);
      ++progress;
    }
  }
}

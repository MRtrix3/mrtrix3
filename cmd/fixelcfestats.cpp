/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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
  + Math::Stats::GLM::column_ones_description

  + "Note that if the -mask option is used, the output fixel directory will still contain the same set of fixels as that "
    "present in the input fixel template, in order to retain fixel correspondence. However a consequence of this is that "
    "all fixels in the template will be initialy visible when the output fixel directory is loaded in mrview. Those fixels "
    "outside the processing mask will immediately disappear from view as soon as any data-file-based fixel colouring or "
    "thresholding is applied.";

  REFERENCES
  + "Raffelt, D.; Smith, RE.; Ridgway, GR.; Tournier, JD.; Vaughan, DN.; Rose, S.; Henderson, R.; Connelly, A." // Internal
    "Connectivity-based fixel enhancement: Whole-brain statistical analysis of diffusion MRI measures in the presence of crossing fibres. \n"
    "Neuroimage, 2015, 15(117):40-55\n"

  + "* If using the -nonstationary option: \n"
    "Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. \n"
    "Adjusting the effect of nonstationarity in cluster-based and TFCE inference. \n"
    "NeuroImage, 2011, 54(3), 2006-19\n" ;

  ARGUMENTS
  + Argument ("in_fixel_directory", "the fixel directory containing the data files for each subject (after obtaining fixel correspondence").type_directory_in()

  + Argument ("subjects", "a text file listing the subject identifiers (one per line). This should correspond with the filenames "
                          "in the fixel directory (including the file extension), and be listed in the same order as the rows of the design matrix.").type_image_in ()

  + Argument ("design", "the design matrix").type_file_in ()

  + Argument ("contrast", "the contrast matrix, specified as rows of weights").type_file_in ()

  + Argument ("tracks", "the tracks used to determine fixel-fixel connectivity").type_tracks_in ()

  + Argument ("out_fixel_directory", "the output directory where results will be saved. Will be created if it does not exist").type_text();


  OPTIONS

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

  + Option ("cfe_norm", "use a normalised form of the cfe equation")

  + Math::Stats::GLM::glm_options ("fixel")

  + OptionGroup ("Additional options for fixelcfestats")

  + Option ("smooth", "smooth the fixel value along the fibre tracts using a Gaussian kernel with the supplied FWHM (default: " + str(DEFAULT_SMOOTHING_FWHM, 2) + "mm)")
  + Argument ("FWHM").type_float (0.0, 200.0)

  + Option ("connectivity", "a threshold to define the required fraction of shared connections to be included in the neighbourhood (default: " + str(DEFAULT_CONNECTIVITY_THRESHOLD, 2) + ")")
  + Argument ("threshold").type_float (0.0, 1.0)

  + Option ("angle", "the max angle threshold for assigning streamline tangents to fixels (Default: " + str(DEFAULT_ANGLE_THRESHOLD, 2) + " degrees)")
  + Argument ("value").type_float (0.0, 90.0)

  + Option ("mask", "provide a fixel data file containing a mask of those fixels to be used during processing")
  + Argument ("file").type_image_in();

}





// Global variabes that need to be set via run() but accessed by other functions / classes

// When a fixel mask is provided, fixel indices are remapped such that the
//   fixels that are within the mask appear contiguously in data matrices without gaps
Fixel::IndexRemapper index_remapper;





template <class VectorType>
void write_fixel_output (const std::string& filename,
                         const VectorType& data,
                         const Header& header)
{
  assert (size_t(header.size(0)) == index_remapper.num_external());
  auto output = Image<float>::create (filename, header);
  for (size_t f = 0; f != index_remapper.num_external(); ++f) {
    output.index(0) = f;
    output.value() = (index_remapper.e2i (f) == index_remapper.invalid) ? NaN : data[index_remapper.e2i (f)];
  }
}



// Define data importer class that will obtain fixel data for a
//   specific subject based on the string path to the image file for
//   that subject
class SubjectFixelImport : public Math::Stats::SubjectDataImportBase
{ MEMALIGN(SubjectFixelImport)
  public:
    SubjectFixelImport (const std::string& path) :
        Math::Stats::SubjectDataImportBase (path),
        H (Header::open (find_image (path))),
        data (H.get_image<float>())
    {
      for (size_t axis = 1; axis < data.ndim(); ++axis) {
        if (data.size(axis) > 1)
          throw Exception ("Image file \"" + path + "\" does not contain fixel data (wrong dimensions)");
      }
    }

    // TODO Consider an alternative function that receives as input both the remapper and the smoothing filter
    // If either is present / nontrivial, then change data to be scratch pre-converted data
    // This may even mean that the index remapper doesn't need to be global?

    void smooth (const Fixel::Filter::Smooth filter)
    {
      assert (!smoothed_data.valid());
      // If smoothing is being done inline, then the smoothing matrix takes into account
      //   fixel index remapping; that is, it's designed to operate on data for which
      //   the re-mapping has already been applied
      Image<float> temp (data); // For thread-safety
      Header H_remapped (H);
      H_remapped.size (0) = index_remapper.num_internal();
      auto scratch = Image<float>::scratch (H_remapped, "scratch index-converted data from \"" + data.name() + "\" prior to smoothing");
      for (temp.index(0) = 0; temp.index(0) != temp.size(0); ++temp.index(0)) {
        if (index_remapper.e2i (temp.index(0)) != index_remapper.invalid) {
          scratch.index (0) = index_remapper.e2i (temp.index(0));
          scratch.value() = temp.value();
        }
      }
      smoothed_data = Image<float>::scratch (H_remapped, "smoothed version of \"" + data.name() + "\"");
      filter (scratch, smoothed_data);
    }

    void operator() (matrix_type::RowXpr row) const override
    {
      if (smoothed_data.valid()) {
        Image<float> temp (smoothed_data); // For thread-safety
        // Smoothed data have already undergone index remapping
        for (auto l = Loop(0) (temp); l; ++l)
          row (temp.index (0)) = temp.value();
      } else {
        Image<float> temp (data); // For thread-safety
        // Straight import of data (but accounting for index remapping)
        for (temp.index(0) = 0; temp.index(0) != temp.size(0); ++temp.index(0)) {
          if (index_remapper.e2i (temp.index(0)) != index_remapper.invalid)
            row (index_remapper.e2i (temp.index(0))) = temp.value();
        }
      }
    }

    default_type operator[] (const size_t index) const override
    {
      assert (index < index_remapper.num_internal());
      // Note that if data were smoothed in the process of importing,
      //   this function will now access the smoothed data stored in scratch
      // However this function is only used for fixel-wise design matrix columns,
      //   which don't pass through the RowXpr functor
      // Also note that 'data' has not been index-remapped (it is the raw input
      //   image on disk), whereas smoothed_data has already been remapped
      if (smoothed_data.valid()) {
        Image<float> temp (smoothed_data); // For thread-safety
        temp.index (0) = index;
        assert (!is_out_of_bounds (temp));
        return default_type (temp.value());
      } else {
        Image<float> temp (data); // For thread-safety
        temp.index(0) = index_remapper.i2e (index);
        assert (!is_out_of_bounds (temp));
        return default_type(temp.value());
      }
    }

    size_t size() const override { return data.size(0); }

    const Header& header() const { return H; }


    static void set_fixel_directory (const std::string& s) { fixel_directory = s; }


  private:
    Header H;
    const Image<float> data;
    Image<float> smoothed_data;

    // Enable input image paths to be either absolute, relative to CWD, or
    //   relative to input fixel template directory
    std::string find_image (const std::string& path) const
    {
      const std::string cat_path = Path::join (fixel_directory, path);
      if (Path::is_file (cat_path))
        return cat_path;
      if (Path::is_file (path))
        return path;
      throw Exception ("Unable to find subject image \"" + path +
                       "\" either in input fixel diretory \"" + fixel_directory +
                       "\" or in current working directory");
      return "";
    }

    static std::string fixel_directory;

};

std::string SubjectFixelImport::fixel_directory;



void run()
{

  const value_type cfe_dh = get_option_value ("cfe_dh", DEFAULT_CFE_DH);
  const value_type cfe_h = get_option_value ("cfe_h", DEFAULT_CFE_H);
  const value_type cfe_e = get_option_value ("cfe_e", DEFAULT_CFE_E);
  const value_type cfe_c = get_option_value ("cfe_c", DEFAULT_CFE_C);
  const bool cfe_norm = get_options ("cfe_norm").size();
  const value_type smoothing_fwhm = get_option_value ("smooth", DEFAULT_SMOOTHING_FWHM) / 2.3548;
  const value_type connectivity_threshold = get_option_value ("connectivity", DEFAULT_CONNECTIVITY_THRESHOLD);
  const value_type angular_threshold = get_option_value ("angle", DEFAULT_ANGLE_THRESHOLD);

  const bool do_nonstationarity_adjustment = get_options ("nonstationarity").size();
  const default_type empirical_skew = get_option_value ("skew_nonstationarity", DEFAULT_EMPIRICAL_SKEW);

  const bool do_smoothing = smoothing_fwhm > value_type(0);

  const std::string input_fixel_directory = argument[0];
  SubjectFixelImport::set_fixel_directory (input_fixel_directory);
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
    index_remapper = Fixel::IndexRemapper (mask);
    mask_fixels = index_remapper.num_internal();
    CONSOLE ("Number of fixels in mask: " + str(mask_fixels));
  } else {
    Header fixel_mask_header = Fixel::data_header_from_index (index_header);
    fixel_mask_header.datatype() = DataType::Bit;
    mask = Image<bool>::scratch (fixel_mask_header, "true-filled scratch fixel mask");
    for (mask.index(0) = 0; mask.index(0) != num_fixels; ++mask.index(0))
      mask.value() = true;
    mask_fixels = num_fixels;
    index_remapper = Fixel::IndexRemapper (num_fixels);
  }



  const std::string output_fixel_directory = argument[5];
  Fixel::copy_index_and_directions_file (input_fixel_directory, output_fixel_directory);

  // Read file names and check files exist
  Math::Stats::CohortDataImport importer;
  importer.initialise<SubjectFixelImport> (argument[1]);
  for (size_t i = 0; i != importer.size(); ++i) {
    if (!Fixel::fixels_match (index_header, dynamic_cast<SubjectFixelImport*>(importer[i].get())->header()))
      throw Exception ("Fixel data file \"" + importer[i]->name() + "\" does not match template fixel image");
  }
  CONSOLE ("Number of subjects: " + str(importer.size()));

  // Load design matrix:
  const matrix_type design = load_matrix (argument[2]);
  if (design.rows() != (ssize_t)importer.size())
    throw Exception ("Number of input files does not match number of rows in design matrix");
  Math::Stats::GLM::check_design (design);

  // Load hypotheses
  const vector<Math::Stats::GLM::Hypothesis> hypotheses = Math::Stats::GLM::load_hypotheses (argument[3]);
  const size_t num_hypotheses = hypotheses.size();
  CONSOLE ("Number of hypotheses: " + str(num_hypotheses));

  // Before validating the contrast matrix, we first need to see if there are any
  //   additional design matrix columns coming from fixel-wise subject data
  vector<Math::Stats::CohortDataImport> extra_columns;
  bool nans_in_columns = false;
  opt = get_options ("column");
  for (size_t i = 0; i != opt.size(); ++i) {
    extra_columns.push_back (Math::Stats::CohortDataImport());
    extra_columns[i].initialise<SubjectFixelImport> (opt[i][0]);
    if (!extra_columns[i].allFinite())
      nans_in_columns = true;
  }
  if (extra_columns.size()) {
    CONSOLE ("Number of element-wise design matrix columns: " + str(extra_columns.size()));
    if (nans_in_columns)
      INFO ("Non-finite values detected in element-wise design matrix columns; individual rows will be removed from fixel-wise design matrices accordingly");
  }

  const ssize_t num_factors = design.cols() + extra_columns.size();
  CONSOLE ("Number of factors: " + str(num_factors));
  if (hypotheses[0].cols() != num_factors)
    throw Exception ("The number of columns in the contrast matrix (" + str(hypotheses[0].cols()) + ")"
                     + (extra_columns.size() ? " (in addition to the " + str(extra_columns.size()) + " uses of -column)" : "")
                     + " does not equal the number of columns in the design matrix (" + str(design.cols()) + ")");

  // Compute fixel-fixel connectivity
  // Here the fixel mask is used only to reduce the length of the list of
  //   fixels traversed by each streamline that must be processed, and to
  //   reduce the amount of RAM required by the command.
  auto connectivity_matrix = Fixel::Matrix::generate (argument[4],
                                                      index_image,
                                                      mask,
                                                      angular_threshold);

  // Normalise connectivity matrix and threshold
  Fixel::Matrix::norm_matrix_type norm_connectivity_matrix;
  // Also pre-compute fixel-fixel weights for smoothing
  Fixel::Matrix::norm_matrix_type smoothing_weights;
  Fixel::Matrix::normalise (connectivity_matrix,
                            index_image,
                            index_remapper,
                            connectivity_threshold,
                            norm_connectivity_matrix,
                            smoothing_fwhm,
                            smoothing_weights);

  // Function normalise_matrix() does not pre-exponentiate the connectivity weights
  //   by parameter C, nor does it calculate the normalisation factor per fixel
  //   for use of the normalised CFE equation
  // TODO Once fixelcfestats is capable of receiving as input a connectivity
  //   matrix & pre-smoothed data instead of a track file, this process will
  //   still need to be run
  {
    ProgressBar progress ("Pre-conditioning connectivity matrix", norm_connectivity_matrix.size());
    index_type num_unconnected_fixels = 0;
    for (index_type fixel_index = 0; fixel_index != mask_fixels; ++fixel_index) {
      if (norm_connectivity_matrix[fixel_index].empty()) {
        // If no streamlines traversed this fixel, connectivity matrix will be empty;
        //   let's at least inform it that it is "fully connected" to itself
        // Note however that we do this here rather than within the matrix generation functions,
        //   as this is specifically a fix for CFE
        norm_connectivity_matrix[fixel_index].push_back (Fixel::Matrix::NormElement (fixel_index, 1.0));
        ++num_unconnected_fixels;
      } else {
        for (auto f : norm_connectivity_matrix[fixel_index])
          f.exponentiate (cfe_c);
        if (cfe_norm)
          norm_connectivity_matrix[fixel_index].normalise();
      }
      ++progress;
    }
    if (num_unconnected_fixels && (cfe_norm || do_nonstationarity_adjustment)) {
      WARN ("A total of " + str(num_unconnected_fixels) + " fixels observed " +
            (get_options ("mask").size() ? "within provided mask " : "in template ") +
            "without any streamlines-based connectivity; " +
            "this may interfere with " +
            (cfe_norm ? (str("normalised CFE expression") + (do_nonstationarity_adjustment ? " and/or " : "")) : "") +
            "non-stationarity correction");
    }
  }

  Header output_header (dynamic_cast<SubjectFixelImport*>(importer[0].get())->header());
  //output_header.keyval()["num permutations"] = str(num_perms);
  output_header.keyval()["cfe_dh"] = str(cfe_dh);
  output_header.keyval()["cfe_e"] = str(cfe_e);
  output_header.keyval()["cfe_h"] = str(cfe_h);
  output_header.keyval()["cfe_c"] = str(cfe_c);
  output_header.keyval()["angular threshold"] = str(angular_threshold);
  output_header.keyval()["connectivity threshold"] = str(connectivity_threshold);
  output_header.keyval()["smoothing FWHM"] = str(smoothing_fwhm);

  // Smoothing the data prior to loading into the data matrix is deferred to the SubjectFixelImport class
  Fixel::Filter::Smooth smoothing_filter (smoothing_weights);

  // Load input data
  matrix_type data = matrix_type::Zero (importer.size(), mask_fixels);
  {
    ProgressBar progress (std::string ("Loading input images") + (do_smoothing ? " and smoothing" : ""), importer.size());
    for (size_t subject = 0; subject != importer.size(); subject++) {
      if (do_smoothing)
        dynamic_cast<SubjectFixelImport*>(importer[subject].get())->smooth (smoothing_filter);
      (*importer[subject]) (data.row (subject));
      progress++;
    }
  }
  const bool nans_in_data = !data.allFinite();
  if (nans_in_data) {
    INFO ("Non-finite values present in data; rows will be removed from fixel-wise design matrices accordingly");
    if (!extra_columns.size()) {
      INFO ("(Note that this will result in slower execution than if such values were not present)");
    }
  }

  // Free the memory occupied by the data smoothing filter; no longer required
  Fixel::Matrix::norm_matrix_type().swap (smoothing_weights);

  // Only add contrast matrix row number to image outputs if there's more than one hypothesis
  auto postfix = [&] (const size_t i) { return (num_hypotheses > 1) ? ("_" + hypotheses[i].name()) : ""; };

  {
    matrix_type betas (num_factors, mask_fixels);
    matrix_type abs_effect_size (mask_fixels, num_hypotheses), std_effect_size (mask_fixels, num_hypotheses);
    vector_type cond (mask_fixels), stdev (mask_fixels);

    Math::Stats::GLM::all_stats (data, design, extra_columns, hypotheses,
                                 cond, betas, abs_effect_size, std_effect_size, stdev);

    ProgressBar progress ("Outputting beta coefficients, effect size and standard deviation", num_factors + (2 * num_hypotheses) + 1 + (nans_in_data || extra_columns.size() ? 1 : 0));

    for (ssize_t i = 0; i != num_factors; ++i) {
      write_fixel_output (Path::join (output_fixel_directory, "beta" + str(i) + ".mif"), betas.row(i), output_header);
      ++progress;
    }
    for (size_t i = 0; i != num_hypotheses; ++i) {
      if (!hypotheses[i].is_F()) {
        write_fixel_output (Path::join (output_fixel_directory, "abs_effect" + postfix(i) + ".mif"), abs_effect_size.col(i), output_header);
        ++progress;
        write_fixel_output (Path::join (output_fixel_directory, "std_effect" + postfix(i) + ".mif"), std_effect_size.col(i), output_header);
        ++progress;
      }
    }
    if (nans_in_data || extra_columns.size()) {
      write_fixel_output (Path::join (output_fixel_directory, "cond.mif"), cond, output_header);
      ++progress;
    }
    write_fixel_output (Path::join (output_fixel_directory, "std_dev.mif"), stdev, output_header);
  }

  // Construct the class for performing the initial statistical tests
  std::shared_ptr<Math::Stats::GLM::TestBase> glm_test;
  if (extra_columns.size() || nans_in_data) {
    glm_test.reset (new Math::Stats::GLM::TestVariable (extra_columns, data, design, hypotheses, nans_in_data, nans_in_columns));
  } else {
    glm_test.reset (new Math::Stats::GLM::TestFixed (data, design, hypotheses));
  }

  // Construct the class for performing fixel-based statistical enhancement
  std::shared_ptr<Stats::EnhancerBase> cfe_integrator (new Stats::CFE (norm_connectivity_matrix, cfe_dh, cfe_e, cfe_h));

  // If performing non-stationarity adjustment we need to pre-compute the empirical CFE statistic
  matrix_type empirical_cfe_statistic;
  if (do_nonstationarity_adjustment) {
    Stats::PermTest::precompute_empirical_stat (glm_test, cfe_integrator, empirical_skew, empirical_cfe_statistic);
    output_header.keyval()["nonstationarity adjustment"] = str(true);
    for (size_t i = 0; i != num_hypotheses; ++i)
      write_fixel_output (Path::join (output_fixel_directory, "cfe_empirical" + postfix(i) + ".mif"), empirical_cfe_statistic.col(i), output_header);
  } else {
    output_header.keyval()["nonstationarity adjustment"] = str(false);
  }

  // Precompute default statistic and CFE statistic
  matrix_type cfe_output, tvalue_output;
  Stats::PermTest::precompute_default_permutation (glm_test, cfe_integrator, empirical_cfe_statistic, cfe_output, tvalue_output);
  for (size_t i = 0; i != num_hypotheses; ++i) {
    write_fixel_output (Path::join (output_fixel_directory, "cfe" + postfix(i) + ".mif"), cfe_output.col(i), output_header);
    write_fixel_output (Path::join (output_fixel_directory, std::string(hypotheses[i].is_F() ? "F" : "t") + "value" + postfix(i) + ".mif"), tvalue_output.col(i), output_header);
  }

  // Perform permutation testing
  if (!get_options ("notest").size()) {

    matrix_type null_distribution, uncorrected_pvalues;
    count_matrix_type null_contributions;
    Stats::PermTest::run_permutations (glm_test, cfe_integrator, empirical_cfe_statistic,
                                       cfe_output, null_distribution, null_contributions, uncorrected_pvalues);

    ProgressBar progress ("Outputting final results", 4*num_hypotheses + 1);

    for (size_t i = 0; i != num_hypotheses; ++i) {
      save_vector (null_distribution.col(i), Path::join (output_fixel_directory, "perm_dist" + postfix(i) + ".txt"));
      ++progress;
    }

    const matrix_type pvalue_output = MR::Math::Stats::fwe_pvalue (null_distribution, cfe_output);
    ++progress;
    for (size_t i = 0; i != num_hypotheses; ++i) {
      write_fixel_output (Path::join (output_fixel_directory, "fwe_pvalue" + postfix(i) + ".mif"), pvalue_output.col(i), output_header);
      ++progress;
      write_fixel_output (Path::join (output_fixel_directory, "uncorrected_pvalue" + postfix(i) + ".mif"), uncorrected_pvalues.col(i), output_header);
      ++progress;
      write_fixel_output (Path::join (output_fixel_directory, "null_contributions" + postfix(i) + ".mif"), null_contributions.col(i), output_header);
      ++progress;
    }
  }
}

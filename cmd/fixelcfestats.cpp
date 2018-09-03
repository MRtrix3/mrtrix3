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
#include "progressbar.h"
#include "thread_queue.h"
#include "algo/loop.h"
#include "transform.h"
#include "image.h"
#include "fixel/helpers.h"
#include "fixel/keys.h"
#include "fixel/loop.h"
#include "math/stats/fwe.h"
#include "math/stats/glm.h"
#include "math/stats/import.h"
#include "math/stats/shuffle.h"
#include "math/stats/typedefs.h"
#include "stats/cfe.h"
#include "stats/enhance.h"
#include "stats/permtest.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/writer.h"


using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography::Mapping;
using namespace MR::Math::Stats;
using namespace MR::Math::Stats::GLM;
using Stats::CFE::direction_type;
using Stats::CFE::connectivity_value_type;
using Stats::CFE::index_type;
using Stats::PermTest::count_matrix_type;

#define DEFAULT_CFE_DH 0.1
#define DEFAULT_CFE_E 2.0
#define DEFAULT_CFE_H 3.0
#define DEFAULT_CFE_C 0.5
#define DEFAULT_ANGLE_THRESHOLD 45.0
#define DEFAULT_CONNECTIVITY_THRESHOLD 0.01
#define DEFAULT_SMOOTHING_STD 10.0
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

  + Option ("smooth", "smooth the fixel value along the fibre tracts using a Gaussian kernel with the supplied FWHM (default: " + str(DEFAULT_SMOOTHING_STD, 2) + "mm)")
  + Argument ("FWHM").type_float (0.0, 200.0)

  + Option ("connectivity", "a threshold to define the required fraction of shared connections to be included in the neighbourhood (default: " + str(DEFAULT_CONNECTIVITY_THRESHOLD, 2) + ")")
  + Argument ("threshold").type_float (0.0, 1.0)

  + Option ("angle", "the max angle threshold for assigning streamline tangents to fixels (Default: " + str(DEFAULT_ANGLE_THRESHOLD, 2) + " degrees)")
  + Argument ("value").type_float (0.0, 90.0)

  + Option ("mask", "provide a fixel data file containing a mask of those fixels to be used during processing")
  + Argument ("file").type_image_in();

}



// Global variabes that need to be set via run() but accessed by other functions / classes

// Lookup table that maps from input fixel index to column number
// Note that if a fixel is masked out, it will have a value of -1
//   in this array; hence require a signed integer type
vector<int32_t> fixel2column;
// We also need the inverse mappping in order to provide ability
//   to access appropriate datum for any random fixel
vector<index_type> column2fixel;





template <class VectorType>
void write_fixel_output (const std::string& filename,
                         const VectorType& data,
                         const Header& header)
{
  assert (size_t(header.size(0)) == fixel2column.size());
  auto output = Image<float>::create (filename, header);
  for (size_t i = 0; i != fixel2column.size(); ++i) {
    output.index(0) = i;
    output.value() = (fixel2column[i] >= 0) ? data[fixel2column[i]] : NaN;
  }
}



// Define data importer class that willl obtain fixel data for a
//   specific subject based on the string path to the image file for
//   that subject
class SubjectFixelImport : public SubjectDataImportBase
{ MEMALIGN(SubjectFixelImport)
  public:
    SubjectFixelImport (const std::string& path) :
        SubjectDataImportBase (path),
        H (Header::open (find_image (path))),
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
      // Due to merging 'stats_enhancements' with '3.0_RC2',
      //   this class now needs to be made aware of the fixel2column contents
      for (temp.index(0) = 0; temp.index(0) != temp.size(0); ++temp.index(0)) {
        if (fixel2column[temp.index(0)] >= 0)
          row (fixel2column[temp.index(0)]) = temp.value();
      }
    }

    // Is this going to require a reverse lookup?
    // I.e. We need to load subject data for a particular row of the
    //   data / design matrix, but this may be a different fixel index
    //   in the input data
    // Would we be better off _not_ reducing the size of the data array,
    //   and instead providing a mask by which to not process particular elements?
    // No, this only applies to the fixel stats for now; it's handled
    //   in other ways in the other stats commands
    // If statistical inference commands for other natures of data are
    //   implemented in the future, it may be worth having a class to
    //   provide forward and reverse index mappings (much like Voxel2Vector
    //   currently does for 3D images)
    default_type operator[] (const size_t index) const override
    {
      assert (index < column2fixel.size());
      Image<float> temp (data); // For thread-safety
      temp.index(0) = column2fixel[index];
      assert (!is_out_of_bounds (temp));
      return default_type(temp.value());
    }

    size_t size() const override { return data.size(0); }

    const Header& header() const { return H; }


    static void set_fixel_directory (const std::string& s) { fixel_directory = s; }


  private:
    Header H;
    const Image<float> data;

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
  const value_type smooth_std_dev = get_option_value ("smooth", DEFAULT_SMOOTHING_STD) / 2.3548;
  const value_type connectivity_threshold = get_option_value ("connectivity", DEFAULT_CONNECTIVITY_THRESHOLD);
  const value_type angular_threshold = get_option_value ("angle", DEFAULT_ANGLE_THRESHOLD);

  const bool do_nonstationarity_adjustment = get_options ("nonstationarity").size();
  const default_type empirical_skew = get_option_value ("skew_nonstationarity", DEFAULT_EMPIRICAL_SKEW);

  const std::string input_fixel_directory = argument[0];
  SubjectFixelImport::set_fixel_directory (input_fixel_directory);
  Header index_header = Fixel::find_index_header (input_fixel_directory);
  auto index_image = index_header.get_image<index_type>();

  const index_type num_fixels = Fixel::get_number_of_fixels (index_header);
  CONSOLE ("Number of fixels: " + str(num_fixels));

  Image<bool> mask;
  index_type mask_fixels;
  fixel2column.resize (num_fixels);
  auto opt = get_options ("mask");
  if (opt.size()) {
    mask = Image<bool>::open (opt[0][0]);
    Fixel::check_data_file (mask);
    if (!Fixel::fixels_match (index_header, mask))
      throw Exception ("Mask image provided using -mask option does not match fixel template");
    mask_fixels = 0;
    for (mask.index(0) = 0; mask.index(0) != num_fixels; ++mask.index(0))
      fixel2column[mask.index(0)] = mask.value() ? mask_fixels++ : -1;
    CONSOLE ("Fixel mask contains " + str(mask_fixels) + " fixels");
#ifdef NDEBUG
    column2fixel.resize (mask_fixels);
#else
    column2fixel.assign (mask_fixels, std::numeric_limits<index_type>::max());
#endif
    for (index_type fixel = 0; fixel != num_fixels; ++fixel) {
      if (fixel2column[fixel] >= 0)
        column2fixel[fixel2column[fixel]] = fixel;
    }
#ifndef NDEBUG
    for (index_type column = 0; column != mask_fixels; ++column)
      assert (column2fixel[column] != std::numeric_limits<index_type>::max());
#endif
  } else {
    Header data_header;
    data_header.ndim() = 3;
    data_header.size(0) = num_fixels;
    data_header.size(1) = 1;
    data_header.size(2) = 1;
    data_header.spacing(0) = data_header.spacing(1) = data_header.spacing(2) = NaN;
    data_header.stride(0) = 1; data_header.stride(1) = 2; data_header.stride(2) = 3;
    mask = Image<bool>::scratch (data_header, "scratch fixel mask");
    for (mask.index(0) = 0; mask.index(0) != num_fixels; ++mask.index(0))
      mask.value() = true;
    mask_fixels = num_fixels;
    column2fixel.resize (num_fixels);
    for (index_type f = 0; f != num_fixels; ++f)
      fixel2column[f] = column2fixel[f] = f;
  }

  vector<Eigen::Vector3> positions (num_fixels);
  vector<direction_type> directions (num_fixels);

  const std::string output_fixel_directory = argument[5];
  Fixel::copy_index_and_directions_file (input_fixel_directory, output_fixel_directory);

  {
    auto directions_data = Fixel::find_directions_header (input_fixel_directory).get_image<default_type>().with_direct_io ({+2,+1});
    // Load template fixel directions
    Transform image_transform (index_image);
    for (auto i = Loop ("loading template fixel directions and positions", index_image, 0, 3)(index_image); i; ++i) {
      Eigen::Vector3 vox ((default_type)index_image.index(0), (default_type)index_image.index(1), (default_type)index_image.index(2));
      vox = image_transform.voxel2scanner * vox;
      index_image.index(3) = 1;
      const index_type offset = index_image.value();
      size_t fixel_index = 0;
      for (auto f = Fixel::Loop (index_image) (directions_data); f; ++f, ++fixel_index) {
        directions[offset + fixel_index] = directions_data.row(1);
        positions[offset + fixel_index] = vox;
      }
    }
  }

  // Read file names and check files exist
  CohortDataImport importer;
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

  // Load contrasts
  const vector<Contrast> contrasts = Math::Stats::GLM::load_contrasts (argument[3]);
  const size_t num_contrasts = contrasts.size();
  CONSOLE ("Number of contrasts: " + str(num_contrasts));

  // Before validating the contrast matrix, we first need to see if there are any
  //   additional design matrix columns coming from fixel-wise subject data
  vector<CohortDataImport> extra_columns;
  bool nans_in_columns = false;
  opt = get_options ("column");
  for (size_t i = 0; i != opt.size(); ++i) {
    extra_columns.push_back (CohortDataImport());
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
  if (contrasts[0].cols() != num_factors)
    throw Exception ("The number of columns per contrast (" + str(contrasts[0].cols()) + ")"
                     + (extra_columns.size() ? " (in addition to the " + str(extra_columns.size()) + " uses of -column)" : "")
                     + " does not equal the number of columns in the design matrix (" + str(design.cols()) + ")");

  // Compute fixel-fixel connectivity
  Stats::CFE::init_connectivity_matrix_type connectivity_matrix (num_fixels);
  vector<uint16_t> fixel_TDI (num_fixels, 0);
  const std::string track_filename = argument[4];
  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<float> track_file (track_filename, properties);
  // Read in tracts, and compute whole-brain fixel-fixel connectivity
  const size_t num_tracks = properties["count"].empty() ? 0 : to<size_t> (properties["count"]);
  if (!num_tracks)
    throw Exception ("No tracks found in input file");
  if (num_tracks < 1000000) {
    WARN ("More than 1 million tracks is preferable to ensure robust fixel-fixel connectivity; file \"" + track_filename + "\" contains only " + str(num_tracks));
  }
  {
    DWI::Tractography::Mapping::TrackLoader loader (track_file, num_tracks, "pre-computing fixel-fixel connectivity");
    DWI::Tractography::Mapping::TrackMapperBase mapper (index_image);
    mapper.set_upsample_ratio (DWI::Tractography::Mapping::determine_upsample_ratio (index_header, properties, 0.333f));
    mapper.set_use_precise_mapping (true);
    //Stats::CFE::TrackProcessor tract_processor (mapper, index_image, directions, mask, fixel_TDI, connectivity_matrix, angular_threshold);
    /*Thread::run_queue (
        loader,
        Thread::batch (DWI::Tractography::Streamline<float>()),
        mapper,
        Thread::batch (DWI::Tractography::Mapping::SetVoxelDir()),
        tract_processor);*/
    //Thread::run_queue (loader, Thread::batch (DWI::Tractography::Streamline<float>()), Thread::multi (tract_processor));
    Stats::CFE::TrackProcessor tract_processor (mapper, index_image, directions, mask, angular_threshold);
    Stats::CFE::MappedTrackReceiver receiver (fixel_TDI, connectivity_matrix);
    Thread::run_queue (loader,
                       Thread::batch (DWI::Tractography::Streamline<float>()),
                       Thread::multi (tract_processor),
                       Thread::batch (vector<index_type>()),
                       receiver);
  }
  track_file.close();

  // Normalise connectivity matrix, threshold, and put in a more efficient format
  Stats::CFE::norm_connectivity_matrix_type norm_connectivity_matrix (mask_fixels);
  // Also pre-compute fixel-fixel weights for smoothing.
  Stats::CFE::norm_connectivity_matrix_type smoothing_weights (mask_fixels);

  const bool do_smoothing = (smooth_std_dev > 0.0);
  const float gaussian_const1 = do_smoothing ? (1.0 / (smooth_std_dev *  std::sqrt (2.0 * Math::pi))) : 1.0;
  const float gaussian_const2 = 2.0 * smooth_std_dev * smooth_std_dev;

  {
    class Source
    { MEMALIGN(Source)
      public:
        Source (Image<bool>& mask) :
            mask (mask),
            num_fixels (mask.size (0)),
            counter (0),
            progress ("normalising and thresholding fixel-fixel connectivity matrix", num_fixels) { }
        bool operator() (size_t& fixel_index) {
          while (counter < num_fixels) {
            mask.index(0) = counter;
            ++progress;
            if (mask.value()) {
              fixel_index = counter++;
              return true;
            }
            ++counter;
          }
          fixel_index = num_fixels;
          return false;
        }
      private:
        Image<bool> mask;
        const size_t num_fixels;
        size_t counter;
        ProgressBar progress;
    };

    auto Sink = [&] (const size_t& fixel_index)
    {
      assert (fixel_index < connectivity_matrix.size());
      const int32_t column = fixel2column[fixel_index];
      assert (column >= 0 && column < int32_t(norm_connectivity_matrix.size()));

      // Here, the connectivity matrix needs to be modified to reflect the
      //   fact that fixel indices in the template fixel image may not
      //   correspond to rows in the statistical analysis
      connectivity_value_type sum_weights = 0.0;
      for (auto& it : connectivity_matrix[fixel_index]) {
        const connectivity_value_type connectivity = it.value() / connectivity_value_type (fixel_TDI[fixel_index]);
        if (connectivity >= connectivity_threshold) {
          if (do_smoothing) {
            const value_type distance = std::sqrt (Math::pow2 (positions[fixel_index][0] - positions[it.index()][0]) +
                Math::pow2 (positions[fixel_index][1] - positions[it.index()][1]) +
                Math::pow2 (positions[fixel_index][2] - positions[it.index()][2]));
            const connectivity_value_type smoothing_weight = connectivity * gaussian_const1 * std::exp (-Math::pow2 (distance) / gaussian_const2);
            if (smoothing_weight >= connectivity_threshold) {
              smoothing_weights[column].push_back (Stats::CFE::NormMatrixElement (fixel2column[it.index()], smoothing_weight));
              sum_weights += smoothing_weight;
            }
          }
          // Here we pre-exponentiate each connectivity value by C
          norm_connectivity_matrix[column].push_back (Stats::CFE::NormMatrixElement (fixel2column[it.index()], std::pow (connectivity, cfe_c)));
        }
      }

      // Make sure the fixel is fully connected to itself
      norm_connectivity_matrix[column].push_back (Stats::CFE::NormMatrixElement (uint32_t(column), connectivity_value_type(1.0)));
      smoothing_weights[column].push_back (Stats::CFE::NormMatrixElement (uint32_t(column), connectivity_value_type(gaussian_const1)));
      sum_weights += connectivity_value_type (gaussian_const1);

      // Normalise smoothing weights
      const connectivity_value_type norm_factor = connectivity_value_type(1.0) / sum_weights;
      for (auto i : smoothing_weights[column])
        i.normalise (norm_factor);

      // Calculate multiplicative factor for CFE normalisation
      if (cfe_norm)
        norm_connectivity_matrix[column].normalise();

      // Force deallocation of memory used for this fixel in the original matrix
      //std::map<uint32_t, Stats::CFE::connectivity>().swap (connectivity_matrix[fixel_index]);
      Stats::CFE::InitMatrixFixel().swap (connectivity_matrix[fixel_index]);

      return true;
    };

    Source source (mask);
    Thread::run_queue (source, size_t(), Thread::multi (Sink));
  }


  // The connectivity matrix is now in vector rather than matrix form;
  //   throw out the structure holding the original data
  // (Note however that all entries in the original structure should
  //   have been deleted during the prior loop)
  Stats::CFE::init_connectivity_matrix_type().swap (connectivity_matrix);


  Header output_header (dynamic_cast<SubjectFixelImport*>(importer[0].get())->header());
  //output_header.keyval()["num permutations"] = str(num_perms);
  output_header.keyval()["dh"] = str(cfe_dh);
  output_header.keyval()["cfe_e"] = str(cfe_e);
  output_header.keyval()["cfe_h"] = str(cfe_h);
  output_header.keyval()["cfe_c"] = str(cfe_c);
  output_header.keyval()["angular threshold"] = str(angular_threshold);
  output_header.keyval()["connectivity threshold"] = str(connectivity_threshold);
  output_header.keyval()["smoothing FWHM"] = str(smooth_std_dev * 2.3548);


  // Load input data
  matrix_type data = matrix_type::Zero (importer.size(), mask_fixels);
  bool nans_in_data = false;
  {
    ProgressBar progress (std::string ("Loading input images") + (do_smoothing ? " and smoothing" : ""), importer.size());
    for (size_t subject = 0; subject != importer.size(); subject++) {

      // TODO It might be faster to import into a vector here, rather than
      //   going straight into the data matrix, since we most likely have to smooth
      (*importer[subject]) (data.row (subject));

      // Smooth the data
      if (do_smoothing) {
        vector_type smoothed_data (vector_type::Zero (mask_fixels));
        for (size_t fixel = 0; fixel != mask_fixels; ++fixel) {
          if (std::isfinite (data (subject, fixel))) {
            value_type value = 0.0, sum_weights = 0.0;
            for (const auto& it : smoothing_weights[fixel]) {
              if (std::isfinite (data (subject, it.index()))) {
                value += data (subject, it.index()) * it.value();
                sum_weights += it.value();
              }
            }
            if (sum_weights)
              smoothed_data[fixel] = value / sum_weights;
            else
              smoothed_data[fixel] = NaN;
          } else {
            smoothed_data[fixel] = NaN;
          }
        }
        data.row (subject) = smoothed_data;
      }
      if (!data.row (subject).allFinite())
        nans_in_data = true;
    }
    progress++;
  }
  if (nans_in_data) {
    INFO ("Non-finite values present in data; rows will be removed from fixel-wise design matrices accordingly");
    if (!extra_columns.size()) {
      INFO ("(Note that this will result in slower execution than if such values were not present)");
    }
  }

  // Free the memory occupied by the data smoothing filter; no longer required
  Stats::CFE::norm_connectivity_matrix_type().swap (smoothing_weights);

  // Only add contrast row number to image outputs if there's more than one contrast
  auto postfix = [&] (const size_t i) { return (num_contrasts > 1) ? ("_" + contrasts[i].name()) : ""; };

  {
    matrix_type betas (num_factors, num_fixels);
    matrix_type abs_effect_size (num_fixels, num_contrasts), std_effect_size (num_fixels, num_contrasts);
    vector_type cond (num_fixels), stdev (num_fixels);

    Math::Stats::GLM::all_stats (data, design, extra_columns, contrasts,
                                 cond, betas, abs_effect_size, std_effect_size, stdev);

    ProgressBar progress ("Outputting beta coefficients, effect size and standard deviation", num_factors + (2 * num_contrasts) + 1 + (nans_in_data || extra_columns.size() ? 1 : 0));

    for (ssize_t i = 0; i != num_factors; ++i) {
      write_fixel_output (Path::join (output_fixel_directory, "beta" + str(i) + ".mif"), betas.row(i), output_header);
      ++progress;
    }
    for (size_t i = 0; i != num_contrasts; ++i) {
      if (!contrasts[i].is_F()) {
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
  std::shared_ptr<GLM::TestBase> glm_test;
  if (extra_columns.size() || nans_in_data) {
    glm_test.reset (new GLM::TestVariable (extra_columns, data, design, contrasts, nans_in_data, nans_in_columns));
  } else {
    glm_test.reset (new GLM::TestFixed (data, design, contrasts));
  }

  // Construct the class for performing fixel-based statistical enhancement
  std::shared_ptr<Stats::EnhancerBase> cfe_integrator (new Stats::CFE::Enhancer (norm_connectivity_matrix, cfe_dh, cfe_e, cfe_h));

  // If performing non-stationarity adjustment we need to pre-compute the empirical CFE statistic
  matrix_type empirical_cfe_statistic;
  if (do_nonstationarity_adjustment) {
    Stats::PermTest::precompute_empirical_stat (glm_test, cfe_integrator, empirical_skew, empirical_cfe_statistic);
    output_header.keyval()["nonstationarity adjustment"] = str(true);
    for (size_t i = 0; i != num_contrasts; ++i)
      write_fixel_output (Path::join (output_fixel_directory, "cfe_empirical" + postfix(i) + ".mif"), empirical_cfe_statistic.col(i), output_header);
  } else {
    output_header.keyval()["nonstationarity adjustment"] = str(false);
  }

  // Precompute default statistic and CFE statistic
  matrix_type cfe_output, tvalue_output;
  Stats::PermTest::precompute_default_permutation (glm_test, cfe_integrator, empirical_cfe_statistic, cfe_output, tvalue_output);
  for (size_t i = 0; i != num_contrasts; ++i) {
    write_fixel_output (Path::join (output_fixel_directory, "cfe" + postfix(i) + ".mif"), cfe_output.col(i), output_header);
    write_fixel_output (Path::join (output_fixel_directory, std::string(contrasts[i].is_F() ? "F" : "t") + "value" + postfix(i) + ".mif"), tvalue_output.col(i), output_header);
  }

  // Perform permutation testing
  if (!get_options ("notest").size()) {

    matrix_type null_distribution, uncorrected_pvalues;
    count_matrix_type null_contributions;
    Stats::PermTest::run_permutations (glm_test, cfe_integrator, empirical_cfe_statistic,
                                       cfe_output, null_distribution, null_contributions, uncorrected_pvalues);

    ProgressBar progress ("Outputting final results", 4*num_contrasts + 1);

    for (size_t i = 0; i != num_contrasts; ++i) {
      save_vector (null_distribution.col(i), Path::join (output_fixel_directory, "perm_dist" + postfix(i) + ".txt"));
      ++progress;
    }

    const matrix_type pvalue_output = MR::Math::Stats::fwe_pvalue (null_distribution, cfe_output);
    ++progress;
    for (size_t i = 0; i != num_contrasts; ++i) {
      write_fixel_output (Path::join (output_fixel_directory, "fwe_pvalue" + postfix(i) + ".mif"), pvalue_output.col(i), output_header);
      ++progress;
      write_fixel_output (Path::join (output_fixel_directory, "uncorrected_pvalue" + postfix(i) + ".mif"), uncorrected_pvalues.col(i), output_header);
      ++progress;
      write_fixel_output (Path::join (output_fixel_directory, "null_contributions" + postfix(i) + ".mif"), null_contributions.col(i), output_header);
      ++progress;
    }
  }
}

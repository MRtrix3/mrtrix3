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


#include "command.h"
#include "image.h"

#include "algo/loop.h"
#include "file/path.h"

#include "math/stats/glm.h"
#include "math/stats/import.h"
#include "math/stats/shuffle.h"
#include "math/stats/typedefs.h"

#include "stats/cluster.h"
#include "stats/enhance.h"
#include "stats/permtest.h"
#include "stats/tfce.h"


using namespace MR;
using namespace App;
using namespace MR::Math::Stats;
using namespace MR::Math::Stats::GLM;


#define DEFAULT_TFCE_DH 0.1
#define DEFAULT_TFCE_H 2.0
#define DEFAULT_TFCE_E 0.5


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Voxel-based analysis using permutation testing and threshold-free cluster enhancement";

  DESCRIPTION
      + Math::Stats::GLM::column_ones_description;

  REFERENCES
   + "* If not using the -threshold command-line option:\n"
   "Smith, S. M. & Nichols, T. E. "
   "Threshold-free cluster enhancement: Addressing problems of smoothing, threshold dependence and localisation in cluster inference. "
   "NeuroImage, 2009, 44, 83-98"

   + "* If using the -nonstationary option:"
   "Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. Adjusting the effect of nonstationarity in cluster-based and TFCE inference. \n"
   "Neuroimage, 2011, 54(3), 2006-19\n";


  ARGUMENTS
  + Argument ("input", "a text file containing the file names of the input images, one file per line").type_file_in()

  + Argument ("design", "the design matrix").type_file_in()

  + Argument ("contrast", "the contrast matrix, only specify one contrast as it will automatically compute the opposite contrast.").type_file_in()

  + Argument ("mask", "a mask used to define voxels included in the analysis.").type_image_in()

  + Argument ("output", "the filename prefix for all output.").type_text();


  OPTIONS
  + Math::Stats::shuffle_options (true)

  + Stats::TFCE::Options (DEFAULT_TFCE_DH, DEFAULT_TFCE_E, DEFAULT_TFCE_H)

  + OptionGroup ("Additional options for mrclusterstats")

    + Option ("threshold", "the cluster-forming threshold to use for a standard cluster-based analysis. "
                           "This disables TFCE, which is the default otherwise.")
      + Argument ("value").type_float (1.0e-6)

    + Option ("column", "add a column to the design matrix corresponding to subject voxel-wise values "
                        "(the contrast vector length must include columns for these additions)").allow_multiple()
      + Argument ("path").type_file_in()

    + Option ("connectivity", "use 26-voxel-neighbourhood connectivity (Default: 6)");

}



typedef Stats::TFCE::value_type value_type;



template <class VectorType>
void write_output (const VectorType& data,
                   const Voxel2Vector& v2v,
                   const std::string& path,
                   const Header& header) {
  auto image = Image<float>::create (path, header);
  for (size_t i = 0; i != v2v.size(); i++) {
    assign_pos_of (v2v[i]).to (image);
    image.value() = data[i];
  }
}



// Define data importer class that will obtain voxel data for a
//   specific subject based on the string path to the image file for
//   that subject
//
// The challenge with this mechanism for voxel data is that the
//   class must know how to map data from voxels in 3D space into
//   a 1D vector of data. This mapping must be done based on the
//   analysis mask prior to the importing of any subject data.
//   Moreover, in the case of voxel-wise design matrix columns, the
//   class must have access to this mapping functionality without
//   any modification of the class constructor (since these data
//   are initialised in the CohortDataImport class).
//
class SubjectVoxelImport : public SubjectDataImportBase
{ MEMALIGN(SubjectVoxelImport)
  public:
    SubjectVoxelImport (const std::string& path) :
        SubjectDataImportBase (path),
        H (Header::open (path)),
        data (H.get_image<float>()) { }

    void operator() (matrix_type::ColXpr column) const override
    {
      assert (v2v);
      assert (column.rows() == size());
      Image<float> temp (data); // For thread-safety
      for (size_t i = 0; i != size(); ++i) {
        assign_pos_of ((*v2v)[i]).to (temp);
        column[i] = temp.value();
      }
    }

    default_type operator[] (const size_t index) const override
    {
      assert (v2v);
      assert (index < size());
      Image<float> temp (data); // For thread-safety
      assign_pos_of ((*v2v)[index]).to (temp);
      return temp.value();
    }

    size_t size() const override { assert (v2v); return v2v->size(); }

    const Header& header() const { return H; }

    void set_mapping (std::shared_ptr<Voxel2Vector>& ptr) {
      v2v = ptr;
    }

  private:
    Header H;
    const Image<float> data;

    static std::shared_ptr<Voxel2Vector> v2v;

};
std::shared_ptr<Voxel2Vector> SubjectVoxelImport::v2v = nullptr;





void run() {

  const value_type cluster_forming_threshold = get_option_value ("threshold", NaN);
  const value_type tfce_dh = get_option_value ("tfce_dh", DEFAULT_TFCE_DH);
  const value_type tfce_H = get_option_value ("tfce_h", DEFAULT_TFCE_H);
  const value_type tfce_E = get_option_value ("tfce_e", DEFAULT_TFCE_E);
  const bool use_tfce = !std::isfinite (cluster_forming_threshold);
  const bool do_26_connectivity = get_options("connectivity").size();
  const bool do_nonstationary_adjustment = get_options ("nonstationary").size();

  // Load analysis mask and compute adjacency
  auto mask_header = Header::open (argument[3]);
  auto mask_image = mask_header.get_image<bool>();
  Voxel2Vector v2v (mask_image, mask_header);
  Filter::Connector connector;
  connector.adjacency.set_26_adjacency (do_26_connectivity);
  connector.adjacency.initialise (mask_header, v2v);
  const size_t num_voxels = v2v.size();

  // Read file names and check files exist
  CohortDataImport importer;
  importer.initialise<SubjectVoxelImport> (argument[0]);
  for (size_t i = 0; i != importer.size(); ++i) {
    if (!dimensions_match (dynamic_cast<SubjectVoxelImport*>(importer[i].get())->header(), mask_header))
      throw Exception ("Image file \"" + importer[i]->name() + "\" does not match analysis mask");
  }
  CONSOLE ("Number of subjects: " + str(importer.size()));

  // Load design matrix
  const matrix_type design = load_matrix<value_type> (argument[1]);
  if (design.rows() != (ssize_t)importer.size())
    throw Exception ("number of input files does not match number of rows in design matrix");

  // Load contrast matrix
  vector<Contrast> contrasts;
  {
    const matrix_type contrast_matrix = load_matrix (argument[2]);
    for (ssize_t row = 0; row != contrast_matrix.rows(); ++row)
      contrasts.emplace_back (Contrast (contrast_matrix.row (row)));
  }
  const size_t num_contrasts = contrasts.size();

  // Before validating the contrast matrix, we first need to see if there are any
  //   additional design matrix columns coming from voxel-wise subject data
  // TODO Functionalise this
  vector<CohortDataImport> extra_columns;
  bool nans_in_columns = false;
  auto opt = get_options ("column");
  for (size_t i = 0; i != opt.size(); ++i) {
    extra_columns.push_back (CohortDataImport());
    extra_columns[i].initialise<SubjectVoxelImport> (opt[i][0]);
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

  matrix_type data (num_voxels, importer.size());
  bool nans_in_data = false;
  {
    // Load images
    ProgressBar progress ("loading input images", importer.size());
    for (size_t subject = 0; subject < importer.size(); subject++) {
      (*importer[subject]) (data.col (subject));
      if (!data.col (subject).allFinite())
        nans_in_data = true;
      progress++;
    }
  }
  if (nans_in_data) {
    INFO ("Non-finite values present in data; rows will be removed from voxel-wise design matrices accordingly");
    if (!extra_columns.size()) {
      INFO ("(Note that this will result in slower execution than if such values were not present)");
    }
  }

  Header output_header (mask_header);
  output_header.datatype() = DataType::Float32;
  //output_header.keyval()["num permutations"] = str(num_perms);
  output_header.keyval()["26 connectivity"] = str(do_26_connectivity);
  output_header.keyval()["nonstationary adjustment"] = str(do_nonstationary_adjustment);
  if (use_tfce) {
    output_header.keyval()["tfce_dh"] = str(tfce_dh);
    output_header.keyval()["tfce_e"] = str(tfce_E);
    output_header.keyval()["tfce_h"] = str(tfce_H);
  } else {
    output_header.keyval()["threshold"] = str(cluster_forming_threshold);
  }

  const std::string prefix (argument[4]);

  matrix_type default_cluster_output (num_contrasts, num_voxels);
  matrix_type tvalue_output (num_contrasts, num_voxels);
  matrix_type empirical_enhanced_statistic;

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
    matrix_type betas (num_contrasts, num_voxels);
    matrix_type abs_effect_size (num_contrasts, num_voxels), std_effect_size (num_contrasts, num_voxels);
    vector_type stdev (num_voxels);

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
          Source (const size_t num_voxels) :
              num_voxels (num_voxels),
              counter (0),
              progress (new ProgressBar ("calculating basic properties of default permutation", num_voxels)) { }

          bool operator() (size_t& voxel_index)
          {
            voxel_index = counter++;
            if (counter >= num_voxels) {
              progress.reset();
              return false;
            }
            assert (progress);
            ++(*progress);
            return true;
          }

        private:
          const size_t num_voxels;
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

          bool operator() (const size_t& voxel_index)
          {
            const matrix_type data_voxel = data.row (voxel_index);
            const matrix_type design_voxel = dynamic_cast<const GLM::TestVariable* const>(glm_test.get())->default_design (voxel_index);
            Math::Stats::GLM::all_stats (data_voxel, design_voxel, contrasts,
                                         local_betas, local_abs_effect_size, local_std_effect_size, local_stdev);
            global_betas.col (voxel_index) = local_betas;
            global_abs_effect_size.col(voxel_index) = local_abs_effect_size.col(0);
            global_std_effect_size.col(voxel_index) = local_std_effect_size.col(0);
            global_stdev[voxel_index] = local_stdev[0];
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

      Source source (num_voxels);
      Functor functor (data, glm_test, contrasts,
                       betas, abs_effect_size, std_effect_size, stdev);
      Thread::run_queue (source, Thread::batch (size_t()), Thread::multi (functor));

    } else {

      ProgressBar progress ("calculating basic properties of default permutation");
      Math::Stats::GLM::all_stats (data, design, contrasts,
                                   betas, abs_effect_size, std_effect_size, stdev);
    }

    ProgressBar progress ("outputting beta coefficients, effect size and standard deviation", num_factors + (2 * num_contrasts) + 1);
    for (ssize_t i = 0; i != num_factors; ++i) {
      write_output (betas.row(i), v2v, prefix + "beta" + str(i) + ".mif", output_header);
      ++progress;
    }
    for (size_t i = 0; i != num_contrasts; ++i) {
      write_output (abs_effect_size.row(i), v2v, prefix + "abs_effect" + postfix(i) + ".mif", output_header); ++progress;
      write_output (std_effect_size.row(i), v2v, prefix + "std_effect" + postfix(i) + ".mif", output_header); ++progress;
    }
    write_output (stdev, v2v, prefix + "std_dev.mif", output_header);
  }

  std::shared_ptr<Stats::EnhancerBase> enhancer;
  if (use_tfce) {
    std::shared_ptr<Stats::TFCE::EnhancerBase> base (new Stats::Cluster::ClusterSize (connector, cluster_forming_threshold));
    enhancer.reset (new Stats::TFCE::Wrapper (base, tfce_dh, tfce_E, tfce_H));
  } else {
    enhancer.reset (new Stats::Cluster::ClusterSize (connector, cluster_forming_threshold));
  }

  if (do_nonstationary_adjustment) {
    if (!use_tfce)
      throw Exception ("nonstationary adjustment is not currently implemented for threshold-based cluster analysis");
    Stats::PermTest::precompute_empirical_stat (glm_test, enhancer, empirical_enhanced_statistic);
    for (size_t i = 0; i != num_contrasts; ++i)
      save_vector (empirical_enhanced_statistic.row(i), prefix + "empirical" + postfix(i) + ".txt");
  }

  if (!get_options ("notest").size()) {

    matrix_type perm_distribution, uncorrected_pvalue;

    Stats::PermTest::run_permutations (glm_test, enhancer, empirical_enhanced_statistic,
                                       default_cluster_output, perm_distribution, uncorrected_pvalue);

    for (size_t i = 0; i != num_contrasts; ++i)
      save_vector (perm_distribution.row(i), prefix + "perm_dist" + postfix(i) + ".txt");

    ProgressBar progress ("generating output", 1 + (2 * num_contrasts));
    for (size_t i = 0; i != num_contrasts; ++i) {
      write_output (uncorrected_pvalue.row(i), v2v, prefix + "uncorrected_pvalue" + postfix(i) + ".mif", output_header);
      ++progress;
    }
    matrix_type fwe_pvalue_output (num_contrasts, num_voxels);
    Math::Stats::statistic2pvalue (perm_distribution, default_cluster_output, fwe_pvalue_output);
    ++progress;
    for (size_t i = 0; i != num_contrasts; ++i) {
      write_output (fwe_pvalue_output.row(i), v2v, prefix + "fwe_pvalue" + postfix(i) + ".mif", output_header);
      ++progress;
    }

  }
}

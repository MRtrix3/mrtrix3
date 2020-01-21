/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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
#include "types.h"

#include "algo/loop.h"
#include "file/path.h"

#include "math/stats/fwe.h"
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

using Stats::PermTest::count_matrix_type;


#define DEFAULT_TFCE_DH 0.1
#define DEFAULT_TFCE_H 2.0
#define DEFAULT_TFCE_E 0.5

#define DEFAULT_EMPIRICAL_SKEW 1.0


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

   + "* If using the -nonstationary option:\n"
   "Salimi-Khorshidi, G. Smith, S.M. Nichols, T.E. Adjusting the effect of nonstationarity in cluster-based and TFCE inference. "
   "Neuroimage, 2011, 54(3), 2006-19";


  ARGUMENTS
  + Argument ("input", "a text file containing the file names of the input images, one file per line").type_file_in()

  + Argument ("design", "the design matrix").type_file_in()

  + Argument ("contrast", "the contrast matrix").type_file_in()

  + Argument ("mask", "a mask used to define voxels included in the analysis.").type_image_in()

  + Argument ("output", "the filename prefix for all output.").type_text();


  OPTIONS
  + Math::Stats::shuffle_options (true, DEFAULT_EMPIRICAL_SKEW)

  + Stats::TFCE::Options (DEFAULT_TFCE_DH, DEFAULT_TFCE_E, DEFAULT_TFCE_H)

  + Math::Stats::GLM::glm_options ("voxel")

  + OptionGroup ("Additional options for mrclusterstats")

    + Option ("threshold", "the cluster-forming threshold to use for a standard cluster-based analysis. "
                           "This disables TFCE, which is the default otherwise.")
      + Argument ("value").type_float (1.0e-6)

    + Option ("connectivity", "use 26-voxel-neighbourhood connectivity (Default: 6)");

}



using value_type = Stats::TFCE::value_type;



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

    virtual ~SubjectVoxelImport() { }

    void operator() (matrix_type::RowXpr row) const override
    {
      assert (v2v);
      Image<float> temp (data); // For thread-safety
      for (size_t i = 0; i != size(); ++i) {
        assign_pos_of ((*v2v)[i]).to (temp);
        row[i] = temp.value();
      }
    }

    default_type operator[] (const size_t index) const override
    {
      assert (v2v);
      Image<float> temp (data); // For thread-safety
      assign_pos_of ((*v2v)[index]).to (temp);
      assert (!is_out_of_bounds (temp));
      return temp.value();
    }

    size_t size() const override { assert (v2v); return v2v->size(); }

    const Header& header() const { return H; }

    static void set_mapping (std::shared_ptr<Voxel2Vector>& ptr) {
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
  const bool do_nonstationarity_adjustment = get_options ("nonstationarity").size();
  const default_type empirical_skew = get_option_value ("skew_nonstationarity", DEFAULT_EMPIRICAL_SKEW);

  // Load analysis mask and compute adjacency
  auto mask_header = Header::open (argument[3]);
  check_effective_dimensionality (mask_header, 3);
  auto mask_image = mask_header.get_image<bool>();
  std::shared_ptr<Voxel2Vector> v2v = make_shared<Voxel2Vector> (mask_image, mask_header);
  SubjectVoxelImport::set_mapping (v2v);
  Filter::Connector connector;
  connector.adjacency.set_26_adjacency (do_26_connectivity);
  connector.adjacency.initialise (mask_header, *v2v);
  const size_t num_voxels = v2v->size();

  // Read file names and check files exist
  CohortDataImport importer;
  importer.initialise<SubjectVoxelImport> (argument[0]);
  for (size_t i = 0; i != importer.size(); ++i) {
    if (!dimensions_match (dynamic_cast<SubjectVoxelImport*>(importer[i].get())->header(), mask_header))
      throw Exception ("Image file \"" + importer[i]->name() + "\" does not match analysis mask");
  }
  CONSOLE ("Number of inputs: " + str(importer.size()));

  // Load design matrix
  const matrix_type design = load_matrix<value_type> (argument[1]);
  if (design.rows() != (ssize_t)importer.size())
    throw Exception ("Number of input files does not match number of rows in design matrix");

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
  const ssize_t num_factors = design.cols() + extra_columns.size();
  CONSOLE ("Number of factors: " + str(num_factors));
  if (extra_columns.size()) {
    CONSOLE ("Number of element-wise design matrix columns: " + str(extra_columns.size()));
    if (nans_in_columns)
      CONSOLE ("Non-finite values detected in element-wise design matrix columns; individual rows will be removed from voxel-wise design matrices accordingly");
  }
  check_design (design, extra_columns.size());

  // Load variance groups
  auto variance_groups = GLM::load_variance_groups (design.rows());
  const size_t num_vgs = variance_groups.size() ? variance_groups.maxCoeff()+1 : 1;
  if (num_vgs > 1)
    CONSOLE ("Number of variance groups: " + str(num_vgs));

  // Load hypotheses
  const vector<Hypothesis> hypotheses = Math::Stats::GLM::load_hypotheses (argument[2]);
  const size_t num_hypotheses = hypotheses.size();
  if (hypotheses[0].cols() != num_factors)
    throw Exception ("The number of columns in the contrast matrix (" + str(hypotheses[0].cols()) + ")"
                     + " does not equal the number of columns in the design matrix (" + str(design.cols()) + ")"
                     + (extra_columns.size() ? " (taking into account the " + str(extra_columns.size()) + " uses of -column)" : ""));
  CONSOLE ("Number of hypotheses: " + str(num_hypotheses));

  matrix_type data (importer.size(), num_voxels);
  {
    // Load images
    ProgressBar progress ("loading input images", importer.size());
    for (size_t subject = 0; subject < importer.size(); subject++) {
      (*importer[subject]) (data.row (subject));
      progress++;
    }
  }
  const bool nans_in_data = !data.allFinite();
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
  output_header.keyval()["nonstationary adjustment"] = str(do_nonstationarity_adjustment);
  if (use_tfce) {
    output_header.keyval()["tfce_dh"] = str(tfce_dh);
    output_header.keyval()["tfce_e"] = str(tfce_E);
    output_header.keyval()["tfce_h"] = str(tfce_H);
  } else {
    output_header.keyval()["threshold"] = str(cluster_forming_threshold);
  }

  const std::string prefix (argument[4]);

  // Only add contrast matrix row number to image outputs if there's more than one hypothesis
  auto postfix = [&] (const size_t i) { return (num_hypotheses > 1) ? ("_" + hypotheses[i].name()) : ""; };

  {
    matrix_type betas (num_factors, num_voxels);
    matrix_type abs_effect_size (num_voxels, num_hypotheses);
    matrix_type std_effect_size (num_voxels, num_hypotheses);
    matrix_type stdev (num_vgs, num_voxels);
    vector_type cond (num_voxels);

    Math::Stats::GLM::all_stats (data, design, extra_columns, hypotheses, variance_groups,
                                 cond, betas, abs_effect_size, std_effect_size, stdev);

    ProgressBar progress ("Outputting beta coefficients, effect size and standard deviation", num_factors + (2 * num_hypotheses) + num_vgs + (nans_in_data || extra_columns.size() ? 1 : 0));
    for (ssize_t i = 0; i != num_factors; ++i) {
      write_output (betas.row(i), *v2v, prefix + "beta" + str(i) + ".mif", output_header);
      ++progress;
    }
    for (size_t i = 0; i != num_hypotheses; ++i) {
      if (!hypotheses[i].is_F()) {
        write_output (abs_effect_size.col(i), *v2v, prefix + "abs_effect" + postfix(i) + ".mif", output_header);
        ++progress;
        if (num_vgs == 1)
          write_output (std_effect_size.col(i), *v2v, prefix + "std_effect" + postfix(i) + ".mif", output_header);
      } else {
        ++progress;
      }
      ++progress;
    }
    if (nans_in_data || extra_columns.size()) {
      write_output (cond, *v2v, prefix + "cond.mif", output_header);
      ++progress;
    }
    if (num_vgs == 1) {
      write_output (stdev.row(0), *v2v, prefix + "std_dev.mif", output_header);
    } else {
      for (size_t i = 0; i != num_vgs; ++i) {
        write_output (stdev.row(i), *v2v, prefix + "std_dev.mif", output_header);
        ++progress;
      }
    }
  }

  // Construct the class for performing the initial statistical tests
  std::shared_ptr<GLM::TestBase> glm_test;
  if (extra_columns.size() || nans_in_data) {
    if (variance_groups.size())
      glm_test.reset (new GLM::TestVariableHeteroscedastic (extra_columns, data, design, hypotheses, variance_groups, nans_in_data, nans_in_columns));
    else
      glm_test.reset (new GLM::TestVariableHomoscedastic (extra_columns, data, design, hypotheses, nans_in_data, nans_in_columns));
  } else {
    if (variance_groups.size())
      glm_test.reset (new GLM::TestFixedHeteroscedastic (data, design, hypotheses, variance_groups));
    else
      glm_test.reset (new GLM::TestFixedHomoscedastic (data, design, hypotheses));
  }

  std::shared_ptr<Stats::EnhancerBase> enhancer;
  if (use_tfce) {
    std::shared_ptr<Stats::TFCE::EnhancerBase> base (new Stats::Cluster::ClusterSize (connector, cluster_forming_threshold));
    enhancer.reset (new Stats::TFCE::Wrapper (base, tfce_dh, tfce_E, tfce_H));
  } else {
    enhancer.reset (new Stats::Cluster::ClusterSize (connector, cluster_forming_threshold));
  }

  matrix_type empirical_enhanced_statistic;
  if (do_nonstationarity_adjustment) {
    if (!use_tfce)
      throw Exception ("Nonstationarity adjustment is not currently implemented for threshold-based cluster analysis");
    Stats::PermTest::precompute_empirical_stat (glm_test, enhancer, empirical_skew, empirical_enhanced_statistic);
    for (size_t i = 0; i != num_hypotheses; ++i)
      write_output (empirical_enhanced_statistic.col(i), *v2v, prefix + "empirical" + postfix(i) + ".mif", output_header);
  }

  // Precompute statistic value and enhanced statistic for the default permutation
  matrix_type default_statistic, default_zstat, default_enhanced;
  Stats::PermTest::precompute_default_permutation (glm_test, enhancer, empirical_enhanced_statistic, default_statistic, default_zstat, default_enhanced);
  for (size_t i = 0; i != num_hypotheses; ++i) {
    write_output (default_statistic.col (i), *v2v, prefix + (hypotheses[i].is_F() ? "F" : "t") + "value" + postfix(i) + ".mif", output_header);
    write_output (default_zstat    .col (i), *v2v, prefix + "Zstat" + postfix(i) + ".mif", output_header);
    write_output (default_enhanced .col (i), *v2v, prefix + (use_tfce ? "tfce" : "clustersize") + postfix(i) + ".mif", output_header);
  }

  if (!get_options ("notest").size()) {

    const bool fwe_strong = get_option_value ("strong", false);
    if (fwe_strong && num_hypotheses == 1) {
      WARN("Option -strong has no effect when testing a single hypothesis only");
    }

    matrix_type null_distribution, uncorrected_pvalue;
    count_matrix_type null_contributions;

    Stats::PermTest::run_permutations (glm_test, enhancer, empirical_enhanced_statistic, default_enhanced, fwe_strong,
                                       null_distribution, null_contributions, uncorrected_pvalue);

    ProgressBar progress ("Outputting final results", (fwe_strong ? 1 : num_hypotheses) + 1 + 3*num_hypotheses);

    if (fwe_strong) {
      save_vector (null_distribution.col(0), prefix + "null_dist.txt");
      ++progress;
    } else {
      for (size_t i = 0; i != num_hypotheses; ++i) {
        save_vector (null_distribution.col(i), prefix + "null_dist" + postfix(i) + ".txt");
        ++progress;
      }
    }

    const matrix_type fwe_pvalue_output = MR::Math::Stats::fwe_pvalue (null_distribution, default_enhanced);
    ++progress;
    for (size_t i = 0; i != num_hypotheses; ++i) {
      write_output (fwe_pvalue_output.col(i), *v2v, prefix + "fwe_1mpvalue" + postfix(i) + ".mif", output_header);
      ++progress;
      write_output (uncorrected_pvalue.col(i), *v2v, prefix + "uncorrected_pvalue" + postfix(i) + ".mif", output_header);
      ++progress;
      write_output (null_contributions.col(i), *v2v, prefix + "null_contributions" + postfix(i) + ".mif", output_header);
      ++progress;
    }

  }
}

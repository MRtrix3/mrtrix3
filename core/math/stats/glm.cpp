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

#include "math/stats/glm.h"

#include "debug.h"
#include "thread_queue.h"
#include "math/betainc.h"
#include "math/erfinv.h"
#include "math/welch_satterthwaite.h"
#include "misc/bitset.h"

#define MRTRIX_USE_ZSTATISTIC_LOOKUP

//#define GLM_ALL_STATS_DEBUG

namespace MR
{
  namespace Math
  {
    namespace Stats
    {
      namespace GLM
      {



        const char* const column_ones_description =
            "In some software packages, a column of ones is automatically added to the "
            "GLM design matrix; the purpose of this column is to estimate the \"global "
            "intercept\", which is the predicted value of the observed variable if all "
            "explanatory variables were to be zero. However there are rare situations "
            "where including such a column would not be appropriate for a particular "
            "experimental design. Hence, in MRtrix3 statistical inference commands, "
            "it is up to the user to determine whether or not this column of ones should "
            "be included in their design matrix, and add it explicitly if necessary. "
            "The contrast matrix must also reflect the presence of this additional column.";


        App::OptionGroup glm_options (const std::string& element_name)
        {
          using namespace App;
          OptionGroup result = OptionGroup ("Options related to the General Linear Model (GLM)")

            + Option ("variance", "define variance groups for the G-statistic; "
                                  "measurements for which the expected variance is equivalent should contain the same index")
              + Argument ("file").type_file_in()

            + Option ("ftests", "perform F-tests; input text file should contain, for each F-test, a row containing "
                                "ones and zeros, where ones indicate the rows of the contrast matrix to be included "
                                "in the F-test.")
              + Argument ("path").type_file_in()

            + Option ("fonly", "only assess F-tests; do not perform statistical inference on entries in the contrast matrix")

            + Option ("column", "add a column to the design matrix corresponding to subject " + element_name + "-wise values "
                                "(note that the contrast matrix must include an additional column for each use of this option); "
                                "the text file provided via this option should contain a file name for each subject").allow_multiple()
              + Argument ("path").type_file_in();

          return result;
        }




        void check_design (const matrix_type& design, const bool extra_factors)
        {
          Eigen::ColPivHouseholderQR<matrix_type> decomp;
          decomp.setThreshold (1e-5);
          decomp = decomp.compute (design);
          if (decomp.rank() < design.cols()) {
            if (extra_factors) {
              CONSOLE ("Design matrix is rank-deficient before addition of element-wise columns");
            } else {
              WARN ("Design matrix is rank-deficient; processing may proceed, but manually checking your matrix is advised");
            }
          } else {
            const default_type cond = Math::condition_number (design);
            if (cond > 100.0) {
              if (extra_factors) {
                CONSOLE ("Design matrix conditioning is poor (condition number: " + str(cond, 6) + ") before the addition of element-wise columns");
              } else {
                WARN ("Design matrix conditioning is poor (condition number: " + str(cond, 6) + "); model fitting may be highly influenced by noise");
              }
            } else {
              CONSOLE (std::string ("Design matrix condition number") + (extra_factors ? " (without element-wise columns)" : "") + ": " + str(cond, 6));
            }
          }
        }



        index_array_type load_variance_groups (const size_t num_inputs)
        {
          auto opt = App::get_options ("variance");
          if (!opt.size())
            return index_array_type();
          try {
            auto data = load_vector<size_t> (opt[0][0]);
            if (size_t(data.size()) != num_inputs)
              throw Exception ("Number of entries in variance group file \"" + std::string(opt[0][0]) + "\" (" + str(data.size()) + ") does not match number of inputs (" + str(num_inputs) + ")");
            const size_t min_coeff = data.minCoeff();
            const size_t max_coeff = data.maxCoeff();
            if (min_coeff > 1)
              throw Exception ("Minimum coefficient needs to be either zero or one");
            if (max_coeff == min_coeff) {
              WARN ("Only a single variance group is defined in file \"" + opt[0][0] + "\"; variance groups will not be used");
              return index_array_type();
            }
            vector<size_t> count_per_group (max_coeff + 1, 0);
            for (size_t i = 0; i != size_t(data.size()); ++i)
              count_per_group[data[i]]++;
            for (size_t vg_index = min_coeff; vg_index <= size_t(max_coeff); ++vg_index) {
              if (!count_per_group[vg_index])
                throw Exception ("No entries found for variance group " + str(vg_index));
            }
            if (min_coeff)
              data.array() -= 1;
            return data.array();
          } catch (Exception& e) {
            throw Exception (e, "unable to read file \"" + opt[0][0] + "\" as variance group data");
          }
        }




        vector<Hypothesis> load_hypotheses (const std::string& file_path)
        {
          vector<Hypothesis> hypotheses;
          const matrix_type contrast_matrix = load_matrix (file_path);
          for (ssize_t row = 0; row != contrast_matrix.rows(); ++row)
            hypotheses.emplace_back (Hypothesis (contrast_matrix.row (row), row));
          auto opt = App::get_options ("ftests");
          if (opt.size()) {
            const matrix_type ftest_matrix = load_matrix (opt[0][0]);
            if (ftest_matrix.cols() != contrast_matrix.rows())
              throw Exception ("Number of columns in F-test matrix (" + str(ftest_matrix.cols()) + ") does not match number of rows in contrast matrix (" + str(contrast_matrix.rows()) + ")");
            if (!((ftest_matrix.array() == 0.0) + (ftest_matrix.array() == 1.0)).all())
              throw Exception ("F-test array must contain ones and zeros only");
            for (ssize_t ftest_index = 0; ftest_index != ftest_matrix.rows(); ++ftest_index) {
              if (!ftest_matrix.row (ftest_index).count())
                throw Exception ("Row " + str(ftest_index+1) + " of F-test matrix does not contain any ones");
              matrix_type this_f_matrix (ftest_matrix.row (ftest_index).count(), contrast_matrix.cols());
              ssize_t ftest_row = 0;
              for (ssize_t contrast_row = 0; contrast_row != contrast_matrix.rows(); ++contrast_row) {
                if (ftest_matrix (ftest_index, contrast_row))
                  this_f_matrix.row (ftest_row++) = contrast_matrix.row (contrast_row);
              }
              hypotheses.emplace_back (Hypothesis (this_f_matrix, ftest_index));
            }
            if (App::get_options ("fonly").size()) {
              vector<Hypothesis> new_hypotheses;
              for (size_t index = contrast_matrix.rows(); index != hypotheses.size(); ++index)
                new_hypotheses.push_back (std::move (hypotheses[index]));
              std::swap (hypotheses, new_hypotheses);
            }
          } else if (App::get_options ("fonly").size()) {
            throw Exception ("Cannot perform F-tests exclusively (-fonly option): No F-test matrix was provided (-ftests option)");
          }
          return hypotheses;
        }






        matrix_type solve_betas (const matrix_type& measurements, const matrix_type& design)
        {
          return design.jacobiSvd (Eigen::ComputeThinU | Eigen::ComputeThinV).solve (measurements);
        }



        vector_type abs_effect_size (const matrix_type& measurements, const matrix_type& design, const Hypothesis& hypothesis)
        {
          if (hypothesis.is_F())
            return vector_type::Constant (measurements.rows(), NaN);
          else
            return hypothesis.matrix() * solve_betas (measurements, design);
        }

        matrix_type abs_effect_size (const matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses)
        {
          matrix_type result (measurements.cols(), hypotheses.size());
          for (size_t ic = 0; ic != hypotheses.size(); ++ic)
            result.col (ic) = abs_effect_size (measurements, design, hypotheses[ic]);
          return result;
        }



        matrix_type stdev (const matrix_type& measurements, const matrix_type& design)
        {
          const matrix_type residuals = measurements - design * solve_betas (measurements, design);
          const matrix_type sse = residuals.colwise().squaredNorm();
          return (sse.array() / value_type(design.rows()-Math::rank (design))).sqrt();
        }


        matrix_type stdev (const matrix_type& measurements, const matrix_type& design, const index_array_type& variance_groups)
        {
          assert (measurements.rows() == design.rows());
          if (!variance_groups.size())
            return stdev (measurements, design);
          assert (measurements.rows() == variance_groups.rows());
          // Residual-forming matrix
          const matrix_type R (matrix_type::Identity (design.rows(), design.rows()) - (design * Math::pinv (design)));
          // Residuals
          const matrix_type e (R * measurements);
          // One standard deviation per element per variance group
          // Rows are variance groups, columns are elements
          const size_t num_vgs = variance_groups.array().maxCoeff() + 1;
          // Sum of residual-forming matrix diagonal elements within each variance group
          //   will be equivalent across elements
          vector_type Rnn_sums (vector_type::Zero (num_vgs));
          for (ssize_t i = 0; i != measurements.rows(); ++i)
            Rnn_sums[variance_groups[i]] += R.diagonal()[i];
          // For each variance group, get the sum of squared residuals within that group
          matrix_type result (num_vgs, measurements.cols());
          for (ssize_t ie = 0; ie != measurements.cols(); ++ie) {
            vector_type sse (vector_type::Zero (num_vgs));
            for (ssize_t i = 0; i != measurements.rows(); ++i)
              sse[variance_groups[i]] += Math::pow2 (e (i, ie));
            // (Rnn_sum / sse) is the inverse of the estimated variance
            result.col (ie) = (sse.array() / Rnn_sums.array()).sqrt();
          }
          return result;
        }



        vector_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const Hypothesis& hypothesis)
        {
          if (hypothesis.is_F())
            return vector_type::Constant (measurements.cols(), NaN);
          return abs_effect_size (measurements, design, hypothesis).array() / stdev (measurements, design).array().col(0);
        }

        matrix_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses)
        {
          const auto stdev_reciprocal = vector_type::Ones (measurements.cols()) / stdev (measurements, design).array().col(0);
          matrix_type result (measurements.cols(), hypotheses.size());
          for (size_t ic = 0; ic != hypotheses.size(); ++ic)
            result.col (ic) = abs_effect_size (measurements, design, hypotheses[ic]) * stdev_reciprocal;
          return result;
        }






        void all_stats (const matrix_type& measurements,
                        const matrix_type& design,
                        const vector<Hypothesis>& hypotheses,
                        const index_array_type& variance_groups,
                        matrix_type& betas,
                        matrix_type& abs_effect_size,
                        matrix_type& std_effect_size,
                        matrix_type& stdev)
        {
#ifndef GLM_ALL_STATS_DEBUG
          // If this function is being invoked from the other version of all_stats(),
          //   on an element-by-element basis, don't interfere with the progress bar
          //   that's being displayed by that outer looping function
          std::unique_ptr<ProgressBar> progress;
          if (measurements.cols() > 1)
            progress.reset (new ProgressBar ("Calculating basic properties of default permutation", 5));
#endif
          betas = solve_betas (measurements, design);
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "Betas: " << betas.rows() << " x " << betas.cols() << ", max " << betas.array().maxCoeff() << "\n";
#else
          if (progress)
            ++*progress;
#endif
          abs_effect_size.resize (measurements.cols(), hypotheses.size());
          for (size_t ic = 0; ic != hypotheses.size(); ++ic) {
            if (hypotheses[ic].is_F()) {
              abs_effect_size.col (ic).fill (NaN);
            } else {
              abs_effect_size.col (ic) = (hypotheses[ic].matrix() * betas).row (0);
            }
          }
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "abs_effect_size: " << abs_effect_size.rows() << " x " << abs_effect_size.cols() << ", max " << abs_effect_size.array().maxCoeff() << "\n";
#else
          if (progress)
            ++*progress;
#endif
          // Explicit calculation of residuals before SSE, rather than in a single
          //   step, appears to be necessary for compatibility with Eigen 3.2.0
          const matrix_type residuals = (measurements - design * betas);
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "Residuals: " << residuals.rows() << " x " << residuals.cols() << ", max " << residuals.array().maxCoeff() << "\n";
#else
          if (progress)
            ++*progress;
#endif
          stdev = GLM::stdev (measurements, design, variance_groups);
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "stdev: " << stdev.rows() << " x " << stdev.cols() << ", max " << stdev.maxCoeff() << "\n";
#else
          if (progress)
            ++*progress;
#endif
          if (variance_groups.size())
            std_effect_size = matrix_type::Constant (measurements.cols(), hypotheses.size(), NaN);
          else
            std_effect_size = abs_effect_size.array().colwise() / stdev.transpose().array().col(0);
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "std_effect_size: " << std_effect_size.rows() << " x " << std_effect_size.cols() << ", max " << std_effect_size.array().maxCoeff() << "\n";
#endif
        }



        void all_stats (const matrix_type& measurements,
                        const matrix_type& fixed_design,
                        const vector<CohortDataImport>& extra_data,
                        const vector<Hypothesis>& hypotheses,
                        const index_array_type& variance_groups,
                        vector_type& cond,
                        matrix_type& betas,
                        matrix_type& abs_effect_size,
                        matrix_type& std_effect_size,
                        matrix_type& stdev)
        {
          if (extra_data.empty() && measurements.allFinite()) {
            all_stats (measurements, fixed_design, hypotheses, variance_groups, betas, abs_effect_size, std_effect_size, stdev);
            return;
          }

          class Source
          { NOMEMALIGN
            public:
              Source (const size_t num_elements) :
                  num_elements (num_elements),
                  counter (0),
                  progress (new ProgressBar ("Calculating basic properties of default permutation", num_elements)) { }
              bool operator() (size_t& element_index)
              {
                element_index = counter++;
                if (element_index >= num_elements) {
                  progress.reset();
                  return false;
                }
                assert (progress);
                ++(*progress);
                return true;
              }
            private:
              const size_t num_elements;
              size_t counter;
              std::unique_ptr<ProgressBar> progress;
          };

          class Functor
          { MEMALIGN(Functor)
            public:
              Functor (const matrix_type& data, const matrix_type& design_fixed, const vector<CohortDataImport>& extra_data, const vector<Hypothesis>& hypotheses, const index_array_type& variance_groups,
                       vector_type& cond, matrix_type& betas, matrix_type& abs_effect_size, matrix_type& std_effect_size, matrix_type& stdev) :
                  data (data),
                  design_fixed (design_fixed),
                  extra_data (extra_data),
                  hypotheses (hypotheses),
                  variance_groups (variance_groups),
                  global_cond (cond),
                  global_betas (betas),
                  global_abs_effect_size (abs_effect_size),
                  global_std_effect_size (std_effect_size),
                  global_stdev (stdev),
                  num_vgs (variance_groups.size() ? variance_groups.maxCoeff()+1 : 1)
              {
                assert (size_t(design_fixed.cols()) + extra_data.size() == size_t(hypotheses[0].cols()));
              }
              bool operator() (const size_t& element_index)
              {
                const matrix_type element_data = data.col (element_index);
                matrix_type element_design (design_fixed.rows(), design_fixed.cols() + extra_data.size());
                element_design.leftCols (design_fixed.cols()) = design_fixed;
                // For each element-wise design matrix column,
                //   acquire the data for this particular element, without permutation
                for (size_t col = 0; col != extra_data.size(); ++col)
                  element_design.col (design_fixed.cols() + col) = (extra_data[col]) (element_index);
                // For each element-wise design matrix, remove any NaN values
                //   present in either the input data or imported from the element-wise design matrix column data
                ssize_t valid_rows = 0;
                for (ssize_t row = 0; row != data.rows(); ++row) {
                  if (std::isfinite (element_data(row)) && element_design.row (row).allFinite())
                    ++valid_rows;
                }
                default_type condition_number = 0.0;
                if (valid_rows == data.rows()) { // No NaNs present
                  condition_number = Math::condition_number (element_design);
                  if (!std::isfinite (condition_number) || condition_number > 1e5) {
                    zero();
                  } else {
                    Math::Stats::GLM::all_stats (element_data, element_design, hypotheses, variance_groups,
                                                 local_betas, local_abs_effect_size, local_std_effect_size, local_stdev);
                  }
                } else if (valid_rows >= element_design.cols()) {
                  // Need to reduce the data and design matrices to contain only finite data
                  matrix_type element_data_finite (valid_rows, 1);
                  matrix_type element_design_finite (valid_rows, element_design.cols());
                  index_array_type variance_groups_finite (variance_groups.size() ? valid_rows : 0);
                  ssize_t output_row = 0;
                  for (ssize_t row = 0; row != data.rows(); ++row) {
                    if (std::isfinite (element_data(row)) && element_design.row (row).allFinite()) {
                      element_data_finite(output_row, 0) = element_data(row);
                      element_design_finite.row (output_row) = element_design.row (row);
                      if (variance_groups.size())
                        variance_groups_finite[output_row] = variance_groups[row];
                      ++output_row;
                    }
                  }
                  assert (output_row == valid_rows);
                  assert (element_data_finite.allFinite());
                  assert (element_design_finite.allFinite());
                  condition_number = Math::condition_number (element_design_finite);
                  if (!std::isfinite (condition_number) || condition_number > 1e5) {
                    zero();
                  } else {
                    Math::Stats::GLM::all_stats (element_data_finite, element_design_finite, hypotheses, variance_groups_finite,
                                                 local_betas, local_abs_effect_size, local_std_effect_size, local_stdev);
                  }
                } else { // Insufficient data to fit model at all
                  zero();
                }
                global_cond[element_index] = condition_number;
                global_betas.col (element_index) = local_betas;
                global_abs_effect_size.row (element_index) = local_abs_effect_size.row (0);
                global_std_effect_size.row (element_index) = local_std_effect_size.row (0);
                global_stdev.col (element_index) = local_stdev;
                return true;
              }
            private:
              const matrix_type& data;
              const matrix_type& design_fixed;
              const vector<CohortDataImport>& extra_data;
              const vector<Hypothesis>& hypotheses;
              const index_array_type& variance_groups;
              vector_type& global_cond;
              matrix_type& global_betas;
              matrix_type& global_abs_effect_size;
              matrix_type& global_std_effect_size;
              matrix_type& global_stdev;
              matrix_type local_betas, local_abs_effect_size, local_std_effect_size, local_stdev;
              const size_t num_vgs;

              void zero () {
                local_betas = matrix_type::Zero (global_betas.rows(), 1);
                local_abs_effect_size = matrix_type::Zero (1, hypotheses.size());
                local_std_effect_size = matrix_type::Zero (1, hypotheses.size());
                local_stdev = matrix_type::Zero (num_vgs, 1);
                for (size_t ih = 0; ih != hypotheses.size(); ++ih) {
                  if (hypotheses[ih].is_F())
                    local_abs_effect_size (0, ih) = local_std_effect_size (0, ih) = NaN;
                }
              }
          };

          Source source (measurements.cols());
          Functor functor (measurements, fixed_design, extra_data, hypotheses, variance_groups,
                           cond, betas, abs_effect_size, std_effect_size, stdev);
          Thread::run_queue (source, Thread::batch (size_t()), Thread::multi (functor));
        }










        // Same model partitioning as is used in FSL randomise
        Hypothesis::Partition Hypothesis::partition (const matrix_type& design) const
        {
          // eval() calls necessary for older versions of Eigen / compiler to work:
          //   can't seem to map Eigen template result to const matrix_type& as the Math::pinv() input
          // TODO See if some better template trickery can be done
          const matrix_type D = (design.transpose() * design).inverse();
          // Note: Cu is transposed with respect to how contrast matrices are stored elsewhere
          const matrix_type Cu = Eigen::FullPivLU<matrix_type> (c).kernel();
          const matrix_type inv_cDc = (c * D * c.transpose()).inverse();
          // Note: Cv is transposed with respect to convention just as Cu is
          const matrix_type Cv = Cu - c.transpose() * inv_cDc * c * D * Cu;
          const matrix_type X = design * D * c.transpose() * inv_cDc;
          // .inverse() leads to NaNs with no nuisance regressors
          const matrix_type Z = Cv.isZero() ?
                                matrix_type::Zero (design.rows(), 1) :
                                (design * D * Cv * (Cv.transpose() * D * Cv).inverse()).eval();
          return Partition (X, Z);
        }



        void Hypothesis::check_nonzero() const
        {
          if (c.isZero())
            throw Exception ("Cannot specify a contrast that consists entirely of zeroes");
        }



        matrix_type Hypothesis::check_rank (const matrix_type& in, const size_t index) const
        {
          // FullPivLU.image() provides column-space of matrix;
          //   here we want the row-space (since it's degeneracy in contrast matrix rows
          //   that has led to the rank-deficiency, whereas we can't exclude factor columns).
          //   Hence the transposing.
          Eigen::FullPivLU<matrix_type> decomp (in.transpose());
          if (decomp.rank() == in.rows())
            return in;
          WARN ("F-test " + str(index+1) + " is rank-deficient; row-space matrix decomposition will instead be used");
          INFO ("Original matrix: " + str(in));
          const matrix_type result = decomp.image (in.transpose()).transpose();
          INFO ("Decomposed matrix: " + str(result));
          return result;
        }









        void TestBase::operator() (const matrix_type& shuffling_matrix, matrix_type& output) const
        {
          matrix_type temp;
          (*this) (shuffling_matrix, temp, output);
        }










//#define GLM_TEST_DEBUG



        TestFixedHomoscedastic::TestFixedHomoscedastic (const matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses) :
            TestBase (measurements, design, hypotheses),
            pinvM (Math::pinv (M)),
            Rm (matrix_type::Identity (num_inputs(), num_inputs()) - (M*pinvM))
        {
          assert (hypotheses[0].cols() == design.cols());
          // When the design matrix is fixed, we can pre-calculate the model partitioning for each hypothesis
          for (const auto& h : hypotheses) {
            partitions.emplace_back (h.partition (design));
            XtX.emplace_back (partitions.back().X.transpose()*partitions.back().X);
            one_over_dof.push_back (1.0 / (num_inputs() - partitions.back().rank_x - partitions.back().rank_z));
          }
        }



        void TestFixedHomoscedastic::operator() (const matrix_type& shuffling_matrix,
                                                matrix_type& stats,
                                                matrix_type& zstats) const
        {
          assert (size_t(shuffling_matrix.rows()) == num_inputs());
          stats .resize (num_elements(), num_hypotheses());
          zstats.resize (num_elements(), num_hypotheses());

          matrix_type Sy, lambdas, residuals, beta;
          vector_type sse;

          // Freedman-Lane for fixed design matrix case
          // Each hypothesis needs to be handled explicitly on its own
          for (size_t ih = 0; ih != c.size(); ++ih) {

            // First, we perform permutation of the input data
            // In Freedman-Lane, the initial 'effective' regression against the nuisance
            //   variables, and permutation of the data, are done in a single step
#ifdef GLM_TEST_DEBUG
            VAR (shuffling_matrix.rows());
            VAR (shuffling_matrix.cols());
            VAR (partitions[ih].Rz.rows());
            VAR (partitions[ih].Rz.cols());
            VAR (y.rows());
            VAR (y.cols());
#endif
            Sy.noalias() = shuffling_matrix * partitions[ih].Rz * y;
#ifdef GLM_TEST_DEBUG
            VAR (Sy.rows());
            VAR (Sy.cols());
            VAR (pinvM.rows());
            VAR (pinvM.cols());
#endif
            // Now, we regress this shuffled data against the full model
            lambdas.noalias() = pinvM * Sy;
#ifdef GLM_TEST_DEBUG
            VAR (lambdas.rows());
            VAR (lambdas.cols());
            //VAR (matrix_type(c[ih]).rows());
            //VAR (matrix_type(c[ih]).cols());
            VAR (Rm.rows());
            VAR (Rm.cols());
            VAR (XtX[ih].rows());
            VAR (XtX[ih].cols());
            VAR (one_over_dof);
#endif
            const size_t dof = num_inputs() - partitions[ih].rank_x - partitions[ih].rank_z;
            const default_type one_over_dof = 1.0 / default_type(dof);
            sse = (Rm*Sy).colwise().squaredNorm();
#ifdef GLM_TEST_DEBUG
            VAR (dof);
            VAR (one_over_dof);
            VAR (sse.size());
#endif
            for (size_t ie = 0; ie != num_elements(); ++ie) {
              beta.noalias() = c[ih].matrix() * lambdas.col (ie);
              const default_type F = ((beta.transpose() * XtX[ih] * beta) (0,0) / c[ih].rank()) /
                                     (one_over_dof * sse[ie]);
              if (!std::isfinite (F)) {
                stats  (ie, ih) = zstats (ie, ih) = value_type(0);
              } else if (c[ih].is_F()) {
                stats  (ie, ih) = F;
#ifdef MRTRIX_USE_ZSTATISTIC_LOOKUP
                zstats (ie, ih) = stat2z->F2z (F, c[ih].rank(), dof);
#else
                zstats (ie, ih) = Math::F2z (F, c[ih].rank(), dof);
#endif
              } else {
                assert (beta.rows() == 1);
                stats  (ie, ih) = std::sqrt (F) * (beta.sum() > 0.0 ? 1.0 : -1.0);
#ifdef MRTRIX_USE_ZSTATISTIC_LOOKUP
                zstats (ie, ih) = stat2z->t2z (stats (ie, ih), dof);
#else
                zstats (ie, ih) = Math::t2z (stats (ie, ih), dof);
#endif
              }
            }

          }
        }









        TestFixedHeteroscedastic::TestFixedHeteroscedastic (const matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses, const index_array_type& variance_groups) :
            TestFixedHomoscedastic (measurements, design, hypotheses),
            VG (variance_groups),
            num_vgs (VG.maxCoeff() + 1),
            inputs_per_vg (num_vgs, 0),
            Rnn_sums (vector_type::Zero (num_vgs)),
            gamma_weights (vector_type::Zero (num_hypotheses()))
        {
          // Pre-calculate whatever can be pre-calculated for G-statistic
          for (size_t input = 0; input != num_inputs(); ++input) {
            // Number of inputs belonging to each VG
            inputs_per_vg[VG[input]]++;
            // Sum of diagonal entries of residual-forming matrix corresponding to each VG
            Rnn_sums[VG[input]] += Rm.diagonal()[input];
          }
#ifdef GLM_TEST_DEBUG
          VAR (inputs_per_vg);
          VAR (Rnn_sums);
#endif
          // Reciprocals of the sums of diagonal entries of residual-forming matrix corresponding to each VG
          inv_Rnn_sums = Rnn_sums.inverse();
#ifdef GLM_TEST_DEBUG
          VAR (inv_Rnn_sums);
#endif
          // Multiplication term for calculation of gamma; unique for each hypothesis
          for (size_t ih = 0; ih != c.size(); ++ih) {
            const size_t s = c[ih].rank();
            gamma_weights[ih] = 2.0*(s-1) / default_type(s*(s+2));
          }
#ifdef GLM_TEST_DEBUG
          VAR (gamma_weights);
#endif
        }



        void TestFixedHeteroscedastic::operator() (const matrix_type& shuffling_matrix, matrix_type& stats, matrix_type& zstats) const
        {
          assert (size_t(shuffling_matrix.rows()) == num_inputs());
          stats.resize (num_elements(), num_hypotheses());
          zstats.resize (num_elements(), num_hypotheses());

          matrix_type Sy, lambdas;
          Eigen::Array<default_type, Eigen::Dynamic, Eigen::Dynamic> sq_residuals, sse, Wterms;
          Eigen::Matrix<default_type, Eigen::Dynamic, 1> W (num_inputs());
#ifdef GLM_TEST_DEBUG
          VAR (shuffling_matrix);
#endif

          for (size_t ih = 0; ih != c.size(); ++ih) {
            // First two steps are identical to the homoscedastic case
            Sy.noalias() = shuffling_matrix * partitions[ih].Rz * y;
#ifdef GLM_TEST_DEBUG
            VAR (Sy);
#endif
            lambdas.noalias() = pinvM * Sy;
#ifdef GLM_TEST_DEBUG
            VAR (lambdas);
#endif
            // Compute sum of residuals per VG immediately
            // Variance groups appear across rows, and one column per element tested
            // Immediately calculate squared residuals; simplifies summation over variance groups
            sq_residuals = (Rm*Sy).array().square();
#ifdef GLM_TEST_DEBUG
            VAR (sq_residuals);
            VAR (sq_residuals.rows());
            VAR (sq_residuals.cols());
#endif
            sse = matrix_type::Zero (num_variance_groups(), num_elements());
            for (size_t input = 0; input != num_inputs(); ++input)
              sse.row(VG[input]) += sq_residuals.row(input);
#ifdef GLM_TEST_DEBUG
            VAR (sse);
            VAR (sse.rows());
            VAR (sse.cols());
#endif
            // These terms are what appears in the weighting matrix based on the VG to which each input belongs;
            //   one row per variance group, one column per element to be tested
            Wterms = sse.array().inverse().colwise() * Rnn_sums;
            for (size_t col = 0; col != num_elements(); ++col) {
              for (size_t row = 0; row != num_vgs; ++row) {
                if (!std::isfinite (Wterms (row, col)))
                  Wterms (row, col) = 0.0;
              }
            }
#ifdef GLM_TEST_DEBUG
            VAR (Wterms);
            VAR (Wterms.rows());
            VAR (Wterms.cols());
#endif
            for (size_t ie = 0; ie != num_elements(); ++ie) {
              // Need to construct the weights diagonal matrix; is unique for each element
              default_type W_trace (0.0);
              for (size_t input = 0; input != num_inputs(); ++input) {
                W[input] = Wterms(VG[input], ie);
                W_trace += W[input];
              }
#ifdef GLM_TEST_DEBUG
              VAR (W_trace);
#endif
              const default_type numerator = lambdas.col (ie).transpose() * c[ih].matrix().transpose() * (c[ih].matrix() * (M.transpose() * W.asDiagonal() * M).inverse() * c[ih].matrix().transpose()).inverse() * c[ih].matrix() * lambdas.col (ie);
#ifdef GLM_TEST_DEBUG
              VAR (numerator);
#endif
              default_type gamma (0.0);
              for (size_t vg_index = 0; vg_index != num_vgs; ++vg_index)
                // Since Wnn is the same for every n in the variance group, can compute that summation as the product of:
                //   - the value inserted in W for that particular VG
                //   - the number of inputs that are a part of that VG
                gamma += inv_Rnn_sums[vg_index] * Math::pow2 (1.0 - ((Wterms(vg_index, ie) * inputs_per_vg[vg_index]) / W_trace));
              gamma = 1.0 + (gamma_weights[ih] * gamma);
#ifdef GLM_TEST_DEBUG
              VAR (gamma);
#endif
              const default_type denominator = gamma * c[ih].rank();
              const default_type G = numerator / denominator;
              if (!std::isfinite (G)) {
                stats  (ie, ih) = zstats (ie, ih) = value_type(0);
              } else {
                stats  (ie, ih) = c[ih].is_F() ?
                                  G :
                                  std::sqrt (G) * ((c[ih].matrix() * lambdas.col (ie)).sum() > 0.0 ? 1.0 : -1.0);
                if (c[ih].is_F() && c[ih].rank() > 1) {
                  const default_type dof = 2.0 * default_type(c[ih].rank() - 1) / (3.0 * (gamma - 1.0));
#ifdef GLM_TEST_DEBUG
                  VAR (dof);
#endif
                  zstats (ie, ih) = stat2z->F2z (G, c[ih].rank(), dof);
                } else {
                  const default_type dof = Math::welch_satterthwaite (Wterms.col (ie).inverse(), inputs_per_vg);
#ifdef GLM_TEST_DEBUG
                  VAR (dof);
#endif
                  zstats (ie, ih) = c[ih].is_F() ?
#ifdef MRTRIX_USE_ZSTATISTIC_LOOKUP
                                    stat2z->G2z (G, c[ih].rank(), dof) :
                                    stat2z->v2z (stats (ie, ih), dof);
#else
                                    Math::F2z (G, c[ih].rank(), dof) :
                                    Math::t2z (stats (ie, ih), dof);
#endif
                }
              }
            }

          }
        }












        TestVariableHomoscedastic::TestVariableHomoscedastic (const vector<CohortDataImport>& importers,
                                                              const matrix_type& measurements,
                                                              const matrix_type& design,
                                                              const vector<Hypothesis>& hypotheses,
                                                              const bool nans_in_data,
                                                              const bool nans_in_columns) :
            TestBase (measurements, design, hypotheses),
            importers (importers),
            nans_in_data (nans_in_data),
            nans_in_columns (nans_in_columns)
        {
          // Make sure that the specified contrast matrix reflects the full design matrix (with additional
          //   data loaded)
          assert (hypotheses[0].cols() == M.cols() + ssize_t(importers.size()));
        }



        void TestVariableHomoscedastic::operator() (const matrix_type& shuffling_matrix,
                                                    matrix_type& stats,
                                                    matrix_type& zstats) const
        {
          stats .resize (num_elements(), num_hypotheses());
          zstats.resize (num_elements(), num_hypotheses());

          matrix_type dof (num_elements(), num_hypotheses());
          matrix_type extra_column_data (num_inputs(), importers.size());
          BitSet element_mask (num_inputs());
          matrix_type shuffling_matrix_masked, Mfull_masked, pinvMfull_masked, Rm;
          vector_type y_masked, Sy, lambda;
          matrix_type XtX, beta;

          // Let's loop over elements first, then hypotheses in the inner loop
          for (ssize_t ie = 0; ie != y.cols(); ++ie) {

            // For each element (row in y), need to load the additional data for that element
            //   for all subjects in order to construct the design matrix
            // Would it be preferable to pre-calculate and store these per-element design matrices,
            //   rather than re-generating them each time? (More RAM, less CPU)
            // No, most of the time that subject data will be memory-mapped, so pre-loading (in
            //   addition to the duplication of the fixed design matrix contents) would hurt bad
            for (ssize_t col = 0; col != ssize_t(importers.size()); ++col)
              extra_column_data.col (col) = importers[col] (ie);

            // What can we do here that's common across all hypotheses?
            // - Import the element-wise data
            // - Identify rows to be excluded based on NaNs in the design matrix
            // - Identify rows to be excluded based on NaNs in the input data
            //
            // Note that this is going to have to operate slightly differently to
            //   how it used to be done, i.e. via the permutation labelling vector,
            //   if we are to support taking the shuffling matrix as input to this functor
            // I think the approach will have to be:
            //   - Both NaNs in design matrix and NaNs in input data need to be removed
            //     in order to perform the initial regression against nuisance variables
            //   - Can then remove the corresponding _columns_ of the permutation matrix?
            //     No, don't think it's removal of columns; think it's removal of any rows
            //     that contain non-zero values in those columns
            //
            get_mask (ie, element_mask, extra_column_data);
            const size_t finite_count = element_mask.count();
            // Additional rejection here:
            // If the number of finite elemets is _not_ equal to the number of subjects
            //   (i.e. at least one subject has been removed), there needs to be a
            //   more stringent criterion met in order to proceed with the test.
            //   Let's do: DoF must be at least equal to the number of factors.
            if (finite_count < std::min (num_inputs(), 2 * num_factors())) {
              stats.row (ie).setZero();
              zstats.row (ie).setZero();
              dof.row (ie).fill (NaN);
            } else {
              apply_mask (element_mask,
                          y.col (ie),
                          shuffling_matrix,
                          extra_column_data,
                          Mfull_masked,
                          shuffling_matrix_masked,
                          y_masked);
              assert (Mfull_masked.allFinite());

              // Test condition number of NaN-masked & data-filled design matrix;
              //   need to skip statistical testing if it is too poor
              // TODO Condition number testing may be quite slow;
              //   would a rank calculation with tolerance be faster?
              const default_type condition_number = Math::condition_number (Mfull_masked);
              if (!std::isfinite (condition_number) || condition_number > 1e5) {
                stats.row (ie).fill (0.0);
                zstats.row (ie).fill (0.0);
                dof.row (ie).fill (NaN);
              } else {

                pinvMfull_masked = Math::pinv (Mfull_masked);

                Rm.noalias() = matrix_type::Identity (finite_count, finite_count) - (Mfull_masked*pinvMfull_masked);

                // We now have our permutation (shuffling) matrix and design matrix prepared,
                //   and can commence regressing the partitioned model of each hypothesis
                for (size_t ih = 0; ih != c.size(); ++ih) {

                  const auto partition = c[ih].partition (Mfull_masked);
                  dof (ie, ih) = finite_count - partition.rank_x - partition.rank_z;
                  if (dof (ie, ih) < 1) {
                    stats (ie, ih) = zstats (ie, ih) = dof (ie, ih) = value_type(0);
                  } else {

                    XtX.noalias() = partition.X.transpose()*partition.X;

                    // Now that we have the individual hypothesis model partition for these data,
                    //   the rest of this function should proceed similarly to the fixed
                    //   design matrix case
                    Sy = shuffling_matrix_masked * partition.Rz * y_masked.matrix();
                    lambda = pinvMfull_masked * Sy.matrix();
                    beta.noalias() = c[ih].matrix() * lambda.matrix();
                    const default_type sse = (Rm*Sy.matrix()).squaredNorm();

                    const default_type F = ((beta.transpose() * XtX * beta) (0, 0) / c[ih].rank()) /
                                            (sse / dof (ie, ih));

                    if (!std::isfinite (F)) {
                      stats  (ie, ih) = zstats (ie, ih) = value_type(0);
                    } else if (c[ih].is_F()) {
                      stats  (ie, ih) = F;
#ifdef MRTRIX_USE_ZSTATISTIC_LOOKUP
                      zstats (ie, ih) = stat2z->F2z (F, c[ih].rank(), dof (ie, ih));
#else
                      zstats (ie, ih) = Math::F2z (F, c[ih].rank(), dof (ie, ih));
#endif
                    } else {
                      assert (beta.rows() == 1);
                      stats  (ie, ih) = std::sqrt (F) * (beta.sum() > 0 ? 1.0 : -1.0);
#ifdef MRTRIX_USE_ZSTATISTIC_LOOKUP
                      zstats (ie, ih) = stat2z->t2z (stats (ie, ih), dof (ie, ih));
#else
                      zstats (ie, ih) = Math::t2z (stats (ie, ih), dof (ie, ih));
#endif
                    }

                  } // End checking for sufficient degrees of freedom

                } // End looping over hypotheses

              } // End checking for adequate condition number after NaN removal

            } // End checking for adequate number of remaining inputs after NaN removal

          } // End looping over elements

        } // End functor








        void TestVariableHomoscedastic::get_mask (const size_t ie, BitSet& mask, const matrix_type& extra_data) const
        {
          mask.clear (true);
          if (nans_in_data) {
            for (ssize_t row = 0; row != y.rows(); ++row) {
              if (!std::isfinite (y (row, ie)))
                mask[row] = false;
            }
          }
          if (nans_in_columns) {
            for (ssize_t row = 0; row != extra_data.rows(); ++row) {
              if (!extra_data.row (row).allFinite())
                mask[row] = false;
            }
          }
        }



        void TestVariableHomoscedastic::apply_mask (const BitSet& mask,
                                                    matrix_type::ConstColXpr data,
                                                    const matrix_type& shuffling_matrix,
                                                    const matrix_type& extra_column_data,
                                                    matrix_type& Mfull_masked,
                                                    matrix_type& shuffling_matrix_masked,
                                                    vector_type& data_masked) const
        {
          const size_t finite_count = mask.count();
          // Do we need to reduce the size of our matrices / vectors
          //   based on the presence of non-finite values?
          if (finite_count == num_inputs()) {

            Mfull_masked.resize (num_inputs(), num_factors());
            Mfull_masked.block (0, 0, num_inputs(), M.cols()) = M;
            Mfull_masked.block (0, M.cols(), num_inputs(), extra_column_data.cols()) = extra_column_data;
            shuffling_matrix_masked = shuffling_matrix;
            data_masked = data;

          } else {

            Mfull_masked.resize (finite_count, num_factors());
            data_masked.resize (finite_count);
            BitSet perm_matrix_mask (num_inputs(), true);
            size_t out_index = 0;
            for (size_t in_index = 0; in_index != num_inputs(); ++in_index) {
              if (mask[in_index]) {
                Mfull_masked.block (out_index, 0, 1, M.cols()) = M.row (in_index);
                Mfull_masked.block (out_index, M.cols(), 1, extra_column_data.cols()) = extra_column_data.row (in_index);
                data_masked[out_index++] = data[in_index];
              } else {
                // Any row in the permutation matrix that contains a non-zero entry
                //   in the column corresponding to in_row needs to be removed
                //   from the permutation matrix
                for (ssize_t perm_row = 0; perm_row != shuffling_matrix.rows(); ++perm_row) {
                  if (shuffling_matrix (perm_row, in_index))
                    perm_matrix_mask[perm_row] = false;
                }
              }
            }
            assert (out_index == finite_count);
            assert (perm_matrix_mask.count() == finite_count);
            assert (data_masked.allFinite());
            // Only after we've reduced the design matrix do we now reduce the shuffling matrix
            // Step 1: Remove rows that contain non-zero entries in columns to be removed
            matrix_type temp (finite_count, num_inputs());
            out_index = 0;
            for (size_t in_index = 0; in_index != num_inputs(); ++in_index) {
              if (perm_matrix_mask[in_index])
                temp.row (out_index++) = shuffling_matrix.row (in_index);
            }
            assert (out_index == finite_count);
            // Step 2: Remove columns
            shuffling_matrix_masked.resize (finite_count, finite_count);
            out_index = 0;
            for (size_t in_index = 0; in_index != num_inputs(); ++in_index) {
              if (mask[in_index])
                shuffling_matrix_masked.col (out_index++) = temp.col (in_index);
            }
            assert (out_index == finite_count);
          }
        }













        TestVariableHeteroscedastic::TestVariableHeteroscedastic (const vector<CohortDataImport>& importers,
                                                                  const matrix_type& measurements,
                                                                  const matrix_type& design,
                                                                  const vector<Hypothesis>& hypotheses,
                                                                  const index_array_type& variance_groups,
                                                                  const bool nans_in_data,
                                                                  const bool nans_in_columns) :
            TestVariableHomoscedastic (importers, measurements, design, hypotheses, nans_in_data, nans_in_columns),
            VG (variance_groups),
            num_vgs (VG.maxCoeff() + 1),
            gamma_weights (vector_type::Zero (num_hypotheses()))
        {
          for (size_t ih = 0; ih != c.size(); ++ih) {
            const size_t s = c[ih].rank();
            gamma_weights[ih] = 2.0*(s-1) / default_type(s*(s+2));
          }
        }




        void TestVariableHeteroscedastic::operator() (const matrix_type& shuffling_matrix, matrix_type& stats, matrix_type& zstats) const
        {
          stats.resize (num_elements(), num_hypotheses());
          zstats.resize (num_elements(), num_hypotheses());

          matrix_type extra_column_data (num_inputs(), importers.size());
          BitSet element_mask (num_inputs());
          matrix_type Mfull_masked, shuffling_matrix_masked, pinvMfull_masked, Rm;
          Eigen::Matrix<default_type, Eigen::Dynamic, 1> W;
          index_array_type VG_masked, VG_counts;
          vector_type y_masked, Sy, lambda, sq_residuals, sse, Rnn_sums, Wterms;

          for (ssize_t ie = 0; ie != y.cols(); ++ie) {
            // Common ground to the TestVariableHomoscedastic case
            for (ssize_t col = 0; col != ssize_t(importers.size()); ++col)
              extra_column_data.col (col) = importers[col] (ie);
            get_mask (ie, element_mask, extra_column_data);
            const size_t finite_count = element_mask.count();
            if (finite_count < std::min (num_inputs(), 2 * num_factors())) {
              stats.row (ie).setZero();
              zstats.row (ie).setZero();
            } else {
              apply_mask (element_mask,
                          y.col (ie),
                          shuffling_matrix,
                          extra_column_data,
                          Mfull_masked,
                          shuffling_matrix_masked,
                          y_masked);
              const default_type condition_number = Math::condition_number (Mfull_masked);
              if (!std::isfinite (condition_number) || condition_number > 1e5) {
                stats.row (ie).fill (0.0);
                zstats.row (ie).fill (0.0);
              } else {
                apply_mask_VG (element_mask, VG_masked, VG_counts);
                if (VG_counts.minCoeff() <= 1) {
                  stats.row (ie).fill (0.0);
                  zstats.row (ie).fill (0.0);
                } else {
                  pinvMfull_masked = Math::pinv (Mfull_masked);
                  Rm.noalias() = matrix_type::Identity (finite_count, finite_count) - (Mfull_masked*pinvMfull_masked);
                  for (size_t ih = 0; ih != c.size(); ++ih) {
                    const auto partition = c[ih].partition (Mfull_masked);

                    // At this point the implementation diverges from the TestVariableHomoscedastic case,
                    //   more closely mimicing the TestFixedHeteroscedastic case
                    Sy = shuffling_matrix_masked * partition.Rz * y_masked.matrix();
                    lambda = pinvMfull_masked * Sy.matrix();
                    sq_residuals = (Rm*Sy.matrix()).array().square();
                    sse = vector_type::Zero (num_variance_groups());
                    Rnn_sums = vector_type::Zero (num_variance_groups());
                    for (size_t input = 0; input != finite_count; ++input) {
                      sse[VG_masked[input]] += sq_residuals[input];
                      Rnn_sums[VG_masked[input]] += Rm.diagonal()[input];
                    }
                    Wterms = sse.inverse() * Rnn_sums;
                    for (size_t vg = 0; vg != num_vgs; ++vg) {
                      if (!std::isfinite (Wterms[vg]))
                        Wterms[vg] = 0.0;
                    }
                    default_type W_trace (0.0);
                    W.resize (finite_count);
                    for (size_t input = 0; input != finite_count; ++input) {
                      W[input] = Wterms[VG_masked[input]];
                      W_trace += W[input];
                    }

                    const default_type numerator = lambda.matrix().transpose() * c[ih].matrix().transpose() * (c[ih].matrix() * (Mfull_masked.transpose() * W.asDiagonal() * Mfull_masked).inverse() * c[ih].matrix().transpose()).inverse() * c[ih].matrix() * lambda.matrix();

                    default_type gamma (0.0);
                    for (size_t vg_index = 0; vg_index != num_vgs; ++vg_index)
                      gamma += Math::pow2 (1.0 - ((Wterms[vg_index] * VG_counts[vg_index]) / W_trace)) / Rnn_sums[vg_index];
                    gamma = 1.0 + (gamma_weights[ih] * gamma);

                    const default_type denominator = gamma * c[ih].rank();
                    const default_type G = numerator / denominator;

                    if (!std::isfinite (G)) {
                      stats  (ie, ih) = zstats (ie, ih) = value_type(0);
                    } else {
                      stats  (ie, ih) = c[ih].is_F() ?
                                        G :
                                        std::sqrt (G) * ((c[ih].matrix() * lambda.matrix()).sum() > 0.0 ? 1.0 : -1.0);
                      if (c[ih].is_F() && c[ih].rank() > 1) {
                        const default_type dof = 2.0 * default_type(c[ih].rank() - 1) / (3.0 * (gamma - 1.0));
                        zstats (ie, ih) = stat2z->F2z (G, c[ih].rank(), dof);
                      } else {
                        const default_type dof = Math::welch_satterthwaite (Wterms.inverse(), VG_counts);
                        zstats (ie, ih) = c[ih].is_F() ?
#ifdef MRTRIX_USE_ZSTATISTIC_LOOKUP
                                          stat2z->G2z (G, c[ih].rank(), dof) :
                                          stat2z->v2z (stats (ie, ih), dof);
#else
                                          Math::F2z (G, c[ih].rank(), dof) :
                                          Math::t2z (stats (ie, ih), dof);
#endif
                      } // End switching for F-test with rank > 1

                    } // End checking for G being finite

                  } // End looping over hypotheses for this element

                } // End check for preservation of at least two elements in each VG

              } // End checking for adequate condition number after NaN removal

            } // End checking for adequate number of remaining inputs after NaN removal

          } // End looping over elements
        }





        void TestVariableHeteroscedastic::apply_mask_VG (const BitSet& mask,
                                                         index_array_type& VG_masked,
                                                         index_array_type& VG_counts) const
        {
          VG_masked.resize (mask.count());
          VG_counts = index_array_type::Zero (num_vgs);
          size_t out_index = 0;
          for (size_t in_index = 0; in_index != mask.size(); ++in_index) {
            if (mask[in_index]) {
              VG_masked[out_index++] = VG[in_index];
              VG_counts[VG[in_index]]++;
            }
          }
          assert (out_index == size_t(VG_masked.size()));
        }




      }
    }
  }
}

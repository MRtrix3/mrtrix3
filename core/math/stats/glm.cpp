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

#include "math/stats/glm.h"

#include "debug.h"
#include "thread_queue.h"
#include "math/betainc.h"
#include "math/erfinv.h"
#include "misc/bitset.h"

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


        const char* const sqrt_f_description =
            "In MRtrix3 statistical inference commands, when F-tests are performed, "
            "it is the square root of the F-statistic that is internally calculated and "
            "tracked. This is to ensure that statistical enhancement algorithms operate "
            "comparably for both t-test and F-test hypotheses. Any export of F-statistics "
            "to file will take the square of this internal value such that it is the actual "
            "F-statistic that is written to file. This approach may however have consequences "
            "in the control of statistical enhancement algorithms; for instance, if manually "
            "setting a cluster-forming threshold, this should be determined based on the "
            "value of sqrt(F).";


        App::OptionGroup glm_options (const std::string& element_name)
        {
          using namespace App;
          OptionGroup result = OptionGroup ("Options related to the General Linear Model (GLM)")

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



        vector_type stdev (const matrix_type& measurements, const matrix_type& design)
        {
          const vector_type sse = (measurements - design * solve_betas (measurements, design)).colwise().squaredNorm();
          return (sse / value_type(design.rows()-Math::rank (design))).sqrt();
        }



        vector_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const Hypothesis& hypothesis)
        {
          if (hypothesis.is_F())
            return vector_type::Constant (measurements.cols(), NaN);
          else
            return abs_effect_size (measurements, design, hypothesis).array() / stdev (measurements, design);
        }

        matrix_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses)
        {
          const auto stdev_reciprocal = vector_type::Ones (measurements.cols()) / stdev (measurements, design);
          matrix_type result (measurements.cols(), hypotheses.size());
          for (size_t ic = 0; ic != hypotheses.size(); ++ic)
            result.col (ic) = abs_effect_size (measurements, design, hypotheses[ic]) * stdev_reciprocal;
          return result;
        }



//#define GLM_ALL_STATS_DEBUG

        void all_stats (const matrix_type& measurements,
                        const matrix_type& design,
                        const vector<Hypothesis>& hypotheses,
                        matrix_type& betas,
                        matrix_type& abs_effect_size,
                        matrix_type& std_effect_size,
                        vector_type& stdev)
        {
#ifndef GLM_ALL_STATS_DEBUG
          // If this function is being invoked from the other version of all_stats(),
          //   on an element-by-element basis, don't interfere with the progress bar
          //   that's being displayed by that outer looping function
          std::unique_ptr<ProgressBar> progress;
          if (measurements.cols() > 1)
            progress.reset (new ProgressBar ("Calculating basic properties of default permutation", 6));
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
          vector_type sse (residuals.cols());
          sse = residuals.colwise().squaredNorm();
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "sse: " << sse.size() << ", max " << sse.maxCoeff() << "\n";
#else
          if (progress)
            ++*progress;
#endif
          const ssize_t dof = design.rows()-Math::rank (design);
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "Degrees of freedom: " << design.rows() << " - " << Math:rank (design) << " = " << dof << "\n";
#endif
          stdev = (sse / value_type(dof)).sqrt();
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "stdev: " << stdev.size() << ", max " << stdev.maxCoeff() << "\n";
#else
          if (progress)
            ++*progress;
#endif
          std_effect_size = abs_effect_size.array().colwise() / stdev;
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "std_effect_size: " << std_effect_size.rows() << " x " << std_effect_size.cols() << ", max " << std_effect_size.array().maxCoeff() << "\n";
#endif
        }



        void all_stats (const matrix_type& measurements,
                        const matrix_type& fixed_design,
                        const vector<CohortDataImport>& extra_columns,
                        const vector<Hypothesis>& hypotheses,
                        vector_type& cond,
                        matrix_type& betas,
                        matrix_type& abs_effect_size,
                        matrix_type& std_effect_size,
                        vector_type& stdev)
        {
          if (extra_columns.empty() && measurements.allFinite()) {
            all_stats (measurements, fixed_design, hypotheses, betas, abs_effect_size, std_effect_size, stdev);
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
              Functor (const matrix_type& data, const matrix_type& design_fixed, const vector<CohortDataImport>& extra_columns, const vector<Hypothesis>& hypotheses,
                       vector_type& cond, matrix_type& betas, matrix_type& abs_effect_size, matrix_type& std_effect_size, vector_type& stdev) :
                  data (data),
                  design_fixed (design_fixed),
                  extra_columns (extra_columns),
                  hypotheses (hypotheses),
                  global_cond (cond),
                  global_betas (betas),
                  global_abs_effect_size (abs_effect_size),
                  global_std_effect_size (std_effect_size),
                  global_stdev (stdev)
              {
                assert (size_t(design_fixed.cols()) + extra_columns.size() == size_t(hypotheses[0].cols()));
              }
              bool operator() (const size_t& element_index)
              {
                const matrix_type element_data = data.col (element_index);
                matrix_type element_design (design_fixed.rows(), design_fixed.cols() + extra_columns.size());
                element_design.leftCols (design_fixed.cols()) = design_fixed;
                // For each element-wise design matrix column,
                //   acquire the data for this particular element, without permutation
                for (size_t col = 0; col != extra_columns.size(); ++col)
                  element_design.col (design_fixed.cols() + col) = (extra_columns[col]) (element_index);
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
                    Math::Stats::GLM::all_stats (element_data, element_design, hypotheses,
                                                 local_betas, local_abs_effect_size, local_std_effect_size, local_stdev);
                  }
                } else if (valid_rows >= element_design.cols()) {
                  // Need to reduce the data and design matrices to contain only finite data
                  matrix_type element_data_finite (valid_rows, 1);
                  matrix_type element_design_finite (valid_rows, element_design.cols());
                  ssize_t output_row = 0;
                  for (ssize_t row = 0; row != data.rows(); ++row) {
                    if (std::isfinite (element_data(row)) && element_design.row (row).allFinite()) {
                      element_data_finite(output_row, 0) = element_data(row);
                      element_design_finite.row (output_row) = element_design.row (row);
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
                    Math::Stats::GLM::all_stats (element_data_finite, element_design_finite, hypotheses,
                                                 local_betas, local_abs_effect_size, local_std_effect_size, local_stdev);
                  }
                } else { // Insufficient data to fit model at all
                  zero();
                }
                global_cond[element_index] = condition_number;
                global_betas.col (element_index) = local_betas;
                global_abs_effect_size.row (element_index) = local_abs_effect_size.row (0);
                global_std_effect_size.row (element_index) = local_std_effect_size.row (0);
                global_stdev[element_index] = local_stdev[0];
                return true;
              }
            private:
              const matrix_type& data;
              const matrix_type& design_fixed;
              const vector<CohortDataImport>& extra_columns;
              const vector<Hypothesis>& hypotheses;
              vector_type& global_cond;
              matrix_type& global_betas;
              matrix_type& global_abs_effect_size;
              matrix_type& global_std_effect_size;
              vector_type& global_stdev;
              matrix_type local_betas, local_abs_effect_size, local_std_effect_size;
              vector_type local_stdev;

              void zero () {
                local_betas = matrix_type::Zero (global_betas.rows(), 1);
                local_abs_effect_size = matrix_type::Zero (1, hypotheses.size());
                local_std_effect_size = matrix_type::Zero (1, hypotheses.size());
                local_stdev = vector_type::Zero (1);
              }
          };

          Source source (measurements.cols());
          Functor functor (measurements, fixed_design, extra_columns, hypotheses,
                           cond, betas, abs_effect_size, std_effect_size, stdev);
          Thread::run_queue (source, Thread::batch (size_t()), Thread::multi (functor));
        }










        // Same model partitioning as is used in FSL randomise
        Hypothesis::Partition Hypothesis::partition (const matrix_type& design) const
        {
          // eval() calls necessary for older versions of Eigen / compiler to work:
          //   can't seem to map Eigen template result to const matrix_type& as the Math::pinv() input
          // TODO See if some better template trickery can be done
          const matrix_type D = Math::pinv ((design.transpose() * design).eval());
          // Note: Cu is transposed with respect to how contrast matrices are stored elsewhere
          const matrix_type Cu = Eigen::FullPivLU<matrix_type> (c).kernel();
          const matrix_type inv_cDc = Math::pinv ((c * D * c.transpose()).eval());
          // Note: Cv is transposed with respect to convention just as Cu is
          const matrix_type Cv = Cu - c.transpose() * inv_cDc * c * D * Cu;
          const matrix_type X = design * D * c.transpose() * inv_cDc;
          const matrix_type Z = design * D * Cv * Math::pinv ((Cv.transpose() * D * Cv).eval());
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



        TestFixed::TestFixed (const matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses) :
            TestBase (measurements, design, hypotheses),
            pinvM (Math::pinv (M)),
            Rm (matrix_type::Identity (num_subjects(), num_subjects()) - (M*pinvM))
        {
          assert (hypotheses[0].cols() == design.cols());
          // When the design matrix is fixed, we can pre-calculate the model partitioning for each hypothesis
          for (const auto& h : hypotheses)
            partitions.emplace_back (h.partition (design));
        }



        void TestFixed::operator() (const matrix_type& shuffling_matrix,
                                    matrix_type& stats,
                                    matrix_type& zstats) const
        {
          assert (size_t(shuffling_matrix.rows()) == num_subjects());
          stats .resize (num_elements(), num_outputs());
          zstats.resize (num_elements(), num_outputs());

          matrix_type Sy, lambdas, XtX, beta;
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
#endif
            XtX.noalias() = partitions[ih].X.transpose()*partitions[ih].X;
#ifdef GLM_TEST_DEBUG
            VAR (XtX.rows());
            VAR (XtX.cols());
#endif
            const size_t dof = num_subjects() - partitions[ih].rank_x - partitions[ih].rank_z;
            const default_type one_over_dof = 1.0 / default_type(dof);
            sse = (Rm*Sy).colwise().squaredNorm();
#ifdef GLM_TEST_DEBUG
            VAR (dof);
            VAR (one_over_dof);
            VAR (sse.size());
#endif
            for (size_t ie = 0; ie != num_elements(); ++ie) {
              beta.noalias() = c[ih].matrix() * lambdas.col (ie);
#ifdef GLM_TEST_DEBUG
              VAR (beta.rows());
              VAR (beta.cols());
#endif
              const value_type F = ((beta.transpose() * XtX * beta) (0,0) / c[ih].rank()) /
                                   (one_over_dof * sse[ie]);
              if (!std::isfinite (F)) {
                stats  (ie, ih) = zstats (ie, ih) = value_type(0);
              } else if (c[ih].is_F()) {
                stats  (ie, ih) = F;
                zstats (ie, ih) = stat2z->F2z (F, c[ih].rank(), dof);
              } else {
                assert (beta.rows() == 1);
                stats  (ie, ih) = std::sqrt (F) * (beta.sum() > 0.0 ? 1.0 : -1.0);
                zstats (ie, ih) = stat2z->t2z (stats (ie, ih), dof);
              }
            }

          }
        }











        TestVariable::TestVariable (const vector<CohortDataImport>& importers,
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




        void TestVariable::operator() (const matrix_type& shuffling_matrix,
                                       matrix_type& stat,
                                       matrix_type& zstat) const
        {
          stat .resize (num_elements(), num_outputs());
          zstat.resize (num_elements(), num_outputs());

          matrix_type dof (num_elements(), num_outputs());

          matrix_type extra_data (num_subjects(), importers.size());
          BitSet element_mask (num_subjects()), perm_matrix_mask (num_subjects());
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
              extra_data.col (col) = importers[col] (ie);

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
            element_mask.clear (true);
            if (nans_in_data) {
              for (ssize_t row = 0; row != y.rows(); ++row) {
                if (!std::isfinite (y (row, ie)))
                  element_mask[row] = false;
              }
            }
            if (nans_in_columns) {
              for (ssize_t row = 0; row != extra_data.rows(); ++row) {
                if (!extra_data.row (row).allFinite())
                  element_mask[row] = false;
              }
            }
            const size_t finite_count = element_mask.count();
            // Additional rejection here:
            // If the number of finite elemets is _not_ equal to the number of subjects
            //   (i.e. at least one subject has been removed), there needs to be a
            //   more stringent criterion met in order to proceed with the test.
            //   Let's do: DoF must be at least equal to the number of factors.
            if (finite_count < num_subjects() && finite_count < 2 * num_factors()) {
              stat.row (ie).setZero();
              zstat.row (ie).setZero();
              dof.row (ie).fill (NaN);
            } else {

              // Do we need to reduce the size of our matrices / vectors
              //   based on the presence of non-finite values?
              if (finite_count == num_subjects()) {

                Mfull_masked.resize (num_subjects(), num_factors());
                Mfull_masked.block (0, 0, num_subjects(), M.cols()) = M;
                Mfull_masked.block (0, M.cols(), num_subjects(), extra_data.cols()) = extra_data;
                shuffling_matrix_masked = shuffling_matrix;
                y_masked = y.col (ie);

              } else {

                Mfull_masked.resize (finite_count, num_factors());
                y_masked.resize (finite_count);
                perm_matrix_mask.clear (true);
                size_t out_index = 0;
                for (size_t in_index = 0; in_index != num_subjects(); ++in_index) {
                  if (element_mask[in_index]) {
                    Mfull_masked.block (out_index, 0, 1, M.cols()) = M.row (in_index);
                    Mfull_masked.block (out_index, M.cols(), 1, extra_data.cols()) = extra_data.row (in_index);
                    y_masked[out_index++] = y (in_index, ie);
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
                assert (y_masked.allFinite());
                // Only after we've reduced the design matrix do we now reduce the shuffling matrix
                // Step 1: Remove rows that contain non-zero entries in columns to be removed
                matrix_type temp (finite_count, num_subjects());
                out_index = 0;
                for (size_t in_index = 0; in_index != num_subjects(); ++in_index) {
                  if (perm_matrix_mask[in_index])
                    temp.row (out_index++) = shuffling_matrix.row (in_index);
                }
                assert (out_index == finite_count);
                // Step 2: Remove columns
                shuffling_matrix_masked.resize (finite_count, finite_count);
                out_index = 0;
                for (size_t in_index = 0; in_index != num_subjects(); ++in_index) {
                  if (element_mask[in_index])
                    shuffling_matrix_masked.col (out_index++) = temp.col (in_index);
                }
                assert (out_index == finite_count);
              }
              assert (Mfull_masked.allFinite());

              // Test condition number of NaN-masked & data-filled design matrix;
              //   need to skip statistical testing if it is too poor
              // TODO Condition number testing may be quite slow;
              //   would a rank calculation with tolerance be faster?
              const default_type condition_number = Math::condition_number (Mfull_masked);
              if (!std::isfinite (condition_number) || condition_number > 1e5) {
                stat.row (ie).fill (0.0);
                zstat.row (ie).fill (0.0);
                dof.row (ie).fill (NaN);
              } else {

                pinvMfull_masked = Math::pinv (Mfull_masked);

                Rm.noalias() = matrix_type::Identity (finite_count, finite_count) - (Mfull_masked*pinvMfull_masked);

                // We now have our permutation (shuffling) matrix and design matrix prepared,
                //   and can commence regressing the partitioned model of each hypothesis
                for (ssize_t ih = 0; ih != c.size(); ++ih) {

                  const auto partition = c[ih].partition (Mfull_masked);
                  dof (ie, ih) = finite_count - partition.rank_x - partition.rank_z;
                  if (dof (ie, ih) < 1) {
                    stat (ie, ih) = zstat (ie, ih) = dof (ie, ih) = value_type(0);
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
                      stat  (ie, ih) = zstat (ie, ih) = value_type(0);
                    } else if (c[ih].is_F()) {
                      stat  (ie, ih) = F;
                      zstat (ie, ih) = stat2z->F2z (F, c[ih].rank(), dof (ie, ih));
                    } else {
                      assert (beta.rows() == 1);
                      stat  (ie, ih) = std::sqrt (F) * (beta.sum() > 0 ? 1.0 : -1.0);
                      zstat (ie, ih) = stat2z->t2z (stat (ie, ih), dof (ie, ih));
                    }

                  } // End checking for sufficient degrees of freedom

                } // End looping over hypotheses

              } // End checking for adequate condition number after NaN removal

            } // End checking for adequate number of remaining subjects after NaN removal

          } // End looping over elements

        } // End functor




      }
    }
  }
}


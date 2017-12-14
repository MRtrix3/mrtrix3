/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include "math/stats/glm.h"

#include "debug.h"
#include "misc/bitset.h"
#include "thread_queue.h"

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




        vector<Contrast> load_contrasts (const std::string& file_path)
        {
          vector<Contrast> contrasts;
          const matrix_type contrast_matrix = load_matrix (file_path);
          for (ssize_t row = 0; row != contrast_matrix.rows(); ++row)
            contrasts.emplace_back (Contrast (contrast_matrix.row (row), row));
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
              contrasts.emplace_back (Contrast (this_f_matrix, ftest_index));
            }
            if (App::get_options ("fonly").size()) {
              vector<Contrast> new_contrasts;
              for (size_t index = contrast_matrix.rows(); index != contrasts.size(); ++index)
                new_contrasts.push_back (std::move (contrasts[index]));
              std::swap (contrasts, new_contrasts);
            }
          } else if (App::get_options ("fonly").size()) {
            throw Exception ("Cannot perform F-tests exclusively (-fonly option): No F-test matrix was provided (-ftests option)");
          }
          return contrasts;
        }






        matrix_type solve_betas (const matrix_type& measurements, const matrix_type& design)
        {
          return design.jacobiSvd (Eigen::ComputeThinU | Eigen::ComputeThinV).solve (measurements);
        }



        vector_type abs_effect_size (const matrix_type& measurements, const matrix_type& design, const Contrast& contrast)
        {
          if (contrast.is_F())
            return vector_type::Constant (measurements.rows(), NaN);
          else
            return contrast.matrix() * solve_betas (measurements, design);
        }

        matrix_type abs_effect_size (const matrix_type& measurements, const matrix_type& design, const vector<Contrast>& contrasts)
        {
          matrix_type result (measurements.cols(), contrasts.size());
          for (size_t ic = 0; ic != contrasts.size(); ++ic)
            result.col (ic) = abs_effect_size (measurements, design, contrasts[ic]);
          return result;
        }



        vector_type stdev (const matrix_type& measurements, const matrix_type& design)
        {
          const vector_type sse = (measurements - design * solve_betas (measurements, design)).colwise().squaredNorm();
          return (sse / value_type(design.rows()-Math::rank (design))).sqrt();
        }



        vector_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const Contrast& contrast)
        {
          if (contrast.is_F())
            return vector_type::Constant (measurements.cols(), NaN);
          else
            return abs_effect_size (measurements, design, contrast).array() / stdev (measurements, design);
        }

        matrix_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const vector<Contrast>& contrasts)
        {
          const auto stdev_reciprocal = vector_type::Ones (measurements.cols()) / stdev (measurements, design);
          matrix_type result (measurements.cols(), contrasts.size());
          for (size_t ic = 0; ic != contrasts.size(); ++ic)
            result.col (ic) = abs_effect_size (measurements, design, contrasts[ic]) * stdev_reciprocal;
          return result;
        }



//#define GLM_ALL_STATS_DEBUG

        void all_stats (const matrix_type& measurements,
                        const matrix_type& design,
                        const vector<Contrast>& contrasts,
                        matrix_type& betas,
                        matrix_type& abs_effect_size,
                        matrix_type& std_effect_size,
                        vector_type& stdev)
        {
#ifndef GLM_ALL_STATS_DEBUG
          ProgressBar progress ("Calculating basic properties of default permutation");
#endif
          betas = solve_betas (measurements, design);
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "Betas: " << betas.rows() << " x " << betas.cols() << ", max " << betas.array().maxCoeff() << "\n";
#else
          ++progress;
#endif
          abs_effect_size.resize (measurements.cols(), contrasts.size());
          for (size_t ic = 0; ic != contrasts.size(); ++ic) {
            if (contrasts[ic].is_F()) {
              abs_effect_size.col (ic).fill (NaN);
            } else {
              abs_effect_size.col (ic) = (contrasts[ic].matrix() * betas).row (0);
            }
          }
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "abs_effect_size: " << abs_effect_size.rows() << " x " << abs_effect_size.cols() << ", max " << abs_effect_size.array().maxCoeff() << "\n";
#else
          ++progress;
#endif
          // Explicit calculation of residuals before SSE, rather than in a single
          //   step, appears to be necessary for compatibility with Eigen 3.2.0
          const matrix_type residuals = (measurements - design * betas);
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "Residuals: " << residuals.rows() << " x " << residuals.cols() << ", max " << residuals.array().maxCoeff() << "\n";
#else
          ++progress;
#endif
          vector_type sse (residuals.cols());
          sse = residuals.colwise().squaredNorm();
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "sse: " << sse.size() << ", max " << sse.maxCoeff() << "\n";
#else
          ++progress;
#endif
          stdev = (sse / value_type(design.rows()-Math::rank (design))).sqrt();
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "stdev: " << stdev.size() << ", max " << stdev.maxCoeff() << "\n";
#else
          ++progress;
#endif
          std_effect_size = abs_effect_size.array().colwise() / stdev;
#ifdef GLM_ALL_STATS_DEBUG
          std::cerr << "std_effect_size: " << std_effect_size.rows() << " x " << std_effect_size.cols() << ", max " << std_effect_size.array().maxCoeff() << "\n";
#endif
        }



        void all_stats (const matrix_type& measurements,
                        const matrix_type& fixed_design,
                        const vector<CohortDataImport>& extra_columns,
                        const vector<Contrast>& contrasts,
                        matrix_type& betas,
                        matrix_type& abs_effect_size,
                        matrix_type& std_effect_size,
                        vector_type& stdev)
        {
          if (extra_columns.empty()) {
            all_stats (measurements, fixed_design, contrasts, betas, abs_effect_size, std_effect_size, stdev);
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
              Functor (const matrix_type& data, const matrix_type& design_fixed, const vector<CohortDataImport>& extra_columns, const vector<Contrast>& contrasts,
                       matrix_type& betas, matrix_type& abs_effect_size, matrix_type& std_effect_size, vector_type& stdev) :
                  data (data),
                  design_fixed (design_fixed),
                  extra_columns (extra_columns),
                  contrasts (contrasts),
                  global_betas (betas),
                  global_abs_effect_size (abs_effect_size),
                  global_std_effect_size (std_effect_size),
                  global_stdev (stdev)
              {
                assert (size_t(design_fixed.cols()) + extra_columns.size() == size_t(contrasts[0].cols()));
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
                Math::Stats::GLM::all_stats (element_data, element_design, contrasts,
                                             local_betas, local_abs_effect_size, local_std_effect_size, local_stdev);
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
              const vector<Contrast>& contrasts;
              matrix_type& global_betas;
              matrix_type& global_abs_effect_size;
              matrix_type& global_std_effect_size;
              vector_type& global_stdev;
              matrix_type local_betas, local_abs_effect_size, local_std_effect_size;
              vector_type local_stdev;
          };

          Source source (measurements.cols());
          Functor functor (measurements, fixed_design, extra_columns, contrasts,
                           betas, abs_effect_size, std_effect_size, stdev);
          Thread::run_queue (source, Thread::batch (size_t()), Thread::multi (functor));
        }










        // Same model partitioning as is used in FSL randomise
        Contrast::Partition Contrast::partition (const matrix_type& design) const
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



        matrix_type Contrast::check_rank (const matrix_type& in, const size_t index) const
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












        TestFixed::TestFixed (const matrix_type& measurements, const matrix_type& design, const vector<Contrast>& contrasts) :
            TestBase (measurements, design, contrasts),
            pinvM (Math::pinv (M)),
            Rm (matrix_type::Identity (num_subjects(), num_subjects()) - (M*pinvM))
        {
          assert (contrasts[0].cols() == design.cols());
          // When the design matrix is fixed, we can pre-calculate the model partitioning for each contrast
          for (const auto c : contrasts)
            partitions.emplace_back (c.partition (design));
        }



        void TestFixed::operator() (const matrix_type& shuffling_matrix, matrix_type& output) const
        {
          assert (size_t(shuffling_matrix.rows()) == num_subjects());
          if (!(size_t(output.rows()) == num_elements() && size_t(output.cols()) == num_outputs()))
            output.resize (num_elements(), num_outputs());

          matrix_type Sy, lambdas, XtX, beta;
          vector_type sse;

          // Freedman-Lane for fixed design matrix case
          // Each contrast needs to be handled explicitly on its own
          for (size_t ic = 0; ic != c.size(); ++ic) {

            // First, we perform permutation of the input data
            // In Freedman-Lane, the initial 'effective' regression against the nuisance
            //   variables, and permutation of the data, are done in a single step
            //VAR (shuffling_matrix.rows());
            //VAR (shuffling_matrix.cols());
            //VAR (partitions[ic].Rz.rows());
            //VAR (partitions[ic].Rz.cols());
            //VAR (y.rows());
            //VAR (y.cols());
            Sy.noalias() = shuffling_matrix * partitions[ic].Rz * y;
            //VAR (Sy.rows());
            //VAR (Sy.cols());
            // Now, we regress this shuffled data against the full model
            //VAR (pinvM.rows());
            //VAR (pinvM.cols());
            lambdas.noalias() = pinvM * Sy;
            //VAR (lambda.rows());
            //VAR (lambda.cols());
            //VAR (matrix_type(c[ic]).rows());
            //VAR (matrix_type(c[ic]).cols());
            //VAR (Rm.rows());
            //VAR (Rm.cols());
            XtX.noalias() = partitions[ic].X.transpose()*partitions[ic].X;
            //VAR (XtX.rows());
            //VAR (XtX.cols());
            const default_type one_over_dof = 1.0 / (num_subjects() - partitions[ic].rank_x - partitions[ic].rank_z);
            sse = (Rm*Sy).colwise().squaredNorm();
            //VAR (sse.size());
            for (size_t ie = 0; ie != num_elements(); ++ie) {
              beta.noalias() = c[ic].matrix() * lambdas.col (ie);
              //VAR (beta.rows());
              //VAR (beta.cols());
              const value_type F = ((beta.transpose() * XtX * beta) (0,0) / c[ic].rank()) /
                                   (one_over_dof * sse[ie]);
              if (!std::isfinite (F)) {
                output (ie, ic) = value_type(0);
              } else if (c[ic].is_F()) {
                output (ie, ic) = F;
              } else {
                assert (beta.rows() == 1);
                output (ie, ic) = std::sqrt (F) * (beta.sum() > 0.0 ? 1.0 : -1.0);
              }
            }

          }
        }











        TestVariable::TestVariable (const vector<CohortDataImport>& importers,
                                    const matrix_type& measurements,
                                    const matrix_type& design,
                                    const vector<Contrast>& contrasts,
                                    const bool nans_in_data,
                                    const bool nans_in_columns) :
            TestBase (measurements, design, contrasts),
            importers (importers),
            nans_in_data (nans_in_data),
            nans_in_columns (nans_in_columns)
        {
          // Make sure that the specified contrasts reflect the full design matrix (with additional
          //   data loaded)
          assert (contrasts[0].cols() == M.cols() + ssize_t(importers.size()));
        }



        void TestVariable::operator() (const matrix_type& shuffling_matrix, matrix_type& output) const
        {
          if (!(size_t(output.rows()) == num_elements() && size_t(output.cols()) == num_outputs()))
            output.resize (num_elements(), num_outputs());

          matrix_type extra_data (num_subjects(), importers.size());
          BitSet element_mask (num_subjects()), perm_matrix_mask (num_subjects());
          matrix_type perm_matrix_masked, Mfull_masked, pinvMfull_masked, Rm;
          vector_type y_masked, Sy, lambda;
          matrix_type XtX, beta;

          // Let's loop over elements first, then contrasts in the inner loop
          for (ssize_t ie = 0; ie != y.cols(); ++ie) {

            // For each element (row in y), need to load the additional data for that element
            //   for all subjects in order to construct the design matrix
            // Would it be preferable to pre-calculate and store these per-element design matrices,
            //   rather than re-generating them each time? (More RAM, less CPU)
            // No, most of the time that subject data will be memory-mapped, so pre-loading (in
            //   addition to the duplication of the fixed design matrix contents) would hurt bad
            for (ssize_t col = 0; col != ssize_t(importers.size()); ++col)
              extra_data.col (col) = importers[col] (ie);

            // What can we do here that's common across all contrasts?
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

            // Do we need to reduce the size of our matrices / vectors
            //   based on the presence of non-finite values?
            if (finite_count == num_subjects()) {

              Mfull_masked.resize (num_subjects(), num_factors());
              Mfull_masked.block (0, 0, num_subjects(), M.cols()) = M;
              Mfull_masked.block (0, M.cols(), num_subjects(), extra_data.cols()) = extra_data;
              perm_matrix_masked = shuffling_matrix;
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
              // Only after we've reduced the design matrix do we now reduce the permutation matrix
              perm_matrix_masked.resize (finite_count, num_subjects());
              out_index = 0;
              for (size_t in_index = 0; in_index != num_subjects(); ++in_index) {
                if (perm_matrix_mask[in_index])
                  perm_matrix_masked.row (out_index++) = shuffling_matrix.row (in_index);
              }
              assert (out_index == finite_count);

            }
            assert (Mfull_masked.allFinite());

            pinvMfull_masked = Math::pinv (Mfull_masked);

            Rm.noalias() = matrix_type::Identity (finite_count, finite_count) - (Mfull_masked*pinvMfull_masked);

            // We now have our permutation (shuffling) matrix and design matrix prepared,
            //   and can commence regressing the partitioned model of each contrast
            for (size_t ic = 0; ic != c.size(); ++ic) {

              const auto partition = c[ic].partition (Mfull_masked);
              XtX.noalias() = partition.X.transpose()*partition.X;

              // Now that we have the individual contrast model partition for these data,
              //   the rest of this function should proceed similarly to the fixed
              //   design matrix case
              Sy = perm_matrix_masked * partition.Rz * y_masked.matrix();
              lambda = pinvMfull_masked * Sy.matrix();
              beta.noalias() = c[ic].matrix() * lambda.matrix();
              const default_type sse = (Rm*Sy.matrix()).squaredNorm();

              const default_type F = ((beta.transpose() * XtX * beta) (0, 0) / c[ic].rank()) /
                                     (sse / value_type (finite_count - partition.rank_x - partition.rank_z));

              if (!std::isfinite (F)) {
                output (ie, ic) = value_type(0);
              } else if (c[ic].is_F()) {
                output (ie, ic) = F;
              } else {
                assert (beta.rows() == 1);
                output (ie, ic) = std::sqrt (F) * (beta.sum() > 0 ? 1.0 : -1.0);
              }

            } // End looping over contrasts

          } // End looping over elements
        }



      }
    }
  }
}


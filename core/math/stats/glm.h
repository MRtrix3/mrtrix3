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

#ifndef __math_stats_glm_h__
#define __math_stats_glm_h__

#include "app.h"
#include "types.h"

#include "math/condition_number.h"
#include "math/least_squares.h"
#include "math/zstatistic.h"
#include "math/stats/import.h"
#include "math/stats/typedefs.h"

#include "misc/bitset.h"

namespace MR
{
  namespace Math
  {
    namespace Stats
    {
      namespace GLM
      {



        extern const char* const column_ones_description;

        App::OptionGroup glm_options (const std::string& element_name);



        // Define a base class to contain information regarding an individual hypothesis, and
        //   pre-compute as much as possible with regards to Freedman-Lane
        // Note: This can be constructed for both t-tests and F-tests
        //   (This is why the constructor is a template: Could be created either from a row()
        //   call on the contrast matrix, or from a matrix explicitly constructed from a set of
        //   rows from the contrast matrix, which is how an F-test is constructed.
        // In the case of a single-row F-test, still need to be able to differentiate between
        //   a t-test and an F-test for the sake of signedness (and taking the square root);
        //   this is managed by having two separate constructor templates
        class Hypothesis
        { MEMALIGN(Hypothesis)
          public:

            class Partition
            { MEMALIGN (Partition)
              public:
                Partition (const matrix_type& x, const matrix_type& z) :
                    X (x),
                    Z (z),
                    Hz (Z.cols() ?
                       (Z * Math::pinv (Z)) :
                       matrix_type (matrix_type::Zero (X.rows(), X.rows()))),
                    Rz (matrix_type::Identity (X.rows(), X.rows()) - Hz),
                    rank_x (Math::rank (X)),
                    rank_z (Z.cols() ? Math::rank (Z) : 0) { }
                // X = Component of design matrix related to effect of interest
                // Z = Component of design matrix related to nuisance regressors
                const matrix_type X, Z;
                // Hz: Projection matrix of nuisance regressors only
                // Rz: Residual-forming matrix due to nuisance regressors only
                const matrix_type Hz, Rz;
                // rank_x: Rank of X
                // rank_z: Rank of Z
                const size_t rank_x, rank_z;
            };

            Hypothesis (matrix_type::ConstRowXpr& in, const size_t index) :
                c (in),
                r (Math::rank (c)),
                F (false),
                i (index) { check_nonzero(); }

            Hypothesis (const matrix_type& in, const size_t index) :
                c (check_rank (in, index)),
                r (Math::rank (c)),
                F (true),
                i (index) { check_nonzero(); }

            Partition partition (const matrix_type&) const;

            const matrix_type& matrix() const { return c; }
            ssize_t cols() const { return c.cols(); }
            size_t rank() const { return r; }
            bool is_F() const { return F; }
            std::string name() const { return std::string(F ? "F" : "t") + str(i+1); }

          private:
            const matrix_type c;
            const size_t r;
            const bool F;
            const size_t i;

            void check_nonzero() const;
            matrix_type check_rank (const matrix_type&, const size_t) const;
        };



        void check_design (const matrix_type&, const bool);

        index_array_type load_variance_groups (const size_t num_inputs);

        vector<Hypothesis> load_hypotheses (const std::string& file_path);



        /** \addtogroup Statistics
          @{ */
        /*! Compute a matrix of the beta coefficients
           * @param measurements a matrix storing the measured data across subjects in each column
           * @param design the design matrix
           * @return the matrix containing the output GLM betas (one column of factor betas per element)
           */
        matrix_type solve_betas (const matrix_type& measurements, const matrix_type& design);



        /*! Compute the effect of interest
         * @param measurements a matrix storing the measured data across subjects in each column
         * @param design the design matrix
         * @param hypothesis a Hypothesis class instance defining the effect of interest
         * @return the matrix containing the output absolute effect sizes (one column of element effect sizes per contrast)
         */
        vector_type abs_effect_size (const matrix_type& measurements, const matrix_type& design, const Hypothesis& hypothesis);
        matrix_type abs_effect_size (const matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses);



        /*! Compute the pooled standard deviation
         * @param measurements a matrix storing the measured data across subjects in each column
         * @param design the design matrix
         * @return the vector containing the output standard deviation for each element
         */
        matrix_type stdev (const matrix_type& measurements, const matrix_type& design);



        /*! Compute the standard deviation of each variance group
         * @param measurements a matrix storing the measured data across subjects in each column
         * @param design the design matrix
         * @return the vector containing the output standard deviation for each element
         */
        matrix_type stdev (const matrix_type& measurements, const matrix_type& design, const index_array_type& variance_groups);



        /*! Compute cohen's d, the standardised effect size between two means
         * @param measurements a matrix storing the measured data across subjects in each column
         * @param design the design matrix
         * @param hypothesis a Hypothesis class instance defining the effect of interest
         * @return the matrix containing the output standardised effect sizes (one column of element effect sizes per contrast)
         */
        vector_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const Hypothesis& hypothesis);
        matrix_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses);



        /*! Compute all GLM-related statistics
         * This function can be used when the design matrix remains fixed for all
         * elements to be tested.
         * @param measurements a matrix storing the measured data for each subject in a column
         * @param design the design matrix
         * @param hypotheses a vector of Hypothesis class instances defining the effects of interest
         * @param betas the matrix containing the output GLM betas
         * @param abs_effect_size the matrix containing the output effect
         * @param std_effect_size the matrix containing the output standardised effect size
         * @param stdev the matrix containing the output standard deviation
         */
        void all_stats (const matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses, const index_array_type& variance_groups,
                        matrix_type& betas, matrix_type& abs_effect_size, matrix_type& std_effect_size, matrix_type& stdev);

        /*! Compute all GLM-related statistics
         * This function can be used when the design matrix varies between elements,
         * due to importing external data for each element from external files
         * @param measurements a matrix storing the measured data for each subject in a column
         * @param design the fixed portion of the design matrix
         * @param extra_columns the variable columns of the design matrix
         * @param hypotheses a vector of Hypothesis class instances defining the effects of interest
         * @param betas the matrix containing the output GLM betas
         * @param abs_effect_size the matrix containing the output effect
         * @param std_effect_size the matrix containing the output standardised effect size
         * @param stdev the matrix containing the output standard deviation
         */
        void all_stats (const matrix_type& measurements, const matrix_type& design, const vector<CohortDataImport>& extra_columns, const vector<Hypothesis>& hypotheses, const index_array_type& variance_groups,
                        vector_type& cond, matrix_type& betas, matrix_type& abs_effect_size, matrix_type& std_effect_size, matrix_type& stdev);

        //! @}






        // Define a base class for GLM tests
        class TestBase
        { MEMALIGN(TestBase)
          public:
            TestBase (const matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses) :
                y (measurements),
                M (design),
                c (hypotheses),
                stat2z (new Math::Zstatistic())
            {
              assert (y.rows() == M.rows());
              // Can no longer apply this assertion here; GLMTTestVariable later
              //   expands the number of columns in M
              //assert (c.cols() == M.cols());
            }

            virtual ~TestBase() { }

            /*! Compute Z-statistics
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param output the matrix containing the output statistics (one column per hypothesis)
             *
             * This version ignores the statistics values themselves, and only exports Z-statistics
             *   (as these are what is used for statistical enhancement)
             */
            virtual void operator() (const matrix_type& shuffling_matrix, matrix_type& output) const;

            /*! Compute the statistics, including conversion to Z-score
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param stat the matrix containing the output statistics (one column per hypothesis)
             * @param zstat the matrix containing the Z-transformed statistics (one column per hypothesis)
             */
            virtual void operator() (const matrix_type& shuffling_matrix, matrix_type& stat, matrix_type& zstat) const = 0;


            size_t num_inputs () const { return M.rows(); }
            size_t num_elements () const { return y.cols(); }
            size_t num_hypotheses () const { return c.size(); }

            virtual size_t num_factors() const { return M.cols(); }

          protected:
            const matrix_type& y, M;
            const vector<Hypothesis>& c;
            std::shared_ptr<Math::Zstatistic> stat2z;

        };




        /** \addtogroup Statistics
        @{ */
        /*! A class to compute statistics from homoscedastic using a fixed General Linear Model.
         * This class produces a statistic per effect of interest: t-statistic for
         * t-tests, sqrt(F-statistic) for F-tests. It should be used in cases where:
         *   - the same design matrix is to be applied for all image elements being
         *     tested (it is thus able to pre-compute a number of matrices before testing,
         *     improving execution speed);
         *   - When the data are considered to be homoscedastic; that is, the variance is
         *     equivalent across all inputs.
         */
        class TestFixedHomoscedastic : public TestBase
        { MEMALIGN(TestFixedHomoscedastic)
          public:
            /*!
             * @param measurements a matrix storing the measured data across subjects in each column
             * @param design the design matrix
             * @param hypotheses a vector of Hypothesis instances
             */
            TestFixedHomoscedastic (const matrix_type& measurements,
                                    const matrix_type& design,
                                    const vector<Hypothesis>& hypotheses);

            /*! Compute the statistics
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param stats the vector containing the output statistics (one column per hypothesis)
             * @param zstats the vector containing the Z-transformed output statistics (one column per hypothesis)
             */
            void operator() (const matrix_type& shuffling_matrix, matrix_type& stats, matrix_type& zstats) const override;

          protected:
            // New classes to store information relevant to Freedman-Lane implementation
            vector<Hypothesis::Partition> partitions;
            const matrix_type pinvM;
            const matrix_type Rm;
            vector<matrix_type> XtX;
            vector<default_type> one_over_dof;

        };
        //! @}


        /** \addtogroup Statistics
        @{ */
        /*! A class to compute statistics from heteroscedastic data using a fixed General Linear Model.
         * This class produces a statistic per effect of interest: Aspin-Welsh v for
         * contrast vectors (i.e. Rank(C) == 1), sqrt(Welsh's v^2) for Rank(C) > 1. It should be used in
         * cases where:
         *   - The same design matrix is to be applied for all image elements being tested;
         *   - The input data are considered to be heteroscedastic; that is, the variance is not equivalent
         *     between all observations, but these can be placed into "variance groups", within which
         *     all observations can be considered to have the same variance.
         */
        class TestFixedHeteroscedastic : public TestFixedHomoscedastic
        { MEMALIGN(TestFixedHeteroscedastic)
          public:
            /*!
             * @param measurements a matrix storing the measured data across subjects in each column
             * @param design the design matrix
             * @param hypotheses a vector of Hypothesis instances
             * @param variance_groups a vector of integers corresponding to variance group assignments (should be indexed from zero)
             */
            TestFixedHeteroscedastic (const matrix_type& measurements,
                                      const matrix_type& design,
                                      const vector<Hypothesis>& hypotheses,
                                      const index_array_type& variance_groups);

            size_t num_variance_groups() const { return num_vgs; }

            /*! Compute the statistics
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param stats the vector containing the output statistics (one column per hypothesis)
             * @param zstats the vector containing the Z-transformed output statistics (one column per hypothesis)
             */
            void operator() (const matrix_type& shuffling_matrix, matrix_type& stats, matrix_type& zstats) const override;

          protected:
            // Variance group assignments
            const index_array_type& VG;
            // Total number of variance groups
            const size_t num_vgs;
            // Number of inputs that are part of each variance group
            vector<size_t> inputs_per_vg;
            // Might as well construct this in the functor rather than here,
            //   given 1 value for each VG is computed
            vector_type Rnn_sums;
            // Also store the reciprocal of these, so as to avoid repeated division
            vector_type inv_Rnn_sums;
            // Multiplication weights for calculation of gamma;
            //   varies depending on the rank of the contrast matrix of each hypothesis
            vector_type gamma_weights;

        };
        //! @}



        /** \addtogroup Statistics
        @{ */
        /*! A class to compute statistics from homoscedastic data using a variable General Linear Model.
         * This class produces a statistic per effect of interest. It should be used in
         * cases where:
         *   - Additional subject data must be imported into the design matrix before
         *     computing t- / F-values; the design matrix therefore does not remain fixed for all
         *     elements being tested, but varies depending on the particular element being tested.
         *     How additional data is imported into the design matrix will depend on the
         *     particular type of data being tested. Therefore an Importer class must be
         *     defined that is responsible for acquiring and vectorising these data.
         *   - Input data are considered to be homoscedastic; that is, the variance is
         *     equivalent across all inputs.
         */
        class TestVariableHomoscedastic : public TestBase
        { MEMALIGN(TestVariableHomoscedastic)
          public:
            TestVariableHomoscedastic (const vector<CohortDataImport>& importers,
                                       const matrix_type& measurements,
                                       const matrix_type& design,
                                       const vector<Hypothesis>& hypotheses,
                                       const bool nans_in_data,
                                       const bool nans_in_columns);

            /*! Compute the statistics
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param stat the vector containing the native output statistics (one column per hypothesis)
             * @param zstat the vector containing the Z-transformed output statistics (one column per hypothesis)
             *
             * In TestVariable* classes, this function additionally needs to import the
             * extra external data individually for each element tested.
             */
            void operator() (const matrix_type& shuffling_matrix, matrix_type& stat, matrix_type& zstat) const override;

            size_t num_factors() const override { return M.cols() + importers.size(); }

          protected:
            const vector<CohortDataImport>& importers;
            const bool nans_in_data, nans_in_columns;

            void get_mask (const size_t ie, BitSet&, const matrix_type& extra_columns) const;
            void apply_mask (const BitSet& mask,
                             matrix_type::ConstColXpr data,
                             const matrix_type& shuffling_matrix,
                             const matrix_type& extra_column_data,
                             matrix_type& Mfull_masked,
                             matrix_type& shuffling_matrix_masked,
                             vector_type& y_masked) const;

        };



        /** \addtogroup Statistics
        @{ */
        /*! A class to compute statistics from heteroscedastic data using a variable General Linear Model.
         * This class produces a statistic per effect of interest. It should be used in
         * cases where:
         *   - Additional subject data must be imported into the design matrix before
         *     computing t- / F-values; the design matrix therefore does not remain fixed for all
         *     elements being tested, but varies depending on the particular element being tested.
         *     How additional data is imported into the design matrix will depend on the
         *     particular type of data being tested. Therefore an Importer class must be
         *     defined that is responsible for acquiring and vectorising these data.
         *   - The input data are considered to be heteroscedastic; that is, the variance is not equivalent
         *     between all observations, but these can be placed into "variance groups", within which
         *     all observations can be considered to have the same variance.
         */
        class TestVariableHeteroscedastic : public TestVariableHomoscedastic
        { MEMALIGN(TestVariableHeteroscedastic)
          public:
            TestVariableHeteroscedastic (const vector<CohortDataImport>& importers,
                                         const matrix_type& measurements,
                                         const matrix_type& design,
                                         const vector<Hypothesis>& hypotheses,
                                         const index_array_type& variance_groups,
                                         const bool nans_in_data,
                                         const bool nans_in_columns);

            /*! Compute the statistics
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param stat the vector containing the native output statistics (one column per hypothesis)
             * @param zstat the vector containing the Z-transformed output statistics (one column per hypothesis)
             *
             * In TestVariable* classes, this function additionally needs to import the
             * extra external data individually for each element tested.
             */
            void operator() (const matrix_type& shuffling_matrix, matrix_type& stat, matrix_type& zstat) const override;

            size_t num_factors() const override { return M.cols() + importers.size(); }
            size_t num_variance_groups() const { return num_vgs; }

          protected:
            // Only a limited amount can be pre-calculated from the variance group information;
            //   other data may vary as rows of the design matrix & data are excluded
            const index_array_type& VG;
            const size_t num_vgs;
            vector_type gamma_weights;

            // Need to apply the row selection mask to the variance groups in addition to other data
            void apply_mask_VG (const BitSet& mask,
                                index_array_type& VG_masked,
                                index_array_type& VG_counts) const;

        };





      }
    }
  }
}


#endif

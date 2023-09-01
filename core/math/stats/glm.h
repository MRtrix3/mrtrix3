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
#include "file/config.h"
#include "math/condition_number.h"
#include "math/least_squares.h"
#include "math/zstatistic.h"
#include "math/stats/import.h"
#include "math/stats/typedefs.h"
#include "misc/bitset.h"

#define MRTRIX_USE_ZSTATISTIC_LOOKUP

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

        index_type batch_size();



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
        {
          public:

            class Partition
            {
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
                const index_type rank_x, rank_z;
            };

            Hypothesis (matrix_type::ConstRowXpr& in, const index_type index) :
                c (in),
                r (Math::rank (c)),
                F (false),
                i (index) { check_nonzero(); }

            Hypothesis (const matrix_type& in, const index_type index) :
                c (check_rank (in, index)),
                r (Math::rank (c)),
                F (true),
                i (index) { check_nonzero(); }

            template <class MatrixType>
            Partition partition (const MatrixType&) const;

            const matrix_type& matrix() const { return c; }
            index_type cols() const { return c.cols(); }
            index_type rank() const { return r; }
            bool is_F() const { return F; }
            std::string name() const { return std::string(F ? "F" : "t") + str(i+1); }

          private:
            const matrix_type c;
            const index_type r;
            const bool F;
            const index_type i;

            void check_nonzero() const;
            matrix_type check_rank (const matrix_type&, const index_type) const;
        };



        void check_design (const matrix_type&, const bool);

        index_array_type load_variance_groups (const index_type num_inputs);

        vector<Hypothesis> load_hypotheses (const std::string& file_path);



        // TODO Investigate how new GLM test classes could potentially be used to make
        //   the set of functions below redundant; not only for brevity, but also because
        //   many of the functions below will not be compatible with data that contains
        //   non-finite values
        // Would need to look into ways to provide access to intermediate data for all
        //   elements, but preferably without affecting performance of main loop






        /** \addtogroup Statistics
          @{ */
        /*! Compute a matrix of the beta coefficients
           * @param measurements a matrix storing the measured data across subjects in each column
           * @param design the design matrix
           * @return the matrix containing the output GLM betas (one column of factor betas per element)
           */
        matrix_type solve_betas (const measurements_matrix_type& measurements, const matrix_type& design);



        /*! Compute the effect of interest
         * @param measurements a matrix storing the measured data across subjects in each column
         * @param design the design matrix
         * @param hypothesis a Hypothesis class instance defining the effect of interest
         * @return the matrix containing the output absolute effect sizes (one column of element effect sizes per contrast)
         */
        vector_type abs_effect_size (const measurements_matrix_type& measurements, const matrix_type& design, const Hypothesis& hypothesis);
        matrix_type abs_effect_size (const measurements_matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses);



        /*! Compute the pooled standard deviation
         * @param measurements a matrix storing the measured data across subjects in each column
         * @param design the design matrix
         * @return the vector containing the output standard deviation for each element
         */
        matrix_type stdev (const measurements_matrix_type& measurements, const matrix_type& design);



        /*! Compute the standard deviation of each variance group
         * @param measurements a matrix storing the measured data across subjects in each column
         * @param design the design matrix
         * @return the vector containing the output standard deviation for each element
         */
        matrix_type stdev (const measurements_matrix_type& measurements, const matrix_type& design, const index_array_type& variance_groups);



        /*! Compute cohen's d, the standardised effect size between two means
         * @param measurements a matrix storing the measured data across subjects in each column
         * @param design the design matrix
         * @param hypothesis a Hypothesis class instance defining the effect of interest
         * @return the matrix containing the output standardised effect sizes (one column of element effect sizes per contrast)
         */
        vector_type std_effect_size (const measurements_matrix_type& measurements, const matrix_type& design, const Hypothesis& hypothesis);
        matrix_type std_effect_size (const measurements_matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses);



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
        void all_stats (const measurements_matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses, const index_array_type& variance_groups,
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
        void all_stats (const measurements_matrix_type& measurements, const matrix_type& design, const vector<CohortDataImport>& extra_columns, const vector<Hypothesis>& hypotheses, const index_array_type& variance_groups,
                        vector_type& cond, matrix_type& betas, matrix_type& abs_effect_size, matrix_type& std_effect_size, matrix_type& stdev);

        //! @}





        // Only exists to permit the use of a base class pointer in the TestBase class
        class SharedBase
        {
          public:
            virtual ~SharedBase() { }
        };

        using SharedHomoscedasticBase = Math::Zstatistic;

        class SharedFixedBase
        {
          public:
            SharedFixedBase (const measurements_matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses);
            vector<Hypothesis::Partition> partitions;
            const matrix_type pinvM;
            const matrix_type Rm;
        };

        class SharedVariableBase
        {
          public:
            SharedVariableBase (const vector<CohortDataImport>& importers,
                                const bool nans_in_data,
                                const bool nans_in_columns);
            const vector<CohortDataImport>& importers;
            const bool nans_in_data;
            const bool nans_in_columns;
        };

        class SharedHeteroscedasticBase
        {
          public:
            SharedHeteroscedasticBase (const vector<Hypothesis>& hypotheses, const index_array_type& variance_groups);
            index_array_type VG;
            size_t num_vgs;
            vector_type gamma_weights;
        };




        class TestBase
        {
          public:
            TestBase (const measurements_matrix_type& measurements, const matrix_type& design, const vector<Hypothesis>& hypotheses) :
                y (measurements),
                M (design),
                c (hypotheses)
            {
              assert (y.rows() == M.rows());
            }

            virtual ~TestBase() { }

            // This gets utilised within the multi-threading back-end to clone derived classes
            //   based on a pointer to this base class
            virtual std::unique_ptr<TestBase> __clone() const = 0;

            /*! Compute the statistics, including conversion to Z-score
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param stat the matrix containing the output statistics (one column per hypothesis)
             * @param zstat the matrix containing the Z-transformed statistics (one column per hypothesis)
             */
            virtual void operator() (const shuffle_matrix_type& shuffling_matrix, matrix_type& stat, matrix_type& zstat) = 0;

            index_type num_inputs () const { return M.rows(); }
            index_type num_elements () const { return y.cols(); }
            index_type num_hypotheses () const { return c.size(); }

            virtual index_type num_factors() const { return M.cols(); }

          protected:
            const measurements_matrix_type& y;
            const matrix_type& M;
            const vector<Hypothesis>& c;

            std::shared_ptr<SharedBase> shared;

        };




        /** \addtogroup Statistics
        @{ */
        /*! A class to compute statistics from homoscedastic using a fixed General Linear Model.
         * This class produces a statistic per effect of interest: t-statistic for
         * t-tests, F-statistic for F-tests. In both cases, corresponding Z-scores are also
         * calculated. It should be used in cases where:
         *   - the same design matrix is to be applied for all image elements being
         *     tested (it is thus able to pre-compute a number of matrices before testing,
         *     improving execution speed);
         *   - When the data are considered to be homoscedastic; that is, the variance is
         *     equivalent across all inputs.
         */
        class TestFixedHomoscedastic : public TestBase
        {
          public:

            class Shared : public SharedBase, public SharedFixedBase
#ifdef MRTRIX_USE_ZSTATISTIC_LOOKUP
            , public SharedHomoscedasticBase
#endif
            {
              public:
                Shared (const measurements_matrix_type& measurements,
                        const matrix_type& design,
                        const vector<Hypothesis>& hypotheses);
                ~Shared() { }
                vector<matrix_type> XtX;
                vector<size_t> dof;
                vector<default_type> one_over_dof;
            };


            /*!
             * @param measurements a matrix storing the measured data across subjects in each column
             * @param design the design matrix
             * @param hypotheses a vector of Hypothesis instances
             */
            TestFixedHomoscedastic (const measurements_matrix_type& measurements,
                                    const matrix_type& design,
                                    const vector<Hypothesis>& hypotheses);

            TestFixedHomoscedastic (const TestFixedHomoscedastic& that);

            std::unique_ptr<TestBase> __clone () const final;

            /*! Compute the statistics
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param stats the vector containing the output statistics (one column per hypothesis)
             * @param zstats the vector containing the Z-transformed output statistics (one column per hypothesis)
             */
            void operator() (const shuffle_matrix_type& shuffling_matrix, matrix_type& stats, matrix_type& zstats) override;

            index_type num_factors() const override { return M.cols(); }

            const Shared& S() const { return *dynamic_cast<const Shared* const> (shared.get()); }


          protected:
            // Temporaries
            matrix_type Sy, lambdas, residuals;
            vector<matrix_type> betas;
            vector_type sse;

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
        class TestFixedHeteroscedastic : public TestBase
        {
          public:

            class Shared : public SharedBase, public SharedFixedBase, public SharedHeteroscedasticBase
            {
              public:
                Shared (const measurements_matrix_type& measurements,
                        const matrix_type& design,
                        const vector<Hypothesis>& hypotheses,
                        const index_array_type& variance_groups);
                ~Shared() { }
                vector<size_t> inputs_per_vg;
                vector_type Rnn_sums;
                vector_type inv_Rnn_sums;
            };



            /*!
             * @param measurements a matrix storing the measured data across subjects in each column
             * @param design the design matrix
             * @param hypotheses a vector of Hypothesis instances
             * @param variance_groups a vector of integers corresponding to variance group assignments (should be indexed from zero)
             */
            TestFixedHeteroscedastic (const measurements_matrix_type& measurements,
                                      const matrix_type& design,
                                      const vector<Hypothesis>& hypotheses,
                                      const index_array_type& variance_groups);

            TestFixedHeteroscedastic (const TestFixedHeteroscedastic& that);

            std::unique_ptr<TestBase> __clone() const final;

            /*! Compute the statistics
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param stats the vector containing the output statistics (one column per hypothesis)
             * @param zstats the vector containing the Z-transformed output statistics (one column per hypothesis)
             */
            void operator() (const shuffle_matrix_type& shuffling_matrix, matrix_type& stats, matrix_type& zstats) override;

            index_type num_factors() const final { return M.cols(); }
            index_type num_variance_groups() const { return S().num_vgs; }

            const Shared& S() const { return *dynamic_cast<const Shared* const> (shared.get()); }

          protected:
            // Temporaries
            matrix_type Sy;
            matrix_type lambdas;
            Eigen::Array<default_type, Eigen::Dynamic, Eigen::Dynamic> sq_residuals;
            Eigen::Array<default_type, Eigen::Dynamic, Eigen::Dynamic> sse_per_vg;
            Eigen::Array<default_type, Eigen::Dynamic, Eigen::Dynamic> Wterms;
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> W;

        };
        //! @}




        class TestVariableBase : public TestBase
        {
          public:
            TestVariableBase (const measurements_matrix_type& measurements,
                              const matrix_type& design,
                              const vector<Hypothesis>& hypotheses,
                              const vector<CohortDataImport>& importers);

            TestVariableBase (const TestVariableBase& that);

            std::unique_ptr<TestBase> __clone() const override = 0;

            virtual index_type num_importers() const = 0;

          protected:
            // Temporaries
            matrix_type dof;
            measurements_matrix_type extra_column_data;
            BitSet element_mask, permuted_mask;
            shuffle_matrix_type intermediate_shuffling_matrix, shuffling_matrix_masked;
            matrix_type Mfull_masked, pinvMfull_masked, Rm;
            measurements_vector_type y_masked;
            vector_type Sy, lambda;

            template <class SharedType>
            void set_mask (const SharedType& s, const index_type ie);

            void apply_mask (const index_type ie, const shuffle_matrix_type& shuffling_matrix);

        };




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
        class TestVariableHomoscedastic : public TestVariableBase
        {
          public:
            class Shared : public SharedBase, public SharedVariableBase
#ifdef MRTRIX_USE_ZSTATISTIC_LOOKUP
            , public SharedHomoscedasticBase
#endif
            {
              public:
                Shared (const vector<CohortDataImport>& importers,
                        const bool nans_in_data,
                        const bool nans_in_columns);
                ~Shared() { }
            };

            TestVariableHomoscedastic (const measurements_matrix_type& measurements,
                                       const matrix_type& design,
                                       const vector<Hypothesis>& hypotheses,
                                       const vector<CohortDataImport>& importers,
                                       const bool nans_in_data,
                                       const bool nans_in_columns);

            TestVariableHomoscedastic (const TestVariableHomoscedastic& that);

            std::unique_ptr<TestBase> __clone() const final;

            /*! Compute the statistics
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param stat the vector containing the native output statistics (one column per hypothesis)
             * @param zstat the vector containing the Z-transformed output statistics (one column per hypothesis)
             *
             * In TestVariable* classes, this function additionally needs to import any
             * extra external data individually for each element tested.
             */
            void operator() (const shuffle_matrix_type& shuffling_matrix, matrix_type& stat, matrix_type& zstat) override;

            index_type num_factors() const final { return M.cols() + num_importers(); }
            index_type num_importers() const final { return S().importers.size(); }

            const Shared& S() const { return *dynamic_cast<const Shared* const> (shared.get()); }

          protected:
            vector<matrix_type> XtX, beta;

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
        class TestVariableHeteroscedastic : public TestVariableBase
        {
          public:
            class Shared : public SharedBase, public SharedVariableBase, public SharedHeteroscedasticBase
            {
              public:
                Shared (const vector<Hypothesis>& hypotheses,
                        const index_array_type& variance_groups,
                        const vector<CohortDataImport>& importers,
                        const bool nans_in_data,
                        const bool nans_in_columns);
                ~Shared() { }
            };

            TestVariableHeteroscedastic (const measurements_matrix_type& measurements,
                                         const matrix_type& design,
                                         const vector<Hypothesis>& hypotheses,
                                         const index_array_type& variance_groups,
                                         const vector<CohortDataImport>& importers,
                                         const bool nans_in_data,
                                         const bool nans_in_columns);

            TestVariableHeteroscedastic (const TestVariableHeteroscedastic& that);

            std::unique_ptr<TestBase> __clone() const final;

            /*! Compute the statistics
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param stat the vector containing the native output statistics (one column per hypothesis)
             * @param zstat the vector containing the Z-transformed output statistics (one column per hypothesis)
             *
             * In TestVariable* classes, this function additionally needs to import the
             * extra external data individually for each element tested.
             */
            void operator() (const shuffle_matrix_type& shuffling_matrix, matrix_type& stat, matrix_type& zstat) override;

            index_type num_factors() const final { return M.cols() + num_importers(); }
            index_type num_variance_groups() const { return S().num_vgs; }
            index_type num_importers() const final { return S().importers.size(); }

            const Shared& S() const { return *dynamic_cast<const Shared* const> (shared.get()); }

          protected:
            // Temporaries
            vector_type W, sq_residuals, sse_per_vg, Rnn_sums, Wterms;
            index_array_type VG_masked, VG_counts;

        };





      }
    }
  }
}


#endif

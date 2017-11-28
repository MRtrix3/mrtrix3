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


#ifndef __math_stats_glm_h__
#define __math_stats_glm_h__

#include "math/least_squares.h"
#include "math/stats/import.h"
#include "math/stats/typedefs.h"

namespace MR
{
  namespace Math
  {
    namespace Stats
    {
      namespace GLM
      {


        // TODO Define a base class to contain information regarding an individual contrast, and
        //   pre-compute as much as possible with regards to Freedman-Lane
        // Note: This can be constructed for both t-tests and F-tests
        //   (This is why the constructor is a template: Could be created either from a row()
        //   call on the contrast matrix, or from a matrix explicitly constructed from a set of
        //   rows from the contrast matrix, which is how an F-test is constructed.
        // TODO In the case of a single-row F-test, still need to be able to differentiate between
        //   a t-test and an F-test for the sake of signedness (and maybe taking the square root)
        //
        // TODO Exactly how this may be utilised depends on whether a fixed or variable design
        //   matrix will be used; ideally the interface to the Contrast class should deal with this
        class Contrast
        { MEMALIGN(Contrast)
          public:

            class Partition
            { MEMALIGN (Partition)
              public:
                Partition (const matrix_type& x, const matrix_type& z) :
                    X (x),
                    Z (z),
                    Hz (Z * Math::pinv (Z)),
                    Rz (matrix_type::Identity (Z.rows(), Z.rows()) - Hz),
                    rank_x (Math::rank (X)),
                    rank_z (Math::rank (Z)) { }
                // X = Component of design matrix related to effect of interest
                // Z = Component of design matrix related to nuisance regressors
                const matrix_type X, Z;
                // We would also like to automatically calculate, on creation of a partition:
                // Hz: Projection matrix of nuisance regressors only
                // Rz: Residual-forming matrix due to nuisance regressors only
                // rank_x: Rank of X
                // rank_z: Rank of Z
                const matrix_type Hz, Rz;
                const size_t rank_x, rank_z;
            };

            Contrast (matrix_type::ConstRowXpr& in) :
                c (in),
                r (Math::rank (c)),
                F (false) { }

            Contrast (const matrix_type& in) :
                c (in),
                r (Math::rank (c)),
                F (true) { }

            Partition operator() (const matrix_type&) const;

            operator const matrix_type& () const { return c; }
            ssize_t cols() const { return c.cols(); }
            size_t rank() const { return r; }
            bool is_F() const { return F; }

          private:
            const matrix_type c;
            const size_t r;
            const bool F;
        };



        extern const char* const column_ones_description;



        /** \addtogroup Statistics
          @{ */
        /*! Compute a matrix of the beta coefficients
           * @param measurements a matrix storing the measured data for each subject in a column
           * @param design the design matrix
           * @return the matrix containing the output GLM betas
           */
        matrix_type solve_betas (const matrix_type& measurements, const matrix_type& design);



        /*! Compute the effect of interest
         * @param measurements a matrix storing the measured data for each subject in a column
         * @param design the design matrix
         * @param contrast a matrix defining the group difference
         * @return the matrix containing the output effect
         */
        vector_type abs_effect_size (const matrix_type& measurements, const matrix_type& design, const Contrast& contrast);
        matrix_type abs_effect_size (const matrix_type& measurements, const matrix_type& design, const vector<Contrast>& contrasts);



        /*! Compute the pooled standard deviation
         * @param measurements a matrix storing the measured data for each subject in a column
         * @param design the design matrix
         * @return the matrix containing the output standard deviation
         */
        vector_type stdev (const matrix_type& measurements, const matrix_type& design);



        /*! Compute cohen's d, the standardised effect size between two means
         * @param measurements a matrix storing the measured data for each subject in a column
         * @param design the design matrix
         * @param contrast a matrix defining the group difference
         * @return the matrix containing the output standardised effect size
         */
        vector_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const Contrast& contrast);
        matrix_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const vector<Contrast>& contrasts);



        /*! Compute all GLM-related statistics
         * @param measurements a matrix storing the measured data for each subject in a column
         * @param design the design matrix
         * @param contrast a matrix defining the group difference
         * @param betas the matrix containing the output GLM betas
         * @param abs_effect_size the matrix containing the output effect
         * @param std_effect_size the matrix containing the output standardised effect size
         * @param stdev the matrix containing the output standard deviation
         */
        void all_stats (const matrix_type& measurements, const matrix_type& design, const vector<Contrast>& contrasts,
                        matrix_type& betas, matrix_type& abs_effect_size, matrix_type& std_effect_size, vector_type& stdev);
        //! @}






        // Define a base class for GLM tests
        // Should support both T-tests and F-tests
        // The latter will always produce 1 column only, whereas the former will produce the same number of columns as there are contrasts
        class TestBase
        { MEMALIGN(TestBase)
          public:
            TestBase (const matrix_type& measurements, const matrix_type& design, const vector<Contrast>& contrasts) :
                y (measurements),
                M (design),
                c (contrasts)
            {
              assert (y.cols() == M.rows());
              // Can no longer apply this assertion here; GLMTTestVariable later
              //   expands the number of columns in M
              //assert (c.cols() == M.cols());
            }

            /*! Compute the statistics
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param output the matrix containing the output statistics (one column per contrast)
             */
            virtual void operator() (const matrix_type& shuffling_matrix, matrix_type& output) const = 0;

            size_t num_elements () const { return y.rows(); }
            size_t num_outputs  () const { return c.size(); }
            size_t num_subjects () const { return M.rows(); }
            virtual size_t num_factors() const { return M.cols(); }

          protected:
            const matrix_type& y, M;
            const vector<Contrast>& c;

        };




        /** \addtogroup Statistics
        @{ */
        /*! A class to compute statistics using a fixed General Linear Model.
         * This class produces a statistic per contrast of interest: t-statistic for
         * t-tests, F-statistic for F-tests. It should be used in
         * cases where the same design matrix is to be applied for all image elements being
         * tested; able to pre-compute a number of matrices before testing, improving
         * execution speed.
         */
        class TestFixed : public TestBase
        { MEMALIGN(TestFixed)
          public:
            /*!
             * @param measurements a matrix storing the measured data for each subject in a column
             * @param design the design matrix
             * @param contrast a matrix containing the contrast of interest.
             */
            TestFixed (const matrix_type& measurements, const matrix_type& design, const vector<Contrast>& contrasts);

            /*! Compute the statistics
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param output the vector containing the output t-statistics (one column per contrast)
             */
            void operator() (const matrix_type& shuffling_matrix, matrix_type& output) const override;

          protected:
            // New classes to store information relevant to Freedman-Lane implementation
            vector<Contrast::Partition> partitions;
            const matrix_type pinvM;
            const matrix_type Rm;

        };
        //! @}



        /** \addtogroup Statistics
        @{ */
        /*! A class to compute statistics using a 'variable' General Linear Model.
         * This class produces a statistic per contrast of interest. It should be used in
         * cases where additional subject data must be imported into the design matrix before
         * computing t-values; the design matrix therefore does not remain fixed for all
         * elements being tested, but varies depending on the particular element being tested.
         *
         * How additional data is imported into the design matrix will depend on the
         * particular type of data being tested. Therefore an Importer class must be
         * defined that is responsible for acquiring and vectorising these data.
         */
        class TestVariable : public TestBase
        { MEMALIGN(TestVariable)
          public:
            TestVariable (const vector<CohortDataImport>& importers,
                          const matrix_type& measurements,
                          const matrix_type& design,
                          const vector<Contrast>& contrasts,
                          const bool nans_in_data,
                          const bool nans_in_columns);

            /*! Compute the statistics
             * @param shuffling_matrix a matrix to permute / sign flip the residuals (for permutation testing)
             * @param output the vector containing the output statistics
             *
             * In TestVariable, this function additionally needs to import the
             * extra external data individually for each element tested.
             */
            void operator() (const matrix_type& shuffling_matrix, matrix_type& output) const override;

            /*! Acquire the design matrix for the default permutation
             * (note that this needs to be re-run for each element being tested)
             * @param index the index of the element for which the design matrix is requested
             * @return the design matrix for that element, including imported data for extra columns
             */
            matrix_type default_design (const size_t index) const;

            size_t num_factors() const override { return M.cols() + importers.size(); }

          protected:
            const vector<CohortDataImport>& importers;
            const bool nans_in_data, nans_in_columns;

        };




      }
    }
  }
}


#endif

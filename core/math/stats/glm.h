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



      extern const char* const glm_column_ones_description;



      namespace GLM
      {

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
        matrix_type abs_effect_size (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrast);



        /*! Compute the pooled standard deviation
         * @param measurements a matrix storing the measured data for each subject in a column
         * @param design the design matrix
         * @return the matrix containing the output standard deviation
         */
        matrix_type stdev (const matrix_type& measurements, const matrix_type& design);



        /*! Compute cohen's d, the standardised effect size between two means
         * @param measurements a matrix storing the measured data for each subject in a column
         * @param design the design matrix
         * @param contrast a matrix defining the group difference
         * @return the matrix containing the output standardised effect size
         */
        matrix_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrast);



        /*! Compute all GLM-related statistics
         * @param measurements a matrix storing the measured data for each subject in a column
         * @param design the design matrix
         * @param contrast a matrix defining the group difference
         * @param betas the matrix containing the output GLM betas
         * @param abs_effect_size the matrix containing the output effect
         * @param std_effect_size the matrix containing the output standardised effect size
         * @param stdev the matrix containing the output standard deviation
         */
        void all_stats (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts,
                        matrix_type& betas, matrix_type& abs_effect_size, matrix_type& std_effect_size, matrix_type& stdev);
        //! @}

      } // End GLM namespace



      // Define a base class for GLM tests
      // Should support both T-tests and F-tests
      // The latter will always produce 1 column only, whereas the former will produce the same number of columns as there are contrasts
      class GLMTestBase
      { MEMALIGN(GLMTestBase)
        public:
          GLMTestBase (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts) :
            y (measurements),
            X (design),
            c (contrasts),
            outputs (c.rows())
          {
            assert (y.cols() == X.rows());
            // Can no longer apply this assertion here; GLMTTestVariable later
            //   expands the number of columns in X
            //assert (c.cols() == X.cols());
          }

          /*! Compute the statistics
           * @param perm_labelling a vector to shuffle the rows in the design matrix (for permutation testing)
           * @param output the matrix containing the output statistics (one vector per contrast)
           */
          virtual void operator() (const vector<size_t>& perm_labelling, matrix_type& output) const = 0;

          size_t num_elements () const { return y.rows(); }
          size_t num_factors  () const { return X.cols(); }
          size_t num_outputs  () const { return outputs; }
          size_t num_subjects () const { return X.rows(); }

        protected:
          const matrix_type& y, X, c;
          size_t outputs;

      };




      /** \addtogroup Statistics
      @{ */
      /*! A class to compute t-statistics using a fixed General Linear Model.
       * This class produces a t-statistic per contrast of interest. It should be used in
       * cases where the same design matrix is to be applied for all image elements being
       * tested; able to pre-compute a number of matrices before testing, improving
       * execution speed.
       */
      class GLMTTestFixed : public GLMTestBase
      { MEMALIGN(GLMTTestFixed)
        public:
          /*!
          * @param measurements a matrix storing the measured data for each subject in a column
          * @param design the design matrix
          * @param contrast a matrix containing the contrast of interest.
          */
          GLMTTestFixed (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrast);

          /*! Compute the t-statistics
          * @param perm_labelling a vector to shuffle the rows in the design matrix (for permutation testing)
          * @param output the vector containing the output t-statistics (one vector per contrast)
          */
          void operator() (const vector<size_t>& perm_labelling, matrix_type& output) const override;

        protected:
          const matrix_type pinvX, scaled_contrasts_t;

        private:

          //! GLM t-test incorporating various optimisations
          /*! note that the data, effects, and residual matrices are transposed.
           * This is to take advantage of Eigen's convention of storing
           * matrices in column-major format by default.
           *
           * This function makes use of member variable scaled_contrasts_t,
           * set up by the GLMTTestFixed constructor, which is also transposed. */
          void ttest (matrix_type& tvalues,
                      const matrix_type& design_t,
                      const matrix_type& pinv_design_t,
                      Eigen::Block<const matrix_type> measurements,
                      matrix_type& betas,
                      matrix_type& residuals_t) const;

          //! Pre-scaling of contrast matrix
          /*! This modulates the contents of the contrast matrix for compatibility
           * with member function ttest().
           *
           * Scaling is performed in a member function such that member scaled_contrasts_t
           * can be defined as const. */
          matrix_type calc_scaled_contrasts() const;

      };
      //! @}



      /** \addtogroup Statistics
      @{ */
      /*! A class to compute t-statistics using a 'variable' General Linear Model.
       * This class produces a t-statistic per contrast of interest. It should be used in
       * cases where additional subject data must be imported into the design matrix before
       * computing t-values; the design matrix therefore does not remain fixed for all
       * elements being tested, but varies depending on the particular element being tested.
       *
       * How additional data is imported into the design matrix will depend on the
       * particular type of data being tested. Therefore an Importer class must be
       * defined that is responsible for acquiring and vectorising these data.
       */
      class GLMTTestVariable : public GLMTestBase
      { MEMALIGN(GLMTTestVariable)
        public:
          GLMTTestVariable (const vector<CohortDataImport>& importers, const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts, const bool nans_in_data, const bool nans_in_columns);

          /*! Compute the t-statistics
           * @param perm_labelling a vector to shuffle the rows in the design matrix (for permutation testing)
           * @param output the vector containing the output t-statistics
           *
           * In GLMTTestVariable, this function additionally needs to import the
           * extra external data individually for each element tested.
           */
          void operator() (const vector<size_t>& perm_labelling, matrix_type& output) const override;

          /*! Acquire the design matrix for the default permutation
           * (note that this needs to be re-run for each element being tested)
           * @param index the index of the element for which the design matrix is requested
           * @return the design matrix for that element, including imported data for extra columns
           */
          matrix_type default_design (const size_t index) const;

        protected:
          const vector<CohortDataImport>& importers;
          const bool nans_in_data, nans_in_columns;

          //! generic GLM t-test
          /*! This version of the t-test function does not incorporate the
           * optimisations that are used in the GLMTTestFixed class, since
           * many are not applicable when the design matrix changes between
           * different elements.
           *
           * Since the design matrix varies between the different elements
           * being tested, this function only accepts testing of a single
           * vector of measurements at a time. */
           void ttest (matrix_type& tvalues,
                       const matrix_type& design,
                       matrix_type::ConstRowXpr measurements,
                       matrix_type& betas,
                       matrix_type& residuals) const;
      };


      /** \addtogroup Statistics
            @{ */
      /*! A class to compute F-statistics using a fixed General Linear Model.
       * This class produces a single F-statistic across all contrasts of interest.
       * NOT YET IMPLEMENTED
       */
      class GLMFTestFixed : public GLMTestBase { MEMALIGN(GLMFTestFixed)
        public:
          /*!
           * @param measurements a matrix storing the measured data for each subject in a column
           * @param design the design matrix
           * @param contrast a matrix containing the contrast of interest.
           */
          GLMFTestFixed (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts, const matrix_type& ftests);

          /*! Compute the F-statistics
           * @param perm_labelling a vector to shuffle the rows in the design matrix (for permutation testing)
           * @param output the vector containing the output f-statistics
           */
          void operator() (const vector<size_t>& perm_labelling, matrix_type& output) const override;

        protected:
          // TODO How to deal with f-tests that apply to specific contrasts only?
          const matrix_type ftests;
      };
      //! @}





    }
  }
}


#endif

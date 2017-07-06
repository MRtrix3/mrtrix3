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



          /** \addtogroup Statistics
          @{ */
          /*! Compute a matrix of the beta coefficients
          * @param measurements a matrix storing the measured data for each subject in a column
          * @param design the design matrix (unlike other packages a column of ones is NOT automatically added for correlation analysis)
          * @return the matrix containing the output effect
          */
          matrix_type solve_betas (const matrix_type& measurements, const matrix_type& design);



          /*! Compute the effect of interest
          * @param measurements a matrix storing the measured data for each subject in a column
          * @param design the design matrix (unlike other packages a column of ones is NOT automatically added for correlation analysis)
          * @param contrast a matrix defining the group difference
          * @return the matrix containing the output effect
          */
          matrix_type abs_effect_size (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrast);



          /*! Compute the pooled standard deviation
          * @param measurements a matrix storing the measured data for each subject in a column
          * @param design the design matrix (unlike other packages a column of ones is NOT automatically added for correlation analysis)
          * @return the matrix containing the output standard deviation size
          */
          matrix_type stdev (const matrix_type& measurements, const matrix_type& design);



          /*! Compute cohen's d, the standardised effect size between two means
          * @param measurements a matrix storing the measured data for each subject in a column
          * @param design the design matrix (unlike other packages a column of ones is NOT automatically added for correlation analysis)
          * @param contrast a matrix defining the group difference
          * @return the matrix containing the output standardised effect size
          */
          matrix_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrast);
          //! @}

      } // End GLM namespace



      // Define a base class for GLM tests
      // Should support both T-tests and F-tests
      // The latter will always produce 1 column only, whereas the former will produce the same number of columns as there are contrasts
      class GLMTestBase { MEMALIGN(GLMTestBase)
        public:
          GLMTestBase (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts) :
            y (measurements),
            X (design),
            c (contrasts),
            dim (c.rows())
          {
            assert (y.cols() == X.rows());
            assert (c.cols() == X.cols());
          }

          /*! Compute the statistics
           * @param perm_labelling a vector to shuffle the rows in the design matrix (for permutation testing)
           * @param output the matrix containing the output statistics (one vector per contrast)
           */
          virtual void operator() (const vector<size_t>& perm_labelling, matrix_type& output) const = 0;

          size_t num_subjects () const { return y.cols(); }
          size_t num_elements () const { return y.rows(); }
          size_t num_outputs  () const { return dim; }

        protected:
          const matrix_type& y, X, c;
          size_t dim;

      };




      /** \addtogroup Statistics
      @{ */
      /*! A class to compute t-statistics using a fixed General Linear Model.
       * This class produces a t-statistic per contrast of interest. It should be used in
       * cases where the same design matrix is to be applied for all image elements being
       * tested; able to pre-compute a number of matrices before testing, improving
       * execution speed.
       */
      class GLMTTestFixed : public GLMTestBase { MEMALIGN(GLMTTestFixed)
        public:
          /*!
          * @param measurements a matrix storing the measured data for each subject in a column
          * @param design the design matrix (unlike other packages a column of ones is NOT automatically added for correlation analysis)
          * @param contrast a matrix containing the contrast of interest.
          */
          GLMTTestFixed (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrast);

          /*! Compute the t-statistics
          * @param perm_labelling a vector to shuffle the rows in the design matrix (for permutation testing)
          * @param output the vector containing the output t-statistics (one vector per contrast)
          */
          void operator() (const vector<size_t>& perm_labelling, matrix_type& output) const override;

        protected:
          const matrix_type pinvX, scaled_contrasts;

        private:

          /*! This function pre-scales a contrast matrix in order to make conversion from GLM betas
           * to t-values more computationally efficient. */
          matrix_type calc_scaled_contrasts() const;

          //! generic GLM t-test
          /*! note that the data, effects, and residual matrices are transposed.
           * This is to take advantage of Eigen's convention of storing
           * matrices in column-major format by default.
           *
           * Note also that the contrast matrix should already have been scaled
           * using the GLM::scale_contrasts() function. */
          void ttest (matrix_type& tvalues,
                      const matrix_type& design,
                      const matrix_type& pinv_design,
                      Eigen::Block<const matrix_type> measurements,
                      matrix_type& betas,
                      matrix_type& residuals) const;
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
      class GLMTTestVariable : public GLMTestBase { NOMEMALIGN
        public:
          GLMTTestVariable (const vector<CohortDataImport>& importers, const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts);

          /*! Compute the t-statistics
           * @param perm_labelling a vector to shuffle the rows in the design matrix (for permutation testing)
           * @param output the vector containing the output t-statistics
           *
           * In GLMTTestVariable, this function additionally needs to import the
           * extra external data individually for each element tested.
           */
          void operator() (const vector<size_t>& perm_labelling, matrix_type& output) const override;

          // A function to acquire the design matrix for the default permutation
          //   (note that this needs to be re-run for each element being tested)
          matrix_type default_design (const matrix_type& design, const size_t index) const;

        protected:
          const vector<CohortDataImport>& importers;

          //! generic GLM t-test
          /*! note that the data, effects, and residual matrices are transposed.
           * This is to take advantage of Eigen's convention of storing
           * matrices in column-major format by default.
           *
           * This version does not require, or take advantage of, pre-calculation
           * of the pseudo-inverse of the design matrix, or pre-scaling of contrasts.
           */
          void ttest (matrix_type& tvalues,
                      const matrix_type& design,
                      const matrix_type& measurements,
                      matrix_type& betas,
                      matrix_type& residuals) const;
      };


      /** \addtogroup Statistics
            @{ */
      /*! A class to compute F-statistics using a fixed General Linear Model.
       * This class produces a single F-statistic across all contrasts of interest.
       */
      class GLMFTestFixed : public GLMTestBase { MEMALIGN(GLMFTestFixed)
        public:
          /*!
           * @param measurements a matrix storing the measured data for each subject in a column
           * @param design the design matrix (unlike other packages a column of ones is NOT automatically added for correlation analysis)
           * @param contrast a matrix containing the contrast of interest.
           */
          GLMFTestFixed (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts, const matrix_type& ftests);

          /*! Compute the F-statistics
           * @param perm_labelling a vector to shuffle the rows in the design matrix (for permutation testing)
           * @param output the vector containing the output f-statistics
           */
          void operator() (const vector<size_t>& perm_labelling, matrix_type& output) const override;

        protected:
          // TODO How to deal with contrast scaling?
          // TODO How to deal with f-tests that apply to specific contrasts only?
          const matrix_type ftests;
      };
      //! @}





    }
  }
}


#endif

/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */
#ifndef __math_stats_glm_h__
#define __math_stats_glm_h__

#include "math/least_squares.h"
#include "math/stats/typedefs.h"

namespace MR
{
  namespace Math
  {
    namespace Stats
    {



      namespace GLM
      {

        //! scale contrasts for use in t-test
        /*! Note each row of the contrast matrix will be treated as an independent contrast. The number
         * of elements in each contrast vector must equal the number of columns in the design matrix */
        matrix_type scale_contrasts (const matrix_type& contrasts, const matrix_type& design, const size_t degrees_of_freedom);



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
                    const matrix_type& measurements,
                    const matrix_type& scaled_contrasts,
                    matrix_type& betas,
                    matrix_type& residuals);



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



      /** \addtogroup Statistics
      @{ */
      /*! A class to compute t-statistics using a General Linear Model. */
      class GLMTTest
      {
        public:
          /*!
          * @param measurements a matrix storing the measured data for each subject in a column //TODO
          * @param design the design matrix (unlike other packages a column of ones is NOT automatically added for correlation analysis)
          * @param contrast a matrix containing the contrast of interest.
          */
          GLMTTest (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrast);

          /*! Compute the t-statistics
          * @param perm_labelling a vector to shuffle the rows in the design matrix (for permutation testing)
          * @param stats the vector containing the output t-statistics
          * @param max_stat the maximum t-statistic
          * @param min_stat the minimum t-statistic
          */
          void operator() (const std::vector<size_t>& perm_labelling, vector_type& stats) const;

          size_t num_subjects () const { return y.cols(); }
          size_t num_elements () const { return y.rows(); }

        protected:
          const matrix_type& y;
          matrix_type X, pinvX, scaled_contrasts;
      };
      //! @}

    }
  }
}


#endif

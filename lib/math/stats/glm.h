/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt and Donald Tournier 23/07/11.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __math_stats_glm_h__
#define __math_stats_glm_h__

#include <gsl/gsl_linalg.h>

#include "math/vector.h"
#include "math/matrix.h"

#define GLM_BATCH_SIZE 1024

namespace MR
{
  namespace Math
  {
    namespace Stats
    {

      typedef float value_type;


      inline void SVD_invert (Math::Matrix<double>& I, const Math::Matrix<double>& M, double precision = 1.0e-10)
      {
        size_t N = std::min (M.rows(), M.columns());
        Math::Matrix<double> U (M), V (N,N);
        Math::Vector<double> S (N), work (N);
        gsl_linalg_SV_decomp (U.gsl(), V.gsl(), S.gsl(), work.gsl());

        for (size_t n = 0; n < N; ++n) {
          double sv = S[n] < precision ? 0.0 : 1.0/S[n];
          for (size_t r = 0; r < N; ++r)
            V(r,n) *= sv;
        }

        Math::mult (I, 1.0, CblasNoTrans, V, CblasTrans, U);
      }


      inline size_t rank (const Math::Matrix<double>& M)
      {
        size_t N = std::min (M.rows(), M.columns());
        Math::Matrix<double> U (M), V (N,N);
        Math::Vector<double> S (N), work (N);
        gsl_linalg_SV_decomp (U.gsl(), V.gsl(), S.gsl(), work.gsl());
        for (size_t n = 0; n < N; ++n)
          if (S[n] < 1.0e-10)
            return n+1;
        return N;
      }



      namespace GLM {

        //! scale contrasts for use in t-test
        /*! Note each row of the contrast matrix will be treated as an
         * independent contrast. */
        template <typename ValueType> 
          inline Math::Matrix<ValueType> scale_contrasts (const Math::Matrix<ValueType>& contrasts, const Math::Matrix<ValueType>& design, size_t degrees_of_freedom)
          {
            Math::Matrix<ValueType> XtX;
            Math::mult (XtX, ValueType(1.0), CblasTrans, design, CblasNoTrans, design);
            {
              Math::Matrix<double> d_XtX = XtX;
              Math::Matrix<double> d_pinv_XtX;
              SVD_invert (d_pinv_XtX, d_XtX);
              XtX = d_pinv_XtX;
            }

            // make sure contrast is a column vector:
            Math::Matrix<ValueType> scaled_contrasts (contrasts);
            if (scaled_contrasts.columns() > 1 && scaled_contrasts.rows() > 1)
              throw Exception ("too many columns in contrast matrix: this implementation currently only supports univariate GLM");
            if (scaled_contrasts.rows() > 1)
              scaled_contrasts = Math::transpose (scaled_contrasts);
            scaled_contrasts.resize (scaled_contrasts.rows(), design.columns());

            for (size_t n = 0; n < contrasts.rows(); ++n) {
              Math::Vector<ValueType> pinv_XtX_c;
              Math::mult (pinv_XtX_c, XtX, contrasts.row(n));
              scaled_contrasts.row(n) *= Math::sqrt (ValueType(degrees_of_freedom) / Math::dot (contrasts.row(n), pinv_XtX_c));
            }

            return scaled_contrasts;
          }



        //! generic GLM t-test
        /*! note that the data, effects, and residual matrices are transposed.
         * This is to take advantage of the GSL's convention of storing
         * matrices in column-major format. 
         *
         * Note also that the contrast matrix should already have been scaled
         * using the GLM::scale_contrasts() function. */
        template <typename ValueType> 
          inline void ttest (
              Math::Matrix<ValueType>& tvalues,
              const Math::Matrix<ValueType>& design, 
              const Math::Matrix<ValueType>& pinv_design, 
              const Math::Matrix<ValueType>& measurements, 
              const Math::Matrix<ValueType>& scaled_contrasts,
              Math::Matrix<ValueType>& betas,
              Math::Matrix<ValueType>& residuals) 
          { 
            Math::mult (betas, ValueType(1.0), CblasNoTrans, measurements, CblasTrans, pinv_design);
            Math::mult (residuals, ValueType(-1.0), CblasNoTrans, betas, CblasTrans, design);
            residuals += measurements;
            Math::mult (tvalues, ValueType(1.0), CblasNoTrans, betas, CblasTrans, scaled_contrasts);
            for (size_t n = 0; n < tvalues.rows(); ++n)
              tvalues.row(n) /= Math::norm (residuals.row(n));
          }


          template <typename ValueType>
            inline void solve_betas (const Math::Matrix<ValueType>& measurements,
                                     const Math::Matrix<ValueType>& design,
                                     Math::Matrix<ValueType>& betas) {
            Math::Matrix<double> d_pinvX, d_X (design);
            SVD_invert (d_pinvX, d_X);
            Math::Matrix<ValueType> pinvX = d_pinvX;
            Math::mult (betas, ValueType(1.0), CblasNoTrans, measurements, CblasTrans, pinvX);
          }



          /** \addtogroup Statistics
          @{ */
          /*! Compute the effect of interest
          * @param measurements a matrix storing the measured data for each subject in a column
          * @param design the design matrix (unlike other packages a column of ones is NOT automatically added for correlation analysis)
          * @param contrast a matrix defining the group difference
          * @param effect the matrix containing the output effect
          */
          template <typename ValueType>
            inline void effect (const Math::Matrix<ValueType>& measurements,
                                  const Math::Matrix<ValueType>& design,
                                  const Math::Matrix<ValueType>& contrast,
                                  Math::Matrix<ValueType>& effect) {
              Math::Matrix<ValueType> betas;
              GLM::solve_betas (measurements, design, betas);
              Math::mult (effect, ValueType(1.0), CblasNoTrans, betas, CblasTrans, contrast);
          }


          /*! Compute cohen's d, the standardised effect size between two means
          * @param measurements a matrix storing the measured data for each subject in a column
          * @param design the design matrix (unlike other packages a column of ones is NOT automatically added for correlation analysis)
          * @param contrast a matrix defining the group difference
          * @param cohens_d the matrix containing the output standardised effect size
          */
          template <typename ValueType>
            inline void std_effect_size (const Math::Matrix<ValueType>& measurements,
                                  const Math::Matrix<ValueType>& design,
                                  const Math::Matrix<ValueType>& contrast,
                                  Math::Matrix<ValueType>& cohens_d) {
              Math::Matrix<ValueType> betas;
              GLM::solve_betas (measurements, design, betas);
              Math::Matrix<ValueType> residuals;
              Math::mult (residuals, ValueType(-1.0), CblasNoTrans, betas, CblasTrans, design);
              residuals += measurements;
              residuals.pow(2.0);
              Math::Matrix<ValueType> one_over_dof (measurements.columns(), 1);
              one_over_dof = 1.0 / ValueType(design.rows()-rank(design));
              Math::Matrix<ValueType> stdev;
              Math::mult (stdev, residuals, one_over_dof);
              stdev.sqrt();
              Math::mult (cohens_d, ValueType(1.0), CblasNoTrans, betas, CblasTrans, contrast);
              cohens_d /= stdev;
          }



          /*! Compute the pooled standard deviation
          * @param measurements a matrix storing the measured data for each subject in a column
          * @param design the design matrix (unlike other packages a column of ones is NOT automatically added for correlation analysis)
          * @param stdev the matrix containing the output standard deviation size
          */
          template <typename ValueType>
            inline void stdev (const Math::Matrix<ValueType>& measurements,
                               const Math::Matrix<ValueType>& design,
                               Math::Matrix<ValueType>& stdev) {
              Math::Matrix<ValueType> betas;
              GLM::solve_betas (measurements, design, betas);
              Math::Matrix<ValueType> residuals;
              Math::mult (residuals, ValueType(-1.0), CblasNoTrans, betas, CblasTrans, design);
              residuals += measurements;
              residuals.pow(2.0);
              Math::Matrix<ValueType> one_over_dof (measurements.columns(), 1);
              one_over_dof = 1.0 / ValueType(design.rows()-rank(design));
              Math::mult (stdev, residuals, one_over_dof);
              stdev.sqrt();
          }
          //! @}
      }

      /** \addtogroup Statistics
      @{ */
      /*! A class to compute t-statistics using a General Linear Model. */
      class GLMTTest
      {
        public:
          /*!
          * @param measurements a matrix storing the measured data for each subject in a column
          * @param design the design matrix (unlike other packages a column of ones is NOT automatically added for correlation analysis)
          * @param contrast a matrix containing the contrast of interest. Note that the opposite contrast will automatically be computed at the same time.
          */
          GLMTTest (const Math::Matrix<value_type>& measurements,
                    const Math::Matrix<value_type>& design,
                    const Math::Matrix<value_type>& contrast) : 
            y (measurements),
            X (design),
            scaled_contrasts (GLM::scale_contrasts (contrast, X, X.rows()-rank(X)))
          {
            Math::Matrix<double> d_pinvX, d_X (X);
            SVD_invert (d_pinvX, d_X);
            pinvX = d_pinvX;
          }

          /*! Compute the t-statistics
          * @param perm_labelling a vector to shuffle the rows in the design matrix (for permutation testing)
          * @param stats the vector containing the output t-statistics
          * @param max_stat the maximum t-statistic
          * @param min_stat the minimum t-statistic
          */
          void operator() (const std::vector<size_t>& perm_labelling, std::vector<value_type>& stats, value_type& max_stat, value_type& min_stat) const
          {
            stats.resize (y.rows(), 0.0);
            Math::Matrix<value_type> tvalues, betas, residuals, SX, pinvSX;

            SX.allocate (X);
            pinvSX.allocate (pinvX);
            for (size_t i = 0; i < X.rows(); ++i) {
              // TODO: check whether we should permute rows or columns
              SX.row(i) = X.row (perm_labelling[i]); 
              pinvSX.column(i) = pinvX.column (perm_labelling[i]);
            }


            for (size_t i = 0; i < y.rows(); i += GLM_BATCH_SIZE) {
              GLM::ttest (tvalues, SX, pinvSX, y.sub(i, std::min (i+GLM_BATCH_SIZE, y.rows()), 0, y.columns()),
                  scaled_contrasts, betas, residuals);
              for (size_t n = 0; n < tvalues.rows(); ++n) {
                value_type val = tvalues(n,0);
                if (val > max_stat)
                  max_stat = val;
                if (val < min_stat)
                  min_stat = val;
                stats[i+n] = val;
              }
            }
          }

          size_t num_samples () const { return y.columns(); }

        protected:
          const Math::Matrix<value_type>& y;
          Math::Matrix<value_type> X, pinvX, scaled_contrasts;
      };
      //! @}

    }
  }
}


#endif

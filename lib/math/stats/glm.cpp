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

#include "math/stats/glm.h"

#define GLM_BATCH_SIZE 1024

namespace MR
{
  namespace Math
  {
    namespace Stats
    {



      namespace GLM
      {

        matrix_type scale_contrasts (const matrix_type& contrasts, const matrix_type& design, const size_t degrees_of_freedom)
        {
          assert (contrasts.cols() == design.cols());
          const matrix_type XtX = design.transpose() * design;
          const matrix_type pinv_XtX =  (XtX.transpose() * XtX).fullPivLu().solve (XtX.transpose());
          matrix_type scaled_contrasts (contrasts);

          for (size_t n = 0; n < size_t(contrasts.rows()); ++n) {
            auto pinv_XtX_c = pinv_XtX * contrasts.row(n).transpose();
            scaled_contrasts.row(n) *= std::sqrt (value_type(degrees_of_freedom) / contrasts.row(n).dot (pinv_XtX_c));
          }
          return scaled_contrasts;
        }



        void ttest (matrix_type& tvalues,
                    const matrix_type& design,
                    const matrix_type& pinv_design,
                    const matrix_type& measurements,
                    const matrix_type& scaled_contrasts,
                    matrix_type& betas,
                    matrix_type& residuals)
        {
          betas.noalias() = measurements * pinv_design;
          residuals.noalias() = measurements - betas * design;
          tvalues.noalias() = betas * scaled_contrasts;
          for (size_t n = 0; n < size_t(tvalues.rows()); ++n)
            tvalues.row(n).array() /= residuals.row(n).norm();
        }



        matrix_type solve_betas (const matrix_type& measurements, const matrix_type& design)
        {
          return design.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(measurements.transpose());
        }



        matrix_type abs_effect_size (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrast)
        {
          return contrast * solve_betas (measurements, design);
        }


        matrix_type stdev (const matrix_type& measurements, const matrix_type& design)
        {
          matrix_type residuals = measurements.transpose() - design * solve_betas (measurements, design); //TODO
          residuals = residuals.array().pow(2.0);
          matrix_type one_over_dof (1, measurements.cols());  //TODO supply transposed measurements
          one_over_dof.fill (1.0 / value_type(design.rows()-Math::rank (design)));
          return (one_over_dof * residuals).array().sqrt();
        }


        matrix_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrast)
        {
          return abs_effect_size (measurements, design, contrast).array() / stdev (measurements, design).array();
        }
      }








      GLMTTest::GLMTTest (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrast) :
          y (measurements),
          X (design),
          scaled_contrasts (GLM::scale_contrasts (contrast, X, X.rows()-rank(X)).transpose())
      {
        pinvX = Math::pinv (X);
      }



      void GLMTTest::operator() (const std::vector<size_t>& perm_labelling, vector_type& stats) const
      {
        stats = vector_type::Zero (y.rows());
        matrix_type tvalues, betas, residuals, SX, pinvSX;

        SX.resize (X.rows(), X.cols());
        pinvSX.resize (pinvX.rows(), pinvX.cols());
        for (ssize_t i = 0; i < X.rows(); ++i) {
          SX.row(i) = X.row (perm_labelling[i]);
          pinvSX.col(i) = pinvX.col (perm_labelling[i]);
        }

        pinvSX.transposeInPlace();
        SX.transposeInPlace();
        for (ssize_t i = 0; i < y.rows(); i += GLM_BATCH_SIZE) {
          const matrix_type tmp = y.block (i, 0, std::min (GLM_BATCH_SIZE, (int)(y.rows()-i)), y.cols());
          GLM::ttest (tvalues, SX, pinvSX, tmp, scaled_contrasts, betas, residuals);
          for (ssize_t n = 0; n < tvalues.rows(); ++n) {
            value_type val = tvalues(n,0);
            if (!std::isfinite (val))
              val = value_type(0);
            stats[i+n] = val;
          }
        }
      }




    }
  }
}


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

namespace MR
{
  namespace Math
  {
    namespace Stats
    {

      typedef float value_type;


      inline void SVD_invert (Math::Matrix<double>& I, const Math::Matrix<double>& M)
      {
        size_t N = std::min (M.rows(), M.columns());
        Math::Matrix<double> U (M), V (N,N);
        Math::Vector<double> S (N), work (N);
        gsl_linalg_SV_decomp (U.gsl(), V.gsl(), S.gsl(), work.gsl());

        for (size_t n = 0; n < N; ++n) {
          double sv = S[n] < 1.0e-10 ? 0.0 : 1.0/S[n];
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

      class GLMTTest
      {
        public:
          GLMTTest (const Math::Matrix<value_type>& data,
                    const Math::Matrix<value_type>& design_matrix,
                    const Math::Matrix<value_type>& contrast_matrix) : data (data)
          {
            // make sure contrast is a column vector:
            Math::Matrix<double> c = contrast_matrix;
            if (c.columns() > 1 && c.rows() > 1)
              throw Exception ("too many columns in contrast matrix: this implementation currently only supports univariate GLM");
            if (c.columns() > 1)
              c = Math::transpose (c);

            // form X_0:
            Math::Matrix<double> T (c.rows(), c.rows());
            T.identity();
            Math::Matrix<double> pinv_c;
            SVD_invert (pinv_c, c);
            Math::mult (T , 1.0, -1.0, CblasNoTrans, c, CblasNoTrans, pinv_c);

            Math::Matrix<double> X0, X = design_matrix;
            Math::mult (X0, X, T );

            // form R_0:
            Math::Matrix<double> d_R0 (X0.rows(), X0.rows());
            d_R0.identity();
            Math::Matrix<double> pinv_X0;
            SVD_invert (pinv_X0, X0);
            Math::mult (d_R0, 1.0, -1.0, CblasNoTrans, X0, CblasNoTrans, pinv_X0);

            // form X_1^*:
            Math::Matrix<double> X1;
            Math::mult (T, 1.0, CblasNoTrans, X, CblasTrans, pinv_c);
            Math::mult (X1, d_R0, T);
            Math::Matrix<double> pinv_X1;
            SVD_invert (pinv_X1, X1);

            // form M:
            Math::Matrix<double> d_M (X1.rows()+1, X1.rows());
            d_M.sub(0,1,0,d_M.columns()) = pinv_X1;
            Math::Matrix<double> R1 = d_M.sub(1,d_M.rows(),0,d_M.columns());
            R1.identity();
            Math::mult (R1, 1.0, -1.0, CblasNoTrans, X1, CblasNoTrans, pinv_X1);

            // precompute kappa:
            Math::mult (T, 1.0, CblasTrans, X1, CblasNoTrans, X1);
            kappa = Math::sqrt (T(0,0) * (X.rows() - rank(X)));


            // store using request value type:
            R0 = d_R0;
            M = d_M;
          }

          // Compute the test statistic
          void operator() (const std::vector<size_t>& perm_labelling, std::vector<value_type>& stats, value_type& max_stat, value_type& min_stat) const
          {
            stats.resize (data.rows(), 0.0);
            Math::Matrix<value_type> Mp, SR0 (R0.rows(), R0.columns());
            for (size_t i = 0; i < R0.columns(); ++i)
              SR0.row(i) = R0.row (perm_labelling[i]); // TODO: check whether we should permute rows or columns

            Math::mult (Mp, M, SR0);

            for (size_t i = 0; i < data.rows(); ++i) {
              stats[i] = compute_tstatistic (data.row(i), Mp);
              if (stats[i] > max_stat)
                max_stat = stats[i];
              if (stats[i] < min_stat)
                min_stat = stats[i];
            }
          }


        protected:
          value_type compute_tstatistic (const Math::Vector<value_type>& values, const Math::Matrix<value_type>& Mp) const
          {
            Math::Vector<value_type> e;
            Math::mult (e, Mp, values);
            return kappa * e[0] / Math::norm (e.sub(1,e.size()));
          }

          const Math::Matrix<value_type>& data;
          value_type kappa;
          Math::Matrix<value_type> M, R0;

      };

    }
  }
}


#endif

/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

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

#ifndef __math_constrained_least_squares_h__
#define __math_constrained_least_squares_h__

#include <set>

#include "math/cholesky.h"
#include "math/LU.h"

namespace MR
{
  namespace Math
  {

    /** @addtogroup linalg
      @{ */

    /** @defgroup icls Inequality-constrained Least-squares 
      @{ */





    namespace ICLS {

      template <typename ValueType>
        class Problem {
          public:
            Problem () { }
            Problem (const Matrix<ValueType>& problem_matrix, 
                const Matrix<ValueType>& constraint_matrix, 
                ValueType min_norm_regularisation = 1.e-8,
                ValueType tolerance = ValueType(1.0e-10), 
                size_t max_iterations = 1000,
                ValueType min_quadratic_constraint_factor = ValueType(1.0), 
                ValueType max_quadratic_constraint_factor = ValueType(1.0e8)) :
              H (problem_matrix),
              chol_HtH (H.columns(), H.columns()), 
              mu_min (min_quadratic_constraint_factor),
              mu_max (max_quadratic_constraint_factor),
              tol (tolerance), 
              tol2 (pow2 (tolerance)), 
              max_niter (max_iterations) {

                // form quadratic problem matrix H'*H:
                rankN_update (chol_HtH, H, CblasTrans, CblasLower);
                // add minimum norm constraint:
                chol_HtH.diagonal() += min_norm_regularisation;
                // get Cholesky decomposition:
                Cholesky::decomp (chol_HtH);

                // form (transpose of) matrix projecting b onto preconditioned
                // quadratic problem chol_HtH\H:
                b2d = H;
                solve_triangular (chol_HtH, b2d, CblasRight);

                // project constraint onto preconditioned quadratic domain, 
                // and normalise each row to help preconditioning (the norm of
                // each row is irrelevant to the constraint itself):
                B = constraint_matrix;
                solve_triangular (chol_HtH, B, CblasRight);
                for (size_t n = 0; n < B.rows(); ++n) {
                  typename Vector<ValueType>::View row (B.row(n));
                  normalise (row);
                }
              }

            Matrix<ValueType> H, chol_HtH, B, b2d;
            ValueType mu_min, mu_max, tol, tol2;
            size_t max_niter;
        };




      template <typename ValueType>
        class Solver {
          public:
            Solver (const Problem<ValueType>& problem) :
              iter(0),
              P (problem),
              HtH_muBtB (P.chol_HtH.rows(), P.chol_HtH.columns()),
              Bk (P.B.rows(), P.B.columns()),
              dx (HtH_muBtB.rows()),
              d (HtH_muBtB.rows()),
              c (P.B.rows()),
              lambda (c.size()),
              prev_lambda (c.size()),
              lambda_k (lambda.size()),
              p (HtH_muBtB.rows()) { }

            int operator() (Vector<ValueType>& x, const Vector<ValueType>& b) 
            {
              //std::ofstream y_stream ("y.txt");
              //std::ofstream lambda_stream ("l.txt");
              //std::ofstream  c_stream ("c.txt");
              //std::ofstream mu_stream ("mu.txt");
              //std::ofstream t_stream ("t.txt");

              //P.chol_HtH.save ("L.txt", 16);

              ValueType mu = P.mu_min;
              lambda = 0.0;
              mult (d, ValueType (1.0), CblasTrans, P.b2d, b);
              size_t num_under_tol = 0;

              dx = x = d;

              // initial estimate of constraint values:
              mult (c, P.B, x);
              
              iter = 0;
              for (; iter < P.max_niter; ++iter) {
                //y_stream << x << std::endl;
                //lambda_stream << lambda << std::endl;
                //c_stream << c << std::endl;
                //mu_stream << mu << std::endl;

                // form matrix of active constraints and corresponding lambdas:
                Bk.allocate (P.B);
                lambda_k.allocate (P.B.rows());
                size_t idx = 0;
                for (size_t n = 0; n < c.size(); ++n) {
                  if (c[n] - mu*lambda[n] <= 0.0) {
                    Bk.row (idx) = P.B.row (n);
                    lambda_k[idx] = lambda[n];
                    ++idx;
                  }
                }

                if (idx) {
                  Bk.resize (idx, P.B.columns());
                  lambda_k.resize (idx);

                  // update problem matrix (which is identity after preconditioning)
                  // with the scaled constraint matrix:
                  rankN_update (HtH_muBtB, Bk, CblasTrans, CblasLower, mu);
                  HtH_muBtB.diagonal() += 1.0;

                  // add constraints to RHS:
                  mult (x, ValueType (1.0), CblasTrans, Bk, lambda_k);
                  x += d;

                  // solve for x by Cholesky decomposition:
                  Cholesky::decomp (HtH_muBtB);
                  Cholesky::solve (x, HtH_muBtB);
                }
                else {
                  // no active constraints
                  x = d;
                  if (iter == 0) { // unconstrained solution does not violate constraints
                    solve_triangular (x, P.chol_HtH);
                    return iter;
                  }
                  HtH_muBtB.identity();
                }

                // check for convergence:
                dx -= x;
                ValueType t = norm2 (dx) / norm2 (x);
                if (t < P.tol2) {
                  // project back to unconditioned domain:
                  solve_triangular (x, P.chol_HtH);
                  return iter;
                }
                if (t < P.tol) {
                  ++num_under_tol;
                  if (num_under_tol > 5) {
                    solve_triangular (x, P.chol_HtH);
                    return iter;
                  }
                }


                // compute constraint values:
                mult (c, P.B, x);


                // update lambda and compute change in lambda values:
                prev_lambda = lambda;
                ValueType min_c = 0.0;
                for (size_t n = 0; n < lambda.size(); ++n) {
                  min_c = std::min (min_c, c[n]);
                  lambda[n] = std::max (ValueType(0.0), lambda[n] - mu * c[n]);
                }

                ValueType old_mu = mu;
                mu = -100.0/min_c;
                mu = std::min (mu, P.mu_max);
                mu = std::max (mu, P.mu_min);

                if (mu / old_mu < 0.1) 
                  lambda = prev_lambda;

                dx = x;
              }

              solve_triangular (x, P.chol_HtH);
              throw Exception ("constrained least-squares failed to converge");
            }

            size_t iterations () const {
              return iter;
            }

          protected:
            size_t iter;
            const Problem<ValueType>& P;
            Matrix<ValueType> HtH_muBtB, Bk;
            Vector<ValueType> dx, d, c, lambda, prev_lambda, lambda_k;
            Permutation p;
        };



    }

    /** @} */
    /** @} */

  }
}

#endif









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

//#define DEBUG_ICLS

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
            //! set up constrained least-squares problem
            /*! the problem is to solve norm(\e Hx - \e b) subject to \e Ax >=
             * 0. This sets up a class to be re-used and shared across threads,
             * assuming the matrices \e H & \e A don't change, but the vector
             * \e b does. 
             *  
             *  - \a problem_matrix: \e H
             *  - \a constraint_matrix: \e A
             *  - \a solution_min_norm_regularisation: an additional minimum
             *  norm constraint that will be added to the problem cost
             *  function, to stablise ill-posed problems.
             *  - \a solution_min_norm_regularisation: used to
             *  stabilise the estimation of the Lagrangian multipliers to
             *  handle cases where they become degenerate (otherwise leads to
             *  errors in the Cholesky decomposition). Default is zero, set it
             *  to a small value such as 1e-10 to help with these kinds of
             *  problem.
             *  - \a max_iterations: the maximum number of iterations to run.
             *  If zero (default), this number is set to 10x the size of \e x.
             */
            Problem (const Matrix<ValueType>& problem_matrix, 
                const Matrix<ValueType>& constraint_matrix, 
                ValueType solution_min_norm_regularisation = 0.0,
                ValueType constraint_min_norm_regularisation = 0.0,
                size_t max_iterations = 0,
                ValueType tolerance = 0.0) :
              H (problem_matrix),
              chol_HtH (H.columns(), H.columns()), 
              lambda_min_norm (constraint_min_norm_regularisation),
              tol (tolerance),
              max_niter (max_iterations ? max_iterations : 10*problem_matrix.columns()) {

                // form quadratic problem matrix H'*H:
                rankN_update (chol_HtH, H, CblasTrans, CblasLower);
                // add minimum norm constraint:
                chol_HtH.diagonal() += solution_min_norm_regularisation * max (chol_HtH.diagonal());
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
            ValueType lambda_min_norm, tol;
            size_t max_niter;
        };




      template <typename ValueType>
        class Solver {
          public:
            Solver (const Problem<ValueType>& problem) :
              P (problem),
              BtB (P.chol_HtH.rows(), P.chol_HtH.columns()),
              B_active (P.B.rows(), P.B.columns()),
              y_u (BtB.rows()),
              c (P.B.rows()),
              c_u (P.B.rows()),
              lambda (c.size()),
              lambda_prev (c.size()),
              l (lambda.size()),
              active (lambda.size(), false) { }

            size_t operator() (Vector<ValueType>& x, const Vector<ValueType>& b) 
            {
#ifdef MRTRIX_ICLS_DEBUG
              std::ofstream l_stream ("l.txt");
              std::ofstream n_stream ("n.txt");
#endif
              // compute unconstrained solution:
              mult (y_u, ValueType (1.0), CblasTrans, P.b2d, b);
              // compute constraint violaions for unconstrained solution:
              mult (c_u, P.B, y_u);

              // set all Lagrangian multipliers to zero:
              lambda = 0.0;
              lambda_prev = 0.0;
              // set active set empty:
              std::fill (active.begin(), active.end(), false);

              // initial estimate of constraint values:
              c = c_u;
              // initial estimate of solution:
              x = y_u;

              size_t min_c_index;
              size_t niter = 0;

              while (min (c, min_c_index) < -P.tol) {
                bool active_set_changed = !active[min_c_index];
                active[min_c_index] = true;

                while (1) {
                  // form submatrix of active constraints:
                  B_active.allocate (P.B);
                  l.allocate (P.B.rows());
                  size_t num_active = 0;
                  for (size_t n = 0; n < active.size(); ++n) {
                    if (active[n]) {
                      B_active.row (num_active) = P.B.row (n);
                      l[num_active] = -c_u[n];
                      ++num_active;
                    }
                  }
                  B_active.resize (num_active, P.B.columns());
                  l.resize (num_active);

                  BtB.allocate (num_active, num_active);
                  // solve for l in B*B'l = -c_u by Cholesky decomposition:
                  rankN_update (BtB, B_active, CblasNoTrans, CblasLower);
                  BtB.diagonal() += P.lambda_min_norm;
                  Cholesky::decomp (BtB);
                  Cholesky::solve (l, BtB);

                  // update lambda values in full vector 
                  // and identify worst offender if any lambda < 0
                  // by projection from previous onto feasible 
                  // subset (i.e. l>=0):
                  ValueType s_min = std::numeric_limits<ValueType>::infinity();
                  size_t s_min_index = 0;
                  size_t a = 0;
                  for (size_t n = 0; n < active.size(); ++n) {
                    if (active[n]) {
                      if (l[a] < 0.0) {
                        ValueType s = lambda_prev[n] / (lambda_prev[n] - l[a]);
                        if (s < s_min) {
                          s_min = s;
                          s_min_index = n;
                        }
                      }
                      lambda[n] = l[a];
                      ++a;
                    }
                    else
                      lambda[n] = 0.0;
                  }

                  // if no lambda < 0, proceed:
                  if (!std::isfinite (s_min))
                    break;
#ifdef MRTRIX_ICLS_DEBUG
                l_stream << lambda << "\n";
#endif

                  // remove worst offending lambda from active set, 
                  // and re-estimate remaining lambdas:
                  if (active[s_min_index])
                    active_set_changed = true;
                  active[s_min_index] = false;
                }

                // store feasible subset of lambdas:
                lambda_prev = lambda;


                // update solution vector:
                mult (x, ValueType(1.0), CblasTrans, B_active, l);
                x += y_u;
#ifdef MRTRIX_ICLS_DEBUG
                l_stream << lambda << "\n";
                for (auto a : active)
                  n_stream << a << " ";
                n_stream << "\n";
#endif

                ++niter;
                if (!active_set_changed || niter > P.max_niter) 
                  break;

                // compute constraint values at updated solution:
                mult (c, P.B, x);
              }

              // project back to unconditioned domain:
              solve_triangular (x, P.chol_HtH);
              return niter;
            }

          protected:
            const Problem<ValueType>& P;
            Matrix<ValueType> BtB, B_active;
            Vector<ValueType> y_u, c, c_u, lambda, lambda_prev, l;
            std::vector<bool> active;
        };



    }

    /** @} */
    /** @} */

  }
}

#endif









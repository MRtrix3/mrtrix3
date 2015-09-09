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
#include "math/math.h"

#include <Eigen/Cholesky> 


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

            typedef ValueType value_type;
            typedef Eigen::Matrix<value_type,Eigen::Dynamic,Eigen::Dynamic> matrix_type;
            typedef Eigen::Matrix<value_type,Eigen::Dynamic,1> vector_type;

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
            Problem (const matrix_type& problem_matrix, 
                const matrix_type& constraint_matrix, 
                value_type solution_min_norm_regularisation = 0.0,
                value_type constraint_min_norm_regularisation = 0.0,
                size_t max_iterations = 0,
                value_type tolerance = 0.0) :
              H (problem_matrix),
              chol_HtH (H.cols(), H.cols()), 
              lambda_min_norm (constraint_min_norm_regularisation),
              tol (tolerance),
              max_niter (max_iterations ? max_iterations : 10*problem_matrix.cols()) {

                // form quadratic problem matrix H'*H:
                // TODO: check whether rank update genuinely is faster than
                // general matrix-matrix product - there is a chance it might
                // not be due to e.g. vectorisation...
                chol_HtH.setZero();
                chol_HtH.template selfadjointView<Eigen::Lower>().rankUpdate (H.transpose());
                // add minimum norm constraint:
                chol_HtH.diagonal().array() += solution_min_norm_regularisation * chol_HtH.diagonal().maxCoeff();
                // get Cholesky decomposition:
                chol_HtH.template triangularView<Eigen::Lower>() = chol_HtH.template selfadjointView<Eigen::Lower>().llt().matrixL();

                // form (transpose of) matrix projecting b onto preconditioned
                // quadratic problem chol_HtH\H:
                b2d.noalias() = chol_HtH.template triangularView<Eigen::Lower>().transpose().template solve<Eigen::OnTheRight> (H);

                // project constraint onto preconditioned quadratic domain, 
                // and normalise each row to help preconditioning (the norm of
                // each row is irrelevant to the constraint itself):
                B.noalias() = chol_HtH.template triangularView<Eigen::Lower>().transpose().template solve<Eigen::OnTheRight> (constraint_matrix);
                for (ssize_t n = 0; n < B.rows(); ++n) 
                  B.row(n).normalize();
              }

            matrix_type H, chol_HtH, B, b2d;
            value_type lambda_min_norm, tol;
            size_t max_niter;
        };




      template <typename ValueType>
        class Solver {
          public:

            typedef ValueType value_type;
            typedef Eigen::Matrix<value_type,Eigen::Dynamic,Eigen::Dynamic> matrix_type;
            typedef Eigen::Matrix<value_type,Eigen::Dynamic,1> vector_type;

            Solver (const Problem<value_type>& problem) :
              P (problem),
              BtB (P.chol_HtH.rows(), P.chol_HtH.cols()),
              B_active (P.B.rows(), P.B.cols()),
              y_u (BtB.rows()),
              c (P.B.rows()),
              c_u (P.B.rows()),
              lambda (c.size()),
              lambda_prev (c.size()),
              l (lambda.size()),
              active (lambda.size(), false) { }

            size_t operator() (vector_type& x, const vector_type& b) 
            {
#ifdef MRTRIX_ICLS_DEBUG
              std::ofstream l_stream ("l.txt");
              std::ofstream n_stream ("n.txt");
#endif
              // compute unconstrained solution:
              y_u = P.b2d.transpose() * b;
              // compute constraint violaions for unconstrained solution:
              c_u = P.B * y_u;

              // set all Lagrangian multipliers to zero:
              lambda.setZero();
              lambda_prev.setZero();
              // set active set empty:
              std::fill (active.begin(), active.end(), false);

              // initial estimate of constraint values:
              c = c_u;
              // initial estimate of solution:
              x = y_u;

              size_t min_c_index;
              size_t niter = 0;

              while (c.minCoeff (&min_c_index) < -P.tol) {
                bool active_set_changed = !active[min_c_index];
                active[min_c_index] = true;

                while (1) {
                  // form submatrix of active constraints:
                  B_active.resizeLike (P.B);
                  l.resize (P.B.rows());
                  size_t num_active = 0;
                  for (size_t n = 0; n < active.size(); ++n) {
                    if (active[n]) {
                      B_active.row (num_active) = P.B.row (n);
                      l[num_active] = -c_u[n];
                      ++num_active;
                    }
                  }
                  B_active.conservativeResize (num_active, Eigen::NoChange);
                  l.conservativeResize (num_active);

                  BtB.resize (num_active, num_active);
                  // solve for l in B*B'l = -c_u by Cholesky decomposition:
                  BtB.setZero();
                  BtB.template selfadjointView<Eigen::Lower>().rankUpdate (B_active);
                  BtB.diagonal().array() += P.lambda_min_norm;
                  BtB.template selfadjointView<Eigen::Lower>().llt().solveInPlace (l);

                  // update lambda values in full vector 
                  // and identify worst offender if any lambda < 0
                  // by projection from previous onto feasible 
                  // subset (i.e. l>=0):
                  value_type s_min = std::numeric_limits<value_type>::infinity();
                  size_t s_min_index = 0;
                  size_t a = 0;
                  for (size_t n = 0; n < active.size(); ++n) {
                    if (active[n]) {
                      if (l[a] < 0.0) {
                        value_type s = lambda_prev[n] / (lambda_prev[n] - l[a]);
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
                x = y_u + B_active.transpose() * l;
#ifdef MRTRIX_ICLS_DEBUG
                l_stream << lambda << "\n";
                for (const auto& a : active)
                  n_stream << a << " ";
                n_stream << "\n";
#endif

                ++niter;
                if (!active_set_changed || niter > P.max_niter) 
                  break;

                // compute constraint values at updated solution:
                c = P.B * x;
              }

              // project back to unconditioned domain:
              P.chol_HtH.template triangularView<Eigen::Lower>().transpose().solveInPlace (x);
              return niter;
            }

          protected:
            const Problem<value_type>& P;
            matrix_type BtB, B_active;
            vector_type y_u, c, c_u, lambda, lambda_prev, l;
            std::vector<bool> active;
        };



    }

    /** @} */
    /** @} */

  }
}

#endif









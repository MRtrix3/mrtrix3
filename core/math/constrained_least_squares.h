/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
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





    //! functionality for solving constrained least-squares problems
    /*! the problem is to solve norm(\e Hx - \e b) subject to \e Ax >=
     * t and optionally \e Bx = s. This sets up a class to be re-used and
     * shared across threads, assuming the matrices \e H, \e A & \e B and
     * vectors \e t & \e s don't change, but the vector \e b does.
     *
     * The arguments passed to the Problem constructor correspond to:
     * - \e H - \a problem_matrix
     * - \e A - \a inequality_constraint_matrix
     * - \e B - \a equality_constraint_matrix
     * - \e b - \a problem_vector
     * - \e t - \a inequality_constraint_vector
     * - \e s - \a equality_constraint_vector
     *
     * It is also possible to apply an additional minimum norm constraint that
     * will be added to the problem cost function, to stablise ill-posed
     * problems, and/or a separate minimum norm constraint on the Lagrangian
     * multipliers to handle cases where they become degenerate (otherwise
     * leads to errors in the Cholesky decomposition). Both default to zero,
     * set one or both of them to a small value such as 1e-10 to help with
     * these kinds of problem.

     * It is also possible to pass the problem already in standard form,
     * whereby the matrix provided as \a problem_matrix is assumed to contain
     * \e H<sup>T</sup>H (only its lower triangular part is taken into
     * account), while the vectors provided to the solver will be assumed to
     * contain \e H<sup>T</sup>b.
     */
    namespace ICLS {

      template <typename ValueType>
        class Problem { MEMALIGN(Problem<ValueType>)
          public:

            using value_type = ValueType;
            using matrix_type = Eigen::Matrix<value_type,Eigen::Dynamic,Eigen::Dynamic>;
            using vector_type = Eigen::Matrix<value_type,Eigen::Dynamic,1>;

            Problem () { }
            //! set up constrained least-squares problem
            /*! With this constructor, equality constraints (if present) are
             * assumed to have been included as the last \e N rows of the
             * constraint matrix, and the number of equality constraints is
             * passed as the \a num_equalities argument.
             *
             * If omitted, the \a inequality_constraint_vector defaults to a
             * vector of zeros.
             */
            Problem (const matrix_type& problem_matrix,
                const matrix_type& inequality_constraint_matrix,
                const vector_type& inequality_constraint_vector = vector_type(),
                size_t num_equalities = 0,
                value_type solution_min_norm_regularisation = 0.0,
                value_type constraint_min_norm_regularisation = 0.0,
                size_t max_iterations = 0,
                value_type tolerance = 0.0,
                bool problem_in_standard_form = false) :
              H (problem_matrix),
              chol_HtH (H.cols(), H.cols()),
              t (inequality_constraint_vector),
              lambda_min_norm (constraint_min_norm_regularisation),
              tol (tolerance),
              max_niter (max_iterations ? max_iterations : 10*problem_matrix.cols()),
              num_eq (num_equalities) {

                if (H.cols() != inequality_constraint_matrix.cols())
                  throw Exception ("FIXME: dimensions of problem and constraint matrices do not match (ICLS)");

                if (solution_min_norm_regularisation < 0.0)
                  throw Exception ("FIXME: solution norm regularisation is negative (ICLS)");

                if (lambda_min_norm < 0.0)
                  throw Exception ("FIXME: constraint norm regularisation is negative (ICLS)");

                if (tolerance < 0.0)
                  throw Exception ("FIXME: tolerance is negative (ICLS)");

                if (t.size() && t.size() != inequality_constraint_matrix.rows())
                  throw Exception ("FIXME: dimensions of constraint matrix and vector do not match (ICLS)");

                if (problem_in_standard_form) {
                  chol_HtH = H;
                }
                else {
                  // form quadratic problem matrix H'*H:
                  chol_HtH.setZero();
                  chol_HtH.template triangularView<Eigen::Lower>() = H.transpose() * H;
                }
                // add minimum norm constraint:
                chol_HtH.diagonal().array() += solution_min_norm_regularisation * chol_HtH.diagonal().maxCoeff();
                // get Cholesky decomposition:
                chol_HtH.template triangularView<Eigen::Lower>() = chol_HtH.template selfadjointView<Eigen::Lower>().llt().matrixL();

                // form (transpose of) matrix projecting b onto preconditioned
                // quadratic problem chol_HtH\H:
                if (problem_in_standard_form)
                  b2d.noalias() = chol_HtH.template triangularView<Eigen::Lower>().transpose().template solve<Eigen::OnTheRight> (Eigen::MatrixXd::Identity (H.rows(),H.cols()));
                else
                  b2d.noalias() = chol_HtH.template triangularView<Eigen::Lower>().transpose().template solve<Eigen::OnTheRight> (H);

                // project constraint onto preconditioned quadratic domain,
                B.noalias() = chol_HtH.template triangularView<Eigen::Lower>().transpose().template solve<Eigen::OnTheRight> (inequality_constraint_matrix);
                for (ssize_t n = 0; n < B.rows(); ++n) {
                  double norm = B.row(n).norm();
                  B.row(n) /= norm;
                  if (t.size())
                    t[n] /= norm;
                }

              }

            //! set up constrained least-squares problem
            /*! With this constructor, equality constraints are passed as a
             * distinct \a equality_constraint_matrix along with the
             * corresponding \a equality_constraint_vector.
             *
             * Note that if either \a inequality_constraint_vector or \a
             * equality_constraint_vector are specified, \e both must be
             * specified.
             */
            Problem (const matrix_type& problem_matrix,
                const matrix_type& inequality_constraint_matrix,
                const matrix_type& equality_constraint_matrix,
                const vector_type& inequality_constraint_vector = vector_type(),
                const vector_type& equality_constraint_vector = vector_type(),
                value_type solution_min_norm_regularisation = 0.0,
                value_type constraint_min_norm_regularisation = 0.0,
                size_t max_iterations = 0,
                value_type tolerance = 0.0,
                bool problem_in_standard_form = false) :
              Problem (problem_matrix,
                  concat (inequality_constraint_matrix, equality_constraint_matrix),
                  concat (inequality_constraint_vector, equality_constraint_vector),
                  equality_constraint_matrix.rows(),
                  solution_min_norm_regularisation,
                  constraint_min_norm_regularisation,
                  max_iterations,
                  tolerance,
                  problem_in_standard_form) {
                if (equality_constraint_vector.size() || inequality_constraint_vector.size()) {
                  if (ssize_t(num_eq) != equality_constraint_vector.size())
                    throw Exception ("FIXME: dimensions of equality constraint matrix and vector do not match (ICLS)");
                }
              }


            size_t num_parameters () const { return H.cols(); }
            size_t num_measurements () const { return H.rows(); }
            size_t num_constraints () const { return B.rows(); }
            size_t num_equalities () const { return num_eq; }

            matrix_type H, chol_HtH, B, b2d;
            vector_type t;
            value_type lambda_min_norm, tol;
            size_t max_niter, num_eq;

            static inline matrix_type concat (const matrix_type& a, const matrix_type& b) {
              matrix_type c (a.rows()+b.rows(), a.cols());
              c.topRows (a.rows()) = a;
              c.bottomRows (b.rows()) = b;
              return c;
            }

            static inline vector_type concat (const vector_type& a, const vector_type& b) {
              vector_type c (a.size()+b.size());
              c.head (a.size()) = a;
              c.tail (b.size()) = b;
              return c;
            }
        };




      template <typename ValueType>
        class Solver { MEMALIGN(Solver<ValueType>)
          public:

            using value_type = ValueType;
            using matrix_type = Eigen::Matrix<value_type,Eigen::Dynamic,Eigen::Dynamic>;
            using vector_type = Eigen::Matrix<value_type,Eigen::Dynamic,1>;

            Solver (const Problem<value_type>& problem) :
              P (problem),
              BtB (P.chol_HtH.rows(), P.chol_HtH.cols()),
              B (P.B.rows(), P.B.cols()),
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
              // compute constraint violations for unconstrained solution:
              c_u = P.B * y_u;
              if (P.t.size())
                c_u -= P.t;

              const size_t num_eq = P.num_equalities();
              const size_t num_ineq = P.num_constraints() - num_eq;

              // set all Lagrangian multipliers to zero:
              lambda.setZero();
              lambda_prev.setZero();
              // set active set empty:
              std::fill (active.begin(), active.end(), false);
              if (num_eq > 0)
                std::fill (active.begin() + num_ineq, active.end(), true);

              // initial estimate of constraint values:
              c = c_u;

              // initial estimate of solution:
              x = y_u;

              size_t min_c_index;
              size_t niter = 0;

              while (c.head(num_ineq).minCoeff (&min_c_index) < -P.tol) {
                bool active_set_changed = !active[min_c_index];
                active[min_c_index] = true;

                while (1) {
                  // form submatrix of active constraints:
                  size_t num_active = 0;
                  for (size_t n = 0; n < active.size(); ++n) {
                    if (active[n]) {
                      B.row (num_active) = P.B.row (n);
                      l[num_active] = -c_u[n];
                      ++num_active;
                    }
                  }
                  auto B_active = B.topRows (num_active);
                  auto l_active = l.head (num_active);

                  BtB.resize (num_active, num_active);
                  // solve for l in B*B'l = -c_u by Cholesky decomposition:
                  BtB.template triangularView<Eigen::Lower>() = B_active * B_active.transpose();
                  BtB.diagonal().array() += P.lambda_min_norm;
                  BtB.template selfadjointView<Eigen::Lower>().llt().solveInPlace (l_active);

                  // update lambda values in full vector
                  // and identify worst offender if any lambda < 0
                  // by projection from previous onto feasible
                  // subset (i.e. l>=0):
                  value_type s_min = std::numeric_limits<value_type>::infinity();
                  size_t s_min_index = 0;
                  size_t a = 0;
                  for (size_t n = 0; n < num_ineq; ++n) {
                    if (active[n]) {
                      if (l_active[a] < 0.0) {
                        value_type s = lambda_prev[n] / (lambda_prev[n] - l_active[a]);
                        if (s < s_min) {
                          s_min = s;
                          s_min_index = n;
                        }
                      }
                      lambda[n] = l_active[a];
                      ++a;
                    }
                    else
                      lambda[n] = 0.0;
                  }

                  // if no lambda < 0, proceed:
                  if (!std::isfinite (s_min)) {
                    // update solution vector:
                    x = y_u + B_active.transpose() * l_active;
                    break;
                  }
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
                if (P.t.size())
                  c -= P.t;
              }

              // project back to unconditioned domain:
              P.chol_HtH.template triangularView<Eigen::Lower>().transpose().solveInPlace (x);
              return niter;
            }

            const Problem<value_type>& problem () const { return P; }

          protected:
            const Problem<value_type>& P;
            matrix_type BtB, B;
            vector_type y_u, c, c_u, lambda, lambda_prev, l;
            vector<bool> active;
        };



    }

    /** @} */
    /** @} */

  }
}

#endif









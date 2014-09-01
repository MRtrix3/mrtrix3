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
            Problem (const Matrix<ValueType>& problem_matrix, const Matrix<ValueType>& constraint_matrix, 
                ValueType initial_constraint_factor = ValueType(1.0), ValueType constraint_multiplier = ValueType(2.0), 
                ValueType final_constraint_factor = ValueType (1.0e8), ValueType tolerance = ValueType(1.0e-8), 
                size_t max_iterations = 1000) :
              H (problem_matrix),
              A (constraint_matrix),
              Q (H.columns(), H.columns()), 
              lambda_init (initial_constraint_factor), 
              lambda_inc (constraint_multiplier),
              lambda_max (final_constraint_factor),
              tol (tolerance), 
              max_niter (max_iterations) {
                rankN_update (Q, H, CblasTrans, CblasLower);
              }
            
            Matrix<ValueType> H, A, Q;
            ValueType lambda_init, lambda_inc, lambda_max, tol;
            size_t max_niter;
      };





      template <typename ValueType>
        class Solver {
          public:
            Solver (const Problem<ValueType>& problem) :
              P (problem),
              QA (P.Q.rows(), P.Q.columns()),
              Ak (P.A.rows(), P.A.columns()),
              d (P.Q.rows()),
              c (P.A.rows()) {
                k.reserve (c.size());
                k_prev.reserve (c.size());
              }

            void operator() (Vector<ValueType>& x, const Vector<ValueType>& b) 
            {
              lambda = P.lambda_init;
              mult (d, ValueType (1.0), CblasTrans, P.H, b);
              x.allocate (d.size());

              QA = P.Q;
              solve(x);

              size_t n = 0;
              for (; n < P.max_niter; ++n) {
                if (active_set_unchanged (x)) {
                  lambda *= P.lambda_inc;
                  if (lambda > P.lambda_max) 
                    break;
                } 
                form_constrained_matrix ();
                solve (x);
              }

              if (n >= P.max_niter)
                throw Exception ("constrained least-squares failed to converge");
            }

          protected:
            const Problem<ValueType>& P;
            Matrix<ValueType> QA, Ak;
            Vector<ValueType> d, c;
            std::vector<size_t> k, k_prev;
            ValueType lambda;

            bool active_set_unchanged (Math::Vector<ValueType>& x) 
            { 
              compute_active_set(x);
              if (k.size() == k_prev.size())
                if (std::equal (k.begin(), k.end(), k_prev.begin()))
                  return true;
              k_prev = k;
              return false;
            };

            void compute_active_set (Math::Vector<ValueType>& x) 
            {
              k.clear();
              mult (c, P.A, x);
              for (size_t n = 0; n < c.size(); ++n)
                if (c[n] < P.tol)
                  k.push_back (n);
            }

            void form_constrained_matrix () 
            {
              Ak.allocate (k.size(), P.A.columns());
              for (size_t n = 0; n < k.size(); ++n)
                Ak.row(n) = P.A.row(k[n]);

              QA = P.Q;
              rankN_update (QA, Ak, CblasTrans, CblasLower, lambda, ValueType(1.0));
            }

            void solve (Math::Vector<ValueType>& x) 
            {
              Cholesky::decomp (QA);
              Cholesky::solve (x, QA, d);
            }
        };

    }

    /** @} */
    /** @} */

  }
}

#endif









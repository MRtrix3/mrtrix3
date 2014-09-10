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

//#include "math/cholesky.h"
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
                ValueType initial_constraint_factor = ValueType(1.0), 
                ValueType constraint_multiplier = ValueType(5.0), 
                ValueType tolerance = ValueType(1.0e-10), 
                size_t max_iterations = 1000) :
              H (problem_matrix),
              A (constraint_matrix),
              HtH (H.columns(), H.columns()), 
              mu_init (initial_constraint_factor), 
              mu_inc (constraint_multiplier),
              tol2 (pow2 (tolerance)), 
              max_niter (max_iterations) {
                //rankN_update (HtH, H, CblasTrans, CblasLower);
                mult (HtH, ValueType(0.0), ValueType(1.0), CblasTrans, H, CblasNoTrans, H);
              }

            Matrix<ValueType> H, A, HtH;
            ValueType mu_init, mu_inc, tol2;
            size_t max_niter;
        };




      template <typename ValueType>
        class Solver {
          public:
            Solver (const Problem<ValueType>& problem) :
              iter(0),
              P (problem),
              HtH_muAtA (P.HtH.rows(), P.HtH.columns()),
              Ak (P.A.rows(), P.A.columns()),
              dx (P.HtH.rows()),
              d (P.HtH.rows()),
              c (P.A.rows()),
              lambda (c.size()),
              lambda_k (lambda.size()) {
                k.reserve (c.size());
              }

            int operator() (Vector<ValueType>& x, const Vector<ValueType>& b) 
            {
              lambda = 0.0;
              mu = P.mu_init;
              mu_inc = P.mu_inc;
              mult (d, ValueType (1.0), CblasTrans, P.H, b);
              x = d;

              HtH_muAtA = P.HtH;
              solve (x, HtH_muAtA);
              mult (c, P.A, x);
              //std::cout << x << std::endl;

              iter = 0;
              for (; iter < P.max_niter; ++iter) {
                find_active_set();
                form_constrained_matrix (x);
                solve (x, HtH_muAtA);
                //std::cout << x << std::endl;

                dx -= x;
                ValueType t = norm2 (dx) / norm2 (x);
                if (t < P.tol2)
                  return iter;

                mult (c, P.A, x);
                update_mu_lambda();
                dx = x;
              }
              throw Exception ("constrained least-squares failed to converge");
            }

            size_t iterations () const {
              return iter;
            }

          protected:
            size_t iter;
            const Problem<ValueType>& P;
            Matrix<ValueType> HtH_muAtA, Ak;
             Vector<ValueType> dx, d, c, lambda, lambda_k;
            std::vector<size_t> k;
            ValueType mu, mu_inc;

            static void solve (Vector<ValueType>& x, Matrix<ValueType>& M)
            {
              Permutation p (M.rows());
              int signum;
              LU::decomp (M, p, signum);
              LU::solve (x, M, p);
            }

            void find_active_set ()
            {
              k.clear();
              for (size_t n = 0; n < c.size(); ++n)
                if (c[n] - mu*lambda[n] <= 0.0)
                  k.push_back (n);
            }

            void form_constrained_matrix (Vector<ValueType>& x) 
            {
              Ak.allocate (k.size(), P.A.columns());
              for (size_t n = 0; n < k.size(); ++n)
                Ak.row(n) = P.A.row(k[n]);

              lambda_k.allocate (k.size());
              for (size_t n = 0; n < k.size(); ++n)
                lambda_k[n] = lambda[k[n]];

              HtH_muAtA = P.HtH;
              //rankN_update (HtH_muAtA, Ak, CblasTrans, CblasLower, mu, ValueType(1.0));
              mult (HtH_muAtA, ValueType(1.0), mu, CblasTrans, Ak, CblasNoTrans, Ak);

              x = d;
              mult (x, ValueType(1.0), ValueType(1.0), CblasTrans, Ak, lambda_k);
            }

            void update_mu_lambda () 
            {
              ValueType max_c (0.0), max_lambda (0.0);
              for (size_t n = 0; n < c.size(); ++n ) {
                if (c[n] < max_c) 
                  max_c = c[n];
                if (lambda[n] > max_lambda)
                  max_lambda = lambda[n];
              }
              if (max_lambda > 0.0 && -mu*max_c > max_lambda) {
                mu /= P.mu_inc; 
                mu_inc = 1.0 + (mu_inc-1.0)/2.0;
              }
              else {
                for (size_t n = 0; n < c.size(); ++n) 
                  lambda[n] = std::max (ValueType(0.0), lambda[n] - mu * c[n]);
                mu *= mu_inc;
              }
            }

        };

    }

    /** @} */
    /** @} */

  }
}

#endif









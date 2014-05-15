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
            Problem (const Matrix<ValueType>& problem_matrix, const Matrix<ValueType>& constraint_matrix) :
              problem_matrix (problem_matrix),
              problem_matrix_decomp (problem_matrix.columns(), problem_matrix.columns()),
              transformed_problem_matrix (problem_matrix.columns(), problem_matrix.rows()),
              transformed_constraint_matrix (constraint_matrix.columns(), constraint_matrix.rows()) {
                rankN_update (problem_matrix_decomp, problem_matrix, CblasTrans, CblasLower);
                Cholesky::decomp (problem_matrix_decomp);

                transformed_problem_matrix = problem_matrix;
                solve_triangular (problem_matrix_decomp, transformed_problem_matrix, CblasRight);

                transformed_constraint_matrix = constraint_matrix;
                solve_triangular (problem_matrix_decomp, transformed_constraint_matrix, CblasRight);
              }
            
            Matrix<ValueType> problem_matrix, problem_matrix_decomp;
            Matrix<ValueType> transformed_problem_matrix, transformed_constraint_matrix;
      };





      template <typename ValueType>
        class Solver {
          public:
            Solver (const Problem<ValueType>& problem) :
              problem (problem),
              transformed_solution (problem.problem_matrix_decomp.rows()),
              transformed_unconstrained_solution (problem.problem_matrix_decomp.rows()),
              lambda (transformed_solution.size()),
              constraint_values (problem.transformed_constraint_matrix.rows()),
              active_constraint_matrix (transformed_solution.size(), transformed_solution.size()),
              active_constraint_decomp (transformed_solution.size(), transformed_solution.size()) { }

            void operator() (Vector<ValueType>& x, const Vector<ValueType>& b) {
              active_constraints.clear();
              previous_active_constraints.clear();
              mult (transformed_unconstrained_solution, ValueType (1.0), CblasTrans, problem.transformed_problem_matrix, b);
              x = transformed_unconstrained_solution;

              do {
                add_active_constraints (x);
                if (active_constraints.empty()) 
                  break;

                do {
                  form_active_constraint_matrix ();
                  get_constraint_coefficients (x);
                } while (remove_inactive_constraints());

                mult (x, ValueType(1.0), CblasTrans, active_constraint_matrix, lambda);
                x += transformed_unconstrained_solution;

              } while (active_constraints_changed());

              solve_triangular (x, problem.problem_matrix_decomp);
            }

          protected:
            const Problem<ValueType>& problem;
            Vector<ValueType> transformed_solution, transformed_unconstrained_solution, lambda, constraint_values;
            Matrix<ValueType> active_constraint_matrix, active_constraint_decomp;
            std::set<size_t> active_constraints, previous_active_constraints;

            void add_active_constraints (const Vector<ValueType>& x) {
              mult (constraint_values, problem.transformed_constraint_matrix, x);
              for (size_t n = 0; n < constraint_values.size() && active_constraints.size() < x.size(); ++n) {
                if (constraint_values[n] < 0.0)
                  active_constraints.insert (n);
              }
            }

            void form_active_constraint_matrix () {
              active_constraint_matrix.allocate (active_constraints.size(), transformed_unconstrained_solution.size());
              size_t n = 0;
              for (typename std::set<size_t>::const_iterator i = active_constraints.begin(); i != active_constraints.end(); ++i)
                active_constraint_matrix.row (n++) = problem.transformed_constraint_matrix.row (*i);
            }

            void get_constraint_coefficients (Vector<ValueType>& x) {
              mult (lambda, ValueType(-1.0), CblasNoTrans, active_constraint_matrix, transformed_unconstrained_solution);
              active_constraint_decomp.allocate (lambda.size(), lambda.size());
              rankN_update (active_constraint_decomp, active_constraint_matrix, CblasNoTrans, CblasLower);
              Cholesky::decomp (active_constraint_decomp);
              Cholesky::solve (lambda, active_constraint_decomp);
            }

            bool remove_inactive_constraints () {
              size_t previous_num_active_constraints = active_constraints.size();
              typename std::set<size_t>::iterator i = active_constraints.begin(); 
              for (size_t n = 0; n < lambda.size(); ++n) {
                if (lambda[n] <= 0.0) {
                  typename std::set<size_t>::iterator del = i;
                  ++i;
                  active_constraints.erase (del);
                }
                else 
                  ++i;
              }
              return active_constraints.size() != previous_num_active_constraints;;
            }

            bool active_constraints_changed () {
              if (active_constraints.size() == previous_active_constraints.size())
                if (std::equal (active_constraints.begin(), active_constraints.end(), previous_active_constraints.begin()))
                  return false;
              previous_active_constraints = active_constraints;
              return true;
            }


            void print_set (const std::set<size_t>& set) {
              std::cout << "[ ";
              for (typename std::set<size_t>::const_iterator i = set.begin(); i != set.end(); ++i)
                std::cout << *i << " ";
              std::cout << "]\n";
            }
        };

    }

    /** @} */
    /** @} */

  }
}

#endif









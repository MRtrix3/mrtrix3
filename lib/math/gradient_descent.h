/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 12/01/09.

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

#ifndef __math_gradient_descent_h__
#define __math_gradient_descent_h__

#include <limits>

#include "math/vector.h"

namespace MR
{
  namespace Math
  {

		//! \addtogroup Optimisation
		// @{

		//! Computes the minimum of a function using a gradient decent approach.
    template <class F, typename T = float, bool verbose = false> class GradientDescent
    {
      public:
        GradientDescent (F& function, T step_size_upfactor = 1.5, T step_size_downfactor = 0.1) :
          func (function),
          step_up (step_size_upfactor),
          step_down (step_size_downfactor),
          x (func.size()),
          x2 (func.size()),
          g (func.size()),
          g2 (func.size()) { }


        T                value () const throw ()     {
          return (f);
        }
        const Vector<T>& state () const throw ()     {
          return (x);
        }
        const Vector<T>& gradient () const throw ()  {
          return (g);
        }

        T   gradient_norm () const throw ()        {
          return (normg);
        }
        int function_evaluations () const throw () {
          return (nfeval);
        }


        void run (const int max_iterations = 1000, const T grad_tolerance = 1e-4) {
          init();

          T gradient_tolerance = grad_tolerance * gradient_norm();

          for (int niter = 0; niter < max_iterations; niter++) {
            bool retval = iterate();
            T grad_norm = gradient_norm();
            error ("iteration " + str (niter) + ": f = " + str (f) + ", |g| = " + str (grad_norm));
            for (size_t n = 0; n < x.size(); ++n)
              std::cout << x[n] << " ";
            std::cout << "\n";

            if (!retval)
              return;

            if (grad_norm < gradient_tolerance)
              return;
          }
//          throw Exception ("failed to converge");
        }


        void init () {
          dt = func.init (x);
          nfeval = 0;
          f = evaluate_func (x, g);
          normg = norm (g);
          assert (finite (f));
          assert (finite (normg));
        }


        bool iterate () {
          assert (normg != 0.0);
          T step = dt / normg;
          T f2;

          while (true) {
            bool no_change = true;
            for (size_t n = 0; n < func.size(); n++) {
              x2[n] = x[n] - step * g[n];
              if (x2[n] != x[n])
                no_change = false;
            }
            if (no_change)
              return false;

            f2 = evaluate_func (x2, g2);

            if (f2 < f) {
              dt *= step_up;
              f = f2;
              x.swap (x2);
              g.swap (g2);
              normg = norm (g);
              return true;
            }
            // quadratic minimum

            T denom = 2.0 * (f2 - f + normg*normg*step);
            if (denom) {
              T step_mult = step * normg * normg / denom;
              assert (step_mult > 0.0 && step_mult < 1.0);
              step *= step_mult;
            }
            else step *= 2.0;

            dt *= step_down;
          }
        }

      protected:
        F& func;
        const T step_up, step_down;
        Vector<T> x, x2, g, g2;
        T f, dt, normg;
        int nfeval;

        T evaluate_func (const Vector<T>& newx, Vector<T>& newg) {
          nfeval++;
          T cost = func (newx, newg);
          if (verbose)
            error ("gradient descent evaluation " + str(nfeval) + ", cost function " +str (cost));
          return cost;
        }
    };
    //! @}
  }
}

#endif


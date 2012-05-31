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

    //! Computes the minimum of a function using a gradient descent approach.
    template <class Function, bool verbose = false>
      class GradientDescent
      {
        public:
          typedef typename Function::value_type value_type;

          GradientDescent (Function& function, value_type step_size_upfactor = 1.5, value_type step_size_downfactor = 0.1) :
            func (function),
            step_up (step_size_upfactor),
            step_down (step_size_downfactor),
            x (func.size()),
            x2 (func.size()),
            g (func.size()),
            g2 (func.size()) { }



          value_type value () const throw () { return f; }
          const Vector<value_type>& state () const throw () { return x; }
          const Vector<value_type>& gradient () const throw ()  { return g; }
          value_type gradient_norm () const throw () { return normg; }
          int function_evaluations () const throw () { return nfeval; }

          void run (const int max_iterations = 1000, const value_type grad_tolerance = 1e-4) {
            init();

            value_type gradient_tolerance = grad_tolerance * gradient_norm();

            for (int niter = 0; niter < max_iterations; niter++) {
              bool retval = iterate();
              value_type grad_norm = gradient_norm();
              if (verbose) {
                console ("iteration " + str (niter) + ": f = " + str (f) + ", |g| = " + str (grad_norm));
                for (size_t n = 0; n < x.size(); ++n)
                  std::cout << x[n] << " ";
                std::cout << "\n";
              }

              if (!retval)
                return;

              if (grad_norm < gradient_tolerance)
                return;
            }
            // throw Exception ("failed to converge");
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
            value_type step = dt / normg;
            value_type f2;

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

              value_type denom = 2.0 * (f2 - f + normg*normg*step);
              if (denom) {
                value_type step_mult = step * normg * normg / denom;
                assert (step_mult > 0.0 && step_mult < 1.0);
                step *= step_mult;
              }
              else step *= 2.0;

              dt *= step_down;
            }
          }

        protected:
          Function& func;
          const value_type step_up, step_down;
          Vector<value_type> x, x2, g, g2;
          value_type f, dt, normg;
          int nfeval;

          value_type evaluate_func (const Vector<value_type>& newx, Vector<value_type>& newg) {
            nfeval++;
            value_type cost = func (newx, newg);
            if (!finite (cost))
              throw Exception ("cost function is NaN or Inf!");
            if (verbose)
              console ("gradient descent evaluation " + str(nfeval) + ", cost function " +str (cost));
            return cost;
          }
      };
    //! @}
  }
}

#endif


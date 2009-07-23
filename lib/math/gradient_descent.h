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

namespace MR {
  namespace Math {
    namespace Optim {

      template <class F, typename T = float> class GradientDescent 
      {
        public:
          GradientDescent (F& function, T step_size_upfactor = 1.5, T step_size_downfactor = 0.1) : 
            func (function), 
            step_up (step_size_upfactor),
            step_down (step_size_downfactor),
            workspace (new T [4*func.size()]),
            x (workspace),
            x2 (workspace + func.size()),
            g (workspace + 2*func.size()), 
            g2 (workspace + 3*func.size()) { }

          ~GradientDescent () { delete [] workspace; }


          T        value () const throw ()                { return (f); }
          const T* state () const throw ()                { return (x); }
          const T* gradient () const throw ()             { return (g); }

          T        gradient_norm () const throw ()        { return (normg); }
          int      function_evaluations () const throw () { return (nfeval); }


          void run (const int max_iterations = 1000, const T grad_tolerance = 1e-4) 
          {
            init();
#ifdef PRINT_STATE
              std::cout << f << " ";
              for (int i = 0; i < func.size(); i++) 
                std::cout << x[i] << " ";
              std::cout << "\n";
#endif
            T gradient_tolerance = grad_tolerance * gradient_norm();

            for (int niter = 0; niter < max_iterations; niter++) {
              if (!iterate()) return;

              T grad_norm = gradient_norm();

              info ("iteration " + str(niter) + ": f = " + str(f) + ", |g| = " + str(grad_norm));
#ifdef PRINT_STATE
              std::cout << f << " ";
              for (int i = 0; i < func.size(); i++) 
                std::cout << x[i] << " ";
              std::cout << "\n";
#endif

              if (grad_norm < gradient_tolerance) return; 
            }
            throw Exception ("failed to converge");
          }


          void init ()
          {
            dt = func.init (x);
            nfeval = 0;
            f = evaluate_func (x, g);
            normg = norm (g, func.size());
          } 


          bool iterate () 
          { 
            T step = dt / normg;
            T f2;

            while (true) { 
              bool no_change = true;
              for (int n = 0; n < func.size(); n++) {
                x2[n] = x[n] - step * g[n];
                if (x2[n] != x[n]) no_change = false;
              }
              if (no_change) return (false);

              f2 = evaluate_func (x2, g2);

              if (f2 < f) {
                dt *= step_up;
                f = f2;
                swap (x, x2);
                swap (g, g2);
                normg = norm (g, func.size());
                return (true);
              }

              // quadratic minimum

              T denom = 2.0 * (f2 - f + normg * step);
              if (denom) {
                T step_mult = step * normg / denom;
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
          T *workspace, *x, *x2, *g, *g2;
          T f, dt, normg;
          int nfeval;

          T evaluate_func (const T* newx, T* newg) { nfeval++; return (func (newx, newg)); }
      };

    }
  }
}

#endif


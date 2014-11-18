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

#ifndef __math_check_gradient_h__
#define __math_check_gradient_h__

#include "debug.h"
#include "datatype.h"
#include "math/vector.h"
#include "math/matrix.h"

namespace MR {
  namespace Math {

    template <class Function>
      void check_function_gradient (
          Function& function, 
          Math::Vector<typename Function::value_type>& x, 
          typename Function::value_type increment, 
          bool show_hessian = false,
          Math::Vector<typename Function::value_type> conditioner = Math::Vector<typename Function::value_type>()) 
      {
        typedef typename Function::value_type value_type;
        const size_t N = function.size();
        Math::Vector<value_type> g (N);

        CONSOLE ("checking gradient for cost function over " + str(N) +
            " parameters of type " + DataType::from<value_type>().specifier());
        value_type step_size = function.init (g);
        CONSOLE ("cost function suggests initial step size = " + str(step_size));
        CONSOLE ("cost function suggests initial position at [ " + str(g) + "]");

        CONSOLE ("checking gradient at position [ " + str(x) + "]:");
        Math::Vector<value_type> g0 (N);
        value_type f0 = function (x, g0);
        CONSOLE ("  cost function = " + str(f0));
        CONSOLE ("  gradient from cost function         = [ " + str(g0) + "]");

        Math::Vector<value_type> g_fd (N);
        Math::Matrix<value_type> hessian;
        if (show_hessian) {
          hessian.allocate (N, N);
          if (conditioner.size()) 
            for (size_t n = 0; n < N; ++n)
              conditioner[n] = std::sqrt(conditioner[n]);
        }

        for (size_t n = 0; n < N; ++n) {
          value_type old_x = x[n];
          value_type inc = increment;
          if (conditioner.size())
            inc *= conditioner[n];

          x[n] += inc;
          value_type f1 = function (x, g);
          if (show_hessian) {
            if (conditioner.size())
              g *= conditioner;
            hessian.column(n) = g;
          }

          x[n] = old_x - inc;
          value_type f2 = function (x, g);
          g_fd[n] = (f1-f2) / (2.0*inc);
          x[n] = old_x;
          if (show_hessian) {
            if (conditioner.size())
              g *= conditioner;
            hessian.column(n) -= g;
          }

        }

        CONSOLE ("gradient by central finite difference = [ " + str(g_fd) + "]");
        CONSOLE ("normalised dot product = " + str(Math::dot (g_fd, g0) / Math::norm2 (g_fd)));

        if (show_hessian) {
          hessian /= 4.0*increment;
          for (size_t j = 0; j < N; ++j) {
            size_t i;
            for (i = 0; i < j; ++i) 
              hessian(i,j) = hessian(j,i);
            for (; i < N; ++i) 
              hessian(i,j) += hessian(j,i);
          }
          CONSOLE ("hessian = [ " + str(hessian) + "]");
        }
      }

  }
}

#endif

/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __math_check_gradient_h__
#define __math_check_gradient_h__

#include <Eigen/SVD>
#include "debug.h"
#include "datatype.h"

namespace MR {
  namespace Math {

    template <class Function>
      Eigen::Matrix<typename Function::value_type, Eigen::Dynamic, Eigen::Dynamic> check_function_gradient (
          Function& function,
          Eigen::Matrix<typename Function::value_type, Eigen::Dynamic, 1> x,
          typename Function::value_type increment,
          bool show_hessian = false,
          Eigen::Matrix<typename Function::value_type, Eigen::Dynamic, 1> conditioner = Eigen::Matrix<typename Function::value_type, Eigen::Dynamic, 1>())
      {
        using value_type = typename Function::value_type;
        const size_t N = function.size();
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> g (N);

        CONSOLE ("checking gradient for cost function over " + str(N) +
            " parameters of type " + DataType::from<value_type>().specifier());
        value_type step_size = function.init (g);
        CONSOLE ("cost function suggests initial step size = " + str(step_size));
        CONSOLE ("cost function suggests initial position at [ " + str(g.transpose()) + "]");

        CONSOLE ("checking gradient at position [ " + str(x.transpose()) + "]:");
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> g0 (N);
        value_type f0 = function (x, g0);
        CONSOLE ("  cost function = " + str(f0));
        CONSOLE ("  gradient from cost function         = [ " + str(g0.transpose()) + "]");

        Eigen::Matrix<value_type, Eigen::Dynamic, 1> g_fd (N);
        Eigen::Matrix<value_type, Eigen::Dynamic, Eigen::Dynamic> hessian;
        if (show_hessian) {
          hessian.resize(N, N);
          if (conditioner.size()){
            assert (conditioner.size() == (ssize_t) N && "conditioner size must equal number of parameters");
            for (size_t n = 0; n < N; ++n)
              conditioner[n] = std::sqrt(conditioner[n]);
          }
        }

        for (size_t n = 0; n < N; ++n) {
          value_type old_x = x[n];
          value_type inc = increment;
          if (conditioner.size()){
            assert (conditioner.size() == (ssize_t) N && "conditioner size must equal number of parameters");
            inc *= conditioner[n];
          }

          x[n] += inc;
          value_type f1 = function (x, g);
          if (show_hessian) {
            if (conditioner.size())
              g.cwiseProduct(conditioner);
            hessian.col(n) = g;
          }

          x[n] = old_x - inc;
          value_type f2 = function (x, g);
          g_fd[n] = (f1-f2) / (2.0*inc);
          x[n] = old_x;
          if (show_hessian) {
            if (conditioner.size())
              g.cwiseProduct(conditioner);
            hessian.col(n) -= g;
          }

        }

        CONSOLE ("gradient by central finite difference = [ " + str(g_fd.transpose()) + "]");
        CONSOLE ("normalised dot product = " + str(g_fd.dot(g0) / g_fd.squaredNorm()));

        if (show_hessian) {
          hessian /= 4.0*increment;
          for (size_t j = 0; j < N; ++j) {
            size_t i;
            for (i = 0; i < j; ++i)
              hessian(i,j) = hessian(j,i);
            for (; i < N; ++i)
              hessian(i,j) += hessian(j,i);
          }
          // CONSOLE ("hessian = [ " + str(hessian) + "]");
          MAT(hessian);
          auto v = Eigen::JacobiSVD<decltype(hessian)> (hessian).singularValues();
          auto conditionnumber = v[0] / v[v.size()-1];
          CONSOLE("\033[00;34mcondition number: " + str(conditionnumber)+"\033[0m");
        }
      return hessian;
      }
  }
}

#endif

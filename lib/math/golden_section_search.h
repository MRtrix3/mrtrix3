/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 09/06/2011

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

#ifndef __math_golden_section_search_h__
#define __math_golden_section_search_h__

#include <cmath>

#include "progressbar.h"
#include "memory.h"

namespace MR
{
  namespace Math
  {
    /** \addtogroup Optimisation
    @{ */

    //! Computes the minimum of a 1D function using a golden section search.
    /*! This function operates on a cost function class that must define a
     *  operator() method. The method must take a single ValueType argument
     *  x and return the cost of the function at x.
     *
     *  The min_bound and max_bound arguments define values that bracket the
     *  expected minimum. The estimate argument is the initial estimate of the
     *  minimum that is required to be larger than min_bound and smaller than max_bound.
     *
     * Typical usage:
     * \code
     * CostFunction cost_function();
     * float optimal_value = Math::golden_section_search(cost_function, "optimising...", min_bound, initial_estimate , max_bound);
     *
     * \endcode
     */

    template <class FunctionType, typename ValueType>
      ValueType golden_section_search (FunctionType& function,
                                       const std::string& message,
                                       ValueType min_bound,
                                       ValueType init_estimate,
                                       ValueType max_bound,
                                       ValueType tolerance = 0.01)	{

        std::unique_ptr<ProgressBar> progress (message.size() ? new ProgressBar (message) : nullptr);

        const ValueType g1 = 0.61803399, g2 = 1 - g1;
        ValueType x0 = min_bound, x1, x2, x3 = max_bound;

        if (std::abs(max_bound - init_estimate) > std::abs(init_estimate - min_bound)) {
          x1 = init_estimate;
          x2 = init_estimate + g2 * (max_bound - init_estimate);
        } else {
          x2 = init_estimate;
          x1 = init_estimate - g2 * (init_estimate - min_bound);
        }

        ValueType f1 = function(x1);
        ValueType f2 = function(x2);

        while (tolerance * (std::abs(x1) + std::abs(x2)) < std::abs(x3 - x0)) {
          if (f2 < f1) {
            x0 = x1;
            x1 = x2;
            x2 = g1 * x1 + g2 * x3;
            f1 = f2, f2 = function(x2);
          } else {
            x3 = x2;
            x2 = x1;
            x1 = g1 * x2 + g2 * x0;
            f2 = f1, f1 = function(x1);
          }
          if (progress)
            ++*progress;
        }
        return f1 < f2 ? x1 : x2;
      }
  }
  }

#endif


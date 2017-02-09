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
     * float optimal_value = Math::golden_section_search(cost_function, "optimising", min_bound, initial_estimate , max_bound);
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


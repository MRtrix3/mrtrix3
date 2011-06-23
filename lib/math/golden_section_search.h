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

namespace MR
{
  namespace Math
  {

		/** \addtogroup Optimisation
		@{ */

		//! Computes the minimum of a 1D function using a golden section search.
    /*! This class operates on generic 'cost function' classes that are required to
     *  implement an evaluate_function() method in the form of:
     *  \code
     *  DataType evaluate_function(DataType input);
     *  \endcode
     *
     *  To begin optimisation run() is called that takes 4 arguments.
     *  The min_bound and max_bound arguments define values that bracket the
     *  expected minimum. The estimate argument is the initial estimate of the
     *  minimum that is required to be larger than min_bound and smaller than max_bound.
     *  The terminating condition is defined by the tolerance argument such that the
     *  difference between the updated min and max bounds is smaller than the tolerance.
     *
		 * Typical usage:
		 * \code
		 * CostFunction cost_function();
		 * Math::GoldenSectionSearch<CostFunction,float> golden_search(cost_function, "optimising...");
		 * float optimal_value = golden_search.run(min_bound, initial_estimate , max_bound);
		 *
		 * \endcode
		 */
    template <class Function, typename DataType = float> class GoldenSectionSearch
    {

			public:
				GoldenSectionSearch (Function& function, const std::string& message) : function_(function), progress_(message) { }

				GoldenSectionSearch (Function& function) : function_(function) { }

				DataType run (DataType min_bound, DataType estimate, DataType max_bound, double tolerance = 1E-5)	{
						DataType g1 = 0.61803399, g2 = 1 - g1;
						DataType x0 = min_bound, x1, x2, x3 = max_bound;

						if (std::abs(max_bound - estimate) > std::abs(estimate - min_bound)) {
								x1 = estimate;
								x2 = estimate + g2 * (max_bound - estimate);
						} else {
								x2 = estimate;
								x1 = estimate - g2 * (estimate - min_bound);
						}

						DataType f1 = function_.evaluate_function(x1);
						DataType f2 = function_.evaluate_function(x2);

						while (std::abs(x3 - x0) > tolerance * (std::abs(x1) + std::abs(x2))) {
							if (f2 < f1) {
								x0 = x1;
								x1 = x2;
								x2 = g1 * x1 + g2 * x3;
							  f1 = f2, f2 = function_.evaluate_function(x2);
							} else {
								x3 = x2;
								x2 = x1;
								x1 = g1 * x2 + g2 * x0;
								f2 = f1, f1 = function_.evaluate_function(x1);
							}
						  ++progress_;
						}
						return f1 < f2 ? x1 : x2;
				}

			private:
				Function& function_;
				ProgressBar progress_;
    };
		//! @}
  }
}

#endif


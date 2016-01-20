/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 19/11/12

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

#ifndef __registration_transform_norm_h__
#define __registration_transform_norm_h__

#include "image.h"
#include "algo/threaded_loop.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

        /** \addtogroup Registration
          @{ */

        /*! Compute the norm of a field
         */
        default_type norm (Image<default_type>& disp_field)
        {
          default_type norm = 0.0;

          auto norm_calculator = [&norm](Image<default_type>& disp) {
            norm += std::pow (disp.value(), 2);
          };
          norm = std::sqrt (norm);

          ThreadedLoop (disp_field).run (norm_calculator, disp_field);
          return norm;
        }

      //! @}
    }
  }
}


#endif

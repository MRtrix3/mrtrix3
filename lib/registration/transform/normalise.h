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

#ifndef __registration_transform_normalise_h__
#define __registration_transform_normalise_h__

#include "image.h"
#include "algo/threaded_loop.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

      namespace {

          class MaxMagThreadKernel {

            public:
              MaxMagThreadKernel (default_type& global_max_magnitude) :
                global_max_magnitude (global_max_magnitude),
                local_max_magnitude (0.0) {}

              ~MaxMagThreadKernel () {
                if (local_max_magnitude > global_max_magnitude)
                  global_max_magnitude = local_max_magnitude;
              }

              void operator() (Image<default_type>& disp_field) {
                Eigen::Vector3 vec = disp_field.row(3);
                default_type mag = 0.0;
                for (size_t dim = 0; dim < 3; ++dim)
                  mag += (vec[dim] / disp_field.spacing(dim)) * (vec[dim] / disp_field.spacing(dim));
                mag = std::sqrt (mag);
                if (mag > local_max_magnitude)
                  local_max_magnitude = mag;
              }

            private:
              default_type& global_max_magnitude;
              default_type local_max_magnitude;
          };
      }


      /** \addtogroup Registration
        @{ */

      /*! Normalise a field so that the maximum vector magnitude is 1 voxel (in mm)
       */
      void normalise_field (Image<default_type>& disp_field)
        {
          default_type global_max_magnitude = 0.0;
          ThreadedLoop (disp_field, 0, 3).run (MaxMagThreadKernel (global_max_magnitude), disp_field);
          if (global_max_magnitude == 0)
            return;

          auto normaliser = [&global_max_magnitude](Image<default_type>& disp) {
            Eigen::Vector3 vec = disp.row(3);
            disp.row(3) = vec.array() / global_max_magnitude;
          };

          ThreadedLoop (disp_field, 0, 3).run (normaliser, disp_field);
        }

      /*! Normalise two fields so that the maximum vector magnitude (across both fields) is 1 voxel (in mm)
       */
      void normalise_field (Image<default_type>& disp_field1, Image<default_type>& disp_field2)
        {
          default_type global_max_magnitude = 0.0;
          ThreadedLoop (disp_field1, 0, 3).run (MaxMagThreadKernel (global_max_magnitude), disp_field1);
          ThreadedLoop (disp_field1, 0, 3).run (MaxMagThreadKernel (global_max_magnitude), disp_field2);
          if (global_max_magnitude == 0)
            return;

          auto normaliser = [&global_max_magnitude](Image<default_type>& disp) {
            Eigen::Vector3 vec = disp.row(3);
            disp.row(3) = vec.array() / global_max_magnitude;
          };

          ThreadedLoop (disp_field1, 0, 3).run (normaliser, disp_field1);
          ThreadedLoop (disp_field2, 0, 3).run (normaliser, disp_field2);
        }


      //! @}
    }
  }
}


#endif

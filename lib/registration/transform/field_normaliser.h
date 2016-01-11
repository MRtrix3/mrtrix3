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

#ifndef __registration_transform_field_normaliser_h__
#define __registration_transform_field_normaliser_h__

#include "image.h"
#include "algo/threaded_loop.h"

namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

      typedef float value_type;

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

              template <class FieldType>
                void operator() (FieldType& field) {
                  Eigen::Matrix<typename FieldType::value_type, 3, 1> vec = field.row(3);
                  default_type mag = 0.0;
                  for (size_t dim = 0; dim < 3; ++dim)
                    mag += (vec[dim] / field.spacing(dim)) * (vec[dim] / field.spacing(dim));
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

      /*! Normalise a field so that the maximum vector magnitude is 1
       */
      template <class FieldType>
        void normalise_field (FieldType& field)
        {
          default_type global_max_magnitude = 0.0;
          ThreadedLoop (field, 0, 3).run (MaxMagThreadKernel (global_max_magnitude), field);
          if (global_max_magnitude == 0)
            return;

          std::cout << global_max_magnitude << std::endl;

          auto normaliser = [&global_max_magnitude](FieldType& field) {
            Eigen::Matrix<typename FieldType::value_type, 3, 1> vec = field.row(3);
            field.row(3) = vec.array() / global_max_magnitude;
          };

          ThreadedLoop (field, 0, 3).run (normaliser, field);
        }

      //! @}
    }
  }
}


#endif

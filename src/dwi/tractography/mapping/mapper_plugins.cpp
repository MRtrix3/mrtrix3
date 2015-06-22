/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2014.

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


#include "dwi/tractography/mapping/mapper_plugins.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {






        const Eigen::Vector3f TWIImagePluginBase::get_last_point_in_fov (const std::vector<Eigen::Vector3f>& tck, const bool end) const
        {
          size_t index = end ? tck.size() - 1 : 0;
          const int step = end ? -1 : 1;
          while (interp.scanner (tck[index])) {
            index += step;
            if (index == tck.size())
              return { NaN, NaN, NaN };
          }
          return tck[index];
        }







        void TWIScalarImagePlugin::load_factors (const std::vector<Eigen::Vector3f>& tck, std::vector<float>& factors)
        {
          if (statistic == ENDS_MIN || statistic == ENDS_MEAN || statistic == ENDS_MAX || statistic == ENDS_PROD) {

            // Only the track endpoints contribute
            for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
              const auto endpoint = get_last_point_in_fov (tck, tck_end_index);
              if (endpoint.allFinite())
                factors.push_back (interp.value());
              else
                factors.push_back (NaN);
            }

          } else {

            // The entire length of the track contributes
            for (const auto i : tck) {
              if (!interp.scanner (i))
                factors.push_back (interp.value());
              else
                factors.push_back (NaN);
            }

          }
        }





        void TWIFODImagePlugin::load_factors (const std::vector<Eigen::Vector3f>& tck, std::vector<float>& factors)
        {
          for (size_t i = 0; i != tck.size(); ++i) {
            if (!interp.scanner (tck[i])) {
              // Get the FOD at this (interploated) point
              for (auto l = Loop (3) (interp); l; ++l) 
                sh_coeffs[interp.index(3)] = interp.value();
              // Get the FOD amplitude along the streamline tangent
              const Eigen::Vector3f dir = (tck[(i == tck.size()-1) ? i : (i+1)] - tck[i ? (i-1) : 0]).normalized();
              factors.push_back (precomputer->value (sh_coeffs, dir));
            } else {
              factors.push_back (NaN);
            }
          }
        }






      }
    }
  }
}





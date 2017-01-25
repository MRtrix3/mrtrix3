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



#include "dwi/tractography/mapping/mapper_plugins.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {






        const Streamline<>::point_type TWIImagePluginBase::get_last_point_in_fov (const Streamline<>& tck, const bool end) const
        {
          int index = end ? tck.size() - 1 : 0;
          const int step = end ? -1 : 1;
          while (!interp.scanner (tck[index])) {
            index += step;
            if (index == -1 || index == int(tck.size()))
              return { NaN, NaN, NaN };
          }
          return tck[index];
        }







        void TWIScalarImagePlugin::load_factors (const Streamline<>& tck, std::vector<default_type>& factors)
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
              if (interp.scanner (i))
                factors.push_back (interp.value());
              else
                factors.push_back (NaN);
            }

          }
        }





        void TWIFODImagePlugin::load_factors (const Streamline<>& tck, std::vector<default_type>& factors)
        {
          for (size_t i = 0; i != tck.size(); ++i) {
            if (interp.scanner (tck[i])) {
              // Get the FOD at this (interploated) point
              for (auto l = Loop (3) (interp); l; ++l) 
                sh_coeffs[interp.index(3)] = interp.value();
              // Get the FOD amplitude along the streamline tangent
              const Eigen::Vector3 dir = (tck[(i == tck.size()-1) ? i : (i+1)] - tck[i ? (i-1) : 0]).cast<default_type>().normalized();
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





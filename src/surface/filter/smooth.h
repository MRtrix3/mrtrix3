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


#ifndef __surface_filter_smooth_h__
#define __surface_filter_smooth_h__

#include "surface/mesh.h"
#include "surface/filter/base.h"


namespace MR
{
  namespace Surface
  {
    namespace Filter
    {



      constexpr default_type default_smoothing_spatial_factor = 10.0;
      constexpr default_type default_smoothing_influence_factor = 10.0;



      class Smooth : public Base
      { MEMALIGN (Smooth)
        public:
          Smooth () :
              Base (),
              spatial (default_smoothing_spatial_factor),
              influence (default_smoothing_influence_factor) { }

          Smooth (const std::string& s) :
              Base (s),
              spatial (default_smoothing_spatial_factor),
              influence (default_smoothing_influence_factor) { }

          Smooth (const default_type spatial_factor, const default_type influence_factor) :
              Base (),
              spatial (spatial_factor),
              influence (influence_factor) { }

          Smooth (const std::string& s, const default_type spatial_factor, const default_type influence_factor) :
              Base (s),
              spatial (spatial_factor),
              influence (influence_factor) { }

          void operator() (const Mesh&, Mesh&) const override;

        private:
          default_type spatial, influence;

      };



    }
  }
}


#endif

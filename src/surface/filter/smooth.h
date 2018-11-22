/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
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

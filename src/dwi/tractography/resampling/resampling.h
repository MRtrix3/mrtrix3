/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __dwi_tractography_resampling_resampling_h__
#define __dwi_tractography_resampling_resampling_h__


#include <vector>

#include "dwi/tractography/tracking/generated_track.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Resampling {



        extern const App::OptionGroup ResampleOption;

        class Base;
        Base* get_resampler();

        // cubic interpolation (tension = 0.0) looks 'bulgy' between control points
        constexpr float hermite_tension = 0.1f;



        class Base
        {
          public:
            Base() { }

            virtual bool operator() (std::vector<Eigen::Vector3f>&) const = 0;

            virtual bool valid () const = 0;
            virtual bool limits (const std::vector<Eigen::Vector3f>&) { return true; }

        };




      }
    }
  }
}

#endif




/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __dwi_tractography_sd_parameters_h__
#define __dwi_tractography_sd_parameters_h__

#include "mrtrix.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      class SDParameters {
        public:
          SDParameters () { reset(); }
          void        reset ();
          size_t      lmax, max_trials;
          bool        precomputed;
      };


      inline void SDParameters::reset () 
      {
        lmax = 0;
        precomputed = true;
        max_trials = 50;
      }

    }
  }
}

#endif


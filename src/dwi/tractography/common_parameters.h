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

#ifndef __dwi_tractography_common_parameters_h__
#define __dwi_tractography_common_parameters_h__

#include "point.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      typedef enum {
        DT_STREAM = 0,
        DT_PROB = 1,
        SD_STREAM = 2,
        SD_PROB = 3,
        UNDEFINED = INT_MAX
      } Method;


      class CommonParameters {
        public:
          CommonParameters () { reset(); }

          void        reset ();

          Method      method;
          std::string      cmd, source, mask;
          float       step_size, max_dist, threshold, init_threshold, min_curv, mask_threshold;
          uint32_t     max_num_tracks, max_num_tracks_generated, num_tracks_generated;

          bool        unidirectional;
          Point       init_dir;
      };





      inline void CommonParameters::reset () 
      {
        method = UNDEFINED;
        step_size = max_dist = threshold = init_threshold = min_curv = NAN;
        unidirectional = false;
        max_num_tracks = max_num_tracks_generated = num_tracks_generated = 0;
        init_dir = Point::Invalid;
      }

    }
  }
}

#endif


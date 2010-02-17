/*
    Copyright 2010 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 17/02/10.

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

#ifndef __mrview_camera_h__
#define __mrview_camera_h__

#include "point.h"
#include "math/quaternion.h"

namespace MR {
  namespace Viewer {

    class Camera {
      public:
        Camera () : 
          orientation (NAN, NAN, NAN, NAN), 
          focus (0.0, 0.0, 0.0),
          projection (2),
          interpolate (true),
          FOV (100.0) { }

        Math::Quaternion orientation;
        Point focus;
        int projection;
        bool interpolate;
        float FOV;
    };

  }
}

#endif





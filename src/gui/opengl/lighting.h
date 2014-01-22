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

#ifndef __gui_opengl_lighting_h__
#define __gui_opengl_lighting_h__

#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace GL
    {

      class Lighting : public QObject
      {
          Q_OBJECT

        public:
          Lighting (QObject* parent) : 
            QObject (parent), 
            ambient (0.4), 
            diffuse (0.7), 
            specular (0.3), 
            shine (5.0), 
            set_background (false) {
            load_defaults();
          }

          float ambient, diffuse, specular, shine;
          float light_color[3], object_color[3], lightpos[3], background_color[3];
          bool set_background;

          void  set () const;
          void  load_defaults ();
          void  update () {
            emit changed();
          }

        signals:
          void changed ();
      };

    }
  }
}

#endif




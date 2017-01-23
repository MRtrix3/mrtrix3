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
      { MEMALIGN(Lighting)
          Q_OBJECT

        public:
          Lighting (QObject* parent) : 
            QObject (parent), 
            ambient (0.5),
            diffuse (0.7), 
            specular (0.7),
            shine (0.5),
            set_background (false) {
            load_defaults();
          }

          float ambient, diffuse, specular, shine;
          float light_color[3], lightpos[3], background_color[3];
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




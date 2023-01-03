/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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




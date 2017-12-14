/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __gui_mrview_mode_ortho_h__
#define __gui_mrview_mode_ortho_h__

#include "app.h"
#include "gui/mrview/mode/slice.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        class Ortho : public Slice
        { MEMALIGN(Ortho)
            Q_OBJECT

          public:
              Ortho () : 
                projections (3, projection),
                current_plane (0) { }

            virtual void paint (Projection& projection);

            virtual void mouse_press_event ();
            virtual void slice_move_event (float x);
            virtual void panthrough_event ();
            virtual const Projection* get_current_projection () const;

          protected:
            vector<Projection> projections;
            int current_plane;
            GL::VertexBuffer frame_VB;
            GL::VertexArrayObject frame_VAO;
            GL::Shader::Program frame_program;
        };

      }
    }
  }
}

#endif






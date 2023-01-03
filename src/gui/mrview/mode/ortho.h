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
            Ortho ();
            virtual void paint (Projection& projection);

            virtual void mouse_press_event ();
            virtual void slice_move_event (float x);
            virtual void panthrough_event ();
            virtual const Projection* get_current_projection () const;
            virtual void request_update_mode_gui (ModeGuiVisitor& visitor) const {
                 visitor.update_ortho_mode_gui(*this); }

            static bool show_as_row;

          public slots:
            void set_show_as_row_slot (bool state);

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





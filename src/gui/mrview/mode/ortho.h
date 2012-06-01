/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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

#ifndef __gui_mrview_mode_ortho_h__
#define __gui_mrview_mode_ortho_h__

#include "app.h"
#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        class Ortho : public Base
        {
            Q_OBJECT

          public:
            Ortho (Window& parent);
            virtual ~Ortho ();

            virtual void paint ();

            virtual bool mouse_click ();
            virtual bool mouse_move ();
            virtual bool mouse_release ();
            virtual bool mouse_wheel (float delta, Qt::Orientation orientation);

            virtual void reset_view ();

          public slots:
            virtual void reset ();

          protected:
            void set_cursor ();
            void draw_projection (int proj);
        };

      }
    }
  }
}

#endif






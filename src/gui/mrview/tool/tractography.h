/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier and David Raffelt, 13/11/09.

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

#ifndef __gui_mrview_tool_tractography_h__
#define __gui_mrview_tool_tractography_h__

#include "gui/mrview/tool/base.h"

class QStringListModel;

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class Tractography : public Base
        {
            Q_OBJECT

          public:

            Tractography (Window& main_window, Dock* parent);

            ~Tractography ();

          private slots:
            void tractogram_open_slot ();
            void tractogram_close_slot ();
            void opacity_slot (int opacity);
            void line_thickness_slot (int thickness);
            void on_slab_thickness_change();

          protected:
             class Model;
             Model* tractogram_list_model;
             QListView* tractogram_list_view;
        };

      }
    }
  }
}

#endif





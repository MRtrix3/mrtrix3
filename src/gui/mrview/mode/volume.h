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

#ifndef __gui_mrview_mode_volume_h__
#define __gui_mrview_mode_volume_h__

#include "app.h"
#include "gui/mrview/mode/mode3d.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        class Volume : public Mode3D
        {
          public:
            Volume (Window& parent);
            virtual ~Volume ();

            virtual void paint ();
            virtual Tool::Dock* get_extra_controls ();

          protected:
            Tool::Dock* extra_controls;
        };

      }
    }
  }
}

#endif






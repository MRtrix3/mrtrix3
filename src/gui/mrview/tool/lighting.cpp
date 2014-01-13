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

#include "gui/mrview/tool/lighting.h"
#include "gui/dialog/lighting.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        Lighting::Lighting (Window& main_window, Dock* parent) : 
          Base (main_window, parent)
        {
          VBoxLayout* main_box = new VBoxLayout (this);
          main_box->addWidget (new Dialog::LightingSettings (this, window.lighting(), false));
          main_box->addStretch ();
          setMinimumSize (main_box->minimumSize());
        }



      }
    }
  }
}








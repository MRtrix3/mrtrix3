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

#include "gui/mrview/tool/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        Base::Base (Window& main_window, Dock* parent) : 
          QFrame (parent),
          window (main_window) { 
            QFont f = font();
            f.setPointSize (MR::File::Config::get_int ("MRViewToolFontSize", f.pointSize()-1));
            setFont (f);

            setFrameShadow (QFrame::Sunken); 
            setFrameShape (QFrame::Panel);
          }



        QSize Base::sizeHint () const
        {
          return minimumSizeHint();
        }


        void Base::draw (const Projection&, bool, int, int) { }

        void Base::drawOverlays (const Projection&) { }

        bool Base::process_batch_command (const std::string&, const std::string&) 
        {
          return false;
        }

        bool Base::mouse_press_event () { return false; }
        bool Base::mouse_move_event () { return false; }
        bool Base::mouse_release_event () { return false; }
        QCursor* Base::get_cursor () { return nullptr; }
      }
    }
  }
}




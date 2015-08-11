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

#include "app.h"
#include "gui/mrview/tool/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        void Dock::closeEvent (QCloseEvent*) { assert (tool); tool->close_event(); }

        Base::Base (Dock* parent) : 
          QFrame (parent) {
            QFont f = font();
            //CONF option: MRViewToolFontSize
            //CONF default: 2 points less than the standard system font
            //CONF The point size for the font to use in MRView Tools.
            f.setPointSize (MR::File::Config::get_int ("MRViewToolFontSize", f.pointSize()-2));
            setFont (f);
            setFrameShadow (QFrame::Sunken); 
            setFrameShape (QFrame::Panel);
          }

        void Base::adjustSize () 
        {
          layout()->update();
          layout()->activate();
          setMinimumSize (layout()->minimumSize());
        }


        QSize Base::sizeHint () const { return minimumSize(); }


        void Base::draw (const Projection&, bool, int, int) { }

        void Base::draw_colourbars () { }

        bool Base::mouse_press_event () { return false; }
        bool Base::mouse_move_event () { return false; }
        bool Base::mouse_release_event () { return false; }
        QCursor* Base::get_cursor () { return nullptr; }

        bool Base::process_commandline_option (const MR::App::ParsedOption&) { return false; }
        void Base::add_commandline_options (MR::App::OptionList&) { }

      }
    }
  }
}




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
            //CONF The point size for the font to use in MRView tools.
            f.setPointSize (MR::File::Config::get_int ("MRViewToolFontSize", f.pointSize()-2));
            setFont (f);
            setFrameShadow (QFrame::Sunken); 
            setFrameShape (QFrame::Panel);
            setAcceptDrops (true);
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




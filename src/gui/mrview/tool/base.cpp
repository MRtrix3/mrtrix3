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

        void __Action__::visibility_slot (bool) { setChecked (dock->isVisible()); }


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


        QSize Base::sizeHint () const { return minimumSizeHint(); }

        void Base::draw (const Projection&, bool, int, int) { }

        void Base::draw_colourbars () { }

        bool Base::mouse_press_event () { return false; }
        bool Base::mouse_move_event () { return false; }
        bool Base::mouse_release_event () { return false; }
        QCursor* Base::get_cursor () { return nullptr; }

        bool Base::process_commandline_option (const MR::App::ParsedOption&) { return false; }
        void Base::add_commandline_options (MR::App::OptionList&) { }

        void CameraInteractor::deactivate () { }
        bool CameraInteractor::slice_move_event (const ModelViewProjection&, float) { return false; }
        bool CameraInteractor::pan_event (const ModelViewProjection&) { return false; }
        bool CameraInteractor::panthrough_event (const ModelViewProjection&) { return false; }
        bool CameraInteractor::tilt_event (const ModelViewProjection&) { return false; }
        bool CameraInteractor::rotate_event (const ModelViewProjection&) { return false; }
      }
    }
  }
}




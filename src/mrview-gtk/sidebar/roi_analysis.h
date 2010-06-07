/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __mrview_sidebar_roi_analysis_h__
#define __mrview_sidebar_roi_analysis_h__

#include <gtkmm/checkbutton.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/frame.h>
#include <gtkmm/scale.h>

#include "mrview/sidebar/base.h"
#include "mrview/sidebar/roi_analysis/roi_list.h"

namespace MR {
  namespace Viewer {
    namespace SideBar {

      class ROIAnalysis : public Base
      {
        public:
          ROIAnalysis ();
          ~ROIAnalysis ();

          void draw();
          bool on_button_press (GdkEventButton* event);
          bool on_motion (GdkEventMotion* event);
          bool on_button_release (GdkEventButton* event);

        protected:
          Gtk::CheckButton     show_ROIs;
          Gtk::Frame           roi_frame, transparency_frame;
          Gtk::HScale          transparency;
          Gtk::ScrolledWindow  roi_scrolled_window;
          DP_ROIList           roi_list;

          void  on_change ();
      };

    }
  }
}

#endif


